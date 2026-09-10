$ErrorActionPreference = 'Stop'
$taskRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$manifestFile = Join-Path $taskRoot '00_项目索引/证据/第四步/中间文件备份.json'
$manifest = Get-Content -LiteralPath $manifestFile -Encoding UTF8 -Raw | ConvertFrom-Json
function Resolve-WorkspaceFile([string]$relative) {
    $resolved = [IO.Path]::GetFullPath((Join-Path $taskRoot $relative))
    if (-not $resolved.StartsWith($taskRoot + '\', [StringComparison]::OrdinalIgnoreCase)) { throw "Outside workspace: $relative" }
    if (-not (Test-Path -LiteralPath $resolved -PathType Leaf)) { throw "Missing file: $relative" }
    if ((Get-Item -LiteralPath $resolved).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Reparse point: $relative" }
    return $resolved
}
$archive = Resolve-WorkspaceFile $manifest.archive
if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $manifest.archive_sha256) { throw 'Backup SHA mismatch' }
if ($manifest.verified_decompressed_files -ne $manifest.files.Count) { throw 'Incomplete backup verification' }
$targets = @()
foreach ($entry in $manifest.files) {
    $resolved = Resolve-WorkspaceFile $entry.path
    if ($entry.path -notmatch '^01_固件工程/(main|development/snapshot-14_refactor-from-10/tw66gw02)/(MDK-ARM/(tw66gw02[^/]*|runs|\.build-runs)/|Bootloader/build(_min)?/)') { throw "Not a compiler output directory: $resolved" }
    if ([IO.Path]::GetExtension($resolved) -notin @('.o','.crf','.d')) { throw "Unexpected extension: $resolved" }
    if ((Get-FileHash -LiteralPath $resolved -Algorithm SHA256).Hash -ne $entry.sha256) { throw "File changed after backup: $resolved" }
    $targets += $resolved
}
# All absolute targets and current hashes are checked before the first mutation.
foreach ($resolved in $targets) { Remove-Item -LiteralPath $resolved -ErrorAction Stop }
@{ removed_files = $targets.Count; removed_bytes = $manifest.original_bytes; archive = $manifest.archive; time = (Get-Date).ToString('o') } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $taskRoot '00_项目索引/证据/第四步/清理执行.json') -Encoding UTF8
Write-Output "Removed $($targets.Count) individually verified compiler intermediates."
