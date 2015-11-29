# chrlauncher

Portable Chromium launcher and updater.

#### Folder structure:
- \bin - Chromium binaries directory.
- \profile - Chromium profile directory.

#### Settings:
~~~
[chrlauncher]

; ChromiumArchitecture=0
; <0 - auto (default), 64 - check for 64-bit Chromium, 32 - check for 32-bit Chromium>

; ChromiumCheckDays=1
; check for new version once in X days.
;
; <0 - disable update checking, 1 - once in day (delault)>

; ChromiumCommandLine=--user-data-dir=..\profile --no-default-browser-check
; command line for run Chromium
; see here: http://peter.sh/experiments/chromium-command-line-switches/
~~~
Website: www.henrypp.org<br />
Support: support@henrypp.org<br />
<br />
(c) 2015 Henry++
