$shared = (Resolve-Path (Join-Path $PSScriptRoot '..\\..\\shared\\release-volume.ps1')).Path
& $shared -ProjectPath (Split-Path $PSScriptRoot -Parent) -StationId 'volume-tram2-g1' -DisplayName 'VOLUME - Tram 2 G1'
exit $LASTEXITCODE

