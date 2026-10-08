# Copies an export folder's chosen models (and the materials they use) into <folder>_Import for Looter.ImportModels,
# so an import brings in only those: stage_import.ps1 <export folder> <model,model,...> (names: PowerShell variables ignore case, so no $Models)
param([string]$Export, [string]$Names)
$keep = $Names.Split(',') | ForEach-Object { $_.Trim() } | Where-Object { $_ }
$dst = "${Export}_Import"
if (Test-Path $dst) { Get-ChildItem -LiteralPath $dst -File | Where-Object { $_.Extension -in '.fbx', '.json' } | Remove-Item }
else { New-Item -ItemType Directory $dst | Out-Null }
$found = @()
foreach ($json in Get-ChildItem -LiteralPath $Export -Filter *.json) {
    $m = Get-Content -Raw $json.FullName | ConvertFrom-Json
    $models = @($m.models | Where-Object { $keep -contains $_.name })
    if (-not $models) { continue }
    $used = @{}
    foreach ($mod in $models) { foreach ($mat in $mod.materials) { $used[$mat] = $true } }
    $m.models = $models
    if ($m.materials -is [System.Management.Automation.PSCustomObject]) {
        $mats = [ordered]@{}
        foreach ($p in $m.materials.PSObject.Properties) { if ($used.ContainsKey($p.Name)) { $mats[$p.Name] = $p.Value } }
        $m.materials = [pscustomobject]$mats
    }
    [IO.File]::WriteAllText((Join-Path $dst $json.Name), ($m | ConvertTo-Json -Depth 12), (New-Object System.Text.UTF8Encoding $false))
    foreach ($mod in $models) { Copy-Item (Join-Path $Export $mod.fbx) $dst; $found += $mod.name }
}
$missing = $keep | Where-Object { $found -notcontains $_ }
"staged {0} of {1} into {2}{3}" -f $found.Count, $keep.Count, $dst, $(if ($missing) { '; MISSING ' + ($missing -join ', ') } else { '' })
