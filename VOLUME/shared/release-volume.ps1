param(
  [Parameter(Mandatory = $true)][string]$ProjectPath,
  [Parameter(Mandatory = $true)][ValidatePattern('^volume-[a-z0-9-]+$')][string]$StationId,
  [Parameter(Mandatory = $true)][string]$DisplayName,
  [string]$Wrangler = 'D:\AI_PROJECTS\Doi_Chieu_Cong_No\server\node_modules\.bin\wrangler.cmd'
)

$ErrorActionPreference = 'Stop'
$platformio = 'C:\Users\quang\.platformio\penv\Scripts\platformio.exe'
$pioRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$sharedOtaRoot = Join-Path $pioRoot 'TRAM_CC\MAIN_CC\ota'
$wranglerConfig = Join-Path $sharedOtaRoot 'wrangler.jsonc'
$mainCpp = Join-Path $ProjectPath 'src\main.cpp'
$binary = Join-Path $ProjectPath '.pio\build\nodemcuv2\firmware.bin'

function Get-HashHex {
  param(
    [Parameter(Mandatory = $true)][string]$Path,
    [Parameter(Mandatory = $true)][ValidateSet('SHA256', 'MD5')][string]$Algorithm
  )
  $stream = [System.IO.File]::OpenRead($Path)
  $hasher = if ($Algorithm -eq 'SHA256') {
    [System.Security.Cryptography.SHA256]::Create()
  } else {
    [System.Security.Cryptography.MD5]::Create()
  }
  try {
    return ([System.BitConverter]::ToString($hasher.ComputeHash($stream))).Replace('-', '').ToLowerInvariant()
  } finally {
    $hasher.Dispose()
    $stream.Dispose()
  }
}

if (!(Test-Path -LiteralPath $mainCpp)) { throw "Khong tim thay main.cpp: $mainCpp" }
if (!(Test-Path -LiteralPath $platformio)) { throw "Khong tim thay PlatformIO: $platformio" }
if (!(Test-Path -LiteralPath $Wrangler)) { throw "Khong tim thay Wrangler: $Wrangler" }
if (!(Test-Path -LiteralPath $wranglerConfig)) { throw "Khong tim thay cau hinh Cloudflare: $wranglerConfig" }

$source = Get-Content -LiteralPath $mainCpp -Raw
$match = [regex]::Match($source, '#define\s+BLYNK_FIRMWARE_VERSION\s+"([0-9]{6}\.[0-9]+)"')
if (!$match.Success) { throw 'Khong doc duoc BLYNK_FIRMWARE_VERSION trong main.cpp' }
$version = $match.Groups[1].Value

& $platformio run --project-dir $ProjectPath
if ($LASTEXITCODE -ne 0) { throw 'Bien dich firmware that bai' }
if (!(Test-Path -LiteralPath $binary)) { throw "Khong tim thay firmware.bin: $binary" }

$item = Get-Item -LiteralPath $binary
$sha256 = Get-HashHex -Path $binary -Algorithm SHA256
$md5 = Get-HashHex -Path $binary -Algorithm MD5
$objectKey = "$StationId/releases/$version/firmware.bin"
$chunkSize = 16384
$chunkPrefix = "$StationId/releases/$version/chunks"
$firmwareBytes = [System.IO.File]::ReadAllBytes($binary)
$chunkCount = [int][Math]::Ceiling($firmwareBytes.Length / [double]$chunkSize)

$outputDir = Join-Path $ProjectPath 'ota\release-output'
New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
$manifestPath = Join-Path $outputDir 'latest.json'
$manifestJson = [ordered]@{
  version = $version
  objectKey = $objectKey
  size = $item.Length
  sha256 = $sha256
  md5 = $md5
  chunkSize = $chunkSize
  chunkCount = $chunkCount
  chunkPrefix = $chunkPrefix
  publishedAtUtc = [DateTime]::UtcNow.ToString('o')
} | ConvertTo-Json
[System.IO.File]::WriteAllText(
  $manifestPath,
  $manifestJson + [Environment]::NewLine,
  (New-Object System.Text.UTF8Encoding($false))
)

$bulkPath = Join-Path $outputDir "chunks-$version.bulk.json"
$chunkEntries = [System.Collections.Generic.List[object]]::new()
for ($index = 0; $index -lt $chunkCount; $index++) {
  $offset = $index * $chunkSize
  $length = [Math]::Min($chunkSize, $firmwareBytes.Length - $offset)
  $chunk = [byte[]]::new($length)
  [Array]::Copy($firmwareBytes, $offset, $chunk, 0, $length)
  $chunkEntries.Add([ordered]@{
    key = "$chunkPrefix/$index"
    value = [Convert]::ToBase64String($chunk)
    base64 = $true
  })
}
[System.IO.File]::WriteAllText(
  $bulkPath,
  ($chunkEntries | ConvertTo-Json -Depth 3 -Compress),
  (New-Object System.Text.UTF8Encoding($false))
)

try {
  & $Wrangler kv bulk put $bulkPath --binding FIRMWARE --remote --config $wranglerConfig
  if ($LASTEXITCODE -ne 0) { throw 'Tai cac khoi firmware len KV that bai; latest.json chua bi thay doi' }
} finally {
  Remove-Item -LiteralPath $bulkPath -Force -ErrorAction SilentlyContinue
}

& $Wrangler kv key put $objectKey --path $binary --binding FIRMWARE --remote --config $wranglerConfig
if ($LASTEXITCODE -ne 0) { throw 'Tai firmware len KV that bai; latest.json chua bi thay doi' }
& $Wrangler kv key put "$StationId/latest.json" --path $manifestPath --binding FIRMWARE --remote --config $wranglerConfig
if ($LASTEXITCODE -ne 0) { throw 'Tai latest.json len KV that bai' }

Write-Host "Da phat hanh $DisplayName $version"
Write-Host "Kich thuoc: $($item.Length) bytes"
Write-Host "SHA-256: $sha256"
Write-Host "MD5: $md5"

