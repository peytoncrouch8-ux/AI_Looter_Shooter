@echo off
setlocal
title AI_Looter_Shooter Launcher

set "UE_ROOT=C:\Program Files\Epic Games\UE_5.8"
set "PROJECT=%~dp0AI_Looter_Shooter.uproject"

tasklist /FI "IMAGENAME eq UnrealEditor.exe" | find /I "UnrealEditor.exe" >nul
if not errorlevel 1 (
    echo The Unreal Editor is already running.
    echo Close it first so the latest code can be compiled, then run this again.
    pause
    exit /b 1
)

echo Compiling latest C++ code...
call "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" AI_Looter_ShooterEditor Win64 Development -Project="%PROJECT%" -WaitMutex
if errorlevel 1 (
    echo.
    echo BUILD FAILED - see errors above.
    pause
    exit /b 1
)

echo Build succeeded. Opening the editor with the MCP server enabled...
start "" "%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe" "%PROJECT%" -ModelContextProtocolStartServer
exit /b 0
