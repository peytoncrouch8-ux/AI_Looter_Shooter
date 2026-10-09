# Clicks a point in one of the editor's dialog windows (a Save Content dialog's Don't Save, a message's OK), found with
# Tools\editorwindows.ps1 and read with Tools\winshot.ps1 first. The point is in the window's own pixels, as winshot's
# picture shows them. It brings that window to the front and clicks only if it really is in front, so a click can never
# land in another program (the same guard as input.ps1). Slate draws its own buttons, so UI Automation can't press them.
# Usage: dialogclick.ps1 <window handle> <x> <y>
#   A Save Content dialog (646x532): Don't Save is at 493 502.  A one-line Message (576x155): OK is at 535 127.
param([Parameter(Mandatory = $true)][long]$Handle, [Parameter(Mandatory = $true)][int]$X, [Parameter(Mandatory = $true)][int]$Y)
Add-Type -TypeDefinition @"
using System; using System.Runtime.InteropServices;
public static class DialogClick {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern void mouse_event(uint f, int x, int y, uint d, UIntPtr e);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
"@
$h = [IntPtr]$Handle
[DialogClick]::SetForegroundWindow($h) | Out-Null
Start-Sleep -Milliseconds 400
if ([DialogClick]::GetForegroundWindow() -ne $h) { "That window is not in front: nothing clicked."; exit 1 }
$r = New-Object DialogClick+RECT
[DialogClick]::GetWindowRect($h, [ref]$r) | Out-Null
[DialogClick]::SetCursorPos($r.Left + $X, $r.Top + $Y) | Out-Null
Start-Sleep -Milliseconds 150
[DialogClick]::mouse_event(2, 0, 0, 0, [UIntPtr]::Zero)
Start-Sleep -Milliseconds 60
[DialogClick]::mouse_event(4, 0, 0, 0, [UIntPtr]::Zero)
"clicked $($r.Left + $X),$($r.Top + $Y)"
