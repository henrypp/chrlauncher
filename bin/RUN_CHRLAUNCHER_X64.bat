@echo off
setlocal EnableExtensions DisableDelayedExpansion

cd /d "%~dp0"

echo ============================================================
echo chrlauncher portable x64 runner
echo ============================================================
echo ROOT: "%CD%"
echo EXE:  "%CD%\chrlauncher.x64.exe"
echo CFG:  "%CD%\Addons\config.ini"
echo.

if not exist "%CD%\chrlauncher.x64.exe" (
    echo [ERR] Missing chrlauncher.x64.exe in this directory.
    echo       Run this BAT from the artifact root, not from bin\aa or another subfolder.
    echo.
    pause
    exit /b 2
)

if not exist "%CD%\Addons\config.ini" (
    echo [ERR] Missing Addons\config.ini next to launcher.
    echo       The correct portable layout is:
    echo.
    echo       chrlauncher.x64.exe
    echo       RUN_CHRLAUNCHER_X64.bat
    echo       Addons\config.ini
    echo       profile\                 ^(created/used by Chromium^)
    echo       bin\chrome.exe            ^(downloaded/installed by launcher^)
    echo.
    pause
    exit /b 3
)

if exist "%CD%\bin.old" (
    echo [WARN] bin.old exists. If update/install fails with 0xC0000043,
    echo        close local Chrome processes and remove bin.old before retrying.
    echo.
)

start "" "%CD%\chrlauncher.x64.exe" %*
endlocal
exit /b 0
