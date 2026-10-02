# Closes the editor's floating "Message Log" window (it pops up with automation results and steals keyboard input), or
# another floating editor window by its title (a "Content Browser" an asset script opened over the game).
# Usage: closemsglog.ps1 [-Title "Content Browser"]. Clicks nothing if no such window is open.
param([string]$Title = 'Message Log')
Add-Type @"
using System; using System.Text; using System.Runtime.InteropServices;
public static class MsgLogWnd {
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr l);
  [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint msg, IntPtr w, IntPtr l);
}
"@ -ErrorAction SilentlyContinue
$found = New-Object System.Collections.ArrayList
[MsgLogWnd]::EnumWindows({ param($h, $l)
    if ([MsgLogWnd]::IsWindowVisible($h)) { $sb = New-Object Text.StringBuilder 256; [void][MsgLogWnd]::GetWindowText($h, $sb, 256); if ($sb.ToString() -eq $Title) { [void]$found.Add($h) } }
    $true }, [IntPtr]::Zero) | Out-Null
foreach ($h in $found) { [void][MsgLogWnd]::PostMessage($h, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) }  # WM_CLOSE
"closed $($found.Count) $Title window(s)"
