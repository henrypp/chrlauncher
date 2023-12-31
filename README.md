<h1 align="center">chrlauncher</h1>

<p align="center">
	<a href="https://github.com/henrypp/chrlauncher/releases"><img src="https://img.shields.io/github/v/release/henrypp/chrlauncher?style=flat-square&include_prereleases&label=version" /></a>
	<a href="https://github.com/henrypp/chrlauncher/releases"><img src="https://img.shields.io/github/downloads/henrypp/chrlauncher/total.svg?style=flat-square" /></a>
	<a href="https://github.com/henrypp/chrlauncher/issues"><img src="https://img.shields.io/github/issues-raw/henrypp/chrlauncher.svg?style=flat-square&label=issues" /></a>
	<a href="https://github.com/henrypp/chrlauncher/graphs/contributors"><img src="https://img.shields.io/github/contributors/henrypp/chrlauncher?style=flat-square" /></a>
	<a href="https://github.com/henrypp/chrlauncher/blob/master/LICENSE"><img src="https://img.shields.io/github/license/henrypp/chrlauncher?style=flat-square" /></a>
</p>

-------

<p align="center">
	<img src="https://www.henrypp.org/images/chrlauncher.png" />
</p>

### Description:
Small and very fast portable launcher and updater for Chromium.

### System requirements:
- Windows 8.1 and above operating system.
- [Visual C++ 2022 Redistributable package](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170)

### Donate:
- [Bitcoin](https://www.blockchain.com/btc/address/1LrRTXPsvHcQWCNZotA9RcwjsGcRghG96c) (BTC)
- [Ethereum](https://www.blockchain.com/explorer/addresses/eth/0xe2C84A62eb2a4EF154b19bec0c1c106734B95960) (ETC)
- [Paypal](https://paypal.me/henrypp) (USD)
- [Yandex Money](https://yoomoney.ru/to/4100115776040583) (RUB)

### GPG Signature:
Binaries have GPG signature `chrlauncher.exe.sig` in application folder.

- Public key: [pubkey.asc](https://raw.githubusercontent.com/henrypp/builder/master/pubkey.asc) ([pgpkeys.eu](https://pgpkeys.eu/pks/lookup?op=index&fingerprint=on&search=0x5635B5FD))
- Key ID: 0x5635B5FD
- Fingerprint: D985 2361 1524 AB29 BE73 30AC 2881 20A7 5635 B5FD

### Default browser:
chrlauncher has feature to use portable Chromium as default browser and it will be open links from another programs through chrlauncher.
- start "SetDefaultBrowser.bat" (as admin).
- start "Control panel" -> "Default programs" -> "Set your default programs" -> "chrlauncher" and set all checkboxes on.

### Command line:
There is list of arguments overrides .ini options
~~~
-autodownload - auto download update and install it!
-bringtofront - bring chrlauncher window to front when download started
-forcecheck - force update checking
-wait - start browser only when check/download/install update complete
-update - use chrlauncher as updater, but does not start Chromium
-ini .\chrlauncher.ini - start chrlauncher with custom configuration
~~~

### Supported browser:
- as launcher - Chromium based (like Google Chrome, Opera, Yandex Browser, Vivaldi, etc.) and Firefox based (Mozilla Firefox, Basilisk, Pale Moon, Waterfox, etc.)
- as updater - Chromium only

### Settings:
~~~ini
[chrlauncher]

# Custom Chromium update URL (string):
#ChromiumUpdateUrl=https://chromium.woolyss.com/api/v3/?os=windows&bit=%d&type=%s&out=string

# Command line for Chromium (string):
# See here: https://peter.sh/experiments/chromium-command-line-switches/
ChromiumCommandLine=--flag-switches-begin --user-data-dir=..\profile --no-default-browser-check --flag-switches-end

# Chromium executable file name (string):
ChromiumBinary=chrome.exe

# Chromium binaries directory (string):
# Relative (to chrlauncher directory) or full path (env. variables supported).
ChromiumDirectory=.\bin

# Set Chromium binaries architecture (integer):
#
# 0		-> 	autodetect (default)
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
# true	-> download & install Chromium update without start
ChromiumUpdateOnly=false

# Type of Chromium builds:
#
# dev-official
#	Official development builds from snapshots repository
#	"storage.googleapis.com/chromium-browser-snapshots/index.html" (32/64 bit)
#
# stable-codecs-sync
#	Unofficial stable builds with codecs
#	"github.com/Hibbiki/chromium-win64/releases" (64 bit)
#	"github.com/Hibbiki/chromium-win32/releases" (32 bit)
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

#
# Internal settings (SDK)
#

# Set custom useragent (string):
#UserAgent=Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/97.0.0.0 Safari/537.36
~~~
### FAQ:
- [If you lost all setting and extensions when copy Chromium to another PC (answer)](https://github.com/henrypp/chrlauncher/issues/116#issuecomment-444426692)
- [How to force check for an update every chrlauncher start?](https://github.com/henrypp/chrlauncher/issues/92#issuecomment-343274418)
- [How to host the Chromium package zip on custom network?](https://github.com/henrypp/chrlauncher/issues/86)
- [Can't sign-in to Chromium](https://github.com/henrypp/chrlauncher/issues/115#issuecomment-444268533)
- [Set proxy configuration for a chrlauncher](https://github.com/henrypp/chrlauncher/issues/61#issuecomment-439295515)
- [Pass on chrlauncher arguments into Chromium](https://github.com/henrypp/chrlauncher/issues/76#issuecomment-312444105)
- [Disable annoying chrlauncher window popup](https://github.com/henrypp/chrlauncher/issues/96#issuecomment-439294915)
- [Is it possible to downgrade Chromium version?](https://github.com/henrypp/chrlauncher/issues/112#issuecomment-440940865)
- [Fix for duplicated taskbar icons creation](https://github.com/henrypp/chrlauncher/issues/49#issuecomment-289285155)

Website: [www.henrypp.org](https://www.henrypp.org)<br />
Support: support@henrypp.org<br />
<br />
(c) 2015-2024 Henry++
