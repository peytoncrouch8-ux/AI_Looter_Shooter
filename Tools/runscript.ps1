# Runs an editor tool script (sandboxed Python that defines run() and calls tools with execute_tool) from a file.
# Usage: runscript.ps1 <file.py>
param([string]$File)
$script = [IO.File]::ReadAllText($File)
$argsJson = @{ script = $script } | ConvertTo-Json -Compress
& (Join-Path $PSScriptRoot 'mcp.ps1') 'editor_toolset.toolsets.programmatic.ProgrammaticToolset' 'execute_tool_script' $argsJson
