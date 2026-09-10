param(
    [Parameter(Mandatory = $true)][string]$WorkbookPath,
    [Parameter(Mandatory = $true)][string]$OutputDir
)

$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$excel = $null
$book = $null
try {
    $excel = New-Object -ComObject Excel.Application
    $excel.Visible = $false
    $excel.DisplayAlerts = $false
    $book = $excel.Workbooks.Open($WorkbookPath, 0, $true)
    foreach ($sheetName in @('测试概览','UI操作逻辑','APP基础测试')) {
        $sheet = $book.Worksheets.Item($sheetName)
        $sheet.Activate()
        $range = $sheet.UsedRange
        $range.CopyPicture(1, 2)
        Start-Sleep -Milliseconds 700
        $width = [Math]::Min([double]$range.Width, 2400.0)
        $height = [Math]::Min([double]$range.Height, 3200.0)
        $chartObject = $sheet.ChartObjects().Add(0, 0, $width, $height)
        $chartObject.Activate()
        $chartObject.Chart.Paste() | Out-Null
        Start-Sleep -Milliseconds 700
        $chartObject.Chart.Export((Join-Path $OutputDir ($sheetName + '.png')), 'PNG') | Out-Null
        $chartObject.Delete()
    }
    $book.Close($false)
    $book = $null
    $excel.Quit()
    $excel = $null
}
finally {
    if ($book -ne $null) { $book.Close($false) }
    if ($excel -ne $null) { $excel.Quit() }
    [GC]::Collect()
    [GC]::WaitForPendingFinalizers()
}
