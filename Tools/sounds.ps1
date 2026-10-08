# Renders the game's sounds from their recipes (Art\Sounds\recipes), all synthesized in Blender's bundled Python with
# numpy: no recordings, no sample libraries. Writes Art\Sounds\Out\<Cue_With_Underscores>_<NN>.wav, the mix in
# Art\Sounds\cues.json, and the check in Saved\SoundCheck (index.html to listen, a picture and numbers per cue).
# Like artrun.ps1 it waits while Saved\ArtPause.flag exists and runs at below-normal priority, so the editor keeps the
# machine. Several Blenders share the cues (-Jobs).
# Usage:
#   sounds.ps1                          every cue, then the mix and the check
#   sounds.ps1 Weapon.*,UI.Hit*         just these cues (names or patterns), then the mix and the check
#   sounds.ps1 -CheckOnly [cues]        only the mix and the check (pictures for the cues given, or all)
#   sounds.ps1 -NoCheck [cues]          render and mix, no pictures
#   sounds.ps1 -Jobs 4                  how many Blenders render at once (default: cores - 2, at most 8)
#   sounds.ps1 -Zoom 0.12 [cues]        also a picture of each cue's first 0.12 s (<cue>_zoom.png), to see the hits
param([Parameter(Position = 0)][string[]]$Cues = @(), [int]$Jobs = 0, [switch]$CheckOnly, [switch]$NoCheck,
    [double]$Zoom = 0)
# Called with -File, a comma list arrives as one string: split it here too.
$Cues = @($Cues | ForEach-Object { $_ -split ',' } | Where-Object { $_ })
$root = Split-Path $PSScriptRoot -Parent
$flag = Join-Path $root 'Saved\ArtPause.flag'
$waited = 0
while (Test-Path $flag) {
    if ($waited -ge 540) { 'Still paused (Saved\ArtPause.flag) after 9 minutes: run this again later.'; exit 3 }
    if ($waited -eq 0) { 'Paused while the main session measures performance (Saved\ArtPause.flag); waiting...' }
    Start-Sleep -Seconds 15
    $waited += 15
}
(Get-Process -Id $PID).PriorityClass = 'BelowNormal'
Set-Location $root

$blender = $env:BLENDER
if (-not $blender) {
    $blender = Get-ChildItem 'C:\Program Files\Blender Foundation\*\blender.exe' -ErrorAction SilentlyContinue | Sort-Object FullName | Select-Object -Last 1 -ExpandProperty FullName
}
if (-not $blender -or -not (Test-Path $blender)) { "Blender not found. Install it, or set BLENDER to blender.exe."; exit 1 }
$script = Join-Path $root 'Art\Sounds\render.py'
$base = @('-b', '--factory-startup', '--python-exit-code', '2', '--python', $script, '--')
$logs = Join-Path $root 'Saved\SoundCheck\logs'
New-Item -ItemType Directory -Force -Path $logs | Out-Null
# Blender's own start-up chatter is dropped from what's shown.
$noise = '^(Blender \d|Read prefs|Fra:\d|\s*$|.*Warning: unable to|.*color management)'
$failed = $false
$start = Get-Date

if (-not $CheckOnly) {
    if ($Jobs -le 0) { $Jobs = [Math]::Max(1, [Math]::Min(8, [Environment]::ProcessorCount - 2)) }
    $procs = @()
    for ($i = 0; $i -lt $Jobs; $i++) {
        $out = Join-Path $logs "render_$i.log"
        $argList = $base + @('--render', '--shard', "$i/$Jobs") + $Cues
        $p = Start-Process -FilePath $blender -ArgumentList $argList -NoNewWindow -PassThru `
            -RedirectStandardOutput $out -RedirectStandardError "$out.err"
        $null = $p.Handle  # keeps the exit code readable after the process ends
        $procs += $p
    }
    $procs | Wait-Process
    for ($i = 0; $i -lt $Jobs; $i++) {
        $out = Join-Path $logs "render_$i.log"
        Get-Content $out, "$out.err" -ErrorAction SilentlyContinue | Where-Object { $_ -notmatch $noise }
        if ($procs[$i].ExitCode -ne 0) { $failed = $true }
    }
}

$finish = @('--mix')
if (-not $NoCheck) { $finish += '--check' }
if ($Zoom -gt 0) { $finish += @('--zoom', "$Zoom") }
& $blender @($base + $finish + $Cues) 2>&1 | ForEach-Object { "$_" } | Where-Object { $_ -notmatch $noise }
if ($LASTEXITCODE -ne 0) { $failed = $true }
'Done in {0:n0} s.' -f ((Get-Date) - $start).TotalSeconds
if ($failed) { 'Some cues failed: see the errors above.'; exit 1 }
