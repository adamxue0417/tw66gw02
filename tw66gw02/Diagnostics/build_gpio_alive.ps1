param([string]$KeilRoot = 'C:\Keil_v5')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $projectRoot 'tools\Build.Common.ps1')
$lock = Enter-MathisBuild $projectRoot
try {
    $outputDir = Join-Path $PSScriptRoot 'build'
    Invoke-MathisSmallBuild -ProjectRoot $projectRoot -KeilRoot $KeilRoot `
        -Source (Join-Path $PSScriptRoot 'gpio_alive.c') -OutputDir $outputDir -Name 'diagnostic_gpio_alive' -Optimization 2
    $artifactDir = Join-Path $projectRoot 'OTA_Artifacts'
    New-Item -ItemType Directory -Force -Path $artifactDir | Out-Null
    foreach ($extension in @('bin','hex')) {
        Copy-Item -LiteralPath (Join-Path $outputDir "diagnostic_gpio_alive.$extension") -Destination $artifactDir -Force
    }
} finally { Exit-MathisBuild $lock }
