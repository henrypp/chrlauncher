# chrlauncher

Small and very fast portable launcher and updater for Chromium and Mozilla Firefox.

#### Supported browser
- Launcher - Chromium, Mozilla Firefox and their clones (hi Google Chrome!).
- Updater - Chromium, Mozilla Firefox.

#### Folder structure
- chromium\bin - Chromium binaries directory.
- firefox\bin - Mozilla Firefox binaries directory.

#### Command line
- /browser chromium - run Chromium
- /browser firefox - run Mozilla Firefox

#### Settings
~~~
[chrlauncher]

# Set binary architecture:
#
# 0	-> autodetect (default)
# 64	-> 64-bit
# 32	-> 32-bit
#
BrowserArchitecture=0

# Last update checking timestamp:
#
BrowserCheckDate=0

# Check for new browser version once in X days:
#
# 0	-> disable update checking
# 1	-> once in day (default)
#
BrowserCheckPeriod=1

# Select internet browser:
#
# chromium	-> Chromium (default)
# firefox	-> Firefox
#
BrowserName=chromium

# Command line for Chromium
#
# See here:
# http://peter.sh/experiments/chromium-command-line-switches/
#
ChromiumCommandLine=--user-data-dir=..\profile --no-default-browser-check

# Chromium binaries directory
#
ChromiumDirectory=.\chromium\bin

# Command line for Firefox
#
# See here:
# https://developer.mozilla.org/docs/Mozilla/Command_Line_Options
#
FirefoxCommandLine=-profile "..\profile" -no-remote

# Firefox binaries directory
#
FirefoxDirectory=.\firefox\bin

# Firefox update channel
#
# release	-> Release channel (default)
# esr		-> ESR (Extended Support Release)
#
FirefoxChannel=release

# Localization:
#
# en-US	-> english (default)
# ru	-> russian
#
# See here:
# https://ftp.mozilla.org/pub/firefox/releases/latest/README.txt
#
FirefoxLocalization=en-US
~~~
Website: www.henrypp.org<br />
Support: support@henrypp.org<br />
<br />
(c) 2015 Henry++
