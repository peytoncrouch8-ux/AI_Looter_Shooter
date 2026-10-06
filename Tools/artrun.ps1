# Runs Blender for model work beside a running editor: builds a model script (optionally rendering its previews), runs
# any Blender script, or export-tests models without importing them.
# It waits while Saved\ArtPause.flag exists (the main session is measuring performance and needs the CPU and GPU to
# itself; it creates the flag before perf.ps1 or tour.ps1 and deletes it after), then runs at below-normal priority, so
# the editor keeps the machine. Scripts start like a scripted model under Tools\models.ps1: an empty scene with
# Tools\Blender on the path.
# Usage:
#   artrun.ps1 -Script Art\Models\Buildings\Chapel.py [-Preview]      build a model script (-Preview renders its previews)
#   artrun.ps1 -Script <any .py> [-ScriptArgs a,b]                     any Blender script, with arguments after '--'
#   artrun.ps1 -Export Chapel,Graves -Family RR_Chapel                 export test (models.ps1 -NoImport) into
#                                                                      Intermediate\ArtExport_<Family>
#   artrun.ps1 -Script Tools\Blender\tangentcheck.py -ScriptArgs Intermediate\ArtExport_RR_Chapel,--where
param([string]$Script = '', [switch]$Preview, [string[]]$ScriptArgs = @(), [string[]]$Export = @(), [string]$Family = '')

$root = Split-Path $PSScriptRoot -Parent
$flag = Join-Path $root 'Saved\ArtPause.flag'
$waited = 0
while (Test-Path $flag) {
    if ($waited -ge 540) { 'Still paused (Saved\ArtPause.flag) after 9 minutes: run this again later.'; exit 3 }
    if ($waited -eq 0) { 'Paused while the main session measures performance (Saved\ArtPause.flag); waiting...' }
    Start-Sleep -Seconds 15
    $waited += 15
}
# Child processes inherit a below-normal priority class.
(Get-Process -Id $PID).PriorityClass = 'BelowNormal'
Set-Location $root

if ($Export) {
    if (-not $Family) { 'Give -Family: the export goes to Intermediate\ArtExport_<Family>.'; exit 1 }
    & powershell -NoProfile -File (Join-Path $PSScriptRoot 'models.ps1') -Only ($Export -join ',') -Out "Intermediate\ArtExport_$Family" -NoImport
    exit $LASTEXITCODE
}
if (-not $Script) { 'Give -Script <file.py> or -Export <names> -Family <family>.'; exit 1 }

$path = if ([IO.Path]::IsPathRooted($Script)) { $Script } else { Join-Path $root $Script }
if (-not (Test-Path $path)) { "No such script: $path"; exit 1 }
# The same Blender Tools\models.ps1 finds: BLENDER, or the newest one installed.
$blender = $env:BLENDER
if (-not $blender) {
    $blender = Get-ChildItem 'C:\Program Files\Blender Foundation\*\blender.exe' -ErrorAction SilentlyContinue | Sort-Object FullName | Select-Object -Last 1 -ExpandProperty FullName
}
if (-not $blender -or -not (Test-Path $blender)) { "Blender not found. Install it, or set BLENDER to blender.exe."; exit 1 }
$helpers = (Join-Path $PSScriptRoot 'Blender').Replace('\', '/')
# The same start Tools\models.ps1 gives a scripted model: the helpers on the path and an empty scene.
$setup = "import sys; sys.path.insert(0, '$helpers'); import bpy; bpy.ops.wm.read_factory_settings(use_empty=True)"
$rest = @()
if ($Preview) { $rest += '--preview' }
$rest += $ScriptArgs
$blenderArgs = @('-b', '--factory-startup', '--python-exit-code', '2', '--python-expr', $setup, '--python', $path, '--') + $rest
& $blender @blenderArgs 2>&1 | ForEach-Object { "$_" } | Where-Object { $_ -notmatch '^(Blender \d|Read prefs|Fra:\d|\s*$)' }
exit $LASTEXITCODE
