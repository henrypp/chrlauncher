# chrlauncher

Small and very fast portable Chromium launcher and updater.

#### Folder structure:
- \bin - Chromium binaries directory.
- \profile - Chromium profile directory.

#### Settings:
~~~
[chrlauncher]

# ChromiumArchitecture
#
# 0 -> auto detected
# 64 -> check for 64-bit Chromium
# 32 -> check for 32-bit Chromium
ChromiumArchitecture=0

# ChromiumCheckDays
#
# Check for new version once in X days
#
# 0 -> disable update checking
# 1 -> once in day
ChromiumCheckDays=1

# ChromiumCommandLine
#
# Command line for run Chromium
# See here: http://peter.sh/experiments/chromium-command-line-switches/
# ChromiumCommandLine=--user-data-dir=..\profile --no-default-browser-check
~~~
Website: www.henrypp.org<br />
Support: support@henrypp.org<br />
<br />
(c) 2015 Henry++
