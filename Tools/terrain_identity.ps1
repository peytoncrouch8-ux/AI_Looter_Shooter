# Checks that the terrain generator still builds an area exactly as committed: its layout_computed.json, meshes and
# maps against HEAD's (Tools\terrain_identity.py has the details). Blender runs headless; the editor isn't needed, and
# nothing in the repository is written. Run it after every change to Art\Levels\area_*.py or a layout's generator data.
# Usage: terrain_identity.ps1 [-Area TutorialIsland] [-Fresh] [-Keep]
param([string]$Area = 'TutorialIsland', [switch]$Fresh, [switch]$Keep)
$blender = $env:BLENDER
if (-not $blender) {
    $blender = Get-ChildItem 'C:\Program Files\Blender Foundation\*\blender.exe' -ErrorAction SilentlyContinue | Sort-Object FullName | Select-Object -Last 1 -ExpandProperty FullName
}
if (-not $blender -or -not (Test-Path $blender)) { "Blender not found. Install it, or set BLENDER to blender.exe."; exit 1 }
# Blender's own Python has numpy and OpenImageIO, which the comparison needs.
$python = Get-ChildItem (Join-Path (Split-Path $blender -Parent) '*\python\bin\python.exe') -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName
if (-not $python) { "Blender's Python wasn't found next to $blender."; exit 1 }
$arguments = @((Join-Path $PSScriptRoot 'terrain_identity.py'), $Area, '--blender', $blender)
if ($Fresh) { $arguments += '--fresh' }
if ($Keep) { $arguments += '--keep' }
& $python @arguments
exit $LASTEXITCODE
