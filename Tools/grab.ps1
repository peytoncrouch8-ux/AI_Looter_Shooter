# Full-resolution screen grab of the editor window (it is brought to the front first), optionally cropped.
# Usage: grab.ps1 <name> [x0 y0 x1 y1 as fractions of the window] [-Scale 1.0]
param([string]$Name = 'grab', [double]$X0 = 0, [double]$Y0 = 0, [double]$X1 = 1, [double]$Y1 = 1, [double]$Scale = 1.0)

Add-Type -AssemblyName System.Drawing
Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public static class Grab {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
}
"@ -ErrorAction SilentlyContinue
[Grab]::SetProcessDPIAware() | Out-Null

$p = Get-Process UnrealEditor -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
[Grab]::keybd_event(0x12, 0, 0, [UIntPtr]::Zero); [Grab]::keybd_event(0x12, 0, 2, [UIntPtr]::Zero)
[Grab]::SetForegroundWindow($p.MainWindowHandle) | Out-Null
Start-Sleep -Milliseconds 400

$r = New-Object Grab+RECT; [Grab]::GetWindowRect($p.MainWindowHandle, [ref]$r) | Out-Null
$w = $r.Right - $r.Left; $h = $r.Bottom - $r.Top
$x = [int]($r.Left + $w * $X0); $y = [int]($r.Top + $h * $Y0)
$cw = [int]($w * ($X1 - $X0)); $ch = [int]($h * ($Y1 - $Y0))
$bmp = New-Object System.Drawing.Bitmap $cw, $ch
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($x, $y, 0, 0, (New-Object System.Drawing.Size $cw, $ch))
$g.Dispose()
if ($Scale -ne 1.0) {
    $sw = [int]($cw * $Scale); $sh = [int]($ch * $Scale)
    $scaled = New-Object System.Drawing.Bitmap $sw, $sh
    $sg = [System.Drawing.Graphics]::FromImage($scaled)
    $sg.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $sg.DrawImage($bmp, 0, 0, $sw, $sh); $sg.Dispose(); $bmp.Dispose(); $bmp = $scaled
}
$dir = Join-Path (Split-Path $PSScriptRoot -Parent) "Saved\Screenshots\Tools"; New-Item -ItemType Directory -Force -Path $dir | Out-Null; $out = Join-Path $dir "$Name.png"
$bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()
"$out ($w x $h window)"
