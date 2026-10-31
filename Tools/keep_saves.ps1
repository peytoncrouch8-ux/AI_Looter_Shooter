# Keeps the player's saves and settings across playtest builds (the user, 2026-10-30: settings must never reset unless the
# player resets them, and saved games must survive a new package).
# A packaged build marked installed (Tools\package.ps1 writes Windows\Engine\Build\InstalledProjectBuild.txt) keeps its
# Saved folder in %LOCALAPPDATA%\AI_Looter_Shooter\Saved. Builds made before that kept it beside the exe, in
# Builds\Playtest\<stamp>\Windows\AI_Looter_Shooter\Saved. This copies the newest such build's SaveGames (sessions,
# graphics, audio, control and key settings, hints) and GameUserSettings.ini into the player's folder, ONCE: only when
# the player's folder has no save yet. It never overwrites or deletes anything.
# Usage: keep_saves.ps1 [-Exclude <build folder>] [-Mark <build folder>]
#   -Exclude  a build folder not to copy from (the one just packaged: it has no saves of its own)
#   -Mark     also marks that older build installed, so it reads and writes the player's folder too
param([string]$Exclude = '', [string]$Mark = '')

$root = Split-Path $PSScriptRoot -Parent
$userSaved = Join-Path $env:LOCALAPPDATA 'AI_Looter_Shooter\Saved'
$userSaves = Join-Path $userSaved 'SaveGames'

if ($Mark) {
    $marker = Join-Path $Mark 'Windows\Engine\Build\InstalledProjectBuild.txt'
    if (Test-Path (Join-Path $Mark 'Windows\AI_Looter_Shooter.exe')) {
        New-Item -ItemType Directory -Force -Path (Split-Path $marker) | Out-Null
        Set-Content -LiteralPath $marker -Encoding ascii -Value 'Marks this build installed (Tools\keep_saves.ps1): saves and settings go to %LOCALAPPDATA%\AI_Looter_Shooter\Saved.'
        "Marked installed: $Mark"
    } else { "Not a build folder (no Windows\AI_Looter_Shooter.exe): $Mark" }
}

if (Test-Path (Join-Path $userSaves '*.sav')) {
    "Saves and settings: kept in $userSaved"
    exit 0
}

# The newest older build with saves of its own (folder names are dates, so by name is by age).
$builds = Join-Path $root 'Builds\Playtest'
$excludeFull = if ($Exclude) { [IO.Path]::GetFullPath($Exclude).TrimEnd('\') } else { '' }
$source = Get-ChildItem -LiteralPath $builds -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending |
    Where-Object { $_.FullName.TrimEnd('\') -ne $excludeFull } |
    Where-Object { Test-Path (Join-Path $_.FullName 'Windows\AI_Looter_Shooter\Saved\SaveGames\*.sav') } | Select-Object -First 1
if (-not $source) {
    "Saves and settings: none yet; the game will keep them in $userSaved"
    exit 0
}

$from = Join-Path $source.FullName 'Windows\AI_Looter_Shooter\Saved'
New-Item -ItemType Directory -Force -Path $userSaves | Out-Null
Copy-Item -Path (Join-Path $from 'SaveGames\*') -Destination $userSaves -Recurse
$ini = Join-Path $from 'Config\Windows\GameUserSettings.ini'
if (Test-Path $ini) {
    $iniTo = Join-Path $userSaved 'Config\Windows'
    New-Item -ItemType Directory -Force -Path $iniTo | Out-Null
    if (-not (Test-Path (Join-Path $iniTo 'GameUserSettings.ini'))) { Copy-Item -LiteralPath $ini -Destination $iniTo }
}
"Saves and settings: copied from build $($source.Name) to $userSaved (once; every build keeps them there from now on)"
exit 0
