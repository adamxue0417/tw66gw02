param(
    [string]$KeilRoot = 'C:\Keil_v5'
)

$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
$assembler = Join-Path $KeilRoot 'ARM\ARM_Compiler_5.06u7\Bin\armasm.exe'
$bootObject = Join-Path $projectRoot 'MDK-ARM\boot_image.o'

& (Join-Path $projectRoot 'Bootloader\build_bootloader.ps1') -KeilRoot $KeilRoot
if ($LASTEXITCODE -ne 0) { throw 'Bootloader build failed.' }

Push-Location $projectRoot
try {
    & $assembler --cpu Cortex-M0 --xref --debug -o $bootObject `
        (Join-Path $projectRoot 'MDK-ARM\boot_image.s')
    if ($LASTEXITCODE -ne 0) { throw 'Bootloader image embedding failed.' }
}
finally {
    Pop-Location
}

Write-Host "Factory boot image prepared: $bootObject"
