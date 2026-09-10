param([Parameter(Mandatory=$true)][ValidateSet('main','history','development','experiments','tools')][string]$Group,[string]$OnlySource='')
$ErrorActionPreference='Stop'
$workspaceRoot=[IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot)).TrimEnd('\')
$evidence=Join-Path $workspaceRoot '00_项目索引\证据\第三步'
$allMaps=Get-Content -LiteralPath (Join-Path $evidence '路径映射.json') -Raw -Encoding UTF8 | ConvertFrom-Json
$maps=@($allMaps | Where-Object { $_.group -eq $Group })
if ($OnlySource) { $maps=@($maps | Where-Object { $_.source -eq $OnlySource }) }
if (!$maps.Count) { throw 'No mappings selected.' }
$checked=@()
foreach ($map in $maps) {
    $src=[IO.Path]::GetFullPath((Join-Path $workspaceRoot $map.source))
    $dst=[IO.Path]::GetFullPath((Join-Path $workspaceRoot $map.target))
    foreach ($p in @($src,$dst)) {
        if (!$p.StartsWith($workspaceRoot+'\',[StringComparison]::OrdinalIgnoreCase)) { throw "Outside workspace: $p" }
        if ($p -eq (Join-Path $workspaceRoot '.git')) { throw 'Root Git metadata must remain in place.' }
    }
    if (!(Test-Path -LiteralPath $src)) { throw "Source absent: $src" }
    if (Test-Path -LiteralPath $dst) { throw "Destination already exists: $dst" }
    $checked+=@{Source=$src;Destination=$dst}
}
# Every absolute source and destination was checked before any directory move.
foreach ($pair in $checked) {
    $null=New-Item -ItemType Directory -Force -Path (Split-Path -Parent $pair.Destination)
    Move-Item -LiteralPath $pair.Source -Destination $pair.Destination
    if ((Test-Path -LiteralPath $pair.Source) -or !(Test-Path -LiteralPath $pair.Destination)) { throw 'Move postcondition failed.' }
    Add-Content -LiteralPath (Join-Path $evidence 'moved.jsonl') -Encoding UTF8 -Value ($pair | ConvertTo-Json -Compress)
}
Write-Output ("Moved {0}: {1} entries" -f $Group,$checked.Count)
