# Answers "Yes, keep waiting" on the editor's Zen startup prompt (the local Zen data cache server is slow to start, and a
# Windows message box asks whether to keep waiting for the editor to load; the user saw it 2026-10-28). The box is a plain
# Win32 dialog, so its Yes button is pressed with BM_CLICK: no mouse, no focus. Only a dialog of an UnrealEditor process
# whose text mentions Zen or waiting is touched. Prints what it pressed; nothing when there is no such box.
# Usage: zenwait.ps1   (launch.ps1 calls it while the editor starts)
Add-Type -TypeDefinition @'
using System; using System.Text; using System.Collections.Generic; using System.Runtime.InteropServices;
public static class ZenWait {
  public delegate bool EnumProc(IntPtr h, IntPtr p);
  [DllImport("user32.dll")] static extern bool EnumWindows(EnumProc cb, IntPtr p);
  [DllImport("user32.dll")] static extern bool EnumChildWindows(IntPtr parent, EnumProc cb, IntPtr p);
  [DllImport("user32.dll")] static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] static extern int GetClassName(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] static extern IntPtr SendMessage(IntPtr h, uint msg, IntPtr w, IntPtr l);
  const uint BM_CLICK = 0x00F5;
  static string Text(IntPtr h) { var s = new StringBuilder(1024); GetWindowText(h, s, 1024); return s.ToString(); }
  static string Class(IntPtr h) { var s = new StringBuilder(256); GetClassName(h, s, 256); return s.ToString(); }
  public static List<string> Press(HashSet<uint> pids) {
    var done = new List<string>();
    EnumWindows((h, p) => {
      uint pid; GetWindowThreadProcessId(h, out pid);
      if (!IsWindowVisible(h) || Class(h) != "#32770") return true;
      // The editor's own box, or another process's box about the editor (a helper's or Windows' own "keep waiting").
      if (!pids.Contains(pid) && !Text(h).ToLowerInvariant().Contains("unreal")) return true;
      var words = new StringBuilder(Text(h));
      var buttons = new List<IntPtr>();
      EnumChildWindows(h, (c, q) => {
        string cls = Class(c), t = Text(c);
        if (cls == "Button") buttons.Add(c); else words.Append(" ").Append(t);
        return true; }, IntPtr.Zero);
      string all = words.ToString().ToLowerInvariant();
      if (!(all.Contains("zen") || all.Contains("keep waiting") || all.Contains("continue waiting"))) return true;
      foreach (var b in buttons) {
        string t = Text(b).Replace("&", "").Trim().ToLowerInvariant();
        if (t == "yes" || t == "wait" || t.StartsWith("keep waiting") || t.StartsWith("continue")) {
          SendMessage(b, BM_CLICK, IntPtr.Zero, IntPtr.Zero);
          done.Add("pressed '" + Text(b).Replace("&", "") + "' on: " + words.ToString().Trim());
          break; } }
      return true; }, IntPtr.Zero);
    return done; }
}
'@
$pids = New-Object 'System.Collections.Generic.HashSet[uint32]'
Get-Process UnrealEditor -ErrorAction SilentlyContinue | ForEach-Object { [void]$pids.Add([uint32]$_.Id) }
if ($pids.Count -eq 0) { exit 0 }
[ZenWait]::Press($pids) | ForEach-Object { "zenwait: $_" }
