param(
    [string]$Image = '.\OTA_Artifacts\mathis_secure_factory_v102_dev.bin',
    [string]$JLinkExe = 'C:\Keil_v5\ARM\Segger\JLink.exe',
    [switch]$EnableBootWriteProtection
)

$ErrorActionPreference = 'Stop'
if (!$EnableBootWriteProtection) {
    throw 'WRP confirmation required: rerun with -EnableBootWriteProtection.'
}
$root = $PSScriptRoot
$imagePath = (Resolve-Path -LiteralPath (Join-Path $root $Image)).Path
$artifactDir = Join-Path $root 'OTA_Artifacts'
$commandFile = Join-Path $artifactDir 'program_secure_factory_v102_dev.jlink'
$logFile = Join-Path $artifactDir 'program_secure_factory_v102_dev_jlink.log'
$consoleFile = Join-Path $artifactDir 'program_secure_factory_v102_dev_console.log'
$wrpFile = Join-Path $artifactDir 'stm32f030_boot_pages_0_7_wrp.bin'
if (!(Test-Path -LiteralPath $JLinkExe)) { throw "J-Link Commander not found: $JLinkExe" }

[byte[]]$imageBytes = [IO.File]::ReadAllBytes($imagePath)
if (($imageBytes.Length -le 0x2008) -or ($imageBytes.Length -gt 0x8400)) {
    throw "Unexpected secure factory image size: $($imageBytes.Length)"
}
$bootSp = [BitConverter]::ToUInt32($imageBytes, 0)
$bootReset = [BitConverter]::ToUInt32($imageBytes, 4)
$appSp = [BitConverter]::ToUInt32($imageBytes, 0x2000)
$appReset = [BitConverter]::ToUInt32($imageBytes, 0x2004)
if ([BitConverter]::ToUInt32($imageBytes, 0x1FC0) -ne 0x4950414D) {
    throw 'Boot API magic is absent at 0x08001FC0.'
}

# WRP0=0xFC protects STM32F030 page groups 0..3 and 4..7. 0x03 is
# its required complement byte. RDP at 0x1FFFF800 is never written.
[IO.File]::WriteAllBytes($wrpFile, [byte[]](0xFC, 0x03))
$commands = @(
    'r', 'h', 'erase',
    "loadbin $imagePath, 0x08000000",
    "verifybin $imagePath, 0x08000000",
    'mem32 0x08000000 2',
    'mem32 0x08002000 2',
    'mem32 0x08001FC0 1',
    'mem8 0x1FFFF800 2',
    "loadbin $wrpFile, 0x1FFFF808",
    "verifybin $wrpFile, 0x1FFFF808",
    'mem8 0x1FFFF808 2',
    'r', 'g', 'qc'
)
[IO.File]::WriteAllLines($commandFile, $commands, [Text.Encoding]::ASCII)
$consoleLines = @(& $JLinkExe -device STM32F030C8 -if SWD -speed 1000 -autoconnect 1 `
    -CommanderScript $commandFile -Log $logFile 2>&1 | ForEach-Object { "$_" })
$consoleLines | ForEach-Object { Write-Host $_ }
$consoleText = $consoleLines -join "`n"
[IO.File]::WriteAllText($consoleFile, $consoleText, [Text.UTF8Encoding]::new($false))
if ($consoleText -match '(?i)FAILED:|Cannot connect|Could not connect|Error:') {
    throw "Secure factory programming failed. See $consoleFile"
}
if ($consoleText -notmatch ('(?i)08000000\s*=\s*{0:X8}\s+{1:X8}' -f $bootSp, $bootReset)) {
    throw 'Bootloader vector readback mismatch.'
}
if ($consoleText -notmatch ('(?i)08002000\s*=\s*{0:X8}\s+{1:X8}' -f $appSp, $appReset)) {
    throw 'Application vector readback mismatch.'
}
if ($consoleText -notmatch '(?i)08001FC0\s*=\s*4950414D') { throw 'Boot API readback mismatch.' }
if ($consoleText -notmatch '(?i)1FFFF800\s*=\s*AA\s+55') { throw 'RDP is not Level 0.' }
if ($consoleText -notmatch '(?i)1FFFF808\s*=\s*FC\s+03') { throw 'Boot WRP readback mismatch.' }
Write-Host "Secure factory SHA-256: $((Get-FileHash -Algorithm SHA256 $imagePath).Hash)"
Write-Host 'Power-cycle the controller, then run VerifySecureProvisioningWithJLink.ps1.'
