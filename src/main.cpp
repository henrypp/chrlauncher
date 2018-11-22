// chrlauncher
// Copyright (c) 2015-2018 Henry++

#include <windows.h>

#include "main.hpp"
#include "rapp.hpp"
#include "routine.hpp"
#include "unzip.hpp"

#include "resource.hpp"

rapp app (APP_NAME, APP_NAME_SHORT, APP_VERSION, APP_COPYRIGHT);

#define CHROMIUM_UPDATE_URL L"https://chromium.woolyss.com/api/v3/?os=windows&bit=%d&type=%s&out=string"
#define BUFFER_SIZE (_R_BUFFER_LENGTH * 4)

HANDLE hthread_check = nullptr;
BROWSER_INFORMATION browser_info;

_R_FASTLOCK lock_download;

void _app_logerror (LPCWSTR fn, DWORD code, LPCWSTR desc)
{
	_r_dbg_write (APP_NAME_SHORT, APP_VERSION, fn, code, desc);
}

rstring _app_getbinaryversion (LPCWSTR path)
{
	rstring result;

	DWORD verHandle = 0;
	const DWORD verSize = GetFileVersionInfoSize (path, &verHandle);

	if (verSize)
	{
		LPSTR verData = new CHAR[verSize];

		if (verData)
		{
			if (GetFileVersionInfo (path, verHandle, verSize, verData))
			{
				LPBYTE buffer = nullptr;
				UINT size = 0;

				if (VerQueryValue (verData, L"\\", (void FAR* FAR*)&buffer, &size))
				{
					if (size)
					{
						VS_FIXEDFILEINFO const *verInfo = (VS_FIXEDFILEINFO*)buffer;

						if (verInfo->dwSignature == 0xfeef04bd)
						{
							// Doesn't matter if you are on 32 bit or 64 bit,
							// DWORD is always 32 bits, so first two revision numbers
							// come from dwFileVersionMS, last two come from dwFileVersionLS

							result.Format (L"%d.%d.%d.%d", (verInfo->dwFileVersionMS >> 16) & 0xffff, (verInfo->dwFileVersionMS >> 0) & 0xffff, (verInfo->dwFileVersionLS >> 16) & 0xffff, (verInfo->dwFileVersionLS >> 0) & 0xffff);
						}
					}
				}
			}

			delete[] verData;
		}
	}

	return result;
}

void update_browser_info (HWND hwnd, BROWSER_INFORMATION* pbi)
{
	SetDlgItemText (hwnd, IDC_BROWSER, _r_fmt (app.LocaleString (IDS_BROWSER, nullptr), pbi->name_full));
	SetDlgItemText (hwnd, IDC_CURRENTVERSION, _r_fmt (app.LocaleString (IDS_CURRENTVERSION, nullptr), !pbi->current_version[0] ? L"<not found>" : pbi->current_version));
	SetDlgItemText (hwnd, IDC_VERSION, _r_fmt (app.LocaleString (IDS_VERSION, nullptr), !pbi->new_version[0] ? L"<not found>" : pbi->new_version));
	SetDlgItemText (hwnd, IDC_DATE, _r_fmt (app.LocaleString (IDS_DATE, nullptr), pbi->timestamp ? _r_fmt_date (pbi->timestamp, FDTF_SHORTDATE | FDTF_SHORTTIME).GetString () : L"<not found>"));
}

void init_browser_info (BROWSER_INFORMATION* pbi)
{
	pbi->urls[0] = 0; // reset

	// configure paths
	GetTempPath (_countof (pbi->cache_path), pbi->cache_path);
	StringCchCat (pbi->cache_path, _countof (pbi->cache_path), APP_NAME_SHORT L"Cache.ZIP");

	StringCchCopy (pbi->binary_dir, _countof (pbi->binary_dir), _r_path_expand (app.ConfigGet (L"ChromiumDirectory", L".\\bin")));
	StringCchPrintf (pbi->binary_path, _countof (pbi->binary_path), L"%s\\%s", pbi->binary_dir, app.ConfigGet (L"ChromiumBinary", L"chrome.exe").GetString ());

	// get browser architecture...
	if (_r_sys_validversion (5, 1, 0, VER_EQUAL) || _r_sys_validversion (5, 2, 0, VER_EQUAL))
		pbi->architecture = 32; // winxp supports only 32-bit

	else
		pbi->architecture = app.ConfigGet (L"ChromiumArchitecture", 0).AsUint ();

	if (!pbi->architecture || (pbi->architecture != 64 && pbi->architecture != 32))
	{
		pbi->architecture = 0;

		// ...by executable
		if (_r_fs_exists (pbi->binary_path))
		{
			DWORD exe_type = 0;

			if (GetBinaryType (pbi->binary_path, &exe_type))
			{
				pbi->architecture = (exe_type == SCS_64BIT_BINARY) ? 64 : 32;
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

	if (!pbi->architecture || (pbi->architecture != 32 && pbi->architecture != 64))
		pbi->architecture = 32; // default architecture

	// set common data
	StringCchCopy (pbi->type, _countof (pbi->type), app.ConfigGet (L"ChromiumType", L"dev-codecs-sync"));
	StringCchPrintf (pbi->name_full, _countof (pbi->name_full), L"Chromium %d-bit (%s)", pbi->architecture, pbi->type);

	StringCchCopy (pbi->current_version, _countof (pbi->current_version), _app_getbinaryversion (pbi->binary_path));

	StringCchCopy (pbi->args, _countof (pbi->args), app.ConfigGet (L"ChromiumCommandLine", L"--user-data-dir=..\\profile --no-default-browser-check --allow-outdated-plugins --disable-logging --disable-breakpad"));

	// parse command line
	{
		INT numargs = 0;
		LPWSTR* arga = CommandLineToArgvW (GetCommandLine (), &numargs);

		if (arga)
		{
			for (INT i = 1; i < numargs; i++)
			{
				if (wcslen (arga[i]) < 2)
					continue;

				if (arga[i][0] == L'/')
				{
					if (_wcsnicmp (arga[i], L"/a", 2) == 0 || _wcsnicmp (arga[i], L"/autodownload", 13) == 0)
					{
						pbi->is_autodownload = true;
					}
					else if (_wcsnicmp (arga[i], L"/b", 2) == 0 || _wcsnicmp (arga[i], L"/bringtofront", 13) == 0)
					{
						pbi->is_bringtofront = true;
					}
					else if (_wcsnicmp (arga[i], L"/f", 2) == 0 || _wcsnicmp (arga[i], L"/forcecheck", 11) == 0)
					{
						pbi->is_forcecheck = true;
					}
					else if (_wcsnicmp (arga[i], L"/w", 2) == 0 || _wcsnicmp (arga[i], L"/wait", 5) == 0)
					{
						pbi->is_waitdownloadend = true;
					}
				}
				else if (arga[i][0] == L'-' && arga[i][1] == L'-')
				{
					// there is Chromium arguments
					StringCchCat (pbi->args, _countof (pbi->args), L" ");
					StringCchCat (pbi->args, _countof (pbi->args), arga[i]);
				}
				else
				{
					// there is Chromium url
					StringCchCat (pbi->urls, _countof (pbi->urls), L" \"");
					StringCchCat (pbi->urls, _countof (pbi->urls), arga[i]);
					StringCchCat (pbi->urls, _countof (pbi->urls), L"\"");
				}
			}

			LocalFree (arga);
		}
	}

	pbi->check_period = app.ConfigGet (L"ChromiumCheckPeriod", 1).AsInt ();

	if (pbi->check_period == -1)
		pbi->is_forcecheck = true;

	// set default config
	if (!pbi->is_autodownload)
		pbi->is_autodownload = app.ConfigGet (L"ChromiumAutoDownload", false).AsBool ();

	if (!pbi->is_bringtofront)
		pbi->is_bringtofront = app.ConfigGet (L"ChromiumBringToFront", true).AsBool ();

	if (!pbi->is_waitdownloadend)
		pbi->is_waitdownloadend = app.ConfigGet (L"ChromiumWaitForDownloadEnd", true).AsBool ();

	// set ppapi info
	{
		const rstring ppapi_path = _r_path_expand (app.ConfigGet (L"FlashPlayerPath", L".\\plugins\\pepflashplayer.dll"));

		if (_r_fs_exists (ppapi_path))
		{
			StringCchCat (pbi->args, _countof (pbi->args), L" --ppapi-flash-path=\"");
			StringCchCat (pbi->args, _countof (pbi->args), ppapi_path);
			StringCchCat (pbi->args, _countof (pbi->args), L"\" --ppapi-flash-version=\"");
			StringCchCat (pbi->args, _countof (pbi->args), _app_getbinaryversion (ppapi_path));
			StringCchCat (pbi->args, _countof (pbi->args), L"\"");
		}
	}
}

void _app_setstatus (HWND hwnd, LPCWSTR text, DWORDLONG v, DWORDLONG t)
{
	// primary part
	_r_status_settext (hwnd, IDC_STATUSBAR, 0, text);

	// second part
	rstring text2;
	UINT percent = 0;

	if (t)
	{
		percent = static_cast<UINT>((double (v) / double (t)) * 100.0);
		text2.Format (L"%s/%s", _r_fmt_size64 (v).GetString (), _r_fmt_size64 (t).GetString ());
	}

	SendDlgItemMessage (hwnd, IDC_PROGRESS, PBM_SETPOS, percent, 0);

	_r_status_settext (hwnd, IDC_STATUSBAR, 1, text2);

	app.TraySetInfo (hwnd, UID, nullptr, nullptr, text ? _r_fmt (L"%s\r\n%s: %d%%", APP_NAME, text, percent) : APP_NAME);
}

void _app_cleanup (BROWSER_INFORMATION* pbi, LPCWSTR current_version)
{
	WIN32_FIND_DATA wfd = {0};

	const HANDLE hfile = FindFirstFile (_r_fmt (L"%s\\*.manifest", pbi->binary_dir), &wfd);

	if (hfile != INVALID_HANDLE_VALUE)
	{
		const size_t len = wcslen (current_version);

		do
		{
			if (_wcsnicmp (current_version, wfd.cFileName, len) != 0)
				_r_fs_delete (_r_fmt (L"%s\\%s", pbi->binary_dir, wfd.cFileName), false);
		}
		while (FindNextFile (hfile, &wfd));

		FindClose (hfile);
	}
}

bool _app_browserisrunning (BROWSER_INFORMATION* pbi)
{
	const HANDLE hfile = CreateFile (pbi->binary_path, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

	bool result = false;

	if (hfile != INVALID_HANDLE_VALUE)
		CloseHandle (hfile);

	else
		result = (GetLastError () == ERROR_SHARING_VIOLATION);

	return result;
}

void _app_openbrowser (BROWSER_INFORMATION* pbi)
{
	if (!_r_fs_exists (pbi->binary_path))
		return;

	if (!pbi->urls[0] && _app_browserisrunning (pbi))
		return;

	WCHAR args[2048] = {0};

	if (!ExpandEnvironmentStrings (pbi->args, args, _countof (pbi->args)))
		StringCchCopy (args, _countof (args), pbi->args);

	if (pbi->urls[0])
	{
		StringCchCat (args, _countof (args), pbi->urls);
		pbi->urls[0] = 0; // reset
	}

	rstring arg;
	arg.Format (L"\"%s\" %s", pbi->binary_path, args);

	_r_run (pbi->binary_path, arg, pbi->binary_dir);
}

bool _app_ishaveupdate (BROWSER_INFORMATION* pbi)
{
	if (pbi->download_url[0] && pbi->new_version[0])
		return true;

	return false;
}

bool _app_checkupdate (HWND hwnd, BROWSER_INFORMATION* pbi, bool *pis_error)
{
	if (_app_ishaveupdate (pbi))
		return true;

	rstring::map_one result;

	const bool is_exists = _r_fs_exists (pbi->binary_path);
	bool result2 = false;

	_app_setstatus (hwnd, app.LocaleString (IDS_STATUS_CHECK, nullptr), 0, 0);

	if (!is_exists || pbi->is_forcecheck || pbi->check_period > 0)
	{
		if (!is_exists || pbi->is_forcecheck || (_r_unixtime_now () - app.ConfigGet (L"ChromiumLastCheck", 0).AsLonglong ()) >= _R_SECONDSCLOCK_DAY (pbi->check_period))
		{
			rstring content;
			rstring url;

			url.Format (app.ConfigGet (L"ChromiumUpdateUrl", CHROMIUM_UPDATE_URL), pbi->architecture, pbi->type);

			if (app.DownloadURL (url, &content, false, nullptr, 0))
			{
				if (!content.IsEmpty ())
				{
					const rstring::rvector vc = content.AsVector (L";");

					for (size_t i = 0; i < vc.size (); i++)
					{
						const size_t pos = vc.at (i).Find (L'=');

						if (pos != rstring::npos)
							result[vc.at (i).Midded (0, pos)] = vc.at (i).Midded (pos + 1);
					}
				}

				*pis_error = false;
			}
			else
			{
				_app_logerror (TEXT (__FUNCTION__), GetLastError (), url);

				*pis_error = true;
			}
		}
	}

	if (!result.empty ())
	{
		StringCchCopy (pbi->download_url, _countof (pbi->download_url), result[L"download"]);
		StringCchCopy (pbi->new_version, _countof (pbi->new_version), result[L"version"]);

		pbi->timestamp = result[L"timestamp"].AsLonglong ();

		update_browser_info (hwnd, &browser_info);

		if (!is_exists || _r_str_versioncompare (pbi->current_version, pbi->new_version) == -1)
		{
			result2 = true;
		}
		else
		{
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
	if (pbi->is_isdownloaded)
		return true;

	bool result = false;

	WCHAR temp_file[MAX_PATH] = {0};
	StringCchPrintf (temp_file, _countof (temp_file), L"%s.tmp", pbi->cache_path);

	_app_setstatus (hwnd, app.LocaleString (IDS_STATUS_DOWNLOAD, nullptr), 0, 0);

	_r_fastlock_acquireexclusive (&lock_download);

	if (app.DownloadURL (pbi->download_url, temp_file, true, &_app_downloadupdate_callback, (LONG_PTR)hwnd))
	{
		_r_fs_move (temp_file, pbi->cache_path);
		result = true;

		*pis_error = false;
	}
	else
	{
		_app_logerror (TEXT (__FUNCTION__), GetLastError (), pbi->download_url);

		_r_fs_delete (temp_file, FALSE);

		*pis_error = true;
	}

	pbi->is_isdownloaded = result;

	_r_fastlock_releaseexclusive (&lock_download);

	_app_setstatus (hwnd, nullptr, 0, 0);

	return result;
}

bool _app_installupdate (HWND hwnd, BROWSER_INFORMATION* pbi, bool *pis_error)
{
	_r_fastlock_acquireexclusive (&lock_download);

	if (!_r_fs_exists (pbi->binary_dir))
		_r_fs_mkdir (pbi->binary_dir);

	bool result = false;

	ZIPENTRY ze = {0};

	const HZIP hzip = OpenZip (pbi->cache_path, nullptr);

	if (IsZipHandleU (hzip))
	{
		INT start_idx = 0; // most zip packages has root directory
		size_t title_length = 0;

		// check archive is official package or not
		if (GetZipItem (hzip, start_idx, &ze) == ZR_OK)
		{
			const size_t pos = rstring (ze.name).Find (L'/');

			RDBG (L"%d %s", start_idx, ze.name);

			if ((ze.attr & FILE_ATTRIBUTE_DIRECTORY) != 0)
			{
				title_length = pos + 1;
				start_idx = 1;
			}
			else if (pos != rstring::npos)
			{
				title_length = pos + 1;
			}
		}

		INT total_files = 0;
		DWORDLONG total_size = 0;
		DWORDLONG total_read = 0; // this is our progress so far

		// count total files
		if (GetZipItem (hzip, -1, &ze) == ZR_OK)
		{
			total_files = ze.index;

			// count total size of unpacked files
			for (INT i = start_idx; i < total_files; i++)
			{
				GetZipItem (hzip, i, &ze);
				total_size += ze.unc_size;
			}
		}

		rstring fpath;

		CHAR buffer[BUFFER_SIZE] = {0};
		DWORD written = 0;

		for (INT i = start_idx; i < total_files; i++)
		{
			if (GetZipItem (hzip, i, &ze) != ZR_OK)
				continue;

			RDBG (L"%d %s", i, ze.name);

			fpath.Format (L"%s\\%s", pbi->binary_dir, rstring (ze.name + title_length).Replace (L"/", L"\\").GetString ());

			_app_setstatus (hwnd, app.LocaleString (IDS_STATUS_INSTALL, nullptr), total_read, total_size);

			if ((ze.attr & FILE_ATTRIBUTE_DIRECTORY) != 0)
			{
				_r_fs_mkdir (fpath);
			}
			else
			{
				{
					const rstring dir = _r_path_extractdir (fpath);

					if (!_r_fs_exists (dir))
						_r_fs_mkdir (dir);
				}

				const HANDLE hfile = CreateFile (fpath, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_FLAG_WRITE_THROUGH, nullptr);

				if (hfile != INVALID_HANDLE_VALUE)
				{
					DWORD total_read_file = 0;

					for (ZRESULT zr = ZR_MORE; zr == ZR_MORE;)
					{
						DWORD bufsize = BUFFER_SIZE;

						zr = UnzipItem (hzip, i, buffer, bufsize);

						if (zr == ZR_OK)
							bufsize = ze.unc_size - total_read_file;

						buffer[bufsize] = 0;

						WriteFile (hfile, buffer, bufsize, &written, nullptr);

						total_read_file += bufsize;
						total_read += bufsize;
					}

					CloseHandle (hfile);
				}
				else
				{
					_app_logerror (L"CreateFile", GetLastError (), fpath);
				}
			}
		}

		result = true;

		CloseZip (hzip);
	}
	else
	{
		_app_logerror (TEXT (__FUNCTION__), GetLastError (), pbi->cache_path);
		_r_fs_delete (pbi->cache_path, false); // remove cache file when zip cannot be opened
	}

	if (result)
	{
		_app_cleanup (pbi, _app_getbinaryversion (pbi->binary_path));
		_r_fs_delete (pbi->cache_path, false); // remove cache file on installation finished
	}

	*pis_error = !result;
	pbi->is_isinstalled = result;

	_r_fastlock_releaseexclusive (&lock_download);

	_app_setstatus (hwnd, nullptr, 0, 0);

	return result;
}

UINT WINAPI _app_thread_check (LPVOID lparam)
{
	BROWSER_INFORMATION* pbi = (BROWSER_INFORMATION*)lparam;
	const HWND hwnd = app.GetHWND ();

	bool is_stayopen = false;
	bool is_haveerror = false;

	_r_ctrl_enable (hwnd, IDC_START_BTN, false);

	_r_status_pbsetmarquee (hwnd, IDC_PROGRESS, true);

	// install downloaded package
	if (_r_fs_exists (pbi->cache_path))
	{
		_r_ctrl_settext (hwnd, IDC_START_BTN, app.LocaleString (IDS_ACTION_INSTALL, nullptr));
		_r_status_pbsetmarquee (hwnd, IDC_PROGRESS, false);

		app.TrayToggle (hwnd, UID, nullptr, true); // show tray icon

		pbi->is_isdownloaded = true;

		if (!_app_browserisrunning (pbi))
		{
			if (pbi->is_bringtofront)
				_r_wnd_toggle (hwnd, true); // show window

			if (_app_installupdate (hwnd, pbi, &is_haveerror))
			{
				init_browser_info (pbi);
				update_browser_info (hwnd, pbi);

				app.ConfigSet (L"ChromiumLastCheck", _r_unixtime_now ());
			}
		}
		else
		{
			_r_ctrl_enable (hwnd, IDC_START_BTN, true);
			is_stayopen = true;
		}
	}

	const bool is_exists = _r_fs_exists (pbi->binary_path);

	if (!pbi->is_isinstalled)
	{
		_r_status_pbsetmarquee (hwnd, IDC_PROGRESS, true);

		if (pbi->check_period != 0)
			app.TrayToggle (hwnd, UID, nullptr, true); // show tray icon

		// it's like "first run"
		if (!is_exists || (pbi->is_waitdownloadend && pbi->check_period != 0))
		{
			app.TrayToggle (hwnd, UID, nullptr, true); // show tray icon

			if (!is_exists || pbi->is_bringtofront)
				_r_wnd_toggle (hwnd, true);
		}

		if (is_exists && !pbi->is_waitdownloadend)
			_app_openbrowser (pbi);

		if (_app_checkupdate (hwnd, pbi, &is_haveerror))
		{
			app.TrayToggle (hwnd, UID, nullptr, true); // show tray icon

			if (!is_exists || pbi->is_autodownload || (!pbi->is_autodownload && pbi->is_ischecked))
			{
				if (pbi->is_bringtofront)
					_r_wnd_toggle (hwnd, true); // show window

				if (is_exists && !pbi->is_isdownloaded && !pbi->is_waitdownloadend)
					_app_openbrowser (pbi);

				_r_status_pbsetmarquee (hwnd, IDC_PROGRESS, false);

				if (_app_downloadupdate (hwnd, pbi, &is_haveerror))
				{
					if (!_app_browserisrunning (pbi))
					{
						_r_ctrl_enable (hwnd, IDC_START_BTN, false);

						if (_app_installupdate (hwnd, pbi, &is_haveerror))
						{
							app.ConfigSet (L"ChromiumLastCheck", _r_unixtime_now ());
							pbi->is_isinstalled = true;
						}
					}
					else
					{
						app.TrayPopup (hwnd, UID, nullptr, NIIF_USER, APP_NAME, app.LocaleString (IDS_STATUS_DOWNLOADED, nullptr)); // inform user

						_r_ctrl_settext (hwnd, IDC_START_BTN, app.LocaleString (IDS_ACTION_INSTALL, nullptr));
						_r_ctrl_enable (hwnd, IDC_START_BTN, true);

						is_stayopen = true;
					}
				}
				else
				{
					app.TrayPopup (hwnd, UID, nullptr, NIIF_USER, APP_NAME, _r_fmt (app.LocaleString (IDS_STATUS_FOUND, nullptr), pbi->new_version)); // just inform user

					_r_ctrl_settext (hwnd, IDC_START_BTN, app.LocaleString (IDS_ACTION_DOWNLOAD, nullptr));
					_r_ctrl_enable (hwnd, IDC_START_BTN, true);

					is_stayopen = true;
				}
			}

			if (!pbi->is_autodownload && !pbi->is_isdownloaded)
			{
				app.TrayPopup (hwnd, UID, nullptr, NIIF_USER, APP_NAME, _r_fmt (app.LocaleString (IDS_STATUS_FOUND, nullptr), pbi->new_version)); // just inform user

				_r_ctrl_settext (hwnd, IDC_START_BTN, app.LocaleString (IDS_ACTION_DOWNLOAD, nullptr));
				_r_ctrl_enable (hwnd, IDC_START_BTN, true);

				is_stayopen = true;
			}

			pbi->is_ischecked = true;
		}
	}

	if (is_haveerror)
	{
		app.TrayPopup (hwnd, UID, nullptr, NIIF_USER, APP_NAME, app.LocaleString (IDS_STATUS_ERROR, nullptr)); // just inform user

		_app_setstatus (hwnd, app.LocaleString (IDS_STATUS_ERROR, nullptr), 0, 0);

		_r_ctrl_settext (hwnd, IDC_START_BTN, app.LocaleString (IDS_ACTION_DOWNLOAD, nullptr));
		_r_ctrl_enable (hwnd, IDC_START_BTN, true);

		is_stayopen = true;
	}

	if (pbi->is_isinstalled)
		_app_openbrowser (pbi);

	_r_status_pbsetmarquee (hwnd, IDC_PROGRESS, false);

	if (!is_stayopen)
		PostMessage (hwnd, WM_DESTROY, 0, 0);

	_endthreadex (ERROR_SUCCESS);

	return ERROR_SUCCESS;
}

INT_PTR CALLBACK DlgProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			_r_fastlock_initialize (&lock_download);

			// configure statusbar
			const INT parts[] = {app.GetDPI (228), -1};
			SendDlgItemMessage (hwnd, IDC_STATUSBAR, SB_SETPARTS, 2, (LPARAM)parts);

			break;
		}

		case RM_INITIALIZE:
		{
			SetCurrentDirectory (app.GetDirectory ());

			init_browser_info (&browser_info);

			app.TrayCreate (hwnd, UID, nullptr, WM_TRAYICON, app.GetSharedIcon (app.GetHINSTANCE (), IDI_MAIN, GetSystemMetrics (SM_CXSMICON)), (_r_fastlock_islocked (&lock_download) || browser_info.is_isdownloaded) ? false : true);

			if (!hthread_check || WaitForSingleObject (hthread_check, 0) == WAIT_OBJECT_0)
			{
				if (hthread_check)
				{
					CloseHandle (hthread_check);
					hthread_check = nullptr;
				}

				hthread_check = _r_createthread (&_app_thread_check, &browser_info);
			}

			if (!browser_info.is_waitdownloadend && _r_fs_exists (browser_info.binary_path))
				_app_openbrowser (&browser_info);

			break;
		}

		case RM_UNINITIALIZE:
		{
			app.TrayDestroy (hwnd, UID, nullptr);
			break;
		}

		case RM_LOCALIZE:
		{
			// localize menu
			const HMENU hmenu = GetMenu (hwnd);

			app.LocaleMenu (hmenu, IDS_FILE, 0, true, nullptr);
			app.LocaleMenu (hmenu, IDS_RUN, IDM_RUN, false, _r_fmt (L" \"%s\"", _r_path_extractfile (browser_info.binary_path).GetString ()));
			app.LocaleMenu (hmenu, IDS_OPEN, IDM_OPEN, false, _r_fmt (L" \"%s\"\tF2", _r_path_extractfile (app.GetConfigPath ()).GetString ()));
			app.LocaleMenu (hmenu, IDS_EXIT, IDM_EXIT, false, L"\tAlt+F4");
			app.LocaleMenu (hmenu, IDS_SETTINGS, 1, true, nullptr);
			app.LocaleMenu (GetSubMenu (hmenu, 1), IDS_LANGUAGE, LANG_MENU, true, L" (Language)");
			app.LocaleMenu (hmenu, IDS_HELP, 2, true, nullptr);
			app.LocaleMenu (hmenu, IDS_WEBSITE, IDM_WEBSITE, false, nullptr);
			app.LocaleMenu (hmenu, IDS_ABOUT, IDM_ABOUT, false, L"\tF1");

			app.LocaleEnum ((HWND)GetSubMenu (hmenu, 1), LANG_MENU, true, IDX_LANGUAGE); // enum localizations

			update_browser_info (hwnd, &browser_info);

			SetDlgItemText (hwnd, IDC_START_BTN, app.LocaleString (browser_info.is_isdownloaded ? IDS_ACTION_INSTALL : IDS_ACTION_DOWNLOAD, nullptr));

			_r_wnd_addstyle (hwnd, IDC_START_BTN, app.IsClassicUI () ? WS_EX_STATICEDGE : 0, WS_EX_STATICEDGE, GWL_EXSTYLE);

			break;
		}

		case WM_CLOSE:
		{
			if (_r_fastlock_islocked (&lock_download) && _r_msg (hwnd, MB_YESNO | MB_ICONQUESTION, APP_NAME, nullptr, app.LocaleString (IDS_QUESTION_STOP, nullptr)) != IDYES)
				return TRUE;

			DestroyWindow (hwnd);

			break;
		}

		case WM_DESTROY:
		{
			app.TrayDestroy (hwnd, UID, nullptr);

			if (browser_info.is_waitdownloadend)
				_app_openbrowser (&browser_info);

			if (_r_fastlock_islocked (&lock_download))
				WaitForSingleObjectEx (hthread_check, 7500, FALSE);

			if (hthread_check)
				CloseHandle (hthread_check);

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

		case WM_NOTIFY:
		{
			switch (LPNMHDR (lparam)->code)
			{
				case NM_CLICK:
				case NM_RETURN:
				{
					ShellExecute (nullptr, nullptr, PNMLINK (lparam)->item.szUrl, nullptr, nullptr, SW_SHOWNORMAL);
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
					app.LocaleMenu (hsubmenu, IDS_RUN, IDM_TRAY_RUN, false, _r_fmt (L" \"%s\"", _r_path_extractfile (browser_info.binary_path).GetString ()));
					app.LocaleMenu (hsubmenu, IDS_OPEN, IDM_TRAY_OPEN, false, _r_fmt (L" \"%s\"", _r_path_extractfile (app.GetConfigPath ()).GetString ()));
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
					if (!hthread_check || WaitForSingleObject (hthread_check, 0) == WAIT_OBJECT_0)
					{
						if (hthread_check)
						{
							CloseHandle (hthread_check);
							hthread_check = nullptr;
						}

						hthread_check = _r_createthread (&_app_thread_check, &browser_info);
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

	if (args)
	{
		SetCurrentDirectory (app.GetDirectory ());

		init_browser_info (&browser_info);

		if (browser_info.urls[0] && _r_fs_exists (browser_info.binary_path))
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

	return (INT)msg.wParam;
}
