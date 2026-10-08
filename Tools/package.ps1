# Packages a Windows Development test build to send to a friend, and zips it. Unreal's BuildCookRun builds the game
# target (and the editor target the cooker needs), cooks the two shipped levels (Config\DefaultGame.ini, section
# /Script/UnrealEd.ProjectPackagingSettings), stages, makes the compressed IoStore containers and archives the game; the
# archived Windows folder is then zipped next to it. The first run is long (every shader compiles for D3D12 SM6 and D3D11
# SM5): start it in the background and watch Saved\Logs\Package.log. The editor must be closed (Tools\close.ps1): the build
# replaces the DLLs it holds. A running game (UnrealEditor.exe -game) stops this too, and is never closed for you.
# Usage: package.ps1 [-OutDir <folder>] [-Clean] [-NoZip] [-DebugInfo]
#   -OutDir     the archive folder (default Saved\Packaging\<yyyyMMdd-HHmmss>); it must not hold a Windows folder yet
#   -Clean      wipes the build products and the cooked data first: a full rebuild and recook
#   -NoZip      stops after the archive
#   -DebugInfo  keeps the .pdb files in the package (a lot bigger; the build keeps its own, which is enough to read a crash)
# Prints the zip's path and size and how long it took. The whole UAT log is Saved\Logs\Package.log; a failure prints the
# first errors from it. The cook's own log is Saved\Logs\AI_Looter_Shooter.log.
param([string]$OutDir = '', [switch]$Clean, [switch]$NoZip, [switch]$DebugInfo)

$root = Split-Path $PSScriptRoot -Parent
$engine = "C:\Program Files\Epic Games\UE_5.8"
$uat = "$engine\Engine\Build\BatchFiles\RunUAT.bat"
$project = "$root\AI_Looter_Shooter.uproject"
$logPath = Join-Path $root 'Saved\Logs\Package.log'
$clock = [Diagnostics.Stopwatch]::StartNew()

if (-not (Test-Path $uat)) { "RunUAT.bat was not found: $uat"; exit 1 }

# The editor, a game someone may be playing, or a leftover cook would all hold the files the build and the cook write.
$running = @(Get-CimInstance Win32_Process -Filter "Name = 'UnrealEditor.exe' OR Name = 'UnrealEditor-Cmd.exe'" -ErrorAction SilentlyContinue)
if ($running.Count -gt 0) {
    $playing = @($running | Where-Object { $_.Name -eq 'UnrealEditor.exe' -and $_.CommandLine -match '(^|\s)-game(\s|$)' })
    if ($playing.Count -gt 0) {
        "The game is running (UnrealEditor.exe -game, process $($playing[0].ProcessId)); someone may be playing. Close it yourself: this script never does."
    } elseif (@($running | Where-Object { $_.Name -eq 'UnrealEditor-Cmd.exe' }).Count -gt 0) {
        "UnrealEditor-Cmd.exe is still running (a cook or a test run, process $(@($running | Where-Object { $_.Name -eq 'UnrealEditor-Cmd.exe' })[0].ProcessId)). Let it finish or stop it, then run this again."
    } else {
        "Close the editor first (Tools\close.ps1)."
    }
    exit 1
}

# A package costs several gigabytes: the intermediate files, the cooked data, the stage, the archive and the zip.
$drive = Get-PSDrive -Name $root.Substring(0, 1) -ErrorAction SilentlyContinue
if ($drive -and $drive.Free -lt 15GB) {
    "Warning: only {0:N1} GB is free on {1}: (packaging needs about 15 GB)." -f ($drive.Free / 1GB), $root.Substring(0, 1)
}

$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
if ($OutDir) { $archive = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutDir) }
else { $archive = Join-Path $root "Saved\Packaging\$stamp" }
if (Test-Path (Join-Path $archive 'Windows')) { "$archive already holds a Windows folder from an earlier package. Use another -OutDir."; exit 1 }
New-Item -ItemType Directory -Force -Path $archive | Out-Null
New-Item -ItemType Directory -Force -Path (Split-Path $logPath) | Out-Null

# The same choices as Config\DefaultGame.ini's packaging section, because UAT only reads some of them from the ini: the
# editor's Package Project translates the rest into these arguments.
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

if (-not $NoZip) {
    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $top = "AI_Looter_Shooter_Test_$stamp"
    $zipPath = Join-Path $archive "$top.zip"
    "Zipping to $zipPath ..."
    $stream = [IO.File]::Open($zipPath, [IO.FileMode]::Create)
    $zip = New-Object IO.Compression.ZipArchive($stream, [IO.Compression.ZipArchiveMode]::Create)
    try {
        $base = (Resolve-Path -LiteralPath $stage).Path.TrimEnd('\') + '\'
        foreach ($file in Get-ChildItem -LiteralPath $stage -Recurse -File -Force) {
            $entry = $top + '/' + $file.FullName.Substring($base.Length).Replace('\', '/')
            # The IoStore containers are compressed already: storing them saves minutes and loses nothing.
            $level = if ($file.Extension -eq '.ucas') { [IO.Compression.CompressionLevel]::NoCompression } else { [IO.Compression.CompressionLevel]::Optimal }
            [void][IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, $file.FullName, $entry, $level)
        }
    } finally {
        $zip.Dispose()
        $stream.Dispose()
    }
    "Zip: {0} ({1:N0} MB)" -f $zipPath, ((Get-Item -LiteralPath $zipPath).Length / 1MB)
}
"Took {0}. Run it with {1}" -f $clock.Elapsed.ToString('hh\:mm\:ss'), $exe.FullName
exit 0
