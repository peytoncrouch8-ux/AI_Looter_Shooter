# Stops PIE and closes the Unreal Editor cleanly. Reports a blocking save dialog instead of forcing anything.
$p = Get-Process UnrealEditor -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
if (-not $p) { "editor not running"; exit 0 }
$j = Start-Job -ArgumentList $PSScriptRoot -ScriptBlock { param($d) & "$d\mcp.ps1" 'EditorToolset.EditorAppToolset' 'StopPIE' '{}' }
Wait-Job $j -Timeout 20 | Out-Null; Remove-Job $j -Force
Start-Sleep 2
# A floating panel (the Content Browser) can hold the process's "main window", and closing that only closes the panel
# (2026-10-08): then ask the editor itself to quit.
if ($p.MainWindowTitle -like '*Unreal Editor*') { $p.CloseMainWindow() | Out-Null }
else { $q = Start-Job -ArgumentList $PSScriptRoot -ScriptBlock { param($d) & "$d\console.ps1" 'QUIT_EDITOR' }; Wait-Job $q -Timeout 20 | Out-Null; Remove-Job $q -Force }
if (-not $p.WaitForExit(90000)) { "EDITOR DID NOT CLOSE (save dialog?)"; exit 2 }
"editor closed"
