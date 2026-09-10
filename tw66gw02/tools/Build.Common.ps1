# Shared Windows PowerShell 5.1 build and artifact helpers. Dot-sourcing has no side effects.
function Get-MathisToolchain([string]$KeilRoot) {
    $bin = Join-Path $KeilRoot 'ARM\ARM_Compiler_5.06u7\Bin'
    $result = @{ UV4 = (Join-Path $KeilRoot 'UV4\UV4.exe') }
    foreach ($name in @('armcc','armasm','armlink','fromelf')) { $result[$name] = Join-Path $bin "$name.exe" }
    foreach ($path in $result.Values) {
        if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Build tool not found: $path" }
    }
    return $result
}

function Invoke-MathisNative([string]$Executable, [string[]]$Arguments, [string]$Operation) {
    if (!(Test-Path -LiteralPath $Executable -PathType Leaf)) { throw "Tool not found: $Executable" }
    & $Executable @Arguments | ForEach-Object { Write-Host $_ }
    $code = $LASTEXITCODE
    if ($code -ne 0) { throw "$Operation failed (exit $code)." }
}

function Enter-MathisBuild([string]$ProjectRoot) {
    $hash = [Security.Cryptography.SHA256]::Create()
    try {
        $key = [BitConverter]::ToString($hash.ComputeHash([Text.Encoding]::UTF8.GetBytes(
            [IO.Path]::GetFullPath($ProjectRoot).ToUpperInvariant()))).Replace('-', '')
    } finally { $hash.Dispose() }
    $mutex = [Threading.Mutex]::new($false, "Local\MathisBuild_$key")
    try {
        try { $acquired = $mutex.WaitOne(120000) }
        catch [Threading.AbandonedMutexException] { $acquired = $true }
        if (!$acquired) { throw 'Another build is using this project; timed out after 120 seconds.' }
        return $mutex
    } catch { $mutex.Dispose(); throw }
}

function Exit-MathisBuild($Mutex) {
    if ($null -ne $Mutex) { try { $Mutex.ReleaseMutex() } finally { $Mutex.Dispose() } }
}

function New-MathisProject([string]$TemplatePath, [string]$Destination, [string]$BuildDirectory,
                          [ValidateSet('factory','app','direct')][string]$Kind, [int]$Version) {
    $xml = [xml]::new()
    $xml.PreserveWhitespace = $true
    $xml.Load($TemplatePath)
    $target = $xml.SelectSingleNode('/Project/Targets/Target[TargetName="tw66gw02"]')
    if ($null -eq $target) { throw 'Expected exactly the tw66gw02 Keil target.' }
    $output = $target.SelectSingleNode('TargetOption/TargetCommonOption/OutputDirectory')
    $scatter = $target.SelectSingleNode('TargetOption/TargetArmAds/LDads/ScatterFile')
    $define = $target.SelectSingleNode('TargetOption/TargetArmAds/Cads/VariousControls/Define')
    if (($null -eq $output) -or ($null -eq $scatter) -or ($null -eq $define)) { throw 'Unsupported Keil template structure.' }
    $output.InnerText = $BuildDirectory.TrimEnd('\') + '\'
    foreach ($hook in $target.SelectNodes('.//BeforeMake | .//AfterMake')) {
        foreach ($node in $hook.ChildNodes) {
            if ($node.Name -like 'RunUserProg*') { $node.InnerText = '0' }
        }
    }
    $defines = @($define.InnerText -split ',' | Where-Object { $_ -notmatch '^\s*MATHIS_FW_VERSION=' })
    $define.InnerText = ($defines + "MATHIS_FW_VERSION=$Version") -join ','
    $scatterName = @{ factory='factory'; app='app'; direct='direct' }[$Kind]
    $scatter.InnerText = ".\tw66gw02_$scatterName.sct"
    if ($Kind -ne 'factory') {
        foreach ($file in @($target.SelectNodes('.//File[FileName="boot_image.o"]'))) { $null = $file.ParentNode.RemoveChild($file) }
    }
    # UV4's legacy XML importer rejects the UTF-8 BOM emitted by XmlDocument.Save(path).
    $settings = [Xml.XmlWriterSettings]::new()
    $settings.Encoding = [Text.UTF8Encoding]::new($false)
    $writer = [Xml.XmlWriter]::Create($Destination,$settings)
    try { $xml.Save($writer) } finally { $writer.Dispose() }
}

function Wait-MathisBuildLog([string]$Log, [int]$TimeoutSeconds = 120) {
    $watch = [Diagnostics.Stopwatch]::StartNew()
    $lastLength = -1L; $lastWrite = [DateTime]::MinValue; $reads = 0
    do {
        if (Test-Path -LiteralPath $Log) {
            $info = Get-Item -LiteralPath $Log
            if (($info.Length -ne $lastLength) -or ($info.LastWriteTimeUtc -ne $lastWrite)) {
                $lastLength = $info.Length; $lastWrite = $info.LastWriteTimeUtc
                $content = Get-Content -LiteralPath $Log -Raw -Encoding Default
                $reads++
                if ($content -match 'Build Time Elapsed:') {
                    if ($content -notmatch '(?m)\b0 Error\(s\)') { throw "Keil compilation failed. See $Log" }
                    return @{ Text=$content; Reads=$reads }
                }
            }
        }
        Start-Sleep -Milliseconds 250
    } while ($watch.Elapsed.TotalSeconds -lt $TimeoutSeconds)
    throw "Keil build timed out without a complete success log. See $Log"
}

function Invoke-MathisKeilBuild([string]$ProjectRoot, [string]$KeilRoot, [string]$Kind,
                              [int]$Version, [string]$Label) {
    $tool = Get-MathisToolchain $KeilRoot
    $mdk = Join-Path $ProjectRoot 'MDK-ARM'
    $artifactDir = Join-Path $ProjectRoot 'OTA_Artifacts'
    # Keep paths short for ARM Compiler 5 / legacy UV4 command-line limits.
    $runId = [Guid]::NewGuid().ToString('N').Substring(0,12)
    $runDir = Join-Path $mdk "runs\$runId"
    $project = Join-Path $mdk "r_$runId.uvprojx"
    $log = Join-Path $artifactDir "r_$runId.log"
    $watch = [Diagnostics.Stopwatch]::StartNew()
    New-Item -ItemType Directory -Force -Path $runDir,$artifactDir | Out-Null
    try {
        New-MathisProject (Join-Path $mdk 'tw66gw02.uvprojx') $project ".\runs\$runId" $Kind $Version
        $arguments = '-r "{0}" -t tw66gw02 -j0 -o "{1}"' -f $project,$log
        $process = Start-Process -FilePath $tool.UV4 -ArgumentList $arguments -WindowStyle Hidden -PassThru
        $exitCode = 0
        if ($process.WaitForExit(1000)) { $exitCode = $process.ExitCode }
        # UV4 may hand off to an existing instance and return 1 before completion.
        if ($exitCode -notin @(0,1)) { throw "Keil failed to launch (exit $exitCode). See $log" }
        $completion = Wait-MathisBuildLog $log
        $axf = Join-Path $runDir 'tw66gw02.axf'
        $hex = Join-Path $runDir 'tw66gw02.hex'
        foreach ($path in @($axf,$hex)) {
            if (!(Test-Path -LiteralPath $path -PathType Leaf) -or ((Get-Item -LiteralPath $path).Length -eq 0)) {
                throw "Keil reported success without a new artifact: $path"
            }
        }
        $bin = Join-Path $runDir 'tw66gw02.bin'
        $binOption = if ($Kind -eq 'factory') { '--bincombined' } else { '--bin' }
        Invoke-MathisNative $tool.fromelf @($binOption,'--output',$bin,$axf) 'BIN generation'
        $size = (Get-Item -LiteralPath $bin).Length
        $limit = if ($Kind -eq 'factory') { 0x1800 + 0x6A80 } else { 0x6A80 }
        if (($size -lt 8) -or ($size -gt $limit)) { throw "Invalid $Kind binary size: $size (limit $limit)." }
        $publishedName = switch ($Kind) { 'factory' {'tw66gw02'} 'direct' {'tw66gw02_direct'} default {"tw66gw02_ota_v$Version"} }
        $published = Join-Path $mdk $publishedName
        New-Item -ItemType Directory -Force -Path $published | Out-Null
        foreach ($extension in @('axf','hex','bin','map','htm','build_log.htm')) {
            $path = Join-Path $runDir "tw66gw02.$extension"
            if (Test-Path -LiteralPath $path) { Copy-Item -LiteralPath $path -Destination $published -Force }
        }
        Copy-Item -LiteralPath $log -Destination (Join-Path $artifactDir "build_$Label.log") -Force
        [ordered]@{ kind=$Kind; version=$Version; elapsed_seconds=$watch.Elapsed.TotalSeconds;
                    log_reads=$completion.Reads; size=$size; run_directory=$runDir } |
            ConvertTo-Json | Set-Content -LiteralPath (Join-Path $runDir 'metrics.json') -Encoding UTF8
        return @{ Bin=$bin; Hex=$hex; Axf=(Join-Path $published 'tw66gw02.axf'); Size=$size; RunDirectory=$runDir }
    } finally {
        # A handed-off UV4 can still need its project after a timeout. Retain it then.
        if ((Test-Path -LiteralPath $log) -and ((Get-Content -LiteralPath $log -Raw) -match 'Build Time Elapsed:')) {
            Remove-Item -LiteralPath $project -Force -ErrorAction SilentlyContinue
        }
    }
}

function Get-MathisCrc32([byte[]]$Bytes) {
    if ($null -eq $script:MathisCrcTable) {
        $script:MathisCrcTable = [uint32[]]::new(256)
        for ($i=0; $i -lt 256; $i++) {
            [uint32]$entry = $i
            for ($bit=0; $bit -lt 8; $bit++) {
                $entry = if (($entry -band 1) -ne 0) { ($entry -shr 1) -bxor [uint32]3988292384 } else { $entry -shr 1 }
            }
            $script:MathisCrcTable[$i] = $entry
        }
    }
    [uint32]$crc = [uint32]::MaxValue
    foreach ($value in $Bytes) { $crc = ($crc -shr 8) -bxor $script:MathisCrcTable[($crc -bxor $value) -band 255] }
    return [uint32]($crc -bxor [uint32]::MaxValue)
}

function Read-MathisHex([string]$Path) {
    $data = [Collections.Generic.SortedDictionary[uint32,byte]]::new()
    [uint32]$upper = 0; $ended = $false; $lineNumber = 0
    $reader=[IO.StreamReader]::new($Path)
    try {
    while ($null -ne ($line=$reader.ReadLine())) {
        $lineNumber++
        if ($ended -or ($line -notmatch '^:([0-9A-Fa-f]{2}){5,}$')) { throw "Invalid HEX record at ${Path}:$lineNumber" }
        $record = [byte[]]::new(($line.Length - 1) / 2)
        [int]$sum = 0
        for ($i=0; $i -lt $record.Length; $i++) { $record[$i]=[Convert]::ToByte($line.Substring(1+2*$i,2),16); $sum+=$record[$i] }
        $count=[int]$record[0]; $address=([int]$record[1] -shl 8) -bor $record[2]; $kind=$record[3]
        if (($record.Length -ne $count+5) -or (($sum -band 255) -ne 0)) { throw "HEX length/checksum error at ${Path}:$lineNumber" }
        switch ($kind) {
            0 {
                if ($address + $count -gt 65536) { throw 'HEX data crosses its address segment.' }
                for ($i=0; $i -lt $count; $i++) {
                    [uint64]$absolute = [uint64]$upper + $address + $i
                    if ($absolute -gt [uint32]::MaxValue) { throw 'HEX address overflow.' }
                    [uint32]$key=$absolute
                    if ($data.ContainsKey($key) -and ($data[$key] -ne $record[4+$i])) { throw 'Conflicting HEX data.' }
                    $data[$key]=$record[4+$i]
                }
            }
            1 { if (($count -ne 0) -or ($address -ne 0)) { throw 'Invalid HEX EOF.' }; $ended=$true }
            2 { if (($count -ne 2) -or ($address -ne 0)) { throw 'Invalid HEX segment.' }; $upper=(([uint32]$record[4] -shl 8) -bor $record[5]) -shl 4 }
            4 { if (($count -ne 2) -or ($address -ne 0)) { throw 'Invalid HEX upper address.' }; $upper=(([uint32]$record[4] -shl 8) -bor $record[5]) -shl 16 }
            { $_ -in 3,5 } { if (($count -ne 4) -or ($address -ne 0)) { throw 'Invalid HEX entry record.' } }
            default { throw "Unsupported HEX record type $kind." }
        }
    }
    } finally { $reader.Dispose() }
    if (!$ended -or ($data.Count -eq 0)) { throw "HEX is empty or missing EOF: $Path" }
    return ,$data
}

function Merge-MathisHex([string]$First, [string]$Second, [string]$Destination) {
    $firstData = Read-MathisHex $First; $secondData = Read-MathisHex $Second
    foreach ($pair in $firstData.GetEnumerator()) {
        if (($pair.Key -lt 0x08000000) -or ($pair.Key -ge 0x08001800)) { throw 'Boot HEX exceeds boot region.' }
    }
    foreach ($pair in $secondData.GetEnumerator()) {
        if (($pair.Key -lt 0x08001800) -or ($pair.Key -ge 0x08008280)) { throw 'Application HEX exceeds application region.' }
        if ($firstData.ContainsKey($pair.Key) -and $firstData[$pair.Key] -ne $pair.Value) { throw 'Factory HEX address conflict.' }
    }
    $lines = [Collections.Generic.List[string]]::new()
    # Each input starts in address segment zero. Explicitly reset between files.
    foreach ($path in @($First,$Second)) {
        $lines.Add(':020000040000FA')
        foreach ($line in [IO.File]::ReadLines($path)) {
            $kind=[Convert]::ToInt32($line.Substring(7,2),16)
            if ($kind -notin @(1,3,5)) { $lines.Add($line) }
        }
    }
    $lines.Add(':00000001FF')
    [IO.File]::WriteAllLines($Destination,$lines,[Text.Encoding]::ASCII)
}

function Join-MathisFactory([byte[]]$Boot, [byte[]]$Application) {
    if (($Boot.Length -lt 8) -or ($Boot.Length -gt 0x1800) -or
        ($Application.Length -lt 192) -or ($Application.Length -gt 0x6A80)) { throw 'Invalid factory component size.' }
    $result = [byte[]]::new(0x1800 + $Application.Length)
    for ($i=$Boot.Length; $i -lt 0x1800; $i++) { $result[$i]=255 }
    [Array]::Copy($Boot,0,$result,0,$Boot.Length)
    [Array]::Copy($Application,0,$result,0x1800,$Application.Length)
    return ,$result
}

function Invoke-MathisSmallBuild([string]$ProjectRoot, [string]$KeilRoot, [string]$Source,
                               [string]$OutputDir, [string]$Name, [int]$Optimization = 3,
                               [switch]$WithUtilities) {
    $tool = Get-MathisToolchain $KeilRoot
    New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
    $startup = Join-Path $OutputDir 'startup.o'
    Invoke-MathisNative $tool.armasm @('--cpu','Cortex-M0','--pd','__MICROLIB SETA 0','--xref','--debug',
        '-o',$startup,(Join-Path $ProjectRoot 'MDK-ARM\startup_stm32f030x8.s')) 'Startup assembly'
    $objects = [Collections.Generic.List[string]]::new(); $objects.Add($startup)
    $sources = @($Source)
    if ($WithUtilities) { $sources += Join-Path $ProjectRoot 'BSP\mathis_util.c' }
    foreach ($sourcePath in $sources) {
        $object = Join-Path $OutputDir ([IO.Path]::GetFileNameWithoutExtension($sourcePath) + '.o')
        $arguments = @('-c','--cpu','Cortex-M0','--c99',"-O$Optimization",'--split_sections','--debug','-DSTM32F030x8',
            '-I',(Join-Path $ProjectRoot 'Drivers\CMSIS\Device\ST\STM32F0xx\Include'),
            '-I',(Join-Path $ProjectRoot 'Drivers\CMSIS\Include'),'-I',(Join-Path $ProjectRoot 'BSP'),'-o',$object,$sourcePath)
        if ($Optimization -eq 3) { $arguments += '-Ospace' }
        Invoke-MathisNative $tool.armcc $arguments 'C compilation'
        $objects.Add($object)
    }
    $axf=Join-Path $OutputDir "$Name.axf"
    Invoke-MathisNative $tool.armlink (@('--cpu','Cortex-M0','--strict','--summary_stderr',
        '--scatter',(Join-Path $ProjectRoot 'Bootloader\bootloader.sct'),'-o',$axf) + $objects.ToArray()) 'Link'
    foreach ($format in @(@('--bin','bin'),@('--i32combined','hex'))) {
        Invoke-MathisNative $tool.fromelf @($format[0],'--output',(Join-Path $OutputDir "$Name.$($format[1])"),$axf) 'Image generation'
    }
    $size=(Get-Item -LiteralPath (Join-Path $OutputDir "$Name.bin")).Length
    if (($size -lt 8) -or ($size -gt 0x1800)) { throw "Invalid small-image size: $size (limit 6144)." }
    Write-Host "$Name built: $size bytes"
}
