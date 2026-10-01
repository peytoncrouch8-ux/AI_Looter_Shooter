# Runs the game standalone on a level, tours its viewpoints with Looter.Tour (frame times and a screenshot from each),
# and prints the results. Screenshots land in Saved\Screenshots\Tour, the timings in Saved\Tour\<level>.csv.
# Timings are only trustworthy with the editor closed (it shares the GPU); screenshots work either way.
# Usage: tour.ps1 [-Map /Game/Maps/Lvl_TutorialIsland] [-Quality Medium] [-Views Art/Levels/TutorialIsland/views.json]
#                 [-NoShots] [-ResX 1920] [-ResY 1080] [-Exec "cvar value,..."] (console commands before the tour)
param([string]$Map = '/Game/Maps/Lvl_TutorialIsland', [string]$Quality = 'Medium',
    [string]$Views = 'Art/Levels/TutorialIsland/views.json', [switch]$NoShots, [int]$ResX = 1920, [int]$ResY = 1080, [string]$Exec = '')

$root = Split-Path $PSScriptRoot -Parent
$engine = "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
if (Get-Process UnrealEditor -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowTitle -like '*Unreal Editor*' }) {
    "Note: the editor is open, so the timings share the GPU with it. Close it (Tools\close.ps1) for real numbers."
}
$tour = "Looter.Tour $Views quit" + $(if ($NoShots) { ' noshots' } else { '' })
$gameArgs = "`"$root\AI_Looter_Shooter.uproject`" $Map -game -windowed -ResX=$ResX -ResY=$ResY -nosplash -log=Tour.log -ExecCmds=`"Looter.Quality $Quality,$(if ($Exec) { "$Exec," })$tour`""
$started = Get-Date
$proc = Start-Process $engine -ArgumentList $gameArgs -PassThru
if (-not $proc.WaitForExit(900000)) { $proc.Kill(); "The game did not finish the tour in time."; exit 2 }

$log = Join-Path $root 'Saved\Logs\Tour.log'
if (-not (Test-Path $log)) { "No log was written."; exit 3 }
$lines = Select-String -Path $log -Pattern 'Looter\.Tour:' | ForEach-Object { $_.Line -replace '^\[[^\]]*\]\[[^\]]*\]LogLooter: (Display: )?', '' }
if (-not $lines) { "The tour didn't run; see $log."; exit 4 }
$lines
if (-not $NoShots) {
    Get-ChildItem (Join-Path $root 'Saved\Screenshots\Tour') -Filter *.png -ErrorAction SilentlyContinue |
        Where-Object { $_.LastWriteTime -ge $started } | ForEach-Object { "  $($_.FullName)" }
}
