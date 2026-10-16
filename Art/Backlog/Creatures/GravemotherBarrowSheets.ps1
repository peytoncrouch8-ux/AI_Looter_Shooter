# Assembles the Gravemother "Barrow" sheets from GravemotherBarrowSheet.py's renders, painted textures and BW.json (run
# after it): Saved/Concepts/Gravemother/Barrow_Moves.png, Barrow_FX.png, Barrow_Hero.png, Barrow_Spiderling.png.
#   Art/Backlog/Creatures/GravemotherBarrowSheets.ps1 [-Only Moves,FX,Hero,Spiderling]
param([string[]]$Only = @('Moves', 'FX', 'Hero', 'Spiderling'))
Add-Type -AssemblyName System.Drawing
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$src = Join-Path $root 'Saved\ArtPreviews\RansomsRest\Concepts\GravemotherBarrow'
$out = Join-Path $root 'Saved\Concepts\Gravemother'
New-Item -ItemType Directory -Force $out | Out-Null
$j = Get-Content (Join-Path $src 'BW.json') -Raw -Encoding UTF8 | ConvertFrom-Json

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
$WHITE = C 'F2EEE6'; $TEXT = C 'DAD4C9'; $DIM = C 'A9A297'; $FRAME = C '2C3038'; $PANEL = C '0B0C0F' 215
$KIND = @{ 'crit' = $ORANGE; 'tell' = $YELLOW; 'shoot' = $CYAN; 'cover' = $CYAN }
$TYPE = @{ 'MESH' = $WHITE; 'CARD' = $ORANGE; 'DECAL' = $CYAN; 'EMISSIVE' = $YELLOW; 'NOTES' = $DIM }
$W = 2400; $M = 24; $GAP = 16; $CW = 772

$measure = [System.Drawing.Graphics]::FromImage((New-Object System.Drawing.Bitmap 8, 8))
function TextW([string]$t, $font) { $measure.MeasureString($t, $font).Width }
function Wrap([string]$text, $font, [float]$width) {
    $lines = New-Object System.Collections.Generic.List[string]
    $cur = ''
    foreach ($w in ($text -split ' ')) {
        $try = if ($cur) { "$cur $w" } else { $w }
        if ($cur -and (TextW $try $font) -gt $width) { $lines.Add($cur); $cur = $w } else { $cur = $try }
    }
    if ($cur) { $lines.Add($cur) }
    , $lines
}
function DrawLines($g, $lines, $font, $brush, [float]$x, [float]$y, [float]$lh) {
    foreach ($l in $lines) { $g.DrawString($l, $font, $brush, $x, $y); $y += $lh }
    $y
}
function NewSheet([int]$h) {
    $bmp = New-Object System.Drawing.Bitmap $W, $h
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = 'AntiAlias'; $g.InterpolationMode = 'HighQualityBicubic'; $g.TextRenderingHint = 'AntiAliasGridFit'
    $g.Clear($BG)
    $bmp, $g
}
function SaveSheet($bmp, $g, [float]$y, [string]$name) {
    $g.Dispose()
    $final = $bmp.Clone((New-Object System.Drawing.Rectangle 0, 0, $W, ([int]$y)), $bmp.PixelFormat)
    $path = Join-Path $out $name
    $final.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    "$path  ${W}x$([int]$y)"
    $final.Dispose(); $bmp.Dispose()
}
function DrawImage($g, [string]$file, [float]$x, [float]$y, [float]$w, [float]$h, [bool]$border = $true) {
    $img = [System.Drawing.Image]::FromFile($file)
    $g.DrawImage($img, $x, $y, $w, $h)
    $img.Dispose()
    if ($border) { $g.DrawRectangle((P $FRAME 2), $x - 1, $y - 1, $w + 1, $h + 1) }
}
function Header($g, [string]$title, [string]$sub) {
    $g.DrawString($title, (F 62 $true), (B $ORANGE), $M - 4, 16)
    $tag = 'The Gravemother  |  concept D, Barrow'
    $g.DrawString($tag, (F 28), (B $DIM), $W - $M - (TextW $tag (F 28)), 44)
    DrawLines $g (Wrap $sub (F 32) ($W - 2 * $M)) (F 32) (B $WHITE) $M 96 42
}
function Legend($g, [float]$y) {
    $x = $M
    foreach ($item in @(@($ORANGE, 'Crit spot'), @($YELLOW, 'Tell: watch for it'), @($CYAN, 'Do this: shoot it, take cover'))) {
        $g.DrawEllipse((P $item[0] 4), $x, $y + 4, 30, 30)
        $g.FillEllipse((B $item[0]), $x + 11, $y + 15, 8, 8)
        $g.DrawString($item[1], (F 30 $true), (B $WHITE), $x + 42, $y)
        $x += 42 + (TextW $item[1] (F 30 $true)) + 50
    }
    $y + 48
}

# Rings on the callout points, each with a short label in a dark box beside it (none when the label is empty).
function Marks($g, [float]$ox, [float]$oy, [float]$w, [float]$h, $entry, $font, [float]$boxH) {
    $s = $w / [float]$entry.w
    $boxes = @()
    foreach ($m in $entry.marks) {
        if ($m.depth -le 0) { continue }
        $px = $ox + $m.x * $s; $py = $oy + $m.y * $s
        if ($px -lt $ox -or $px -gt $ox + $w -or $py -lt $oy -or $py -gt $oy + $h) { continue }
        $col = $KIND[$m.kind]
        $shadow = P (C '000000' 150) 6
        $g.DrawEllipse($shadow, $px - 16, $py - 16, 32, 32)
        $g.DrawEllipse((P $col 4), $px - 16, $py - 16, 32, 32)
        $g.FillEllipse((B $col), $px - 4, $py - 4, 8, 8)
        if (-not $m.label) { continue }
        $bw = (TextW $m.label $font) + 24
        # Try right-above, left-above, right-below, left-below: the first inside the frame that covers no other box and
        # no other ring.
        $rings = @($entry.marks | Where-Object { $_ -ne $m } | ForEach-Object { , @(($ox + $_.x * $s), ($oy + $_.y * $s)) })
        $cands = @(@(($px + 34), ($py - $boxH - 26)), @(($px - 34 - $bw), ($py - $boxH - 26)),
                   @(($px + 34), ($py + 30)), @(($px - 34 - $bw), ($py + 30)))
        $pick = $null
        foreach ($cnd in $cands) {
            $cx0 = $cnd[0]; $cy0 = $cnd[1]
            if ($cx0 -lt $ox + 8 -or $cx0 + $bw -gt $ox + $w - 8 -or $cy0 -lt $oy + 8 -or $cy0 + $boxH -gt $oy + $h - 8) { continue }
            $bad = $false
            foreach ($o in $boxes) { if ($cx0 -lt $o[0] + $o[2] + 6 -and $cx0 + $bw + 6 -gt $o[0] -and $cy0 -lt $o[1] + $o[3] + 6 -and $cy0 + $boxH + 6 -gt $o[1]) { $bad = $true } }
            foreach ($rg in $rings) { if ($rg[0] -gt $cx0 - 20 -and $rg[0] -lt $cx0 + $bw + 20 -and $rg[1] -gt $cy0 - 20 -and $rg[1] -lt $cy0 + $boxH + 20) { $bad = $true } }
            if (-not $bad) { $pick = $cnd; break }
        }
        if ($pick) { $bx = $pick[0]; $by = $pick[1] } else {
            $bx = $px + 34; $by = $py - $boxH - 26
            if ($bx + $bw -gt $ox + $w - 8) { $bx = $px - 34 - $bw }
            if ($bx -lt $ox + 8) { $bx = [Math]::Max($ox + 8, [Math]::Min($px - $bw / 2, $ox + $w - 8 - $bw)) }
            if ($by -lt $oy + 8) { $by = $py + 30 }
        }
        for ($pass = 0; $pass -lt 6; $pass++) {
            $moved = $false
            foreach ($o in $boxes) {
                if ($bx -lt $o[0] + $o[2] -and $bx + $bw -gt $o[0] -and $by -lt $o[1] + $o[3] + 6 -and $by + $boxH + 6 -gt $o[1]) {
                    $by = $o[1] + $o[3] + 8; $moved = $true
                }
            }
            if ($by + $boxH -gt $oy + $h - 8) {
                # No room below: go above every box it meets instead.
                $by = $oy + $h - $boxH - 8
                foreach ($o in $boxes) {
                    if ($bx -lt $o[0] + $o[2] -and $bx + $bw -gt $o[0] -and $by -lt $o[1] + $o[3] + 6 -and $by + $boxH + 6 -gt $o[1]) {
                        $by = $o[1] - $boxH - 8
                    }
                }
            }
            if (-not $moved) { break }
        }
        $boxes += , @($bx, $by, $bw, $boxH)
        $ex = if ($bx + $bw / 2 -lt $px) { $bx + $bw } else { $bx }
        $ey = $by + $boxH / 2
        $g.DrawLine($shadow, $px, $py, $ex, $ey)
        $g.DrawLine((P $col 3), $px, $py, $ex, $ey)
        $g.FillRectangle((B $PANEL), $bx, $by, $bw, $boxH)
        $g.FillRectangle((B $col), $bx, $by, 5, $boxH)
        $g.DrawString($m.label, $font, (B $col), $bx + 13, $by + ($boxH - $font.Size * 1.25) / 2)
    }
}
function Tag($g, [float]$x, [float]$y, [string]$big, [string]$small, $col) {
    $fb = F 34 $true; $fs = F 26 $true
    $w = (TextW $big $fb) + 20
    if ($small) { $w += (TextW $small $fs) + 6 }
    $g.FillRectangle((B $PANEL), $x + 8, $y + 8, $w, 48)
    $g.FillRectangle((B $col), $x + 8, $y + 8, 5, 48)
    $tx = $x + 20
    if ($small) { $g.DrawString($small, $fs, (B $DIM), $tx, $y + 18); $tx += (TextW $small $fs) + 4 }
    $g.DrawString($big, $fb, (B $col), $tx, $y + 12)
}
function Seconds([double]$t) { if ($t -eq [Math]::Floor($t)) { '{0:0.0} s' -f $t } else { '{0:0.0#} s' -f $t } }

# --- Barrow_Moves ---
if ('Moves' -in $Only) {
    $FH = 420
    $bmp, $g = NewSheet 9000
    $y = Header $g 'Barrow: her moves in time' ("Every move as six frames from one fixed camera, times in seconds from the move's start. " +
        "The clay figure is the player (1.8 m); she is the game's spider at 1.8x; the spiderlings are the same spider at 0.45x.")
    $y = Legend $g ($y + 6)
    $fCap = F 34; $fNum = F 34 $true
    foreach ($key in $j.order) {
        $s = $j.moves.$key
        if (-not $s) { continue }
        $y += 16
        $g.FillRectangle((B $ORANGE), $M, $y + 8, 8, 40)
        $g.DrawString($s.title, (F 46 $true), (B $WHITE), $M + 20, $y)
        $y += 62
        $y = DrawLines $g (Wrap $s.summary (F 33) ($W - 2 * $M)) (F 33) (B $TEXT) $M $y 43
        $y += 12
        for ($r = 0; $r -lt 2; $r++) {
            $capH = 0
            for ($c = 0; $c -lt 3; $c++) {
                $i = $r * 3 + $c; $fr = $s.frames[$i]; $x = $M + $c * ($CW + $GAP)
                DrawImage $g (Join-Path $src $fr.file) $x $y $CW $FH
                Marks $g $x $y $CW $FH $fr (F 25 $true) 38
                Tag $g $x $y (Seconds $fr.t) ("$($i + 1)") $WHITE
                $lines = Wrap $fr.caption $fCap ($CW - 8)
                $capH = [Math]::Max($capH, $lines.Count * 42)
                [void](DrawLines $g $lines $fCap (B $TEXT) ($x + 4) ($y + $FH + 8) 42)
            }
            $y += $FH + 8 + $capH + 16
        }
        $g.DrawLine((P $FRAME 2), $M, $y + 6, $W - $M, $y + 6)
        $y += 10
    }
    SaveSheet $bmp $g ($y + 14) 'Barrow_Moves.png'
}

# --- Barrow_FX ---
function ScaleBar($g, [float]$x, [float]$y, [float]$pxPerM) {
    $want = 170.0 / $pxPerM * 100.0
    $cm = (@(5, 10, 20, 25, 50, 100, 200, 500) | Sort-Object { [Math]::Abs($_ - $want) })[0]
    $len = $cm / 100.0 * $pxPerM
    $pen = P $WHITE 3
    $g.DrawLine($pen, $x, $y, $x + $len, $y)
    $g.DrawLine($pen, $x, $y - 8, $x, $y + 8); $g.DrawLine($pen, $x + $len, $y - 8, $x + $len, $y + 8)
    $g.DrawString("$cm cm", (F 24 $true), (B $WHITE), $x + $len + 10, $y - 15)
}
function CellHead($g, [float]$x, [float]$y, [float]$w, [string]$title, [string]$kind) {
    $g.DrawString($title, (F 31 $true), (B $WHITE), $x, $y)
    $kw = (TextW $kind (F 24 $true)) + 18
    $g.FillRectangle((B $PANEL), $x + $w - $kw, $y + 4, $kw, 34)
    $g.DrawString($kind, (F 24 $true), (B $TYPE[$kind]), $x + $w - $kw + 9, $y + 7)
    $y + 46
}
function CellFoot($g, [float]$x, [float]$y, [float]$w, $lines, [string]$note) {
    foreach ($l in $lines) { $y = DrawLines $g (Wrap $l (F 28) $w) (F 28) (B $TEXT) $x $y 35 }
    if ($note) { $y = DrawLines $g (Wrap $note (F 27) $w) (F 27) (B $DIM) $x $y 34 }
    $y
}
function RenderCell($g, $e, [float]$x, [float]$y) {
    $y = CellHead $g $x $y $CW $e.title $e.kind
    $h = 434
    $g.FillRectangle((B (C '000000')), $x, $y, $CW, $h)
    DrawImage $g (Join-Path $src $e.file) $x $y $CW $h
    $iw = 230; $ih = [int]($iw * $h / $CW)
    $ix = $x + $CW - $iw - 8; $iy = $y + 8
    DrawImage $g (Join-Path $src $e.alpha) $ix $iy $iw $ih $false
    $g.DrawRectangle((P (C '6A6F78') 2), $ix, $iy, $iw, $ih)
    $g.DrawString('ALPHA', (F 20 $true), (B $DIM), $ix + 6, $iy + $ih + 2)
    ScaleBar $g ($x + 18) ($y + $h - 26) ([float]$e.px_per_m)
    $lines = @($e.dims | ForEach-Object { "$($_[0]): $($_[1])" })
    CellFoot $g $x ($y + $h + 8) $CW $lines $e.note
}
# A painted texture on black and its alpha below or beside it, scaled to fit a box.
function TexPair($g, $t, [float]$x, [float]$y, [float]$w, [float]$h, [string]$label, [bool]$stack = $true, [string]$under = '000000') {
    $img = [System.Drawing.Image]::FromFile($t.file)
    $s = [Math]::Min($w / $img.Width, $h / $img.Height)
    $dw = $img.Width * $s; $dh = $img.Height * $s
    $img.Dispose()
    $g.FillRectangle((B (C $under)), $x, $y, $dw, $dh)
    DrawImage $g $t.file $x $y $dw $dh
    if ($stack) { $ax = $x; $ay = $y + $dh + 8 } else { $ax = $x + $dw + 8; $ay = $y }
    DrawImage $g $t.alpha $ax $ay $dw $dh
    $g.DrawString($label, (F 22 $true), (B $DIM), $x, $(if ($stack) { $ay + $dh + 2 } else { $y + $dh + 2 }))
    @($dw, $dh)
}
if ('FX' -in $Only) {
    $bmp, $g = NewSheet 7000
    $y = Header $g 'Barrow: the FX pieces' ("Each piece alone on black, its alpha inset (white: opaque). Sizes in cm; the bar is to scale. " +
        "Built from meshes, sprite cards with soft alpha, decals and emissive surfaces only: no volumes, no bloom (the game adds its own). " +
        "Fresh grave earth is darker than the sun-baked floor so the clods, rubble, berms and mounds read against it.")
    $y += 14
    $fx = $j.fx; $tex = $j.textures
    $colX = @($M, ($M + $CW + $GAP), ($M + 2 * ($CW + $GAP)))
    $rows = @(
        @('clods', 'trail', '#decals'),
        @('burst', 'debris', 'rubble'),
        @('#crack', '', 'jets'),
        @('berm', 'mounds', '#puffs'),
        @('chips', 'sparks', 'daze'),
        @('#sprites', '#furrow', ''),
        @('#notes', '', ''))
    foreach ($row in $rows) {
        $rowEnd = $y
        for ($c = 0; $c -lt 3; $c++) {
            $key = $row[$c]; $x = $colX[$c]
            if (-not $key) { continue }
            $end = $y
            switch ($key) {
                '#decals' {
                    $yy = CellHead $g $x $y $CW 'Decals: impact, landing, bullet' 'DECAL'
                    $grey = '5A5650'
                    [void](TexPair $g $tex.ImpactScar $x $yy 210 210 '' $true $grey)
                    [void](TexPair $g $tex.LandShadow ($x + 228) $yy 210 210 '' $true $grey)
                    [void](TexPair $g $tex.StoneScar ($x + 456) ($yy + 52) 300 150 '' $true $grey)
                    $g.DrawString('impact scar', (F 22 $true), (B $DIM), $x, $yy + 432)
                    $g.DrawString('landing shadow', (F 22 $true), (B $DIM), $x + 228, $yy + 432)
                    $g.DrawString('stone scar', (F 22 $true), (B $DIM), $x + 456, $yy + 52 + 316)
                    $end = CellFoot $g $x ($yy + 466) $CW @('Impact scar 512 px: under a burst clod 260 cm, the slam 340 cm, a mound''s crater 230 cm.',
                        'Landing shadow 256 px: grows from 100 to 260 cm under the clod as it comes down.') 'Stone scar 256 x 128 px at 20 x 10 cm, where a shot glanced off the headstone. Decal colour shown on grey (they darken the floor), alpha below each.'
                }
                '#crack' {
                    $w2 = 2 * $CW + $GAP
                    $yy = CellHead $g $x $y $w2 'Heave crack decal: a strip that tiles, and its end cap' 'DECAL'
                    $ppc = 3.4; $tw = 200 * $ppc; $th = 50 * $ppc; $cw2 = 50 * $ppc
                    foreach ($pass in 0, 1) {
                        $py = $yy + $pass * ($th + 34)
                        $g.FillRectangle((B (C '000000')), $x, $py, $w2, $th)
                        $file = if ($pass) { 'alpha' } else { 'file' }
                        for ($k = 0; $k -lt 2; $k++) { DrawImage $g $tex.HeaveCrack.$file ($x + 20 + $k * $tw) $py $tw $th $false }
                        DrawImage $g $tex.HeaveCrackCap.$file ($x + 20 + 2 * $tw) $py $cw2 $th $false
                        foreach ($sx in @(($x + 20 + $tw), ($x + 20 + 2 * $tw))) {
                            $g.DrawLine((P $ORANGE 2), $sx, $py - 6, $sx, $py + $th + 6)
                        }
                        $lab = if ($pass) { 'ALPHA' } else { 'COLOR' }
                        $g.DrawString($lab, (F 20 $true), (B $DIM), $x + 20, $py + $th + 4)
                    }
                    $g.DrawString('seam', (F 22 $true), (B $ORANGE), $x + 26 + $tw, $yy + $th + 4)
                    $g.DrawString('cap joins here', (F 22 $true), (B $ORANGE), $x + 26 + 2 * $tw, $yy + $th + 4)
                    $end = CellFoot $g $x ($yy + 2 * $th + 70) $w2 @('Strip tile 200 x 50 cm (1024 x 256 px), shown twice: it repeats along the heave and the plough line, stretched to whole tiles. End cap 50 x 50 cm (256 px): the strip''s first quarter tapered shut, so its join edge is the strip''s own edge.',
                        'In the fight it lies on the heave''s crest at 70 cm wide (tile 280 cm); dust jets rise from its newest stretch.') ''
                }
                '#puffs' {
                    $yy = CellHead $g $x $y $CW 'Dust card sprite: a 2x2 sheet' 'CARD'
                    $sz = TexPair $g $tex.DustPuffs $x $yy 372 372 '' $false
                    $g.DrawString('color, on black', (F 22 $true), (B $DIM), $x, $yy + 376)
                    $g.DrawString('alpha', (F 22 $true), (B $DIM), $x + 380, $yy + 376)
                    $end = CellFoot $g $x ($yy + 408) $CW @('Four puffs, 256 px each, lit from the top left; each card picks one at random and is tinted.',
                        'Every dust card on these sheets is one of them: bursts, kicks, the slam ring, jets'' bases.') 'Lit translucent (diffuse plus light through it), never additive.'
                }
                '#sprites' {
                    $yy = CellHead $g $x $y $CW 'Sprites: dust jets, hit flash, spark streak' 'CARD'
                    [void](TexPair $g $tex.DustJets $x $yy 150 300 '' $false)
                    [void](TexPair $g $tex.Flash ($x + 330) $yy 190 190 '')
                    [void](TexPair $g $tex.SparkStreak ($x + 540) ($yy + 30) 220 40 '')
                    $g.DrawString('dust jets (2), color | alpha', (F 22 $true), (B $DIM), $x, $yy + 304)
                    $g.DrawString('hit flash', (F 22 $true), (B $DIM), $x + 330, $yy + 392)
                    $g.DrawString('spark streak', (F 22 $true), (B $DIM), $x + 540, $yy + 98)
                    $end = CellFoot $g $x ($yy + 424) $CW @('Jets 128 x 512 px each: upright cards, 34 x 100 to 70 x 210 cm, in earth dust. Flash 256 px, streak 256 x 32 px: emissive over their alpha.') ''
                }
                '#furrow' {
                    $yy = CellHead $g $x $y $CW 'Furrow decal: the ploughed strip' 'DECAL'
                    [void](TexPair $g $tex.Furrow $x $yy 372 186 '' $false '5A5650')
                    $g.DrawString('colour, on grey', (F 22 $true), (B $DIM), $x, $yy + 190)
                    $g.DrawString('alpha', (F 22 $true), (B $DIM), $x + 380, $yy + 190)
                    $end = CellFoot $g $x ($yy + 226) $CW @('Tile 512 x 256 px, 200 x 100 cm: churned dark earth with clods, ragged soft edges; it tiles along the plough line at 190 cm wide, the berms lying along both its edges.') 'It stays as long as the berms do, and fades with them at 20 s.'
                }
                '#notes' {
                    $w2 = 3 * $CW + 2 * $GAP
                    $yy = CellHead $g $x $y $w2 'How it builds (1080p Medium, no Lumen)' 'NOTES'
                    $notes = @(
                        'Meshes: clods (3 sizes), debris (5), rubble pile, berm segment, spawn mound (bulging: its scale animates over the 1 s; burst: swap to the crater), headstone halves and chips, the stun stars. All on the World master through material instances; clods, rubble and mounds are GroundDirt tinted #8D7A66, berms #B3A08A, the heave is the floor''s own material.',
                        'Cards: dust puffs (2x2 sheet), dust jets, the clod''s trail ribbon, the flash and spark streaks. Puffs and jets are lit translucent: earth dust (#806B55) from anything dug up, floor dust (#DCC9A4) from footfalls; sparks, flash and stars are emissive. No volumes, no bloom in these renders: the game adds its own bloom.',
                        'Decals: the heave crack strip and cap (on the heave''s crest and the slam''s cracks), the ploughed furrow, the impact scar, the landing shadow, the stone''s bullet scar; the mound''s cracks are an overlay in its material, mapped from above.',
                        'Timings: clod tell 1.2 s, flight 1.5 s, rubble 10 s; stun 2 s; heave 2 s, berms 20 s; mounds bulge 1 s. Everything here was rendered in Eevee with screen-space light only.')
                    $ny = $yy
                    foreach ($n in $notes) { $ny = DrawLines $g (Wrap $n (F 29) $w2) (F 29) (B $TEXT) $x $ny 37; $ny += 10 }
                    $end = $ny
                }
                default {
                    $e = $fx.$key
                    if ($e) { $end = RenderCell $g $e $x $y }
                }
            }
            $rowEnd = [Math]::Max($rowEnd, $end)
        }
        $y = $rowEnd + 26
    }
    SaveSheet $bmp $g $y 'Barrow_FX.png'
}

# --- Barrow_Hero ---
if ('Hero' -in $Only) {
    $bmp, $g = NewSheet 4200
    $y = Header $g 'Barrow: her turnaround' ("The game's spider at 1.8x in a mud-grey hide, a toppled grey headstone bound to her abdomen by roots (sandstone would vanish against the Sink's wall), " +
        "grave mud caked on her legs. The stone is armour: shots glance off it with sparks and chips. At 33% it cracks off and her pale Bone abdomen is bare.")
    $y = Legend $g ($y + 6)
    $h = $j.hero
    $fm = F 26 $true
    $bigW = 1560; $bigH = 878; $closeW = $W - 2 * $M - $bigW - $GAP
    DrawImage $g (Join-Path $src $h.three.file) $M $y $bigW $bigH
    Marks $g $M $y $bigW $bigH $h.three (F 30 $true) 46
    Tag $g $M $y $h.three.title '' $WHITE
    $cx = $M + $bigW + $GAP
    DrawImage $g (Join-Path $src $h.close.file) $cx $y $closeW $bigH
    Marks $g $cx $y $closeW $bigH $h.close $fm 40
    Tag $g $cx $y $h.close.title '' $WHITE
    $y += $bigH + $GAP
    foreach ($row in @(@('front', 'side', 'back'), @('top', 'bare', '#key'))) {
        for ($c = 0; $c -lt 3; $c++) {
            $key = $row[$c]; $x = $M + $c * ($CW + $GAP)
            if ($key -eq '#key') {
                $g.DrawString('Colour key', (F 34 $true), (B $WHITE), $x, $y)
                $keys = @(@('8C8070', 'Mud-grey hide'), @('C9B392', 'Grave mud'), @('8E8B86', 'Headstone granite'),
                          @('5B4330', 'Roots'), @('BAB2A1', 'Bare Bone abdomen'), @('6E5C49', 'Fresh grave earth'))
                for ($i = 0; $i -lt $keys.Count; $i++) {
                    $kx = $x + ($i % 2) * 386; $ky = $y + 54 + [Math]::Floor($i / 2) * 82
                    $g.FillRectangle((B (C $keys[$i][0])), $kx, $ky, 64, 64)
                    $g.DrawRectangle((P (C '000000' 160) 2), $kx, $ky, 64, 64)
                    $g.DrawString($keys[$i][1], (F 25 $true), (B $WHITE), $kx + 76, $ky + 4)
                    $g.DrawString("#$($keys[$i][0])", (F 22), (B $DIM), $kx + 76, $ky + 36)
                }
                continue
            }
            $e = $h.$key
            DrawImage $g (Join-Path $src $e.file) $x $y $CW 434
            Marks $g $x $y $CW 434 $e (F 25 $true) 38
            Tag $g $x $y $e.title '' $WHITE
        }
        $y += 434 + $GAP
    }
    $y = DrawLines $g (Wrap ('Crits: the head always; the abdomen''s soft rear (under the stone''s back edge) while the stone is on; the whole abdomen after 33%. ' +
        'Everything is Spider.py''s mesh and rig unchanged: the headstone, roots and mud are attached meshes on the abdomen and leg bones.') (F 27) ($W - 2 * $M)) (F 27) (B $DIM) $M ($y + 4) 35
    SaveSheet $bmp $g ($y + 18) 'Barrow_Hero.png'
}

# --- Barrow_Spiderling ---
if ('Spiderling' -in $Only) {
    $bmp, $g = NewSheet 1800
    $l = $j.ling
    $y = Header $g 'Barrow: a spiderling digging out' ("The game's spider at 0.45x ($($l.size)), in its own hide caked in grave mud. " +
        "At 66% and 33% four of them dig out of spawn mounds that bulge for 1 s first (see Unearthed on the moves sheet).")
    $y += 14
    $FH2 = 560
    $capH = 0
    for ($c = 0; $c -lt 3; $c++) {
        $e = $l.shots[$c]; $x = $M + $c * ($CW + $GAP)
        DrawImage $g (Join-Path $src $e.file) $x $y $CW $FH2
        Tag $g $x $y (Seconds $e.t) ("$($c + 1)") $WHITE
        $lines = Wrap $e.caption (F 34) ($CW - 8)
        $capH = [Math]::Max($capH, $lines.Count * 43)
        [void](DrawLines $g $lines (F 34) (B $TEXT) ($x + 4) ($y + $FH2 + 10) 43)
    }
    $y += $FH2 + 10 + $capH + 10
    $y = DrawLines $g (Wrap ('Times from the mound''s first bulge. The mound is a dome of grave earth with a crack overlay; when it bursts it swaps to the crater (a ring of clods round a dark hole decal). The mud is clumps on the abdomen and carapace and crusts on the lower legs, shed as it runs.') (F 27) ($W - 2 * $M)) (F 27) (B $DIM) $M $y 35
    SaveSheet $bmp $g ($y + 18) 'Barrow_Spiderling.png'
}
