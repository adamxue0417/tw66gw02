param([string]$KeilRoot = 'C:\Keil_v5')
$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
. (Join-Path $projectRoot 'tools\Build.Common.ps1')
$lock = Enter-MathisBuild $projectRoot
try {
    & (Join-Path $projectRoot 'PrepareFactoryBootImage.ps1') -KeilRoot $KeilRoot
    $build = Invoke-MathisKeilBuild $projectRoot $KeilRoot 'factory' 100 'factory_keil'
    $outputDir = Join-Path $projectRoot 'OTA_Artifacts'
    Copy-Item -LiteralPath $build.Hex -Destination (Join-Path $outputDir 'keil_factory_v100.hex') -Force
    Copy-Item -LiteralPath $build.Bin -Destination (Join-Path $outputDir 'keil_factory_v100.bin') -Force
    Write-Host "Keil factory target: $($build.Axf)"
} finally { Exit-MathisBuild $lock }
