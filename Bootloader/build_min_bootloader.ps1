param(
    [string]$KeilRoot = 'C:\Keil_v5'
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$outputDir = Join-Path $PSScriptRoot 'build_min'
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

$compiler = Join-Path $KeilRoot 'ARM\ARM_Compiler_5.06u7\Bin\armcc.exe'
$assembler = Join-Path $KeilRoot 'ARM\ARM_Compiler_5.06u7\Bin\armasm.exe'
$linker = Join-Path $KeilRoot 'ARM\ARM_Compiler_5.06u7\Bin\armlink.exe'
$fromelf = Join-Path $KeilRoot 'ARM\ARM_Compiler_5.06u7\Bin\fromelf.exe'
$cmsisDevice = Join-Path $projectRoot 'Drivers\CMSIS\Device\ST\STM32F0xx\Include'
$cmsisCore = Join-Path $projectRoot 'Drivers\CMSIS\Include'
$bsp = Join-Path $projectRoot 'BSP'

& $assembler --cpu Cortex-M0 --pd '__MICROLIB SETA 0' --xref --debug `
    -o (Join-Path $outputDir 'startup.o') (Join-Path $projectRoot 'MDK-ARM\startup_stm32f030x8.s')
if ($LASTEXITCODE -ne 0) { throw 'Minimal bootloader startup assembly failed.' }

& $compiler -c --cpu Cortex-M0 --c99 -O3 -Ospace --split_sections --debug `
    -DSTM32F030x8 -I $cmsisDevice -I $cmsisCore -I $bsp `
    -o (Join-Path $outputDir 'min_boot_main.o') (Join-Path $PSScriptRoot 'min_boot_main.c')
if ($LASTEXITCODE -ne 0) { throw 'Minimal bootloader compilation failed.' }

& $linker --cpu Cortex-M0 --strict --summary_stderr `
    --scatter (Join-Path $PSScriptRoot 'bootloader.sct') `
    -o (Join-Path $outputDir 'mathis_min_bootloader.axf') `
    (Join-Path $outputDir 'startup.o') (Join-Path $outputDir 'min_boot_main.o')
if ($LASTEXITCODE -ne 0) { throw 'Minimal bootloader link failed.' }

& $fromelf --bin --output (Join-Path $outputDir 'mathis_min_bootloader.bin') `
    (Join-Path $outputDir 'mathis_min_bootloader.axf')
if ($LASTEXITCODE -ne 0) { throw 'Minimal bootloader BIN generation failed.' }
& $fromelf --i32combined --output (Join-Path $outputDir 'mathis_min_bootloader.hex') `
    (Join-Path $outputDir 'mathis_min_bootloader.axf')
if ($LASTEXITCODE -ne 0) { throw 'Minimal bootloader HEX generation failed.' }

$size = (Get-Item (Join-Path $outputDir 'mathis_min_bootloader.bin')).Length
Write-Host "Minimal bootloader built: $size bytes"
