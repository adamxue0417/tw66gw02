param(
    [string]$Image = '.\OTA_Artifacts\mathis_factory_v100.bin',
    [string]$JLinkExe = 'C:\Keil_v5\ARM\Segger\JLink.exe'
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'tools\JLink.Common.ps1')
$imagePath=Resolve-MathisImage $PSScriptRoot $Image
$bytes=[IO.File]::ReadAllBytes($imagePath)
if (($bytes.Length -lt 0x18C0) -or ($bytes.Length -gt 0x8280)) { throw 'Invalid complete factory image size.' }
$boot=Assert-MathisVector $bytes 0 0x08000000 0x1800
$app=Assert-MathisVector $bytes 0x1800 0x08001800 ($bytes.Length-0x1800) -Relocated
$commands=@('r','h','erase',('loadbin "{0}", 0x08000000' -f $imagePath),
    ('verifybin "{0}", 0x08000000' -f $imagePath),'mem32 0x08000000 2','mem32 0x08001800 2','r','g','qc')
$lock=Enter-MathisBuild $PSScriptRoot
try {
    $text=Invoke-MathisJLink $JLinkExe (Join-Path $PSScriptRoot 'OTA_Artifacts') 'program_factory' $commands -RequireVerification
    foreach ($check in @(@('08000000',$boot),@('08001800',$app))) {
        $expected='{0:X8}/{1:X8}' -f $check[1].Stack,$check[1].Reset
        $actual=@(Get-MathisVectorReadbacks $text $check[0])
        if (($actual.Count -ne 1) -or ($actual[0] -ne $expected)) { throw "Vector readback mismatch at $($check[0])." }
    }
    Write-Host 'Factory image programmed and verified.'
} finally { Exit-MathisBuild $lock }
