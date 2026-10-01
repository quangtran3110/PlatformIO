$shared = (Resolve-Path (Join-Path $PSScriptRoot '..\\..\\shared\\release-volume.ps1')).Path
& $shared -ProjectPath (Split-Path $PSScriptRoot -Parent) -StationId 'volume-trambhd' -DisplayName 'VOLUME - Tram Binh Hiep Dong'
exit $LASTEXITCODE

