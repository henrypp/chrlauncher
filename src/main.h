// chrlauncher
// Copyright (c) 2015-2021 Henry++

#pragma once

#include "routine.h"

#include "resource.h"
#include "app.h"

// config
#define UID 2008
#define LANG_MENU 0

#define CHROMIUM_UPDATE_URL L"https://chromium.woolyss.com/api/v3/?os=windows&bit=%d&type=%s&out=string"

typedef struct tagBROWSER_INFORMATION
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

	INT check_period;
	INT architecture;

	BOOLEAN is_autodownload;
	BOOLEAN is_bringtofront;
	BOOLEAN is_forcecheck;
	BOOLEAN is_waitdownloadend;
	BOOLEAN is_opennewwindow;
	BOOLEAN is_onlyupdate;
} BROWSER_INFORMATION, *PBROWSER_INFORMATION;
