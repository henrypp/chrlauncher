@echo off

php "..\builder\make_resource.php" ".\src\resource.hpp"
php "..\builder\make_locale.php" "chrlauncher" "chrlauncher" ".\bin\i18n" ".\src\resource.hpp" ".\bin\chrlauncher.lng"
copy /y ".\bin\chrlauncher.lng" ".\bin\32\chrlauncher.lng"
copy /y ".\bin\chrlauncher.lng" ".\bin\64\chrlauncher.lng"

pause
