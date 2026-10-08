# Runs the game standalone on a level, photographs the gameplay HUD through its states with Looter.HudShots (a screenshot
# WITH the UI at each: calm, hit, low health, heal, experience, level-up, reload, empty magazine, two guns, boss bar,
# objective done), and prints the pictures' paths. Pictures land in Saved\Screenshots\HudShots\<NN>_<state>.png.
# A map named on the command line plays a new game with no session, which is what this wants (level 1, unarmed start).
# Screenshots work with the editor open or closed; the window is 1920x1080 by default, and the pictures' sizes are checked.
# Usage: hudshots.ps1 [-Map /Game/Maps/Lvl_TutorialIsland] [-Quality Medium] [-ResX 1920] [-ResY 1080]
#                     [-Exec "cvar value,..."] (console commands before the shots) [-TimeoutSeconds 900]
param([string]$Map = '/Game/Maps/Lvl_TutorialIsland', [string]$Quality = 'Medium', [int]$ResX = 1920, [int]$ResY = 1080,
    [string]$Exec = '', [int]$TimeoutSeconds = 900)

$root = Split-Path $PSScriptRoot -Parent
$engine = "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
if (Get-Process UnrealEditor -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowTitle -like '*Unreal Editor*' }) {
    "Note: the editor is open; the game runs beside it (fine for pictures, slower to start)."
}
$shots = "Looter.HudShots quit"
$gameArgs = "`"$root\AI_Looter_Shooter.uproject`" $Map -game -windowed -ResX=$ResX -ResY=$ResY -nosplash -log=HudShots.log -ExecCmds=`"Looter.Quality $Quality,$(if ($Exec) { "$Exec," })$shots`""
$started = Get-Date
$proc = Start-Process $engine -ArgumentList $gameArgs -PassThru
if (-not $proc.WaitForExit($TimeoutSeconds * 1000)) { $proc.Kill(); "The game did not finish the shots in time."; exit 2 }

$log = Join-Path $root 'Saved\Logs\HudShots.log'
if (-not (Test-Path $log)) { "No log was written."; exit 3 }
$lines = Select-String -Path $log -Pattern 'Looter\.HudShots:' | ForEach-Object { $_.Line -replace '^\[[^\]]*\]\[[^\]]*\]LogLooter: (Display: )?', '' }
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
$pictures = Get-ChildItem (Join-Path $root 'Saved\Screenshots\HudShots') -Filter *.png -ErrorAction SilentlyContinue |
    Where-Object { $_.LastWriteTime -ge $started } | Sort-Object Name
if (-not $pictures) { "No pictures were written; see $log."; exit 5 }
"Pictures:"
$expected = "${ResX}x${ResY}"
foreach ($picture in $pictures) {
    $size = PngSize $picture.FullName
    "  $($picture.FullName)  ($size)" + $(if ($size -ne $expected) { "  <- not $expected" } else { '' })
}
