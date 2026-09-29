# Calls one tool on the Unreal Editor's MCP server (the editor must be running with -ModelContextProtocolStartServer).
# Usage: mcp.ps1 <toolset> <tool> '<json args>' [imageOutName]
param([string]$Toolset, [string]$Tool, [string]$ArgsJson = '{}', [string]$ImageName = '')

$url = 'http://127.0.0.1:8000/mcp'
$headers = @{ 'Accept' = 'application/json, text/event-stream' }

$init = Invoke-WebRequest -Uri $url -Method Post -ContentType 'application/json' -Headers $headers -UseBasicParsing -Body (@{
    jsonrpc = '2.0'; id = 1; method = 'initialize'
    params = @{ protocolVersion = '2025-03-26'; capabilities = @{}; clientInfo = @{ name = 'looter-tools'; version = '1' } }
} | ConvertTo-Json -Depth 5)
$headers['Mcp-Session-Id'] = $init.Headers['Mcp-Session-Id']

$callArgs = @{ tool_name = $Tool; arguments = ($ArgsJson | ConvertFrom-Json) }
if ($Toolset) { $callArgs.toolset_name = $Toolset }
$body = @{ jsonrpc = '2.0'; id = 2; method = 'tools/call'; params = @{ name = 'call_tool'; arguments = $callArgs } } | ConvertTo-Json -Depth 30

$resp = Invoke-WebRequest -Uri $url -Method Post -ContentType 'application/json' -Headers $headers -UseBasicParsing -Body $body -TimeoutSec 300
$json = $resp.Content | ConvertFrom-Json
if ($json.error) { "ERROR: " + ($json.error | ConvertTo-Json -Depth 5); exit 1 }

$text = ($json.result.content | Where-Object { $_.type -eq 'text' } | ForEach-Object { $_.text }) -join "`n"
if ($ImageName) {
    $m = [regex]::Match($text, '"data"\s*:\s*"([A-Za-z0-9+/=]+)"')
    if ($m.Success) {
        $dir = Join-Path (Split-Path $PSScriptRoot -Parent) 'Saved\Screenshots\Tools'
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
        $out = Join-Path $dir "$ImageName.png"
        [IO.File]::WriteAllBytes($out, [Convert]::FromBase64String($m.Groups[1].Value))
        $out
    } else { "NO IMAGE: " + $text.Substring(0, [Math]::Min(400, $text.Length)) }
} else {
    if ($json.result.isError) { "TOOL ERROR: " + $text } else { $text }
}
