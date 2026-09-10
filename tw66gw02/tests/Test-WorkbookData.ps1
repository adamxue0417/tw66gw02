$ErrorActionPreference='Stop'
$workspace=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$baseline=Join-Path $workspace 'outputs\optimization\baseline\build_mathis_test_workbook.ps1'
$optimized=Join-Path $workspace 'build_mathis_test_workbook.ps1'
function Get-CaseData([string]$Path) {
    $source=[IO.File]::ReadAllText($Path,[Text.Encoding]::UTF8)
    $begin=$source.IndexOf('function Get-OleColor')
    $end=$source.IndexOf('$outputDir = Split-Path')
    & ([scriptblock]::Create($source.Substring($begin,$end-$begin)))
    return @{Ui=$uiCases;App=$appCases;Headers=$headers;Widths=$columnWidths;Palette=$palette}
}
$old=Get-CaseData $baseline
$new=Get-CaseData $optimized
foreach ($key in @('Ui','App','Headers','Widths','Palette')) {
    if (($old[$key] | ConvertTo-Json -Depth 12 -Compress) -cne ($new[$key] | ConvertTo-Json -Depth 12 -Compress)) {
        throw "Workbook data changed: $key"
    }
}
foreach ($row in @($new.Ui)+@($new.App)) {
    if ($row.Count -ne 13) { throw 'A test case does not contain 13 cells.' }
}

. (Join-Path $workspace 'tools\Excel.Common.ps1')
Initialize-ExcelComTracking
$script:written=$null
$cells=[pscustomobject]@{}
$cells | Add-Member ScriptMethod Item { param($r,$c) return @($r,$c) }
$range=[pscustomobject]@{Value2=$null}
$sheet=[pscustomobject]@{Cells=$cells;Target=$range}
$sheet | Add-Member ScriptMethod Range { param($first,$last) return $this.Target }
$matrix=[object[,]]::new($new.Ui.Count,13)
for ($r=0; $r -lt $new.Ui.Count; $r++) {
    for ($c=0; $c -lt 13; $c++) { $matrix[$r,$c]=$new.Ui[$r][$c] }
}
Set-ExcelMatrix $sheet 5 1 $matrix
if ($range.Value2.GetLength(0) -ne $new.Ui.Count) { throw 'Batch write lost rows.' }
for ($r=0; $r -lt $new.Ui.Count; $r++) {
    for ($c=0; $c -lt 13; $c++) {
        if ($range.Value2[$r,$c] -cne $new.Ui[$r][$c]) { throw 'Batch write changed cell data.' }
    }
}
$app=[pscustomobject]@{QuitCalled=$false}
$app | Add-Member ScriptMethod Quit { $this.QuitCalled=$true }
$book=[pscustomobject]@{}
$book | Add-Member ScriptMethod Close { param($save) throw 'Injected close failure' }
Close-ExcelResources $app @($book) -WarningAction SilentlyContinue
if (!$app.QuitCalled) { throw 'Excel Quit was skipped after a close failure.' }
Write-Host "PASS: $($new.Ui.Count) UI cases, $($new.App.Count) APP cases, headers, widths, palette, batch write, cleanup failure path."
