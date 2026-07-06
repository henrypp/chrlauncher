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
	<img src="/images/chrlauncher.png?hgcv" />
</p>

### Description:
Small and very fast portable launcher and updater for Chromium.

### System requirements:
- Windows 7, 8, 8.1, 10, 11 32-bit/64-bit
- An SSE2-capable CPU
- <s>KB2533623</s> KB3063858 update for Windows 7 was required [[x64](https://www.microsoft.com/en-us/download/details.aspx?id=47442) / [x32](https://www.microsoft.com/en-us/download/details.aspx?id=47409)]

### Donate:
- [Bitcoin](https://www.blockchain.com/btc/address/1LrRTXPsvHcQWCNZotA9RcwjsGcRghG96c) (BTC)
- [Ethereum](https://www.blockchain.com/explorer/addresses/eth/0xe2C84A62eb2a4EF154b19bec0c1c106734B95960) (ETH)
- [Yandex Money](https://yoomoney.ru/to/4100115776040583) (RUB)
- [Paypal](https://paypal.me/henrypp) (USD)

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
~~~

### Supported browser:
- as launcher - Chromium based (like Google Chrome, Opera, Yandex Browser, Vivaldi, etc.) and Firefox based (Mozilla Firefox, Basilisk, Pale Moon, Waterfox, etc.)
- as updater - Chromium only

### Chrome++:
- By default Chromium encrypt profile with user SID, which is disabled by [Chrome++](https://github.com/Bush2021/chrome_plus), so it is recomended. Starting with version 2.7 of chrlauncher it added support of Chrome++.

### Settings:
~~~ini
[chrlauncher]

# Custom Chromium update URL (string):
#ChromiumUpdateUrl=https://chromium.woolyss.com/api/v3/?os=windows&bit=%d&type=%s&out=string

# Command line for Chromium (string):
# See here: https://peter.sh/experiments/chromium-command-line-switches/
#
# When chrlauncher receives an URL from another application, it automatically
# appends Chromium's last used profile from "Local State" unless this command
# line already contains --profile-directory.
ChromiumCommandLine=--flag-switches-begin --user-data-dir=..\profile --no-default-browser-check --disable-logging --no-report-upload --flag-switches-end

# Repair pinned taskbar shortcuts that point directly to Chromium (boolean):
#
# false	-> do not touch pinned taskbar shortcuts
# true	-> rewrite matching pinned chrome.exe shortcuts to launch chrlauncher.exe (default)
#
# Keep this enabled if Windows taskbar/Jumplist opens chrome.exe directly and
# loses chrlauncher command-line options such as --user-data-dir.
ChromiumRepairTaskbarPins=true

# Use Chromium's last used profile when launcher starts from taskbar without URL (boolean):
#
# false	-> do not append --profile-directory for empty taskbar launches
# true	-> append the last profile from Local State and open a new window (default)
ChromiumUseLastProfileOnTaskbarLaunch=true

# Show Chromium profile picker when launcher starts without URL and the portable
# user-data-dir contains more than one profile (boolean):
#
# false	-> launch Chromium normally
# true	-> open chrome://profile-picker/ if no last profile can be resolved (default)
ChromiumShowProfilePickerOnTaskbarLaunch=true

# Launcher worker threads (integer):
#
# 0	-> auto-detect CPU count and clamp to 1..4 (default)
# 1..4	-> explicit worker thread count for launcher background work
LauncherWorkQueueThreads=0

# Spoof Chromium region / language hints (boolean):
#
# false	-> use Chromium/system profile region settings (default)
# true	-> add --lang and --accept-lang from the options below
#
# This changes Chromium locale and Accept-Language hints only. It does not
# change your IP address, GPS/location permission result, system timezone, or
# WebRTC network exposure.
ChromiumSpoofRegion=false

# Use current Windows user locale for ChromiumSpoofRegion (boolean):
#
# false	-> use ChromiumSpoofRegionLocale and ChromiumSpoofRegionAcceptLanguage (default)
# true	-> read the current Windows user locale and use it for --lang/--accept-lang
ChromiumSpoofUseWindowsLocale=false

# Locale used when ChromiumSpoofRegion=true (string):
ChromiumSpoofRegionLocale=en-US

# Accept-Language list used when ChromiumSpoofRegion=true (string):
ChromiumSpoofRegionAcceptLanguage=en-US,en

# Optional geolocation spoof command line (boolean/string):
#
# Chromium has no stable global command line for fake GPS coordinates. This is
# an explicit hook for builds/extensions that provide such a flag.
ChromiumSpoofGeolocation=false
ChromiumSpoofGeolocationCommandLine=

# Optional timezone spoof command line (boolean/string):
#
# Modern Chromium reads timezone from the OS. The default command line is a
# best-effort compatibility hook for older builds/testing builds only.
ChromiumSpoofTimeZone=false
ChromiumSpoofTimeZoneId=Europe/Warsaw
ChromiumSpoofTimeZoneCommandLine=--time-zone-for-testing=

# Optional browser hardware-id/fingerprint command line hook (boolean/string):
#
# This does not change real Windows hardware IDs. It only appends Chromium
# flags you explicitly provide for builds that support browser-level overrides.
ChromiumSpoofHardwareId=false
ChromiumSpoofHardwareIdCommandLine=

# Lossless browser optimization bundle (boolean):
#
# false	-> do not add conservative performance feature flags
# true	-> add flags from ChromiumLosslessOptimizationCommandLine and features
#          from ChromiumLosslessOptimizationFeatures (default)
ChromiumEnableLosslessOptimization=true

# Additional command line used when ChromiumEnableLosslessOptimization=true (string):
ChromiumLosslessOptimizationCommandLine=

# Chromium features enabled when ChromiumEnableLosslessOptimization=true (string):
ChromiumLosslessOptimizationFeatures=ParallelDownloading

# Long generated text / AI chat performance fixes (boolean):
#
# false	-> do not add long-text rendering/cache flags
# true	-> add flags from ChromiumTextPerformanceCommandLine and disable
#          features from ChromiumTextPerformanceDisableFeatures (default)
#
# This targets pages that append a lot of generated text over time. It reduces
# translate overhead and increases Skia font/resource cache limits. It does not
# rewrite page DOM or remove old chat nodes from websites.
ChromiumEnableTextPerformanceFixes=true

# Additional command line used when ChromiumEnableTextPerformanceFixes=true (string):
ChromiumTextPerformanceCommandLine=--disable-translate --skia-font-cache-limit-mb=64 --skia-resource-cache-limit-mb=128

# Chromium features disabled when ChromiumEnableTextPerformanceFixes=true (string):
ChromiumTextPerformanceDisableFeatures=

# Aggressive long-text/font/network fixes (boolean):
#
# false	-> keep accessibility, remote webfonts and smooth scrolling enabled (default)
# true	-> add flags from ChromiumAggressiveTextPerformanceCommandLine
#
# This can reduce renderer work on very long AI pages and avoid remote font
# downloads, but it may break screen readers, change font rendering, and make
# some sites look different.
ChromiumEnableAggressiveTextPerformanceFixes=false

# Additional command line used when ChromiumEnableAggressiveTextPerformanceFixes=true (string):
ChromiumAggressiveTextPerformanceCommandLine=--disable-renderer-accessibility --disable-remote-fonts --disable-font-subpixel-positioning --disable-smooth-scrolling

# Reduce work from hidden/background Chromium tabs (boolean):
#
# false	-> do not add background throttling / occlusion flags
# true	-> add flags from ChromiumBackgroundResourceSavingCommandLine
#
# This helps hidden tabs use fewer CPU/GPU resources. It cannot guarantee that
# every site pauses video decoding; Chromium and the site decide final behavior.
ChromiumEnableBackgroundResourceSaving=false

# Additional command line used when ChromiumEnableBackgroundResourceSaving=true (string):
ChromiumBackgroundResourceSavingCommandLine=

# Chromium features enabled when ChromiumEnableBackgroundResourceSaving=true (string):
ChromiumBackgroundResourceSavingFeatures=IntensiveWakeUpThrottling,CalculateNativeWinOcclusion

# Enable stronger renderer / JavaScript isolation hints (boolean):
#
# false	-> do not add renderer safety flags
# true	-> add flags from ChromiumRendererSafetyCommandLine
#
# This can reduce the chance that a bad script takes the whole browser with it.
# It is not a memory sanitizer and does not add real buffer-overflow detection.
ChromiumEnableRendererSafety=false

# Additional command line used when ChromiumEnableRendererSafety=true (string):
ChromiumRendererSafetyCommandLine=--site-per-process --js-flags=--stack_size=512

# Enable Windows 11 Chromium UI/runtime hints (boolean):
#
# false	-> do not add Windows 11 command line flags
# true	-> add flags from ChromiumWindows11CommandLine
ChromiumEnableWindows11Features=false

# Additional command line used when ChromiumEnableWindows11Features=true (string):
ChromiumWindows11CommandLine=

# Chromium features enabled when ChromiumEnableWindows11Features=true (string):
ChromiumWindows11Features=Windows11MicaTitlebar

# Enable DirectX / ANGLE D3D11 backend (boolean):
#
# false	-> let Chromium choose graphics backend (default)
# true	-> force flags from ChromiumDirectXCommandLine
ChromiumEnableDirectX=false

# Additional command line used when ChromiumEnableDirectX=true (string):
ChromiumDirectXCommandLine=--use-angle=d3d11

# Enable Vulkan backend (boolean):
#
# false	-> do not force Vulkan (default)
# true	-> add flags from ChromiumVulkanCommandLine
#
# Vulkan can improve some GPU paths but may be less stable than D3D11 depending
# on the Windows driver. If enabled, check chrome://gpu after restart.
ChromiumEnableVulkan=false

# Additional command line used when ChromiumEnableVulkan=true (string):
ChromiumVulkanCommandLine=--use-vulkan=native

# Chromium features enabled when ChromiumEnableVulkan=true (string):
ChromiumVulkanFeatures=Vulkan

# Enable multithreaded raster hints for Chromium (boolean):
#
# false	-> do not add raster thread flags
# true	-> add flags from ChromiumMultithreadingCommandLine
ChromiumEnableMultithreadedRaster=false

# Additional command line used when ChromiumEnableMultithreadedRaster=true (string):
ChromiumMultithreadingCommandLine=--num-raster-threads=4

# Override Chromium GPU software rendering blocklist (boolean):
#
# false	-> keep Chromium's built-in GPU blocklist
# true	-> add chrome://flags/#ignore-gpu-blocklist equivalent (default)
ChromiumIgnoreGpuBlocklist=true

# Additional command line used when ChromiumIgnoreGpuBlocklist=true (string):
ChromiumIgnoreGpuBlocklistCommandLine=--ignore-gpu-blocklist

# Enable Chromium hardware acceleration / GPU support (boolean):
#
# false	-> let Chromium use its normal GPU acceleration path (default)
# true	-> force extra GPU rasterization and zero-copy flags from ChromiumHardwareAccelerationCommandLine
#
# If your GPU driver is unstable, set ChromiumIgnoreGpuBlocklist=false first.
ChromiumEnableHardwareAcceleration=false

# Additional command line used when ChromiumEnableHardwareAcceleration=true (string):
ChromiumHardwareAccelerationCommandLine=--enable-gpu-rasterization --enable-zero-copy

# Enable scrollable / medium-width tab strip where Chromium still supports it (boolean):
#
# false	-> let Chromium shrink tabs normally
# true	-> keep tabs around medium width and scroll the tab strip instead (default)
ChromiumEnableScrollableTabs=true

# Chromium features enabled when ChromiumEnableScrollableTabs=true (string):
#
# 72  -> medium tabs
# 140 -> large tabs
# 256 -> tabs do not shrink
ChromiumScrollableTabsFeatures=ScrollableTabStrip:minimum_tab_width/72

# Enable local "Close tabs to the right" extension (boolean):
#
# false	-> do not load bundled tab-management extension
# true	-> add a local extension action/context menu for closing tabs to the right (default)
ChromiumEnableCloseTabsRightExtension=true

# Directory with the bundled close-tabs-right extension (string):
ChromiumCloseTabsRightExtensionDirectory=extensions\close-tabs-right

# Enable conservative autofill/password form compatibility flags (boolean):
#
# false	-> do not add form compatibility flags
# true	-> add ChromiumAutofillPasswordCommandLine (default)
ChromiumEnableAutofillPasswordFixes=true

# Additional command line used when ChromiumEnableAutofillPasswordFixes=true (string):
ChromiumAutofillPasswordCommandLine=--ignore-autocomplete-off-autofill --enable-single-click-autofill --enable-password-generation

# Enable QUIC / HTTP3 startup hints (boolean):
#
# false	-> do not add QUIC command line flags
# true	-> add flags from ChromiumQuicCommandLine
ChromiumEnableQuic=false

# Additional command line used when ChromiumEnableQuic=true (string):
ChromiumQuicCommandLine=--enable-quic

# Enable DNS startup hooks (boolean):
#
# false	-> use Chromium/system DNS settings (default)
# true	-> add flags from ChromiumDnsCommandLine
#
# For DNS-over-HTTPS, Chromium normally uses profile preferences or enterprise
# policy. Use this field for command-line DNS switches such as async DNS or
# host resolver rules, not as a full DoH policy replacement.
ChromiumEnableDnsOptions=false

# Additional command line used when ChromiumEnableDnsOptions=true (string):
ChromiumDnsCommandLine=--enable-async-dns

# Lightweight DNS blocklist from URL (boolean):
#
# false	-> do not download DNS/hosts blocklist rules (default)
# true	-> download ChromiumDnsBlocklistUrl and convert supported entries to
#          --host-resolver-rules before Chromium starts
#
# Supported input examples: "0.0.0.0 ads.example.com",
# "127.0.0.1 ads.example.com", "||ads.example.com^", or a plain domain.
# This is lighter at runtime than a content-script adblocker, but the Chromium
# command line has a practical size limit, so keep MaxRules conservative.
ChromiumEnableDnsBlocklistUrl=false

# DNS/hosts/adblock list URL used when ChromiumEnableDnsBlocklistUrl=true (string):
ChromiumDnsBlocklistUrl=

# Cache downloaded DNS blocklists in the dnsblock folder (boolean):
#
# false	-> download the list on every launch
# true	-> use cached dnsblock/list_<hash>.txt files and refresh stale entries (default)
ChromiumDnsBlocklistCache=true

# DNS blocklist cache directory (string):
# Relative paths are resolved against the chrlauncher directory.
ChromiumDnsBlocklistCacheDirectory=dnsblock

# Maximum cache age before trying to refresh from the network (integer, hours):
# If refresh fails, the stale cached list is used as fallback.
ChromiumDnsBlocklistCacheMaxAgeHours=24

# Address used for blocked domains (string):
ChromiumDnsBlocklistSink=0.0.0.0

# Also block subdomains with MAP *.domain rules (boolean):
ChromiumDnsBlocklistIncludeSubdomains=true

# Maximum accepted domains from the downloaded list (integer):
ChromiumDnsBlocklistMaxRules=256

# Maximum generated --host-resolver-rules payload length in characters (integer):
ChromiumDnsBlocklistMaxCommandLineChars=12000

# Improve Google Web Store / Google site Chrome detection (boolean):
#
# false	-> do not add Google Web Store compatibility flags
# true	-> add flags from ChromiumGoogleWebStoreCommandLine
#
# This spoofs a current Chrome UA, suppresses UA Client Hints that can reveal
# Chromium, and sets Web Store CRX URLs. It only applies on a cold browser
# start; close every running Chromium process before testing it.
#
# It cannot turn an ungoogled or stripped Chromium build into official Google
# Chrome, and Google pages can still use server-side or account-side checks.
ChromiumEnableGoogleWebStoreFix=false

# Additional command line used when ChromiumEnableGoogleWebStoreFix=true (string):
ChromiumGoogleWebStoreCommandLine=--user-agent="Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/150.0.0.0 Safari/537.36" --apps-gallery-url=https://chromewebstore.google.com/ --apps-gallery-update-url=https://clients2.google.com/service/update2/crx --apps-gallery-download-url="https://clients2.google.com/service/update2/crx?response=redirect&prodversion=150.0&x=id=%s&installsource=ondemand&uc"

# Chromium features disabled when ChromiumEnableGoogleWebStoreFix=true (string):
ChromiumGoogleWebStoreDisableFeatures=UserAgentClientHint

# Enable Chromecast / Google Cast support (boolean):
#
# false	-> do not add Cast workaround flags
# true	-> add Media Router, DIAL, all-IP and TCL discovery workaround flags
#
# Cast requires a Chromium build with Media Router support and Google component
# update access. Builds without Google integration (for example ungoogled)
# may not discover or connect to Cast devices even when this option is enabled.
#
# If a TCL TV is still missing, check that the TV and PC are on the same LAN/VLAN,
# AP/client isolation is disabled, and Windows Firewall allows private-network
# local discovery traffic.
ChromiumEnableCast=false

# Additional command line used when ChromiumEnableCast=true (string):
ChromiumCastCommandLine=--enable-media-router --load-media-router-component-extension

# Chromium features enabled when ChromiumEnableCast=true (string):
ChromiumCastFeatures=MediaRouter,CastAllowAllIPs,AllowAllSitesToInitiateMirroring,DialMediaRouteProvider,FallbackToAudioTabMirroring

# Chromium features disabled when ChromiumEnableCast=true (string):
ChromiumCastDisableFeatures=DelayMediaSinkDiscovery

# Chromium executable file name (string):
ChromiumBinary=chrome.exe

# Chromium binaries directory (string):
# Relative (to chrlauncher directory) or full path (env. variables supported).
ChromiumDirectory=.\bin

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
ChromiumType=dev-official

# Check for new Chromium version once in X days (integer):
#
# 2	-> check updates once in a X days (default)
# 0	-> disable update checking
# -1	-> force update checking
ChromiumCheckPeriod=2

# Last cached update checking timestamp (integer):
ChromiumLastCheck=0

# Start browser when downloading and installing is over (boolean)
#
ChromiumRunAtEnd=true

# A DLL hijack implements Chrome full portability as well as tab enhancements.
# https://github.com/Bush2021/chrome_plus
#ChromePlusDirectory=.\chrome_plus

#
# Internal settings (SDK)
#

# Set custom useragent (string):
#UserAgent=Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/128.0.0.0 Safari/537.36

# Set proxy configuration (string):
#Proxy=127.0.0.1:80
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
---
- Website: [github.com/henrypp](https://github.com/henrypp)
- Support: sforce5@mail.ru
---
(c) 2015-2026 Henry++
