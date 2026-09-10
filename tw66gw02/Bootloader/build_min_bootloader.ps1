param([string]$KeilRoot = 'C:\Keil_v5')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $projectRoot 'tools\Build.Common.ps1')
$lock = Enter-MathisBuild $projectRoot
try {
    $outputDir = Join-Path $PSScriptRoot 'build_min'
    Invoke-MathisSmallBuild -ProjectRoot $projectRoot -KeilRoot $KeilRoot `
        -Source (Join-Path $PSScriptRoot 'min_boot_main.c') -OutputDir $outputDir -Name 'mathis_min_bootloader' 

} finally { Exit-MathisBuild $lock }
