# Runs every Looter.* automation test in the editor and prints the results (failures with their errors).
# Usage: runtests.ps1 [-Filter Looter]
param([string]$Filter = 'Looter')
$s = $PSScriptRoot; $T = 'AutomationTestToolset.AutomationTestToolset'
& "$s\mcp.ps1" $T "DiscoverTests" '{"bForceRediscover":true}' | Out-Null
$list = ((& "$s\mcp.ps1" $T "ListTests" (@{ nameFilter = $Filter; tagFilter = ''; limit = 1000 } | ConvertTo-Json -Compress)) | ConvertFrom-Json).returnValue | ConvertFrom-Json
$names = @($list.tests)
& "$s\mcp.ps1" $T "RunTests" (@{ testNames = $names } | ConvertTo-Json -Compress) | Out-Null
for ($i = 0; $i -lt 120; $i++) { Start-Sleep 2; $st = & "$s\mcp.ps1" $T "GetTestStatus" '{}'; if ($st -notmatch 'InProcess|Running|running') { break } }
$res = ((& "$s\mcp.ps1" $T "GetTestResults" '{}') | ConvertFrom-Json).returnValue | ConvertFrom-Json
"passed=$($res.passed) failed=$($res.failed) total=$($res.total)"
foreach ($t in $res.tests | Sort-Object name) {
    "  {0,-8} {1}" -f $t.state, $t.name
    if ($t.state -notmatch 'Success') { $t.errors | ForEach-Object { "      error: $_" } }
}
