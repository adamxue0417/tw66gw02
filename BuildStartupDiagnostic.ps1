param(
    [ValidateRange(0,255)][int]$Version = 100,
    [string]$KeilRoot = 'C:\Keil_v5'
)

$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
$mdkDir = Join-Path $projectRoot 'MDK-ARM'
$outputDir = Join-Path $projectRoot 'OTA_Artifacts'
$generatedProject = Join-Path $mdkDir 'tw66gw02_direct.generated.uvprojx'
$uv4 = Join-Path $KeilRoot 'UV4\UV4.exe'
$fromelf = Join-Path $KeilRoot 'ARM\ARM_Compiler_5.06u7\Bin\fromelf.exe'
$log = Join-Path $outputDir 'build_direct_v100.log'

New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

try {
    $template = Get-Content (Join-Path $mdkDir 'tw66gw02.uvprojx') -Raw -Encoding UTF8
    $template = $template.Replace('<OutputDirectory>.\tw66gw02\</OutputDirectory>',
                                  '<OutputDirectory>.\tw66gw02_direct\</OutputDirectory>')
    $template = $template.Replace(
        '<Define>USE_HAL_DRIVER,STM32F030x8</Define>',
        "<Define>USE_HAL_DRIVER,STM32F030x8,MATHIS_FW_VERSION=$Version</Define>")
    $template = $template.Replace('.\tw66gw02_factory.sct', '.\tw66gw02_direct.sct')
    $bootObjectEntry = @'
            <File>
              <FileName>boot_image.o</FileName>
              <FileType>3</FileType>
              <FilePath>boot_image.o</FilePath>
            </File>
'@
    $template = $template.Replace($bootObjectEntry, '')
    [IO.File]::WriteAllText($generatedProject, $template, [Text.UTF8Encoding]::new($false))
    if (Test-Path -LiteralPath $log) { Remove-Item -LiteralPath $log -Force }
    & $uv4 -r $generatedProject -t tw66gw02 -j0 -o $log
    # UV4 may hand the build to an already-running instance and return 1 before
    # that instance creates the log.  The completed log is authoritative.
    $deadline = [DateTime]::UtcNow.AddSeconds(120)
    do {
        Start-Sleep -Milliseconds 250
        if (Test-Path -LiteralPath $log) {
            $logText = Get-Content $log -Raw -Encoding Default
            if ($logText -match 'Build Time Elapsed:') { break }
        }
    } while ([DateTime]::UtcNow -lt $deadline)
    if (!(Test-Path -LiteralPath $log) -or ($logText -notmatch '0 Error\(s\)')) {
        throw "Direct-start diagnostic build failed or timed out. See $log"
    }

    $directOutputDir = Join-Path $mdkDir 'tw66gw02_direct'
    $axf = Join-Path $directOutputDir 'tw66gw02.axf'
    $bin = Join-Path $outputDir 'diagnostic_direct_v100.bin'
    $hex = Join-Path $outputDir 'diagnostic_direct_v100.hex'
    & $fromelf --bin --output $bin $axf
    if ($LASTEXITCODE -ne 0) { throw 'Direct-start BIN generation failed.' }
    Copy-Item -Force (Join-Path $directOutputDir 'tw66gw02.hex') $hex
    Write-Host "Direct-start diagnostic: $bin (program once at 0x08000000)"

    & (Join-Path $projectRoot 'Bootloader\build_min_bootloader.ps1') -KeilRoot $KeilRoot
    if ($LASTEXITCODE -ne 0) { throw 'Minimal bootloader diagnostic build failed.' }
    [byte[]]$minBoot = [IO.File]::ReadAllBytes(
        (Join-Path $projectRoot 'Bootloader\build_min\mathis_min_bootloader.bin'))
    [byte[]]$relocatedApp = [IO.File]::ReadAllBytes(
        (Join-Path $outputDir "mathis_app_v$Version.bin"))
    [byte[]]$minFactory = New-Object byte[] (0x1800 + $relocatedApp.Length)
    for ($index = 0; $index -lt $minFactory.Length; $index++) { $minFactory[$index] = 0xFF }
    [Array]::Copy($minBoot, 0, $minFactory, 0, $minBoot.Length)
    [Array]::Copy($relocatedApp, 0, $minFactory, 0x1800, $relocatedApp.Length)
    $minFactoryPath = Join-Path $outputDir 'diagnostic_minboot_factory_v100.bin'
    [IO.File]::WriteAllBytes($minFactoryPath, $minFactory)
    Write-Host "Minimal-boot diagnostic: $minFactoryPath (program once at 0x08000000)"
}
finally {
    if ((Test-Path -LiteralPath $log) -and (Test-Path -LiteralPath $generatedProject)) {
        Remove-Item -LiteralPath $generatedProject -Force
    }
}
