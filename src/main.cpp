// chrlauncher
// Copyright (c) 2015-2020 Henry++

#include "routine.hpp"

#include "main.hpp"
#include "rapp.hpp"
#include "unzip.hpp"

#include "CpuArch.h"

#include "7z.h"
#include "7zAlloc.h"
#include "7zBuf.h"
#include "7zCrc.h"
#include "7zFile.h"
#include "7zVersion.h"

#include "resource.hpp"

rapp app;

HANDLE hthread_check = nullptr;
BROWSER_INFORMATION browser_info;

_R_FASTLOCK lock_download;
_R_FASTLOCK lock_thread;

rstring _app_getbinaryversion (LPCWSTR path)
{
	rstring result;

	DWORD verHandle = 0;
	const DWORD verSize = GetFileVersionInfoSize (path, &verHandle);

	if (verSize)
	{
		void* verData = _r_mem_allocex (verSize, 0);

		if (verData)
		{
			if (GetFileVersionInfo (path, verHandle, verSize, verData))
			{
				void* buffer = nullptr;
				UINT size = 0;

				if (VerQueryValue (verData, L"\\", &buffer, &size))
				{
					if (size)
					{
						VS_FIXEDFILEINFO const *verInfo = (VS_FIXEDFILEINFO*)buffer;

						if (verInfo->dwSignature == 0xFEEF04BD)
						{
							// Doesn't matter if you are on 32 bit or 64 bit,
							// DWORD is always 32 bits, so first two revision numbers
							// come from dwFileVersionMS, last two come from dwFileVersionLS

							result.Format (L"%d.%d.%d.%d", (verInfo->dwFileVersionMS >> 16) & 0xFFFF, (verInfo->dwFileVersionMS >> 0) & 0xFFFF, (verInfo->dwFileVersionLS >> 16) & 0xFFFF, (verInfo->dwFileVersionLS >> 0) & 0xFFFF);
						}
					}
				}
			}

			_r_mem_free (verData);
		}
	}

	return result;
}

BOOL CALLBACK activate_browser_window_callback (HWND hwnd, LPARAM lparam)
{
	BROWSER_INFORMATION* pbi = (BROWSER_INFORMATION*)lparam;

	DWORD pid = 0;
	GetWindowThreadProcessId (hwnd, &pid);

	if (GetCurrentProcessId () == pid)
		return TRUE;

	if (!IsWindowVisible (hwnd))
		return TRUE;

	WCHAR buffer[MAX_PATH] = {0};
	DWORD length = _countof (buffer);

	HANDLE hproc = OpenProcess (app.IsVistaOrLater () ? PROCESS_QUERY_LIMITED_INFORMATION : PROCESS_QUERY_INFORMATION, false, pid);

	if (_r_fs_isvalidhandle (hproc))
	{
		const HMODULE hlib = GetModuleHandle (L"kernel32.dll");

		if (hlib)
		{
			using QFPIN = decltype (&QueryFullProcessImageName); // vista+
			const QFPIN _QueryFullProcessImageName = (QFPIN)GetProcAddress (hlib, "QueryFullProcessImageNameW");

			if (_QueryFullProcessImageName)
				_QueryFullProcessImageName (hproc, 0, buffer, &length);
		}

		if (_r_str_isempty (buffer))
		{
			WCHAR path[_R_BYTESIZE_KB] = {0};

			if (GetProcessImageFileName (hproc, path, _countof (path))) // winxp fallback
				_r_str_copy (buffer, length, _r_path_dospathfromnt (path));
		}

		CloseHandle (hproc);
	}

	if (!_r_str_isempty (buffer) && _r_str_compare (buffer, pbi->binary_path, _r_str_length (pbi->binary_path)) == 0)
	{
		_r_wnd_toggle (hwnd, true);
		return FALSE;
	}

	return TRUE;
}

void activate_browser_window (BROWSER_INFORMATION* pbi)
{
	EnumWindows (&activate_browser_window_callback, (LPARAM)pbi);
}

bool path_is_url (LPCWSTR path)
{
	return !PathMatchSpec (path, L"*.ini");

	//if (PathIsURL (path) || PathIsHTMLFile (path))
	//	return true;

	//static LPCWSTR types[] = {
	//	L"application/pdf",
	//	L"image/svg+xml",
	//	L"image/webp",
	//	L"text/html",
	//};

	//for (size_t i = 0; i < _countof (types); i++)
	//{
	//	if (PathIsContentType (path, types[i]))
	//		return true;
	//}

	//return false;
}

void update_browser_info (HWND hwnd, BROWSER_INFORMATION* pbi)
{
	_r_ctrl_settabletext (hwnd, IDC_BROWSER, app.LocaleString (IDS_BROWSER, L":"), IDC_BROWSER_DATA, pbi->name_full);
	_r_ctrl_settabletext (hwnd, IDC_CURRENTVERSION, app.LocaleString (IDS_CURRENTVERSION, L":"), IDC_CURRENTVERSION_DATA, _r_str_isempty (pbi->current_version) ? app.LocaleString (IDS_STATUS_NOTFOUND, nullptr).GetString () : pbi->current_version);
	_r_ctrl_settabletext (hwnd, IDC_VERSION, app.LocaleString (IDS_VERSION, L":"), IDC_VERSION_DATA, _r_str_isempty (pbi->new_version) ? app.LocaleString (IDS_STATUS_NOTFOUND, nullptr).GetString () : pbi->new_version);
	_r_ctrl_settabletext (hwnd, IDC_DATE, app.LocaleString (IDS_DATE, L":"), IDC_DATE_DATA, pbi->timestamp ? _r_fmt_date (pbi->timestamp, FDTF_SHORTDATE | FDTF_SHORTTIME).GetString () : app.LocaleString (IDS_STATUS_NOTFOUND, nullptr).GetString ());
}

void init_browser_info (BROWSER_INFORMATION* pbi)
{
	LPCWSTR binNames[] = {
		L"brave.exe",
		L"firefox.exe",
		L"basilisk.exe",
		L"palemoon.exe",
		L"waterfox.exe",
		L"dragon.exe",
		L"iridium.exe",
		L"iron.exe",
		L"opera.exe",
		L"slimjet.exe",
		L"vivaldi.exe",
		L"chromium.exe",
		L"chrome.exe", // default
	};

	// reset
	pbi->urls[0] = UNICODE_NULL;
	pbi->args[0] = UNICODE_NULL;

	// configure paths
	_r_str_copy (pbi->binary_dir, _countof (pbi->binary_dir), _r_path_expand (app.ConfigGet (L"ChromiumDirectory", L".\\bin")));
	_r_str_printf (pbi->binary_path, _countof (pbi->binary_path), L"%s\\%s", pbi->binary_dir, app.ConfigGet (L"ChromiumBinary", L"chrome.exe").GetString ());

	if (!_r_fs_exists (pbi->binary_path))
	{
		for (size_t i = 0; i < _countof (binNames); i++)
		{
			_r_str_printf (pbi->binary_path, _countof (pbi->binary_path), L"%s\\%s", pbi->binary_dir, binNames[i]);

			if (_r_fs_exists (pbi->binary_path))
				break;
		}

		if (!_r_fs_exists (pbi->binary_path))
			_r_str_printf (pbi->binary_path, _countof (pbi->binary_path), L"%s\\%s", pbi->binary_dir, app.ConfigGet (L"ChromiumBinary", L"chrome.exe").GetString ()); // fallback (use defaults)
	}

	_r_str_copy (pbi->cache_path, _countof (pbi->cache_path), _r_path_expand (_r_fmt (L"%%TEMP%%\\" APP_NAME_SHORT L"_%" PR_SIZE_T L".bin", _r_str_hash (pbi->binary_path))).GetString ());

	// get browser architecture...
	if (_r_sys_validversion (5, 1, 0, VER_EQUAL) || _r_sys_validversion (5, 2, 0, VER_EQUAL))
		pbi->architecture = 32; // winxp supports only 32-bit

	else
		pbi->architecture = app.ConfigGet (L"ChromiumArchitecture", 0).AsInt ();

	if (pbi->architecture != 64 && pbi->architecture != 32)
	{
		pbi->architecture = 0;

		// ...by executable
		if (_r_fs_exists (pbi->binary_path))
		{
			DWORD exe_type = 0;

			if (GetBinaryType (pbi->binary_path, &exe_type))
				pbi->architecture = (exe_type == SCS_64BIT_BINARY) ? 64 : 32;
		}

		// ...by processor architecture
		if (!pbi->architecture)
		{
			SYSTEM_INFO si = {0};
			GetNativeSystemInfo (&si);

			pbi->architecture = (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) ? 64 : 32;
		}
	}

	if (pbi->architecture != 32 && pbi->architecture != 64)
		pbi->architecture = 32; // default architecture

	// set common data
	_r_str_copy (pbi->type, _countof (pbi->type), app.ConfigGet (L"ChromiumType", L"dev-codecs-sync"));
	_r_str_printf (pbi->name_full, _countof (pbi->name_full), L"%s (%d-bit)", pbi->type, pbi->architecture);

	_r_str_copy (pbi->current_version, _countof (pbi->current_version), _app_getbinaryversion (pbi->binary_path));

	_r_str_copy (pbi->args, _countof (pbi->args), app.ConfigGet (L"ChromiumCommandLine", L"--flag-switches-begin --user-data-dir=..\\profile --no-default-browser-check --allow-outdated-plugins --disable-logging --disable-breakpad --flag-switches-end"));

	// parse command line
	{
		INT numargs = 0;
		LPWSTR* arga = CommandLineToArgvW (GetCommandLine (), &numargs);

		if (arga)
		{
			for (INT i = 1; i < numargs; i++)
			{
				if (_r_str_length (arga[i]) < 2)
					continue;

				if (arga[i][0] == L'/')
				{
					if (_r_str_compare (arga[i], L"/a", 2) == 0 || _r_str_compare (arga[i], L"/autodownload", 13) == 0)
					{
						pbi->is_autodownload = true;
					}
					else if (_r_str_compare (arga[i], L"/b", 2) == 0 || _r_str_compare (arga[i], L"/bringtofront", 13) == 0)
					{
						pbi->is_bringtofront = true;
					}
					else if (_r_str_compare (arga[i], L"/f", 2) == 0 || _r_str_compare (arga[i], L"/forcecheck", 11) == 0)
					{
						pbi->is_forcecheck = true;
					}
					else if (_r_str_compare (arga[i], L"/w", 2) == 0 || _r_str_compare (arga[i], L"/wait", 5) == 0)
					{
						pbi->is_waitdownloadend = true;
					}
					else if (_r_str_compare (arga[i], L"/u", 2) == 0 || _r_str_compare (arga[i], L"/update", 7) == 0)
					{
						pbi->is_onlyupdate = true;
					}
				}
				else if (arga[i][0] == L'-')
				{
					if (
						_r_str_compare (arga[i], L"-new-tab", 8) == 0 ||
						_r_str_compare (arga[i], L"-new-window", 11) == 0 ||
						_r_str_compare (arga[i], L"--new-window", 12) == 0 ||
						_r_str_compare (arga[i], L"-new-instance", 13) == 0
						)
					{
						pbi->is_opennewwindow = true;
					}

					// there is Chromium arguments
					_r_str_cat (pbi->args, _countof (pbi->args), L" ");
					_r_str_cat (pbi->args, _countof (pbi->args), arga[i]);
				}
				else if (path_is_url (arga[i]))
				{
					// there is Chromium url
					_r_str_cat (pbi->urls, _countof (pbi->urls), L" \"");
					_r_str_cat (pbi->urls, _countof (pbi->urls), arga[i]);
					_r_str_cat (pbi->urls, _countof (pbi->urls), L"\"");
				}
			}

			SAFE_LOCAL_FREE (arga);
		}
	}

	pbi->check_period = app.ConfigGet (L"ChromiumCheckPeriod", 2).AsInt ();

	if (pbi->check_period == INVALID_INT)
		pbi->is_forcecheck = true;

	// set default config
	if (!pbi->is_autodownload)
		pbi->is_autodownload = app.ConfigGet (L"ChromiumAutoDownload", false).AsBool ();

	if (!pbi->is_bringtofront)
		pbi->is_bringtofront = app.ConfigGet (L"ChromiumBringToFront", true).AsBool ();

	if (!pbi->is_waitdownloadend)
		pbi->is_waitdownloadend = app.ConfigGet (L"ChromiumWaitForDownloadEnd", true).AsBool ();

	if (!pbi->is_onlyupdate)
		pbi->is_onlyupdate = app.ConfigGet (L"ChromiumUpdateOnly", false).AsBool ();

	// rewrite options when update-only mode is enabled
	if (pbi->is_onlyupdate)
	{
		pbi->is_forcecheck = true;
		pbi->is_bringtofront = true;
		pbi->is_waitdownloadend = true;
	}

	// set ppapi info
	{
		const rstring ppapi_path = _r_path_expand (app.ConfigGet (L"FlashPlayerPath", L".\\plugins\\pepflashplayer.dll"));

		if (_r_fs_exists (ppapi_path))
		{
			_r_str_cat (pbi->args, _countof (pbi->args), L" --ppapi-flash-path=\"");
			_r_str_cat (pbi->args, _countof (pbi->args), ppapi_path);
			_r_str_cat (pbi->args, _countof (pbi->args), L"\" --ppapi-flash-version=\"");
			_r_str_cat (pbi->args, _countof (pbi->args), _app_getbinaryversion (ppapi_path));
			_r_str_cat (pbi->args, _countof (pbi->args), L"\"");
		}
	}
}

void _app_setstatus (HWND hwnd, LPCWSTR text, DWORDLONG v, DWORDLONG t)
{
	INT percent = 0;

	if (!v && t)
	{
		_r_status_settext (hwnd, IDC_STATUSBAR, 0, _r_fmt (L"%s 0%%", text));
		_r_tray_setinfo (hwnd, UID, nullptr, !_r_str_isempty (text) ? _r_fmt (L"%s\r\n%s: 0%%", APP_NAME, text) : APP_NAME);
	}
	else if (v && t)
	{
		percent = std::clamp ((INT)_R_PERCENT_OF (v, t), 0, 100);
		//buffer.Format (L"%s %s/%s", text, _r_fmt_size64 (v).GetString (), _r_fmt_size64 (t).GetString ());

		_r_status_settext (hwnd, IDC_STATUSBAR, 0, _r_fmt (L"%s %d%%", text, percent));
		_r_tray_setinfo (hwnd, UID, nullptr, !_r_str_isempty (text) ? _r_fmt (L"%s\r\n%s: %d%%", APP_NAME, text, percent) : APP_NAME);
	}
	else
	{
		_r_status_settext (hwnd, IDC_STATUSBAR, 0, text);
		_r_tray_setinfo (hwnd, UID, nullptr, !_r_str_isempty (text) ? _r_fmt (L"%s\r\n%s", APP_NAME, text) : APP_NAME);
	}

	SendDlgItemMessage (hwnd, IDC_PROGRESS, PBM_SETPOS, (WPARAM)percent, 0);
}

void _app_cleanup (BROWSER_INFORMATION* pbi, LPCWSTR current_version)
{
	WIN32_FIND_DATA wfd = {0};

	const HANDLE hfile = FindFirstFile (_r_fmt (L"%s\\*.manifest", pbi->binary_dir), &wfd);

	if (_r_fs_isvalidhandle (hfile))
	{
		const size_t len = _r_str_length (current_version);

		do
		{
			if (_r_str_compare (current_version, wfd.cFileName, len) != 0)
				_r_fs_remove (_r_fmt (L"%s\\%s", pbi->binary_dir, wfd.cFileName), 0);
		}
		while (FindNextFile (hfile, &wfd));

		FindClose (hfile);
	}
}

bool _app_browserisrunning (BROWSER_INFORMATION* pbi)
{
	const HANDLE hfile = CreateFile (pbi->binary_path, GENERIC_WRITE | GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);

	if (_r_fs_isvalidhandle (hfile))
		CloseHandle (hfile);

	else
		return (GetLastError () == ERROR_SHARING_VIOLATION);

	return false;
}

void _app_openbrowser (BROWSER_INFORMATION* pbi)
{
	if (!_r_fs_exists (pbi->binary_path))
		return;

	const bool is_running = _app_browserisrunning (pbi);

	if (is_running && _r_str_isempty (pbi->urls) && !pbi->is_opennewwindow)
	{
		activate_browser_window (pbi);
		return;
	}

	WCHAR args[2048] = {0};

	if (!ExpandEnvironmentStrings (pbi->args, args, _countof (pbi->args)))
		_r_str_copy (args, _countof (args), pbi->args);

	if (!_r_str_isempty (pbi->urls))
	{
		_r_str_cat (args, _countof (args), pbi->urls);
		pbi->urls[0] = UNICODE_NULL; // reset
	}

	pbi->is_opennewwindow = false;

	rstring arg;
	arg.Format (L"\"%s\" %s", pbi->binary_path, args);

	if (!_r_run (pbi->binary_path, arg, pbi->binary_dir))
		app.ShowErrorMessage (app.GetHWND (), nullptr, GetLastError (), nullptr);
}

bool _app_ishaveupdate (BROWSER_INFORMATION* pbi)
{
	if (!_r_str_isempty (pbi->download_url) && !_r_str_isempty (pbi->new_version))
		return true;

	return false;
}

bool _app_isupdatedownloaded (BROWSER_INFORMATION* pbi)
{
	return _r_fs_exists (pbi->cache_path);
}

bool _app_isupdaterequired (BROWSER_INFORMATION* pbi)
{
	if (pbi->is_forcecheck)
		return true;

	if (pbi->check_period && ((_r_unixtime_now () - app.ConfigGet (L"ChromiumLastCheck", 0LL).AsLonglong ()) >= _R_SECONDSCLOCK_DAY (pbi->check_period)))
		return true;

	return false;
}

UINT _app_getactionid (BROWSER_INFORMATION* pbi)
{
	if (_app_isupdatedownloaded (pbi))
		return IDS_ACTION_INSTALL;

	else if (_app_ishaveupdate (pbi))
		return IDS_ACTION_DOWNLOAD;

	return IDS_ACTION_CHECK;
}

bool _app_checkupdate (HWND hwnd, BROWSER_INFORMATION* pbi, bool *pis_error)
{
	if (_app_ishaveupdate (pbi))
		return true;

	rstringmap1 result;

	const bool is_exists = _r_fs_exists (pbi->binary_path);
	const bool is_checkupdate = _app_isupdaterequired (pbi);

	bool result2 = false;

	_app_setstatus (hwnd, app.LocaleString (IDS_STATUS_CHECK, nullptr), 0, 0);

	pbi->new_version[0] = UNICODE_NULL;
	pbi->timestamp = 0;

	update_browser_info (hwnd, pbi);

	if (!is_exists || is_checkupdate)
	{
		rstring url;
		rstring content;

		url.Format (app.ConfigGet (L"ChromiumUpdateUrl", CHROMIUM_UPDATE_URL), pbi->architecture, pbi->type);

		rstring proxy_addr = app.GetProxyConfiguration ();
		HINTERNET hsession = _r_inet_createsession (app.GetUserAgent (), proxy_addr);

		if (hsession)
		{
			DWORD rc = _r_inet_downloadurl (hsession, proxy_addr, url, (LONG_PTR)&content, false, nullptr, 0);

			if (rc == ERROR_SUCCESS)
			{
				if (!content.IsEmpty ())
					_r_str_unserialize (content, L';', L'=', &result);

				*pis_error = false;
			}
			else
			{
				app.LogError (TEXT (__FUNCTION__), rc, url, UID);

				*pis_error = true;
			}

			_r_inet_close (hsession);
		}
	}

	if (!result.empty ())
	{
		_r_str_copy (pbi->download_url, _countof (pbi->download_url), result[L"download"]);
		_r_str_copy (pbi->new_version, _countof (pbi->new_version), result[L"version"]);

		pbi->timestamp = result[L"timestamp"].AsLonglong ();

		update_browser_info (hwnd, &browser_info);

		if (!is_exists || _r_str_versioncompare (pbi->current_version, pbi->new_version) == -1)
		{
			result2 = true;
		}
		else
		{
			pbi->download_url[0] = UNICODE_NULL; // clear download url if update not found

			app.ConfigSet (L"ChromiumLastCheck", _r_unixtime_now ());
		}
	}

	_app_setstatus (hwnd, nullptr, 0, 0);

	return result2;
}

bool _app_downloadupdate_callback (DWORD total_written, DWORD total_length, LONG_PTR lpdata)
{
	const HWND hwnd = (HWND)lpdata;

	if (lpdata)
		_app_setstatus (hwnd, app.LocaleString (IDS_STATUS_DOWNLOAD, nullptr), total_written, total_length);

	return true;
}

bool _app_downloadupdate (HWND hwnd, BROWSER_INFORMATION* pbi, bool *pis_error)
{
	if (_app_isupdatedownloaded (pbi))
		return true;

	bool result = false;

	WCHAR temp_file[MAX_PATH] = {0};
	_r_str_printf (temp_file, _countof (temp_file), L"%s.tmp", pbi->cache_path);
	_r_str_copy (temp_file, _countof (temp_file), _r_path_makeunique (temp_file));

	_r_fs_remove (pbi->cache_path, RFS_FORCEREMOVE);

	_app_setstatus (hwnd, app.LocaleString (IDS_STATUS_DOWNLOAD, nullptr), 0, 1);

	_r_fastlock_acquireshared (&lock_download);

	rstring proxy_addr = app.GetProxyConfiguration ();
	HINTERNET hsession = _r_inet_createsession (app.GetUserAgent (), proxy_addr);

	if (hsession)
	{
		HANDLE hfile = CreateFile (temp_file, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

		if (!_r_fs_isvalidhandle (hfile))
		{
			app.LogError (L"CreateFile", GetLastError (), temp_file, UID);

			*pis_error = true;
		}
		else
		{
			DWORD rc = _r_inet_downloadurl (hsession, proxy_addr, pbi->download_url, (LONG_PTR)hfile, true, &_app_downloadupdate_callback, (LONG_PTR)hwnd);

			CloseHandle (hfile); // close handle (required!)

			if (rc == ERROR_SUCCESS)
			{
				pbi->download_url[0] = UNICODE_NULL; // clear download url

				_r_fs_move (temp_file, pbi->cache_path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED);

				result = true;

				*pis_error = false;
			}
			else
			{
				app.LogError (TEXT (__FUNCTION__), rc, pbi->download_url, UID);

				_r_fs_remove (pbi->cache_path, RFS_FORCEREMOVE);

				*pis_error = true;
			}

			_r_fs_remove (temp_file, RFS_FORCEREMOVE);
		}

		_r_inet_close (hsession);
	}

	_r_fastlock_releaseshared (&lock_download);

	_app_setstatus (hwnd, nullptr, 0, 0);

	return result;
}

bool _app_unpack_7zip (HWND hwnd, BROWSER_INFORMATION* pbi, LPCWSTR binName)
{
#define kInputBufSize ((size_t)1 << 18)

	static const ISzAlloc g_Alloc = {SzAlloc, SzFree};

	bool result = false;

	ISzAlloc allocImp = g_Alloc;
	ISzAlloc allocTempImp = g_Alloc;

	CFileInStream archiveStream;
	CLookToRead2 lookStream;
	CSzArEx db;
	UInt16 *tempBuf = nullptr;
	size_t tempSize = 0;

	SRes rc = InFile_OpenW (&archiveStream.file, pbi->cache_path);

	if (rc != ERROR_SUCCESS)
	{
		app.LogError (L"InFile_OpenW", rc, pbi->cache_path, 0);
		return false;
	}

	FileInStream_CreateVTable (&archiveStream);
	LookToRead2_CreateVTable (&lookStream, 0);

	lookStream.buf = (byte*)ISzAlloc_Alloc (&allocImp, kInputBufSize);

	if (!lookStream.buf)
	{
		app.LogError (L"ISzAlloc_Alloc", SZ_ERROR_MEM, nullptr, 0);
	}
	else
	{
		lookStream.bufSize = kInputBufSize;
		lookStream.realStream = &archiveStream.vt;
		LookToRead2_Init (&lookStream);

		CrcGenerateTable ();

		SzArEx_Init (&db);

		rc = SzArEx_Open (&db, &lookStream.vt, &allocImp, &allocTempImp);

		if (rc != SZ_OK)
		{
			app.LogError (L"SzArEx_Open", rc, pbi->cache_path, 0);
		}
		else
		{
			/*
				if you need cache, use these 3 variables.
				if you use external function, you can make these variable as static.
			*/

			UInt32 blockIndex = UINT32_MAX; // it can have any value before first call (if outBuffer = 0)
			Byte *outBuffer = nullptr; // it must be 0 before first call for each new archive.
			size_t outBufferSize = 0; // it can have any value before first call (if outBuffer = 0)

			rstring root_dir_name;

			UInt64 total_size = 0;
			UInt64 total_read = 0;

			// find root directory which contains main executable
			for (UInt32 i = 0; i < db.NumFiles; i++)
			{
				if (SzArEx_IsDir (&db, i))
					continue;

				const size_t len = SzArEx_GetFileNameUtf16 (&db, i, nullptr);

				total_size += SzArEx_GetFileSize (&db, i);

				if (len > tempSize)
				{
					SzFree (nullptr, tempBuf);
					tempSize = len;
					tempBuf = (UInt16 *)SzAlloc (nullptr, tempSize * sizeof (UInt16));

					if (!tempBuf)
					{
						app.LogError (L"SzAlloc", SZ_ERROR_MEM, nullptr, 0);
						break;
					}
				}

				if (root_dir_name.IsEmpty () && SzArEx_GetFileNameUtf16 (&db, i, tempBuf))
				{
					LPWSTR buffer = (LPWSTR)tempBuf;

					_r_str_replace (buffer, L'/', OBJ_NAME_PATH_SEPARATOR);

					LPCWSTR file_name = _r_path_getfilename (buffer);

					if (_r_str_isempty (file_name) || _r_str_isempty (binName))
						continue;

					const size_t fname_len = _r_str_length (file_name);

					if (_r_str_compare (file_name, binName, fname_len) == 0)
					{
						const size_t root_dir_len = _r_str_length (buffer) - fname_len;

						root_dir_name = buffer;
						root_dir_name.SetLength (root_dir_len);

						_r_str_trim (root_dir_name, L"\\");
					}
				}
			}

			for (UInt32 i = 0; i < db.NumFiles; i++)
			{
				const size_t len = SzArEx_GetFileNameUtf16 (&db, i, tempBuf);

				if (len)
				{
					tempBuf[len - 1] = UNICODE_NULL;

					LPWSTR file_name = (LPWSTR)tempBuf;

					_r_str_replace (file_name, L'/', OBJ_NAME_PATH_SEPARATOR);

					rstring dir_name = _r_path_getdirectory (file_name);

					// skip non-root dirs
					if (!root_dir_name.IsEmpty () && ((len - 1) <= root_dir_name.GetLength () || _r_str_compare (root_dir_name, dir_name, root_dir_name.GetLength ()) != 0))
						continue;

					CSzFile outFile;

					rstring dest_path;
					dest_path.Format (L"%s\\%s", pbi->binary_dir, file_name + root_dir_name.GetLength ());

					if (SzArEx_IsDir (&db, i))
					{
						_r_fs_mkdir (dest_path);
					}
					else
					{
						total_read += SzArEx_GetFileSize (&db, i);

						_app_setstatus (hwnd, app.LocaleString (IDS_STATUS_INSTALL, nullptr), total_read, total_size);

						// create directory if not-exist
						{
							rstring sub_dir = _r_path_getdirectory (dest_path);

							if (!_r_fs_exists (sub_dir))
								_r_fs_mkdir (sub_dir);
						}

						size_t offset = 0;
						size_t outSizeProcessed = 0;

						rc = SzArEx_Extract (&db, &lookStream.vt, i, &blockIndex, &outBuffer, &outBufferSize, &offset, &outSizeProcessed, &allocImp, &allocTempImp);

						if (rc != SZ_OK)
						{
							app.LogError (L"SzArEx_Extract", rc, dest_path, 0);
						}
						else
						{
							rc = OutFile_OpenW (&outFile, dest_path);

							if (rc != SZ_OK)
							{
								app.LogError (L"OutFile_OpenW", rc, dest_path, 0);
							}
							else
							{
								size_t processedSize = outSizeProcessed;

								rc = File_Write (&outFile, outBuffer + offset, &processedSize);

								if (rc != SZ_OK || processedSize != outSizeProcessed)
								{
									app.LogError (L"File_Write", rc, dest_path, 0);
								}
								else
								{
									if (SzBitWithVals_Check (&db.Attribs, i))
									{
										UInt32 attrib = db.Attribs.Vals[i];

										/*
											p7zip stores posix attributes in high 16 bits and adds 0x8000 as marker.
											We remove posix bits, if we detect posix mode field
										*/

										if ((attrib & 0xF0000000) != 0)
											attrib &= 0x7FFF;

										SetFileAttributes (dest_path, attrib);
									}
								}

								File_Close (&outFile);
							}
						}
					}

					if (!result)
						result = true;
				}
			}

			ISzAlloc_Free (&allocImp, outBuffer);
		}
	}

	SzFree (nullptr, tempBuf);
	SzArEx_Free (&db, &allocImp);

	ISzAlloc_Free (&allocImp, lookStream.buf);

	File_Close (&archiveStream.file);

	return result;
}

bool _app_unpack_zip (HWND hwnd, BROWSER_INFORMATION* pbi, LPCWSTR binName)
{
	bool result = false;

	ZIPENTRY ze = {0};

	const HZIP hzip = OpenZip (pbi->cache_path, nullptr);

	if (!IsZipHandleU (hzip))
	{
		app.LogError (L"OpenZip", GetLastError (), pbi->cache_path, 0);
	}
	else
	{
		INT total_files = 0;
		ULONG64 total_size = 0;
		ULONG64 total_read = 0; // this is our progress so far

		// count total files
		if (GetZipItem (hzip, INVALID_INT, &ze) == ZR_OK)
			total_files = ze.index;

		rstring root_dir_name;

		// find root directory which contains main executable
		for (INT i = 0; i < total_files; i++)
		{
			if (GetZipItem (hzip, i, &ze) != ZR_OK)
				continue;

			if (((ze.attr & FILE_ATTRIBUTE_DIRECTORY) != 0))
				continue;

			// count total size of unpacked files
			total_size += ze.unc_size;

			if (root_dir_name.IsEmpty () && !_r_str_isempty (binName))
			{
				const size_t len_path = _r_str_length (ze.name);
				_r_str_replace (ze.name, L'/', OBJ_NAME_PATH_SEPARATOR);

				LPCWSTR fname = _r_path_getfilename (ze.name);

				if (!fname)
					continue;

				const size_t len_name = _r_str_length (fname);

				if (_r_str_compare (fname, binName, len_name) == 0)
				{
					const size_t root_dir_len = len_path - len_name;

					root_dir_name = ze.name;
					root_dir_name.SetLength (root_dir_len);

					_r_str_trim (root_dir_name, L"\\");
				}
			}
		}

		CHAR buffer[_R_BUFFER_NET_LENGTH] = {0};
		DWORD written = 0;

		for (INT i = 0; i < total_files; i++)
		{
			if (GetZipItem (hzip, i, &ze) != ZR_OK)
				continue;

			const size_t len = _r_str_length (ze.name);
			_r_str_replace (ze.name, L'/', OBJ_NAME_PATH_SEPARATOR);

			rstring dir_name = _r_path_getdirectory (ze.name);

			// skip non-root dirs
			if (!root_dir_name.IsEmpty () && (len <= root_dir_name.GetLength () || _r_str_compare (root_dir_name, dir_name, root_dir_name.GetLength ()) != 0))
				continue;

			rstring dest_path;
			dest_path.Format (L"%s\\%s", pbi->binary_dir, ze.name + root_dir_name.GetLength ());

			_app_setstatus (hwnd, app.LocaleString (IDS_STATUS_INSTALL, nullptr), total_read, total_size);

			if ((ze.attr & FILE_ATTRIBUTE_DIRECTORY) != 0)
			{
				_r_fs_mkdir (dest_path);
			}
			else
			{
				{
					rstring sub_dir = _r_path_getdirectory (dest_path);

					if (!_r_fs_exists (sub_dir))
						_r_fs_mkdir (sub_dir);
				}

				const HANDLE hfile = CreateFile (dest_path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

				if (!_r_fs_isvalidhandle (hfile))
				{
					app.LogError (L"CreateFile", GetLastError (), dest_path, 0);
				}
				else
				{
					DWORD total_read_file = 0;

					for (ZRESULT zr = ZR_MORE; zr == ZR_MORE;)
					{
						DWORD bufsize = _R_BUFFER_NET_LENGTH;

						zr = UnzipItem (hzip, i, buffer, bufsize);

						if (zr == ZR_OK)
							bufsize = ze.unc_size - total_read_file;

						buffer[bufsize] = 0;

						WriteFile (hfile, buffer, bufsize, &written, nullptr);

						total_read_file += bufsize;
						total_read += bufsize;
					}

					CloseHandle (hfile);

					SetFileAttributes (dest_path, ze.attr);
				}
			}

			if (!result)
				result = true;
		}

		CloseZip (hzip);
	}

	return result;
}

bool _app_installupdate (HWND hwnd, BROWSER_INFORMATION* pbi, bool *pis_error)
{
	_r_fastlock_acquireshared (&lock_download);

	if (!_r_fs_exists (pbi->binary_dir))
		_r_fs_mkdir (pbi->binary_dir);

	bool result = false;

	LPCWSTR binName = _r_path_getfilename (pbi->binary_path);

	const HZIP hzip = OpenZip (pbi->cache_path, nullptr);

	if (IsZipHandleU (hzip))
	{
		CloseZip (hzip);

		result = _app_unpack_zip (hwnd, pbi, binName);
	}
	else
	{
		result = _app_unpack_7zip (hwnd, pbi, binName);
	}

	if (result)
	{
		_r_str_copy (pbi->current_version, _countof (pbi->current_version), _app_getbinaryversion (pbi->binary_path));

		_app_cleanup (pbi, _app_getbinaryversion (pbi->binary_path));
	}
	else
	{
		app.LogError (TEXT (__FUNCTION__), GetLastError (), pbi->cache_path, 0);

		_r_fs_remove (pbi->binary_dir, RFS_FORCEREMOVE); // no recurse
	}

	_r_fs_remove (pbi->cache_path, RFS_FORCEREMOVE); // remove cache file when zip cannot be opened

	*pis_error = !result;

	_r_fastlock_releaseshared (&lock_download);

	_app_setstatus (hwnd, nullptr, 0, 0);

	return result;
}

THREAD_FN _app_thread_check (LPVOID lparam)
{
	BROWSER_INFORMATION* pbi = (BROWSER_INFORMATION*)lparam;
	const HWND hwnd = app.GetHWND ();

	bool is_haveerror = false;
	bool is_stayopen = false;
	bool is_installed = false;

	_r_fastlock_acquireshared (&lock_thread);

	_r_ctrl_enable (hwnd, IDC_START_BTN, false);

	_r_progress_setmarquee (hwnd, IDC_PROGRESS, true);

	_r_ctrl_settext (hwnd, IDC_START_BTN, app.LocaleString (_app_getactionid (pbi), nullptr));

	// unpack downloaded package
	if (_app_isupdatedownloaded (pbi))
	{
		_r_progress_setmarquee (hwnd, IDC_PROGRESS, false);

		_r_tray_toggle (hwnd, UID, true); // show tray icon

		if (!_app_browserisrunning (pbi))
		{
			if (pbi->is_bringtofront)
				_r_wnd_toggle (hwnd, true); // show window

			if (_app_installupdate (hwnd, pbi, &is_haveerror))
			{
				init_browser_info (pbi);
				update_browser_info (hwnd, pbi);

				app.ConfigSet (L"ChromiumLastCheck", _r_unixtime_now ());

				is_installed = true;
			}
		}
		else
		{
			_r_ctrl_enable (hwnd, IDC_START_BTN, true);
			is_stayopen = true;
		}
	}

	// check/download/unpack
	if (!is_installed && !is_stayopen)
	{
		_r_progress_setmarquee (hwnd, IDC_PROGRESS, true);

		const bool is_exists = _r_fs_exists (pbi->binary_path);
		const bool is_checkupdate = _app_isupdaterequired (pbi);

		// show launcher gui
		if (!is_exists || pbi->is_onlyupdate || pbi->is_bringtofront)
		{
			_r_tray_toggle (hwnd, UID, true); // show tray icon
			_r_wnd_toggle (hwnd, true);
		}

		if (is_exists && !pbi->is_waitdownloadend && !pbi->is_onlyupdate)
			_app_openbrowser (pbi);

		if (_app_checkupdate (hwnd, pbi, &is_haveerror))
		{
			_r_tray_toggle (hwnd, UID, true); // show tray icon

			if ((!is_exists || pbi->is_autodownload) || (!pbi->is_autodownload && _app_ishaveupdate (pbi)))
			{
				if (pbi->is_bringtofront)
					_r_wnd_toggle (hwnd, true); // show window

				if (is_exists && !pbi->is_onlyupdate && !pbi->is_waitdownloadend && !_app_isupdatedownloaded (pbi))
					_app_openbrowser (pbi);

				_r_progress_setmarquee (hwnd, IDC_PROGRESS, false);

				if (_app_downloadupdate (hwnd, pbi, &is_haveerror))
				{
					if (!_app_browserisrunning (pbi))
					{
						_r_ctrl_enable (hwnd, IDC_START_BTN, false);

						if (_app_installupdate (hwnd, pbi, &is_haveerror))
							app.ConfigSet (L"ChromiumLastCheck", _r_unixtime_now ());
					}
					else
					{
						_r_tray_popup (hwnd, UID, NIIF_INFO, APP_NAME, app.LocaleString (IDS_STATUS_DOWNLOADED, nullptr)); // inform user

						_r_ctrl_settext (hwnd, IDC_START_BTN, app.LocaleString (_app_getactionid (pbi), nullptr));
						_r_ctrl_enable (hwnd, IDC_START_BTN, true);

						is_stayopen = true;
					}
				}
				else
				{
					_r_tray_popup (hwnd, UID, NIIF_INFO, APP_NAME, _r_fmt (app.LocaleString (IDS_STATUS_FOUND, nullptr), pbi->new_version)); // just inform user

					_r_ctrl_settext (hwnd, IDC_START_BTN, app.LocaleString (_app_getactionid (pbi), nullptr));
					_r_ctrl_enable (hwnd, IDC_START_BTN, true);

					is_stayopen = true;
				}
			}

			if (!pbi->is_autodownload && !_app_isupdatedownloaded (pbi))
			{
				_r_tray_popup (hwnd, UID, NIIF_INFO, APP_NAME, _r_fmt (app.LocaleString (IDS_STATUS_FOUND, nullptr), pbi->new_version)); // just inform user

				_r_ctrl_settext (hwnd, IDC_START_BTN, app.LocaleString (_app_getactionid (pbi), nullptr));
				_r_ctrl_enable (hwnd, IDC_START_BTN, true);

				is_stayopen = true;
			}
		}
	}

	_r_progress_setmarquee (hwnd, IDC_PROGRESS, false);

	if (is_haveerror || pbi->is_onlyupdate)
	{
		_r_ctrl_settext (hwnd, IDC_START_BTN, app.LocaleString (_app_getactionid (pbi), nullptr));
		_r_ctrl_enable (hwnd, IDC_START_BTN, true);

		if (is_haveerror)
		{
			_r_tray_popup (hwnd, UID, NIIF_INFO, APP_NAME, app.LocaleString (IDS_STATUS_ERROR, nullptr)); // just inform user
			_app_setstatus (hwnd, app.LocaleString (IDS_STATUS_ERROR, nullptr), 0, 0);
		}

		is_stayopen = true;
	}

	if (!pbi->is_onlyupdate)
		_app_openbrowser (pbi);

	_r_fastlock_releaseshared (&lock_thread);

	if (is_stayopen)
	{
		update_browser_info (hwnd, pbi);
	}
	else
	{
		PostMessage (hwnd, WM_DESTROY, 0, 0);
	}

	return _r_sys_endthread (ERROR_SUCCESS);
}

INT_PTR CALLBACK DlgProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			_r_fastlock_initialize (&lock_download);
			_r_fastlock_initialize (&lock_thread);

#ifndef _APP_NO_DARKTHEME
			_r_wnd_setdarktheme (hwnd);
#endif // _APP_NO_DARKTHEME

			const HWND htip = _r_ctrl_createtip (hwnd);

			if (htip)
			{
				_r_ctrl_settip (htip, hwnd, IDC_BROWSER_DATA, LPSTR_TEXTCALLBACK);
				_r_ctrl_settip (htip, hwnd, IDC_CURRENTVERSION_DATA, LPSTR_TEXTCALLBACK);
				_r_ctrl_settip (htip, hwnd, IDC_VERSION_DATA, LPSTR_TEXTCALLBACK);
				_r_ctrl_settip (htip, hwnd, IDC_DATE_DATA, LPSTR_TEXTCALLBACK);
			}

			break;
		}

		case WM_NCCREATE:
		{
			_r_wnd_enablenonclientscaling (hwnd);
			break;
		}

		case RM_INITIALIZE:
		{
			SetCurrentDirectory (app.GetDirectory ());

			init_browser_info (&browser_info);

			_r_tray_create (hwnd, UID, WM_TRAYICON, app.GetSharedImage (app.GetHINSTANCE (), IDI_MAIN, _r_dc_getsystemmetrics (hwnd, SM_CXSMICON)), APP_NAME, (_r_fastlock_islocked (&lock_download) || _app_isupdatedownloaded (&browser_info)) ? false : true);

			if (!hthread_check || WaitForSingleObject (hthread_check, 0) == WAIT_OBJECT_0)
			{
				SAFE_DELETE_HANDLE (hthread_check);

				hthread_check = _r_sys_createthread (&_app_thread_check, &browser_info, false, THREAD_PRIORITY_ABOVE_NORMAL);
			}

			if (!browser_info.is_waitdownloadend && !browser_info.is_onlyupdate && _r_fs_exists (browser_info.binary_path) && !_app_isupdatedownloaded (&browser_info))
				_app_openbrowser (&browser_info);

			break;
		}

		case RM_UNINITIALIZE:
		{
			_r_tray_destroy (hwnd, UID);
			break;
		}

		case RM_LOCALIZE:
		{
			// localize menu
			const HMENU hmenu = GetMenu (hwnd);

			app.LocaleMenu (hmenu, IDS_FILE, 0, true, nullptr);
			app.LocaleMenu (hmenu, IDS_RUN, IDM_RUN, false, _r_fmt (L" \"%s\"", _r_path_getfilename (browser_info.binary_path)));
			app.LocaleMenu (hmenu, IDS_OPEN, IDM_OPEN, false, _r_fmt (L" \"%s\"\tF2", _r_path_getfilename (app.GetConfigPath ())));
			app.LocaleMenu (hmenu, IDS_EXIT, IDM_EXIT, false, nullptr);
			app.LocaleMenu (hmenu, IDS_SETTINGS, 1, true, nullptr);
			app.LocaleMenu (GetSubMenu (hmenu, 1), IDS_LANGUAGE, LANG_MENU, true, L" (Language)");
			app.LocaleMenu (hmenu, IDS_HELP, 2, true, nullptr);
			app.LocaleMenu (hmenu, IDS_WEBSITE, IDM_WEBSITE, false, nullptr);
			app.LocaleMenu (hmenu, IDS_ABOUT, IDM_ABOUT, false, L"\tF1");

			app.LocaleEnum ((HWND)GetSubMenu (hmenu, 1), LANG_MENU, true, IDX_LANGUAGE); // enum localizations

			update_browser_info (hwnd, &browser_info);

			SetDlgItemText (hwnd, IDC_LINKS, L"<a href=\"https://github.com/henrypp\">github.com/henrypp</a>\r\n<a href=\"https://chromium.woolyss.com\">chromium.woolyss.com</a>");

			SetDlgItemText (hwnd, IDC_START_BTN, app.LocaleString (_app_getactionid (&browser_info), nullptr));

			_r_wnd_addstyle (hwnd, IDC_START_BTN, app.IsClassicUI () ? WS_EX_STATICEDGE : 0, WS_EX_STATICEDGE, GWL_EXSTYLE);

			break;
		}

		case RM_TASKBARCREATED:
		{
			_r_tray_destroy (hwnd, UID);
			_r_tray_create (hwnd, UID, WM_TRAYICON, app.GetSharedImage (app.GetHINSTANCE (), IDI_MAIN, _r_dc_getsystemmetrics (hwnd, SM_CXSMICON)), APP_NAME, (_r_fastlock_islocked (&lock_download) || _app_isupdatedownloaded (&browser_info)) ? false : true);

			break;
		}

		case RM_DPICHANGED:
		{
			_r_tray_setinfo (hwnd, UID, app.GetSharedImage (app.GetHINSTANCE (), IDI_MAIN, _r_dc_getsystemmetrics (hwnd, SM_CXSMICON)), APP_NAME);

			SendDlgItemMessage (hwnd, IDC_STATUSBAR, WM_SIZE, 0, 0);

			break;
		}

		case WM_CLOSE:
		{
			if (_r_fastlock_islocked (&lock_download) && app.ShowMessage (hwnd, MB_YESNO | MB_ICONQUESTION, nullptr, nullptr, app.LocaleString (IDS_QUESTION_STOP, nullptr)) != IDYES)
			{
				SetWindowLongPtr (hwnd, DWLP_MSGRESULT, TRUE);
				return TRUE;
			}

			DestroyWindow (hwnd);

			break;
		}

		case WM_DESTROY:
		{
			_r_tray_destroy (hwnd, UID);

			if (browser_info.is_waitdownloadend && !browser_info.is_onlyupdate)
				_app_openbrowser (&browser_info);

			if (_r_fastlock_islocked (&lock_download))
				WaitForSingleObjectEx (hthread_check, 7500, FALSE);

			SAFE_DELETE_HANDLE (hthread_check);

			PostQuitMessage (0);

			break;
		}

		case WM_LBUTTONDOWN:
		{
			PostMessage (hwnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, 0);
			break;
		}

		case WM_ENTERSIZEMOVE:
		case WM_EXITSIZEMOVE:
		case WM_CAPTURECHANGED:
		{
			LONG_PTR exstyle = GetWindowLongPtr (hwnd, GWL_EXSTYLE);

			if ((exstyle & WS_EX_LAYERED) == 0)
				SetWindowLongPtr (hwnd, GWL_EXSTYLE, exstyle | WS_EX_LAYERED);

			SetLayeredWindowAttributes (hwnd, 0, (msg == WM_ENTERSIZEMOVE) ? 100 : 255, LWA_ALPHA);
			SetCursor (LoadCursor (nullptr, (msg == WM_ENTERSIZEMOVE) ? IDC_SIZEALL : IDC_ARROW));

			break;
		}

		case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT drawInfo = (LPDRAWITEMSTRUCT)lparam;

			if (drawInfo->CtlID != IDC_LINE)
				break;

			HDC bufferDc = CreateCompatibleDC (drawInfo->hDC);

			if (bufferDc)
			{
				HBITMAP bufferBitmap = CreateCompatibleBitmap (drawInfo->hDC, _R_RECT_WIDTH (&drawInfo->rcItem), _R_RECT_HEIGHT (&drawInfo->rcItem));

				if (bufferBitmap)
				{
					HGDIOBJ oldBufferBitmap = SelectObject (bufferDc, bufferBitmap);

					SetBkMode (bufferDc, TRANSPARENT);

					_r_dc_fillrect (bufferDc, &drawInfo->rcItem, GetSysColor (COLOR_WINDOW));

					for (INT i = drawInfo->rcItem.left; i < _R_RECT_WIDTH (&drawInfo->rcItem); i++)
						SetPixelV (bufferDc, i, drawInfo->rcItem.top, GetSysColor (COLOR_APPWORKSPACE));

					BitBlt (drawInfo->hDC, drawInfo->rcItem.left, drawInfo->rcItem.top, drawInfo->rcItem.right, drawInfo->rcItem.bottom, bufferDc, 0, 0, SRCCOPY);

					SelectObject (bufferDc, oldBufferBitmap);

					SAFE_DELETE_OBJECT (bufferBitmap);
				}

				SAFE_DELETE_DC (bufferDc);
			}

			SetWindowLongPtr (hwnd, DWLP_MSGRESULT, TRUE);
			return TRUE;
		}

		case WM_NOTIFY:
		{
			switch (LPNMHDR (lparam)->code)
			{
				case TTN_GETDISPINFO:
				{
					LPNMTTDISPINFO lpnmdi = (LPNMTTDISPINFO)lparam;

					if ((lpnmdi->uFlags & TTF_IDISHWND) == 0)
						break;

					WCHAR buffer[1024] = {0};
					const INT ctrl_id = GetDlgCtrlID ((HWND)lpnmdi->hdr.idFrom);

					_r_str_copy (buffer, _countof (buffer), _r_ctrl_gettext (hwnd, ctrl_id));

					if (!_r_str_isempty (buffer))
						lpnmdi->lpszText = buffer;

					break;
				}

				case NM_CLICK:
				case NM_RETURN:
				{
					PNMLINK pnmlink = (PNMLINK)lparam;

					if (!_r_str_isempty (pnmlink->item.szUrl))
						ShellExecute (nullptr, nullptr, pnmlink->item.szUrl, nullptr, nullptr, SW_SHOWNORMAL);

					break;
				}
			}

			break;
		}

		case WM_TRAYICON:
		{
			switch (LOWORD (lparam))
			{
				case NIN_BALLOONUSERCLICK:
				{
					_r_wnd_toggle (hwnd, true);
					break;
				}

				case WM_LBUTTONUP:
				{
					SetForegroundWindow (hwnd);
					break;
				}

				case WM_MBUTTONUP:
				{
					SendMessage (hwnd, WM_COMMAND, MAKEWPARAM (IDM_EXPLORE, 0), 0);
					break;
				}

				case WM_LBUTTONDBLCLK:
				{
					_r_wnd_toggle (hwnd, false);
					break;
				}

				case WM_RBUTTONUP:
				{
					SetForegroundWindow (hwnd); // don't touch

					const HMENU hmenu = LoadMenu (nullptr, MAKEINTRESOURCE (IDM_TRAY));
					const HMENU hsubmenu = GetSubMenu (hmenu, 0);

					// localize
					app.LocaleMenu (hsubmenu, IDS_TRAY_SHOW, IDM_TRAY_SHOW, false, nullptr);
					app.LocaleMenu (hsubmenu, IDS_RUN, IDM_TRAY_RUN, false, _r_fmt (L" \"%s\"", _r_path_getfilename (browser_info.binary_path)));
					app.LocaleMenu (hsubmenu, IDS_OPEN, IDM_TRAY_OPEN, false, _r_fmt (L" \"%s\"", _r_path_getfilename (app.GetConfigPath ())));
					app.LocaleMenu (hsubmenu, IDS_WEBSITE, IDM_TRAY_WEBSITE, false, nullptr);
					app.LocaleMenu (hsubmenu, IDS_ABOUT, IDM_TRAY_ABOUT, false, nullptr);
					app.LocaleMenu (hsubmenu, IDS_EXIT, IDM_TRAY_EXIT, false, nullptr);

					if (!_r_fs_exists (browser_info.binary_path))
						EnableMenuItem (hsubmenu, IDM_TRAY_RUN, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

					POINT pt = {0};
					GetCursorPos (&pt);

					TrackPopupMenuEx (hsubmenu, TPM_RIGHTBUTTON | TPM_LEFTBUTTON, pt.x, pt.y, hwnd, nullptr);

					DestroyMenu (hmenu);

					break;
				}
			}

			break;
		}

		case WM_COMMAND:
		{
			if (HIWORD (wparam) == 0 && LOWORD (wparam) >= IDX_LANGUAGE && LOWORD (wparam) <= IDX_LANGUAGE + app.LocaleGetCount ())
			{
				app.LocaleApplyFromMenu (GetSubMenu (GetSubMenu (GetMenu (hwnd), 1), LANG_MENU), LOWORD (wparam), IDX_LANGUAGE);
				return FALSE;
			}

			switch (LOWORD (wparam))
			{
				case IDCANCEL: // process Esc key
				case IDM_TRAY_SHOW:
				{
					_r_wnd_toggle (hwnd, false);
					break;
				}

				case IDM_EXIT:
				case IDM_TRAY_EXIT:
				{
					PostMessage (hwnd, WM_CLOSE, 0, 0);
					break;
				}

				case IDM_RUN:
				case IDM_TRAY_RUN:
				{
					_app_openbrowser (&browser_info);
					break;
				}

				case IDM_OPEN:
				case IDM_TRAY_OPEN:
				{
					if (_r_fs_exists (app.GetConfigPath ()))
						ShellExecute (hwnd, nullptr, app.GetConfigPath (), nullptr, nullptr, SW_SHOWDEFAULT);

					break;
				}

				case IDM_EXPLORE:
				{
					if (_r_fs_exists (browser_info.binary_dir))
						ShellExecute (hwnd, nullptr, browser_info.binary_dir, nullptr, nullptr, SW_SHOWDEFAULT);

					break;
				}

				case IDM_WEBSITE:
				case IDM_TRAY_WEBSITE:
				{
					ShellExecute (hwnd, nullptr, _APP_WEBSITE_URL, nullptr, nullptr, SW_SHOWDEFAULT);
					break;
				}

				case IDM_ABOUT:
				case IDM_TRAY_ABOUT:
				{
					app.CreateAboutWindow (hwnd);
					break;
				}

				case IDC_START_BTN:
				{
					if (!hthread_check || !_r_fastlock_islocked (&lock_thread))
					{
						SAFE_DELETE_HANDLE (hthread_check);

						hthread_check = _r_sys_createthread (&_app_thread_check, &browser_info, false, THREAD_PRIORITY_ABOVE_NORMAL);
					}

					break;
				}
			}

			break;
		}
	}

	return FALSE;
}

INT APIENTRY wWinMain (HINSTANCE, HINSTANCE, LPWSTR args, INT)
{
	MSG msg = {0};

	if (app.Initialize (APP_NAME, APP_NAME_SHORT, APP_VERSION, APP_COPYRIGHT))
	{
		if (args)
		{
			SetCurrentDirectory (app.GetDirectory ());

			init_browser_info (&browser_info);

			if (!_r_str_isempty (browser_info.urls) && _r_fs_exists (browser_info.binary_path))
			{
				_app_openbrowser (&browser_info);

				return ERROR_SUCCESS;
			}
		}

		if (app.CreateMainWindow (IDD_MAIN, IDI_MAIN, &DlgProc))
		{
			const HACCEL haccel = LoadAccelerators (app.GetHINSTANCE (), MAKEINTRESOURCE (IDA_MAIN));

			if (haccel)
			{
				while (GetMessage (&msg, nullptr, 0, 0) > 0)
				{
					TranslateAccelerator (app.GetHWND (), haccel, &msg);

					if (!IsDialogMessage (app.GetHWND (), &msg))
					{
						TranslateMessage (&msg);
						DispatchMessage (&msg);
					}
				}

				DestroyAcceleratorTable (haccel);
			}
		}
	}

	return (INT)msg.wParam;
}
