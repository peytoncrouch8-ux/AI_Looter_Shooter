# Starts PIE, focuses the game viewport, and optionally runs console commands first (typed into the editor console).
# Usage: pie.ps1 [-Commands "Looter.DebugStance 1","..."]
# Sends no input unless the editor is running and the play session really started (input.ps1 also refuses to type into
# any window that isn't the editor).
param([string[]]$Commands = @())
if (-not (Get-Process UnrealEditor -ErrorAction SilentlyContinue)) { "editor not running"; exit 1 }
& "$PSScriptRoot\mcp.ps1" 'EditorToolset.LogsToolset' 'SetVerbosity' '{"category":"LogLooter","verbosity":"Verbose"}' | Out-Null
& "$PSScriptRoot\mcp.ps1" 'EditorToolset.EditorAppToolset' 'StartPIE' '{"options":{}}' | Out-Null
Start-Sleep 6
$running = & "$PSScriptRoot\mcp.ps1" 'EditorToolset.EditorAppToolset' 'IsPIERunning' '{}' 2>$null
if ($running -notmatch 'true' -or -not (Get-Process UnrealEditor -ErrorAction SilentlyContinue)) { "PIE did not start (editor crashed or refused); no input sent"; exit 1 }
$steps = @("focus")
foreach ($c in $Commands) { $steps += @("clickat 0.25 0.984", "wait 300", "type $c", "key ENTER", "wait 300") }
$steps += @("clickat 0.43 0.7", "wait 800")
$sent = & "$PSScriptRoot\input.ps1" @steps 2>$null
if ($sent -notcontains 'done') { "input stopped: the editor was not in front"; exit 1 }
"pie started"
