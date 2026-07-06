@echo off
setlocal EnableExtensions DisableDelayedExpansion

set "START_DIR=%~dp0"
set "ROOT="

for %%I in ("%START_DIR%.") do set "CUR=%%~fI"

:find_root
if exist "%CUR%\chrlauncher.x64.exe" if exist "%CUR%\Addons\config.ini" (
    set "ROOT=%CUR%"
    goto :root_found
)

for %%I in ("%CUR%\..") do set "NEXT=%%~fI"
if /I "%NEXT%"=="%CUR%" goto :root_missing
set "CUR=%NEXT%"
goto :find_root

:root_missing
echo ============================================================
echo chrlauncher portable x64 runner
echo ============================================================
echo [ERR] Portable root was not found from: "%START_DIR%"
echo.
echo Expected layout at the same level or in a parent directory:
echo.
echo   chrlauncher.x64.exe
echo   Addons\config.ini
echo   profile\                 ^(created/used by Chromium^)
echo   bin\chrome.exe            ^(downloaded/installed by launcher^)
echo.
echo Do not place only chrlauncher.x64.exe inside bin\aa. Keep the full artifact layout.
echo.
pause
exit /b 3

:root_found
cd /d "%ROOT%"

echo ============================================================
echo chrlauncher portable x64 runner
echo ============================================================
echo BAT:  "%START_DIR%"
echo ROOT: "%ROOT%"
echo EXE:  "%ROOT%\chrlauncher.x64.exe"
echo CFG:  "%ROOT%\Addons\config.ini"
echo.

if exist "%ROOT%\bin.old" (
    echo [WARN] bin.old exists. If update/install fails with 0xC0000043,
    echo        close local Chrome processes and remove bin.old before retrying.
    echo.
)

start "" "%ROOT%\chrlauncher.x64.exe" %*
endlocal
exit /b 0
