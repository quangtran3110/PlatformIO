param(
  [string]$ProjectPath = (Split-Path $PSScriptRoot -Parent),
  [string]$Wrangler = 'D:\AI_PROJECTS\Doi_Chieu_Cong_No\server\node_modules\.bin\wrangler.cmd'
)

$ErrorActionPreference = 'Stop'
$platformio = 'C:\Users\quang\.platformio\penv\Scripts\platformio.exe'
$wranglerConfig = Join-Path $PSScriptRoot 'wrangler.jsonc'
$mainCpp = Join-Path $ProjectPath 'src\main.cpp'
$binary = Join-Path $ProjectPath '.pio\build\nodemcuv2\firmware.bin'

if (!(Test-Path -LiteralPath $mainCpp)) { throw "Khong tim thay main.cpp: $mainCpp" }
if (!(Test-Path -LiteralPath $platformio)) { throw "Khong tim thay PlatformIO: $platformio" }
if (!(Test-Path -LiteralPath $Wrangler)) { throw "Khong tim thay Wrangler: $Wrangler" }

$source = Get-Content -LiteralPath $mainCpp -Raw
$match = [regex]::Match($source, '#define\s+BLYNK_FIRMWARE_VERSION\s+"([0-9]{6}\.[0-9]+)"')
if (!$match.Success) { throw 'Khong doc duoc BLYNK_FIRMWARE_VERSION trong main.cpp' }
$version = $match.Groups[1].Value

& $platformio run --project-dir $ProjectPath
if ($LASTEXITCODE -ne 0) { throw 'Bien dich firmware that bai' }
if (!(Test-Path -LiteralPath $binary)) { throw "Khong tim thay firmware.bin: $binary" }

$item = Get-Item -LiteralPath $binary
$sha256 = (Get-FileHash -LiteralPath $binary -Algorithm SHA256).Hash.ToLowerInvariant()
$md5 = (Get-FileHash -LiteralPath $binary -Algorithm MD5).Hash.ToLowerInvariant()
$objectKey = "tram-cc/releases/$version/firmware.bin"

$outputDir = Join-Path $PSScriptRoot 'release-output'
New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
$manifestPath = Join-Path $outputDir 'latest.json'
[ordered]@{
  version = $version
  objectKey = $objectKey
  size = $item.Length
  sha256 = $sha256
  md5 = $md5
  publishedAtUtc = [DateTime]::UtcNow.ToString('o')
} | ConvertTo-Json | Set-Content -LiteralPath $manifestPath -Encoding utf8NoBOM

& $Wrangler kv key put $objectKey --path $binary --binding FIRMWARE --remote --config $wranglerConfig
if ($LASTEXITCODE -ne 0) { throw 'Tai firmware len KV that bai; latest.json chua bi thay doi' }
& $Wrangler kv key put 'tram-cc/latest.json' --path $manifestPath --binding FIRMWARE --remote --config $wranglerConfig
if ($LASTEXITCODE -ne 0) { throw 'Tai latest.json len KV that bai' }

Write-Host "Da phat hanh Tram Cai Cat $version"
Write-Host "Kich thuoc: $($item.Length) bytes"
Write-Host "SHA-256: $sha256"
Write-Host "MD5: $md5"
