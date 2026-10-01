$shared = (Resolve-Path (Join-Path $PSScriptRoot '..\\..\\shared\\release-volume.ps1')).Path
& $shared -ProjectPath (Split-Path $PSScriptRoot -Parent) -StationId 'volume-tram-cc-g1' -DisplayName 'VOLUME - Tram Cai Cat G1'
exit $LASTEXITCODE

