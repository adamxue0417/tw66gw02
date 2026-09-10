param(
    [string]$JLinkExe = 'C:\Keil_v5\ARM\Segger\JLink.exe'
)

$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
$artifactDir = Join-Path $projectRoot 'OTA_Artifacts'
$bootPath = Join-Path $projectRoot 'Bootloader\build\mathis_bootloader.bin'
$commandFile = Join-Path $artifactDir 'program_bootloader_only.jlink'
$logFile = Join-Path $artifactDir 'program_bootloader_only_jlink.log'
$consoleFile = Join-Path $artifactDir 'program_bootloader_only_jlink_console.log'

if (!(Test-Path -LiteralPath $JLinkExe)) { throw "J-Link Commander not found: $JLinkExe" }
if (!(Test-Path -LiteralPath $bootPath)) { throw "Bootloader BIN not found: $bootPath" }
[byte[]]$bootBytes = [IO.File]::ReadAllBytes($bootPath)
if (($bootBytes.Length -lt 8) -or ($bootBytes.Length -gt 0x1800)) {
    throw "Invalid bootloader size: $($bootBytes.Length)"
}
$bootSp = [BitConverter]::ToUInt32($bootBytes, 0)
$bootReset = [BitConverter]::ToUInt32($bootBytes, 4)

$commands = @(
    'r',
    'h',
    'mem32 0x08001800 2',
    "loadbin $bootPath, 0x08000000",
    "verifybin $bootPath, 0x08000000",
    'mem32 0x08000000 2',
    'mem32 0x08001800 2',
    'r',
    'g',
    'sleep 1000',
    'h',
    'regs',
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

if ($consoleText -match '(?i)FAILED:\s*Cannot connect|Cannot connect to the probe|Could not connect|Error:') {
    throw "J-Link bootloader update failed. See $consoleFile"
}
$bootPattern = ('(?i)08000000\s*=\s*{0:X8}\s+{1:X8}' -f $bootSp, $bootReset)
if ($consoleText -notmatch $bootPattern) {
    throw "Bootloader vector readback mismatch. See $consoleFile"
}
if ($consoleText -notmatch '(?i)08001800\s*=\s*20001598\s+08001991') {
    throw "Relocated application vector changed or is invalid. See $consoleFile"
}
if ($consoleText -notmatch '(?i)20001FF0\s*=\s*A9900007') {
    throw "Application did not reach its scheduler loop. See $consoleFile"
}

Write-Host 'Bootloader-only update verified; application scheduler is running.'
Write-Host "Log: $consoleFile"
