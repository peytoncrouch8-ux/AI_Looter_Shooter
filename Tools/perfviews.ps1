# Splits a CSV profiler capture of Looter.Tour by its viewpoints and lists each view's most expensive stats.
# Capture: perf.ps1 -Map /Game/Maps/Lvl_TutorialIsland -Exec "Looter.Quality Medium,Looter.Tour noshots" -GpuStats
#          -Frames 9000 -Skip 0 -NoRecord     (the tour marks each measured stretch with a "Tour <view>" event)
# Usage: perfviews.ps1 <capture.csv> [-Pattern 'GPU/'] [-Top 12]
param([string]$Csv, [string]$Pattern = 'GPU/', [int]$Top = 12)

$lines = Get-Content $Csv
$headerRows = @(for ($i = 0; $i -lt $lines.Count; $i++) { if ($lines[$i].StartsWith('EVENTS,')) { $i } })
$header = $lines[$headerRows[-1]].Split(',')
$columns = @(for ($c = 0; $c -lt $header.Count; $c++) { if ($header[$c] -match $Pattern -or $header[$c] -eq 'FrameTime') { $c } })

$view = $null
$sums = [ordered]@{}
$counts = @{}
for ($i = 1; $i -lt $lines.Count; $i++) {
    $line = $lines[$i]
    if ($line.StartsWith('EVENTS,') -or $line.StartsWith('[')) { break }
    $cells = $line.Split(',')
    if ($cells[0] -match 'Tour end') { $view = $null }
    if ($cells[0] -match 'Tour (\w+)' -and $Matches[1] -ne 'end') { $view = $Matches[1] }
    if (-not $view) { continue }
    if (-not $sums.Contains($view)) { $sums[$view] = @{}; $counts[$view] = 0 }
    $counts[$view]++
    foreach ($c in $columns) {
        if ($c -lt $cells.Count -and $cells[$c] -ne '') {
            $sums[$view][$header[$c]] += [double]::Parse($cells[$c], [Globalization.CultureInfo]::InvariantCulture)
        }
    }
}
foreach ($v in $sums.Keys) {
    $n = $counts[$v]
    $frame = $sums[$v]['FrameTime'] / $n
    "== $v ($n frames, frame {0:0.0} ms)" -f $frame
    $sums[$v].GetEnumerator() | Where-Object { $_.Key -ne 'FrameTime' } | ForEach-Object { [pscustomobject]@{ Stat = $_.Key; Ms = $_.Value / $n } } |
        Sort-Object Ms -Descending | Select-Object -First $Top | ForEach-Object { "  {0,6:0.00}  {1}" -f $_.Ms, $_.Stat }
}
