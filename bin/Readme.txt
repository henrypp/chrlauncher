chrlauncher

Small and very fast portable Chromium launcher and updater.

Supported browser:
- Launcher - all Chromium based browsers (see "Folder structure").
- Updater - only Chromium.

Folder structure:
- \bin - Chromium binaries directory.
- \profile - Chromium profile directory.

Settings:
[chrlauncher]

# Select Chromium architecture:
#
# 0 -> auto detected
# 64 -> check for 64-bit Chromium
# 32 -> check for 32-bit Chromium
ChromiumArchitecture=0

# Check for new version once in X days:
#
# 0 -> disable update checking
# 1 -> once in day
ChromiumCheckDays=1

# Command line for run Chromium
# See here: http://peter.sh/experiments/chromium-command-line-switches/
#
#ChromiumCommandLine=--user-data-dir=..\profile --no-default-browser-check

# Chromium binaries directory
#
#ChromiumDirectory=bin

Website: www.henrypp.org
Support: support@henrypp.org

(c) 2015 Henry++
