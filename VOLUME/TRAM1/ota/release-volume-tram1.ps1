$shared = (Resolve-Path (Join-Path $PSScriptRoot '..\\..\\shared\\release-volume.ps1')).Path
& $shared -ProjectPath (Split-Path $PSScriptRoot -Parent) -StationId 'volume-tram1' -DisplayName 'VOLUME - Tram 1'
exit $LASTEXITCODE

