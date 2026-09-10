param(
    [string]$KeilRoot = 'C:\Keil_v5',
    [string]$KeyRoot = (Join-Path (Split-Path (Split-Path $PSScriptRoot -Parent) -Parent) 'snapshot-12_secure-ota\private_keys')
)
$ErrorActionPreference='Stop'
$root=$PSScriptRoot; $out=Join-Path $root 'OTA_Artifacts'; $bootBuild=Join-Path $root 'Bootloader\build_v2'
$originalV103=Join-Path $out 'mathis_app_v103.bin'; $expectedV103='C3AD66C2302ACA9CFCC19BFD8E77197EB7204CA1FE13DB7CFA4FE37F7869ACAE'
if ((Get-FileHash -Algorithm SHA256 $originalV103).Hash -ne $expectedV103) { throw 'Original 913c2b5 v103 app hash mismatch.' }
& (Join-Path $root 'Bootloader\build_bootloader.ps1') -KeilRoot $KeilRoot -OutputDir $bootBuild
if ($LASTEXITCODE -ne 0) { throw 'Bootloader v2 build failed.' }
Copy-Item -Force (Join-Path $bootBuild 'mathis_bootloader.bin') (Join-Path $out 'mathis_secure_bootloader_dev_v2.bin')
Copy-Item -Force (Join-Path $bootBuild 'mathis_bootloader.hex') (Join-Path $out 'mathis_secure_bootloader_dev_v2.hex')
Copy-Item -Force (Join-Path $bootBuild 'mathis_bootloader.map') (Join-Path $out 'build_secure_bootloader_dev_v2.map')
Copy-Item -Force (Join-Path $bootBuild 'mathis_bootloader.htm') (Join-Path $out 'build_secure_bootloader_dev_v2_callgraph.htm')
& (Join-Path $root 'BuildOtaArtifacts.ps1') -OtaOnly -OtaVersion 104 -KeilRoot $KeilRoot `
  -DevPrivateKey (Join-Path $KeyRoot 'mathis_dev_rsa3072_private.pem') `
  -DevPublicPem (Join-Path $KeyRoot 'mathis_dev_rsa3072_public.pem') `
  -DevPublicDer (Join-Path $KeyRoot 'mathis_dev_rsa3072_public.der')
if ($LASTEXITCODE -ne 0) { throw 'v104 build/sign failed.' }
Copy-Item -Force (Join-Path $root 'MDK-ARM\tw66gw02_ota_v104\tw66gw02.map') (Join-Path $out 'build_v104.map')
Copy-Item -Force (Join-Path $root 'MDK-ARM\tw66gw02_ota_v104\tw66gw02.htm') (Join-Path $out 'build_v104_callgraph.htm')
[byte[]]$boot=[IO.File]::ReadAllBytes((Join-Path $out 'mathis_secure_bootloader_dev_v2.bin'))
[byte[]]$app=[IO.File]::ReadAllBytes($originalV103)
[byte[]]$factory=New-Object byte[] (0x2000+$app.Length)
for($index=0;$index-lt$factory.Length;$index++){$factory[$index]=0xFF}
[Array]::Copy($boot,0,$factory,0,$boot.Length); [Array]::Copy($app,0,$factory,0x2000,$app.Length)
[IO.File]::WriteAllBytes((Join-Path $out 'mathis_secure_factory_v103_bootv2_dev.bin'),$factory)
$lines=[Collections.Generic.List[string]]::new()
foreach($line in Get-Content (Join-Path $out 'mathis_secure_bootloader_dev_v2.hex')){if($line-ne':00000001FF'){$lines.Add($line)}}
foreach($line in Get-Content (Join-Path $out 'mathis_app_v103.hex')){if($line-ne':00000001FF'){$lines.Add($line)}}
$lines.Add(':00000001FF'); [IO.File]::WriteAllLines((Join-Path $out 'mathis_secure_factory_v103_bootv2_dev.hex'),$lines,[Text.Encoding]::ASCII)
Write-Host 'v104 delivery build complete; private/public key files were referenced in place and not copied.'
