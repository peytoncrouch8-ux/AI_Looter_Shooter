# Runs every Looter.* automation test in the editor and prints the results (failures with their errors).
# Usage: runtests.ps1 [-Filter Looter] [-OneRun]
# The tests run in batches, one per group (Looter.Bosses, Looter.World, ...), with the garbage collected after each. A test
# world's scene keeps its GPU Scene buffers (reserved, about a gigabyte of address space each) until the world is
# collected, and the editor collects only about once a minute: run all at once, ~200 finished test worlds piled up past
# the RHI's 256 GB budget and the driver removed the device (2026-10-07). -OneRun runs them in a single batch as before.
param([string]$Filter = 'Looter', [switch]$OneRun)
$s = $PSScriptRoot; $Toolset = 'AutomationTestToolset.AutomationTestToolset'
& "$s\mcp.ps1" $Toolset "DiscoverTests" '{"bForceRediscover":true}' | Out-Null
$list = ((& "$s\mcp.ps1" $Toolset "ListTests" (@{ nameFilter = $Filter; tagFilter = ''; limit = 1000 } | ConvertTo-Json -Compress)) | ConvertFrom-Json).returnValue | ConvertFrom-Json
$names = @($list.tests)
# PowerShell variables ignore case, so nothing here is named like the toolset.
$batches = @()
if ($OneRun) { $batches += ,$names }
else { foreach ($group in ($names | Group-Object { ($_ -split '\.')[0..1] -join '.' } | Sort-Object Name)) { $batches += ,@($group.Group) } }
$results = @()
foreach ($batch in $batches) {
    & "$s\mcp.ps1" $Toolset "RunTests" (@{ testNames = @($batch) } | ConvertTo-Json -Compress) | Out-Null
    for ($i = 0; $i -lt 300; $i++) { Start-Sleep 2; $st = & "$s\mcp.ps1" $Toolset "GetTestStatus" '{}'; if ($st -notmatch 'InProcess|Running|running') { break } }
    $res = ((& "$s\mcp.ps1" $Toolset "GetTestResults" '{}') | ConvertFrom-Json).returnValue | ConvertFrom-Json
    if (-not $res) { "the editor stopped answering during $(@($batch)[0]) and the tests after it (crashed?)"; break }
    $results += @($res.tests)
    # The batch's finished test worlds, and their scenes, go now rather than at the next timed collection.
    & "$s\console.ps1" "obj gc" | Out-Null
}
$passed = @($results | Where-Object { $_.state -match 'Success' }).Count
"passed=$passed failed=$($results.Count - $passed) total=$($results.Count) of $($names.Count)"
foreach ($test in $results | Sort-Object name) {
    "  {0,-8} {1}" -f $test.state, $test.name
    if ($test.state -notmatch 'Success') { $test.errors | ForEach-Object { "      error: $_" } }
}
