param([string]$JLinkExe = 'C:\Keil_v5\ARM\Segger\JLink.exe')
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'tools\JLink.Common.ps1')
$bootPath=Resolve-MathisImage $PSScriptRoot 'Bootloader\build\mathis_bootloader.bin'
$bytes=[IO.File]::ReadAllBytes($bootPath)
if (($bytes.Length -lt 8) -or ($bytes.Length -gt 0x1800)) { throw 'Invalid bootloader size.' }
$boot=Assert-MathisVector $bytes 0 0x08000000 $bytes.Length
$commands=@('r','h','mem32 0x08001800 2',('loadbin "{0}", 0x08000000' -f $bootPath),
    ('verifybin "{0}", 0x08000000' -f $bootPath),'mem32 0x08000000 2','mem32 0x08001800 2',
    'r','g','sleep 1000','h','regs','mem32 0x20001FF0 1','g','qc')
$lock=Enter-MathisBuild $PSScriptRoot
try {
    $text=Invoke-MathisJLink $JLinkExe (Join-Path $PSScriptRoot 'OTA_Artifacts') 'program_bootloader_only' $commands -RequireVerification
    $actual=@(Get-MathisVectorReadbacks $text '08000000')
    if (($actual.Count -ne 1) -or ($actual[0] -ne ('{0:X8}/{1:X8}' -f $boot.Stack,$boot.Reset))) { throw 'Bootloader vector mismatch.' }
    $apps=@(Get-MathisVectorReadbacks $text '08001800')
    if (($apps.Count -ne 2) -or ($apps[0] -ne $apps[1])) { throw 'Application vectors changed during bootloader update.' }
    $parts=$apps[0].Split('/')
    $appVector=[BitConverter]::GetBytes([Convert]::ToUInt32($parts[0],16)) + [BitConverter]::GetBytes([Convert]::ToUInt32($parts[1],16))
    $null=Assert-MathisVector $appVector 0 0x08001800 0x6A80 -Relocated
    if ($text -notmatch '(?im)^\s*20001FF0\s*=\s*A9900007') { throw 'Application did not reach its scheduler loop.' }
    Write-Host 'Bootloader-only update verified; application scheduler is running.'
} finally { Exit-MathisBuild $lock }
