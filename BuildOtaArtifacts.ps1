param(
    [ValidateRange(0,255)][int]$FactoryVersion = 100,
    [ValidateRange(0,255)][int]$OtaVersion = 101,
    [string]$KeilRoot = 'C:\Keil_v5'
)

$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
$mdkDir = Join-Path $projectRoot 'MDK-ARM'
$outputDir = Join-Path $projectRoot 'OTA_Artifacts'
$generatedProject = Join-Path $mdkDir 'tw66gw02_ota.generated.uvprojx'
$uv4 = Join-Path $KeilRoot 'UV4\UV4.exe'
$fromelf = Join-Path $KeilRoot 'ARM\ARM_Compiler_5.06u7\Bin\fromelf.exe'

New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

function Invoke-AppBuild([int]$Version, [string]$Label) {
    $template = Get-Content (Join-Path $mdkDir 'tw66gw02.uvprojx') -Raw -Encoding UTF8
    $temporaryOutput = ".\tw66gw02_ota_$Label\"
    $template = $template.Replace('<OutputDirectory>.\tw66gw02\</OutputDirectory>',
                                  "<OutputDirectory>$temporaryOutput</OutputDirectory>")
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
    [IO.File]::WriteAllText($generatedProject, $template, [Text.UTF8Encoding]::new($false))
    $log = Join-Path $outputDir "build_$Label.log"
    if (Test-Path -LiteralPath $log) { Remove-Item -LiteralPath $log -Force }
    & $uv4 -r $generatedProject -t tw66gw02 -j0 -o $log
    if ($LASTEXITCODE -ne 0) { throw "Application build $Label failed. See $log" }
    $deadline = [DateTime]::UtcNow.AddSeconds(120)
    do {
        Start-Sleep -Milliseconds 250
        if (Test-Path -LiteralPath $log) {
            $logText = Get-Content $log -Raw -Encoding Default
            if ($logText -match 'Build Time Elapsed:') { break }
        }
    } while ([DateTime]::UtcNow -lt $deadline)
    if (!(Test-Path -LiteralPath $log) -or ($logText -notmatch 'Build Time Elapsed:')) {
        throw "Timed out waiting for Keil to build $Label."
    }
    if ($logText -notmatch '0 Error\(s\)') { throw "Application build $Label failed. See $log" }
    $temporaryOutputDir = Join-Path $mdkDir "tw66gw02_ota_$Label"
    $axf = Join-Path $temporaryOutputDir 'tw66gw02.axf'
    $bin = Join-Path $outputDir "mathis_app_$Label.bin"
    $hex = Join-Path $outputDir "mathis_app_$Label.hex"
    & $fromelf --bin --output $bin $axf
    if ($LASTEXITCODE -ne 0) { throw "BIN generation for $Label failed." }
    Copy-Item -Force (Join-Path $temporaryOutputDir 'tw66gw02.hex') $hex
    $size = (Get-Item $bin).Length
    if ($size -gt 0x6A80) { throw "Application $Label is $size bytes; maximum is 27264 bytes." }
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
    & (Join-Path $projectRoot 'Bootloader\build_bootloader.ps1') -KeilRoot $KeilRoot
    if ($LASTEXITCODE -ne 0) { throw 'Bootloader build failed.' }
    $factory = Invoke-AppBuild $FactoryVersion "v$FactoryVersion"
    $ota = Invoke-AppBuild $OtaVersion "v$OtaVersion"

    $bootHex = Join-Path $projectRoot 'Bootloader\build\mathis_bootloader.hex'
    $bootBin = Join-Path $projectRoot 'Bootloader\build\mathis_bootloader.bin'
    $factoryHex = Join-Path $outputDir "mathis_factory_v$FactoryVersion.hex"
    Merge-IntelHex $bootHex $factory.Hex $factoryHex

    [byte[]]$bootBytes = [IO.File]::ReadAllBytes($bootBin)
    [byte[]]$factoryAppBytes = [IO.File]::ReadAllBytes($factory.Bin)
    [byte[]]$factoryBytes = New-Object byte[] (0x1800 + $factoryAppBytes.Length)
    for ($index = 0; $index -lt $factoryBytes.Length; $index++) { $factoryBytes[$index] = 0xFF }
    [Array]::Copy($bootBytes, 0, $factoryBytes, 0, $bootBytes.Length)
    [Array]::Copy($factoryAppBytes, 0, $factoryBytes, 0x1800, $factoryAppBytes.Length)
    $factoryBin = Join-Path $outputDir "mathis_factory_v$FactoryVersion.bin"
    [IO.File]::WriteAllBytes($factoryBin, $factoryBytes)

    [byte[]]$appBytes = [IO.File]::ReadAllBytes($ota.Bin)
    [byte[]]$artifact = New-Object byte[] ($appBytes.Length + 384)
    [Array]::Copy($appBytes, 0, $artifact, 0, $appBytes.Length)
    $otaFile = Join-Path $outputDir "mathis_ota_v$OtaVersion.ota"
    [IO.File]::WriteAllBytes($otaFile, $artifact)
    $crc = Get-Crc32 $artifact
    $manifest = [ordered]@{
        format = 'mathis-dev-ota-v1'
        target_version = $OtaVersion
        application_size = $appBytes.Length
        signature_size = 384
        artifact_size = $artifact.Length
        crc32_iso_hdlc = ('0x{0:X8}' -f $crc)
        application_base = '0x08001800'
        development_signature_placeholder = $true
    }
    $manifest | ConvertTo-Json | Set-Content -Encoding UTF8 `
        (Join-Path $outputDir "mathis_ota_v$OtaVersion.manifest.json")
    Write-Host "Factory image: $factoryHex"
    Write-Host "Factory binary: $factoryBin (program at 0x08000000)"
    Write-Host "OTA artifact: $otaFile ($($artifact.Length) bytes, CRC $('0x{0:X8}' -f $crc))"
}
finally {
    if (Test-Path -LiteralPath $generatedProject) { Remove-Item -LiteralPath $generatedProject -Force }
}
