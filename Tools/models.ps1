# Exports Blender models to FBX with the project's fixed settings and imports them into /Game/Art/<Category>.
# Sources live in Art\Models\<Category>\: hand-made .blend files and scripted .py models (a script builds its model in an
# empty scene with Tools\Blender\looter_model.py). Exports go to Intermediate\ArtExport. The import runs in the open
# editor (Looter.ImportModels); with the editor closed, run that command after opening it.
# Usage: models.ps1 [-Only LanternPost,Crate] [-Source <file or folder>] [-Out <folder>] [-NoImport]
param([string[]]$Only = @(), [string]$Source = '', [string]$Out = '', [switch]$NoImport)
# Called with -File, a comma list arrives as one string: split it here too.
$Only = @($Only | ForEach-Object { $_ -split ',' } | Where-Object { $_ })
$root = Split-Path $PSScriptRoot -Parent
$blender = $env:BLENDER
if (-not $blender) {
    $blender = Get-ChildItem 'C:\Program Files\Blender Foundation\*\blender.exe' -ErrorAction SilentlyContinue | Sort-Object FullName | Select-Object -Last 1 -ExpandProperty FullName
}
if (-not $blender -or -not (Test-Path $blender)) { "Blender not found. Install it, or set BLENDER to blender.exe."; exit 1 }

$sourceRoot = if ($Source) { (Resolve-Path $Source).Path } else { Join-Path $root 'Art\Models' }
$sources = @(Get-ChildItem $sourceRoot -Recurse -File -Include *.blend, *.py | Where-Object { -not $Only -or $Only -contains $_.BaseName })
if (-not $sources) { "No models found under $sourceRoot."; exit 1 }

# The export folder is cleared of the last run's files (so only this run's models get imported): only its own .fbx and
# .json files, never its subfolders. A bad -Out once made this clear every such file in the project, so the folder is
# checked first and anything odd stops the script.
$exportDir = if (-not $Out) { Join-Path $root 'Intermediate\ArtExport' } elseif ([IO.Path]::IsPathRooted($Out)) { $Out } else { Join-Path (Get-Location).Path $Out }
$exportDir = [IO.Path]::GetFullPath($exportDir).TrimEnd('\')
if ($exportDir -eq $root -or $root.StartsWith("$exportDir\") -or $exportDir.Length -le 3) { "Won't export into ${exportDir}: give the export a folder of its own."; exit 1 }
New-Item -ItemType Directory -Force -Path $exportDir -ErrorAction Stop | Out-Null
Get-ChildItem -LiteralPath $exportDir -File -ErrorAction Stop | Where-Object { $_.Extension -in '.fbx', '.json' } | Remove-Item -ErrorAction Stop

$exporter = Join-Path $PSScriptRoot 'Blender\looter_export.py'
$helpers = (Join-Path $PSScriptRoot 'Blender').Replace('\', '/')
$failed = 0
foreach ($file in $sources) {
    $category = Split-Path (Split-Path $file.FullName -Parent) -Leaf
    $relative = $(if ($file.FullName.StartsWith("$root\")) { $file.FullName.Substring($root.Length + 1) } else { $file.FullName }).Replace('\', '/')
    $exportArgs = @('--python', $exporter, '--', '--out', $exportDir, '--category', $category, '--source', $relative)
    $setup = "import sys; sys.path.append('$helpers')"
    if ($file.Extension -eq '.blend') {
        $blenderArgs = @('-b', $file.FullName, '--python-exit-code', '2', '--python-expr', $setup) + $exportArgs
    } else {
        # A scripted model starts from an empty scene.
        $setup += "; import bpy; bpy.ops.wm.read_factory_settings(use_empty=True)"
        $blenderArgs = @('-b', '--factory-startup', '--python-exit-code', '2', '--python-expr', $setup, '--python', $file.FullName) + $exportArgs
    }
    $output = & $blender @blenderArgs 2>&1 | ForEach-Object { "$_" }
    $output | Where-Object { $_ -match '^LOOTER: |Error|Traceback|^\s+File "' } | ForEach-Object { "  $_" }
    if ($LASTEXITCODE -ne 0 -or -not ($output -match '^LOOTER: exported')) { "FAILED: $relative (Blender exit code $LASTEXITCODE)"; $failed++ }
}
if ($failed) { "$failed of $($sources.Count) sources failed; nothing imported."; exit 1 }

if ($NoImport) { "Exported to $exportDir. Import with Looter.ImportModels in the editor."; exit 0 }
if (-not (Get-Process UnrealEditor -ErrorAction SilentlyContinue)) { "Exported to $exportDir. Open the editor and run Looter.ImportModels."; exit 0 }
# From this run's folder: with -Out, the default folder still holds some earlier run's models.
& "$PSScriptRoot\console.ps1" "Looter.ImportModels $($exportDir.Replace('\', '/'))" -Until 'Looter\.ImportModels: imported' |
    Where-Object { $_ -match 'LogModelImporter|LogLooterEditor|LogFbx.*(Warning|Error)|Error' }
