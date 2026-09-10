param([string]$JLinkExe = 'C:\Keil_v5\ARM\Segger\JLink.exe')
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'tools\JLink.Common.ps1')
$commands=@('h','regs','mem32 0x08000000 2','mem32 0x08001800 2','mem32 0x20001FF0 1','g','qc')
$lock=Enter-MathisBuild $PSScriptRoot
try {
    $text=Invoke-MathisJLink $JLinkExe (Join-Path $PSScriptRoot 'OTA_Artifacts') 'read_startup_state' $commands
    foreach ($pattern in @('(?im)^\s*PC\s*=','(?im)^\s*08000000\s*=','(?im)^\s*08001800\s*=','(?im)^\s*20001FF0\s*=')) {
        if ($text -notmatch $pattern) { throw 'J-Link did not capture a complete MCU state.' }
    }
    Write-Host 'Startup state captured in OTA_Artifacts/read_startup_state_jlink_console.log.'
} finally { Exit-MathisBuild $lock }
