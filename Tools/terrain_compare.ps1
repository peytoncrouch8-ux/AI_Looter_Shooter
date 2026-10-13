# Compares an area's terrain from the working tree with HEAD's inside HEAD's playable boundary: heights cell for cell
# (a grown map square, map.grownFrom, shares its cells), the baked occlusion and curvature, and with -Macro the macro
# map at 1 m (Tools\terrain_compare.py has the details). Blender runs headless; the editor isn't needed, and nothing in
# the repository is written. -Except names a JSON of the areas meant to change ({"areas": [{"id", "polygon"}]}, cm).
# Usage: terrain_compare.ps1 -Area RansomsRest [-Layout <scratch layout.json>] [-Macro] [-Except <file>] [-Fresh] [-Keep]
param([string]$Area = 'RansomsRest', [string]$Layout = '', [switch]$Macro, [string]$Except = '', [switch]$Fresh,
      [switch]$Keep)
$blender = $env:BLENDER
if (-not $blender) {
    $blender = Get-ChildItem 'C:\Program Files\Blender Foundation\*\blender.exe' -ErrorAction SilentlyContinue | Sort-Object FullName | Select-Object -Last 1 -ExpandProperty FullName
}
if (-not $blender -or -not (Test-Path $blender)) { "Blender not found. Install it, or set BLENDER to blender.exe."; exit 1 }
# Blender's own Python has numpy and OpenImageIO, which the comparison needs.
$python = Get-ChildItem (Join-Path (Split-Path $blender -Parent) '*\python\bin\python.exe') -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName
if (-not $python) { "Blender's Python wasn't found next to $blender."; exit 1 }
$arguments = @((Join-Path $PSScriptRoot 'terrain_compare.py'), $Area, '--blender', $blender)
if ($Layout) { $arguments += @('--layout', $Layout) }
if ($Macro) { $arguments += '--macro' }
if ($Except) { $arguments += @('--except', $Except) }
if ($Fresh) { $arguments += '--fresh' }
if ($Keep) { $arguments += '--keep' }
& $python @arguments
exit $LASTEXITCODE
