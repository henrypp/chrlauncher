// chrlauncher
// Copyright (c) 2015-2022 Henry++

#pragma once

#include "routine.h"

#include "resource.h"
#include "app.h"

DEFINE_GUID (GUID_TrayIcon, 0xead41630, 0x90bb, 0x4836, 0x82, 0x41, 0xae, 0xae, 0x12, 0xe8, 0x69, 0x12);

// config
#define LANG_MENU 0

#define FOOTER_STRING L"<a href=\"https://github.com/henrypp\">github.com/henrypp</a>\r\n" \
	L"<a href=\"https://chromium.woolyss.com\">chromium.woolyss.com</a>"

#define CHROMIUM_UPDATE_URL L"https://chromium.woolyss.com/api/v3/?os=windows&bit=%d&type=%s&out=string"

#define CHROMIUM_TYPE L"dev-official"
#define CHROMIUM_COMMAND_LINE L"--flag-switches-begin --user-data-dir=..\\profile --no-default-browser-check --flag-switches-end"

typedef struct _BROWSER_INFORMATION
{
	WCHAR args[512];
	WCHAR urls[512];

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
	BOOLEAN is_forcecheck;
	BOOLEAN is_hasurls;
	BOOLEAN is_onlyupdate;
	BOOLEAN is_opennewwindow;
	BOOLEAN is_waitdownloadend;
} BROWSER_INFORMATION, *PBROWSER_INFORMATION;
