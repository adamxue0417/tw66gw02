param([string]$KeilRoot = 'C:\Keil_v5')
$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
. (Join-Path $projectRoot 'tools\Build.Common.ps1')
$lock = Enter-MathisBuild $projectRoot
try {
    $tool = Get-MathisToolchain $KeilRoot
    & (Join-Path $projectRoot 'Bootloader\build_bootloader.ps1') -KeilRoot $KeilRoot
    Push-Location $projectRoot
    try {
        Invoke-MathisNative $tool.armasm @('--cpu','Cortex-M0','--xref','--debug','-o',
            (Join-Path $projectRoot 'MDK-ARM\boot_image.o'),(Join-Path $projectRoot 'MDK-ARM\boot_image.s')) 'Boot image embedding'
    } finally { Pop-Location }
} finally { Exit-MathisBuild $lock }
