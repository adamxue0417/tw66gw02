param(
    [string]$Image = '.\OTA_Artifacts\mathis_factory_v100.bin',
    [string]$JLinkExe = 'C:\Keil_v5\ARM\Segger\JLink.exe'
)

$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
$imagePath = (Resolve-Path -LiteralPath (Join-Path $projectRoot $Image)).Path
$artifactDir = Join-Path $projectRoot 'OTA_Artifacts'
$commandFile = Join-Path $artifactDir 'program_factory.jlink'
$logFile = Join-Path $artifactDir 'program_factory_jlink.log'
$consoleFile = Join-Path $artifactDir 'program_factory_jlink_console.log'

if (!(Test-Path -LiteralPath $JLinkExe)) { throw "J-Link Commander not found: $JLinkExe" }
[byte[]]$imageBytes = [IO.File]::ReadAllBytes($imagePath)
if ($imageBytes.Length -lt 8) { throw 'Image is too short to contain an STM32 vector table.' }
$initialSp = [BitConverter]::ToUInt32($imageBytes, 0)
$resetVector = [BitConverter]::ToUInt32($imageBytes, 4)

$commands = @(
    'r',
    'h',
    'erase',
    "loadbin $imagePath, 0x08000000",
    "verifybin $imagePath, 0x08000000",
    'mem32 0x08000000 2',
    'mem32 0x08001800 2',
    'r',
    'g',
    'qc'
)
[IO.File]::WriteAllLines($commandFile, $commands, [Text.Encoding]::ASCII)

$consoleLines = @(& $JLinkExe -device STM32F030C8 -if SWD -speed 1000 -autoconnect 1 `
    -CommanderScript $commandFile -Log $logFile 2>&1 | ForEach-Object { "$_" })
$consoleLines | ForEach-Object { Write-Host $_ }
$consoleText = $consoleLines -join "`n"
[IO.File]::WriteAllText($consoleFile, $consoleText, [Text.UTF8Encoding]::new($false))
if ($consoleText -match '(?i)FAILED:\s*Cannot connect|Cannot connect to the probe|Could not connect|Error:') {
    throw "J-Link did not connect/program the MCU. See $consoleFile"
}
$basePattern = ('(?i)08000000\s*=\s*{0:X8}\s+{1:X8}' -f $initialSp, $resetVector)
if ($consoleText -notmatch $basePattern) {
    throw "Flash-base vector readback did not match the image. See $consoleFile"
}

Write-Host 'Factory image programmed and verified.'
Write-Host "Log: $logFile"
Write-Host ('Expected flash-base vectors: {0:X8}/{1:X8}.' -f $initialSp, $resetVector)
if ($imageBytes.Length -ge 0x1808) {
    $appSp = [BitConverter]::ToUInt32($imageBytes, 0x1800)
    $appReset = [BitConverter]::ToUInt32($imageBytes, 0x1804)
    $appPattern = ('(?i)08001800\s*=\s*{0:X8}\s+{1:X8}' -f $appSp, $appReset)
    if ($consoleText -notmatch $appPattern) {
        throw "Relocated-application vector readback did not match the image. See $consoleFile"
    }
    Write-Host ('Expected relocated-app vectors: {0:X8}/{1:X8}.' -f $appSp, $appReset)
}
