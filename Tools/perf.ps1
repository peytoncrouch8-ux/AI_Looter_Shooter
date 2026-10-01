# Measures the game in a standalone window (no editor overhead) with Unreal's CSV profiler, and records the averages in
# Docs\Performance.md so every change can be compared with the baseline. The editor must be closed (it would compete for
# the GPU). The first frames (loading, the minimap bake, settling) are skipped.
# Usage: perf.ps1 -Label "what changed" [-Frames 900] [-Skip 300] [-ResX 1920] [-ResY 1080] [-NoRecord] [-Map /Game/Maps/...]
#        perf.ps1 -Label "..." -GpuStats -Exec "r.Nanite 0" -NoRecord   (per-pass GPU timings; console commands at start)
#        perf.ps1 -Label "..." -CsvPath <existing capture.csv>   (analyze a capture without running the game)
# Compare two captures pass by pass with Tools\perfdiff.ps1.
param([string]$Label = 'run', [int]$Frames = 900, [int]$Skip = 300, [int]$ResX = 1920, [int]$ResY = 1080, [switch]$NoRecord, [string]$CsvPath = '',
    [switch]$GpuStats, [string]$Exec = '', [string]$Map = '/Game/Maps/Lvl_TutorialIsland')

$root = Split-Path $PSScriptRoot -Parent
$engine = "C:\Program Files\Epic Games\UE_5.8"
# A standalone game run by the installed engine writes its Saved data (logs, captures) under AppData, not the project.
$savedDirs = @((Join-Path $root 'Saved'), (Join-Path $env:LOCALAPPDATA 'UnrealEngine\5.8\Saved'))
$csvDirs = $savedDirs | ForEach-Object { Join-Path $_ 'Profiling\CSV' }

if ($CsvPath) {
    $csv = Get-Item $CsvPath
} else {
    if (Get-Process UnrealEditor -ErrorAction SilentlyContinue) { "Close the editor first (Tools\close.ps1)."; exit 1 }
    $before = @($csvDirs | Where-Object { Test-Path $_ } | ForEach-Object { Get-ChildItem $_ -Filter *.csv | ForEach-Object FullName })
    $gameArgs = "`"$root\AI_Looter_Shooter.uproject`" $Map -game -windowed -ResX=$ResX -ResY=$ResY -nosplash -csvCaptureFrames=$($Frames + $Skip) -ExitAfterCsvProfiling -log=Perf.log"
    $commands = @()
    if ($GpuStats) { $commands += 'r.GPUCsvStatsEnabled 1' }
    if ($Exec) { $commands += $Exec.Split(',') | ForEach-Object { $_.Trim() } }
    if ($commands) { $gameArgs += " -ExecCmds=`"$($commands -join ',')`"" }
    $proc = Start-Process "$engine\Engine\Binaries\Win64\UnrealEditor.exe" -ArgumentList $gameArgs -PassThru
    if (-not $proc.WaitForExit(900000)) { $proc.Kill(); "The game did not exit in time."; exit 2 }
    $csv = $csvDirs | Where-Object { Test-Path $_ } | ForEach-Object { Get-ChildItem $_ -Filter *.csv } | Where-Object { $before -notcontains $_.FullName } | Sort-Object LastWriteTime | Select-Object -Last 1
    if (-not $csv) { "No CSV was written."; exit 3 }
}

# Per-frame rows sit between the header row and the trailing metadata rows.
# Layout: a header row, one row per frame, then (when stats appeared mid-capture) the complete header row again, then a
# metadata row. Columns only ever get appended, so the last header row describes every frame row; a frame row with fewer
# cells simply didn't have the later stats yet.
$lines = Get-Content $csv.FullName
$headerRows = @(for ($i = 0; $i -lt $lines.Count; $i++) { if ($lines[$i].StartsWith('EVENTS,')) { $i } })
$header = $lines[$headerRows[-1]].Split(',')
$rows = New-Object System.Collections.Generic.List[string[]]
for ($i = 1; $i -lt $lines.Count; $i++) {
    if ($lines[$i].StartsWith('EVENTS,') -or $lines[$i].StartsWith('[')) { break }
    $rows.Add($lines[$i].Split(','))
}
$rows = @($rows | Select-Object -Skip $Skip)
function Column([string]$Name) {
    $index = [Array]::IndexOf($header, $Name)
    if ($index -lt 0) { return $null }
    return @($rows | ForEach-Object { if ($index -lt $_.Count -and $_[$index] -ne '') { [double]::Parse($_[$index], [Globalization.CultureInfo]::InvariantCulture) } else { 0.0 } })
}
function Avg($Values) { if ($null -eq $Values -or $Values.Count -eq 0) { return $null }; ($Values | Measure-Object -Average).Average }
function P95($Values) { if ($null -eq $Values -or $Values.Count -eq 0) { return $null }; $s = $Values | Sort-Object; $s[[int][math]::Floor($s.Count * 0.95)] }

$frame = Column 'FrameTime'
$result = [ordered]@{
    Frames = $rows.Count
    FrameAvg = Avg $frame; FrameP95 = P95 $frame
    Game = Avg (Column 'GameThreadTime'); Render = Avg (Column 'RenderThreadTime'); RHI = Avg (Column 'RHIThreadTime'); GPU = Avg (Column 'GPUTime')
    Draws = Avg (Column 'RHI/DrawCalls'); Prims = Avg (Column 'RHI/PrimitivesDrawn')
}
$log = $savedDirs | ForEach-Object { Join-Path $_ 'Logs\Perf.log' } | Where-Object { Test-Path $_ } | Sort-Object { (Get-Item $_).LastWriteTime } | Select-Object -Last 1
$load = if ($log -and -not $CsvPath) { (Select-String -Path $log -Pattern 'Took ([0-9.]+) seconds to LoadMap\(/Game/Maps/' | Select-Object -Last 1).Matches.Groups[1].Value } else { $null }

function F($Value, [string]$Format = '0.0') { if ($null -eq $Value) { 'n/a' } else { ([double]$Value).ToString($Format, [Globalization.CultureInfo]::InvariantCulture) } }
$fps = if ($result.FrameAvg) { 1000.0 / $result.FrameAvg } else { $null }
"Frames {0} | frame {1} ms ({2} fps), p95 {3} ms | game {4} | render {5} | RHI {6} | GPU {7} ms | draws {8} | triangles {9} | load {10} s" -f `
    $result.Frames, (F $result.FrameAvg), (F $fps '0'), (F $result.FrameP95), (F $result.Game), (F $result.Render), (F $result.RHI), (F $result.GPU), (F $result.Draws '0'), (F $result.Prims '0'), (F $load '0.00')
"CSV: $($csv.FullName)"

if (-not $NoRecord) {
    $doc = Join-Path $root 'Docs\Performance.md'
    if (-not (Test-Path $doc)) {
        New-Item -ItemType Directory -Force -Path (Split-Path $doc) | Out-Null
        $intro = @(
            '# Performance history',
            '',
            'Measured with `Tools\perf.ps1`: standalone game window, CSV profiler, first frames (loading) skipped. Times are averages in ms.',
            'Target: 60 fps at 1080p on the Medium preset on the reference PC (Radeon RX 580, Core i7-8700, 16 GB).',
            '',
            '| Date | Change | Commit | Resolution | Frame | fps | p95 | Game | Render | RHI | GPU | Draws | Triangles | Load (s) |',
            '|---|---|---|---|---|---|---|---|---|---|---|---|---|---|')
        [IO.File]::WriteAllLines($doc, $intro)
    }
    $commit = (& git -C $root rev-parse --short HEAD 2>$null)
    $row = "| {0} | {1} | {2} | {3}x{4} | {5} | {6} | {7} | {8} | {9} | {10} | {11} | {12} | {13} | {14} |" -f (Get-Date -Format 'yyyy-MM-dd'), $Label, $commit, $ResX, $ResY, `
        (F $result.FrameAvg), (F $fps '0'), (F $result.FrameP95), (F $result.Game), (F $result.Render), (F $result.RHI), (F $result.GPU), (F $result.Draws '0'), (F $result.Prims '0'), (F $load '0.00')
    [IO.File]::AppendAllText($doc, $row + "`r`n")
    "Recorded in Docs\Performance.md"
}
