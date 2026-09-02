param([Parameter(ValueFromRemainingArguments=$true)][string[]]$Arguments)
$python = Join-Path $PSScriptRoot '.venv\Scripts\python.exe'
if (!(Test-Path -LiteralPath $python)) { throw 'Run Setup-PC-BLE.ps1 first.' }
& $python (Join-Path $PSScriptRoot 'mathis_ble.py') @Arguments
exit $LASTEXITCODE
