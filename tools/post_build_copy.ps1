param(
  [string]$Config = "Release",
  [string]$PluginName = "Muse",
  [switch]$VST3,
  [switch]$CLAP
)

$ErrorActionPreference = "Stop"

function Ensure-Dir($path) {
  if (-not (Test-Path -LiteralPath $path)) {
    New-Item -ItemType Directory -Path $path | Out-Null
  }
}

$repoRoot = Split-Path -Parent $PSScriptRoot
$artefacts = Join-Path $repoRoot "build\Muse_artefacts\$Config"

if ($VST3) {
  $srcVst3 = Join-Path $artefacts "VST3\$PluginName.vst3"
  # Per-user VST3 path avoids admin; many DAWs will scan this
  $dstVst3 = Join-Path $env:APPDATA "VST3"
  Ensure-Dir $dstVst3
  if (Test-Path -LiteralPath $srcVst3) {
    Write-Host "Copying VST3: $srcVst3 -> $dstVst3"
    Copy-Item -LiteralPath $srcVst3 -Destination $dstVst3 -Recurse -Force
  } else {
    Write-Warning "VST3 not found: $srcVst3"
  }
}

if ($CLAP) {
  $srcClap = Join-Path $artefacts "CLAP\$PluginName.clap"
  # Per-user CLAP location
  $dstClap = Join-Path $env:LOCALAPPDATA "Programs\Common\CLAP"
  Ensure-Dir $dstClap
  if (Test-Path -LiteralPath $srcClap) {
    Write-Host "Copying CLAP: $srcClap -> $dstClap"
    Copy-Item -LiteralPath $srcClap -Destination $dstClap -Force
  } else {
    Write-Warning "CLAP not found: $srcClap"
  }
}

