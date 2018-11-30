chrlauncher [![Github All Releases](https://img.shields.io/github/downloads/henrypp/chrlauncher/total.svg)](https://github.com/henrypp/chrlauncher/releases) [![GitHub issues](https://img.shields.io/github/issues-raw/henrypp/chrlauncher.svg)](https://github.com/henrypp/chrlauncher/issues) [![Donate via PayPal](https://img.shields.io/badge/donate-paypal-red.svg)](https://www.paypal.me/henrypp/15) [![Donate via Bitcoin](https://img.shields.io/badge/donate-bitcoin-red.svg)](https://blockchain.info/address/1LrRTXPsvHcQWCNZotA9RcwjsGcRghG96c) [![Licence](https://img.shields.io/badge/license-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0.en.html)
=======

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
- /ini .\chrlauncher.ini - start chrlauncher with custom configuration

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

# Custom Chromium update URL (string):
#ChromiumUpdateUrl=https://chromium.woolyss.com/api/v3/?os=windows&bit=%d&type=%s&out=string

# Command line for Chromium (string):
# See here: http://peter.sh/experiments/chromium-command-line-switches/
ChromiumCommandLine=--user-data-dir=..\profile --no-default-browser-check --allow-outdated-plugins --disable-logging --disable-breakpad

# Chromium executable file name (string):
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
# false	-> show tray tip if update found, downloading manually (default)
# true	-> auto download update and install it!
ChromiumAutoDownload=false

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
# dev-official		-> Official development builds from snapshots repository "commondatastorage.googleapis.com/chromium-browser-snapshots/index.html"
#
# dev-codecs-sync	-> Unofficial development builds with codecs "github.com/henrypp/chromium/releases" (default)
#
# dev-clang-nosync	-> Unofficial development builds without Google services "github.com/henrypp/chromium/releases"
#
# stable-codecs-sync	-> Unofficial stable builds with codecs "github.com/henrypp/chromium/releases"
#
# ungoogled-chromium	-> Unofficial builds without google integration and enhanced privacy "github.com/Eloston/ungoogled-chromium"
#
# dev-codecs-nosync	-> !!! DISCONTINUED since June 2018 !!!
# Unofficial development builds with codecs and without Google services
#
# stable-codecs-nosync	-> !!! DISCONTINUED since June 2018 !!!
# Unofficial stable builds with codecs and without google services
#
# dev-codecs-nosync-experimental -> !!! EXPERIMENTAL !!!
# Unofficial development builds with codecs and without Google services "github.com/macchrome/winchrome/releases"
ChromiumType=dev-codecs-sync

# Check for new Chromium version once in X days (integer):
#
# 2	-> check updates once in a X days (default)
# 0	-> disable update checking
# -1	-> force update checking
ChromiumCheckPeriod=2

# Last cached update checking timestamp (integer):
ChromiumLastCheck=0

##########################
# Internal settings (SDK)
##########################

# Enable classic theme UI (boolean):
#ClassicUI=true

# Set custom useragent (string):
#UserAgent=Mozilla/5.0 AppleWebKit/537.36 (KHTML, like Gecko) Chrome/55.0.2883.87 Safari/537.36

# Set proxy configuration (string):
#Proxy=127.0.0.1:80
~~~
#### FAQ:
- [How to force check for an update every chrlauncher start?](https://github.com/henrypp/chrlauncher/issues/92#issuecomment-343274418)
- [Disable annoying chrlauncher window popup](https://github.com/henrypp/chrlauncher/issues/96#issuecomment-439294915)
- [Is it possible to downgrade Chromium version?](https://github.com/henrypp/chrlauncher/issues/112#issuecomment-440940865)
- [Fix for duplicated taskbar icons creation](https://github.com/henrypp/chrlauncher/issues/49#issuecomment-289285155)
- [Set proxy configuration for a chrlauncher](https://github.com/henrypp/chrlauncher/issues/61#issuecomment-439295515)
- [Pass on chrlauncher arguments into Chromium](https://github.com/henrypp/chrlauncher/issues/76#issuecomment-312444105)
- [Update flash PPAPI plugin](https://github.com/henrypp/chrlauncher/issues/21#issuecomment-232352687)
- [Fix for flash PPAPI plugin loading](https://github.com/henrypp/chrlauncher/issues/24#issuecomment-242173325)
- [Host the Chromium package zip on custom network?](https://github.com/henrypp/chrlauncher/issues/86)

Website: [www.henrypp.org](https://www.henrypp.org)<br />
Support: support@henrypp.org<br />
<br />
(c) 2015-2018 Henry++
