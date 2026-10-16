# Renders, paints and sheets a hero's portrait concepts (Docs/Pipeline.md, "Character portraits").
# Usage: portraits.ps1 <Hero> [-Keys A,B,C] [-Size 1024] [-Samples 80] [-Out <dir>] [-NoRender] [-AuxOnly] [-NoSheet]
#   -AuxOnly redoes only the aux pass (ids and glow for paint.js; seconds a bust) and repaints; -NoRender only repaints.
#   <Hero>: Ellis (script EllisBlender.py, Frame.svg) or Odessa | Crane | Gauge | Pike (<Hero>.py, Frame_<Hero>.svg).
#   Each key: Blender renders <Hero><Key>_color.png and _aux.png into <Out>, then render.js paints and frames them
#   into <Hero><Key>.png; then sheet.js lays the keys on one sheet. <Out> defaults to Saved\ArtPreviews\Portraits\<Hero>.
#   1024 px at 80 samples is about 2 minutes a bust here; -Size 600 -Samples 32 for a look.
param([Parameter(Mandatory = $true)][string]$Hero, [string[]]$Keys = @('A', 'B', 'C'), [int]$Size = 1024, [int]$Samples = 80,
    [string]$Out = '', [switch]$NoRender, [switch]$AuxOnly, [switch]$NoSheet)
$root = Split-Path $PSScriptRoot -Parent
$blender = 'C:\Program Files\Blender Foundation\Blender 4.4\blender.exe'
$node = Join-Path $env:LOCALAPPDATA 'Programs\node'
if (Test-Path $node) { $env:Path = "$node;$env:Path" }
$script = if ($Hero -eq 'Ellis') { 'EllisBlender.py' } else { "$Hero.py" }
$frame = if ($Hero -eq 'Ellis') { 'Frame.svg' } else { "Frame_$Hero.svg" }
if (-not $Out) { $Out = Join-Path $root "Saved\ArtPreviews\Portraits\$Hero" }
New-Item -ItemType Directory -Force $Out | Out-Null
$portraits = Join-Path $root 'Art\Portraits'
if (-not (Test-Path (Join-Path $portraits $frame))) { "no $frame yet (node Art\Portraits\frame\gen.js $Hero makes it): using Ellis's"; $frame = 'Frame.svg' }
foreach ($key in $Keys) {
    if (-not $NoRender) {
        $t = Get-Date
        $extra = if ($AuxOnly) { @('--aux-only') } else { @() }
        & $blender -b --factory-startup --python-exit-code 1 --python (Join-Path $portraits $script) -- $key --size $Size --samples $Samples --out $Out @extra 2>&1 |
            Select-String -Pattern 'built in|rendered in|Error|Traceback' | ForEach-Object { "  $Hero$key : $($_.Line.Trim())" }
        if ($LASTEXITCODE -ne 0) { "RENDER FAILED: $Hero$key"; exit 1 }
        "  $Hero$key rendered in $([int]((Get-Date) - $t).TotalSeconds) s"
    }
    node (Join-Path $portraits 'render.js') "$Hero$key" --passes $Out --size $Size --script $script --frame $frame 2>&1 | Select-Object -Last 1
}
if (-not $NoSheet) { node (Join-Path $portraits 'sheet.js') $Out --hero $Hero 2>&1 | Select-Object -Last 2 }
"done: $Out"
