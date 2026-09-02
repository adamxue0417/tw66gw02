param([string]$PythonLauncher = 'py')
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$venv = Join-Path $root '.venv'
if (!(Test-Path -LiteralPath $venv)) {
    & $PythonLauncher -3.11 -m venv $venv
    if ($LASTEXITCODE -ne 0) { throw 'Python 3.11 virtual environment creation failed.' }
}
$python = Join-Path $venv 'Scripts\python.exe'
& $python -m pip install --disable-pip-version-check -r (Join-Path $root 'requirements.txt')
if ($LASTEXITCODE -ne 0) { throw 'Dependency installation failed.' }
& $python (Join-Path $root 'mathis_ble.py') selftest
if ($LASTEXITCODE -ne 0) { throw 'Offline selftest failed.' }
Write-Host "Ready. Use: $python $root\mathis_ble.py discover"
