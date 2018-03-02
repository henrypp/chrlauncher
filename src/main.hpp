// chrlauncher
// Copyright (c) 2015-2018 Henry++

#ifndef __MAIN_H__
#define __MAIN_H__

#include <windows.h>
#include <commctrl.h>
#include "resource.hpp"
#include "app.hpp"

// config
#define WM_TRAYICON WM_APP + 1
#define UID 2008

// libs
#pragma comment(lib, "version.lib")

struct STATIC_DATA
{
	HANDLE thread = nullptr;
	HANDLE end_evt = nullptr;
	HANDLE stop_evt = nullptr;
	HANDLE check_evt = nullptr;
	HANDLE download_evt = nullptr;

	bool is_autodownload = false;
	bool is_bringtofront = false;
	bool is_forcecheck = false;
	bool is_waitdownloadend = false;

	bool is_isdownloading = false;
	bool is_isinstalling = false;

	bool is_isdownloaded = false;
	bool is_isinstalled = false;

	UINT architecture = 0;

	WCHAR current_version[32] = {0};

	WCHAR name[64] = {0};
	WCHAR name_full[64] = {0};
	WCHAR type[64] = {0};

	WCHAR cache_path[512] = {0};
	WCHAR binary_dir[512] = {0};
	WCHAR binary_path[512] = {0};
	WCHAR download_url[512] = {0};

	WCHAR urls[1024] = {0};

	WCHAR args[2048] = {0};
};

#endif // __MAIN_H__
