// chrlauncher
// Copyright (c) 2015-2022 Henry++

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

BROWSER_INFORMATION browser_info = {0};

R_QUEUED_LOCK lock_download = PR_QUEUED_LOCK_INIT;
R_QUEUED_LOCK lock_thread = PR_QUEUED_LOCK_INIT;

R_WORKQUEUE workqueue;

BOOL CALLBACK activate_browser_window_callback (
	_In_ HWND hwnd,
	_In_ LPARAM lparam
)
{
	PBROWSER_INFORMATION pbi;
	PR_STRING process_path;
	HANDLE hprocess;
	ULONG pid;
	NTSTATUS status;
	BOOL result;

	GetWindowThreadProcessId (hwnd, &pid);

	if (HandleToUlong (NtCurrentProcessId ()) == pid)
		return TRUE;

	if (!_r_wnd_isvisible (hwnd))
		return TRUE;

	status = _r_sys_openprocess (UlongToHandle (pid), PROCESS_QUERY_LIMITED_INFORMATION, &hprocess);

	if (!NT_SUCCESS (status))
		return TRUE;

	result = TRUE;

	status = _r_sys_queryprocessstring (hprocess, ProcessImageFileNameWin32, (PVOID_PTR)&process_path);

	if (NT_SUCCESS (status))
	{
		pbi = (PBROWSER_INFORMATION)lparam;

		if (_r_str_isequal (&pbi->binary_path->sr, &process_path->sr, TRUE))
		{
			_r_wnd_toggle (hwnd, TRUE);
			result = FALSE;
		}

		_r_obj_dereference (process_path);
	}

	NtClose (hprocess);

	return result;
}

FORCEINLINE VOID activate_browser_window (
	_In_ PBROWSER_INFORMATION pbi
)
{
	EnumWindows (&activate_browser_window_callback, (LPARAM)pbi);
}

BOOLEAN path_is_url (
	_In_ LPCWSTR path
)
{
	static LPCWSTR types[] = {
		L"application/pdf",
		L"image/svg+xml",
		L"image/webp",
		L"text/html",
	};

	if (PathMatchSpec (path, L"*.ini"))
		return FALSE;

	if (PathIsURL (path) || PathIsHTMLFile (path))
		return TRUE;

	for (SIZE_T i = 0; i < RTL_NUMBER_OF (types); i++)
	{
		if (PathIsContentType (path, types[i]))
			return TRUE;
	}

	return FALSE;
}

VOID update_browser_info (
	_In_ HWND hwnd,
	_In_ PBROWSER_INFORMATION pbi
)
{
	PR_STRING date_dormat;
	PR_STRING localized_string;
	R_STRINGREF empty_string;

	_r_obj_initializestringrefconst (&empty_string, _r_locale_getstring (IDS_STATUS_NOTFOUND));

	date_dormat = _r_format_unixtime_ex (pbi->timestamp, FDTF_SHORTDATE | FDTF_SHORTTIME);

	localized_string = _r_format_string (L"%s:", _r_locale_getstring (IDS_BROWSER));

	_r_ctrl_settablestring (
		hwnd,
		IDC_BROWSER,
		&localized_string->sr,
		IDC_BROWSER_DATA,
		pbi->browser_name ? &pbi->browser_name->sr : &empty_string
	);

	_r_obj_movereference (&localized_string, _r_format_string (L"%s:", _r_locale_getstring (IDS_CURRENTVERSION)));

	_r_ctrl_settablestring (
		hwnd,
		IDC_CURRENTVERSION,
		&localized_string->sr,
		IDC_CURRENTVERSION_DATA,
		pbi->current_version ? &pbi->current_version->sr : &empty_string
	);

	_r_obj_movereference (&localized_string, _r_format_string (L"%s:", _r_locale_getstring (IDS_VERSION)));

	_r_ctrl_settablestring (
		hwnd,
		IDC_VERSION,
		&localized_string->sr,
		IDC_VERSION_DATA,
		pbi->new_version ? &pbi->new_version->sr : &empty_string
	);

	_r_obj_movereference (&localized_string, _r_format_string (L"%s:", _r_locale_getstring (IDS_DATE)));

	_r_ctrl_settablestring (
		hwnd,
		IDC_DATE,
		&localized_string->sr,
		IDC_DATE_DATA,
		date_dormat ? &date_dormat->sr : &empty_string
	);

	if (date_dormat)
		_r_obj_dereference (date_dormat);

	_r_obj_dereference (localized_string);
}

VOID init_browser_info (
	_Inout_ PBROWSER_INFORMATION pbi
)
{
	static R_STRINGREF bin_names[] = {
		PR_STRINGREF_INIT (L"brave.exe"),
		PR_STRINGREF_INIT (L"firefox.exe"),
		PR_STRINGREF_INIT (L"basilisk.exe"),
		PR_STRINGREF_INIT (L"palemoon.exe"),
		PR_STRINGREF_INIT (L"waterfox.exe"),
		PR_STRINGREF_INIT (L"dragon.exe"),
		PR_STRINGREF_INIT (L"iridium.exe"),
		PR_STRINGREF_INIT (L"iron.exe"),
		PR_STRINGREF_INIT (L"opera.exe"),
		PR_STRINGREF_INIT (L"slimjet.exe"),
		PR_STRINGREF_INIT (L"vivaldi.exe"),
		PR_STRINGREF_INIT (L"chromium.exe"),
		PR_STRINGREF_INIT (L"chrome.exe"), // default
	};

	static R_STRINGREF separator_sr = PR_STRINGREF_INIT (L"\\");

	PR_STRING binary_dir;
	PR_STRING binary_name;
	PR_STRING browser_type;
	PR_STRING browser_arguments;
	PR_STRING string;
	ULONG binary_type;

	// Reset
	pbi->is_hasurls = FALSE;

	pbi->urls[0] = UNICODE_NULL;
	pbi->args[0] = UNICODE_NULL;

	// Configure paths
	binary_dir = _r_config_getstringexpand (L"ChromiumDirectory", L".\\bin");
	binary_name = _r_config_getstring (L"ChromiumBinary", L"chrome.exe");

	if (!binary_dir || !binary_name)
	{
		RtlRaiseStatus (STATUS_INVALID_PARAMETER);
		return;
	}

	_r_obj_movereference (&pbi->binary_dir, binary_dir);
	_r_str_trimstring2 (pbi->binary_dir, L"\\", 0);

	string = _r_path_search (pbi->binary_dir->buffer, NULL, TRUE);

	if (string)
		_r_obj_movereference (&pbi->binary_dir, string);

	string = _r_obj_concatstringrefs (
		3,
		&pbi->binary_dir->sr,
		&separator_sr,
		&binary_name->sr
	);

	_r_obj_movereference (&pbi->binary_path, string);

	if (!pbi->binary_dir || !pbi->binary_path)
	{
		RtlRaiseStatus (STATUS_OBJECT_PATH_NOT_FOUND);
		return;
	}

	if (!_r_fs_exists (pbi->binary_path->buffer))
	{
		for (SIZE_T i = 0; i < RTL_NUMBER_OF (bin_names); i++)
		{
			string = _r_obj_concatstringrefs (
				3,
				&pbi->binary_dir->sr,
				&separator_sr, &bin_names[i]
			);

			_r_obj_movereference (&pbi->binary_path, string);

			if (_r_fs_exists (pbi->binary_path->buffer))
				break;
		}

		if (_r_obj_isstringempty (pbi->binary_path) || !_r_fs_exists (pbi->binary_path->buffer))
		{
			string = _r_obj_concatstringrefs (
				3,
				&pbi->binary_dir->sr,
				&separator_sr,
				&binary_name->sr
			);

			_r_obj_movereference (&pbi->binary_path, string); // fallback (use defaults)
		}
	}

	_r_obj_dereference (binary_name);

	string = _r_format_string (
		L"%s\\%s_%" TEXT (PR_ULONG) L".bin",
		_r_sys_gettempdirectory ()->buffer,
		_r_app_getnameshort (),
		_r_str_gethash2 (pbi->binary_path, TRUE)
	);

	_r_obj_movereference (&pbi->cache_path, string);

	// Get browser architecture
	pbi->architecture = _r_config_getlong (L"ChromiumArchitecture", 0);

	if (pbi->architecture != 64 && pbi->architecture != 32)
	{
		pbi->architecture = 0;

		// ...by executable
		if (_r_fs_exists (pbi->binary_path->buffer))
		{
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
		pbi->architecture = 64; // default architecture

	// Set common data
	browser_type = _r_config_getstring (L"ChromiumType", CHROMIUM_TYPE);
	browser_arguments = _r_config_getstringexpand (L"ChromiumCommandLine", CHROMIUM_COMMAND_LINE);

	if (browser_type)
		_r_obj_movereference (&pbi->browser_type, browser_type);

	if (browser_arguments)
	{
		_r_str_copy (pbi->args, RTL_NUMBER_OF (pbi->args), browser_arguments->buffer);

		_r_obj_dereference (browser_arguments);
	}

	string = _r_format_string (L"%s (%" TEXT (PR_LONG) L"-bit)", pbi->browser_type->buffer, pbi->architecture);

	_r_obj_movereference (&pbi->browser_name, string);
	_r_obj_movereference (&pbi->current_version, _r_res_queryversionstring (pbi->binary_path->buffer));

	// Parse command line
	{
		LPWSTR *arga;
		LPWSTR key;
		LPWSTR key2;
		SIZE_T first_arg_length;
		INT numargs;

		first_arg_length = 0;

		arga = CommandLineToArgvW (_r_sys_getimagecommandline (), &numargs);

		if (arga)
		{
			first_arg_length = _r_str_getlength (arga[0]);

			if (numargs > 1)
			{
				for (INT i = 1; i < numargs; i++)
				{
					key = arga[i];

					if (*key == L'/' || *key == L'-')
					{
						key2 = PTR_ADD_OFFSET (key, sizeof (WCHAR));

						if (_r_str_compare_length (key2, L"autodownload", 12) == 0)
						{
							pbi->is_autodownload = TRUE;
						}
						else if (_r_str_compare_length (key2, L"bringtofront", 12) == 0)
						{
							pbi->is_bringtofront = TRUE;
						}
						else if (_r_str_compare_length (key2, L"forcecheck", 10) == 0)
						{
							pbi->is_forcecheck = TRUE;
						}
						else if (_r_str_compare_length (key2, L"wait", 4) == 0)
						{
							pbi->is_waitdownloadend = TRUE;
						}
						else if (_r_str_compare_length (key2, L"update", 6) == 0)
						{
							pbi->is_onlyupdate = TRUE;
						}
						else if (*key == L'-')
						{
							if (!pbi->is_opennewwindow)
							{
								if (
									_r_str_compare_length (key, L"-new-tab", 8) == 0 ||
									_r_str_compare_length (key, L"-new-window", 11) == 0 ||
									_r_str_compare_length (key, L"--new-window", 12) == 0 ||
									_r_str_compare_length (key, L"-new-instance", 13) == 0
									)
								{
									pbi->is_opennewwindow = TRUE;
								}
							}

							// there is Chromium arguments
							//_r_str_appendformat (pbi->urls, RTL_NUMBER_OF (pbi->urls), L" %s", key);
						}
					}
					else if (path_is_url (key))
					{
						// there is Chromium url
						pbi->is_hasurls = TRUE;
					}
				}
			}

			LocalFree (arga);
		}

		if (pbi->is_hasurls)
		{
			_r_str_copy (pbi->urls, RTL_NUMBER_OF (pbi->urls), _r_sys_getimagecommandline () + first_arg_length + 2);
			_r_str_trim (pbi->urls, L" ");
		}
	}

	pbi->check_period = _r_config_getlong (L"ChromiumCheckPeriod", 2);

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

VOID _app_setstatus (
	_In_ HWND hwnd,
	_In_opt_ LPCWSTR text,
	_In_opt_ ULONG64 v,
	_In_opt_ ULONG64 t
)
{
	LONG64 percent = 0;

	if (!v && t)
	{
		_r_status_settextformat (hwnd, IDC_STATUSBAR, 0, L"%s 0%%", text);

		if (!_r_str_isempty (text))
		{
			_r_tray_setinfoformat (hwnd, &GUID_TrayIcon, NULL, L"%s\r\n%s: 0%%", _r_app_getname (), text);
		}
		else
		{
			_r_tray_setinfo (hwnd, &GUID_TrayIcon, NULL, _r_app_getname ());
		}
	}
	else if (v && t)
	{
		percent = _r_calc_clamp64 (_r_calc_percentof64 (v, t), 0, 100);

		_r_status_settextformat (hwnd, IDC_STATUSBAR, 0, L"%s %" PR_LONG64 L"%%", text, percent);

		if (!_r_str_isempty (text))
		{
			_r_tray_setinfoformat (
				hwnd,
				&GUID_TrayIcon,
				NULL, L"%s\r\n%s: %" TEXT (PR_LONG64) L"%%",
				_r_app_getname (),
				text,
				percent
			);
		}
		else
		{
			_r_tray_setinfo (hwnd, &GUID_TrayIcon, NULL, _r_app_getname ());
		}
	}
	else
	{
		_r_status_settext (hwnd, IDC_STATUSBAR, 0, text);

		if (!_r_str_isempty (text))
		{
			_r_tray_setinfoformat (hwnd, &GUID_TrayIcon, NULL, L"%s\r\n%s", _r_app_getname (), text);
		}
		else
		{
			_r_tray_setinfo (hwnd, &GUID_TrayIcon, NULL, _r_app_getname ());
		}
	}

	SendDlgItemMessage (hwnd, IDC_PROGRESS, PBM_SETPOS, (WPARAM)(LONG)percent, 0);
}

VOID _app_cleanupoldbinary (
	_In_ PBROWSER_INFORMATION pbi
)
{
	WIN32_FIND_DATA wfd;
	PR_STRING path;
	HANDLE hfile;

	if (!pbi->binary_dir)
		return;

	// remove old binary directory
	// https://github.com/henrypp/chrlauncher/issues/180

	path = _r_obj_concatstrings (
		2,
		pbi->binary_dir->buffer,
		L"\\*"
	);

	hfile = FindFirstFile (path->buffer, &wfd);

	if (_r_fs_isvalidhandle (hfile))
	{
		do
		{
			if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
				continue;

			if (wfd.cFileName[0] == L'.')
				continue;

			_r_obj_movereference (&path, _r_obj_concatstrings (3, pbi->binary_dir->buffer, L"\\", wfd.cFileName));

			_r_fs_deletedirectory (path->buffer, TRUE);
		}
		while (FindNextFile (hfile, &wfd));

		FindClose (hfile);
	}

	_r_obj_dereference (path);
}

VOID _app_cleanupoldmanifest (
	_In_ PBROWSER_INFORMATION pbi
)
{
	WIN32_FIND_DATA wfd;
	PR_STRING path;
	HANDLE hfile;

	if (!pbi->binary_dir || !pbi->current_version)
		return;

	path = _r_obj_concatstrings (
		2,
		pbi->binary_dir->buffer,
		L"\\*.manifest"
	);

	hfile = FindFirstFile (path->buffer, &wfd);

	if (_r_fs_isvalidhandle (hfile))
	{
		do
		{
			if (_r_str_isstartswith2 (&pbi->current_version->sr, wfd.cFileName, TRUE))
			{
				_r_obj_movereference (&path, _r_obj_concatstrings (3, pbi->binary_dir->buffer, L"\\", wfd.cFileName));

				_r_fs_deletefile (path->buffer, 0);
			}
		}
		while (FindNextFile (hfile, &wfd));

		FindClose (hfile);
	}

	_r_obj_dereference (path);
}

BOOLEAN _app_browserisrunning (
	_In_ PBROWSER_INFORMATION pbi
)
{
	HANDLE hfile;

	hfile = CreateFile (
		pbi->binary_path->buffer,
		GENERIC_WRITE | GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
	);

	if (!_r_fs_isvalidhandle (hfile))
		return (GetLastError () == ERROR_SHARING_VIOLATION);

	CloseHandle (hfile);

	return FALSE;
}

VOID _app_openbrowser (
	_In_ PBROWSER_INFORMATION pbi
)
{
	WCHAR args[512];
	PR_STRING arg;
	NTSTATUS status;
	BOOLEAN is_running;

	if (_r_obj_isstringempty (pbi->binary_path) || !_r_fs_exists (pbi->binary_path->buffer))
		return;

	is_running = _app_browserisrunning (pbi);

	if (is_running && !pbi->is_hasurls && !pbi->is_opennewwindow)
	{
		activate_browser_window (pbi);
		return;
	}

	_r_str_copy (args, RTL_NUMBER_OF (args), pbi->args);

	if (pbi->is_hasurls)
	{
		_r_str_appendformat (args, RTL_NUMBER_OF (args), L" %s", pbi->urls);

		// reset
		pbi->is_hasurls = FALSE;
		pbi->urls[0] = UNICODE_NULL;
	}

	pbi->is_opennewwindow = FALSE;

	arg = _r_obj_concatstrings (
		4,
		L"\"",
		pbi->binary_path->buffer,
		L"\" ",
		args
	);

	status = _r_sys_createprocess (pbi->binary_path->buffer, arg->buffer, pbi->binary_dir->buffer);

	if (status != STATUS_SUCCESS)
		_r_show_errormessage (_r_app_gethwnd (), NULL, status, NULL);

	_r_obj_dereference (arg);
}

BOOLEAN _app_ishaveupdate (
	_In_ PBROWSER_INFORMATION pbi
)
{
	return !_r_obj_isstringempty (pbi->download_url) && !_r_obj_isstringempty (pbi->new_version);
}

BOOLEAN _app_isupdatedownloaded (
	_In_ PBROWSER_INFORMATION pbi
)
{
	return !_r_obj_isstringempty (pbi->cache_path) && _r_fs_exists (pbi->cache_path->buffer);
}

BOOLEAN _app_isupdaterequired (
	_In_ PBROWSER_INFORMATION pbi
)
{
	if (pbi->is_forcecheck)
		return TRUE;

	if (pbi->check_period)
	{
		if ((_r_unixtime_now () - _r_config_getlong64 (L"ChromiumLastCheck", 0)) >= _r_calc_days2seconds (pbi->check_period))
			return TRUE;
	}

	return FALSE;
}

UINT _app_getactionid (
	_In_ PBROWSER_INFORMATION pbi
)
{
	if (_app_isupdatedownloaded (pbi))
		return IDS_ACTION_INSTALL;

	else if (_app_ishaveupdate (pbi))
		return IDS_ACTION_DOWNLOAD;

	return IDS_ACTION_CHECK;
}

BOOLEAN _app_checkupdate (
	_In_ HWND hwnd,
	_In_ PBROWSER_INFORMATION pbi,
	_Out_ PBOOLEAN is_error_ptr
)
{
	*is_error_ptr = FALSE;

	if (_app_ishaveupdate (pbi))
		return TRUE;

	PR_HASHTABLE hashtable = NULL;

	R_DOWNLOAD_INFO download_info;
	PR_STRING update_url;
	PR_STRING url;

	HINTERNET hsession;
	PR_STRING string;
	ULONG code;

	BOOLEAN is_exists;
	BOOLEAN is_checkupdate;

	BOOLEAN result;

	result = FALSE;

	is_exists = _r_fs_exists (pbi->binary_path->buffer);
	is_checkupdate = _app_isupdaterequired (pbi);

	_app_setstatus (hwnd, _r_locale_getstring (IDS_STATUS_CHECK), 0, 0);

	SAFE_DELETE_REFERENCE (pbi->new_version);
	pbi->timestamp = 0;

	update_browser_info (hwnd, pbi);

	if (!is_exists || is_checkupdate)
	{
		update_url = _r_config_getstring (L"ChromiumUpdateUrl", CHROMIUM_UPDATE_URL);

		if (!update_url)
		{
			RtlRaiseStatus (STATUS_INVALID_PARAMETER);
			return FALSE;
		}

		url = _r_format_string (update_url->buffer, pbi->architecture, pbi->browser_type->buffer);

		_r_obj_dereference (update_url);

		if (url)
		{
			hsession = _r_inet_createsession (_r_app_getuseragent ());

			if (hsession)
			{
				_r_inet_initializedownload (&download_info);

				code = _r_inet_begindownload (hsession, url, &download_info);

				if (code == ERROR_SUCCESS)
				{
					string = download_info.u.string;

					if (_r_obj_isstringempty (string))
					{
						_r_show_message (hwnd, MB_OK | MB_ICONSTOP, NULL, L"Configuration was not found.");
						*is_error_ptr = TRUE;
					}
					else
					{
						hashtable = _r_str_unserialize (&string->sr, L';', L'=');
						*is_error_ptr = FALSE;
					}
				}
				else
				{
					_r_log (LOG_LEVEL_ERROR, &GUID_TrayIcon, TEXT (__FUNCTION__), code, url->buffer);

					*is_error_ptr = TRUE;
				}

				_r_inet_destroydownload (&download_info);

				_r_inet_close (hsession);
			}

			_r_obj_dereference (url);
		}
	}

	if (hashtable)
	{
		string = _r_obj_findhashtablepointer (hashtable, _r_str_gethash (L"download", TRUE));
		_r_obj_movereference (&pbi->download_url, string);

		string = _r_obj_findhashtablepointer (hashtable, _r_str_gethash (L"version", TRUE));
		_r_obj_movereference (&pbi->new_version, string);

		string = _r_obj_findhashtablepointer (hashtable, _r_str_gethash (L"timestamp", TRUE));

		if (string)
		{
			pbi->timestamp = _r_str_tolong64 (&string->sr);

			_r_obj_dereference (string);
		}

		update_browser_info (hwnd, &browser_info);

		if (!is_exists || (pbi->new_version && _r_str_versioncompare (&pbi->current_version->sr, &pbi->new_version->sr) == -1))
		{
			result = TRUE;
		}
		else
		{
			SAFE_DELETE_REFERENCE (pbi->download_url); // clear download url if update not found

			_r_config_setlong64 (L"ChromiumLastCheck", _r_unixtime_now ());
		}

		_r_obj_dereference (hashtable);
	}

	_app_setstatus (hwnd, NULL, 0, 0);

	return result;
}

BOOLEAN WINAPI _app_downloadupdate_callback (
	_In_ ULONG total_written,
	_In_ ULONG total_length,
	_In_opt_ PVOID lparam
)
{
	if (lparam)
		_app_setstatus ((HWND)lparam, _r_locale_getstring (IDS_STATUS_DOWNLOAD), total_written, total_length);

	return TRUE;
}

BOOLEAN _app_downloadupdate (
	_In_ HWND hwnd,
	_In_ PBROWSER_INFORMATION pbi,
	_Out_ PBOOLEAN is_error_ptr
)
{
	R_DOWNLOAD_INFO download_info;
	PR_STRING temp_file;
	HINTERNET hsession;
	HANDLE hfile;
	ULONG status;
	BOOLEAN result;

	*is_error_ptr = FALSE;

	if (_app_isupdatedownloaded (pbi))
		return TRUE;

	result = FALSE;

	temp_file = _r_obj_concatstrings (
		2,
		pbi->cache_path->buffer,
		L".tmp"
	);

	_r_fs_deletefile (pbi->cache_path->buffer, TRUE);

	_app_setstatus (hwnd, _r_locale_getstring (IDS_STATUS_DOWNLOAD), 0, 1);

	_r_queuedlock_acquireshared (&lock_download);

	hsession = _r_inet_createsession (_r_app_getuseragent ());

	if (hsession)
	{
		hfile = CreateFile (
			temp_file->buffer,
			GENERIC_WRITE,
			FILE_SHARE_READ,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		if (!_r_fs_isvalidhandle (hfile))
		{
			_r_log (LOG_LEVEL_ERROR, &GUID_TrayIcon, L"CreateFile", GetLastError (), temp_file->buffer);

			*is_error_ptr = TRUE;
		}
		else
		{
			_r_inet_initializedownload_ex (&download_info, hfile, &_app_downloadupdate_callback, hwnd);

			status = _r_inet_begindownload (hsession, pbi->download_url, &download_info);

			_r_inet_destroydownload (&download_info); // required!

			if (status != ERROR_SUCCESS)
			{
				_r_log (LOG_LEVEL_ERROR, &GUID_TrayIcon, TEXT (__FUNCTION__), status, pbi->download_url->buffer);

				_r_fs_deletefile (pbi->cache_path->buffer, TRUE);

				*is_error_ptr = TRUE;
			}
			else
			{
				SAFE_DELETE_REFERENCE (pbi->download_url); // clear download url

				_r_fs_movefile (temp_file->buffer, pbi->cache_path->buffer, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED);

				result = TRUE;

				*is_error_ptr = FALSE;
			}

			_r_fs_deletefile (temp_file->buffer, TRUE);
		}

		_r_inet_close (hsession);
	}

	_r_queuedlock_releaseshared (&lock_download);

	_r_obj_dereference (temp_file);

	_app_setstatus (hwnd, NULL, 0, 0);

	return result;
}

BOOLEAN _app_unpack_7zip (
	_In_ HWND hwnd,
	_In_ PBROWSER_INFORMATION pbi,
	_In_ PR_STRINGREF bin_name
)
{
#define kInputBufSize ((SIZE_T)1 << 18)

	static const ISzAlloc g_Alloc = {SzAlloc, SzFree};
	static R_STRINGREF separator_sr = PR_STRINGREF_INIT (L"\\");

	ISzAlloc alloc_imp = g_Alloc;
	ISzAlloc alloc_temp_imp = g_Alloc;

	CFileInStream archive_stream;
	CLookToRead2 look_stream;
	CSzArEx db;
	UInt16 *temp_buff;
	SIZE_T temp_size;

	// if you need cache, use these 3 variables.
	// if you use external function, you can make these variable as static.

	UInt32 block_index = UINT32_MAX; // it can have any value before first call (if out_buffer = 0)
	Byte *out_buffer = NULL; // it must be 0 before first call for each new archive.
	size_t out_buffer_size = 0; // it can have any value before first call (if out_buffer = 0)

	R_STRINGREF path;
	PR_STRING root_dir_name;
	PR_STRING dest_path;
	PR_STRING sub_dir;
	CSzFile out_file;
	size_t offset;
	size_t out_size_processed;
	UInt32 attrib;
	UInt64 total_size;
	UInt64 total_read;
	SIZE_T processed_size;
	SIZE_T length;
	SRes status;
	BOOLEAN is_success;

	status = InFile_OpenW (&archive_stream.file, pbi->cache_path->buffer);

	if (status != ERROR_SUCCESS)
	{
		_r_log (LOG_LEVEL_ERROR, NULL, L"InFile_OpenW", status, pbi->cache_path->buffer);
		return FALSE;
	}

	temp_buff = NULL;
	root_dir_name = NULL;

	is_success = FALSE;

	FileInStream_CreateVTable (&archive_stream);
	LookToRead2_CreateVTable (&look_stream, 0);

	SzArEx_Init (&db);

	look_stream.buf = (byte *)ISzAlloc_Alloc (&alloc_imp, kInputBufSize);

	if (!look_stream.buf)
	{
		_r_log (LOG_LEVEL_ERROR, NULL, L"ISzAlloc_Alloc", SZ_ERROR_MEM, NULL);
		goto CleanupExit;
	}

	total_size = 0;
	total_read = 0;

	look_stream.bufSize = kInputBufSize;
	look_stream.realStream = &archive_stream.vt;
	LookToRead2_Init (&look_stream);

	CrcGenerateTable ();

	status = SzArEx_Open (&db, &look_stream.vt, &alloc_imp, &alloc_temp_imp);

	if (status != SZ_OK)
	{
		_r_log (LOG_LEVEL_ERROR, NULL, L"SzArEx_Open", status, pbi->cache_path->buffer);
		goto CleanupExit;
	}

	temp_size = 0;

	// find root directory which contains main executable
	for (UInt32 i = 0; i < db.NumFiles; i++)
	{
		if (SzArEx_IsDir (&db, i))
			continue;

		length = SzArEx_GetFileNameUtf16 (&db, i, NULL);
		total_size += SzArEx_GetFileSize (&db, i);

		if (length > temp_size)
		{
			temp_size = length;

			if (temp_buff)
			{
				temp_buff = _r_mem_reallocatezero (temp_buff, temp_size * sizeof (UInt16));
			}
			else
			{
				temp_buff = _r_mem_allocatezero (temp_size * sizeof (UInt16));
			}
		}

		if (!root_dir_name)
		{
			length = SzArEx_GetFileNameUtf16 (&db, i, temp_buff);

			if (!length)
				continue;

			_r_obj_initializestringref_ex (&path, (LPWSTR)temp_buff, (length - 1) * sizeof (WCHAR));

			_r_str_replacechar (&path, L'/', OBJ_NAME_PATH_SEPARATOR);

			length = path.length - bin_name->length - separator_sr.length;

			if (_r_str_isendsswith (&path, bin_name, TRUE) &&
				path.buffer[length / sizeof (WCHAR)] == OBJ_NAME_PATH_SEPARATOR)
			{
				_r_obj_movereference (
					&root_dir_name,
					_r_obj_createstring_ex (path.buffer, path.length - bin_name->length)
				);

				_r_str_trimstring (root_dir_name, &separator_sr, 0);
			}
		}
	}

	for (UInt32 i = 0; i < db.NumFiles; i++)
	{
		length = SzArEx_GetFileNameUtf16 (&db, i, temp_buff);

		if (!length)
			continue;

		_r_obj_initializestringref_ex (&path, (LPWSTR)temp_buff, (length - 1) * sizeof (WCHAR));

		_r_str_replacechar (&path, L'/', OBJ_NAME_PATH_SEPARATOR);
		_r_str_trimstringref (&path, &separator_sr, 0);

		// skip non-root dirs
		if (!_r_obj_isstringempty (root_dir_name) &&
			(path.length <= root_dir_name->length || !_r_str_isstartswith (&path, &root_dir_name->sr, TRUE)))
		{
			continue;
		}

		if (root_dir_name)
			_r_obj_skipstringlength (&path, root_dir_name->length + separator_sr.length);

		dest_path = _r_obj_concatstringrefs (
			3,
			&pbi->binary_dir->sr,
			&separator_sr,
			&path
		);

		if (SzArEx_IsDir (&db, i))
		{
			_r_fs_mkdir (dest_path->buffer);
		}
		else
		{
			total_read += SzArEx_GetFileSize (&db, i);

			_app_setstatus (hwnd, _r_locale_getstring (IDS_STATUS_INSTALL), total_read, total_size);

			// create directory if not-exist
			sub_dir = _r_path_getbasedirectory (&dest_path->sr);

			if (sub_dir)
			{
				if (!_r_fs_exists (sub_dir->buffer))
					_r_fs_mkdir (sub_dir->buffer);

				_r_obj_dereference (sub_dir);
			}

			offset = 0;
			out_size_processed = 0;

			status = SzArEx_Extract (
				&db,
				&look_stream.vt,
				i,
				&block_index,
				&out_buffer,
				&out_buffer_size,
				&offset,
				&out_size_processed,
				&alloc_imp,
				&alloc_temp_imp
			);

			if (status != SZ_OK)
			{
				_r_log (LOG_LEVEL_ERROR, NULL, L"SzArEx_Extract", status, dest_path->buffer);
			}
			else
			{
				status = OutFile_OpenW (&out_file, dest_path->buffer);

				if (status != SZ_OK)
				{
					_r_log (LOG_LEVEL_ERROR, NULL, L"OutFile_OpenW", status, dest_path->buffer);
				}
				else
				{
					processed_size = out_size_processed;

					status = File_Write (&out_file, out_buffer + offset, &processed_size);

					if (status != SZ_OK || processed_size != out_size_processed)
					{
						_r_log (LOG_LEVEL_ERROR, NULL, L"File_Write", status, dest_path->buffer);
					}
					else
					{
						if (SzBitWithVals_Check (&db.Attribs, i))
						{
							attrib = db.Attribs.Vals[i];

							//	p7zip stores posix attributes in high 16 bits and adds 0x8000 as marker.
							//	We remove posix bits, if we detect posix mode field
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

		if (!is_success)
			is_success = TRUE;
	}

CleanupExit:

	if (root_dir_name)
		_r_obj_dereference (root_dir_name);

	if (out_buffer)
		ISzAlloc_Free (&alloc_imp, out_buffer);

	if (look_stream.buf)
		ISzAlloc_Free (&alloc_imp, look_stream.buf);

	if (temp_buff)
		_r_mem_free (temp_buff);

	SzArEx_Free (&db, &alloc_imp);

	File_Close (&archive_stream.file);

	return is_success;
}

BOOLEAN _app_unpack_zip (
	_In_ HWND hwnd,
	_In_ PBROWSER_INFORMATION pbi,
	_In_ PR_STRINGREF bin_name
)
{
	static R_STRINGREF separator_sr = PR_STRINGREF_INIT (L"\\");

	mz_zip_archive zip_archive;
	mz_bool zip_bool;

	PR_BYTE bytes;
	PR_STRING root_dir_name;
	R_BYTEREF path_sr;
	PR_STRING path;
	PR_STRING dest_path;
	PR_STRING sub_dir;
	mz_zip_archive_file_stat file_stat;
	ULONG64 total_size;
	ULONG64 total_read; // this is our progress so far
	SIZE_T length;
	INT total_files;
	BOOLEAN is_success;
	NTSTATUS status;

	root_dir_name = NULL;
	is_success = FALSE;

	status = _r_str_unicode2multibyte (&pbi->cache_path->sr, &bytes);

	if (status != STATUS_SUCCESS)
	{
		_r_log (LOG_LEVEL_ERROR, NULL, L"_r_str_unicode2multibyte", status, NULL);

		goto CleanupExit;
	}

	RtlZeroMemory (&zip_archive, sizeof (zip_archive));

	zip_bool = mz_zip_reader_init_file (&zip_archive, bytes->buffer, 0);

	if (!zip_bool)
	{
		//_r_log (LOG_LEVEL_ERROR, NULL, L"mz_zip_reader_init_file", GetLastError (), NULL);

		goto CleanupExit;
	}

	total_size = 0;
	total_read = 0;

	total_files = mz_zip_reader_get_num_files (&zip_archive);

	// find root directory which contains main executable
	for (INT i = 0; i < total_files; i++)
	{
		if (mz_zip_reader_is_file_a_directory (&zip_archive, i) || !mz_zip_reader_file_stat (&zip_archive, i, &file_stat))
			continue;

		if (file_stat.m_is_directory)
			continue;

		// count total size of unpacked files
		total_size += file_stat.m_uncomp_size;

		if (!root_dir_name)
		{
			_r_obj_initializebyteref (&path_sr, file_stat.m_filename);

			if (_r_str_multibyte2unicode (&path_sr, &path) != STATUS_SUCCESS)
				continue;

			_r_str_replacechar (&path->sr, L'/', OBJ_NAME_PATH_SEPARATOR);

			length = path->length - bin_name->length - separator_sr.length;

			if (_r_str_isendsswith (&path->sr, bin_name, TRUE) && path->buffer[length / sizeof (WCHAR)] == OBJ_NAME_PATH_SEPARATOR)
			{
				_r_obj_movereference (&root_dir_name, _r_obj_createstring_ex (path->buffer, path->length - bin_name->length));

				_r_str_trimstring (root_dir_name, &separator_sr, 0);
			}

			_r_obj_dereference (path);
		}
	}

	for (INT i = 0; i < total_files; i++)
	{
		if (!mz_zip_reader_file_stat (&zip_archive, i, &file_stat))
			continue;

		_r_obj_initializebyteref (&path_sr, file_stat.m_filename);

		if (_r_str_multibyte2unicode (&path_sr, &path) != STATUS_SUCCESS)
			continue;

		_r_str_replacechar (&path->sr, L'/', OBJ_NAME_PATH_SEPARATOR);
		_r_str_trimstring (path, &separator_sr, 0);

		// skip non-root dirs
		if (!_r_obj_isstringempty (root_dir_name) &&
			(path->length <= root_dir_name->length || !_r_str_isstartswith (&path->sr, &root_dir_name->sr, TRUE)))
		{
			continue;
		}

		if (root_dir_name)
			_r_obj_skipstringlength (&path->sr, root_dir_name->length + separator_sr.length);

		dest_path = _r_obj_concatstringrefs (
			3,
			&pbi->binary_dir->sr,
			&separator_sr,
			&path->sr
		);

		_app_setstatus (hwnd, _r_locale_getstring (IDS_STATUS_INSTALL), total_read, total_size);

		if (mz_zip_reader_is_file_a_directory (&zip_archive, i))
		{
			_r_fs_mkdir (dest_path->buffer);
		}
		else
		{
			sub_dir = _r_path_getbasedirectory (&dest_path->sr);

			if (sub_dir)
			{
				if (!_r_fs_exists (sub_dir->buffer))
					_r_fs_mkdir (sub_dir->buffer);

				_r_obj_dereference (sub_dir);
			}

			if (_r_str_unicode2multibyte (&dest_path->sr, &bytes) != STATUS_SUCCESS)
				continue;

			if (!mz_zip_reader_extract_to_file (&zip_archive, i, bytes->buffer, MZ_ZIP_FLAG_DO_NOT_SORT_CENTRAL_DIRECTORY))
			{
				_r_log (LOG_LEVEL_ERROR, NULL, L"mz_zip_reader_extract_to_file", GetLastError (), dest_path->buffer);
				continue;
			}

			total_read += file_stat.m_uncomp_size;
		}

		_r_obj_dereference (dest_path);

		if (!is_success)
			is_success = TRUE;
	}

CleanupExit:

	if (bytes)
		_r_obj_dereference (bytes);

	if (root_dir_name)
		_r_obj_dereference (root_dir_name);

	mz_zip_reader_end (&zip_archive);

	return is_success;
}

BOOLEAN _app_installupdate (
	_In_ HWND hwnd,
	_In_ PBROWSER_INFORMATION pbi,
	_Out_ PBOOLEAN is_error_ptr
)
{
	R_STRINGREF bin_name;
	BOOLEAN is_success;

	_r_queuedlock_acquireshared (&lock_download);

	if (!_r_fs_exists (pbi->binary_dir->buffer))
		_r_fs_mkdir (pbi->binary_dir->buffer);

	_app_cleanupoldbinary (pbi);

	_r_path_getpathinfo (&pbi->binary_path->sr, NULL, &bin_name);

	_r_sys_setthreadexecutionstate (ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED);

	is_success = _app_unpack_zip (hwnd, pbi, &bin_name);

	if (!is_success)
		is_success = _app_unpack_7zip (hwnd, pbi, &bin_name);

	if (is_success)
	{
		_r_obj_movereference (&pbi->current_version, _r_res_queryversionstring (pbi->binary_path->buffer));

		_app_cleanupoldmanifest (pbi);
	}
	else
	{
		_r_log (LOG_LEVEL_ERROR, NULL, TEXT (__FUNCTION__), GetLastError (), pbi->cache_path->buffer);

		_r_fs_deletedirectory (pbi->binary_dir->buffer, FALSE); // no recurse
	}

	_r_fs_deletefile (pbi->cache_path->buffer, TRUE); // remove cache file when zip cannot be opened

	*is_error_ptr = !is_success;

	_r_queuedlock_releaseshared (&lock_download);

	_r_sys_setthreadexecutionstate (ES_CONTINUOUS);

	_app_setstatus (hwnd, NULL, 0, 0);

	return is_success;
}

VOID _app_thread_check (
	_In_ PVOID arglist,
	_In_ ULONG busy_count
)
{
	PBROWSER_INFORMATION pbi;
	HWND hwnd;

	BOOLEAN is_haveerror = FALSE;
	BOOLEAN is_stayopen = FALSE;
	BOOLEAN is_installed = FALSE;

	pbi = (PBROWSER_INFORMATION)arglist;
	hwnd = _r_app_gethwnd ();

	_r_queuedlock_acquireshared (&lock_thread);

	_r_ctrl_enable (hwnd, IDC_START_BTN, FALSE);

	_r_progress_setmarquee (hwnd, IDC_PROGRESS, TRUE);

	_r_ctrl_setstring (hwnd, IDC_START_BTN, _r_locale_getstring (_app_getactionid (pbi)));

	// unpack downloaded package
	if (_app_isupdatedownloaded (pbi))
	{
		_r_progress_setmarquee (hwnd, IDC_PROGRESS, FALSE);

		_r_tray_toggle (hwnd, &GUID_TrayIcon, TRUE); // show tray icon

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
			_r_tray_toggle (hwnd, &GUID_TrayIcon, TRUE); // show tray icon
			_r_wnd_toggle (hwnd, TRUE);
		}

		if (is_exists && !pbi->is_waitdownloadend && !pbi->is_onlyupdate)
			_app_openbrowser (pbi);

		if (_app_checkupdate (hwnd, pbi, &is_haveerror))
		{
			_r_tray_toggle (hwnd, &GUID_TrayIcon, TRUE); // show tray icon

			if ((!is_exists || pbi->is_autodownload) && _app_ishaveupdate (pbi))
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
						_r_tray_popup (
							hwnd,
							&GUID_TrayIcon,
							NIIF_INFO,
							_r_app_getname (),
							_r_locale_getstring (IDS_STATUS_DOWNLOADED)
						); // inform user

						_r_ctrl_setstring (hwnd, IDC_START_BTN, _r_locale_getstring (_app_getactionid (pbi)));
						_r_ctrl_enable (hwnd, IDC_START_BTN, TRUE);

						is_stayopen = TRUE;
					}
				}
				else
				{
					_r_tray_popupformat (
						hwnd,
						&GUID_TrayIcon,
						NIIF_INFO,
						_r_app_getname (),
						_r_locale_getstring (IDS_STATUS_FOUND),
						pbi->new_version->buffer
					); // just inform user

					_r_ctrl_setstring (hwnd, IDC_START_BTN, _r_locale_getstring (_app_getactionid (pbi)));
					_r_ctrl_enable (hwnd, IDC_START_BTN, TRUE);

					is_stayopen = TRUE;
				}
			}

			if (!pbi->is_autodownload && !_app_isupdatedownloaded (pbi))
			{
				_r_tray_popupformat (
					hwnd,
					&GUID_TrayIcon,
					NIIF_INFO,
					_r_app_getname (),
					_r_locale_getstring (IDS_STATUS_FOUND),
					pbi->new_version->buffer
				); // just inform user

				_r_ctrl_setstring (hwnd, IDC_START_BTN, _r_locale_getstring (_app_getactionid (pbi)));
				_r_ctrl_enable (hwnd, IDC_START_BTN, TRUE);

				is_stayopen = TRUE;
			}
		}
	}

	_r_progress_setmarquee (hwnd, IDC_PROGRESS, FALSE);

	if (is_haveerror || pbi->is_onlyupdate)
	{
		_r_ctrl_setstring (hwnd, IDC_START_BTN, _r_locale_getstring (_app_getactionid (pbi)));
		_r_ctrl_enable (hwnd, IDC_START_BTN, TRUE);

		if (is_haveerror)
		{
			_r_tray_popup (
				hwnd,
				&GUID_TrayIcon,
				NIIF_INFO,
				_r_app_getname (),
				_r_locale_getstring (IDS_STATUS_ERROR)
			); // just inform user

			_app_setstatus (hwnd, _r_locale_getstring (IDS_STATUS_ERROR), 0, 0);
		}

		is_stayopen = TRUE;
	}

	if (!pbi->is_onlyupdate)
		_app_openbrowser (pbi);

	_r_queuedlock_releaseshared (&lock_thread);

	if (is_stayopen)
	{
		update_browser_info (hwnd, pbi);
	}
	else
	{
		PostMessage (hwnd, WM_DESTROY, 0, 0);
	}
}

INT_PTR CALLBACK DlgProc (
	_In_ HWND hwnd,
	_In_ UINT msg,
	_In_ WPARAM wparam,
	_In_ LPARAM lparam
)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			HWND htip;

			htip = _r_ctrl_createtip (hwnd);

			if (htip)
			{
				_r_ctrl_settiptext (htip, hwnd, IDC_BROWSER_DATA, LPSTR_TEXTCALLBACK);
				_r_ctrl_settiptext (htip, hwnd, IDC_CURRENTVERSION_DATA, LPSTR_TEXTCALLBACK);
				_r_ctrl_settiptext (htip, hwnd, IDC_VERSION_DATA, LPSTR_TEXTCALLBACK);
				_r_ctrl_settiptext (htip, hwnd, IDC_DATE_DATA, LPSTR_TEXTCALLBACK);
			}

			break;
		}

		case RM_INITIALIZE:
		{
			HICON hicon;

			LONG dpi_value;
			LONG icon_small;

			dpi_value = _r_dc_gettaskbardpi ();

			icon_small = _r_dc_getsystemmetrics (SM_CXSMICON, dpi_value);

			hicon = _r_sys_loadsharedicon (_r_sys_getimagebase (), MAKEINTRESOURCE (IDI_MAIN), icon_small);

			init_browser_info (&browser_info);

			_r_tray_create (
				hwnd,
				&GUID_TrayIcon,
				RM_TRAYICON,
				hicon,
				_r_app_getname (),
				(_r_queuedlock_islocked (&lock_download) || _app_isupdatedownloaded (&browser_info)) ? FALSE : TRUE
			);

			_r_workqueue_queueitem (&workqueue, &_app_thread_check, &browser_info);

			if (!browser_info.is_waitdownloadend && !browser_info.is_onlyupdate &&
				_r_fs_exists (browser_info.binary_path->buffer) && !_app_isupdatedownloaded (&browser_info))
			{
				_app_openbrowser (&browser_info);
			}

			break;
		}

		case RM_UNINITIALIZE:
		{
			_r_tray_destroy (hwnd, &GUID_TrayIcon);
			break;
		}

		case RM_LOCALIZE:
		{
			// localize menu
			HMENU hmenu;
			HMENU hsubmenu;

			hmenu = GetMenu (hwnd);

			if (hmenu)
			{
				_r_menu_setitemtext (hmenu, 0, TRUE, _r_locale_getstring (IDS_FILE));
				_r_menu_setitemtext (hmenu, 1, TRUE, _r_locale_getstring (IDS_SETTINGS));

				hsubmenu = GetSubMenu (hmenu, 1);

				if (hsubmenu)
					_r_menu_setitemtextformat (hsubmenu, LANG_MENU, TRUE, L"%s (Language)", _r_locale_getstring (IDS_LANGUAGE));

				_r_menu_setitemtext (hmenu, 2, TRUE, _r_locale_getstring (IDS_HELP));

				_r_menu_setitemtextformat (hmenu, IDM_RUN, FALSE, L"%s...", _r_locale_getstring (IDS_RUN));
				_r_menu_setitemtextformat (hmenu, IDM_OPEN, FALSE, L"%s...", _r_locale_getstring (IDS_OPEN));
				_r_menu_setitemtext (hmenu, IDM_EXIT, FALSE, _r_locale_getstring (IDS_EXIT));
				_r_menu_setitemtext (hmenu, IDM_WEBSITE, FALSE, _r_locale_getstring (IDS_WEBSITE));
				_r_menu_setitemtextformat (hmenu, IDM_ABOUT, FALSE, L"%s\tF1", _r_locale_getstring (IDS_ABOUT));

				_r_locale_enum ((HWND)GetSubMenu (hmenu, 1), LANG_MENU, IDX_LANGUAGE); // enum localizations
			}

			update_browser_info (hwnd, &browser_info);

			_r_ctrl_setstring (hwnd, IDC_LINKS, FOOTER_STRING);

			_r_ctrl_setstring (hwnd, IDC_START_BTN, _r_locale_getstring (_app_getactionid (&browser_info)));

			break;
		}

		case RM_TASKBARCREATED:
		{
			HICON hicon;

			LONG dpi_value;
			LONG icon_small;

			dpi_value = _r_dc_gettaskbardpi ();

			icon_small = _r_dc_getsystemmetrics (SM_CXSMICON, dpi_value);

			hicon = _r_sys_loadsharedicon (_r_sys_getimagebase (), MAKEINTRESOURCE (IDI_MAIN), icon_small);

			_r_tray_create (
				hwnd,
				&GUID_TrayIcon,
				RM_TRAYICON,
				hicon,
				_r_app_getname (),
				(_r_queuedlock_islocked (&lock_download) || _app_isupdatedownloaded (&browser_info)) ? FALSE : TRUE
			);

			break;
		}

		case WM_DPICHANGED:
		{
			HICON hicon;

			LONG dpi_value;
			LONG icon_small;

			dpi_value = _r_dc_gettaskbardpi ();

			icon_small = _r_dc_getsystemmetrics (SM_CXSMICON, dpi_value);

			hicon = _r_sys_loadsharedicon (_r_sys_getimagebase (), MAKEINTRESOURCE (IDI_MAIN), icon_small);

			_r_tray_setinfo (hwnd, &GUID_TrayIcon, hicon, _r_app_getname ());

			SendDlgItemMessage (hwnd, IDC_STATUSBAR, WM_SIZE, 0, 0);

			break;
		}

		case WM_CLOSE:
		{
			if (_r_queuedlock_islocked (&lock_download) &&
				_r_show_message (hwnd, MB_YESNO | MB_ICONQUESTION, NULL, _r_locale_getstring (IDS_QUESTION_STOP)) != IDYES)
			{
				SetWindowLongPtr (hwnd, DWLP_MSGRESULT, TRUE);
				return TRUE;
			}

			DestroyWindow (hwnd);

			break;
		}

		case WM_DESTROY:
		{
			_r_tray_destroy (hwnd, &GUID_TrayIcon);

			if (browser_info.is_waitdownloadend && !browser_info.is_onlyupdate)
				_app_openbrowser (&browser_info);

			//_r_workqueue_waitforfinish (&workqueue);
			//_r_workqueue_destroy (&workqueue);

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
			LONG_PTR exstyle;

			exstyle = _r_wnd_getstyle_ex (hwnd);

			if ((exstyle & WS_EX_LAYERED) == 0)
				_r_wnd_setstyle_ex (hwnd, exstyle | WS_EX_LAYERED);

			SetLayeredWindowAttributes (hwnd, 0, (msg == WM_ENTERSIZEMOVE) ? 100 : 255, LWA_ALPHA);
			SetCursor (LoadCursor (NULL, (msg == WM_ENTERSIZEMOVE) ? IDC_SIZEALL : IDC_ARROW));

			break;
		}

		case WM_NOTIFY:
		{
			LPNMHDR lpnmhdr;

			lpnmhdr = (LPNMHDR)lparam;

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
					string = _r_ctrl_getstring (hwnd, ctrl_id);

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
					PNMLINK nmlink;

					nmlink = (PNMLINK)lparam;

					if (!_r_str_isempty (nmlink->item.szUrl))
						_r_shell_opendefault (nmlink->item.szUrl);

					break;
				}
			}

			break;
		}

		case RM_TRAYICON:
		{
			switch (LOWORD (lparam))
			{
				case NIN_BALLOONUSERCLICK:
				{
					_r_wnd_toggle (hwnd, TRUE);
					break;
				}

				case NIN_KEYSELECT:
				{
					if (GetForegroundWindow () != hwnd)
					{
						_r_wnd_toggle (hwnd, FALSE);
					}

					break;
				}

				case WM_MBUTTONUP:
				{
					PostMessage (hwnd, WM_COMMAND, MAKEWPARAM (IDM_EXPLORE, 0), 0);
					break;
				}

				case WM_LBUTTONUP:
				{
					_r_wnd_toggle (hwnd, FALSE);
					break;
				}

				case WM_CONTEXTMENU:
				{
					HMENU hmenu;
					HMENU hsubmenu;

					SetForegroundWindow (hwnd); // don't touch

					hmenu = LoadMenu (NULL, MAKEINTRESOURCE (IDM_TRAY));

					if (hmenu)
					{
						hsubmenu = GetSubMenu (hmenu, 0);

						if (hsubmenu)
						{
							// localize
							_r_menu_setitemtext (hsubmenu, IDM_TRAY_SHOW, FALSE, _r_locale_getstring (IDS_TRAY_SHOW));
							_r_menu_setitemtextformat (hsubmenu, IDM_TRAY_RUN, FALSE, L"%s...", _r_locale_getstring (IDS_RUN));
							_r_menu_setitemtextformat (hsubmenu, IDM_TRAY_OPEN, FALSE, L"%s...", _r_locale_getstring (IDS_OPEN));
							_r_menu_setitemtext (hsubmenu, IDM_TRAY_WEBSITE, FALSE, _r_locale_getstring (IDS_WEBSITE));
							_r_menu_setitemtext (hsubmenu, IDM_TRAY_ABOUT, FALSE, _r_locale_getstring (IDS_ABOUT));
							_r_menu_setitemtext (hsubmenu, IDM_TRAY_EXIT, FALSE, _r_locale_getstring (IDS_EXIT));

							if (_r_obj_isstringempty (browser_info.binary_path) || !_r_fs_exists (browser_info.binary_path->buffer))
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
				_r_locale_apply (GetSubMenu (GetSubMenu (GetMenu (hwnd), 1), LANG_MENU), LOWORD (wparam), IDX_LANGUAGE);
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
					_app_openbrowser (&browser_info);
					break;
				}

				case IDM_OPEN:
				case IDM_TRAY_OPEN:
				{
					PR_STRING path;

					path = _r_app_getconfigpath ();

					if (_r_fs_exists (path->buffer))
						_r_shell_opendefault (path->buffer);

					break;
				}

				case IDM_EXPLORE:
				{
					if (browser_info.binary_dir)
					{
						if (_r_fs_exists (browser_info.binary_dir->buffer))
							_r_shell_opendefault (browser_info.binary_dir->buffer);
					}

					break;
				}

				case IDM_WEBSITE:
				case IDM_TRAY_WEBSITE:
				{
					_r_shell_opendefault (_r_app_getwebsite_url ());
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
					_r_workqueue_queueitem (&workqueue, &_app_thread_check, &browser_info);
					break;
				}
			}

			break;
		}
	}

	return FALSE;
}

INT APIENTRY wWinMain (
	_In_ HINSTANCE hinst,
	_In_opt_ HINSTANCE prev_hinst,
	_In_ LPWSTR cmdline,
	_In_ INT show_cmd
)
{
	HWND hwnd;
	PR_STRING path;

	if (!_r_app_initialize ())
		return ERROR_APP_INIT_FAILURE;

	_r_workqueue_initialize (&workqueue, 0, 1, 250, NULL);

	path = _r_app_getdirectory ();

	SetCurrentDirectory (path->buffer);

	if (cmdline)
	{
		init_browser_info (&browser_info);

		if (browser_info.is_hasurls && _r_fs_exists (browser_info.binary_path->buffer))
		{
			_app_openbrowser (&browser_info);

			return ERROR_SUCCESS;
		}
	}

	hwnd = _r_app_createwindow (hinst, MAKEINTRESOURCE (IDD_MAIN), MAKEINTRESOURCE (IDI_MAIN), &DlgProc);

	if (!hwnd)
		return ERROR_APP_INIT_FAILURE;

	return _r_wnd_message_callback (hwnd, MAKEINTRESOURCE (IDA_MAIN));
}
