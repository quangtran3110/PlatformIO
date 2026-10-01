$shared = (Resolve-Path (Join-Path $PSScriptRoot '..\\..\\shared\\release-volume.ps1')).Path
& $shared -ProjectPath (Split-Path $PSScriptRoot -Parent) -StationId 'volume-tram3' -DisplayName 'VOLUME - Tram 3'
exit $LASTEXITCODE

