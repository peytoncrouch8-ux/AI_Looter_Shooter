# Checks an area's layout.json and layout_computed.json before anything is built from them: feature rules, ramp grades
# and widths, every cliff course at most 12 m, features clear of the seam band, the playable boundary, the open-ground
# metric (Tools\terrain_check.py has the details). Runs Blender's own Python (numpy); the editor isn't needed and
# nothing is written. Exit code 1 on any error.
# Usage: terrain_check.ps1 <Area> [-Computed <layout_computed.json>]
param([Parameter(Mandatory = $true)][string]$Area, [string]$Computed = '')
$blender = $env:BLENDER
if (-not $blender) {
    $blender = Get-ChildItem 'C:\Program Files\Blender Foundation\*\blender.exe' -ErrorAction SilentlyContinue | Sort-Object FullName | Select-Object -Last 1 -ExpandProperty FullName
}
if (-not $blender -or -not (Test-Path $blender)) { "Blender not found. Install it, or set BLENDER to blender.exe."; exit 1 }
$python = Get-ChildItem (Join-Path (Split-Path $blender -Parent) '*\python\bin\python.exe') -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName
if (-not $python) { "Blender's Python wasn't found next to $blender."; exit 1 }
$arguments = @((Join-Path $PSScriptRoot 'terrain_check.py'), $Area)
if ($Computed) { $arguments += @('--computed', $Computed) }
& $python @arguments
exit $LASTEXITCODE
