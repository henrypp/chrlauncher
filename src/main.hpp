// chrlauncher
// Copyright (c) 2015-2020 Henry++

#ifndef __MAIN_H__
#define __MAIN_H__

#include <windows.h>
#include <commctrl.h>

#include "routine.hpp"
#include "resource.hpp"
#include "app.hpp"

// config
#define UID 2008
#define LANG_MENU 0

#define CHROMIUM_UPDATE_URL L"https://chromium.woolyss.com/api/v3/?os=windows&bit=%d&type=%s&out=string"

// libs
#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "version.lib")

struct BROWSER_INFORMATION
{
	bool is_autodownload = false;
	bool is_bringtofront = false;
	bool is_forcecheck = false;
	bool is_waitdownloadend = false;
	bool is_opennewwindow = false;
	bool is_onlyupdate = false;

	time_t timestamp = 0;

	INT check_period = 0;
	INT architecture = 0;

	WCHAR current_version[32] = {0};
	WCHAR new_version[32] = {0};

	WCHAR name[64] = {0};
	WCHAR name_full[64] = {0};
	WCHAR type[64] = {0};

	WCHAR cache_path[MAX_PATH] = {0};
	WCHAR binary_dir[MAX_PATH] = {0};
	WCHAR binary_path[MAX_PATH] = {0};

	WCHAR download_url[512] = {0};

	WCHAR urls[1024] = {0};

	WCHAR args[2048] = {0};
};

#endif // __MAIN_H__
