$shared = (Resolve-Path (Join-Path $PSScriptRoot '..\\..\\shared\\release-volume.ps1')).Path
& $shared -ProjectPath (Split-Path $PSScriptRoot -Parent) -StationId 'volume-tram2bpt' -DisplayName 'VOLUME - Tram 2 BPT'
exit $LASTEXITCODE

