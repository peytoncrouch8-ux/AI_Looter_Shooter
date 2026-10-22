# Photographs the heroes holding the guns in third person (Tools/Unreal/hero_guns.py) and tiles one contact sheet per
# hero: rows gun x state, columns the views. Sheets: Saved\Screenshots\HeroGuns\<Hero>_<Tag>.png; the pictures in
# Saved\Screenshots\HeroGuns\<Tag>\. Docs/Pipeline/Heroes.md "The hero pick" (the third-person hold).
# Usage: heroguns.ps1 [-Tag now] [-Heroes Ellis,Crane] [-Slots 1,2,3] [-States hip,aim] [-Views 330,270,180] [-Setup]
# -Setup starts a play in Ransom's Rest when none runs (pie.ps1; check nobody is playing first: pie_check.py), gives the
# Bullpup, Ranchhand and Drover (slots 1-3), fills the ammo and turns to third person. Every run stands the player in the
# town street (0, -1400) facing 195 degrees: the sun on the front and the right side.
# States: hip, aim, reload, sprint, crouch, crouchaim. Views: degrees round from behind (330 behind the right shoulder,
# 270 right side, 180 front, 90 left side, -1 the game's camera). About 20 s a hero for the default.
param([string]$Tag = 'now', [string[]]$Heroes = @('Ellis', 'Odessa', 'Crane', 'Gauge', 'Pike'), [string[]]$Slots = @('1', '2', '3'),
    [string[]]$States = @('hip', 'aim'), [string[]]$Views = @('330', '270', '180'), [switch]$Setup)
$root = Split-Path $PSScriptRoot -Parent
$console = Join-Path $PSScriptRoot 'console.ps1'
$out = Join-Path $root 'Saved\Screenshots\HeroGuns'
New-Item -ItemType Directory -Force -Path $out | Out-Null

if ($Setup) {
    $running = & "$PSScriptRoot\mcp.ps1" 'EditorToolset.EditorAppToolset' 'IsPIERunning' '{}' 2>$null
    if ($running -notmatch 'true') {
        $started = & "$PSScriptRoot\pie.ps1" -Map RansomsRest
        if ($started -notcontains 'pie started') { "PIE did not start: $started"; exit 1 }
    }
    foreach ($gun in 'Rifle', 'Shotgun', 'Revolver') { & $console "Looter.GiveWeapon $gun Rare 5" -Until 'GiveWeapon:' | Out-Null }
    & $console "py $root/Tools/Unreal/play.py ammo" -Until 'PLAY' | Out-Null
    & $console "py $root/Tools/Unreal/play.py view 1" -Until 'PLAY' | Out-Null
}
& $console "py $root/Tools/Unreal/play.py at 0 -1400 400 195" -Until 'PLAY' | Out-Null
Set-Content (Join-Path $out 'args.txt') "$Tag $($Heroes -join ',') $($Slots -join ',') $($States -join ',') $($Views -join ',')" -Encoding ascii
$log = & $console "py $root/Tools/Unreal/hero_guns.py @" -Until 'HEROGUNS (done|failed)'
$result = $log | Select-String 'HEROGUNS (done|failed).*' | ForEach-Object { $_.Matches[0].Value }
$result
if ($result -notmatch 'done') { exit 2 }

# The sheets: each picture's middle (the character), half size, a label over each.
Add-Type -AssemblyName System.Drawing
$gunNames = @{ '1' = 'Bullpup'; '2' = 'Ranchhand'; '3' = 'Drover' }
$viewNames = @{ '-1' = 'game camera'; '0' = 'behind'; '330' = 'behind right'; '30' = 'behind left'; '90' = 'left side'; '270' = 'right side'; '180' = 'front' }
$tileW = 600; $tileH = 500; $label = 34
$font = New-Object System.Drawing.Font('Segoe UI', 15, [System.Drawing.FontStyle]::Bold)
$back = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(255, 24, 26, 30))
foreach ($hero in $Heroes) {
    $rows = @(foreach ($slot in $Slots) { foreach ($state in $States) { , @($slot, $state) } })
    $bmp = New-Object System.Drawing.Bitmap ($Views.Count * $tileW), ($rows.Count * ($tileH + $label))
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.FillRectangle($back, 0, 0, $bmp.Width, $bmp.Height)
    for ($r = 0; $r -lt $rows.Count; $r++) {
        for ($c = 0; $c -lt $Views.Count; $c++) {
            $slot = $rows[$r][0]; $state = $rows[$r][1]; $view = $Views[$c]
            $x = $c * $tileW; $y = $r * ($tileH + $label)
            $g.DrawString("$hero  $($gunNames[$slot])  $state  $($viewNames[$view])", $font, [System.Drawing.Brushes]::White, $x + 8, $y + 4)
            $file = Join-Path $out "$Tag\${hero}_${slot}_${state}_$view.png"
            if (Test-Path $file) {
                $img = [System.Drawing.Image]::FromFile($file)
                $g.DrawImage($img, (New-Object System.Drawing.Rectangle $x, ($y + $label), $tileW, $tileH),
                    (New-Object System.Drawing.Rectangle 360, 40, 1200, 1000), [System.Drawing.GraphicsUnit]::Pixel)
                $img.Dispose()
            }
        }
    }
    $g.Dispose()
    $sheet = Join-Path $out "${hero}_$Tag.png"
    $bmp.Save($sheet, [System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()
    $sheet
}
