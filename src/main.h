// chrlauncher
// Copyright (c) 2015-2026 Henry++

#pragma once

#include "routine.h"

#include "resource.h"
#include "app.h"

DEFINE_GUID (GUID_TrayIcon, 0xEAD41630, 0x90BB, 0x4836, 0x82, 0x41, 0xAE, 0xAE, 0x12, 0xE8, 0x69, 0x12);

// config
#define LANG_MENU 3

#define FOOTER_STRING L"<a href=\"https://github.com/henrypp\">github.com/henrypp</a>\r\n" \
	L"<a href=\"https://chromium.woolyss.com\">chromium.woolyss.com</a>"

#define CHROMIUM_UPDATE_URL L"https://chromium.woolyss.com/api/v3/?os=windows&bit=%d&type=%s&out=string"

#define CHROMIUM_TYPE L"dev-official"
#define CHROMIUM_COMMAND_LINE L"--flag-switches-begin --user-data-dir=..\\profile --no-default-browser-check --disable-logging --no-report-upload --flag-switches-end"
#define CHROMIUM_CAST_COMMAND_LINE L"--enable-media-router --load-media-router-component-extension"
#define CHROMIUM_CAST_FEATURES L"MediaRouter,CastAllowAllIPs,AllowAllSitesToInitiateMirroring,DialMediaRouteProvider,FallbackToAudioTabMirroring"
#define CHROMIUM_CAST_DISABLE_FEATURES L"DelayMediaSinkDiscovery"
#define CHROMIUM_IGNORE_GPU_BLOCKLIST_COMMAND_LINE L"--ignore-gpu-blocklist"
#define CHROMIUM_HARDWARE_ACCELERATION_COMMAND_LINE L"--enable-gpu-rasterization --enable-zero-copy"
#define CHROMIUM_DIRECTX_COMMAND_LINE L"--use-angle=d3d11"
#define CHROMIUM_VULKAN_COMMAND_LINE L"--use-vulkan=native"
#define CHROMIUM_VULKAN_FEATURES L"Vulkan"
#define CHROMIUM_WINDOWS11_COMMAND_LINE L""
#define CHROMIUM_WINDOWS11_FEATURES L"Windows11MicaTitlebar"
#define CHROMIUM_MULTITHREADING_COMMAND_LINE L"--num-raster-threads=4"
#define CHROMIUM_SPOOF_REGION_LOCALE L"en-US"
#define CHROMIUM_SPOOF_REGION_ACCEPT_LANGUAGE L"en-US,en"
#define CHROMIUM_SPOOF_GEOLOCATION_COMMAND_LINE L""
#define CHROMIUM_SPOOF_TIMEZONE_ID L"Europe/Warsaw"
#define CHROMIUM_SPOOF_TIMEZONE_COMMAND_LINE L"--time-zone-for-testing="
#define CHROMIUM_SPOOF_HARDWARE_ID_COMMAND_LINE L""
#define CHROMIUM_LOSSLESS_OPTIMIZATION_COMMAND_LINE L""
#define CHROMIUM_LOSSLESS_OPTIMIZATION_FEATURES L"ParallelDownloading"
#define CHROMIUM_TEXT_PERFORMANCE_COMMAND_LINE L"--disable-translate --skia-font-cache-limit-mb=64 --skia-resource-cache-limit-mb=128"
#define CHROMIUM_TEXT_PERFORMANCE_DISABLE_FEATURES L""
#define CHROMIUM_TEXT_AGGRESSIVE_COMMAND_LINE L"--disable-renderer-accessibility --disable-remote-fonts --disable-font-subpixel-positioning --disable-smooth-scrolling"
#define CHROMIUM_BACKGROUND_RESOURCE_SAVING_COMMAND_LINE L""
#define CHROMIUM_BACKGROUND_RESOURCE_SAVING_FEATURES L"IntensiveWakeUpThrottling,CalculateNativeWinOcclusion"
#define CHROMIUM_RENDERER_SAFETY_COMMAND_LINE L"--site-per-process --js-flags=--stack_size=512"
#define CHROMIUM_SCROLLABLE_TABS_FEATURES L"ScrollableTabStrip:minimum_tab_width/72"
#define APP_ADDONS_DIRECTORY L"Addons"
#define APP_ADDONS_CONFIG_FILE L"config.ini"
#define CHROMIUM_CLOSE_TABS_RIGHT_EXTENSION_DIRECTORY L"Addons\\extensions\\close-tabs-right"
#define CHROMIUM_AUTOFILL_PASSWORD_COMMAND_LINE L"--ignore-autocomplete-off-autofill --enable-single-click-autofill --enable-password-generation"
#define CHROMIUM_QUIC_COMMAND_LINE L"--enable-quic"
#define CHROMIUM_DNS_COMMAND_LINE L"--enable-async-dns"
#define CHROMIUM_SECURE_DNS_COMMAND_LINE L"--dns-over-https-mode=secure --dns-over-https-templates=\"%s\""
#define CHROMIUM_DNS_BLOCKLIST_CACHE_DIRECTORY L"Addons\\dnsblock"
#define CHROME_PLUS_DIRECTORY L"Addons\\chrome_plus"
#define CHROMIUM_DNS_BLOCKLIST_SINK L"0.0.0.0"
#define CHROMIUM_GOOGLE_WEBSTORE_COMMAND_LINE L"--user-agent=\"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/150.0.0.0 Safari/537.36\" --apps-gallery-url=https://chromewebstore.google.com/ --apps-gallery-update-url=https://clients2.google.com/service/update2/crx --apps-gallery-download-url=\"https://clients2.google.com/service/update2/crx?response=redirect&prodversion=150.0&x=id=%s&installsource=ondemand&uc\""
#define CHROMIUM_GOOGLE_WEBSTORE_DISABLE_FEATURES L"UserAgentClientHint"

typedef struct _BROWSER_INFORMATION
{
	HANDLE htaskbar;
	HANDLE hwnd;

	PR_STRING args_str;
	PR_STRING urls_str;
	PR_STRING user_data_dir;

	PR_STRING chrome_plus_dir;
	PR_STRING browser_name;
	PR_STRING browser_type;
	PR_STRING cache_path;
	PR_STRING binary_dir;
	PR_STRING binary_path;
	PR_STRING download_url;
	PR_STRING current_version;
	PR_STRING new_version;

	LONG64 timestamp;
	LONG64 reserved1;

	LONG check_period;
	LONG architecture;

	BOOLEAN is_autodownload;
	BOOLEAN is_bringtofront;
	BOOLEAN is_deletetorecycle;
	BOOLEAN is_forcecheck;
	BOOLEAN is_hasurls;
	BOOLEAN is_hasprofiledir;
	BOOLEAN is_onlyupdate;
	BOOLEAN is_opennewwindow;
	BOOLEAN is_waitdownloadend;
} BROWSER_INFORMATION, *PBROWSER_INFORMATION;
