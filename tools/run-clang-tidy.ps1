$buildScript = Join-Path $PSScriptRoot 'Invoke-Build.ps1'

if ($args.Count -gt 0) {
    & $buildScript -Operation Analyze -AnalysisPath $args
} else {
    & $buildScript -Operation Analyze
}

exit $LASTEXITCODE
