from pathlib import Path
import re
p=Path('build_mathis_test_workbook.ps1'); s=p.read_text(encoding='utf-8')
s=s.replace("$ErrorActionPreference = 'Stop'",'''$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'tools\\Excel.Common.ps1')
Initialize-ExcelComTracking
$OutputPath = [IO.Path]::GetFullPath($OutputPath)
$PreviewDir = [IO.Path]::GetFullPath($PreviewDir)''',1)
s=s.replace("    $h = $Hex.TrimStart('#')", "    if ($Hex -notmatch '^#?[0-9a-fA-F]{6}$') { throw \"Invalid RGB color: $Hex\" }\n    $h = $Hex.TrimStart('#')")
s=s.replace('''    for ($r = 0; $r -lt $rows.Count; $r++) {
        for ($c = 0; $c -lt 13; $c++) {
            $sheet.Cells.Item($startRow + $r, $c + 1).Value2 = $rows[$r][$c]
        }
    }''','''    if ($rows.Count -eq 0) { throw "No test cases for worksheet $name." }
    $values = [object[,]]::new($rows.Count, 13)
    for ($r = 0; $r -lt $rows.Count; $r++) {
        if ($rows[$r].Count -ne 13) { throw "Test row $r must contain 13 columns." }
        for ($c = 0; $c -lt 13; $c++) { $values[$r,$c] = $rows[$r][$c] }
    }
    Set-ExcelMatrix $sheet $startRow 1 $values''')
s=s.replace('$workbook = $null\ntry {','$workbook = $null\n$checkBook = $null\ntry {')
s=s.replace('    $checkBook.Close($false)\n    $excel.Quit()\n    $excel = $null','    $checkBook.Close($false)\n    $checkBook = $null')
start=s.rfind('finally {')
s=s[:start]+'''finally {
    Close-ExcelResources $excel @($checkBook,$workbook)
}
'''
# Track explicit COM references, while scalar arrays and authored data remain untouched.
s=re.sub(r'(?m)^(\s*\$\w+ = )((?:New-Object -ComObject Excel.Application|\$(?:sheet|workbook|excel|checkBook|range)\.(?:Worksheets|Workbooks|Range|ListObjects|FormatConditions)[^\n]*))$',r'\1Register-ExcelCom (\2)',s)
p.write_text(s,encoding='utf-8-sig')
Path('render_mathis_preview.ps1').write_text('''param(
    [Parameter(Mandatory = $true)][string]$WorkbookPath,
    [Parameter(Mandatory = $true)][string]$OutputDir
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'tools\\Excel.Common.ps1')
Initialize-ExcelComTracking
$WorkbookPath = (Resolve-Path -LiteralPath $WorkbookPath -ErrorAction Stop).Path
$OutputDir = [IO.Path]::GetFullPath($OutputDir)
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$excel = $null; $book = $null
try {
    $excel = Register-ExcelCom (New-Object -ComObject Excel.Application)
    $excel.Visible = $false
    $excel.DisplayAlerts = $false
    $books = Register-ExcelCom $excel.Workbooks
    $book = Register-ExcelCom ($books.Open($WorkbookPath, 0, $true))
    $sheets = Register-ExcelCom $book.Worksheets
    foreach ($sheetName in @('测试概览','UI操作逻辑','APP基础测试')) {
        $sheet = Register-ExcelCom ($sheets.Item($sheetName))
        $sheet.Activate()
        $range = Register-ExcelCom $sheet.UsedRange
        $width = [Math]::Min([double]$range.Width, 2400.0)
        $height = [Math]::Min([double]$range.Height, 3200.0)
        $charts = Register-ExcelCom ($sheet.ChartObjects())
        $destination = Join-Path $OutputDir ($sheetName + '.png')
        $exported = $false
        for ($attempt=0; $attempt -lt 3; $attempt++) {
            $chartObject = $null
            try {
                $range.CopyPicture(1, 2)
                Start-Sleep -Milliseconds 200
                $chartObject = Register-ExcelCom ($charts.Add(0, 0, $width, $height))
                $chartObject.Activate()
                $chart = Register-ExcelCom $chartObject.Chart
                $chart.Paste() | Out-Null
                if ($chart.Shapes.Count -lt 1) { throw 'Excel has not pasted the picture.' }
                if (!$chart.Export($destination, 'PNG')) { throw 'Excel returned false when exporting PNG.' }
                if (!(Test-Path -LiteralPath $destination) -or ((Get-Item -LiteralPath $destination).Length -eq 0)) { throw 'PNG is empty.' }
                $exported = $true
                break
            } catch {
                if ($attempt -eq 2) { throw }
            } finally {
                if ($null -ne $chartObject) { $chartObject.Delete() }
            }
        }
        if (!$exported) { throw "Could not render worksheet $sheetName." }
    }
} finally {
    Close-ExcelResources $excel @($book)
}
''',encoding='utf-8-sig')
