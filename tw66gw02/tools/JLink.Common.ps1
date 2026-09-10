. (Join-Path $PSScriptRoot 'Build.Common.ps1')

function Resolve-MathisImage([string]$ProjectRoot, [string]$Image) {
    $candidate = if ([IO.Path]::IsPathRooted($Image)) { $Image } else { Join-Path $ProjectRoot $Image }
    $path = (Resolve-Path -LiteralPath $candidate -ErrorAction Stop).Path
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Image is not a file: $path" }
    if ($path -match '[\x00-\x1F";]') { throw 'Image path contains J-Link command delimiters.' }
    return $path
}

function Assert-MathisVector([byte[]]$Image, [int]$Offset, [uint32]$Base, [uint32]$Length, [switch]$Relocated) {
    if (($Offset -lt 0) -or ($Image.Length -lt $Offset+8) -or ($Length -lt 8)) { throw 'Image has no complete vector table.' }
    [uint32]$stack = [BitConverter]::ToUInt32($Image,$Offset)
    [uint32]$reset = [BitConverter]::ToUInt32($Image,$Offset+4)
    [uint32]$ramMin = if ($Relocated) { 0x200000C0 } else { 0x20000004 }
    [uint32]$ramMax = if ($Relocated) { 0x20001FF0 } else { 0x20002000 }
    [uint32]$entry = $reset -band [uint32]4294967294
    if (($stack -lt $ramMin) -or ($stack -gt $ramMax) -or (($stack -band 3) -ne 0) -or
        (($reset -band 1) -eq 0) -or ($entry -lt $Base) -or ([uint64]$entry -ge [uint64]$Base+$Length)) {
        throw ('Invalid image vectors: {0:X8}/{1:X8}.' -f $stack,$reset)
    }
    return @{ Stack=$stack; Reset=$reset }
}

function Get-MathisVectorReadbacks([string]$Text, [string]$Address) {
    $pattern = '(?im)^\s*' + [regex]::Escape($Address) + '\s*=\s*([0-9A-F]{8})\s+([0-9A-F]{8})'
    return @([regex]::Matches($Text,$pattern) | ForEach-Object { $_.Groups[1].Value.ToUpperInvariant()+ '/'+$_.Groups[2].Value.ToUpperInvariant() })
}

function Invoke-MathisJLink([string]$JLinkExe, [string]$ArtifactDir, [string]$Label,
                          [string[]]$Commands, [switch]$RequireVerification) {
    if (!(Test-Path -LiteralPath $JLinkExe -PathType Leaf)) { throw "J-Link Commander not found: $JLinkExe" }
    New-Item -ItemType Directory -Force -Path $ArtifactDir | Out-Null
    $commandFile=Join-Path $ArtifactDir "$Label.jlink"
    $logFile=Join-Path $ArtifactDir "${Label}_jlink.log"
    $consoleFile=Join-Path $ArtifactDir "${Label}_jlink_console.log"
    $encoding=[Text.Encoding]::Default
    foreach ($command in $Commands) {
        if ($encoding.GetString($encoding.GetBytes($command)) -cne $command) { throw 'J-Link command cannot be represented in the Windows system encoding.' }
    }
    [IO.File]::WriteAllLines($commandFile,$Commands,$encoding)
    $lines=@(& $JLinkExe -device STM32F030C8 -if SWD -speed 1000 -autoconnect 1 `
        -ExitOnError 1 -CommanderScript $commandFile -Log $logFile 2>&1 | ForEach-Object { "$_" })
    $exitCode=$LASTEXITCODE
    $text=$lines -join "`r`n"
    [IO.File]::WriteAllText($consoleFile,$text,[Text.UTF8Encoding]::new($false))
    $lines | ForEach-Object { Write-Host $_ }
    if (($exitCode -ne 0) -or ($text -match '(?i)FAILED:|Cannot connect|Could not connect|Error:|Verification failed|Verify failed')) {
        throw "J-Link failed (exit $exitCode). See $consoleFile"
    }
    if ($RequireVerification -and ($text -notmatch '(?i)Verify successful|Verification (?:OK|successful)|Verified successfully')) {
        throw "J-Link did not report successful binary verification. See $consoleFile"
    }
    return $text
}
