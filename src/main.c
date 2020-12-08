// chrlauncher
// Copyright (c) 2015-2021 Henry++

#include "routine.h"

#include "main.h"
#include "rapp.h"

#include "CpuArch.h"

#include "7z.h"
#include "7zAlloc.h"
#include "7zBuf.h"
#include "7zCrc.h"
#include "7zFile.h"
#include "7zVersion.h"

#include "miniz.h"

#include "resource.h"

HANDLE hthread_check = NULL;
PBROWSER_INFORMATION browser_info = NULL;

R_FASTLOCK lock_download;
R_FASTLOCK lock_thread;

PR_STRING _app_getbinaryversion (PR_STRING path)
{
	PR_STRING result = NULL;

	ULONG ver_handle = 0;
	ULONG ver_size = GetFileVersionInfoSize (path->buffer, &ver_handle);

	if (ver_size)
	{
		PVOID ver_data = _r_mem_allocatesafe (ver_size);

		if (ver_data)
		{
			if (GetFileVersionInfo (path->buffer, ver_handle, ver_size, ver_data))
			{
				PVOID buffer = NULL;
				UINT size = 0;

				if (VerQueryValue (ver_data, L"\\", &buffer, &size))
				{
					if (size)
					{
						VS_FIXEDFILEINFO const *verInfo = (VS_FIXEDFILEINFO*)buffer;

						if (verInfo->dwSignature == 0xFEEF04BD)
						{
							// Doesn't matter if you are on 32 bit or 64 bit,
							// DWORD is always 32 bits, so first two revision numbers
							// come from dwFileVersionMS, last two come from dwFileVersionLS

							result = _r_format_string (L"%d.%d.%d.%d", (verInfo->dwFileVersionMS >> 16) & 0xFFFF, (verInfo->dwFileVersionMS >> 0) & 0xFFFF, (verInfo->dwFileVersionLS >> 16) & 0xFFFF, (verInfo->dwFileVersionLS >> 0) & 0xFFFF);
						}
					}
				}
			}

			_r_mem_free (ver_data);
		}
	}

	return result;
}

BOOL CALLBACK activate_browser_window_callback (HWND hwnd, LPARAM lparam)
{
	PBROWSER_INFORMATION pbi = (PBROWSER_INFORMATION)lparam;

	ULONG pid = 0;
	GetWindowThreadProcessId (hwnd, &pid);

	if (GetCurrentProcessId () == pid)
		return TRUE;

	if (!IsWindowVisible (hwnd))
		return TRUE;

	WCHAR path[1024] = {0};
	ULONG length = RTL_NUMBER_OF (path);

	HANDLE hproc = OpenProcess (PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);

	if (_r_fs_isvalidhandle (hproc))
	{
		QueryFullProcessImageName (hproc, 0, path, &length);

		CloseHandle (hproc);
	}

	if (!_r_str_isempty (path) && _r_str_compare_length (path, pbi->binary_path->buffer, _r_obj_getstringlength (pbi->binary_path)) == 0)
	{
		_r_wnd_toggle (hwnd, TRUE);
		return FALSE;
	}

	return TRUE;
}

VOID activate_browser_window (PBROWSER_INFORMATION pbi)
{
	EnumWindows (&activate_browser_window_callback, (LPARAM)pbi);
}

BOOLEAN path_is_url (LPCWSTR path)
{
	if (PathMatchSpec (path, L"*.ini"))
		return FALSE;

	if (PathIsURL (path) || PathIsHTMLFile (path))
		return TRUE;

	static LPCWSTR types[] = {
		L"application/pdf",
		L"image/svg+xml",
		L"image/webp",
		L"text/html",
	};

	for (SIZE_T i = 0; i < RTL_NUMBER_OF (types); i++)
	{
		if (PathIsContentType (path, types[i]))
			return TRUE;
	}

	return FALSE;
}

VOID update_browser_info (HWND hwnd, PBROWSER_INFORMATION pbi)
{
	PR_STRING localized_string = NULL;
	WCHAR date_dormat[128] = {0};
	LPCWSTR empty = _r_locale_getstring (IDS_STATUS_NOTFOUND);

	if (pbi->timestamp)
	{
		_r_format_unixtimeex (date_dormat, RTL_NUMBER_OF (date_dormat), pbi->timestamp, FDTF_SHORTDATE | FDTF_SHORTTIME);
	}

	_r_obj_movereference (&localized_string, _r_format_string (L"%s:", _r_locale_getstring (IDS_BROWSER)));
	_r_ctrl_settabletext (hwnd, IDC_BROWSER, _r_obj_getstringorempty (localized_string), IDC_BROWSER_DATA, _r_obj_getstringordefault (pbi->browser_name, empty));

	_r_obj_movereference (&localized_string, _r_format_string (L"%s:", _r_locale_getstring (IDS_CURRENTVERSION)));
	_r_ctrl_settabletext (hwnd, IDC_CURRENTVERSION, _r_obj_getstringorempty (localized_string), IDC_CURRENTVERSION_DATA, _r_obj_getstringordefault (pbi->current_version, empty));

	_r_obj_movereference (&localized_string, _r_format_string (L"%s:", _r_locale_getstring (IDS_VERSION)));
	_r_ctrl_settabletext (hwnd, IDC_VERSION, _r_obj_getstringorempty (localized_string), IDC_VERSION_DATA, _r_obj_getstringordefault (pbi->new_version, empty));

	_r_obj_movereference (&localized_string, _r_format_string (L"%s:", _r_locale_getstring (IDS_VERSION)));
	_r_ctrl_settabletext (hwnd, IDC_DATE, _r_obj_getstringorempty (localized_string), IDC_DATE_DATA, pbi->timestamp ? date_dormat : empty);

	_r_obj_dereference (localized_string);
}

VOID init_browser_info (PBROWSER_INFORMATION pbi)
{
	LPCWSTR bin_names[] = {
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
	_r_obj_movereference (&pbi->binary_dir, _r_str_expandenvironmentstring (_r_config_getstring (L"ChromiumDirectory", L".\\bin")));
	_r_obj_trimstring (pbi->binary_dir, L"\\");
	_r_obj_movereference (&pbi->binary_path, _r_format_string (L"%s\\%s", pbi->binary_dir->buffer, _r_config_getstring (L"ChromiumBinary", L"chrome.exe")));

	if (pbi->binary_path)
	{
		if (!_r_fs_exists (pbi->binary_path->buffer))
		{
			for (SIZE_T i = 0; i < RTL_NUMBER_OF (bin_names); i++)
			{
				_r_obj_movereference (&pbi->binary_path, _r_format_string (L"%s\\%s", pbi->binary_dir->buffer, bin_names[i]));

				if (_r_fs_exists (pbi->binary_path->buffer))
					break;
			}

			if (_r_obj_isstringempty (pbi->binary_path) || (!_r_fs_exists (pbi->binary_path->buffer)))
				_r_obj_movereference (&pbi->binary_path, _r_format_string (L"%s\\%s", pbi->binary_dir->buffer, _r_config_getstring (L"ChromiumBinary", L"chrome.exe"))); // fallback (use defaults)
		}
	}

	WCHAR path[256];
	_r_str_printf (path, RTL_NUMBER_OF (path), L"%%TEMP%%\\" APP_NAME_SHORT L"_%" TEXT (PR_SIZE_T) L".bin", _r_obj_getstringhash (pbi->binary_path));

	//_r_path_getknownfolder (CSIDL_PERSONAL);
	_r_obj_movereference (&pbi->cache_path, _r_str_expandenvironmentstring (path));

	// get browser architecture...
	pbi->architecture = _r_config_getinteger (L"ChromiumArchitecture", 0);

	if (pbi->architecture != 64 && pbi->architecture != 32)
	{
		pbi->architecture = 0;

		// ...by executable
		if (_r_fs_exists (pbi->binary_path->buffer))
		{
			ULONG binary_type;

			if (GetBinaryType (pbi->binary_path->buffer, &binary_type))
			{
				pbi->architecture = (binary_type == SCS_64BIT_BINARY) ? 64 : 32;
			}
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
	_r_obj_movereference (&pbi->browser_type, _r_obj_createstring (_r_config_getstring (L"ChromiumType", L"dev-codecs-sync")));
	_r_obj_movereference (&pbi->browser_name, _r_format_string (L"%s (%d-bit)", pbi->browser_type->buffer, pbi->architecture));

	_r_obj_movereference (&pbi->current_version, _app_getbinaryversion (pbi->binary_path));

	_r_str_copy (pbi->args, RTL_NUMBER_OF (pbi->args), _r_config_getstring (L"ChromiumCommandLine", L"--flag-switches-begin --user-data-dir=..\\profile --no-default-browser-check --allow-outdated-plugins --disable-logging --disable-breakpad --flag-switches-end"));

	// parse command line
	{
		INT numargs;
		LPWSTR* arga = CommandLineToArgvW (_r_sys_getimagecommandline (), &numargs);

		if (arga)
		{
			for (INT i = 1; i < numargs; i++)
			{
				if (_r_str_length (arga[i]) < 2)
					continue;

				if (arga[i][0] == L'/')
				{
					if (_r_str_compare_length (arga[i], L"/a", 2) == 0 || _r_str_compare_length (arga[i], L"/autodownload", 13) == 0)
					{
						pbi->is_autodownload = TRUE;
					}
					else if (_r_str_compare_length (arga[i], L"/b", 2) == 0 || _r_str_compare_length (arga[i], L"/bringtofront", 13) == 0)
					{
						pbi->is_bringtofront = TRUE;
					}
					else if (_r_str_compare_length (arga[i], L"/f", 2) == 0 || _r_str_compare_length (arga[i], L"/forcecheck", 11) == 0)
					{
						pbi->is_forcecheck = TRUE;
					}
					else if (_r_str_compare_length (arga[i], L"/w", 2) == 0 || _r_str_compare_length (arga[i], L"/wait", 5) == 0)
					{
						pbi->is_waitdownloadend = TRUE;
					}
					else if (_r_str_compare_length (arga[i], L"/u", 2) == 0 || _r_str_compare_length (arga[i], L"/update", 7) == 0)
					{
						pbi->is_onlyupdate = TRUE;
					}
				}
				else if (arga[i][0] == L'-')
				{
					if (
						_r_str_compare_length (arga[i], L"-new-tab", 8) == 0 ||
						_r_str_compare_length (arga[i], L"-new-window", 11) == 0 ||
						_r_str_compare_length (arga[i], L"--new-window", 12) == 0 ||
						_r_str_compare_length (arga[i], L"-new-instance", 13) == 0
						)
					{
						pbi->is_opennewwindow = TRUE;
					}

					// there is Chromium arguments
					_r_str_appendformat (pbi->urls, RTL_NUMBER_OF (pbi->urls), L" %s", arga[i]);
				}
				else if (path_is_url (arga[i]))
				{
					// there is Chromium url
					_r_str_appendformat (pbi->urls, RTL_NUMBER_OF (pbi->urls), L" \"%s\"", arga[i]);
				}
			}

			LocalFree (arga);
		}
	}

	pbi->check_period = _r_config_getinteger (L"ChromiumCheckPeriod", 2);

	if (pbi->check_period == -1)
		pbi->is_forcecheck = TRUE;

	// set default config
	if (!pbi->is_autodownload)
		pbi->is_autodownload = _r_config_getboolean (L"ChromiumAutoDownload", FALSE);

	if (!pbi->is_bringtofront)
		pbi->is_bringtofront = _r_config_getboolean (L"ChromiumBringToFront", TRUE);

	if (!pbi->is_waitdownloadend)
		pbi->is_waitdownloadend = _r_config_getboolean (L"ChromiumWaitForDownloadEnd", TRUE);

	if (!pbi->is_onlyupdate)
		pbi->is_onlyupdate = _r_config_getboolean (L"ChromiumUpdateOnly", FALSE);

	// rewrite options when update-only mode is enabled
	if (pbi->is_onlyupdate)
	{
		pbi->is_forcecheck = TRUE;
		pbi->is_bringtofront = TRUE;
		pbi->is_waitdownloadend = TRUE;
	}
}

VOID _app_setstatus (HWND hwnd, LPCWSTR text, ULONG64 v, ULONG64 t)
{
	LONG64 percent = 0;

	if (!v && t)
	{
		_r_status_settextformat (hwnd, IDC_STATUSBAR, 0, L"%s 0%%", text);

		if (!_r_str_isempty (text))
		{
			_r_tray_setinfoformat (hwnd, UID, NULL, L"%s\r\n%s: 0%%", APP_NAME, text);
		}
		else
		{
			_r_tray_setinfo (hwnd, UID, NULL, APP_NAME);
		}
	}
	else if (v && t)
	{
		percent = _r_calc_clamp64 (_r_calc_percentof64 (v, t), 0, 100);

		_r_status_settextformat (hwnd, IDC_STATUSBAR, 0, L"%s %" TEXT (PR_LONG64) L"%%", text, percent);

		if (!_r_str_isempty (text))
		{
			_r_tray_setinfoformat (hwnd, UID, NULL, L"%s\r\n%s: %" TEXT (PR_LONG64) L"%%", APP_NAME, text, percent);
		}
		else
		{
			_r_tray_setinfo (hwnd, UID, NULL, APP_NAME);
		}
	}
	else
	{
		_r_status_settext (hwnd, IDC_STATUSBAR, 0, text);

		if (!_r_str_isempty (text))
		{
			_r_tray_setinfoformat (hwnd, UID, NULL, L"%s\r\n%s", APP_NAME, text);
		}
		else
		{
			_r_tray_setinfo (hwnd, UID, NULL, APP_NAME);
		}
	}

	SendDlgItemMessage (hwnd, IDC_PROGRESS, PBM_SETPOS, (WPARAM)(LONG)percent, 0);
}

VOID _app_cleanup (PBROWSER_INFORMATION pbi, PR_STRING current_version)
{
	WIN32_FIND_DATA wfd;
	PR_STRING path;
	HANDLE hfile;

	path = _r_format_string (L"%s\\*.manifest", pbi->binary_dir->buffer);
	hfile = FindFirstFile (path->buffer, &wfd);

	if (_r_fs_isvalidhandle (hfile))
	{
		SIZE_T length = _r_obj_getstringlength (current_version);

		do
		{
			if (_r_str_compare_length (current_version->buffer, wfd.cFileName, length) != 0)
			{
				_r_obj_movereference (&path, _r_format_string (L"%s\\%s", pbi->binary_dir->buffer, wfd.cFileName));
				_r_fs_remove (path->buffer, 0);
			}
		}
		while (FindNextFile (hfile, &wfd));

		FindClose (hfile);
	}

	_r_obj_dereference (path);
}

BOOLEAN _app_browserisrunning (PBROWSER_INFORMATION pbi)
{
	HANDLE hfile = CreateFile (pbi->binary_path->buffer, GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

	if (_r_fs_isvalidhandle (hfile))
	{
		CloseHandle (hfile);
	}
	else
	{
		return (GetLastError () == ERROR_SHARING_VIOLATION);
	}

	return FALSE;
}

VOID _app_openbrowser (PBROWSER_INFORMATION pbi)
{
	if (_r_obj_isstringempty (pbi->binary_path) || !_r_fs_exists (pbi->binary_path->buffer))
		return;

	BOOLEAN is_running = _app_browserisrunning (pbi);

	if (is_running && _r_str_isempty (pbi->urls) && !pbi->is_opennewwindow)
	{
		activate_browser_window (pbi);
		return;
	}

	WCHAR args[512];
	PR_STRING expanded_path = _r_str_expandenvironmentstring (pbi->args);

	if (expanded_path)
	{
		_r_str_copy (args, RTL_NUMBER_OF (args), expanded_path->buffer);

		_r_obj_dereference (expanded_path);
	}
	else
	{
		_r_str_copy (args, RTL_NUMBER_OF (args), pbi->args);
	}

	if (!_r_str_isempty (pbi->urls))
	{
		_r_str_append (args, RTL_NUMBER_OF (args), pbi->urls);
		pbi->urls[0] = UNICODE_NULL; // reset
	}

	pbi->is_opennewwindow = FALSE;

	PR_STRING arg;
	arg = _r_format_string (L"\"%s\" %s", pbi->binary_path->buffer, args);

	if (arg)
	{
		if (!_r_sys_createprocess (pbi->binary_path->buffer, _r_obj_getstring (arg), pbi->binary_dir->buffer))
			_r_show_errormessage (_r_app_gethwnd (), NULL, GetLastError (), NULL, NULL);

		_r_obj_dereference (arg);
	}
}

BOOLEAN _app_ishaveupdate (PBROWSER_INFORMATION pbi)
{
	if (!_r_obj_isstringempty (pbi->download_url) && !_r_obj_isstringempty (pbi->new_version))
		return TRUE;

	return FALSE;
}

BOOLEAN _app_isupdatedownloaded (PBROWSER_INFORMATION pbi)
{
	return (!_r_obj_isstringempty (pbi->cache_path) && _r_fs_exists (pbi->cache_path->buffer));
}

BOOLEAN _app_isupdaterequired (PBROWSER_INFORMATION pbi)
{
	if (pbi->is_forcecheck)
		return TRUE;

	if (pbi->check_period && ((_r_unixtime_now () - _r_config_getlong64 (L"ChromiumLastCheck", 0)) >= _r_calc_days2seconds (pbi->check_period)))
		return TRUE;

	return FALSE;
}

UINT _app_getactionid (PBROWSER_INFORMATION pbi)
{
	if (_app_isupdatedownloaded (pbi))
		return IDS_ACTION_INSTALL;

	else if (_app_ishaveupdate (pbi))
		return IDS_ACTION_DOWNLOAD;

	return IDS_ACTION_CHECK;
}

BOOLEAN _app_checkupdate (HWND hwnd, PBROWSER_INFORMATION pbi, PBOOLEAN pis_error)
{
	if (_app_ishaveupdate (pbi))
		return TRUE;

	PR_HASHTABLE result = NULL;

	BOOLEAN is_exists = _r_fs_exists (pbi->binary_path->buffer);
	BOOLEAN is_checkupdate = _app_isupdaterequired (pbi);

	BOOLEAN result2 = FALSE;

	_app_setstatus (hwnd, _r_locale_getstring (IDS_STATUS_CHECK), 0, 0);

	SAFE_DELETE_REFERENCE (pbi->new_version);
	pbi->timestamp = 0;

	update_browser_info (hwnd, pbi);

	if (!is_exists || is_checkupdate)
	{
		R_DOWNLOAD_INFO info = {0};
		PR_STRING url;

		url = _r_format_string (_r_config_getstring (L"ChromiumUpdateUrl", CHROMIUM_UPDATE_URL), pbi->architecture, pbi->browser_type->buffer);

		HINTERNET hsession = _r_inet_createsession (_r_app_getuseragent ());

		if (hsession)
		{
			ULONG code = _r_inet_downloadurl (hsession, _r_obj_getstring (url), &info);

			if (code == ERROR_SUCCESS)
			{
				if (_r_obj_isstringempty (info.string))
				{
					_r_show_message (hwnd, MB_OK | MB_ICONSTOP, NULL, NULL, L"Configuration not found.");
					*pis_error = TRUE;
				}
				else
				{
					result = _r_str_unserialize (info.string, L';', L'=');
					*pis_error = FALSE;
				}

				_r_obj_dereference (info.string);
			}
			else
			{
				_r_log (Error, UID, TEXT (__FUNCTION__), code, _r_obj_getstring (url));

				*pis_error = TRUE;
			}

			_r_inet_close (hsession);
		}

		if (url)
			_r_obj_dereference (url);
	}

	if (!_r_obj_ishashtableempty (result))
	{
		PR_HASHSTORE hashstore;

		hashstore = _r_obj_findhashtable (result, _r_str_hash (L"download"));

		if (hashstore)
			_r_obj_movereference (&pbi->download_url, hashstore->value_string);

		hashstore = _r_obj_findhashtable (result, _r_str_hash (L"version"));

		if (hashstore)
			_r_obj_movereference (&pbi->new_version, hashstore->value_string);

		hashstore = _r_obj_findhashtable (result, _r_str_hash (L"timestamp"));

		if (hashstore)
			pbi->timestamp = _r_str_tolong64 (_r_obj_getstring (hashstore->value_string));

		update_browser_info (hwnd, browser_info);

		if (!is_exists || _r_str_versioncompare (pbi->current_version->buffer, pbi->new_version->buffer) == -1)
		{
			result2 = TRUE;
		}
		else
		{
			SAFE_DELETE_REFERENCE (pbi->download_url); // clear download url if update not found

			_r_config_setlong64 (L"ChromiumLastCheck", _r_unixtime_now ());
		}
	}

	_app_setstatus (hwnd, NULL, 0, 0);

	return result2;
}

BOOLEAN WINAPI _app_downloadupdate_callback (ULONG total_written, ULONG total_length, PVOID pdata)
{
	if (pdata)
		_app_setstatus ((HWND)pdata, _r_locale_getstring (IDS_STATUS_DOWNLOAD), total_written, total_length);

	return TRUE;
}

BOOLEAN _app_downloadupdate (HWND hwnd, PBROWSER_INFORMATION pbi, PBOOLEAN pis_error)
{
	if (_app_isupdatedownloaded (pbi))
		return TRUE;

	BOOLEAN result = FALSE;

	PR_STRING temp_file;
	temp_file = _r_format_string (L"%s.tmp", pbi->cache_path->buffer);

	PR_STRING unique_file;

	unique_file = _r_path_makeunique (temp_file->buffer);

	if (unique_file)
		_r_obj_movereference (&temp_file, unique_file);

	_r_fs_remove (pbi->cache_path->buffer, PR_FLAG_REMOVE_FORCE);

	_app_setstatus (hwnd, _r_locale_getstring (IDS_STATUS_DOWNLOAD), 0, 1);

	_r_fastlock_acquireshared (&lock_download);

	HINTERNET hsession = _r_inet_createsession (_r_app_getuseragent ());

	if (hsession)
	{
		HANDLE hfile = CreateFile (temp_file->buffer, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (!_r_fs_isvalidhandle (hfile))
		{
			_r_log (Error, UID, L"CreateFile", GetLastError (), temp_file->buffer);

			*pis_error = TRUE;
		}
		else
		{
			R_DOWNLOAD_INFO info = {0};

			info.is_filepath = TRUE;
			info.hfile = hfile;
			info._callback = &_app_downloadupdate_callback;
			info.pdata = hwnd;

			ULONG code = _r_inet_downloadurl (hsession, pbi->download_url->buffer, &info);

			CloseHandle (hfile); // close handle (required!)

			if (code == ERROR_SUCCESS)
			{
				SAFE_DELETE_REFERENCE (pbi->download_url); // clear download url

				_r_fs_move (temp_file->buffer, pbi->cache_path->buffer, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED);

				result = TRUE;

				*pis_error = FALSE;
			}
			else
			{
				_r_log (Error, UID, TEXT (__FUNCTION__), code, pbi->download_url->buffer);

				_r_fs_remove (pbi->cache_path->buffer, PR_FLAG_REMOVE_FORCE);

				*pis_error = TRUE;
			}

			_r_fs_remove (temp_file->buffer, PR_FLAG_REMOVE_FORCE);
		}

		_r_inet_close (hsession);
	}

	_r_fastlock_releaseshared (&lock_download);

	_app_setstatus (hwnd, NULL, 0, 0);

	return result;
}

BOOLEAN _app_unpack_7zip (HWND hwnd, PBROWSER_INFORMATION pbi, LPCWSTR bin_name)
{
#define kInputBufSize ((SIZE_T)1 << 18)

	static const ISzAlloc g_Alloc = {SzAlloc, SzFree};

	BOOLEAN result = FALSE;

	ISzAlloc alloc_imp = g_Alloc;
	ISzAlloc alloc_temp_imp = g_Alloc;

	CFileInStream archive_stream;
	CLookToRead2 look_stream;
	CSzArEx db;
	UInt16 *temp_buff = NULL;
	SIZE_T temp_size = 0;

	SRes code = InFile_OpenW (&archive_stream.file, pbi->cache_path->buffer);

	if (code != ERROR_SUCCESS)
	{
		_r_log (Error, 0, L"InFile_OpenW", code, pbi->cache_path->buffer);
		return FALSE;
	}

	FileInStream_CreateVTable (&archive_stream);
	LookToRead2_CreateVTable (&look_stream, 0);

	look_stream.buf = (byte*)ISzAlloc_Alloc (&alloc_imp, kInputBufSize);

	if (!look_stream.buf)
	{
		_r_log (Error, 0, L"ISzAlloc_Alloc", SZ_ERROR_MEM, NULL);
	}
	else
	{
		look_stream.bufSize = kInputBufSize;
		look_stream.realStream = &archive_stream.vt;
		LookToRead2_Init (&look_stream);

		CrcGenerateTable ();

		SzArEx_Init (&db);

		code = SzArEx_Open (&db, &look_stream.vt, &alloc_imp, &alloc_temp_imp);

		if (code != SZ_OK)
		{
			_r_log (Error, 0, L"SzArEx_Open", code, pbi->cache_path->buffer);
		}
		else
		{
			/*
				if you need cache, use these 3 variables.
				if you use external function, you can make these variable as static.
			*/

			UInt32 block_index = UINT32_MAX; // it can have any value before first call (if out_buffer = 0)
			Byte *out_buffer = NULL; // it must be 0 before first call for each new archive.
			size_t out_buffer_size = 0; // it can have any value before first call (if out_buffer = 0)

			PR_STRING root_dir_name = NULL;

			UInt64 total_size = 0;
			UInt64 total_read = 0;

			// find root directory which contains main executable
			for (UInt32 i = 0; i < db.NumFiles; i++)
			{
				if (SzArEx_IsDir (&db, i))
					continue;

				SIZE_T len = SzArEx_GetFileNameUtf16 (&db, i, NULL);

				total_size += SzArEx_GetFileSize (&db, i);

				if (len > temp_size)
				{
					_r_mem_free (temp_buff);
					temp_size = len;
					temp_buff = (UInt16*)_r_mem_allocatesafe (temp_size * sizeof (UInt16));

					if (!temp_buff)
					{
						_r_log (Error, 0, L"SzAlloc", SZ_ERROR_MEM, NULL);
						break;
					}
				}

				if (!root_dir_name && SzArEx_GetFileNameUtf16 (&db, i, temp_buff))
				{
					LPWSTR buffer = (LPWSTR)temp_buff;

					_r_str_replacechar (buffer, _r_str_length (buffer), L'/', OBJ_NAME_PATH_SEPARATOR);

					LPCWSTR file_name = _r_path_getbasename (buffer);

					if (_r_str_isempty (file_name) || _r_str_isempty (bin_name))
						continue;

					SIZE_T fname_len = _r_str_length (file_name);

					if (_r_str_compare_length (file_name, bin_name, fname_len) == 0)
					{
						SIZE_T root_dir_len = _r_str_length (buffer) - fname_len;

						_r_obj_movereference (&root_dir_name, _r_obj_createstringex (buffer, root_dir_len * sizeof (WCHAR)));

						_r_obj_trimstring (root_dir_name, L"\\");
					}
				}
			}

			for (UInt32 i = 0; i < db.NumFiles; i++)
			{
				SIZE_T len = SzArEx_GetFileNameUtf16 (&db, i, temp_buff);

				if (len)
				{
					temp_buff[len - 1] = UNICODE_NULL;

					LPWSTR file_name = (LPWSTR)temp_buff;

					_r_str_replacechar (file_name, _r_str_length (file_name), L'/', OBJ_NAME_PATH_SEPARATOR);

					PR_STRING dir_name = _r_path_getbasedirectory (file_name);

					// skip non-root dirs
					if (!_r_obj_isstringempty (root_dir_name) && ((len - 1) <= _r_obj_getstringlength (root_dir_name) || _r_str_compare_length (root_dir_name->buffer, dir_name->buffer, _r_obj_getstringlength (root_dir_name)) != 0))
						continue;

					CSzFile out_file;

					PR_STRING dest_path = _r_format_string (L"%s\\%s", pbi->binary_dir->buffer, file_name + _r_obj_getstringlength (root_dir_name));

					if (dest_path)
					{
						if (SzArEx_IsDir (&db, i))
						{
							_r_fs_mkdir (dest_path->buffer);
						}
						else
						{
							total_read += SzArEx_GetFileSize (&db, i);

							_app_setstatus (hwnd, _r_locale_getstring (IDS_STATUS_INSTALL), total_read, total_size);

							// create directory if not-exist
							{
								PR_STRING sub_dir = _r_path_getbasedirectory (dest_path->buffer);

								if (sub_dir)
								{
									if (!_r_fs_exists (sub_dir->buffer))
										_r_fs_mkdir (sub_dir->buffer);

									_r_obj_dereference (sub_dir);
								}
							}

							size_t offset = 0;
							size_t out_size_processed = 0;

							code = SzArEx_Extract (&db, &look_stream.vt, i, &block_index, &out_buffer, &out_buffer_size, &offset, &out_size_processed, &alloc_imp, &alloc_temp_imp);

							if (code != SZ_OK)
							{
								_r_log (Error, 0, L"SzArEx_Extract", code, dest_path->buffer);
							}
							else
							{
								code = OutFile_OpenW (&out_file, dest_path->buffer);

								if (code != SZ_OK)
								{
									_r_log (Error, 0, L"OutFile_OpenW", code, dest_path->buffer);
								}
								else
								{
									size_t processed_size = out_size_processed;

									code = File_Write (&out_file, out_buffer + offset, &processed_size);

									if (code != SZ_OK || processed_size != out_size_processed)
									{
										_r_log (Error, 0, L"File_Write", code, dest_path->buffer);
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

											SetFileAttributes (dest_path->buffer, attrib);
										}
									}

									File_Close (&out_file);
								}
							}
						}

						_r_obj_dereference (dest_path);
					}

					if (!result)
						result = TRUE;
				}
			}

			if (root_dir_name)
				_r_obj_dereference (root_dir_name);

			ISzAlloc_Free (&alloc_imp, out_buffer);
		}
	}

	_r_mem_free (temp_buff);
	SzArEx_Free (&db, &alloc_imp);

	ISzAlloc_Free (&alloc_imp, look_stream.buf);

	File_Close (&archive_stream.file);

	return result;
}

BOOLEAN _app_unpack_zip (HWND hwnd, PBROWSER_INFORMATION pbi, LPCWSTR bin_name)
{
	mz_zip_archive zip_archive;
	mz_bool status;

	PR_BYTE bytes = NULL;
	PR_STRING string = NULL;
	PR_STRING root_dir_name = NULL;
	PR_STRING dir_name = NULL;
	BOOLEAN result = FALSE;

	bytes = _r_str_unicode2multibyte (pbi->cache_path->buffer);

	if (!bytes)
	{
		_r_log (Error, 0, L"_r_str_unicode2multibyte", GetLastError (), NULL);

		goto CleanupExit;
	}

	memset (&zip_archive, 0, sizeof (zip_archive));

	status = mz_zip_reader_init_file (&zip_archive, bytes->buffer, 0);

	if (!status)
	{
		_r_log (Error, 0, L"mz_zip_reader_init_file", GetLastError (), NULL);

		goto CleanupExit;
	}

	mz_zip_archive_file_stat file_stat;
	ULONG64 total_size = 0;
	ULONG64 total_read = 0; // this is our progress so far
	SIZE_T len_path;
	INT total_files = mz_zip_reader_get_num_files (&zip_archive);

	// find root directory which contains main executable
	for (INT i = 0; i < total_files; i++)
	{
		if (mz_zip_reader_is_file_a_directory (&zip_archive, i) || !mz_zip_reader_file_stat (&zip_archive, i, &file_stat))
			continue;

		// count total size of unpacked files
		total_size += file_stat.m_uncomp_size;

		if (_r_obj_isstringempty (root_dir_name) && !_r_str_isempty (bin_name))
		{
			_r_obj_movereference (&string, _r_str_multibyte2unicode (file_stat.m_filename));

			if (_r_obj_isstringempty (string))
				continue;

			len_path = _r_obj_getstringlength (string);
			_r_str_replacechar (string->buffer, len_path, L'/', OBJ_NAME_PATH_SEPARATOR);

			LPCWSTR fname = _r_path_getbasename (string->buffer);

			if (!fname)
				continue;

			SIZE_T len_name = _r_str_length (fname);

			if (_r_str_compare_length (fname, bin_name, len_name) == 0)
			{
				SIZE_T root_dir_len = len_path - len_name;

				_r_obj_movereference (&root_dir_name, _r_obj_createstringex (string->buffer, root_dir_len * sizeof (WCHAR)));

				_r_obj_trimstring (root_dir_name, L"\\");
			}
		}
	}

	for (INT i = 0; i < total_files; i++)
	{
		if (!mz_zip_reader_file_stat (&zip_archive, i, &file_stat))
			continue;

		_r_obj_movereference (&string, _r_str_multibyte2unicode (file_stat.m_filename));

		if (_r_obj_isstringempty (string))
			continue;

		SIZE_T len_path = _r_obj_getstringlength (string);
		_r_str_replacechar (string->buffer, len_path, L'/', OBJ_NAME_PATH_SEPARATOR);

		_r_obj_movereference (&dir_name, _r_path_getbasedirectory (string->buffer));

		// skip non-root dirs
		if (!_r_obj_isstringempty (root_dir_name) && (len_path <= _r_obj_getstringlength (root_dir_name) || _r_str_compare_length (root_dir_name->buffer, dir_name->buffer, _r_obj_getstringlength (root_dir_name)) != 0))
			continue;

		PR_STRING dest_path = _r_format_string (L"%s\\%s", pbi->binary_dir->buffer, string->buffer + _r_obj_getstringlength (root_dir_name));

		if (dest_path)
		{
			_app_setstatus (hwnd, _r_locale_getstring (IDS_STATUS_INSTALL), total_read, total_size);

			if (mz_zip_reader_is_file_a_directory (&zip_archive, i))
			{
				_r_fs_mkdir (dest_path->buffer);
			}
			else
			{
				PR_STRING sub_dir = _r_path_getbasedirectory (dest_path->buffer);

				if (sub_dir)
				{
					if (!_r_fs_exists (sub_dir->buffer))
						_r_fs_mkdir (sub_dir->buffer);

					_r_obj_dereference (sub_dir);
				}

				_r_obj_movereference (&bytes, _r_str_unicode2multibyte (dest_path->buffer));

				if (!bytes)
					continue;

				if (!mz_zip_reader_extract_to_file (&zip_archive, i, bytes->buffer, MZ_ZIP_FLAG_DO_NOT_SORT_CENTRAL_DIRECTORY))
				{
					_r_log (Error, 0, L"mz_zip_reader_extract_to_file", GetLastError (), dest_path->buffer);
					continue;
				}

				total_read += file_stat.m_uncomp_size;
			}

			_r_obj_dereference (dest_path);
		}


		if (!result)
			result = TRUE;
	}

CleanupExit:

	if (bytes)
		_r_obj_dereference (bytes);

	if (string)
		_r_obj_dereference (string);

	if (dir_name)
		_r_obj_dereference (dir_name);

	if (root_dir_name)
		_r_obj_dereference (root_dir_name);

	mz_zip_reader_end (&zip_archive);

	return result;
}

BOOLEAN _app_installupdate (HWND hwnd, PBROWSER_INFORMATION pbi, BOOLEAN *pis_error)
{
	_r_fastlock_acquireshared (&lock_download);

	if (!_r_fs_exists (pbi->binary_dir->buffer))
		_r_fs_mkdir (pbi->binary_dir->buffer);

	LPCWSTR bin_name = _r_path_getbasename (pbi->binary_path->buffer);
	BOOLEAN result = FALSE;

	result = _app_unpack_zip (hwnd, pbi, bin_name);

	if (!result)
	{
		result = _app_unpack_7zip (hwnd, pbi, bin_name);
	}

	if (result)
	{
		_r_obj_movereference (&pbi->current_version, _app_getbinaryversion (pbi->binary_path));

		_app_cleanup (pbi, pbi->current_version);
	}
	else
	{
		_r_log (Error, 0, TEXT (__FUNCTION__), GetLastError (), pbi->cache_path->buffer);

		_r_fs_remove (pbi->binary_dir->buffer, PR_FLAG_REMOVE_FORCE); // no recurse
	}

	_r_fs_remove (pbi->cache_path->buffer, PR_FLAG_REMOVE_FORCE); // remove cache file when zip cannot be opened

	*pis_error = !result;

	_r_fastlock_releaseshared (&lock_download);

	_app_setstatus (hwnd, NULL, 0, 0);

	return result;
}

THREAD_API _app_thread_check (PVOID lparam)
{
	PBROWSER_INFORMATION pbi = lparam;
	HWND hwnd = _r_app_gethwnd ();

	BOOLEAN is_haveerror = FALSE;
	BOOLEAN is_stayopen = FALSE;
	BOOLEAN is_installed = FALSE;

	_r_fastlock_acquireshared (&lock_thread);

	_r_ctrl_enable (hwnd, IDC_START_BTN, FALSE);

	_r_progress_setmarquee (hwnd, IDC_PROGRESS, TRUE);

	_r_ctrl_settext (hwnd, IDC_START_BTN, _r_locale_getstring (_app_getactionid (pbi)));

	// unpack downloaded package
	if (_app_isupdatedownloaded (pbi))
	{
		_r_progress_setmarquee (hwnd, IDC_PROGRESS, FALSE);

		_r_tray_toggle (hwnd, UID, TRUE); // show tray icon

		if (!_app_browserisrunning (pbi))
		{
			if (pbi->is_bringtofront)
				_r_wnd_toggle (hwnd, TRUE); // show window

			if (_app_installupdate (hwnd, pbi, &is_haveerror))
			{
				init_browser_info (pbi);
				update_browser_info (hwnd, pbi);

				_r_config_setlong64 (L"ChromiumLastCheck", _r_unixtime_now ());

				is_installed = TRUE;
			}
		}
		else
		{
			_r_ctrl_enable (hwnd, IDC_START_BTN, TRUE);
			is_stayopen = TRUE;
		}
	}

	// check/download/unpack
	if (!is_installed && !is_stayopen)
	{
		_r_progress_setmarquee (hwnd, IDC_PROGRESS, TRUE);

		BOOLEAN is_exists = _r_fs_exists (pbi->binary_path->buffer);
		BOOLEAN is_checkupdate = _app_isupdaterequired (pbi);

		// show launcher gui
		if (!is_exists || pbi->is_onlyupdate || pbi->is_bringtofront)
		{
			_r_tray_toggle (hwnd, UID, TRUE); // show tray icon
			_r_wnd_toggle (hwnd, TRUE);
		}

		if (is_exists && !pbi->is_waitdownloadend && !pbi->is_onlyupdate)
			_app_openbrowser (pbi);

		if (_app_checkupdate (hwnd, pbi, &is_haveerror))
		{
			_r_tray_toggle (hwnd, UID, TRUE); // show tray icon

			if ((!is_exists || pbi->is_autodownload) || (!pbi->is_autodownload && _app_ishaveupdate (pbi)))
			{
				if (pbi->is_bringtofront)
					_r_wnd_toggle (hwnd, TRUE); // show window

				if (is_exists && !pbi->is_onlyupdate && !pbi->is_waitdownloadend && !_app_isupdatedownloaded (pbi))
					_app_openbrowser (pbi);

				_r_progress_setmarquee (hwnd, IDC_PROGRESS, FALSE);

				if (_app_downloadupdate (hwnd, pbi, &is_haveerror))
				{
					if (!_app_browserisrunning (pbi))
					{
						_r_ctrl_enable (hwnd, IDC_START_BTN, FALSE);

						if (_app_installupdate (hwnd, pbi, &is_haveerror))
							_r_config_setlong64 (L"ChromiumLastCheck", _r_unixtime_now ());
					}
					else
					{
						_r_tray_popup (hwnd, UID, NIIF_INFO, APP_NAME, _r_locale_getstring (IDS_STATUS_DOWNLOADED)); // inform user

						_r_ctrl_settext (hwnd, IDC_START_BTN, _r_locale_getstring (_app_getactionid (pbi)));
						_r_ctrl_enable (hwnd, IDC_START_BTN, TRUE);

						is_stayopen = TRUE;
					}
				}
				else
				{
					_r_tray_popupformat (hwnd, UID, NIIF_INFO, APP_NAME, _r_locale_getstring (IDS_STATUS_FOUND), pbi->new_version->buffer); // just inform user

					_r_ctrl_settext (hwnd, IDC_START_BTN, _r_locale_getstring (_app_getactionid (pbi)));
					_r_ctrl_enable (hwnd, IDC_START_BTN, TRUE);

					is_stayopen = TRUE;
				}
			}

			if (!pbi->is_autodownload && !_app_isupdatedownloaded (pbi))
			{
				_r_tray_popupformat (hwnd, UID, NIIF_INFO, APP_NAME, _r_locale_getstring (IDS_STATUS_FOUND), pbi->new_version->buffer); // just inform user

				_r_ctrl_settext (hwnd, IDC_START_BTN, _r_locale_getstring (_app_getactionid (pbi)));
				_r_ctrl_enable (hwnd, IDC_START_BTN, TRUE);

				is_stayopen = TRUE;
			}
		}
	}

	_r_progress_setmarquee (hwnd, IDC_PROGRESS, FALSE);

	if (is_haveerror || pbi->is_onlyupdate)
	{
		_r_ctrl_settext (hwnd, IDC_START_BTN, _r_locale_getstring (_app_getactionid (pbi)));
		_r_ctrl_enable (hwnd, IDC_START_BTN, TRUE);

		if (is_haveerror)
		{
			_r_tray_popup (hwnd, UID, NIIF_INFO, APP_NAME, _r_locale_getstring (IDS_STATUS_ERROR)); // just inform user
			_app_setstatus (hwnd, _r_locale_getstring (IDS_STATUS_ERROR), 0, 0);
		}

		is_stayopen = TRUE;
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

	return ERROR_SUCCESS;
}

VOID _app_initialize ()
{
	_r_fastlock_initialize (&lock_download);
	_r_fastlock_initialize (&lock_thread);

	if (!browser_info)
		browser_info = _r_mem_allocatezero (sizeof (BROWSER_INFORMATION));
}

INT_PTR CALLBACK DlgProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			_app_initialize ();

#ifdef _APP_HAVE_DARKTHEME
			_r_wnd_setdarktheme (hwnd);
#endif // _APP_HAVE_DARKTHEME

			HWND htip = _r_ctrl_createtip (hwnd);

			if (htip)
			{
				_r_ctrl_settiptext (htip, hwnd, IDC_BROWSER_DATA, LPSTR_TEXTCALLBACK);
				_r_ctrl_settiptext (htip, hwnd, IDC_CURRENTVERSION_DATA, LPSTR_TEXTCALLBACK);
				_r_ctrl_settiptext (htip, hwnd, IDC_VERSION_DATA, LPSTR_TEXTCALLBACK);
				_r_ctrl_settiptext (htip, hwnd, IDC_DATE_DATA, LPSTR_TEXTCALLBACK);
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
			SetCurrentDirectory (_r_app_getdirectory ());

			init_browser_info (browser_info);

			_r_tray_create (hwnd, UID, WM_TRAYICON, _r_app_getsharedimage (_r_sys_getimagebase (), IDI_MAIN, _r_dc_getsystemmetrics (hwnd, SM_CXSMICON)), APP_NAME, (_r_fastlock_islocked (&lock_download) || _app_isupdatedownloaded (browser_info)) ? FALSE : TRUE);

			if (!hthread_check || WaitForSingleObject (hthread_check, 0) == WAIT_OBJECT_0)
			{
				if (NT_SUCCESS (_r_sys_createthreadex (&_app_thread_check, browser_info, &hthread_check, THREAD_PRIORITY_ABOVE_NORMAL)))
					_r_sys_resumethread (hthread_check);
			}

			if (!browser_info->is_waitdownloadend && !browser_info->is_onlyupdate && _r_fs_exists (browser_info->binary_path->buffer) && !_app_isupdatedownloaded (browser_info))
				_app_openbrowser (browser_info);

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
			HMENU hmenu = GetMenu (hwnd);

			if (hmenu)
			{
				_r_menu_setitemtext (hmenu, 0, TRUE, _r_locale_getstring (IDS_FILE));
				_r_menu_setitemtext (hmenu, 1, TRUE, _r_locale_getstring (IDS_SETTINGS));
				_r_menu_setitemtextformat (GetSubMenu (hmenu, 1), LANG_MENU, TRUE, L"%s (Language)", _r_locale_getstring (IDS_LANGUAGE));
				_r_menu_setitemtext (hmenu, 2, TRUE, _r_locale_getstring (IDS_HELP));

				_r_menu_setitemtextformat (hmenu, IDM_RUN, FALSE, L"%s \"%s\"", _r_locale_getstring (IDS_RUN), _r_path_getbasename (browser_info->binary_path->buffer));
				_r_menu_setitemtextformat (hmenu, IDM_OPEN, FALSE, L"%s \"%s\"", _r_locale_getstring (IDS_OPEN), _r_path_getbasename (_r_config_getpath ()));
				_r_menu_setitemtext (hmenu, IDM_EXIT, FALSE, _r_locale_getstring (IDS_EXIT));
				_r_menu_setitemtext (hmenu, IDM_WEBSITE, FALSE, _r_locale_getstring (IDS_WEBSITE));
				_r_menu_setitemtextformat (hmenu, IDM_ABOUT, FALSE, L"%s\tF1", _r_locale_getstring (IDS_ABOUT));

				_r_locale_enum ((HWND)GetSubMenu (hmenu, 1), LANG_MENU, IDX_LANGUAGE); // enum localizations
			}

			update_browser_info (hwnd, browser_info);

			SetDlgItemText (hwnd, IDC_LINKS, L"<a href=\"https://github.com/henrypp\">github.com/henrypp</a>\r\n<a href=\"https://chromium.woolyss.com\">chromium.woolyss.com</a>");

			SetDlgItemText (hwnd, IDC_START_BTN, _r_locale_getstring (_app_getactionid (browser_info)));

			_r_wnd_addstyle (hwnd, IDC_START_BTN, _r_app_isclassicui () ? WS_EX_STATICEDGE : 0, WS_EX_STATICEDGE, GWL_EXSTYLE);

			break;
		}

		case RM_TASKBARCREATED:
		{
			_r_tray_destroy (hwnd, UID);
			_r_tray_create (hwnd, UID, WM_TRAYICON, _r_app_getsharedimage (_r_sys_getimagebase (), IDI_MAIN, _r_dc_getsystemmetrics (hwnd, SM_CXSMICON)), APP_NAME, (_r_fastlock_islocked (&lock_download) || _app_isupdatedownloaded (browser_info)) ? FALSE : TRUE);

			break;
		}

		case WM_DPICHANGED:
		{
			_r_tray_setinfo (hwnd, UID, _r_app_getsharedimage (_r_sys_getimagebase (), IDI_MAIN, _r_dc_getsystemmetrics (hwnd, SM_CXSMICON)), APP_NAME);

			SendDlgItemMessage (hwnd, IDC_STATUSBAR, WM_SIZE, 0, 0);

			break;
		}

		case WM_CLOSE:
		{
			if (_r_fastlock_islocked (&lock_download) && _r_show_message (hwnd, MB_YESNO | MB_ICONQUESTION, NULL, NULL, _r_locale_getstring (IDS_QUESTION_STOP)) != IDYES)
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

			if (browser_info->is_waitdownloadend && !browser_info->is_onlyupdate)
				_app_openbrowser (browser_info);

			if (_r_fastlock_islocked (&lock_download))
				WaitForSingleObjectEx (hthread_check, 7500, FALSE);

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
			SetCursor (LoadCursor (NULL, (msg == WM_ENTERSIZEMOVE) ? IDC_SIZEALL : IDC_ARROW));

			break;
		}

		case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT draw_info = (LPDRAWITEMSTRUCT)lparam;

			if (draw_info->CtlID != IDC_LINE)
				break;

			HDC buffer_hdc = CreateCompatibleDC (draw_info->hDC);

			if (buffer_hdc)
			{
				HBITMAP buffer_bitmap = CreateCompatibleBitmap (draw_info->hDC, _r_calc_rectwidth (&draw_info->rcItem), _r_calc_rectheight (&draw_info->rcItem));

				if (buffer_bitmap)
				{
					HGDIOBJ old_buffer_bitmap = SelectObject (buffer_hdc, buffer_bitmap);

					SetBkMode (buffer_hdc, TRANSPARENT);

					_r_dc_fillrect (buffer_hdc, &draw_info->rcItem, GetSysColor (COLOR_WINDOW));

					for (INT i = draw_info->rcItem.left; i < _r_calc_rectwidth (&draw_info->rcItem); i++)
						SetPixelV (buffer_hdc, i, draw_info->rcItem.top, GetSysColor (COLOR_APPWORKSPACE));

					BitBlt (draw_info->hDC, draw_info->rcItem.left, draw_info->rcItem.top, draw_info->rcItem.right, draw_info->rcItem.bottom, buffer_hdc, 0, 0, SRCCOPY);

					SelectObject (buffer_hdc, old_buffer_bitmap);

					SAFE_DELETE_OBJECT (buffer_bitmap);
				}

				SAFE_DELETE_DC (buffer_hdc);
			}

			SetWindowLongPtr (hwnd, DWLP_MSGRESULT, TRUE);
			return TRUE;
		}

		case WM_NOTIFY:
		{
			LPNMHDR lpnmhdr = (LPNMHDR)lparam;

			switch (lpnmhdr->code)
			{
				case TTN_GETDISPINFO:
				{
					WCHAR buffer[1024];
					PR_STRING string;
					INT ctrl_id;

					LPNMTTDISPINFO lpnmdi = (LPNMTTDISPINFO)lparam;

					if ((lpnmdi->uFlags & TTF_IDISHWND) == 0)
						break;

					ctrl_id = GetDlgCtrlID ((HWND)lpnmdi->hdr.idFrom);
					string = _r_ctrl_gettext (hwnd, ctrl_id);

					if (string)
					{
						_r_str_copy (buffer, RTL_NUMBER_OF (buffer), string->buffer);

						if (!_r_str_isempty (buffer))
							lpnmdi->lpszText = buffer;

						_r_obj_dereference (string);
					}

					break;
				}

				case NM_CLICK:
				case NM_RETURN:
				{
					PNMLINK pnmlink = (PNMLINK)lparam;

					if (!_r_str_isempty (pnmlink->item.szUrl))
						ShellExecute (NULL, NULL, pnmlink->item.szUrl, NULL, NULL, SW_SHOWNORMAL);

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
					_r_wnd_toggle (hwnd, TRUE);
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
					_r_wnd_toggle (hwnd, FALSE);
					break;
				}

				case WM_RBUTTONUP:
				{
					SetForegroundWindow (hwnd); // don't touch

					HMENU hmenu = LoadMenu (NULL, MAKEINTRESOURCE (IDM_TRAY));

					if (hmenu)
					{
						HMENU hsubmenu = GetSubMenu (hmenu, 0);

						if (hsubmenu)
						{
							// localize
							_r_menu_setitemtext (hsubmenu, IDM_TRAY_SHOW, FALSE, _r_locale_getstring (IDS_TRAY_SHOW));
							_r_menu_setitemtextformat (hsubmenu, IDM_TRAY_RUN, FALSE, L"%s \"%s\"", _r_locale_getstring (IDS_RUN), _r_path_getbasename (browser_info->binary_path->buffer));
							_r_menu_setitemtextformat (hsubmenu, IDM_TRAY_OPEN, FALSE, L"%s \"%s\"", _r_locale_getstring (IDS_OPEN), _r_path_getbasename (_r_config_getpath ()));
							_r_menu_setitemtext (hsubmenu, IDM_TRAY_WEBSITE, FALSE, _r_locale_getstring (IDS_WEBSITE));
							_r_menu_setitemtext (hsubmenu, IDM_TRAY_ABOUT, FALSE, _r_locale_getstring (IDS_ABOUT));
							_r_menu_setitemtext (hsubmenu, IDM_TRAY_EXIT, FALSE, _r_locale_getstring (IDS_EXIT));

							if (_r_obj_isstringempty (browser_info->binary_path) || !_r_fs_exists (browser_info->binary_path->buffer))
								_r_menu_enableitem (hsubmenu, IDM_TRAY_RUN, MF_BYCOMMAND, FALSE);

							_r_menu_popup (hsubmenu, hwnd, NULL, TRUE);
						}

						DestroyMenu (hmenu);
					}

					break;
				}
			}

			break;
		}

		case WM_COMMAND:
		{
			if (HIWORD (wparam) == 0 && LOWORD (wparam) >= IDX_LANGUAGE && LOWORD (wparam) <= IDX_LANGUAGE + _r_locale_getcount ())
			{
				_r_locale_applyfrommenu (GetSubMenu (GetSubMenu (GetMenu (hwnd), 1), LANG_MENU), LOWORD (wparam));
				return FALSE;
			}

			switch (LOWORD (wparam))
			{
				case IDCANCEL: // process Esc key
				case IDM_TRAY_SHOW:
				{
					_r_wnd_toggle (hwnd, FALSE);
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
					_app_openbrowser (browser_info);
					break;
				}

				case IDM_OPEN:
				case IDM_TRAY_OPEN:
				{
					if (_r_fs_exists (_r_config_getpath ()))
						ShellExecute (hwnd, NULL, _r_config_getpath (), NULL, NULL, SW_SHOWDEFAULT);

					break;
				}

				case IDM_EXPLORE:
				{
					if (browser_info->binary_dir)
					{
						if (_r_fs_exists (browser_info->binary_dir->buffer))
							ShellExecute (hwnd, NULL, browser_info->binary_dir->buffer, NULL, NULL, SW_SHOWDEFAULT);
					}

					break;
				}

				case IDM_WEBSITE:
				case IDM_TRAY_WEBSITE:
				{
					ShellExecute (hwnd, NULL, APP_WEBSITE_URL, NULL, NULL, SW_SHOWDEFAULT);
					break;
				}

				case IDM_ABOUT:
				case IDM_TRAY_ABOUT:
				{
					_r_show_aboutmessage (hwnd);
					break;
				}

				case IDC_START_BTN:
				{
					if (!hthread_check || !_r_fastlock_islocked (&lock_thread))
					{
						if (NT_SUCCESS (_r_sys_createthreadex (&_app_thread_check, browser_info, &hthread_check, THREAD_PRIORITY_ABOVE_NORMAL)))
							_r_sys_resumethread (hthread_check);
					}

					break;
				}
			}

			break;
		}
	}

	return FALSE;
}

INT APIENTRY wWinMain (_In_ HINSTANCE hinst, _In_opt_ HINSTANCE prev_hinst, _In_ LPWSTR cmdline, _In_ INT show_cmd)
{
	MSG msg;

	if (_r_app_initialize (APP_NAME, APP_NAME_SHORT, APP_VERSION, APP_COPYRIGHT))
	{
		if (cmdline)
		{
			_app_initialize ();

			SetCurrentDirectory (_r_app_getdirectory ());

			init_browser_info (browser_info);

			if (!_r_str_isempty (browser_info->urls) && _r_fs_exists (browser_info->binary_path->buffer))
			{
				_app_openbrowser (browser_info);

				return ERROR_SUCCESS;
			}
		}

		if (_r_app_createwindow (IDD_MAIN, IDI_MAIN, &DlgProc))
		{
			HACCEL haccel = LoadAccelerators (_r_sys_getimagebase (), MAKEINTRESOURCE (IDA_MAIN));

			if (haccel)
			{
				while (GetMessage (&msg, NULL, 0, 0) > 0)
				{
					TranslateAccelerator (_r_app_gethwnd (), haccel, &msg);

					if (!IsDialogMessage (_r_app_gethwnd (), &msg))
					{
						TranslateMessage (&msg);
						DispatchMessage (&msg);
					}
				}

				DestroyAcceleratorTable (haccel);
			}
		}
	}

	return ERROR_SUCCESS;
}
