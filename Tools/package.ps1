# Packages a Windows Development build of the game for playtests. Unreal's BuildCookRun builds the game target (and the
# editor target the cooker needs), cooks the shipped levels (Config\DefaultGame.ini, section
# /Script/UnrealEd.ProjectPackagingSettings: Lvl_TutorialIsland, which is also the menu's map, and Lvl_RansomsRest, plus
# the DirectoriesToAlwaysCook), stages, makes the compressed IoStore containers and archives the game to
# Builds\Playtest\<yyyy-MM-dd_HHmm>\Windows\ (git-ignored; "Play Latest Build.bat" starts the newest). The first run is
# long (every shader compiles for D3D12 SM6 and D3D11 SM5): start it in the background and watch the log.
# The editor must be closed: packaging fights it for files (the build replaces DLLs it holds) and for CPU. Stop PIE first
# (the caller's job) and either run Tools\close.ps1 or pass -CloseEditor. A running game (UnrealEditor.exe -game) stops
# this too, and is never closed for you.
# Usage: package.ps1 [-CloseEditor] [-SmokeTest] [-OutDir <folder>] [-Clean] [-Zip] [-DebugInfo]
#   -CloseEditor  closes the editor first, the same way launch.ps1 does (Tools\close.ps1); stop PIE yourself before this
#   -SmokeTest    after packaging, starts the packaged game windowed 1280x720 twice (the menu, then Lvl_RansomsRest named on the
#                 command line), checks its log for the map loads and for crashes, and closes it again. Pass or fail is printed
#   -OutDir       the archive folder (default Builds\Playtest\<yyyy-MM-dd_HHmm>); it must not hold a Windows folder yet
#   -Clean        wipes the build products and the cooked data first: a full rebuild and recook
#   -Zip          also zips the game next to it, to send to someone (playtests on this PC don't need it)
#   -NoZip        accepted and ignored (older notes use it; the zip is opt-in now)
#   -DebugInfo    keeps the .pdb files in the package (a lot bigger; the build keeps its own, which is enough to read a crash)
# Prints the elapsed time and the archive's size. The whole UAT log is Saved\Logs\package_<stamp>.log (smoke runs write
# package_<stamp>_smoke_*.log beside it); a failure prints the first errors from it. The cook's own log is
# Saved\Logs\AI_Looter_Shooter.log. Exit codes: 0 ok, 1 refused to start, 2 packaging failed, 3 smoke test failed.
param([string]$OutDir = '', [switch]$CloseEditor, [switch]$SmokeTest, [switch]$Clean, [switch]$Zip, [switch]$NoZip, [switch]$DebugInfo)

$root = Split-Path $PSScriptRoot -Parent
$engine = "C:\Program Files\Epic Games\UE_5.8"
$uat = "$engine\Engine\Build\BatchFiles\RunUAT.bat"
$project = "$root\AI_Looter_Shooter.uproject"
$stamp = Get-Date -Format 'yyyy-MM-dd_HHmm'
$logPath = Join-Path $root "Saved\Logs\package_$stamp.log"
$clock = [Diagnostics.Stopwatch]::StartNew()

if (-not (Test-Path $uat)) { "RunUAT.bat was not found: $uat"; exit 1 }

# The editor, a game someone may be playing, or a leftover cook would all hold the files the build and the cook write.
# Get-Process finds them everywhere; the CIM query (for the command line, to tell a game from the editor) can come back
# empty without an error in a restricted shell (2026-10-22: the editor was missed and the build hit Live Coding), so it
# only adds detail: a process it can't see is treated as the editor.
function Get-UnrealProcesses {
    $found = @(Get-Process -Name 'UnrealEditor', 'UnrealEditor-Cmd' -ErrorAction SilentlyContinue)
    $details = @(Get-CimInstance Win32_Process -Filter "Name = 'UnrealEditor.exe' OR Name = 'UnrealEditor-Cmd.exe'" -ErrorAction SilentlyContinue)
    @($found | ForEach-Object {
        $id = $_.Id
        $cim = $details | Where-Object { $_.ProcessId -eq $id } | Select-Object -First 1
        [pscustomobject]@{ Name = "$($_.ProcessName).exe"; ProcessId = $id; CommandLine = $(if ($cim) { $cim.CommandLine } else { '' }) }
    })
}
$running = @(Get-UnrealProcesses)
if ($running.Count -gt 0) {
    $playing = @($running | Where-Object { $_.Name -eq 'UnrealEditor.exe' -and $_.CommandLine -match '(^|\s)-game(\s|$)' })
    $cooking = @($running | Where-Object { $_.Name -eq 'UnrealEditor-Cmd.exe' })
    if ($playing.Count -gt 0) {
        "The game is running (UnrealEditor.exe -game, process $($playing[0].ProcessId)); someone may be playing. Close it yourself: this script never does."
        exit 1
    }
    if ($cooking.Count -gt 0) {
        "UnrealEditor-Cmd.exe is still running (a cook or a test run, process $($cooking[0].ProcessId)). Let it finish or stop it, then run this again."
        exit 1
    }
    if (-not $CloseEditor) {
        "The Unreal Editor is open: packaging fights it for files and CPU. Stop PIE and close it (Tools\close.ps1), or pass -CloseEditor."
        exit 1
    }
    # close.ps1 stops PIE through MCP, then asks the editor to quit and waits up to 90 s (a save dialog blocks it).
    $closed = & "$PSScriptRoot\close.ps1"
    $closed
    if ($closed -match 'DID NOT CLOSE') { exit 1 }
    for ($i = 0; $i -lt 30 -and @(Get-UnrealProcesses).Count -gt 0; $i++) { Start-Sleep 1 }
    if (@(Get-UnrealProcesses).Count -gt 0) { "The editor is still running after close.ps1. Close it yourself, then run this again."; exit 1 }
}

# A package costs several gigabytes: the intermediate files, the cooked data, the stage and the archive (and the zip).
$drive = Get-PSDrive -Name $root.Substring(0, 1) -ErrorAction SilentlyContinue
if ($drive -and $drive.Free -lt 15GB) {
    "Warning: only {0:N1} GB is free on {1}: (packaging needs about 15 GB)." -f ($drive.Free / 1GB), $root.Substring(0, 1)
}

if ($OutDir) { $archive = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutDir) }
else { $archive = Join-Path $root "Builds\Playtest\$stamp" }
if (Test-Path (Join-Path $archive 'Windows')) { "$archive already holds a Windows folder from an earlier package. Use another -OutDir (or wait a minute: folders are named by the minute)."; exit 1 }
New-Item -ItemType Directory -Force -Path $archive | Out-Null
New-Item -ItemType Directory -Force -Path (Split-Path $logPath) | Out-Null

# The same choices as Config\DefaultGame.ini's packaging section, because UAT only reads some of them from the ini: the
# editor's Package Project translates the rest into these arguments. The maps to cook are not repeated here: UAT takes
# MapsToCook from that section.
$uatArgs = @(
    'BuildCookRun', "-project=$project", '-nop4', '-unattended', '-utf8output',
    '-platform=Win64', '-target=AI_Looter_Shooter', '-clientconfig=Development',
    '-build', '-cook', '-stage', '-pak', '-iostore', '-compressed', '-prereqs', '-package', '-archive', "-archivedirectory=$archive"
)
# No -SkipCookingEditorContent: the engine's Landmass plugin loads materials at startup that refer to /Engine/EditorMaterials,
# and that flag marks them never-cook, which fails the cook (the first test build, 2026-10-08).
if (-not $DebugInfo) { $uatArgs += '-nodebuginfo' }
if ($Clean) { $uatArgs += '-clean' }

"Packaging to $archive$(if ($Clean) { ' (clean)' }). Log: $logPath"
$bannerPattern = '\*{5,} .*COMMAND (STARTED|COMPLETED) \*{5,}|BuildCookRun time|BUILD (SUCCESSFUL|FAILED)|AutomationTool exiting'
$progressPattern = 'Cooked packages \d+'
$lastProgress = [DateTime]::MinValue
$writer = New-Object IO.StreamWriter($logPath, $false, (New-Object Text.UTF8Encoding($false)))
$writer.AutoFlush = $true
$writer.WriteLine("> RunUAT.bat $($uatArgs -join ' ')")
$oldEncoding = [Console]::OutputEncoding
try { [Console]::OutputEncoding = [Text.Encoding]::UTF8 } catch {}
try {
    # stderr is folded into the pipeline: UAT sends a few messages there, and each line stays a line in the log.
    & $uat @uatArgs 2>&1 | ForEach-Object {
        $line = "$_"
        $writer.WriteLine($line)
        if ($line -match $bannerPattern) {
            "[{0}] {1}" -f $clock.Elapsed.ToString('hh\:mm\:ss'), ($line -replace '^[\s\*]+|[\s\*]+$', '')
        } elseif ($line -match $progressPattern -and ([DateTime]::Now - $lastProgress).TotalSeconds -gt 120) {
            $lastProgress = [DateTime]::Now
            "[{0}] {1}" -f $clock.Elapsed.ToString('hh\:mm\:ss'), ($line -replace '^.*(Cooked packages)', '$1')
        }
    }
    $exitCode = $LASTEXITCODE
} finally {
    $writer.Dispose()
    try { [Console]::OutputEncoding = $oldEncoding } catch {}
}

$stage = Join-Path $archive 'Windows'
$exe = if (Test-Path $stage) { Get-ChildItem -LiteralPath $stage -Filter 'AI_Looter_Shooter.exe' -File -ErrorAction SilentlyContinue | Select-Object -First 1 } else { $null }
if ($exitCode -ne 0 -or -not $exe) {
    "PACKAGING FAILED (exit code $exitCode$(if ($exitCode -eq 0) { ', but no AI_Looter_Shooter.exe in the archive' })) after $($clock.Elapsed.ToString('hh\:mm\:ss')). First errors in ${logPath}:"
    $errorPattern = '(?i)(: error |error C\d+|error LNK|\bERROR:|\bError: |LogCook: Error|unhandled exception|fatal error|\bFailed to |ExitCode=[1-9])'
    Select-String -Path $logPath -Pattern $errorPattern | Where-Object { $_.Line -notmatch '(?i)\b0 errors?\b|errors?: 0\b' } |
        Select-Object -First 25 | ForEach-Object { '  ' + $_.Line.Trim().Substring(0, [Math]::Min(220, $_.Line.Trim().Length)) }
    exit 2
}
"Packaged: $stage"

if ($Zip) {
    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $top = "AI_Looter_Shooter_Test_$stamp"
    $zipPath = Join-Path $archive "$top.zip"
    "Zipping to $zipPath ..."
    $stream = [IO.File]::Open($zipPath, [IO.FileMode]::Create)
    $zipFile = New-Object IO.Compression.ZipArchive($stream, [IO.Compression.ZipArchiveMode]::Create)
    try {
        $base = (Resolve-Path -LiteralPath $stage).Path.TrimEnd('\') + '\'
        foreach ($file in Get-ChildItem -LiteralPath $stage -Recurse -File -Force) {
            $entry = $top + '/' + $file.FullName.Substring($base.Length).Replace('\', '/')
            # The IoStore containers are compressed already: storing them saves minutes and loses nothing.
            $level = if ($file.Extension -eq '.ucas') { [IO.Compression.CompressionLevel]::NoCompression } else { [IO.Compression.CompressionLevel]::Optimal }
            [void][IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zipFile, $file.FullName, $entry, $level)
        }
    } finally {
        $zipFile.Dispose()
        $stream.Dispose()
    }
    "Zip: {0} ({1:N0} MB)" -f $zipPath, ((Get-Item -LiteralPath $zipPath).Length / 1MB)
}

$sizeBytes = (Get-ChildItem -LiteralPath $archive -Recurse -File -Force -ErrorAction SilentlyContinue | Measure-Object Length -Sum).Sum
"Archive: {0} ({1:N2} GB)" -f $archive, ($sizeBytes / 1GB)

# Smoke test: the real game exe (the root exe is a stub that starts it and exits, so its process couldn't be watched). Each
# run is judged by its own log (-abslog), which the engine keeps open: read it with sharing allowed.
function Read-SharedText([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path)) { return '' }
    try {
        $fs = [IO.File]::Open($Path, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete)
        try { return (New-Object IO.StreamReader($fs)).ReadToEnd() } finally { $fs.Dispose() }
    } catch { return '' }
}
function Invoke-SmokeRun([string]$Name, [string]$GameExe, [string]$MapArg, [string]$LoadedPattern, [int]$TimeoutSec) {
    $log = Join-Path $root "Saved\Logs\package_${stamp}_smoke_$Name.log"
    Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
    $gameArgs = "$MapArg -windowed -ResX=1280 -ResY=720 -nosplash -log -abslog=`"$log`"".Trim()
    $proc = Start-Process $GameExe -ArgumentList $gameArgs -PassThru
    $fatalPattern = '(?i)Fatal error|Assertion failed|Unhandled Exception|appError called'
    $verdict = $null
    $deadline = [DateTime]::Now.AddSeconds($TimeoutSec)
    while ($null -eq $verdict -and [DateTime]::Now -lt $deadline) {
        Start-Sleep 2
        $text = Read-SharedText $log
        if ($text -match $fatalPattern) { $verdict = "crash: $($Matches[0])" }
        elseif ($text -match $LoadedPattern) {
            # Stay up a few seconds past the load: a crash in the first frames of play counts.
            Start-Sleep 6
            $text = Read-SharedText $log
            if ($proc.HasExited) { $verdict = "the game exited right after the map loaded (exit code $($proc.ExitCode))" }
            elseif ($text -match $fatalPattern) { $verdict = "crash: $($Matches[0])" }
            else { $verdict = 'PASS' }
        }
        elseif ($proc.HasExited) { $verdict = "the game exited before the map loaded (exit code $($proc.ExitCode))" }
    }
    if ($null -eq $verdict) { $verdict = "no map load in the log after $TimeoutSec s" }
    if (-not $proc.HasExited) {
        [void]$proc.CloseMainWindow()
        if (-not $proc.WaitForExit(10000)) { $proc.Kill(); $proc.WaitForExit(5000) | Out-Null }
    }
    $ok = ($verdict -eq 'PASS')
    # Write-Host, not output: the function's output is its true/false answer.
    Write-Host ("Smoke {0}: {1}{2} (log: {3})" -f $Name, $(if ($ok) { 'PASS' } else { 'FAIL' }), $(if ($ok) { '' } else { " - $verdict" }), $log)
    if (-not $ok) {
        (Read-SharedText $log) -split "`r?`n" | Select-Object -Last 15 | ForEach-Object { Write-Host ('  ' + $_) }
    }
    return $ok
}

if ($SmokeTest) {
    $gameExe = Get-ChildItem -LiteralPath $stage -Recurse -Filter 'AI_Looter_Shooter.exe' -File -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match '\\Binaries\\Win64\\' } | Select-Object -First 1
    if (-not $gameExe) { "Smoke test: no Binaries\Win64\AI_Looter_Shooter.exe under $stage"; exit 3 }
    # A map named on the command line plays that level straight away (no menu, no session), as perf.ps1 does with -game.
    # No map: the game opens the default map with ?game=Menu (DefaultEngine.ini), the main menu over the tutorial island.
    $menuOk = Invoke-SmokeRun 'menu' $gameExe.FullName '' 'LoadMap\(/Game/Maps/Lvl_TutorialIsland' 300
    $rrOk = Invoke-SmokeRun 'ransomsrest' $gameExe.FullName '/Game/Maps/Lvl_RansomsRest' 'LoadMap\(/Game/Maps/Lvl_RansomsRest' 300
    if (-not ($menuOk -and $rrOk)) { "SMOKE TEST FAILED after $($clock.Elapsed.ToString('hh\:mm\:ss'))."; exit 3 }
    "Smoke test passed."
}
"Took {0}. Run it with {1}" -f $clock.Elapsed.ToString('hh\:mm\:ss'), $exe.FullName
exit 0
