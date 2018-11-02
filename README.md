## chrlauncher

![chrlauncher](https://www.henrypp.org/images/chrlauncher.png)

Small and very fast portable launcher and updater for Chromium.

#### Default browser:
chrlauncher has feature to use portable Chromium as default browser and it will be open links from another programs through chrlauncher.
- start "SetDefaultBrowser.bat" (as admin).
- start "Control panel" -> "Default programs" -> "Set your default programs" -> "chrlauncher" and set all checkboxes on.

#### Command line:
There is list of arguments overrides .ini options
- /autodownload - auto download update and install it!
- /bringtofront - bring chrlauncher window to front when download started
- /forcecheck - force update checking
- /wait - start browser only when check/download/install update complete

#### Supported browser:
- Launcher - Chromium and their clones (like Google Chrome, Yandex Browser and other legitimate trojans!).
- Updater - Chromium only.

#### Flash Player:
chrlauncher has feature for use portable Flash Player PPAPI.
- download portable Flash Player PPAPI 32-bit/64-bit: http://effect8.ru/soft/media/adobe-flash-player-portable.html
- unpack archive to the "Plugins" folder.
- open "chrlauncher.ini" and find "FlashPlayerPath" option and then set ".\plugins\%flash_player_dll_name_here%"

#### Settings:
~~~ini
[chrlauncher]

# Custom Chromium update url (string):
#ChromiumUpdateUrl=https://chromium.woolyss.com/api/v3/?os=windows&bit=%d&type=%s&out=string

# Command line for Chromium (string):
# See here: http://peter.sh/experiments/chromium-command-line-switches/
ChromiumCommandLine=--user-data-dir=..\profile --no-default-browser-check --allow-outdated-plugins --disable-logging --disable-breakpad

# Chromium binary file name (string):
ChromiumBinary=chrome.exe

# Chromium binaries directory (string):
# Relative (to chrlauncher directory) or full path (env. variables supported).
ChromiumDirectory=.\bin

# Adobe Flash Player PPAPI portable library path (string):
# Relative (to chrlauncher directory) or full path (env. variables supported).
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
# false	-> show tray tip if update found, downloading manually
# true	-> auto download update and install it! (default)
ChromiumAutoDownload=true

# Bring chrlauncher window when download started (boolean)
#
# false	-> don't bring main window to front automatically
# true	-> bring chrlauncher window to front when download started (default)
ChromiumBringToFront=true

# Set download in foreground mode (boolean):
#
# false	-> start browser and check/download/install update in background
# true	-> start browser only when check/download/install update complete (default)
ChromiumWaitForDownloadEnd=true

# Type of Chromium builds:
#
# dev-official		-> official development builds from snapshots repository "commondatastorage.googleapis.com/chromium-browser-snapshots/index.html"
# dev-codecs-sync	-> unofficial development builds with codecs "github.com/henrypp/chromium/releases" (default)
# stable-codecs-sync	-> unofficial stable builds with codecs "github.com/henrypp/chromium/releases"
# dev-codecs-nosync	-> unofficial development builds with codecs and without google services "github.com/henrypp/chromium/releases"
# stable-codecs-nosync	-> unofficial stable builds with codecs and without google services "github.com/henrypp/chromium/releases"
# ungoogled-chromium	-> unofficial builds without google integration and enhanced privacy "github.com/Eloston/ungoogled-chromium"
ChromiumType=dev-codecs-sync

# Check for new Chromium version once in X days (integer):
#
# 1	-> check updates once in a X day (default)
# 0	-> disable update checking
# -1	-> force update checking
ChromiumCheckPeriod=1
~~~
Website: [www.henrypp.org](https://www.henrypp.org)<br />
Support: support@henrypp.org<br />
<br />
(c) 2015-2018 Henry++
