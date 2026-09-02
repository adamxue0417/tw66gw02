param(
    [string]$KeilRoot = 'C:\Keil_v5',
    [string]$OutputDir = ''
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($OutputDir)) { $outputDir = Join-Path $PSScriptRoot 'build' }
else { $outputDir = [IO.Path]::GetFullPath($OutputDir) }
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

$compiler = Join-Path $KeilRoot 'ARM\ARM_Compiler_5.06u7\Bin\armcc.exe'
$assembler = Join-Path $KeilRoot 'ARM\ARM_Compiler_5.06u7\Bin\armasm.exe'
$linker = Join-Path $KeilRoot 'ARM\ARM_Compiler_5.06u7\Bin\armlink.exe'
$fromelf = Join-Path $KeilRoot 'ARM\ARM_Compiler_5.06u7\Bin\fromelf.exe'
$cmsisDevice = Join-Path $projectRoot 'Drivers\CMSIS\Device\ST\STM32F0xx\Include'
$cmsisCore = Join-Path $projectRoot 'Drivers\CMSIS\Include'
$bsp = Join-Path $projectRoot 'BSP'
$bearInc = Join-Path $projectRoot 'ThirdParty\BearSSL\inc'
$bearSrc = Join-Path $projectRoot 'ThirdParty\BearSSL\src'

& $assembler --cpu Cortex-M0 --pd '__MICROLIB SETA 1' --xref --debug `
    -o (Join-Path $outputDir 'startup.o') (Join-Path $projectRoot 'MDK-ARM\startup_stm32f030x8.s')
if ($LASTEXITCODE -ne 0) { throw 'Bootloader startup assembly failed.' }

& $compiler -c --cpu Cortex-M0 --c99 -O3 -Ospace --split_sections --debug --library_type=microlib `
    -DSTM32F030x8 -I $cmsisDevice -I $cmsisCore -I $bsp -I $bearInc -I $bearSrc `
    -o (Join-Path $outputDir 'boot_main.o') (Join-Path $PSScriptRoot 'boot_main.c')
if ($LASTEXITCODE -ne 0) { throw 'Bootloader C compilation failed.' }

& $compiler -c --cpu Cortex-M0 --c99 -O3 -Ospace --split_sections --debug --library_type=microlib `
    -DSTM32F030x8 -I $cmsisDevice -I $cmsisCore -I $bsp -I $bearInc -I $bearSrc `
    -o (Join-Path $outputDir 'boot_security.o') (Join-Path $PSScriptRoot 'boot_security.c')
if ($LASTEXITCODE -ne 0) { throw 'Bootloader security compilation failed.' }

$cryptoSources = @(
    'rsa\rsa_pkcs1_sig_unpad.c',
    'int\i15_decode.c', 'int\i15_decmod.c', 'int\i15_encode.c', 'int\i15_ninv15.c',
    'int\i15_modpow2.c', 'int\i15_montmul.c', 'int\i15_muladd.c',
    'int\i15_tmont.c', 'int\i15_fmont.c', 'int\i15_bitlen.c',
    'int\i15_add.c', 'int\i15_sub.c', 'codec\ccopy.c',
    'codec\dec32be.c', 'codec\enc32be.c', 'hash\sha2small.c'
)
$cryptoObjects = @()
foreach ($source in $cryptoSources) {
    $objectName = ([IO.Path]::GetFileNameWithoutExtension($source) + '.o')
    $objectPath = Join-Path $outputDir $objectName
    & $compiler -c --cpu Cortex-M0 --c99 -O3 -Ospace --split_sections --debug --library_type=microlib `
        -I $bearInc -I $bearSrc -o $objectPath (Join-Path $bearSrc $source)
    if ($LASTEXITCODE -ne 0) { throw "BearSSL compilation failed: $source" }
    $cryptoObjects += $objectPath
}

& $linker --cpu Cortex-M0 --strict --summary_stderr --callgraph --map --info=stack --library_type=microlib `
    --scatter (Join-Path $PSScriptRoot 'bootloader.sct') `
    --list (Join-Path $outputDir 'mathis_bootloader.map') `
    -o (Join-Path $outputDir 'mathis_bootloader.axf') `
    (Join-Path $outputDir 'startup.o') (Join-Path $outputDir 'boot_main.o') `
    (Join-Path $outputDir 'boot_security.o') $cryptoObjects
if ($LASTEXITCODE -ne 0) { throw 'Bootloader link failed.' }

& $fromelf --i32combined --output (Join-Path $outputDir 'mathis_bootloader.hex') (Join-Path $outputDir 'mathis_bootloader.axf')
if ($LASTEXITCODE -ne 0) { throw 'Bootloader HEX generation failed.' }
& $fromelf --bin --output (Join-Path $outputDir 'mathis_bootloader.bin') (Join-Path $outputDir 'mathis_bootloader.axf')
if ($LASTEXITCODE -ne 0) { throw 'Bootloader BIN generation failed.' }

$bootSize = (Get-Item (Join-Path $outputDir 'mathis_bootloader.bin')).Length
if ($bootSize -gt 0x2000) { throw "Bootloader is $bootSize bytes; limit is 8192 bytes." }
Write-Host "Bootloader built: $bootSize bytes"
