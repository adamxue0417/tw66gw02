param(
    [ValidateRange(0,255)][int]$FactoryVersion = 100,
    [ValidateRange(0,255)][int]$OtaVersion = 101,
    [string]$KeilRoot = 'C:\Keil_v5'
)

$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
$outputDir = Join-Path $projectRoot 'OTA_Artifacts'
. (Join-Path $projectRoot 'tools\Build.Common.ps1')
$lock = Enter-MathisBuild $projectRoot
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
try {
    & (Join-Path $projectRoot 'Bootloader\build_bootloader.ps1') -KeilRoot $KeilRoot

    $factory = Invoke-MathisKeilBuild $projectRoot $KeilRoot 'app' $FactoryVersion "v$FactoryVersion"
    Copy-Item -LiteralPath $factory.Bin -Destination (Join-Path $outputDir "mathis_app_v$FactoryVersion.bin") -Force
    Copy-Item -LiteralPath $factory.Hex -Destination (Join-Path $outputDir "mathis_app_v$FactoryVersion.hex") -Force
    $factory.Bin = Join-Path $outputDir "mathis_app_v$FactoryVersion.bin"
    $factory.Hex = Join-Path $outputDir "mathis_app_v$FactoryVersion.hex"
    if ($OtaVersion -eq $FactoryVersion) { $ota = $factory }
    else {
        $ota = Invoke-MathisKeilBuild $projectRoot $KeilRoot 'app' $OtaVersion "v$OtaVersion"
        Copy-Item -LiteralPath $ota.Bin -Destination (Join-Path $outputDir "mathis_app_v$OtaVersion.bin") -Force
        Copy-Item -LiteralPath $ota.Hex -Destination (Join-Path $outputDir "mathis_app_v$OtaVersion.hex") -Force
        $ota.Bin = Join-Path $outputDir "mathis_app_v$OtaVersion.bin"
        $ota.Hex = Join-Path $outputDir "mathis_app_v$OtaVersion.hex"
    }

    $bootHex = Join-Path $projectRoot 'Bootloader\build\mathis_bootloader.hex'
    $bootBin = Join-Path $projectRoot 'Bootloader\build\mathis_bootloader.bin'
    $factoryHex = Join-Path $outputDir "mathis_factory_v$FactoryVersion.hex"
    Merge-MathisHex $bootHex $factory.Hex $factoryHex

    [byte[]]$bootBytes = [IO.File]::ReadAllBytes($bootBin)
    [byte[]]$factoryAppBytes = [IO.File]::ReadAllBytes($factory.Bin)
    [byte[]]$factoryBytes = Join-MathisFactory $bootBytes $factoryAppBytes
    $factoryBin = Join-Path $outputDir "mathis_factory_v$FactoryVersion.bin"
    [IO.File]::WriteAllBytes($factoryBin, $factoryBytes)

    [byte[]]$appBytes = [IO.File]::ReadAllBytes($ota.Bin)
    [byte[]]$artifact = New-Object byte[] ($appBytes.Length + 384)
    [Array]::Copy($appBytes, 0, $artifact, 0, $appBytes.Length)
    $otaFile = Join-Path $outputDir "mathis_ota_v$OtaVersion.ota"
    [IO.File]::WriteAllBytes($otaFile, $artifact)
    $crc = Get-MathisCrc32 $artifact
    $sha256Provider = [Security.Cryptography.SHA256]::Create()
    try {
        $sha256 = -join ($sha256Provider.ComputeHash($artifact) | ForEach-Object { $_.ToString('x2') })
    }
    finally {
        $sha256Provider.Dispose()
    }
    $manifest = [ordered]@{
        format = 'mathis-dev-ota-v1'
        warning = 'DEVELOPMENT OTA - NOT FOR PRODUCTION; RSA verification and anti-rollback are disabled'
        target_version = $OtaVersion
        application_size = $appBytes.Length
        signature_size = 384
        artifact_size = $artifact.Length
        crc32_iso_hdlc = ('0x{0:X8}' -f $crc)
        sha256 = $sha256
        application_base = '0x08001800'
        development_signature_placeholder = $true
    }
    $manifestPath = Join-Path $outputDir "mathis_ota_v$OtaVersion.manifest.json"
    $manifest | ConvertTo-Json | Set-Content -Encoding UTF8 $manifestPath

    # Produce a physically separated hand-off tree so a raw application image
    # cannot be confused with either the complete factory image or App OTA file.
    $releaseRoot = Join-Path $outputDir "Release_v${FactoryVersion}_to_v${OtaVersion}"
    $factoryDelivery = Join-Path $releaseRoot 'Factory_Flash'
    $appDelivery = Join-Path $releaseRoot 'App_Upload'
    $debugDelivery = Join-Path $releaseRoot 'Debug_Only'
    New-Item -ItemType Directory -Force -Path $factoryDelivery,$appDelivery,$debugDelivery | Out-Null
    Copy-Item -Force $factoryHex,$factoryBin -Destination $factoryDelivery
    Copy-Item -Force $otaFile,$manifestPath -Destination $appDelivery
    Copy-Item -Force $factory.Bin,$factory.Hex,$ota.Bin,$ota.Hex -Destination $debugDelivery

    $deliveryFiles = @(
        Join-Path $factoryDelivery (Split-Path -Leaf $factoryHex)
        Join-Path $factoryDelivery (Split-Path -Leaf $factoryBin)
        Join-Path $appDelivery (Split-Path -Leaf $otaFile)
        Join-Path $appDelivery (Split-Path -Leaf $manifestPath)
    )
    $checksumLines = foreach ($deliveryFile in $deliveryFiles) {
        $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $deliveryFile).Hash.ToLowerInvariant()
        $relativeDeliveryPath = $deliveryFile.Substring($releaseRoot.Length + 1)
        "$hash  $relativeDeliveryPath"
    }
    [IO.File]::WriteAllLines((Join-Path $releaseRoot 'SHA256SUMS.txt'), $checksumLines,
                             [Text.UTF8Encoding]::new($false))
    $releaseNote = @"
Mathis development OTA v$FactoryVersion -> v$OtaVersion
DEVELOPMENT ONLY - RSA verification and anti-rollback are disabled.

Factory_Flash: full-chip erase, then program mathis_factory_v$FactoryVersion.hex with Keil.
App_Upload: bundle mathis_ota_v$OtaVersion.ota and use the adjacent manifest values.
Debug_Only: relocated application-only images; never use these as factory images or App artifacts.
"@
    [IO.File]::WriteAllText((Join-Path $releaseRoot 'README.txt'), $releaseNote,
                            [Text.UTF8Encoding]::new($false))
    Write-Host "Factory image: $factoryHex"
    Write-Host "Factory binary: $factoryBin (program at 0x08000000)"
    Write-Host "OTA artifact: $otaFile ($($artifact.Length) bytes, CRC $('0x{0:X8}' -f $crc))"
    Write-Host "Separated delivery: $releaseRoot"
}
finally { Exit-MathisBuild $lock }
