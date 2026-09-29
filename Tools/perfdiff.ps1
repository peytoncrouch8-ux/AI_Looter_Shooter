# Compares two CSV profiler captures column by column (averages after the first frames) and lists the biggest changes.
# Usage: perfdiff.ps1 <before.csv> <after.csv> [-Pattern 'GPU/'] [-Skip 300] [-Top 25]
param([string]$Before, [string]$After, [string]$Pattern = 'GPU/', [int]$Skip = 300, [int]$Top = 25)

function Averages([string]$Path) {
    $lines = Get-Content $Path
    $headerLines = @(for ($i = 0; $i -lt $lines.Count; $i++) { if ($lines[$i].StartsWith('EVENTS,')) { $i } })
    $header = $lines[$headerLines[-1]].Split(',')
    $rows = New-Object System.Collections.Generic.List[string[]]
    for ($i = 1; $i -lt $lines.Count; $i++) {
        if ($lines[$i].StartsWith('EVENTS,') -or $lines[$i].StartsWith('[')) { break }
        $rows.Add($lines[$i].Split(','))
    }
    $rows = @($rows | Select-Object -Skip $Skip)
    $result = @{}
    for ($c = 0; $c -lt $header.Count; $c++) {
        if ($header[$c] -notmatch $Pattern) { continue }
        $sum = 0.0; $n = 0
        foreach ($r in $rows) {
            if ($c -lt $r.Count -and $r[$c] -ne '') { $sum += [double]::Parse($r[$c], [Globalization.CultureInfo]::InvariantCulture) }
            $n++
        }
        if ($n -gt 0) { $result[$header[$c]] = $sum / $n }
    }
    $result
}

$a = Averages $Before
$b = Averages $After
$names = @($a.Keys) + @($b.Keys) | Sort-Object -Unique
$names | ForEach-Object {
    $x = if ($a.ContainsKey($_)) { $a[$_] } else { 0.0 }
    $y = if ($b.ContainsKey($_)) { $b[$_] } else { 0.0 }
    [pscustomobject]@{ Stat = $_; Before = [math]::Round($x, 2); After = [math]::Round($y, 2); Change = [math]::Round($y - $x, 2) }
} | Sort-Object { [math]::Abs($_.Change) } -Descending | Select-Object -First $Top | Format-Table -AutoSize
