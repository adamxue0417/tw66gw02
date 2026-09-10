exec(open('outputs/optimization/edit_firmware.py',encoding='utf-8').read().split("s=read('Core/Src/main.c')")[0])
for script,source,directory,name,utility in [
 ('Bootloader/build_bootloader.ps1','boot_main.c','build','mathis_bootloader',True),
 ('Bootloader/build_min_bootloader.ps1','min_boot_main.c','build_min','mathis_min_bootloader',False),
 ('Diagnostics/build_gpio_alive.ps1','gpio_alive.c','build','diagnostic_gpio_alive',False)]:
    text='''param([string]$KeilRoot = 'C:\\Keil_v5')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $projectRoot 'tools\\Build.Common.ps1')
$lock = Enter-MathisBuild $projectRoot
try {
    $outputDir = Join-Path $PSScriptRoot 'DIRECTORY'
    Invoke-MathisSmallBuild -ProjectRoot $projectRoot -KeilRoot $KeilRoot `
        -Source (Join-Path $PSScriptRoot 'SOURCE') -OutputDir $outputDir -Name 'NAME' UTILITY
EXTRA
} finally { Exit-MathisBuild $lock }
'''.replace('DIRECTORY',directory).replace('SOURCE',source).replace('NAME',name).replace('UTILITY','-WithUtilities' if utility else ('-Optimization 2' if 'gpio' in name else ''))
    extra=''
    if 'gpio' in name:
        extra='''    $artifactDir = Join-Path $projectRoot 'OTA_Artifacts'
    New-Item -ItemType Directory -Force -Path $artifactDir | Out-Null
    foreach ($extension in @('bin','hex')) {
        Copy-Item -LiteralPath (Join-Path $outputDir "diagnostic_gpio_alive.$extension") -Destination $artifactDir -Force
    }'''
    write(script,text.replace('EXTRA',extra))

s=read('BuildOtaArtifacts.ps1')
prefix=s[:s.index('$ErrorActionPreference')]
body=s[s.index('try {\n    & (Join-Path $projectRoot'):s.rfind('finally {')]
body=body.replace("    if ($LASTEXITCODE -ne 0) { throw 'Bootloader build failed.' }",'')
body=body.replace('$factory = Invoke-AppBuild $FactoryVersion "v$FactoryVersion"', '''$factory = Invoke-MathisKeilBuild $projectRoot $KeilRoot 'app' $FactoryVersion "v$FactoryVersion"
    Copy-Item -LiteralPath $factory.Bin -Destination (Join-Path $outputDir "mathis_app_v$FactoryVersion.bin") -Force
    Copy-Item -LiteralPath $factory.Hex -Destination (Join-Path $outputDir "mathis_app_v$FactoryVersion.hex") -Force
    $factory.Bin = Join-Path $outputDir "mathis_app_v$FactoryVersion.bin"
    $factory.Hex = Join-Path $outputDir "mathis_app_v$FactoryVersion.hex"''')
body=body.replace('$ota = Invoke-AppBuild $OtaVersion "v$OtaVersion"', '''if ($OtaVersion -eq $FactoryVersion) { $ota = $factory }
    else {
        $ota = Invoke-MathisKeilBuild $projectRoot $KeilRoot 'app' $OtaVersion "v$OtaVersion"
        Copy-Item -LiteralPath $ota.Bin -Destination (Join-Path $outputDir "mathis_app_v$OtaVersion.bin") -Force
        Copy-Item -LiteralPath $ota.Hex -Destination (Join-Path $outputDir "mathis_app_v$OtaVersion.hex") -Force
        $ota.Bin = Join-Path $outputDir "mathis_app_v$OtaVersion.bin"
        $ota.Hex = Join-Path $outputDir "mathis_app_v$OtaVersion.hex"
    }''')
body=body.replace('Merge-IntelHex','Merge-MathisHex').replace('Get-Crc32','Get-MathisCrc32')
begin=body.index('    [byte[]]$factoryBytes =')
end=body.index('    $factoryBin =',begin)
body=body[:begin]+'    [byte[]]$factoryBytes = Join-MathisFactory $bootBytes $factoryAppBytes\n'+body[end:]
write('BuildOtaArtifacts.ps1',prefix+'''$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
$outputDir = Join-Path $projectRoot 'OTA_Artifacts'
. (Join-Path $projectRoot 'tools\\Build.Common.ps1')
$lock = Enter-MathisBuild $projectRoot
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
'''+body+'finally { Exit-MathisBuild $lock }\n')

write('BuildKeilFactoryTarget.ps1','''param([string]$KeilRoot = 'C:\\Keil_v5')
$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
. (Join-Path $projectRoot 'tools\\Build.Common.ps1')
$lock = Enter-MathisBuild $projectRoot
try {
    & (Join-Path $projectRoot 'PrepareFactoryBootImage.ps1') -KeilRoot $KeilRoot
    $build = Invoke-MathisKeilBuild $projectRoot $KeilRoot 'factory' 100 'factory_keil'
    $outputDir = Join-Path $projectRoot 'OTA_Artifacts'
    Copy-Item -LiteralPath $build.Hex -Destination (Join-Path $outputDir 'keil_factory_v100.hex') -Force
    Copy-Item -LiteralPath $build.Bin -Destination (Join-Path $outputDir 'keil_factory_v100.bin') -Force
    Write-Host "Keil factory target: $($build.Axf)"
} finally { Exit-MathisBuild $lock }
''')
write('PrepareFactoryBootImage.ps1','''param([string]$KeilRoot = 'C:\\Keil_v5')
$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
. (Join-Path $projectRoot 'tools\\Build.Common.ps1')
$lock = Enter-MathisBuild $projectRoot
try {
    $tool = Get-MathisToolchain $KeilRoot
    & (Join-Path $projectRoot 'Bootloader\\build_bootloader.ps1') -KeilRoot $KeilRoot
    Push-Location $projectRoot
    try {
        Invoke-MathisNative $tool.armasm @('--cpu','Cortex-M0','--xref','--debug','-o',
            (Join-Path $projectRoot 'MDK-ARM\\boot_image.o'),(Join-Path $projectRoot 'MDK-ARM\\boot_image.s')) 'Boot image embedding'
    } finally { Pop-Location }
} finally { Exit-MathisBuild $lock }
''')
write('BuildStartupDiagnostic.ps1','''param(
    [ValidateRange(0,255)][int]$Version = 100,
    [string]$KeilRoot = 'C:\\Keil_v5'
)
$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
. (Join-Path $projectRoot 'tools\\Build.Common.ps1')
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
    & (Join-Path $projectRoot 'Bootloader\\build_min_bootloader.ps1') -KeilRoot $KeilRoot
    $boot = [IO.File]::ReadAllBytes((Join-Path $projectRoot 'Bootloader\\build_min\\mathis_min_bootloader.bin'))
    $combined = Join-MathisFactory $boot ([IO.File]::ReadAllBytes($app.Bin))
    [IO.File]::WriteAllBytes((Join-Path $outputDir "diagnostic_minboot_factory_v$Version.bin"),$combined)
    Write-Host "Startup diagnostics v$Version built."
} finally { Exit-MathisBuild $lock }
''')
