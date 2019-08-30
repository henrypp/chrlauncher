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
- /update - use chrlauncher as updater, but does not start Chromium
- /ini .\chrlauncher.ini - start chrlauncher with custom configuration

#### Supported browser:
- as launcher - Chromium based (like Google Chrome, Opera, Yandex Browser, Vivaldi, etc.) and Firefox based (Mozilla Firefox, Basilisk, Pale Moon, Waterfox, etc.)
- as updater - Chromium only

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
# See here: https://peter.sh/experiments/chromium-command-line-switches/
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

# Auto download updates if found (boolean)
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

# Use chrlauncher as updater, but does not start Chromium (boolean):
#
# false	-> update & start Chromium (default)
# true	-> only check for Chromium update
ChromiumUpdateOnly=false

# Type of Chromium builds:
#
# dev-official
#	Official development builds from snapshots repository
#	"storage.googleapis.com/chromium-browser-snapshots/index.html" (32/64 bit)
#
# stable-codecs-sync
#	Unofficial stable builds with codecs
#	"github.com/macchrome/winchrome/releases" (32/64 bit)
#
# dev-nosync
#	Unofficial development builds without Google services
#	"github.com/RobRich999/Chromium_Clang/releases" (32/64 bit)
#
# dev-codecs-sync
#	Unofficial development builds with codecs and without Google services
#	"github.com/macchrome/winchrome/releases" (64 bit)
#
# dev-codecs-nosync
#	Unofficial development builds with codecs and without Google services
#	"github.com/macchrome/winchrome/releases" (64 bit)
#
# ungoogled-chromium
#	Unofficial builds without Google integration and enhanced privacy (based on Eloston project)
#	"github.com/macchrome/winchrome/releases/" (32/64 bit)
#	"github.com/Eloston/ungoogled-chromium"
#
# stable-codecs-nosync
#	Unofficial stable builds with codecs and without google services
#	!!! DISCONTINUED since June 2018 !!!
ChromiumType=dev-official

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
- [If you lost all setting and extensions when copy Chromium to another PC (answer)](https://github.com/henrypp/chrlauncher/issues/116#issuecomment-444426692)
- [How to force check for an update every chrlauncher start?](https://github.com/henrypp/chrlauncher/issues/92#issuecomment-343274418)
- [How to host the Chromium package zip on custom network?](https://github.com/henrypp/chrlauncher/issues/86)
- [Can't sign-in to Chromium](https://github.com/henrypp/chrlauncher/issues/115#issuecomment-444268533)
- [Set proxy configuration for a chrlauncher](https://github.com/henrypp/chrlauncher/issues/61#issuecomment-439295515)
- [Pass on chrlauncher arguments into Chromium](https://github.com/henrypp/chrlauncher/issues/76#issuecomment-312444105)
- [Disable annoying chrlauncher window popup](https://github.com/henrypp/chrlauncher/issues/96#issuecomment-439294915)
- [Is it possible to downgrade Chromium version?](https://github.com/henrypp/chrlauncher/issues/112#issuecomment-440940865)
- [Fix for duplicated taskbar icons creation](https://github.com/henrypp/chrlauncher/issues/49#issuecomment-289285155)
- [Update flash PPAPI plugin](https://github.com/henrypp/chrlauncher/issues/21#issuecomment-232352687)
- [Fix for flash PPAPI plugin loading](https://github.com/henrypp/chrlauncher/issues/24#issuecomment-242173325)

Website: [www.henrypp.org](https://www.henrypp.org)<br />
Support: support@henrypp.org<br />
<br />
(c) 2015-2019 Henry++
