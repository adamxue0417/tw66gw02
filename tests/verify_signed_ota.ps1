param(
    [string]$OpenSsl = 'C:\Program Files\Git\usr\bin\openssl.exe',
    [string]$PublicKey = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'private_keys\mathis_dev_rsa3072_public.pem')
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$artifactPath = Join-Path $root 'OTA_Artifacts\mathis_ota_v103_dev_signed.ota'
$manifestPath = Join-Path $root 'OTA_Artifacts\mathis_ota_v103_dev_signed.manifest.json'
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
$artifact = [IO.File]::ReadAllBytes($artifactPath)
$appLength = [int]$manifest.application_size
$signatureLength = [int]$manifest.signature_size
if ($artifact.Length -ne ($appLength + $signatureLength)) { throw 'Manifest size mismatch.' }

$tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$testDir = Join-Path $tempRoot ('mathis-v103-signature-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $testDir | Out-Null
try {
    $app = New-Object byte[] $appLength
    $signature = New-Object byte[] $signatureLength
    [Array]::Copy($artifact, 0, $app, 0, $appLength)
    [Array]::Copy($artifact, $appLength, $signature, 0, $signatureLength)
    $appPath = Join-Path $testDir 'app.bin'
    $signaturePath = Join-Path $testDir 'signature.bin'
    [IO.File]::WriteAllBytes($appPath, $app)
    [IO.File]::WriteAllBytes($signaturePath, $signature)

    & $OpenSsl dgst -sha256 -verify $PublicKey -signature $signaturePath $appPath | Out-Host
    if ($LASTEXITCODE -ne 0) { throw 'Valid signature was rejected.' }

    $corruptApp = [byte[]]$app.Clone(); $corruptApp[100] = $corruptApp[100] -bxor 1
    $corruptAppPath = Join-Path $testDir 'corrupt-app.bin'
    [IO.File]::WriteAllBytes($corruptAppPath, $corruptApp)
    $ErrorActionPreference = 'SilentlyContinue'
    & $OpenSsl dgst -sha256 -verify $PublicKey -signature $signaturePath $corruptAppPath 2>&1 | Out-Null
    $negativeExit = $LASTEXITCODE
    $ErrorActionPreference = 'Stop'
    if ($negativeExit -eq 0) { throw 'Modified application was accepted.' }

    $corruptSignature = [byte[]]$signature.Clone(); $corruptSignature[10] = $corruptSignature[10] -bxor 1
    $corruptSignaturePath = Join-Path $testDir 'corrupt-signature.bin'
    [IO.File]::WriteAllBytes($corruptSignaturePath, $corruptSignature)
    $ErrorActionPreference = 'SilentlyContinue'
    & $OpenSsl dgst -sha256 -verify $PublicKey -signature $corruptSignaturePath $appPath 2>&1 | Out-Null
    $negativeExit = $LASTEXITCODE
    $ErrorActionPreference = 'Stop'
    if ($negativeExit -eq 0) { throw 'Modified signature was accepted.' }

    $zeroSignaturePath = Join-Path $testDir 'zero-signature.bin'
    [IO.File]::WriteAllBytes($zeroSignaturePath, (New-Object byte[] $signatureLength))
    $ErrorActionPreference = 'SilentlyContinue'
    & $OpenSsl dgst -sha256 -verify $PublicKey -signature $zeroSignaturePath $appPath 2>&1 | Out-Null
    $negativeExit = $LASTEXITCODE
    $ErrorActionPreference = 'Stop'
    if ($negativeExit -eq 0) { throw 'Zero signature was accepted.' }

    $wrongPrivate = Join-Path $testDir 'wrong-private.pem'
    $wrongPublic = Join-Path $testDir 'wrong-public.pem'
    $ErrorActionPreference = 'SilentlyContinue'
    & $OpenSsl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:3072 -out $wrongPrivate 2>&1 | Out-Null
    $keyExit = $LASTEXITCODE
    $ErrorActionPreference = 'Stop'
    if ($keyExit -ne 0) { throw 'Wrong-key generation failed.' }
    $ErrorActionPreference = 'SilentlyContinue'
    & $OpenSsl pkey -in $wrongPrivate -pubout -out $wrongPublic 2>&1 | Out-Null
    $keyExit = $LASTEXITCODE
    $ErrorActionPreference = 'Stop'
    if ($keyExit -ne 0) { throw 'Wrong public-key generation failed.' }
    $ErrorActionPreference = 'SilentlyContinue'
    & $OpenSsl dgst -sha256 -verify $wrongPublic -signature $signaturePath $appPath 2>&1 | Out-Null
    $negativeExit = $LASTEXITCODE
    $ErrorActionPreference = 'Stop'
    if ($negativeExit -eq 0) { throw 'Signature was accepted by a wrong key.' }

    Write-Host 'PASS: valid signature accepted; modified app, modified signature, zero signature, and wrong key rejected.'
}
finally {
    $resolved = [IO.Path]::GetFullPath($testDir)
    if (!$resolved.StartsWith($tempRoot, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove unexpected path: $resolved"
    }
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
