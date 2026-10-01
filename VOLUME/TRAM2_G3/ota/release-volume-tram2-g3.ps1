$shared = (Resolve-Path (Join-Path $PSScriptRoot '..\\..\\shared\\release-volume.ps1')).Path
& $shared -ProjectPath (Split-Path $PSScriptRoot -Parent) -StationId 'volume-tram2-g3' -DisplayName 'VOLUME - Tram 2 G3'
exit $LASTEXITCODE

