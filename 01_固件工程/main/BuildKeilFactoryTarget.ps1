param(
    [string]$KeilRoot = 'C:\Keil_v5',
    [ValidateSet(0,1)][int]$IdleSleepEnabled = 1
)

$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
$mdkDir = Join-Path $projectRoot 'MDK-ARM'
$artifactDir = Join-Path $projectRoot 'OTA_Artifacts'
$generatedProject = Join-Path $mdkDir 'tw66gw02_factory.generated.uvprojx'
$log = Join-Path $artifactDir 'build_factory_keil.log'
$uv4 = Join-Path $KeilRoot 'UV4\UV4.exe'
$fromelf = Join-Path $KeilRoot 'ARM\ARM_Compiler_5.06u7\Bin\fromelf.exe'

New-Item -ItemType Directory -Force -Path $artifactDir | Out-Null
& (Join-Path $projectRoot 'PrepareFactoryBootImage.ps1') -KeilRoot $KeilRoot
if ($LASTEXITCODE -ne 0) { throw 'Factory boot image preparation failed.' }

try {
    $template = Get-Content (Join-Path $mdkDir 'tw66gw02.uvprojx') -Raw -Encoding UTF8
    $template = $template.Replace('<Define>USE_HAL_DRIVER,STM32F030x8</Define>',
        "<Define>USE_HAL_DRIVER,STM32F030x8,SCH_IDLE_SLEEP_ENABLED=$IdleSleepEnabled</Define>")
    $disabledBeforeMake = @'
          <BeforeMake>
            <RunUserProg1>0</RunUserProg1>
            <RunUserProg2>0</RunUserProg2>
            <UserProg1Name></UserProg1Name>
            <UserProg2Name></UserProg2Name>
            <UserProg1Dos16Mode>0</UserProg1Dos16Mode>
            <UserProg2Dos16Mode>0</UserProg2Dos16Mode>
            <nStopB1X>0</nStopB1X>
            <nStopB2X>0</nStopB2X>
          </BeforeMake>
'@
    $template = [regex]::Replace($template, '(?s)          <BeforeMake>.*?          </BeforeMake>',
                                 $disabledBeforeMake, 1)
    [IO.File]::WriteAllText($generatedProject, $template, [Text.UTF8Encoding]::new($false))
    if (Test-Path -LiteralPath $log) { Remove-Item -LiteralPath $log -Force }
    & $uv4 -r $generatedProject -t tw66gw02 -j0 -o $log

    $deadline = [DateTime]::UtcNow.AddSeconds(120)
    do {
        Start-Sleep -Milliseconds 250
        if (Test-Path -LiteralPath $log) {
            $logText = Get-Content $log -Raw -Encoding Default
            if ($logText -match 'Build Time Elapsed:') { break }
        }
    } while ([DateTime]::UtcNow -lt $deadline)
    if (!(Test-Path -LiteralPath $log) -or ($logText -notmatch '0 Error\(s\)')) {
        throw "Keil factory target failed or timed out. See $log"
    }

    $axf = Join-Path $mdkDir 'tw66gw02\tw66gw02.axf'
    $hex = Join-Path $artifactDir 'keil_factory_v100.hex'
    $bin = Join-Path $artifactDir 'keil_factory_v100.bin'
    Copy-Item -Force (Join-Path $mdkDir 'tw66gw02\tw66gw02.hex') $hex
    & $fromelf --bincombined --output $bin $axf
    if ($LASTEXITCODE -ne 0) { throw 'Keil factory BIN generation failed.' }
    Write-Host "Keil factory target: $axf"
    Write-Host "Factory HEX: $hex"
    Write-Host "Factory BIN: $bin"
}
finally {
    if (Test-Path -LiteralPath $generatedProject) { Remove-Item -LiteralPath $generatedProject -Force }
}
