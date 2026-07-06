@echo off
@setlocal enableextensions

pushd "%~dp0..\builder"
call build_locale chrlauncher
set "BUILD_STATUS=%ERRORLEVEL%"
popd

echo done...
pause
exit /b %BUILD_STATUS%
