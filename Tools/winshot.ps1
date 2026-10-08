# Captures one window by its handle without bringing it forward (PrintWindow), such as the editor's "Save Content"
# dialog that Tools\editorwindows.ps1 lists, so its contents can be read before answering it.
# Usage: winshot.ps1 <handle> [name]   (writes Saved\Screenshots\Tools\<name>.png and prints the path)
param([long]$Handle, [string]$Name = 'win')
Add-Type -AssemblyName System.Drawing
if (-not ('WSW' -as [type])) {
Add-Type @'
using System; using System.Runtime.InteropServices;
public static class WSW { [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint f);
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
 public struct RECT { public int L, T, R, B; } }
'@
}
$h = [IntPtr]$Handle
$r = New-Object WSW+RECT; [WSW]::GetWindowRect($h, [ref]$r) | Out-Null
$bmp = New-Object System.Drawing.Bitmap ($r.R - $r.L), ($r.B - $r.T); $g = [System.Drawing.Graphics]::FromImage($bmp)
$dc = $g.GetHdc(); [WSW]::PrintWindow($h, $dc, 2) | Out-Null; $g.ReleaseHdc($dc)
$dir = Join-Path (Split-Path $PSScriptRoot -Parent) 'Saved\Screenshots\Tools'
New-Item -ItemType Directory -Force $dir | Out-Null
$out = Join-Path $dir "$Name.png"
$bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Png); $out
