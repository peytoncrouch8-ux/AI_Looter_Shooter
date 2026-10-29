# Runs the game standalone twice and photographs the menus through their states with Looter.UIShots (a screenshot WITH
# the UI at each), then prints the pictures' paths. Pictures land in Saved\Screenshots\UIShots\<NN>_<state>.png.
#  1. The menu map with ?game=Menu and -NoLogos (no startup cards): 01 the main menu arrived, 02 Settings lit by the keys,
#     03 the settings opened from it (Graphics), 04 the Single Player session picker.
#  2. A level (a map on the command line plays a new game with no session, so nothing is saved): 05 the pause menu
#     (Graphics), 06 a row focused, 07 Audio, 08 Interface, 09 Controls (the tabs turned with Page Down), then the
#     level-up popup: 10 during its count, 11 holding.
# The run drives the screens with their own keys, so the same command photographs any version of them (a redesign's
# before and after). Screenshots work with the editor open or closed; the window is 1920x1080 by default, and the
# pictures' sizes are checked.
# Usage: uishots.ps1 [-MenuMap /Game/Maps/Lvl_MenuTeropa] [-Map /Game/Maps/Lvl_RansomsRest] [-Quality Medium]
#                    [-ResX 1920] [-ResY 1080] [-Exec "cvar value,..."] (console commands before the shots)
#                    [-TimeoutSeconds 900] (each run) [-Label before] (moves this run's pictures into UIShots\<Label>)
param([string]$MenuMap = '/Game/Maps/Lvl_MenuTeropa', [string]$Map = '/Game/Maps/Lvl_RansomsRest', [string]$Quality = 'Medium',
    [int]$ResX = 1920, [int]$ResY = 1080, [string]$Exec = '', [int]$TimeoutSeconds = 900, [string]$Label = '')

$root = Split-Path $PSScriptRoot -Parent
$engine = "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
if (Get-Process UnrealEditor -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowTitle -like '*Unreal Editor*' }) {
    "Note: the editor is open; the game runs beside it (fine for pictures, slower to start)."
}
$shots = "Looter.UIShots quit"
$started = Get-Date

# One standalone run on $RunMap (its extra switches in $Extra), its log's Looter.UIShots lines printed to the host (so the
# function's output is only whether it worked).
function RunHalf([string]$Name, [string]$RunMap, [string]$Extra) {
    $logName = "UIShots$Name.log"
    $gameArgs = "`"$root\AI_Looter_Shooter.uproject`" $RunMap -game -windowed -ResX=$ResX -ResY=$ResY -nosplash $Extra -log=$logName -ExecCmds=`"Looter.Quality $Quality,$(if ($Exec) { "$Exec," })$shots`""
    "== The $Name half: $RunMap" | Out-Host
    $proc = Start-Process $engine -ArgumentList $gameArgs -PassThru
    if (-not $proc.WaitForExit($TimeoutSeconds * 1000)) { $proc.Kill(); "The game did not finish the $Name shots in time." | Out-Host; return $false }
    $log = Join-Path $root "Saved\Logs\$logName"
    if (-not (Test-Path $log)) { "No log was written for the $Name half." | Out-Host; return $false }
    $lines = Select-String -Path $log -Pattern 'Looter\.UIShots:' | ForEach-Object { $_.Line -replace '^\[[^\]]*\]\[[^\]]*\]LogLooter: (Display: )?', '' }
    if (-not $lines) { "The $Name shots didn't run; see $log." | Out-Host; return $false }
    $lines | Out-Host
    return $true
}

$menuOk = RunHalf 'Menu' "${MenuMap}?game=Menu" '-NoLogos'
$levelOk = RunHalf 'Level' $Map ''

# The pictures this run made, with their sizes (a window that doesn't fit the screen comes out smaller than asked).
function PngSize([string]$Path) {
    $head = New-Object byte[] 24
    $stream = [System.IO.File]::OpenRead($Path)
    try { [void]$stream.Read($head, 0, 24) } finally { $stream.Dispose() }
    $w = ($head[16] * 16777216) + ($head[17] * 65536) + ($head[18] * 256) + $head[19]
    $h = ($head[20] * 16777216) + ($head[21] * 65536) + ($head[22] * 256) + $head[23]
    "${w}x${h}"
}
$pictures = Get-ChildItem (Join-Path $root 'Saved\Screenshots\UIShots') -Filter *.png -ErrorAction SilentlyContinue |
    Where-Object { $_.LastWriteTime -ge $started } | Sort-Object Name
if (-not $pictures) { "No pictures were written; see Saved\Logs\UIShotsMenu.log and UIShotsLevel.log."; exit 5 }
if ($Label) {
    $folder = Join-Path $root "Saved\Screenshots\UIShots\$Label"
    New-Item -ItemType Directory -Force $folder | Out-Null
    $pictures = foreach ($picture in $pictures) { Move-Item $picture.FullName $folder -Force -PassThru }
}
"Pictures:"
$expected = "${ResX}x${ResY}"
foreach ($picture in $pictures) {
    $size = PngSize $picture.FullName
    "  $($picture.FullName)  ($size)" + $(if ($size -ne $expected) { "  <- not $expected" } else { '' })
}
if (-not ($menuOk -and $levelOk)) { "One half failed (see above)."; exit 4 }
