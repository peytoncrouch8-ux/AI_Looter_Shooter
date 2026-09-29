# Lists the tools in an editor MCP toolset: name(params) - first line of its description.
# Usage: describe.ps1 <toolset> [nameFilter]    e.g. describe.ps1 editor_toolset.toolsets.asset.AssetTools
param([string]$Toolset, [string]$Filter = '')
$url = 'http://127.0.0.1:8000/mcp'
$h = @{ 'Accept' = 'application/json, text/event-stream' }
$init = Invoke-WebRequest -Uri $url -Method Post -ContentType 'application/json' -Headers $h -UseBasicParsing -Body (@{
    jsonrpc = '2.0'; id = 1; method = 'initialize'
    params = @{ protocolVersion = '2025-03-26'; capabilities = @{}; clientInfo = @{ name = 'looter-tools'; version = '1' } } } | ConvertTo-Json -Depth 5)
$h['Mcp-Session-Id'] = $init.Headers['Mcp-Session-Id']
$b = @{ jsonrpc = '2.0'; id = 2; method = 'tools/call'; params = @{ name = 'describe_toolset'; arguments = @{ toolset_name = $Toolset } } } | ConvertTo-Json -Depth 10
$r = Invoke-WebRequest -Uri $url -Method Post -ContentType 'application/json' -Headers $h -UseBasicParsing -Body $b
$text = (($r.Content | ConvertFrom-Json).result.content | ForEach-Object { $_.text }) -join ''
$desc = $text | ConvertFrom-Json
foreach ($t in $desc.tools) {
    $short = $t.name.Split('.')[-1]
    if ($Filter -and $short -notmatch $Filter) { continue }
    $first = ($t.description -split "`n")[0].Trim()
    $params = ($t.inputSchema.properties.PSObject.Properties | ForEach-Object { $_.Name }) -join ', '
    "$short($params) - $first"
}
