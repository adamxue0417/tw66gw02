param(
    [ValidateRange(0,255)][int]$FactoryVersion = 102,
    [ValidateRange(0,255)][int]$OtaVersion = 103,
    [switch]$OtaOnly,
    [string]$KeilRoot = 'C:\Keil_v5',
    [string]$OpenSsl = 'C:\Program Files\Git\usr\bin\openssl.exe',
    [string]$DevPrivateKey = ''
)

$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
$mdkDir = Join-Path $projectRoot 'MDK-ARM'
$outputDir = Join-Path $projectRoot 'OTA_Artifacts'
$generatedProject = Join-Path $mdkDir 'tw66gw02_ota.generated.uvprojx'
$uv4 = Join-Path $KeilRoot 'UV4\UV4.exe'
$fromelf = Join-Path $KeilRoot 'ARM\ARM_Compiler_5.06u7\Bin\fromelf.exe'
if ([string]::IsNullOrWhiteSpace($DevPrivateKey)) {
    $DevPrivateKey = Join-Path (Split-Path -Parent $projectRoot) 'private_keys\mathis_dev_rsa3072_private.pem'
}
$devPublicPem = Join-Path (Split-Path -Parent $projectRoot) 'private_keys\mathis_dev_rsa3072_public.pem'
$devPublicDer = Join-Path (Split-Path -Parent $projectRoot) 'private_keys\mathis_dev_rsa3072_public.der'
if (!(Test-Path -LiteralPath $OpenSsl)) { throw "OpenSSL not found: $OpenSsl" }
if (!(Test-Path -LiteralPath $DevPrivateKey)) { throw "DEV private key not found: $DevPrivateKey" }
if (!(Test-Path -LiteralPath $devPublicPem) -or !(Test-Path -LiteralPath $devPublicDer)) {
    throw 'DEV public key files are missing.'
}

New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

function Invoke-AppBuild([int]$Version, [string]$Label, [string]$ArtifactBase) {
    $template = Get-Content (Join-Path $mdkDir 'tw66gw02.uvprojx') -Raw -Encoding UTF8
    $temporaryOutput = ".\tw66gw02_ota_$Label\"
    $template = $template.Replace('<OutputDirectory>.\tw66gw02\</OutputDirectory>',
                                  "<OutputDirectory>$temporaryOutput</OutputDirectory>")
    $template = $template.Replace('<ListingPath></ListingPath>',
                                  "<ListingPath>$temporaryOutput</ListingPath>")
    $template = $template.Replace(
        '<Define>USE_HAL_DRIVER,STM32F030x8</Define>',
        "<Define>USE_HAL_DRIVER,STM32F030x8,MATHIS_FW_VERSION=$Version</Define>")
    $template = $template.Replace('.\tw66gw02_factory.sct', '.\tw66gw02_app.sct')
    $bootObjectEntry = @'
            <File>
              <FileName>boot_image.o</FileName>
              <FileType>3</FileType>
              <FilePath>boot_image.o</FilePath>
            </File>
'@
    $template = $template.Replace($bootObjectEntry, '')
    $disabledBeforeMake = @'
          <BeforeMake>
            <RunUserProg1>0</RunUserProg1>
            <RunUserProg2>0</RunUserProg2>
            <UserProg1Name></UserProg1Name>
            <UserProg2Name></UserProg2Name>
            <UserProg1Dos16Mode>0</UserProg1Dos16Mode>
            <UserProg2Dos16Mode>0</UserProg2Dos16Mode>
            <nStopB1X>0</nStopB1X>
            <nStopB2X>0</nStopB2X>
          </BeforeMake>
'@
    $template = [regex]::Replace($template, '(?s)          <BeforeMake>.*?          </BeforeMake>',
                                 $disabledBeforeMake, 1)
    $disabledAfterMake = $disabledBeforeMake.Replace('BeforeMake', 'AfterMake')
    $template = [regex]::Replace($template, '(?s)          <AfterMake>.*?          </AfterMake>',
                                 $disabledAfterMake, 1)
    [IO.File]::WriteAllText($generatedProject, $template, [Text.UTF8Encoding]::new($false))
    $log = Join-Path $outputDir "build_$Label.log"
    if (Test-Path -LiteralPath $log) { Remove-Item -LiteralPath $log -Force }
    $uvArguments = @('-r', ('"{0}"' -f $generatedProject), '-t', 'tw66gw02',
                     '-j0', '-o', ('"{0}"' -f $log))
    $uvProcess = Start-Process -FilePath $uv4 -ArgumentList $uvArguments -WindowStyle Hidden -Wait -PassThru
    $deadline = [DateTime]::UtcNow.AddSeconds(120)
    do {
        Start-Sleep -Milliseconds 250
        if (Test-Path -LiteralPath $log) {
            $logText = Get-Content $log -Raw -Encoding Default
            if ($logText -match 'Build Time Elapsed:') { break }
        }
    } while ([DateTime]::UtcNow -lt $deadline)
    if (!(Test-Path -LiteralPath $log) -or ($logText -notmatch 'Build Time Elapsed:')) {
        throw "Keil build $Label did not produce a completed log (exit $($uvProcess.ExitCode))."
    }
    if ($logText -notmatch '0 Error\(s\)') { throw "Application build $Label failed. See $log" }
    $temporaryOutputDir = Join-Path $mdkDir "tw66gw02_ota_$Label"
    $axf = Join-Path $temporaryOutputDir 'tw66gw02.axf'
    $bin = Join-Path $outputDir "$ArtifactBase.bin"
    $hex = Join-Path $outputDir "$ArtifactBase.hex"
    & $fromelf --bin --output $bin $axf
    if ($LASTEXITCODE -ne 0) { throw "BIN generation for $Label failed." }
    Copy-Item -Force (Join-Path $temporaryOutputDir 'tw66gw02.hex') $hex
    $size = (Get-Item $bin).Length
    if ($size -gt 0x6280) { throw "Application $Label is $size bytes; maximum is 25216 bytes." }
    return @{ Bin = $bin; Hex = $hex; Size = $size }
}

function Get-Crc32([byte[]]$Bytes) {
    [uint32]$crc = [uint32]::MaxValue
    foreach ($value in $Bytes) {
        $crc = $crc -bxor [uint32]$value
        for ($bit = 0; $bit -lt 8; $bit++) {
            if (($crc -band 1) -ne 0) { $crc = ($crc -shr 1) -bxor [uint32]3988292384 }
            else { $crc = $crc -shr 1 }
        }
    }
    return [uint32]($crc -bxor [uint32]::MaxValue)
}

function Merge-IntelHex([string]$First, [string]$Second, [string]$Destination) {
    $lines = [Collections.Generic.List[string]]::new()
    foreach ($line in (Get-Content $First)) { if ($line -ne ':00000001FF') { $lines.Add($line) } }
    foreach ($line in (Get-Content $Second)) { if ($line -ne ':00000001FF') { $lines.Add($line) } }
    $lines.Add(':00000001FF')
    [IO.File]::WriteAllLines($Destination, $lines, [Text.Encoding]::ASCII)
}

try {
    $legacyFactory = Join-Path $outputDir 'mathis_factory_v100.bin'
    $legacyFactoryHash = if (Test-Path -LiteralPath $legacyFactory) {
        (Get-FileHash -Algorithm SHA256 -LiteralPath $legacyFactory).Hash
    } else { $null }
    if (!$OtaOnly) {
        & (Join-Path $projectRoot 'Bootloader\build_bootloader.ps1') -KeilRoot $KeilRoot
        if ($LASTEXITCODE -ne 0) { throw 'Bootloader build failed.' }
        $factory = Invoke-AppBuild $FactoryVersion "secure_v$FactoryVersion" "mathis_secure_app_v$FactoryVersion"

        $bootHex = Join-Path $projectRoot 'Bootloader\build\mathis_bootloader.hex'
        $bootBin = Join-Path $projectRoot 'Bootloader\build\mathis_bootloader.bin'
        $factoryHex = Join-Path $outputDir "mathis_secure_factory_v${FactoryVersion}_dev.hex"
        Merge-IntelHex $bootHex $factory.Hex $factoryHex

        [byte[]]$bootBytes = [IO.File]::ReadAllBytes($bootBin)
        [byte[]]$factoryAppBytes = [IO.File]::ReadAllBytes($factory.Bin)
        [byte[]]$factoryBytes = New-Object byte[] (0x2000 + $factoryAppBytes.Length)
        for ($index = 0; $index -lt $factoryBytes.Length; $index++) { $factoryBytes[$index] = 0xFF }
        [Array]::Copy($bootBytes, 0, $factoryBytes, 0, $bootBytes.Length)
        [Array]::Copy($factoryAppBytes, 0, $factoryBytes, 0x2000, $factoryAppBytes.Length)
        $factoryBin = Join-Path $outputDir "mathis_secure_factory_v${FactoryVersion}_dev.bin"
        [IO.File]::WriteAllBytes($factoryBin, $factoryBytes)

        Copy-Item -Force $bootBin (Join-Path $outputDir 'mathis_secure_bootloader_dev_v1.bin')
        Copy-Item -Force $bootHex (Join-Path $outputDir 'mathis_secure_bootloader_dev_v1.hex')
    }

    $ota = Invoke-AppBuild $OtaVersion "v$OtaVersion" "mathis_app_v$OtaVersion"

    [byte[]]$appBytes = [IO.File]::ReadAllBytes($ota.Bin)
    $signatureFile = Join-Path $outputDir "mathis_app_v$OtaVersion.signature.bin"
    & $OpenSsl dgst -sha256 -sign $DevPrivateKey -out $signatureFile $ota.Bin
    if ($LASTEXITCODE -ne 0) { throw 'DEV signing failed.' }
    if ((Get-Item -LiteralPath $signatureFile).Length -ne 384) { throw 'RSA-3072 signature is not 384 bytes.' }
    & $OpenSsl dgst -sha256 -verify $devPublicPem -signature $signatureFile $ota.Bin | Out-Null
    if ($LASTEXITCODE -ne 0) { throw 'Host-side signature verification failed.' }
    [byte[]]$signatureBytes = [IO.File]::ReadAllBytes($signatureFile)
    [byte[]]$artifact = New-Object byte[] ($appBytes.Length + $signatureBytes.Length)
    [Array]::Copy($appBytes, 0, $artifact, 0, $appBytes.Length)
    [Array]::Copy($signatureBytes, 0, $artifact, $appBytes.Length, $signatureBytes.Length)
    $otaFile = Join-Path $outputDir "mathis_ota_v${OtaVersion}_dev_signed.ota"
    [IO.File]::WriteAllBytes($otaFile, $artifact)
    $crc = Get-Crc32 $artifact
    $manifest = [ordered]@{
        format = 'mathis-signed-ota-v1'
        target_version = $OtaVersion
        security_version = $OtaVersion
        application_size = $appBytes.Length
        signature_size = 384
        artifact_size = $artifact.Length
        crc32_iso_hdlc = ('0x{0:X8}' -f $crc)
        application_sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $ota.Bin).Hash
        artifact_sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $otaFile).Hash
        signature_algorithm = 'RSA-3072-PKCS1-v1_5-SHA256'
        signing_key_class = 'DEV'
        signing_key_fingerprint_sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $devPublicDer).Hash
        application_base = '0x08002000'
        boot_api_version = 1
        development_signature_placeholder = $false
    }
    $manifest | ConvertTo-Json | Set-Content -Encoding UTF8 `
        (Join-Path $outputDir "mathis_ota_v${OtaVersion}_dev_signed.manifest.json")
    Remove-Item -LiteralPath $signatureFile -Force
    if (($legacyFactoryHash -ne $null) -and
        ((Get-FileHash -Algorithm SHA256 -LiteralPath $legacyFactory).Hash -ne $legacyFactoryHash)) {
        throw 'Protected mathis_factory_v100.bin changed during secure build.'
    }
    if (!$OtaOnly) {
        Write-Host "Factory image: $factoryHex"
        Write-Host "Factory binary: $factoryBin (program at 0x08000000)"
    }
    Write-Host "OTA artifact: $otaFile ($($artifact.Length) bytes, CRC $('0x{0:X8}' -f $crc))"
}
finally {
    if (Test-Path -LiteralPath $generatedProject) { Remove-Item -LiteralPath $generatedProject -Force }
}
