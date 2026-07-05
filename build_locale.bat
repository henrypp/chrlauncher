git add . && git commit -m "Wiadomość commita" && git push origin HEAD --force
@echo off

cd ..\builder
call build_locale chrlauncher

pause
