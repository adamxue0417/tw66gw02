param(
    [string]$JLinkExe = 'C:\Keil_v5\ARM\Segger\JLink.exe'
)

$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
$artifactDir = Join-Path $projectRoot 'OTA_Artifacts'
$commandFile = Join-Path $artifactDir 'read_startup_state.jlink'
$logFile = Join-Path $artifactDir 'read_startup_state_jlink.log'
$consoleFile = Join-Path $artifactDir 'read_startup_state_jlink_console.log'

if (!(Test-Path -LiteralPath $JLinkExe)) { throw "J-Link Commander not found: $JLinkExe" }

$commands = @(
    'h',
    'regs',
    'mem32 0x08000000 2',
    'mem32 0x08001800 2',
    'mem32 0x20001FF0 1',
    'g',
    'qc'
)
[IO.File]::WriteAllLines($commandFile, $commands, [Text.Encoding]::ASCII)

$consoleLines = @(& $JLinkExe -device STM32F030C8 -if SWD -speed 1000 -autoconnect 1 `
    -CommanderScript $commandFile -Log $logFile 2>&1 | ForEach-Object { "$_" })
$consoleLines | ForEach-Object { Write-Host $_ }
$consoleText = $consoleLines -join "`n"
[IO.File]::WriteAllText($consoleFile, $consoleText, [Text.UTF8Encoding]::new($false))
if (($consoleText -match '(?i)FAILED:\s*Cannot connect|Cannot connect to the probe|Could not connect|Error:') -or
    ($consoleText -notmatch '(?im)^PC\s*=') -or
    ($consoleText -notmatch '(?im)^08000000\s*=') -or
    ($consoleText -notmatch '(?im)^20001FF0\s*=')) {
    throw "J-Link did not capture a valid MCU state. See $consoleFile"
}

Write-Host "Startup state captured: $logFile"
