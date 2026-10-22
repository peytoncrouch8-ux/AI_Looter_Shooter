@echo off
setlocal enableextensions
title AI_Looter_Shooter (latest playtest build)
rem Double-click to play the newest packaged build from Builds\Playtest (made by Tools\package.ps1). It needs no Unreal
rem install and no compiling: it is the cooked game. Folder names are dates (yyyy-MM-dd_HHmm), so the last one by name is the
rem newest. Builds with no game exe in them (a failed package) are skipped.
rem Paths inside ( ) blocks are printed in quotes: a ")" in a folder name would end the block otherwise.

set "BUILDS=%~dp0Builds\Playtest"
set "GAME_DIR="
set "GAME_NAME="

if exist "%BUILDS%\" (
    for /f "delims=" %%D in ('dir /b /ad /o-n "%BUILDS%" 2^>nul') do (
        if not defined GAME_DIR if exist "%BUILDS%\%%D\Windows\AI_Looter_Shooter.exe" (
            set "GAME_DIR=%BUILDS%\%%D\Windows"
            set "GAME_NAME=%%D"
        )
    )
)

if not defined GAME_DIR (
    echo There is no playtest build yet.
    echo Make one with Tools\package.ps1 ^(close the Unreal Editor first; it takes a while^), then run this again.
    echo Looked in:
    echo   "%BUILDS%"
    echo.
    pause
    exit /b 1
)

echo Starting build %GAME_NAME%...
rem -fullscreen like the editor launcher; Alt+Enter switches to a window and back. No map is named, so the game opens
rem the main menu. The folder is the working directory so the game finds its Saved folder beside it.
pushd "%GAME_DIR%"
start "" "%GAME_DIR%\AI_Looter_Shooter.exe" -fullscreen
popd
exit /b 0
