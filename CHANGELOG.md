v1.9.2 (16 Juny 2016)
- removed update checking when Chromium is running (issue #16)
- fixed working under proxy (issue #17)
- changed default browser type to "dev-codecs-sync"
- updated PPAPI to 21.0.0.242

v1.9.1 (23 April 2016)
+ now you can pass environment variables via command line
- update checking time don't saved sometimes

v1.9 (19 April 2016)
+ now you can pass arguments to Chromium through launcher command line
- fixed parsing arguments ("blank tab" bug)
- fixed custom builds cleanup
- updated "SetDefaultBrowser" script
- updated PPAPI to 21.0.0.213
- minor bug fixes

v1.8 (19 March 2016)
+ now distributed under MIT license
+ support to download unofficial build with codecs (see chrlauncher.ini)
- updated PPAPI to 21.0.0.182

v1.7 (23 February 2016)
+ support to use portable version of PPAPI (see "FlashPlayerPath" config)
- fixed "SetDefaultBrowser" script (now used internal win7 feature to set as default browser)
- removed std::regex (saved about 100kb)
- stability improvements

v1.6 (22 January 2016)
+ added script to set chrlauncher as default browser
- cleanup Chromium package on extract
- removed Mozilla Firefox support (because his have native autoupdater)
- minor bug fixes

v1.5 (17 December 2015)
+ added "Mozilla Firefox" support 
+ added new settings
+ added command line for run specified browser
- fixed CreateProcess current directory parameter

v1.4 (4 December 2015)
+ added "ChromiumDirectory" setting
- fixed statusbar flickering

v1.3 (1 December 2015)
- fixed incorrect version checking

v1.2 (30 November 2015)
+ added checking for installation package is really Chromium archive
+ added exit confirmation when downloading/installing
- fixed architecture detection on various systems
- changed settings description (see chrlauncher.ini)

v1.1 (29 November 2015)
+ code rewritten on c++
+ added select for downloading architecture
+ added new settings (see chrlauncher.ini)
+ more information about update

v1.0 (26 November 2015)
- first public version
