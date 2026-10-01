@echo off
setlocal
title AI_Looter_Shooter
rem Double-click to play AI_Looter_Shooter as a standalone game: no editor UI, fullscreen on the main monitor.
rem When Unreal isn't running it first compiles the latest C++, because the game runs the editor build's code.
rem To open the Unreal Editor instead, use "Launch AI_Looter_Shooter Editor (backup).bat".
rem Paths inside ( ) blocks are printed in quotes: a ")" in a folder name would end the block otherwise.

set "UE_ROOT=C:\Program Files\Epic Games\UE_5.8"
set "UE_EXE=%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe"
set "PROJECT=%~dp0AI_Looter_Shooter.uproject"

if not exist "%UE_EXE%" (
    echo Unreal Engine 5.8 was not found here:
    echo   "%UE_EXE%"
    echo Install Unreal Engine 5.8 from the Epic Games Launcher, or change UE_ROOT at the top of this file.
    echo.
    pause
    exit /b 1
)
if not exist "%PROJECT%" (
    echo The project file was not found here:
    echo   "%PROJECT%"
    echo Keep this launcher in the project folder, next to AI_Looter_Shooter.uproject.
    echo.
    pause
    exit /b 1
)

rem A running editor (or game) has the compiled code loaded, so it can't be rebuilt now. The game then starts from
rem the same build the editor is using.
"%SystemRoot%\System32\tasklist.exe" /NH 2>nul | "%SystemRoot%\System32\findstr.exe" /I /B /C:"UnrealEditor.exe" /C:"UnrealEditor-Cmd.exe" >nul
if not errorlevel 1 (
    echo Unreal is already open, so the code is not recompiled: the game uses the build the editor is running.
) else (
    echo Compiling the latest C++ code. This can take a few minutes after code changes...
    call "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" AI_Looter_ShooterEditor Win64 Development -Project="%PROJECT%" -WaitMutex
    if errorlevel 1 (
        echo.
        echo BUILD FAILED - the errors are listed above, so the game was not started.
        echo Fix the code, then double-click this launcher again.
        echo.
        pause
        exit /b 1
    )
)

rem -game runs the project as a standalone game instead of opening the editor. -fullscreen uses borderless fullscreen
rem at the desktop resolution; Alt+Enter switches to a window and back.
echo Starting AI_Looter_Shooter...
start "" "%UE_EXE%" "%PROJECT%" -game -fullscreen
if errorlevel 1 (
    echo.
    echo The game could not be started. Try again, or open the editor with the backup launcher.
    echo.
    pause
    exit /b 1
)
exit /b 0
