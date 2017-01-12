chrlauncher

Small and very fast portable launcher and updater for Chromium.

Default browser:
chrlauncher has feature to use portable Chromium as default browser and it will be open links from another programs through chrlauncher.
- start "SetDefaultBrowser.bat" (as admin).
- start "Control panel" -> "Default programs" -> "Set your default programs" -> "chrlauncher" and set all checkboxes on.

Command line:
There is list of arguments overrides .ini options.
/a - auto download new version of Chromium if present
/b - bring chrlauncher to front on download started
/f - force update checking
/q - no error messages will be shown (quiet)

Supported browser:
- Launcher - Chromium and their clones (like Google Chrome, Yandex Browser and other legitimate trojans!).
- Updater - Chromium only.

Flash Player:
chrlauncher has feature for use portable Flash Player PPAPI.
- download portable Flash Player PPAPI 32-bit/64-bit: http://effect8.ru/soft/media/adobe-flash-player-portable.html
- unpack archive to the "Plugins" folder.
- open "chrlauncher.ini" and find "FlashPlayerPath" option and then set ".\Plugins\%flash_player_dll_name_here%"

Settings:
[chrlauncher]

# Command line for Chromium (string):
# See here: http://peter.sh/experiments/chromium-command-line-switches/
ChromiumCommandLine=--user-data-dir=..\profile --no-default-browser-check --allow-outdated-plugins --disable-component-update

# Chromium binaries directory (string):
ChromiumDirectory=.\bin

# Chromium binary file name (string):
ChromiumBinary=chrome.exe

# Adobe Flash Player PPAPI portable library path (string):
# Download here: http://effect8.ru/soft/media/adobe-flash-player-portable.html
FlashPlayerPath=.\plugins\pepflashplayer.dll

# Set Chromium binaries architecture (integer):
#
# 0	-> autodetect (default)
# 64	-> 64-bit
# 32	-> 32-bit
ChromiumArchitecture=0

# Auto download updates if founded (boolean)
#
# false	-> show tray tip if update found, downloading manually (default)
# true	-> auto download update and install it!
ChromiumAutoDownload=falseboolean

# Bring chrlauncher window when download started (boolean)
#
# false	-> don't bring main window to front automatically (default)
# true	-> bring chrlauncher window to front when download started
ChromiumBringToFront=false

# Set error mode (boolean):
#
# false	-> show warning messages (default)
# true	-> no error messages will be shown
ChromiumIsSilent=false

# Type of Chromium builds:
#
# dev-official		-> official development builds from snapshots repository "commondatastorage.googleapis.com/chromium-browser-snapshots/index.html"
# dev-codecs-sync	-> unofficial development builds with codecs from "github.com/henrypp/chromium/releases" (default)
# stable-codecs-sync	-> unofficial stable builds with codecs from "github.com/henrypp/chromium/releases"
# dev-codecs-nosync	-> unofficial development builds with codecs and without the profile button from "github.com/henrypp/chromium/releases"
# stable-codecs-nosync	-> unofficial stable builds with codecs and without the profile button from "github.com/henrypp/chromium/releases"
ChromiumType=dev-codecs-sync

# Check for new Chromium version once in X days (integer):
#
# 1	-> check updates once in a 1 day (default)
# 0	-> disable update checking
# -1	-> force update checking
ChromiumCheckPeriod=1

# Last cached founded build date of Chromium in unix-time format(integer):
ChromiumLastBuild=0

# Last cached update checking timestamp (integer):
ChromiumLastCheck=0

# Last cached founded version of Chromium (string):
ChromiumLastVersion=

Website: www.henrypp.org
Support: support@henrypp.org

(c) 2015-2017 Henry++
