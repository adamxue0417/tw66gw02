param([string]$JLinkExe = 'C:\Keil_v5\ARM\Segger\JLink.exe')

$ErrorActionPreference = 'Stop'
$artifactDir = Join-Path $PSScriptRoot 'OTA_Artifacts'
$commandFile = Join-Path $artifactDir 'verify_secure_provisioning.jlink'
$logFile = Join-Path $artifactDir 'verify_secure_provisioning_jlink.log'
$consoleFile = Join-Path $artifactDir 'verify_secure_provisioning_console.log'
if (!(Test-Path -LiteralPath $JLinkExe)) { throw "J-Link Commander not found: $JLinkExe" }
$commands = @(
    'h',
    'mem32 0x08000000 2',
    'mem32 0x08002000 2',
    'mem32 0x08001FC0 1',
    'mem32 0x08001FD8 8',
    'mem8 0x1FFFF800 2',
    'mem8 0x1FFFF808 2',
    'g', 'qc'
)
[IO.File]::WriteAllLines($commandFile, $commands, [Text.Encoding]::ASCII)
$consoleLines = @(& $JLinkExe -device STM32F030C8 -if SWD -speed 1000 -autoconnect 1 `
    -CommanderScript $commandFile -Log $logFile 2>&1 | ForEach-Object { "$_" })
$consoleLines | ForEach-Object { Write-Host $_ }
$consoleText = $consoleLines -join "`n"
[IO.File]::WriteAllText($consoleFile, $consoleText, [Text.UTF8Encoding]::new($false))
if ($consoleText -match '(?i)FAILED:|Cannot connect|Could not connect|Error:') {
    throw "Provisioning readback failed. See $consoleFile"
}
if ($consoleText -notmatch '(?i)08001FC0\s*=\s*4950414D') { throw 'Boot API magic mismatch.' }
if ($consoleText -notmatch '(?i)1FFFF800\s*=\s*AA\s+55') { throw 'RDP must remain Level 0.' }
if ($consoleText -notmatch '(?i)1FFFF808\s*=\s*FC\s+03') { throw 'WRP pages 0..7 are not enabled.' }
$fingerprintWords = @(
    'B7DCCC3F','804F5127','C7FE64A2','05C33369',
    'E93C2D26','A9F8912F','EC4E1E71','D4C0841A'
)
foreach ($word in $fingerprintWords) {
    if ($consoleText -notmatch $word) { throw "DEV fingerprint word missing: $word" }
}
Write-Host 'PASS: RDP0, boot pages 0..7 WRP, Boot API, and DEV public-key fingerprint verified.'
Write-Host 'DEV fingerprint: 3FCCDCB727514F80A264FEC76933C305262D3CE92F91F8A9711E4EEC1A84C0D4'
