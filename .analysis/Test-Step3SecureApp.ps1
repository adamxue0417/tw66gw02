param([Parameter(Mandatory=$true)][string]$ProjectRoot,[Parameter(Mandatory=$true)][int]$Version,[Parameter(Mandatory=$true)][string]$ReportDirectory)
$ErrorActionPreference='Stop'
$projectRoot=(Resolve-Path -LiteralPath $ProjectRoot).Path
$mdkDir=Join-Path $projectRoot 'MDK-ARM'
$outputDir=[IO.Path]::GetFullPath($ReportDirectory)
$null=New-Item -ItemType Directory -Force -Path $outputDir
$generatedProject=Join-Path $mdkDir 'tw66gw02_step3.generated.uvprojx'
if (Test-Path -LiteralPath $generatedProject) { throw 'Verification project already exists.' }
$uv4='C:\Keil_v5\UV4\UV4.exe'
$fromelf='C:\Keil_v5\ARM\ARM_Compiler_5.06u7\Bin\fromelf.exe'
$tokens=$null;$parseErrors=$null
$ast=[Management.Automation.Language.Parser]::ParseFile((Join-Path $projectRoot 'BuildOtaArtifacts.ps1'),[ref]$tokens,[ref]$parseErrors)
if ($parseErrors.Count) { throw 'Source build script does not parse.' }
$definition=$ast.Find({param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Invoke-AppBuild'},$true)
if (!$definition) { throw 'Expected application build function not found.' }
# Load only the existing application-build function. No key access, signing or flashing.
. ([scriptblock]::Create($definition.Extent.Text))
try {
    $result=Invoke-AppBuild $Version 'step3' "step3_app_v$Version"
    $result | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $outputDir 'result.json') -Encoding UTF8
    Write-Output ("Application v{0}: {1} bytes" -f $Version,$result.Size)
} finally {
    if (Test-Path -LiteralPath $generatedProject) { Remove-Item -LiteralPath $generatedProject -Force }
}
