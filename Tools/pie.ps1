# Starts PIE, focuses the game viewport, and optionally runs console commands first (typed into the editor console).
# Usage: pie.ps1 [-Commands "Looter.DebugStance 1","..."]
param([string[]]$Commands = @())
& "$PSScriptRoot\mcp.ps1" 'EditorToolset.LogsToolset' 'SetVerbosity' '{"category":"LogLooter","verbosity":"Verbose"}' | Out-Null
& "$PSScriptRoot\mcp.ps1" 'EditorToolset.EditorAppToolset' 'StartPIE' '{"options":{}}' | Out-Null
Start-Sleep 6
$steps = @("focus")
foreach ($c in $Commands) { $steps += @("clickat 0.25 0.984", "wait 300", "type $c", "key ENTER", "wait 300") }
$steps += @("clickat 0.43 0.7", "wait 800")
& "$PSScriptRoot\input.ps1" @steps | Out-Null
"pie started"
