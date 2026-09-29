# Play-test input driver: sends real OS keyboard/mouse input to the Unreal Editor window.
# Usage: input.ps1 "focus" "key F1" "wait 500" "move 200 0" "click" "down ALT" "up ALT" "clickat 0.5 0.5"
param([Parameter(ValueFromRemainingArguments = $true)][string[]]$Steps)

Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public static class Inp {
    [StructLayout(LayoutKind.Sequential)] public struct MOUSEINPUT { public int dx; public int dy; public uint mouseData; public uint dwFlags; public uint time; public IntPtr dwExtraInfo; }
    [StructLayout(LayoutKind.Sequential)] public struct KEYBDINPUT { public ushort wVk; public ushort wScan; public uint dwFlags; public uint time; public IntPtr dwExtraInfo; }
    [StructLayout(LayoutKind.Explicit)] public struct INPUTUNION { [FieldOffset(0)] public MOUSEINPUT mi; [FieldOffset(0)] public KEYBDINPUT ki; }
    [StructLayout(LayoutKind.Sequential)] public struct INPUT { public uint type; public INPUTUNION u; }
    [DllImport("user32.dll")] public static extern uint SendInput(uint n, INPUT[] inputs, int size);
    [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint code, uint mapType);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int c);
    [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr h);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
    [DllImport("user32.dll")] public static extern short VkKeyScan(char ch);
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }

    public static void Key(ushort vk, bool up) {
        uint flags = up ? 2u : 0u;
        if (vk == 0x2E || vk == 0xA5 || vk == 0xA3 || (vk >= 0x25 && vk <= 0x28)) flags |= 1u; // extended keys
        keybd_event((byte)vk, (byte)MapVirtualKey(vk, 0), flags, UIntPtr.Zero);
    }
    public static void Mouse(int dx, int dy, uint flags, uint data) {
        var i = new INPUT(); i.type = 0;
        i.u.mi.dx = dx; i.u.mi.dy = dy; i.u.mi.dwFlags = flags; i.u.mi.mouseData = data;
        SendInput(1, new[] { i }, Marshal.SizeOf(typeof(INPUT)));
    }
}
"@ -ErrorAction SilentlyContinue

$vk = @{
    'F1'=0x70; 'F5'=0x74; 'F8'=0x77; 'F10'=0x79; 'ESC'=0x1B; 'ESCAPE'=0x1B; 'ALT'=0xA4; 'LALT'=0xA4; 'CTRL'=0xA2; 'SHIFT'=0xA0; 'TAB'=0x09; 'ENTER'=0x0D;
    'DELETE'=0x2E; 'SPACE'=0x20; 'W'=0x57; 'A'=0x41; 'S'=0x53; 'D'=0x44; 'Q'=0x51; 'E'=0x45; 'R'=0x52; 'G'=0x47;
    'LEFT'=0x25; 'UP'=0x26; 'RIGHT'=0x27; 'DOWN'=0x28; 'F'=0x46; 'X'=0x58; 'B'=0x42; 'Z'=0x5A; 'I'=0x49; '1'=0x31; '2'=0x32; '3'=0x33; '4'=0x34; '5'=0x35; '6'=0x36; '7'=0x37; '8'=0x38; '9'=0x39
}

function Get-EditorWindow {
    Get-Process UnrealEditor -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
}

# True when the window that would receive input belongs to the Unreal Editor.
function Test-EditorForeground {
    $fg = [Inp]::GetForegroundWindow()
    if ($fg -eq [IntPtr]::Zero) { return $false }
    $procId = [uint32]0; [Inp]::GetWindowThreadProcessId($fg, [ref]$procId) | Out-Null
    $proc = Get-Process -Id $procId -ErrorAction SilentlyContinue
    return [bool]($proc -and $proc.ProcessName -eq 'UnrealEditor')
}

# Keys and clicks go to whatever window is in front. If that isn't the editor (it crashed, closed, or lost focus), stop:
# on 2026-09-29 a crashed editor let a console command get typed and sent in the Claude chat window.
function Stop-IfNotEditor([string]$Step) {
    if (-not (Test-EditorForeground)) {
        Write-Error "input.ps1: the Unreal Editor is not the foreground window; stopped before '$Step' so no input reaches another app."
        exit 1
    }
}

foreach ($step in $Steps) {
    # Delete and Ctrl reach the level editor whenever the game has not got focus (Ctrl+A, Delete wiped a level once).
    # They only go through when the step is forced with a leading "!".
    $forced = $step.StartsWith('!'); if ($forced) { $step = $step.Substring(1) }
    if (-not $forced -and $step -match '(?i)^(key|down|hold)\s+(DELETE|CTRL)\b') { Write-Warning "input.ps1: refusing '$step' (prefix with ! to force)"; continue }
    $parts = $step -split '\s+'
    $action = $parts[0].ToLower()
    if ($action -ne 'focus' -and $action -ne 'wait') { Stop-IfNotEditor $step }
    switch ($action) {
        'focus' {
            $p = Get-EditorWindow
            if (-not $p) { Write-Error "input.ps1: the Unreal Editor is not running; no input sent."; exit 1 }
            if ([Inp]::IsIconic($p.MainWindowHandle)) { [Inp]::ShowWindow($p.MainWindowHandle, 9) | Out-Null }
            # A shrunken editor window puts clicks outside the game viewport: maximize it.
            $wr = New-Object Inp+RECT; [Inp]::GetWindowRect($p.MainWindowHandle, [ref]$wr) | Out-Null
            if (($wr.Right - $wr.Left) -lt 1200 -or ($wr.Bottom - $wr.Top) -lt 700) { [Inp]::ShowWindow($p.MainWindowHandle, 3) | Out-Null; Start-Sleep -Milliseconds 500 }
            # Tap Alt so Windows allows the foreground change, then focus the editor. The switch can take a moment, so
            # wait for it (asking again now and then) before anything is typed.
            for ($try = 0; $try -lt 20 -and -not (Test-EditorForeground); $try++) {
                if ($try % 5 -eq 0) {
                    [Inp]::keybd_event(0x12, 0, 0, [UIntPtr]::Zero); [Inp]::keybd_event(0x12, 0, 2, [UIntPtr]::Zero)
                    [Inp]::SetForegroundWindow($p.MainWindowHandle) | Out-Null
                }
                Start-Sleep -Milliseconds 100
            }
            Start-Sleep -Milliseconds 200
            Stop-IfNotEditor $step
        }
        'clickat' {
            # Click at a fraction of the editor window (to give the PIE viewport focus).
            $p = Get-EditorWindow; $r = New-Object Inp+RECT; [Inp]::GetWindowRect($p.MainWindowHandle, [ref]$r) | Out-Null
            $x = [int]($r.Left + ($r.Right - $r.Left) * [double]$parts[1]); $y = [int]($r.Top + ($r.Bottom - $r.Top) * [double]$parts[2])
            [Inp]::SetCursorPos($x, $y) | Out-Null; Start-Sleep -Milliseconds 50
            [Inp]::Mouse(0, 0, 0x2, 0); Start-Sleep -Milliseconds 60; [Inp]::Mouse(0, 0, 0x4, 0)
        }
        'key'   { $c = $vk[$parts[1].ToUpper()]; [Inp]::Key($c, $false); Start-Sleep -Milliseconds 80; [Inp]::Key($c, $true) }
        'down'  { [Inp]::Key($vk[$parts[1].ToUpper()], $false) }
        'up'    { [Inp]::Key($vk[$parts[1].ToUpper()], $true) }
        'hold'  { $c = $vk[$parts[1].ToUpper()]; [Inp]::Key($c, $false); Start-Sleep -Milliseconds ([int]$parts[2]); [Inp]::Key($c, $true) }
        'move'  {
            # Relative mouse motion in small steps so the game sees continuous movement.
            $dx = [int]$parts[1]; $dy = [int]$parts[2]; $n = 20
            for ($k = 0; $k -lt $n; $k++) { [Inp]::Mouse([int]($dx / $n), [int]($dy / $n), 0x1, 0); Start-Sleep -Milliseconds 10 }
        }
        'click' { [Inp]::Mouse(0, 0, 0x2, 0); Start-Sleep -Milliseconds 60; [Inp]::Mouse(0, 0, 0x4, 0) }
        # Put the cursor at a fraction of the editor window without clicking (hover), and press/release the left button.
        'moveto' {
            $p = Get-EditorWindow; $r = New-Object Inp+RECT; [Inp]::GetWindowRect($p.MainWindowHandle, [ref]$r) | Out-Null
            [Inp]::SetCursorPos([int]($r.Left + ($r.Right - $r.Left) * [double]$parts[1]), [int]($r.Top + ($r.Bottom - $r.Top) * [double]$parts[2])) | Out-Null
        }
        'mdown' { [Inp]::Mouse(0, 0, 0x2, 0) }
        'mup'   { [Inp]::Mouse(0, 0, 0x4, 0) }
        'rclick' { [Inp]::Mouse(0, 0, 0x8, 0); Start-Sleep -Milliseconds 60; [Inp]::Mouse(0, 0, 0x10, 0) }
        # Hold the left button for N ms (full-auto bursts).
        'fire'  { [Inp]::Mouse(0, 0, 0x2, 0); Start-Sleep -Milliseconds ([int]$parts[1]); [Inp]::Mouse(0, 0, 0x4, 0) }
        'wheel' { [Inp]::Mouse(0, 0, 0x800, [BitConverter]::ToUInt32([BitConverter]::GetBytes([int]$parts[1] * 120), 0)) }
        'wait'  { Start-Sleep -Milliseconds ([int]$parts[1]) }
        # Types the rest of the step as text (letters, digits, punctuation on the US layout).
        'type'  {
            foreach ($ch in ($step.Substring(5)).ToCharArray()) {
                Stop-IfNotEditor $step
                $code = [Inp]::VkKeyScan($ch); $vkc = [byte]($code -band 0xFF); $shift = ($code -band 0x100) -ne 0
                if ($shift) { [Inp]::keybd_event(0xA0, 0, 0, [UIntPtr]::Zero) }
                [Inp]::keybd_event($vkc, 0, 0, [UIntPtr]::Zero); Start-Sleep -Milliseconds 15; [Inp]::keybd_event($vkc, 0, 2, [UIntPtr]::Zero)
                if ($shift) { [Inp]::keybd_event(0xA0, 0, 2, [UIntPtr]::Zero) }
                Start-Sleep -Milliseconds 15
            }
        }
    }
    Start-Sleep -Milliseconds 60
}
"done"
