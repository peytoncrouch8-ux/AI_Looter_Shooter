# Lists the Unreal Editor's visible top-level windows (handle | title | position size). Finds modal dialogs that block MCP.
Add-Type -TypeDefinition @'
using System; using System.Text; using System.Collections.Generic; using System.Runtime.InteropServices;
public static class WinEnum {
  public delegate bool EnumProc(IntPtr h, IntPtr p);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr p);
  [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
  public static List<string> List(uint pidWanted) {
    var o = new List<string>();
    EnumWindows((h, p) => { uint pid; GetWindowThreadProcessId(h, out pid); if (pid == pidWanted && IsWindowVisible(h)) { var sb = new StringBuilder(256); GetWindowText(h, sb, 256); RECT r; GetWindowRect(h, out r); o.Add(h.ToString() + " | " + sb.ToString() + " | " + r.L + "," + r.T + " " + (r.R - r.L) + "x" + (r.B - r.T)); } return true; }, IntPtr.Zero);
    return o;
  }
}
'@ -ErrorAction SilentlyContinue
$p = Get-Process UnrealEditor -ErrorAction SilentlyContinue | Select-Object -First 1
if ($p) { [WinEnum]::List([uint32]$p.Id) } else { "editor not running" }
