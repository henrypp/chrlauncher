# chrlauncher

Small and very fast portable launcher and updater for Chromium.

```
To set as default internet browser run SetDefaultBrowser.bat (as admin).
```

#### Supported browser
- Launcher - Chromium and their clones (hi Google Chrome!).
- Updater - Chromium only.

#### Folder structure
- \bin - binaries directory.
- \profile - profile directory.

#### Settings
~~~
[chrlauncher]

# Set browser architecture:
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

# Command line for Chromium
#
# See here:
# http://peter.sh/experiments/chromium-command-line-switches/
#
ChromiumCommandLine=--user-data-dir=..\profile --no-default-browser-check

# Chromium binaries directory
#
ChromiumDirectory=.\bin
~~~
Website: www.henrypp.org<br />
Support: support@henrypp.org<br />
<br />
(c) 2016 Henry++
