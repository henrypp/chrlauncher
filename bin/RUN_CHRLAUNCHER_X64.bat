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
echo chrlauncher portable x64 temp runner
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
echo Do not run chrlauncher.x64.exe directly from C:\.
echo Use this BAT so the EXE is copied to TEMP before launch.
echo.
pause
exit /b 3

:root_found
cd /d "%ROOT%"

set "RUN_ID=%RANDOM%%RANDOM%"
set "TMP_ROOT=%TEMP%\chrlauncher_temp_run_%RUN_ID%"
set "TMP_EXE=%TMP_ROOT%\chrlauncher.x64.exe"

echo ============================================================
echo chrlauncher portable x64 temp runner
echo ============================================================
echo BAT:  "%START_DIR%"
echo ROOT: "%ROOT%"
echo SRC:  "%ROOT%\chrlauncher.x64.exe"
echo TEMP: "%TMP_EXE%"
echo CFG:  "%ROOT%\Addons\config.ini"
echo.

if exist "%ROOT%\bin.old" (
    echo [WARN] bin.old exists. If update/install fails with 0xC0000043,
    echo        close local Chrome processes and remove bin.old before retrying.
    echo.
)

mkdir "%TMP_ROOT%" >nul 2>nul
if errorlevel 1 goto :copy_failed

copy /Y "%ROOT%\chrlauncher.x64.exe" "%TMP_EXE%" >nul
if errorlevel 1 goto :copy_failed

pushd "%ROOT%"
start "" "%TMP_EXE%" %*
popd

endlocal
exit /b 0

:copy_failed
echo [ERR] Cannot create temporary launcher copy.
echo TMP_ROOT="%TMP_ROOT%"
echo.
echo Check %%TEMP%% permissions and antivirus quarantine.
echo.
pause
exit /b 5
