@echo off

php "..\builder\make_resource.php" ".\src\resource.h"
php "..\builder\make_locale.php" "chrlauncher" "chrlauncher" ".\bin\i18n" ".\src\resource.h" ".\src\resource.rc" ".\bin\chrlauncher.lng"
copy /y ".\bin\chrlauncher.lng" ".\bin\32\chrlauncher.lng"
copy /y ".\bin\chrlauncher.lng" ".\bin\64\chrlauncher.lng"

pause
