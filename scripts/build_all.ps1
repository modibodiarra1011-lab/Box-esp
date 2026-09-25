param(
  [string]$Projects = "projects",
  [string]$Output = "BUILD_OUTPUT",
  [string]$SD = "SD_READY"
)
$ErrorActionPreference = "Stop"
python "$PSScriptRoot\build_all.py" --projects $Projects --output $Output --sd $SD
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Host "Build Arduino + preparation SD termines."
