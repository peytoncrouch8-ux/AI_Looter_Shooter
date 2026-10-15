# Assembles the Gravemother fight concept sheets from GravemotherFightConcepts.py's renders and JSON (run after it):
#   Art/Backlog/Creatures/GravemotherFightSheets.ps1 [-Letters A,B] [-Overview]
param([string[]]$Letters = @('A', 'B', 'C', 'D'), [switch]$Overview)
Add-Type -AssemblyName System.Drawing
$root = 'C:\Dev\AI_Looter_Shooter'
$src = Join-Path $root 'Saved\ArtPreviews\RansomsRest\Concepts\GravemotherFight'
$out = Join-Path $root 'Saved\Concepts\Gravemother'
New-Item -ItemType Directory -Force $out | Out-Null

$pfc = New-Object System.Drawing.Text.PrivateFontCollection
$pfc.AddFontFile((Join-Path $root 'Content\UI\Fonts\ChakraPetch-Regular.ttf'))
$pfc.AddFontFile((Join-Path $root 'Content\UI\Fonts\ChakraPetch-Bold.ttf'))
$fam = $pfc.Families[0]
function F([float]$size, [bool]$bold = $false) {
    $style = if ($bold) { [System.Drawing.FontStyle]::Bold } else { [System.Drawing.FontStyle]::Regular }
    New-Object System.Drawing.Font($fam, $size, $style, [System.Drawing.GraphicsUnit]::Pixel)
}
function C([string]$hex, [int]$a = 255) {
    $v = [Convert]::ToInt32($hex.TrimStart('#'), 16)
    [System.Drawing.Color]::FromArgb($a, ($v -shr 16) -band 255, ($v -shr 8) -band 255, $v -band 255)
}
function B($color) { New-Object System.Drawing.SolidBrush($color) }
function P($color, [float]$w) { $p = New-Object System.Drawing.Pen($color, $w); $p.LineJoin = 'Round'; $p }

$BG = C '121418'; $ORANGE = C 'F2922E'; $CYAN = C '5FD2E0'; $YELLOW = C 'FFD24A'
$WHITE = C 'F2EEE6'; $TEXT = C 'DAD4C9'; $DIM = C 'A9A297'; $FRAME = C '2C3038'
$LABEL = @{ 'Does' = $WHITE; 'Tell' = $ORANGE; 'Answer' = $CYAN; 'Meshes' = $WHITE; 'Cards' = $ORANGE;
            'Decals' = $CYAN; 'Emissive meshes' = $WHITE }
$fTitle = F 66 $true; $fPitch = F 34; $fPanel = F 38 $true; $fLabel = F 30 $true; $fBody = F 30; $fCap = F 28
$fCallT = F 32 $true; $fCallS = F 26; $fFx = F 25 $true; $fKey = F 28 $true; $fHex = F 24; $fFoot = F 24

$measure = [System.Drawing.Graphics]::FromImage((New-Object System.Drawing.Bitmap 8, 8))
function Wrap([string]$text, $font, [float]$width) {
    $lines = New-Object System.Collections.Generic.List[string]
    $cur = ''
    foreach ($w in ($text -split ' ')) {
        $try = if ($cur) { "$cur $w" } else { $w }
        if ($cur -and $measure.MeasureString($try, $font).Width -gt $width) { $lines.Add($cur); $cur = $w } else { $cur = $try }
    }
    if ($cur) { $lines.Add($cur) }
    , $lines
}
function TextW([string]$t, $font) { $measure.MeasureString($t, $font).Width }

function DrawLines($g, $lines, $font, $brush, [float]$x, [float]$y, [float]$lh) {
    foreach ($l in $lines) { $g.DrawString($l, $font, $brush, $x, $y); $y += $lh }
    $y
}

# A panel's caption: its title, then label / text rows with a hanging indent.
function CaptionHeight($panel, [float]$w) {
    $h = 52
    foreach ($row in $panel[1]) {
        $lab = $row[0]; $indent = [Math]::Max(130, (TextW $lab $fLabel) + 16)
        $h += (Wrap $row[1] $fBody ($w - $indent)).Count * 38 + 8
    }
    $h
}
function DrawCaption($g, $panel, [float]$x, [float]$y, [float]$w) {
    $g.DrawString($panel[0], $fPanel, (B $WHITE), $x, $y)
    $y += 52
    foreach ($row in $panel[1]) {
        $lab = $row[0]; $indent = [Math]::Max(130, (TextW $lab $fLabel) + 16)
        $col = if ($LABEL.ContainsKey($lab)) { $LABEL[$lab] } else { $WHITE }
        $g.DrawString($lab, $fLabel, (B $col), $x, $y)
        $y = DrawLines $g (Wrap $row[1] $fBody ($w - $indent)) $fBody (B $TEXT) ($x + $indent) $y 38
        $y += 8
    }
    $y
}

function MarkColor([string]$key) {
    if ($key -in @('head', 'abdomen', 'bare')) { return $ORANGE }
    if ($key -in @('tell', 'line')) { return $YELLOW }
    $CYAN
}

# Callouts: a ring on each point, a leader to a labelled box. 'columns' stacks boxes down the left and right edges.
function DrawCallouts($g, [float]$ox, [float]$oy, [float]$w, [float]$h, $marks, $labels, [string]$mode) {
    $items = @()
    foreach ($m in $marks) {
        $lab = $labels.($m.key)
        if (-not $lab -or -not $lab[0]) { continue }
        $bw = [Math]::Max((TextW $lab[0] $fCallT), (TextW $lab[1] $fCallS)) + 26
        $items += [pscustomobject]@{ key = $m.key; x = [float]$m.x; y = [float]$m.y; t = $lab[0]; s = $lab[1]; bw = $bw; bh = 82.0; bx = 0.0; by = 0.0 }
    }
    if ($mode -eq 'columns') {
        foreach ($side in 'L', 'R') {
            $col = @($items | Where-Object { if ($side -eq 'L') { $_.x -lt $w * 0.5 } else { $_.x -ge $w * 0.5 } } | Sort-Object y)
            $prev = -1000.0
            foreach ($it in $col) {
                $it.by = [Math]::Max([Math]::Min($it.y - 120, $h - $it.bh - 14), 14)
                if ($it.by -lt $prev + $it.bh + 14) { $it.by = $prev + $it.bh + 14 }
                $prev = $it.by
                $it.bx = if ($side -eq 'L') { [Math]::Max(24, $it.x - 380 - $it.bw) } else { [Math]::Min($w - 24 - $it.bw, $it.x + 380) }
            }
        }
    } else {
        foreach ($it in $items) {
            $it.bx = [Math]::Min([Math]::Max($it.x + 50, 12), $w - $it.bw - 12)
            $it.by = [Math]::Min([Math]::Max($it.y - 130, 12), $h - $it.bh - 12)
            if ($it.bx -lt $it.x -and $it.bx + $it.bw -gt $it.x -and $it.by + $it.bh -gt $it.y - 30) { $it.by = [Math]::Max($it.y - 30 - $it.bh - 20, 12) }
        }
    }
    foreach ($it in $items) {
        $col = MarkColor $it.key
        $px, $py = ($ox + $it.x), ($oy + $it.y)
        $bx, $by = ($ox + $it.bx), ($oy + $it.by)
        $ex = if ($bx + $it.bw / 2 -lt $px) { $bx + $it.bw } else { $bx }
        $ey = $by + $it.bh / 2
        $shadow = P (C '000000' 150) 6
        $g.DrawLine($shadow, $px, $py, $ex, $ey)
        $g.DrawLine((P $col 3), $px, $py, $ex, $ey)
        $g.DrawEllipse($shadow, $px - 18, $py - 18, 36, 36)
        $g.DrawEllipse((P $col 4), $px - 18, $py - 18, 36, 36)
        $g.FillEllipse((B $col), $px - 4, $py - 4, 8, 8)
        $g.FillRectangle((B (C '0B0C0F' 205)), $bx, $by, $it.bw, $it.bh)
        $g.FillRectangle((B $col), $bx, $by, 5, $it.bh)
        $g.DrawString($it.t, $fCallT, (B $col), $bx + 13, $by + 6)
        $g.DrawString($it.s, $fCallS, (B $TEXT), $bx + 13, $by + 44)
    }
}

function DrawImage($g, [string]$file, [float]$x, [float]$y, [float]$w, [float]$h) {
    $img = [System.Drawing.Image]::FromFile($file)
    $g.DrawImage($img, $x, $y, $w, $h)
    $img.Dispose()
    $g.DrawRectangle((P $FRAME 2), $x - 1, $y - 1, $w + 1, $h + 1)
}

$W = 2000; $M = 20; $PW = 970; $PH = 560; $GAP = 20
$heroes = @{}
foreach ($L in $Letters) {
    $j = Get-Content (Join-Path $src "GMF_$L.json") -Raw -Encoding UTF8 | ConvertFrom-Json
    $bmp = New-Object System.Drawing.Bitmap $W, 5200
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = 'AntiAlias'; $g.InterpolationMode = 'HighQualityBicubic'; $g.TextRenderingHint = 'AntiAliasGridFit'
    $g.Clear($BG)
    # Header.
    $y = 18.0
    $g.DrawString("$L   $($j.name)", $fTitle, (B $ORANGE), $M - 4, $y)
    $tag = 'The Gravemother  |  fight concept'
    $g.DrawString($tag, $fCap, (B $DIM), $W - $M - (TextW $tag $fCap), $y + 34)
    $y += 84
    $y = DrawLines $g (Wrap $j.pitch $fPitch ($W - 2 * $M)) $fPitch (B $WHITE) $M $y 44
    $y += 12
    # Hero with its callouts.
    $hero = $j.shots.hero
    DrawImage $g (Join-Path $src $hero.file) $M $y 1960 1000
    DrawCallouts $g $M $y 1960 1000 $hero.marks $j.marks 'columns'
    $y += 1008
    $y = DrawLines $g (Wrap $j.hero $fCap ($W - 2 * $M)) $fCap (B $DIM) $M $y 36
    $y += 22
    # Panels, two by two, then the FX parts beside the colour key.
    $pairs = @(@('spit', 'tell'), @('charge', 'special'))
    foreach ($pair in $pairs) {
        $ch = 0
        for ($i = 0; $i -lt 2; $i++) {
            $shot = $pair[$i]; $x = $M + $i * ($PW + $GAP)
            $s = $j.shots.$shot
            DrawImage $g (Join-Path $src $s.file) $x $y $PW $PH
            if ($shot -ne 'charge' -and -not ($shot -eq 'special' -and $L -eq 'A')) {
                DrawCallouts $g $x $y $PW $PH @($s.marks | Where-Object { $_.key -ne 'head' }) $j.marks 'near'
            }
            $ch = [Math]::Max($ch, (CaptionHeight $j.panels.$shot ($PW - 8)))
        }
        for ($i = 0; $i -lt 2; $i++) {
            $x = $M + $i * ($PW + $GAP)
            [void](DrawCaption $g $j.panels.($pair[$i]) ($x + 4) ($y + $PH + 14) ($PW - 8))
        }
        $y += $PH + 14 + $ch + 24
    }
    # FX parts with their labels.
    $fx = $j.shots.fx
    DrawImage $g (Join-Path $src $fx.file) $M $y $PW $PH
    foreach ($mk in $fx.marks) {
        $t = $j.fx_labels.($mk.key)
        $tw = (TextW $t $fFx) + 18
        $lx = [Math]::Min([Math]::Max($M + $mk.x - $tw / 2, $M + 6), $M + $PW - $tw - 6)
        $ly = $y + $mk.y + 4
        $g.FillRectangle((B (C '0B0C0F' 200)), $lx, $ly, $tw, 36)
        $g.DrawString($t, $fFx, (B $WHITE), $lx + 9, $ly + 3)
    }
    $fxCapY = $y + $PH + 14
    $fxEnd = DrawCaption $g $j.panels.fx ($M + 4) $fxCapY ($PW - 8)
    # Colour key.
    $kx = $M + $PW + $GAP + 4
    $g.DrawString('Colour key', $fPanel, (B $WHITE), $kx, $y)
    $ky = $y + 60
    for ($i = 0; $i -lt $j.key.Count; $i++) {
        $k = $j.key[$i]
        $cx = $kx + ($i % 2) * 470; $cy = $ky + [Math]::Floor($i / 2) * 100
        $g.FillRectangle((B (C $k[0])), $cx, $cy, 80, 80)
        $g.DrawRectangle((P (C '000000' 160) 2), $cx, $cy, 80, 80)
        $g.DrawString($k[1], $fKey, (B $WHITE), $cx + 96, $cy + 8)
        $g.DrawString("#$($k[0])", $fHex, (B $DIM), $cx + 96, $cy + 44)
    }
    $ny = $ky + 3 * 100 + 16
    $notes = @(
        'Orange rings: crit spots (head and abdomen, the existing hit hulls). Yellow: a tell. Cyan: what to watch.',
        'Everything here builds at 1080p Medium without Lumen: meshes, emissive surfaces, soft sprite cards, ribbons and decals; no volumes.',
        'The clay figure is the player at 1.8 m. Spiderlings are the game''s spider at 0.45x; she is 1.8x.')
    foreach ($n in $notes) { $ny = DrawLines $g (Wrap $n $fFoot ($PW - 8)) $fFoot (B $DIM) $kx $ny 32; $ny += 10 }
    $y = [Math]::Max($fxEnd, $ny) + 24
    $g.Dispose()
    # Crop to the used height.
    $final = $bmp.Clone((New-Object System.Drawing.Rectangle 0, 0, $W, ([int]$y)), $bmp.PixelFormat)
    $path = Join-Path $out $j.file
    $final.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    "$path  ${W}x$([int]$y)"
    $final.Dispose(); $bmp.Dispose()
    $heroes[$L] = $j
}

if ($Overview) {
    $OW = 2400; $HW = 1180; $HH = 602; $G2 = 13
    $bmp = New-Object System.Drawing.Bitmap $OW, 2000
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = 'AntiAlias'; $g.InterpolationMode = 'HighQualityBicubic'; $g.TextRenderingHint = 'AntiAliasGridFit'
    $g.Clear($BG)
    $g.DrawString('The Gravemother: four fight concepts', (F 64 $true), (B $ORANGE), $G2 + 4, 16)
    $sub = 'Each sheet shows her, her projectile, her ground charge (tell, run, crack), the FX parts and one fight detail. Pick one, or mix.'
    $y = DrawLines $g (Wrap $sub (F 32) ($OW - 2 * $G2)) (F 32) (B $WHITE) ($G2 + 4) 98 42
    $y += 14
    $order = @('A', 'B', 'C', 'D')
    for ($r = 0; $r -lt 2; $r++) {
        $rowH = 0
        for ($c = 0; $c -lt 2; $c++) {
            $L = $order[$r * 2 + $c]
            $j = Get-Content (Join-Path $src "GMF_$L.json") -Raw -Encoding UTF8 | ConvertFrom-Json
            $x = $G2 + $c * ($HW + $G2)
            DrawImage $g (Join-Path $src $j.shots.hero.file) $x $y $HW $HH
            $g.DrawString("$L   $($j.name)", (F 48 $true), (B $ORANGE), $x, $y + $HH + 8)
            $ey = DrawLines $g (Wrap $j.pitch (F 30) ($HW - 8)) (F 30) (B $TEXT) ($x + 2) ($y + $HH + 70) 38
            $rowH = [Math]::Max($rowH, $ey - $y)
        }
        $y += $rowH + 26
    }
    $g.Dispose()
    $final = $bmp.Clone((New-Object System.Drawing.Rectangle 0, 0, $OW, ([int]$y)), $bmp.PixelFormat)
    $path = Join-Path $out 'Gravemother_Fight_Overview.png'
    $final.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    "$path  ${OW}x$([int]$y)"
    $final.Dispose(); $bmp.Dispose()
}
