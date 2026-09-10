param(
    [string]$KeilRoot = 'C:\Keil_v5'
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$outputDir = Join-Path $PSScriptRoot 'build'
$artifactDir = Join-Path $projectRoot 'OTA_Artifacts'
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
New-Item -ItemType Directory -Force -Path $artifactDir | Out-Null

$compiler = Join-Path $KeilRoot 'ARM\ARM_Compiler_5.06u7\Bin\armcc.exe'
$assembler = Join-Path $KeilRoot 'ARM\ARM_Compiler_5.06u7\Bin\armasm.exe'
$linker = Join-Path $KeilRoot 'ARM\ARM_Compiler_5.06u7\Bin\armlink.exe'
$fromelf = Join-Path $KeilRoot 'ARM\ARM_Compiler_5.06u7\Bin\fromelf.exe'
$cmsisDevice = Join-Path $projectRoot 'Drivers\CMSIS\Device\ST\STM32F0xx\Include'
$cmsisCore = Join-Path $projectRoot 'Drivers\CMSIS\Include'

& $assembler --cpu Cortex-M0 --pd '__MICROLIB SETA 0' --xref --debug `
    -o (Join-Path $outputDir 'startup.o') (Join-Path $projectRoot 'MDK-ARM\startup_stm32f030x8.s')
if ($LASTEXITCODE -ne 0) { throw 'GPIO diagnostic startup assembly failed.' }

& $compiler -c --cpu Cortex-M0 --c99 -O2 --split_sections --debug `
    -DSTM32F030x8 -I $cmsisDevice -I $cmsisCore `
    -o (Join-Path $outputDir 'gpio_alive.o') (Join-Path $PSScriptRoot 'gpio_alive.c')
if ($LASTEXITCODE -ne 0) { throw 'GPIO diagnostic compilation failed.' }

& $linker --cpu Cortex-M0 --strict --summary_stderr `
    --scatter (Join-Path $projectRoot 'Bootloader\bootloader.sct') `
    -o (Join-Path $outputDir 'diagnostic_gpio_alive.axf') `
    (Join-Path $outputDir 'startup.o') (Join-Path $outputDir 'gpio_alive.o')
if ($LASTEXITCODE -ne 0) { throw 'GPIO diagnostic link failed.' }

$bin = Join-Path $artifactDir 'diagnostic_gpio_alive.bin'
$hex = Join-Path $artifactDir 'diagnostic_gpio_alive.hex'
& $fromelf --bin --output $bin (Join-Path $outputDir 'diagnostic_gpio_alive.axf')
if ($LASTEXITCODE -ne 0) { throw 'GPIO diagnostic BIN generation failed.' }
& $fromelf --i32combined --output $hex (Join-Path $outputDir 'diagnostic_gpio_alive.axf')
if ($LASTEXITCODE -ne 0) { throw 'GPIO diagnostic HEX generation failed.' }
Write-Host "GPIO-alive diagnostic built: $bin ($((Get-Item $bin).Length) bytes)"
