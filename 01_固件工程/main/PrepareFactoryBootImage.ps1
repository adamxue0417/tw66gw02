param(
    [string]$KeilRoot = 'C:\Keil_v5'
)

$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
$assembler = Join-Path $KeilRoot 'ARM\ARM_Compiler_5.06u7\Bin\armasm.exe'
$bootObject = Join-Path $projectRoot 'MDK-ARM\boot_image.o'

& (Join-Path $projectRoot 'Bootloader\build_bootloader.ps1') -KeilRoot $KeilRoot
if ($LASTEXITCODE -ne 0) { throw 'Bootloader build failed.' }

# This assembly wrapper is a generated file and is not kept in Git.
# Recreate it so factory builds also work from a clean checkout.
$bootAssembly = Join-Path $projectRoot 'MDK-ARM\boot_image.s'
$bootAssemblyText = @'
                AREA    BOOT_IMAGE, DATA, READONLY, ALIGN=2
                EXPORT  __boot_image_start
                EXPORT  __boot_image_end
__boot_image_start
                INCBIN  Bootloader\build\mathis_bootloader.bin
__boot_image_end
                END
'@
[IO.File]::WriteAllText($bootAssembly, $bootAssemblyText + "`r`n", [Text.Encoding]::ASCII)

Push-Location $projectRoot
try {
    & $assembler --cpu Cortex-M0 --xref --debug -o $bootObject `
        $bootAssembly
    if ($LASTEXITCODE -ne 0) { throw 'Bootloader image embedding failed.' }
}
finally {
    Pop-Location
}

Write-Host "Factory boot image prepared: $bootObject"
