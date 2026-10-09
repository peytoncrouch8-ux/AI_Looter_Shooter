# Runs the game standalone on a level, gives the player a realistic loadout and photographs the inventory through its states
# with Looter.MenuShots (a screenshot WITH the UI at each: the loadout, a backpack gun and its comparison, an upgrade,
# Inspect, an equip, a swap target, a sort, the ledger, the missions), and prints the pictures' paths. Pictures land in
# Saved\Screenshots\MenuShots\<NN>_<state>.png. The run drives the screen with key presses only, so the same command
# photographs any version of the inventory (a redesign's before and after).
# A map named on the command line plays a new game with no session, so nothing is saved.
# Screenshots work with the editor open or closed; the window is 1920x1080 by default, and the pictures' sizes are checked.
# Usage: menushots.ps1 [-Map /Game/Maps/Lvl_TutorialIsland] [-Quality Medium] [-ResX 1920] [-ResY 1080]
#                      [-Exec "cvar value,..."] (console commands before the shots) [-TimeoutSeconds 900]
#                      [-Label before] (moves this run's pictures into MenuShots\<Label>, so a later run doesn't overwrite them)
param([string]$Map = '/Game/Maps/Lvl_TutorialIsland', [string]$Quality = 'Medium', [int]$ResX = 1920, [int]$ResY = 1080,
    [string]$Exec = '', [int]$TimeoutSeconds = 900, [string]$Label = '')

$root = Split-Path $PSScriptRoot -Parent
$engine = "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
if (Get-Process UnrealEditor -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowTitle -like '*Unreal Editor*' }) {
    "Note: the editor is open; the game runs beside it (fine for pictures, slower to start)."
}
$shots = "Looter.MenuShots quit"
$gameArgs = "`"$root\AI_Looter_Shooter.uproject`" $Map -game -windowed -ResX=$ResX -ResY=$ResY -nosplash -log=MenuShots.log -ExecCmds=`"Looter.Quality $Quality,$(if ($Exec) { "$Exec," })$shots`""
$started = Get-Date
$proc = Start-Process $engine -ArgumentList $gameArgs -PassThru
if (-not $proc.WaitForExit($TimeoutSeconds * 1000)) { $proc.Kill(); "The game did not finish the shots in time."; exit 2 }

$log = Join-Path $root 'Saved\Logs\MenuShots.log'
if (-not (Test-Path $log)) { "No log was written."; exit 3 }
$lines = Select-String -Path $log -Pattern 'Looter\.MenuShots:' | ForEach-Object { $_.Line -replace '^\[[^\]]*\]\[[^\]]*\]LogLooter: (Display: )?', '' }
if (-not $lines) { "The shots didn't run; see $log."; exit 4 }
$lines

# The pictures this run made, with their sizes (a window that doesn't fit the screen comes out smaller than asked).
function PngSize([string]$Path) {
    $head = New-Object byte[] 24
    $stream = [System.IO.File]::OpenRead($Path)
    try { [void]$stream.Read($head, 0, 24) } finally { $stream.Dispose() }
    $w = ($head[16] * 16777216) + ($head[17] * 65536) + ($head[18] * 256) + $head[19]
    $h = ($head[20] * 16777216) + ($head[21] * 65536) + ($head[22] * 256) + $head[23]
    "${w}x${h}"
}
$pictures = Get-ChildItem (Join-Path $root 'Saved\Screenshots\MenuShots') -Filter *.png -ErrorAction SilentlyContinue |
    Where-Object { $_.LastWriteTime -ge $started } | Sort-Object Name
if (-not $pictures) { "No pictures were written; see $log."; exit 5 }
if ($Label) {
    $folder = Join-Path $root "Saved\Screenshots\MenuShots\$Label"
    New-Item -ItemType Directory -Force $folder | Out-Null
    $pictures = foreach ($picture in $pictures) { Move-Item $picture.FullName $folder -Force -PassThru }
}
"Pictures:"
$expected = "${ResX}x${ResY}"
foreach ($picture in $pictures) {
    $size = PngSize $picture.FullName
    "  $($picture.FullName)  ($size)" + $(if ($size -ne $expected) { "  <- not $expected" } else { '' })
}
