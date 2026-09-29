# Runs a console command in the open editor by typing it into the status bar's Cmd box through the Slate inspector (no
# window focus or simulated keys needed), then prints the log lines written while it ran.
# Usage: console.ps1 "stat unit" [-Wait 2]
#        console.ps1 "Looter.ImportModels" -Until 'ImportModels: imported'   (waits for a log line, up to 3 minutes)
param([Parameter(Mandatory)][string]$Command, [double]$Wait = 1.5, [string]$Until = '')
$S = 'SlateInspectorToolset.SlateInspectorToolset'
$root = Split-Path $PSScriptRoot -Parent
$log = Join-Path $root 'Saved\Logs\AI_Looter_Shooter.log'

function Read-LogSince([long]$Offset) {
    $stream = [IO.File]::Open($log, 'Open', 'Read', 'ReadWrite')
    try {
        $stream.Seek($Offset, 'Begin') | Out-Null
        return (New-Object IO.StreamReader($stream)).ReadToEnd()
    } finally { $stream.Dispose() }
}

$snapshot = (& "$PSScriptRoot\mcp.ps1" $S 'Snapshot' '{"ref":"","maxDepth":60,"bIncludeSourceLocations":false}' | Out-String).Replace('\n', "`n").Replace('\"', '"')
$lines = $snapshot -split "`n"
$cmd = ($lines | Select-String -Pattern 'text "Cmd"' | Select-Object -First 1).LineNumber
if (-not $cmd) { "The Cmd box isn't on screen (is the editor's main window open?)"; exit 1 }
$box = $lines[$cmd..($cmd + 4)] | ForEach-Object { [regex]::Match($_, '^\s*textbox .*\[ref=(tb\d+)\]') } | Where-Object Success | Select-Object -First 1
if (-not $box) { "No text box next to the Cmd label"; exit 1 }
$typeArgs = @{ ref = $box.Groups[1].Value; text = $Command; submit = $true } | ConvertTo-Json -Compress

# Typing sometimes fails to focus the box on the first try; the log says whether the command ran.
$start = (Get-Item $log).Length
for ($attempt = 1; $attempt -le 3; $attempt++) {
    $mark = (Get-Item $log).Length
    & "$PSScriptRoot\mcp.ps1" $S 'Type' $typeArgs | Out-Null
    Start-Sleep -Milliseconds 400
    if ((Read-LogSince $mark).Contains("Cmd: $Command")) { break }
    if ($attempt -eq 3) { "The command didn't reach the console."; exit 1 }
}
if ($Until) {
    $deadline = (Get-Date).AddMinutes(3)
    while (-not ((Read-LogSince $start) -match $Until)) {
        if ((Get-Date) -gt $deadline) { "Timed out waiting for '$Until'."; break }
        Start-Sleep -Milliseconds 500
    }
} else {
    Start-Sleep -Milliseconds ([int]($Wait * 1000))
}
(Read-LogSince $start) -split "`r?`n" | Where-Object { $_ -ne '' -and $_ -notmatch 'LogModelContextProtocol|LogSlateInspectorToolset' } |
    ForEach-Object { $_ -replace '^\[[^\]]+\]\[\s*\d+\]', '' }
