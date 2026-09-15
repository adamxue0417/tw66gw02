param([string]$ProjectRoot, [int]$Version)
$ErrorActionPreference = 'Stop'
# Use the source script's exact app-build function without its signing preflight.
$scriptPath = Join-Path $ProjectRoot 'BuildOtaArtifacts.ps1'
$tokens = $null; $parseErrors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile($scriptPath, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count -gt 0) { throw 'Source build script parsing failed.' }
$function = $ast.Find({param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Invoke-AppBuild'}, $true)
if ($null -eq $function) { throw 'Invoke-AppBuild not found.' }
. ([scriptblock]::Create($function.Extent.Text))
$projectRoot = [IO.Path]::GetFullPath($ProjectRoot)
$mdkDir = Join-Path $projectRoot 'MDK-ARM'
$outputDir = Join-Path $projectRoot 'OTA_Artifacts'
$generatedProject = Join-Path $mdkDir 'tw66gw02_ota.generated.uvprojx'
$uv4 = 'C:\Keil_v5\UV4\UV4.exe'
$fromelf = 'C:\Keil_v5\ARM\ARM_Compiler_5.06u7\Bin\fromelf.exe'
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
Push-Location $projectRoot
try { Invoke-AppBuild $Version "v$Version" "mathis_app_v$Version" | Out-String | Write-Host }
finally {
    if (Test-Path -LiteralPath $generatedProject) { Remove-Item -LiteralPath $generatedProject -Force }
    Pop-Location
}
