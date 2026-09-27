$ErrorActionPreference = 'Stop'

$appRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$url = 'http://127.0.0.1:8765/'

try {
  $health = Invoke-RestMethod -Uri ($url + 'api/health') -TimeoutSec 2
  if ($health.ok) {
    Start-Process $url
    exit 0
  }
} catch {
  # The service is not running yet.
}

$python = Get-Command python.exe -ErrorAction Stop
$dataRoot = Join-Path $appRoot 'data'
New-Item -ItemType Directory -Path $dataRoot -Force | Out-Null

$stdout = Join-Path $dataRoot 'ota-manager.out.log'
$stderr = Join-Path $dataRoot 'ota-manager.err.log'
Start-Process `
  -FilePath $python.Source `
  -ArgumentList '-B', 'app.py' `
  -WorkingDirectory $appRoot `
  -WindowStyle Hidden `
  -RedirectStandardOutput $stdout `
  -RedirectStandardError $stderr | Out-Null

for ($attempt = 0; $attempt -lt 30; $attempt++) {
  Start-Sleep -Milliseconds 250
  try {
    $health = Invoke-RestMethod -Uri ($url + 'api/health') -TimeoutSec 2
    if ($health.ok) {
      Start-Process $url
      exit 0
    }
  } catch {
    # Keep waiting for the local service.
  }
}

Add-Type -AssemblyName PresentationFramework
[System.Windows.MessageBox]::Show(
  "Không khởi động được OTA Manager. Vui lòng gửi hai tệp log trong thư mục data để kiểm tra.",
  'OTA Manager',
  'OK',
  'Error'
) | Out-Null
exit 1
