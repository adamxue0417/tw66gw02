param(
    [ValidateRange(0,255)][int]$Version = 100,
    [string]$KeilRoot = 'C:\Keil_v5'
)
$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
. (Join-Path $projectRoot 'tools\Build.Common.ps1')
$lock = Enter-MathisBuild $projectRoot
try {
    $outputDir = Join-Path $projectRoot 'OTA_Artifacts'
    $build = Invoke-MathisKeilBuild $projectRoot $KeilRoot 'direct' $Version "direct_v$Version"
    Copy-Item -LiteralPath $build.Bin -Destination (Join-Path $outputDir "diagnostic_direct_v$Version.bin") -Force
    Copy-Item -LiteralPath $build.Hex -Destination (Join-Path $outputDir "diagnostic_direct_v$Version.hex") -Force
    # Build the relocated companion in this run rather than silently using a stale file.
    $app = Invoke-MathisKeilBuild $projectRoot $KeilRoot 'app' $Version "v$Version"
    Copy-Item -LiteralPath $app.Bin -Destination (Join-Path $outputDir "mathis_app_v$Version.bin") -Force
    Copy-Item -LiteralPath $app.Hex -Destination (Join-Path $outputDir "mathis_app_v$Version.hex") -Force
    & (Join-Path $projectRoot 'Bootloader\build_min_bootloader.ps1') -KeilRoot $KeilRoot
    $boot = [IO.File]::ReadAllBytes((Join-Path $projectRoot 'Bootloader\build_min\mathis_min_bootloader.bin'))
    $combined = Join-MathisFactory $boot ([IO.File]::ReadAllBytes($app.Bin))
    [IO.File]::WriteAllBytes((Join-Path $outputDir "diagnostic_minboot_factory_v$Version.bin"),$combined)
    Write-Host "Startup diagnostics v$Version built."
} finally { Exit-MathisBuild $lock }
