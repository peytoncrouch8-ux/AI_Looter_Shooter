# One step of a play check in a running play session (PIE): moves the player with Tools/Unreal/play.py, presses keys in the
# game, says what the player looks at, lists the running missions and grabs the screen, in that order (each optional).
# Usage: playstep.ps1 [-Py "goto Speaker_Delia 200"] [-Cmd "Looter.Mission.Complete Main2"] [-Keys "key TAB","wait 900"]
#                     [-Button ACCEPT] [-Focus] [-List] [-Shot name]
# Game actions (Interact, Fire, Move...) go in with play.py press, which needs no focus. Keys are for what has no input
# action (Tab, number keys): a left click in the viewport gives it focus back first (the console box takes it; only a
# left click does, and it fires a held gun). input.ps1 still refuses to type anywhere but the editor.
param([string]$Py = '', [string[]]$Keys = @(), [string[]]$Cmd = @(), [string]$Button = '', [switch]$Focus, [switch]$List, [string]$Shot = '',
    [double]$Settle = 0.6)
$tools = $PSScriptRoot
if ($Py) {
    & "$tools\console.ps1" "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/play.py $Py" -Until 'PLAY |Error: ' |
        Where-Object { $_ -match 'PLAY |Error|Traceback|line \d+' -and $_ -notmatch '^Cmd:' }
    Start-Sleep -Milliseconds ([int]($Settle * 1000))
}
foreach ($c in $Cmd) {
    & "$tools\console.ps1" $c -Wait 1 | Where-Object { $_ -match 'LogLooter|Error|Warning' }
}
if ($Keys) {
    $sent = & "$tools\input.ps1" (@('focus', 'clickat 0.43 0.55', 'wait 250') + $Keys) 2>&1
    if ($sent -notcontains 'done') { "input stopped: $sent" }
}
if ($Button) {
    # A game menu's button (an offer card's ACCEPT, a choice): found in the Slate tree once it shows (30 s), then clicked
    # with the real mouse at its middle (the inspector's own Click doesn't reach widgets in the game's viewport).
    $S = 'SlateInspectorToolset.SlateInspectorToolset'
    $found = $null
    for ($i = 0; $i -lt 60 -and -not $found; $i++) {
        $snap = (& "$tools\mcp.ps1" $S 'Snapshot' '{"ref":"","maxDepth":80,"bIncludeSourceLocations":false}' | Out-String).Replace('\n', "`n").Replace('\"', '"')
        $m = [regex]::Match($snap, "button `"$([regex]::Escape($Button))`" \[pos=(-?\d+),(-?\d+) size=(\d+),(\d+)\]")
        if ($m.Success) { $found = $m } else { Start-Sleep -Milliseconds 500 }
    }
    if ($found) {
        $p = Get-Process UnrealEditor | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
        Add-Type -TypeDefinition 'using System; using System.Runtime.InteropServices; public static class PsWin { [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; } [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r); }' -ErrorAction SilentlyContinue
        $r = New-Object PsWin+RECT; [PsWin]::GetWindowRect($p.MainWindowHandle, [ref]$r) | Out-Null
        $x = ([int]$found.Groups[1].Value + [int]$found.Groups[3].Value / 2 - $r.L) / ($r.R - $r.L)
        $y = ([int]$found.Groups[2].Value + [int]$found.Groups[4].Value / 2 - $r.T) / ($r.B - $r.T)
        $sent = & "$tools\input.ps1" 'focus' ("clickat {0:0.0000} {1:0.0000}" -f $x, $y) 'wait 800' 2>&1
        if ($sent -contains 'done') { "clicked $Button" } else { "input stopped: $sent" }
    } else { "no button '$Button' showed" }
}
if ($Focus) { & "$tools\console.ps1" 'Looter.Interaction.Focus' | Where-Object { $_ -match 'Focus:' } }
if ($List) {
    $lines = & "$tools\console.ps1" 'Looter.Mission.List' -Wait 1.5 | Where-Object { $_ -match 'LogLooter' }
    $show = $false
    foreach ($l in $lines) {
        if ($l -match '^\S*LogLooter:\s{3}(\w+)\s') { $show = $Matches[1] -ne 'locked' -and $Matches[1] -ne 'completed' }
        if ($show -or $l -match 'Campaign:') { $l -replace '^LogLooter: ', '' }
    }
    & "$tools\console.ps1" 'py C:/Dev/AI_Looter_Shooter/Tools/Unreal/play.py guns' -Until 'PLAY ' | Where-Object { $_ -match 'PLAY ' }
}
if ($Shot) { & "$tools\grab.ps1" $Shot -Scale 0.5 }
