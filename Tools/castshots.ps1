# Runs the game standalone on a level and photographs the cast with Looter.CastShots: every creature kind (spider,
# Restless and Gravebound spiders, spiderling, the Gravemother, slimes, the Unpaid and its ranks) on open flat ground and
# on a slope in each state (idle, walk, chase, attack wind-up, hurt, death), packs closing on the player, the player's
# body in third person, and the story's characters where they stand (Sexton, Amos leaning and sitting, Hob, Pa on his
# board, Abel, Delia's door, Tilly's window), each from three angles with time stopped and no UI. Pictures land in
# Saved\Screenshots\CastShots\<NNN>_<who>-<place>_<state>_<angle>.png; numbers.csv beside them has what pictures miss
# (feet in or over the ground, limbs through bodies, sliding feet, pops, packs in each other, characters in the level),
# marked "!" past what a player would notice. This prints the run's log lines, the marked numbers and the pictures.
# A map named on the command line plays a new game with no session (nothing is saved); the editor may stay open.
# -Tag before puts the run in Saved\Screenshots\CastShots\before, so runs before and after a change sit side by side.
# Usage: castshots.ps1 [-Map /Game/Maps/Lvl_RansomsRest] [-Only spider,unpaid] [-Flat X,Y] [-Slope X,Y] [-Tag name]
#                      [-Quality Medium] [-ResX 1600] [-ResY 900] [-Exec "cvar value,..."] [-TimeoutSeconds 2400]
param([string]$Map = '/Game/Maps/Lvl_RansomsRest', [string]$Only = '', [string]$Flat = '', [string]$Slope = '', [string]$Tag = '',
    [string]$Quality = 'Medium', [int]$ResX = 1600, [int]$ResY = 900, [string]$Exec = '', [int]$TimeoutSeconds = 2400)

$root = Split-Path $PSScriptRoot -Parent
$engine = "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
if (Get-Process UnrealEditor -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowTitle -like '*Unreal Editor*' }) {
    "Note: the editor is open; the game runs beside it (fine for pictures, slower to start)."
}
# -ExecCmds splits commands at commas: the command takes '+' between names and ':' inside a point.
$shots = "Looter.CastShots quit"
if ($Only) { $shots += " only=$($Only -replace ',', '+')" }
if ($Flat) { $shots += " flat=$($Flat -replace ',', ':')" }
if ($Slope) { $shots += " slope=$($Slope -replace ',', ':')" }
if ($Tag) { $shots += " tag=$Tag" }
$picturesDir = Join-Path $root 'Saved\Screenshots\CastShots'
if ($Tag) { $picturesDir = Join-Path $picturesDir $Tag }
$gameArgs = "`"$root\AI_Looter_Shooter.uproject`" $Map -game -windowed -ResX=$ResX -ResY=$ResY -nosplash -log=CastShots.log -ExecCmds=`"Looter.Quality $Quality,$(if ($Exec) { "$Exec," })$shots`""
$started = Get-Date
$proc = Start-Process $engine -ArgumentList $gameArgs -PassThru
if (-not $proc.WaitForExit($TimeoutSeconds * 1000)) { $proc.Kill(); "The game did not finish the shots in time."; exit 2 }

$log = Join-Path $root 'Saved\Logs\CastShots.log'
if (-not (Test-Path $log)) { "No log was written."; exit 3 }
$lines = Select-String -Path $log -Pattern 'Looter\.CastShots:' | ForEach-Object { $_.Line -replace '^\[[^\]]*\]\[[^\]]*\]LogLooter: (Display: |Warning: )?', '' }
if (-not $lines) { "The shots didn't run; see $log."; exit 4 }
$lines | Where-Object { $_ -notmatch '^Looter\.CastShots: \d{3} ' }

# The numbers past their marks, then every picture this run made.
$csv = Join-Path $picturesDir 'numbers.csv'
if ((Test-Path $csv) -and (Get-Item $csv).LastWriteTime -ge $started) {
    $marked = Import-Csv $csv | Where-Object { $_.past_mark -eq '!' }
    "Numbers past their marks: $($marked.Count) (all in $csv)"
    foreach ($row in $marked) { "  $($row.picture) $($row.who)-$($row.place) $($row.state): $($row.metric) $($row.value) $($row.detail)" }
}
$pictures = Get-ChildItem $picturesDir -Filter *.png -ErrorAction SilentlyContinue | Where-Object { $_.LastWriteTime -ge $started } | Sort-Object Name
if (-not $pictures) { "No pictures were written; see $log."; exit 5 }
"Pictures: $($pictures.Count) in $picturesDir"
foreach ($picture in $pictures) { "  $($picture.Name)" }
