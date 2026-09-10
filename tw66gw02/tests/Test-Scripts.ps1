param([string]$KeilRoot = 'C:\Keil_v5')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
. (Join-Path $root 'tools\Build.Common.ps1')
. (Join-Path $root 'tools\JLink.Common.ps1')
$output=Join-Path $PSScriptRoot 'build\script-tests'
New-Item -ItemType Directory -Force -Path $output | Out-Null
$script:checks=0
function Assert-True([bool]$Condition,[string]$Message) {
    $script:checks++
    if (!$Condition) { throw "FAILED: $Message" }
}
function Assert-Throws([scriptblock]$Action,[string]$Message) {
    $thrown=$false
    try { & $Action | Out-Null } catch { $thrown=$true }
    Assert-True $thrown $Message
}

# Source parsing must succeed under the interpreter used by Keil hooks.
$files=@(Get-ChildItem -LiteralPath $root -Recurse -Filter *.ps1 | Where-Object { $_.FullName -notlike '*\tests\build\*' })
$files+=Get-Item (Join-Path (Split-Path $root) 'build_mathis_test_workbook.ps1'),(Join-Path (Split-Path $root) 'render_mathis_preview.ps1')
foreach ($file in $files) {
    $tokens=$null; $errors=$null
    $null=[System.Management.Automation.Language.Parser]::ParseFile($file.FullName,[ref]$tokens,[ref]$errors)
    Assert-True ($errors.Count -eq 0) "Parse $($file.Name)"
}
Assert-True ((Get-MathisCrc32 ([Text.Encoding]::ASCII.GetBytes('123456789'))) -eq [uint32]3421780262) 'CRC known vector'
Assert-True ((Get-MathisCrc32 ([byte[]]::new(0))) -eq 0) 'CRC empty'
Assert-Throws { Get-MathisToolchain (Join-Path $output 'missing') } 'Missing toolchain'
Assert-Throws { Invoke-MathisNative (Join-Path $output 'missing.exe') @() 'Missing tool' } 'Missing executable'
Assert-Throws { Invoke-MathisNative "$env:SystemRoot\System32\cmd.exe" @('/d','/c','exit','7') 'Test failure' } 'Nonzero process exit'

$xmlPath=Join-Path $output 'generated.uvprojx'
New-MathisProject (Join-Path $root 'MDK-ARM\tw66gw02.uvprojx') $xmlPath '.\with space\build' 'app' 203
$xml=[xml](Get-Content -LiteralPath $xmlPath -Raw)
Assert-True ($xml.SelectSingleNode('//Cads/VariousControls/Define').InnerText -match 'MATHIS_FW_VERSION=203') 'Version override'
Assert-True ($xml.SelectNodes('//File[FileName="boot_image.o"]').Count -eq 0) 'No boot object in app target'
Assert-True ($xml.SelectNodes('//BeforeMake/RunUserProg1[text()="1"] | //AfterMake/RunUserProg1[text()="1"]').Count -eq 0) 'No build hooks'
Assert-True ([IO.File]::ReadAllBytes($xmlPath)[0] -eq 60) 'Keil XML has no BOM'

$log=Join-Path $output 'complete.log'
[IO.File]::WriteAllText($log,'0 Error(s), 0 Warning(s).'+"`r`n"+'Build Time Elapsed: 00:00:01')
$result=Wait-MathisBuildLog $log 1
Assert-True ($result.Reads -eq 1) 'Complete log read once'
[IO.File]::WriteAllText($log,'0 Error(s), 0 Warning(s).')
Assert-Throws { Wait-MathisBuildLog $log 1 } 'Incomplete log is not success'
Assert-Throws { Wait-MathisBuildLog (Join-Path $output 'absent.log') 1 } 'Absent log timeout'
[IO.File]::WriteAllText($log,"1 Error(s).`r`nBuild Time Elapsed: 00:00:01")
Assert-Throws { Wait-MathisBuildLog $log 1 } 'Compiler errors'

$hex=Join-Path $output 'valid.hex'
[IO.File]::WriteAllLines($hex,@(':020000040800F2',':020000000102FB',':00000001FF'))
$bytes=Read-MathisHex $hex
Assert-True ($bytes[0x08000001] -eq 2) 'HEX address decoding'
foreach ($records in @(
    @(':020000040800F2',':020000000102FA',':00000001FF'),
    @(':020000040800F2',':020000000102FB'),
    @(':020000040800F2',':020000000102FB',':020000000103FA',':00000001FF'),
    @(':020000040800F2',':020000000102FB',':00000001FF',':00000001FF'),
    @(':030000000102FB',':00000001FF')
)) {
    [IO.File]::WriteAllLines($hex,$records)
    Assert-Throws { Read-MathisHex $hex } 'Malformed/conflicting HEX rejected'
}
Assert-Throws { Join-MathisFactory ([byte[]]::new(6145)) ([byte[]]::new(192)) } 'Oversized boot image'
$factory=Join-MathisFactory ([byte[]]::new(8)) ([byte[]]::new(192))
Assert-True ($factory.Length -eq 6336 -and $factory[8] -eq 255 -and $factory[6144] -eq 0) 'Factory gap padding'
Assert-Throws { Assert-MathisVector ([byte[]]::new(8)) 0 0x08000000 100 } 'Invalid vectors'
$vector=[BitConverter]::GetBytes([uint32]0x20001FF0)+[BitConverter]::GetBytes([uint32]0x080018C1)
$null=Assert-MathisVector $vector 0 0x08001800 256 -Relocated
$rb=@(Get-MathisVectorReadbacks "08001800 = 20001FF0 080018C1`r`n08001800 = 20001FF0 080018C1" '08001800')
Assert-True ($rb.Count -eq 2 -and $rb[0] -eq $rb[1]) 'Before/after vectors'
$unicodeName=([char]0x6D4B).ToString()+([char]0x8BD5).ToString()+' image.bin'
$unicodePath=Join-Path $output $unicodeName
[IO.File]::WriteAllBytes($unicodePath,$vector)
Assert-True ((Resolve-MathisImage $root $unicodePath) -eq $unicodePath) 'Unicode and space image path'

# New command scripts are reviewable without invoking a real J-Link executable.
$invalidPath=Join-Path $output 'bad;command.bin'
[IO.File]::WriteAllBytes($invalidPath,$vector)
Assert-Throws { Resolve-MathisImage $root $invalidPath } 'J-Link delimiter rejected'
$lock1=Enter-MathisBuild $output
try {
    $lock2=Enter-MathisBuild $output
    try { Assert-True ($null -ne $lock2) 'Nested build lock is reentrant' }
    finally { Exit-MathisBuild $lock2 }
    # A separate process must not acquire the same named mutex while this owner holds it.
    $hash=[Security.Cryptography.SHA256]::Create()
    try { $key=[BitConverter]::ToString($hash.ComputeHash([Text.Encoding]::UTF8.GetBytes([IO.Path]::GetFullPath($output).ToUpperInvariant()))).Replace('-','') }
    finally { $hash.Dispose() }
    $probeScript=Join-Path $output 'probe_lock.ps1'
    [IO.File]::WriteAllText($probeScript,('$m=[Threading.Mutex]::new($false,"Local\MathisBuild_'+$key+'"); try { if ($m.WaitOne(0)) { $m.ReleaseMutex(); exit 1 } } finally { $m.Dispose() }'))
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $probeScript
    Assert-True ($LASTEXITCODE -eq 0) 'Concurrent process excluded'
} finally { Exit-MathisBuild $lock1 }
Write-Host "PASS: $script:checks script checks."
