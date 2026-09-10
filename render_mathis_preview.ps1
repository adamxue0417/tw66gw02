param(
    [Parameter(Mandatory = $true)][string]$WorkbookPath,
    [Parameter(Mandatory = $true)][string]$OutputDir
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'tools\Excel.Common.ps1')
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
