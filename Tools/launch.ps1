# Opens the editor with the MCP server (optionally building first; a running editor is closed cleanly for the build)
# and waits until the server answers.
# Usage: launch.ps1 [-Build]
param([switch]$Build)
$root = Split-Path $PSScriptRoot -Parent
$engine = "C:\Program Files\Epic Games\UE_5.8"
$p = Get-Process UnrealEditor -ErrorAction SilentlyContinue
if ($p -and $Build) {
    $closed = & "$PSScriptRoot\close.ps1"
    if ($closed -match 'DID NOT CLOSE') { $closed; exit 2 }
    $p = $null
}
if ($Build) {
    $out = & "$engine\Engine\Build\BatchFiles\Build.bat" AI_Looter_ShooterEditor Win64 Development "-Project=$root\AI_Looter_Shooter.uproject" -WaitMutex -NoHotReload 2>&1
    $out | Select-String -Pattern 'error C|error LNK|: error|Result:' | Select-Object -Last 25 | ForEach-Object { $_.Line }
    if (-not ($out -match 'Result: Succeeded')) { "BUILD FAILED"; exit 1 }
}
if (-not $p) { Start-Process "$engine\Engine\Binaries\Win64\UnrealEditor.exe" -ArgumentList "`"$root\AI_Looter_Shooter.uproject`" -ModelContextProtocolStartServer" }
for ($i = 0; $i -lt 72; $i++) {
    Start-Sleep 5
    try { $r = & "$PSScriptRoot\mcp.ps1" 'EditorToolset.EditorAppToolset' 'IsPIERunning' '{}' 2>$null; if ($r -and $r -notmatch 'ERROR') { Start-Sleep 3; & "$PSScriptRoot\closemsglog.ps1" | Out-Null; "ready"; exit 0 } } catch {}
}
"timeout"
