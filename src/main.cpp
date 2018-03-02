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

STATIC_DATA config;

VOID _app_setstatus (HWND hwnd, LPCWSTR text, DWORDLONG v, DWORDLONG t)
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

	if (text)
		app.TraySetInfo (UID, nullptr, _r_fmt (L"%s\r\n%s: %d%%", APP_NAME, text, percent));
	else
		app.TraySetInfo (UID, nullptr, APP_NAME);
}

VOID _app_cleanup (LPCWSTR version)
{
	WIN32_FIND_DATA wfd = {0};
	const HANDLE h = FindFirstFile (_r_fmt (L"%s\\*.manifest", config.binary_dir), &wfd);

	if (h != INVALID_HANDLE_VALUE)
	{
		const size_t len = wcslen (version);

		do
		{
			if (_wcsnicmp (version, wfd.cFileName, len) != 0)
				_r_fs_delete (_r_fmt (L"%s\\%s", config.binary_dir, wfd.cFileName), false);
		}
		while (FindNextFile (h, &wfd));

		FindClose (h);
	}
}

VOID _app_openbrowser (LPCWSTR url)
{
	if (!_r_fs_exists (config.binary_path))
		return;

	if (!url && _r_process_is_exists (config.binary_dir, wcslen (config.binary_dir)))
		return;

	WCHAR args[2048] = {0};

	if (!ExpandEnvironmentStrings (config.args, args, _countof (config.args)))
		StringCchCopy (args, _countof (args), config.args);

	if (url)
		StringCchCat (args, _countof (args), url);

	rstring arg;
	arg.Format (L"\"%s\" %s", config.binary_path, args);

	_r_run (config.binary_path, arg, config.binary_dir);
}

rstring _app_getversion (LPCWSTR path)
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

				if (VerQueryValue (verData, L"\\", (VOID FAR* FAR*)&buffer, &size))
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

bool _app_checkupdate (HWND hwnd)
{
	HINTERNET hconnect = nullptr;
	HINTERNET hrequest = nullptr;

	rstring::map_one result;

	const INT days = app.ConfigGet (L"ChromiumCheckPeriod", 1).AsInt ();
	const bool is_exists = _r_fs_exists (config.binary_path);
	bool is_stopped = false;

	if (days == -1) // there is .ini option to force update checking
		config.is_forcecheck = true;

	if (config.is_forcecheck || (days > 0 || !is_exists))
	{
		if (config.is_forcecheck || !is_exists || (_r_unixtime_now () - app.ConfigGet (L"ChromiumLastCheck", 0).AsLonglong ()) >= (86400 * days))
		{
			HINTERNET hsession = _r_inet_createsession (app.GetUserAgent ());

			if (hsession)
			{
				if (_r_inet_openurl (hsession, _r_fmt (app.ConfigGet (L"ChromiumUpdateUrl", CHROMIUM_UPDATE_URL), config.architecture, config.type), &hconnect, &hrequest, nullptr))
				{
					const DWORD buffer_length = 2048;

					LPSTR buffera = new CHAR[buffer_length];
					rstring bufferw;

					if (buffera)
					{
						DWORD total_length = 0;

						while (true)
						{
							if (WaitForSingleObjectEx (config.stop_evt, 0, FALSE) == WAIT_OBJECT_0)
							{
								is_stopped = true;
								break;
							}

							if (!_r_inet_readrequest (hrequest, buffera, buffer_length - 1, nullptr, &total_length))
								break;

							bufferw.Append (buffera);
						}

						delete[] buffera;
					}

					if (!is_stopped && !bufferw.IsEmpty ())
					{
						rstring::rvector vc = bufferw.AsVector (L";");

						for (size_t i = 0; i < vc.size (); i++)
						{
							const size_t pos = vc.at (i).Find (L'=');

							if (pos != rstring::npos)
								result[vc.at (i).Midded (0, pos)] = vc.at (i).Midded (pos + 1);
						}
					}
				}

				if (hrequest)
					_r_inet_close (hrequest);

				if (hconnect)
					_r_inet_close (hconnect);

				_r_inet_close (hsession);
			}
		}
	}

	if (!is_stopped && !result.empty ())
	{
		if (!is_exists || _wcsnicmp (result[L"version"], config.current_version, wcslen (config.current_version)) != 0)
		{
			SetDlgItemText (hwnd, IDC_BROWSER, _r_fmt (app.LocaleString (IDS_BROWSER, nullptr), config.name_full));
			SetDlgItemText (hwnd, IDC_CURRENTVERSION, _r_fmt (app.LocaleString (IDS_CURRENTVERSION, nullptr), !config.current_version[0] ? L"<not found>" : config.current_version));
			SetDlgItemText (hwnd, IDC_VERSION, _r_fmt (app.LocaleString (IDS_VERSION, nullptr), result[L"version"].GetString ()));
			SetDlgItemText (hwnd, IDC_DATE, _r_fmt (app.LocaleString (IDS_DATE, nullptr), _r_fmt_date (result[L"timestamp"].AsLonglong (), FDTF_SHORTDATE | FDTF_SHORTTIME).GetString ()));

			StringCchCopy (config.download_url, _countof (config.download_url), result[L"download"]);

			app.ConfigSet (L"ChromiumLastBuild", result[L"timestamp"]);
			app.ConfigSet (L"ChromiumLastVersion", result[L"version"]);

			return true;
		}
	}

	return false;
}

bool _app_downloadupdate (HWND hwnd, LPCWSTR url, LPCWSTR path)
{
	bool result = false;

	HINTERNET hconnect = nullptr;
	HINTERNET hrequest = nullptr;

	HINTERNET hsession = _r_inet_createsession (app.GetUserAgent ());

	if (hsession)
	{
		DWORD total_length = 0;
		DWORD total_written = 0;

		if (_r_inet_openurl (hsession, url, &hconnect, &hrequest, &total_length))
		{
			WCHAR temp_file[MAX_PATH] = {0};
			StringCchPrintf (temp_file, _countof (temp_file), L"%s.tmp", path);

			HANDLE hfile = CreateFile (temp_file, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_FLAG_WRITE_THROUGH, nullptr);

			if (hfile != INVALID_HANDLE_VALUE)
			{
				DWORD retn = 0;

				const DWORD buffer_size = BUFFER_SIZE;
				LPSTR buffera = new CHAR[buffer_size];

				if (buffera)
				{
					DWORD notneed = 0;
					DWORD written = 0;

					config.is_isdownloading = true;

					while ((retn = WaitForSingleObjectEx (config.stop_evt, 0, FALSE)) != WAIT_OBJECT_0)
					{
						if (!_r_inet_readrequest (hrequest, buffera, buffer_size - 1, &written, &total_written))
							break;

						WriteFile (hfile, buffera, written, &notneed, nullptr);

						_app_setstatus (hwnd, app.LocaleString (IDS_STATUS_DOWNLOAD, nullptr), total_written, total_length);

						_r_sleep (1);
					}

					config.is_isdownloading = false;

					delete[] buffera;
				}

				CloseHandle (hfile);

				if (retn != WAIT_OBJECT_0)
				{
					_r_fs_move (temp_file, path);
					result = true;
				}
				else
				{
					//_r_fs_delete (temp_file, FALSE);
				}
			}
		}

		if (hrequest)
			_r_inet_close (hrequest);

		if (hconnect)
			_r_inet_close (hconnect);

		_r_inet_close (hsession);
	}

	_app_setstatus (hwnd, nullptr, 0, 0);

	return result;
}

bool _app_installupdate (HWND hwnd, LPCWSTR path)
{
	config.is_isinstalling = true;

	if (!_r_fs_exists (config.binary_dir))
		_r_fs_mkdir (config.binary_dir);

	bool result = false;
	bool is_stopped = false;
	ZIPENTRY ze = {0};

	HZIP hz = OpenZip (path, nullptr);

	if (IsZipHandleU (hz))
	{
		INT start_idx = 0;
		size_t title_length = wcslen (L"chrome-win32");

		// check archive is official package or not
		GetZipItem (hz, 0, &ze);

		if (_wcsnicmp (ze.name, L"chrome-win32", title_length) == 0)
		{
			start_idx = 1;
			title_length += 1;
		}
		else
		{
			title_length = 0;
		}

		DWORDLONG total_size = 0;
		DWORDLONG total_read = 0; // this is our progress so far

		// count total files
		GetZipItem (hz, -1, &ze);
		const INT total_files = ze.index;

		// count total size of unpacked files
		for (INT i = start_idx; i < total_files; i++)
		{
			GetZipItem (hz, i, &ze);

			total_size += ze.unc_size;
		}

		rstring fpath;

		CHAR buffer[BUFFER_SIZE] = {0};
		DWORD written = 0;

		for (INT i = start_idx; i < total_files; i++)
		{
			if (WaitForSingleObjectEx (config.stop_evt, 0, FALSE) == WAIT_OBJECT_0)
			{
				is_stopped = true;
				break;
			}

			GetZipItem (hz, i, &ze);

			fpath.Format (L"%s\\%s", config.binary_dir, rstring (ze.name + title_length).Replace (L"/", L"\\").GetString ());

			_app_setstatus (hwnd, app.LocaleString (IDS_STATUS_INSTALL, nullptr), total_read, total_size);

			if ((ze.attr & FILE_ATTRIBUTE_DIRECTORY) != 0)
			{
				_r_fs_mkdir (fpath);
			}
			else
			{
				{
					rstring dir = _r_path_extractdir (fpath);

					if (!_r_fs_exists (dir))
						_r_fs_mkdir (dir);
				}

				const HANDLE h = CreateFile (fpath, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_FLAG_WRITE_THROUGH, nullptr);

				if (h != INVALID_HANDLE_VALUE)
				{
					DWORD total_read_file = 0;

					for (ZRESULT zr = ZR_MORE; zr == ZR_MORE;)
					{
						DWORD bufsize = BUFFER_SIZE;

						zr = UnzipItem (hz, i, buffer, bufsize);

						if (zr == ZR_OK)
							bufsize = ze.unc_size - total_read_file;

						buffer[bufsize] = 0;

						WriteFile (h, buffer, bufsize, &written, nullptr);

						total_read_file += bufsize;
						total_read += bufsize;
					}

					CloseHandle (h);
				}
			}
		}

		if (!is_stopped)
			result = true;

		CloseZip (hz);
	}
	else
	{
		_r_fs_delete (path, false); // remove cache file when zip cannot be opened
	}

	if (result)
	{
		_app_cleanup (_app_getversion (config.binary_path));
		_r_fs_delete (path, false); // remove cache file on installation finished
	}

	_app_setstatus (hwnd, nullptr, 0, 0);

	config.is_isinstalling = false;

	return result;
}

UINT WINAPI _app_thread (LPVOID lparam)
{
	const HWND hwnd = (HWND)lparam;
	const HANDLE evts[] = {config.stop_evt, config.check_evt, config.download_evt};

	while (true)
	{
		const DWORD state = WaitForMultipleObjectsEx (_countof (evts), evts, FALSE, INFINITE, FALSE);

		if (state == WAIT_OBJECT_0) // stop_evt
		{
			break;
		}
		else if (state == WAIT_OBJECT_0 + 1) // check_evt
		{
			_r_ctrl_enable (hwnd, IDC_START_BTN, false);

			if (_r_fs_exists (config.cache_path))
			{
				_r_ctrl_settext (hwnd, IDC_START_BTN, app.LocaleString (IDS_ACTION_INSTALL, nullptr));
				app.TrayToggle (UID, true); // show tray icon

				if (!_r_process_is_exists (config.binary_dir, wcslen (config.binary_dir)))
				{
					if (config.is_bringtofront)
						_r_wnd_toggle (hwnd, true); // show window

					if (_app_installupdate (hwnd, config.cache_path))
					{
						app.ConfigSet (L"ChromiumLastCheck", _r_unixtime_now ());
						config.is_isinstalled = true;
					}
				}
				else
				{
					_r_ctrl_enable (hwnd, IDC_START_BTN, true);
				}
			}

			if (!config.is_isinstalled)
			{
				if (!config.is_waitdownloadend)
					_app_openbrowser (nullptr);

				if (_app_checkupdate (hwnd))
				{
					if (!_r_fs_exists (config.binary_path) || config.is_autodownload)
					{
						SetEvent (config.download_evt);
					}
					else
					{
						_r_ctrl_settext (hwnd, IDC_START_BTN, app.LocaleString (IDS_ACTION_DOWNLOAD, nullptr));
						_r_ctrl_enable (hwnd, IDC_START_BTN, true);

						app.TrayToggle (UID, true); // show tray icon
						app.TrayPopup (UID, NIIF_USER, APP_NAME, app.LocaleString (IDS_STATUS_FOUND, nullptr)); // just inform user
					}
				}
				else
				{
					break; // update not found
				}
			}
		}
		else if (state == WAIT_OBJECT_0 + 2) // download_evt
		{
			_r_ctrl_enable (hwnd, IDC_START_BTN, false);
			_r_ctrl_settext (hwnd, IDC_START_BTN, app.LocaleString (IDS_ACTION_DOWNLOAD, nullptr));

			app.TrayToggle (UID, true); // show tray icon

			if (config.is_bringtofront)
				_r_wnd_toggle (hwnd, true); // show window

			if (!config.is_isdownloaded && !config.is_waitdownloadend)
				_app_openbrowser (nullptr);

			if (config.is_isdownloaded || _app_downloadupdate (hwnd, config.download_url, config.cache_path))
			{
				config.is_isdownloaded = true;

				if (!_r_process_is_exists (config.binary_dir, wcslen (config.binary_dir)))
				{
					_r_ctrl_enable (hwnd, IDC_START_BTN, false);

					if (_app_installupdate (hwnd, config.cache_path))
					{
						app.ConfigSet (L"ChromiumLastCheck", _r_unixtime_now ());
						config.is_isinstalled = true;
					}
				}
				else
				{
					app.TrayPopup (UID, NIIF_USER, APP_NAME, app.LocaleString (IDS_STATUS_DOWNLOADED, nullptr)); // inform user

					_r_ctrl_enable (hwnd, IDC_START_BTN, true);
					_r_ctrl_settext (hwnd, IDC_START_BTN, app.LocaleString (IDS_ACTION_INSTALL, nullptr));
				}
			}
			else
			{
				_r_ctrl_enable (hwnd, IDC_START_BTN, true);
			}

			if (config.is_isinstalled)
			{
				_app_openbrowser (nullptr);
				break;
			}
		}
		else
		{
			break; // stop or error
		}
	}

	SetEvent (config.end_evt);

	PostMessage (hwnd, WM_DESTROY, 0, 0);

	return ERROR_SUCCESS;
}

BOOL initializer_callback (HWND hwnd, DWORD msg, LPVOID, LPVOID)
{
	switch (msg)
	{
		case _RM_INITIALIZE:
		{
			app.TrayCreate (hwnd, UID, WM_TRAYICON, _r_loadicon (app.GetHINSTANCE (), MAKEINTRESOURCE (IDI_MAIN), GetSystemMetrics (SM_CXSMICON)), (config.is_isdownloading || config.is_isinstalling || config.is_isdownloaded) ? false : true);

			SetCurrentDirectory (app.GetDirectory ());

			// configure paths
			GetTempPath (_countof (config.cache_path), config.cache_path);
			StringCchCat (config.cache_path, _countof (config.cache_path), APP_NAME_SHORT L"Cache.ZIP");

			StringCchCopy (config.binary_dir, _countof (config.binary_dir), _r_path_expand (app.ConfigGet (L"ChromiumDirectory", L".\\bin")));
			StringCchPrintf (config.binary_path, _countof (config.binary_path), L"%s\\%s", config.binary_dir, app.ConfigGet (L"ChromiumBinary", L"chrome.exe").GetString ());

			// get browser architecture...
			if (_r_sys_validversion (5, 1, 0, VER_EQUAL) || _r_sys_validversion (5, 2, 0, VER_EQUAL))
				config.architecture = 32; // on XP only 32-bit supported
			else
				config.architecture = app.ConfigGet (L"ChromiumArchitecture", 0).AsUint ();

			if (!config.architecture || (config.architecture != 64 && config.architecture != 32))
			{
				config.architecture = 0;

				// ...by executable
				if (_r_fs_exists (config.binary_path))
				{
					DWORD exe_type = 0;

					if (GetBinaryType (config.binary_path, &exe_type))
					{
						if (exe_type == SCS_32BIT_BINARY)
							config.architecture = 32;
						else if (exe_type == SCS_64BIT_BINARY)
							config.architecture = 64;
					}
				}

				// ...by processor architecture
				if (!config.architecture)
				{
					SYSTEM_INFO si = {0};
					GetNativeSystemInfo (&si);

					if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
						config.architecture = 64;
					else
						config.architecture = 32;
				}
			}

			if (!config.architecture || (config.architecture != 32 && config.architecture != 64))
				config.architecture = 32; // default architecture

			// set common data
			StringCchCopy (config.type, _countof (config.type), app.ConfigGet (L"ChromiumType", L"dev-codecs-sync"));
			StringCchPrintf (config.name_full, _countof (config.name_full), L"Chromium %d-bit (%s)", config.architecture, config.type);

			StringCchCopy (config.args, _countof (config.args), app.ConfigGet (L"ChromiumCommandLine", L"--user-data-dir=..\\profile --no-default-browser-check --allow-outdated-plugins --disable-logging --disable-breakpad"));

			StringCchCopy (config.current_version, _countof (config.current_version), _app_getversion (config.binary_path));

			// set controls data
			SetDlgItemText (hwnd, IDC_BROWSER, _r_fmt (app.LocaleString (IDS_BROWSER, nullptr), config.name_full));
			SetDlgItemText (hwnd, IDC_CURRENTVERSION, _r_fmt (app.LocaleString (IDS_CURRENTVERSION, nullptr), !config.current_version[0] ? L"<not found>" : config.current_version));
			SetDlgItemText (hwnd, IDC_VERSION, _r_fmt (app.LocaleString (IDS_VERSION, nullptr), app.ConfigGet (L"ChromiumLastVersion", L"<not found>").GetString ()));
			SetDlgItemText (hwnd, IDC_DATE, _r_fmt (app.LocaleString (IDS_DATE, nullptr), _r_fmt_date (app.ConfigGet (L"ChromiumLastBuild", 0).AsLonglong (), FDTF_SHORTDATE | FDTF_SHORTTIME).GetString ()));

			_r_wnd_addstyle (hwnd, IDC_START_BTN, app.IsClassicUI () ? WS_EX_STATICEDGE : 0, WS_EX_STATICEDGE, GWL_EXSTYLE);

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
								config.is_autodownload = true;
							}
							else if (_wcsnicmp (arga[i], L"/b", 2) == 0 || _wcsnicmp (arga[i], L"/bringtofront", 13) == 0)
							{
								config.is_bringtofront = true;
							}
							else if (_wcsnicmp (arga[i], L"/f", 2) == 0 || _wcsnicmp (arga[i], L"/forcecheck", 11) == 0)
							{
								config.is_forcecheck = true;
							}
							else if (_wcsnicmp (arga[i], L"/w", 2) == 0 || _wcsnicmp (arga[i], L"/wait", 5) == 0)
							{
								config.is_waitdownloadend = true;
							}
						}
						else if (arga[i][0] == L'-' && arga[i][1] == L'-')
						{
							// there is Chromium arguments
							StringCchCat (config.args, _countof (config.args), L" ");
							StringCchCat (config.args, _countof (config.args), arga[i]);
						}
						else
						{
							// there is Chromium url
							StringCchCat (config.urls, _countof (config.urls), L" \"");
							StringCchCat (config.urls, _countof (config.urls), arga[i]);
							StringCchCat (config.urls, _countof (config.urls), L"\"");
						}
					}

					LocalFree (arga);
				}
			}

			// set default config
			if (!config.is_autodownload)
				config.is_autodownload = app.ConfigGet (L"ChromiumAutoDownload", false).AsBool ();

			if (!config.is_bringtofront)
				config.is_bringtofront = app.ConfigGet (L"ChromiumBringToFront", false).AsBool ();

			if (!config.is_waitdownloadend)
				config.is_waitdownloadend = app.ConfigGet (L"ChromiumWaitForDownloadEnd", false).AsBool ();

			// set ppapi info
			{
				rstring ppapi_path = _r_path_expand (app.ConfigGet (L"FlashPlayerPath", L".\\plugins\\pepflashplayer.dll"));

				if (_r_fs_exists (ppapi_path))
				{
					StringCchCat (config.args, _countof (config.args), L" --ppapi-flash-path=\"");
					StringCchCat (config.args, _countof (config.args), ppapi_path);
					StringCchCat (config.args, _countof (config.args), L"\" --ppapi-flash-version=\"");
					StringCchCat (config.args, _countof (config.args), _app_getversion (ppapi_path));
					StringCchCat (config.args, _countof (config.args), L"\"");
				}
			}

			if (hwnd)
			{
				SetEvent (config.check_evt);
			}
			else
			{
				_app_openbrowser (config.urls);
			}

			break;
		}

		case _RM_UNINITIALIZE:
		{
			app.TrayDestroy (UID);
			break;
		}

		case _RM_LOCALIZE:
		{
			// localize menu
			const HMENU menu = GetMenu (hwnd);

			app.LocaleMenu (menu, IDS_FILE, 0, true, nullptr);
			app.LocaleMenu (menu, IDS_EXIT, IDM_EXIT, false, nullptr);
			app.LocaleMenu (menu, IDS_HELP, 1, true, nullptr);
			app.LocaleMenu (menu, IDS_WEBSITE, IDM_WEBSITE, false, nullptr);
			app.LocaleMenu (menu, IDS_DONATE, IDM_DONATE, false, nullptr);
			app.LocaleMenu (menu, IDS_ABOUT, IDM_ABOUT, false, L"\tF1");

			break;
		}

		case _RM_ARGUMENTS:
		{
			initializer_callback (nullptr, _RM_INITIALIZE, nullptr, nullptr);
			break;
		}
	}

	return FALSE;
}

INT_PTR CALLBACK DlgProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			// configure statusbar
			const INT parts[] = {app.GetDPI (228), -1};
			SendDlgItemMessage (hwnd, IDC_STATUSBAR, SB_SETPARTS, 2, (LPARAM)parts);

			config.stop_evt = CreateEvent (nullptr, FALSE, FALSE, nullptr);
			config.end_evt = CreateEvent (nullptr, FALSE, FALSE, nullptr);
			config.check_evt = CreateEvent (nullptr, FALSE, FALSE, nullptr);
			config.download_evt = CreateEvent (nullptr, FALSE, FALSE, nullptr);

			config.thread = (HANDLE)_beginthreadex (nullptr, 0, &_app_thread, hwnd, 0, nullptr);

			break;
		}

		case WM_CLOSE:
		{
			if ((config.is_isdownloading || config.is_isinstalling) && _r_msg (hwnd, MB_YESNO | MB_ICONQUESTION, APP_NAME, nullptr, app.LocaleString (IDS_QUESTION_STOP, nullptr)) != IDYES)
				return TRUE;

			DestroyWindow (hwnd);

			break;
		}

		case WM_DESTROY:
		{
			app.TrayDestroy (UID);

			if (config.is_waitdownloadend)
				_app_openbrowser (nullptr);

			SetEvent (config.stop_evt);

			if (config.is_isdownloading || config.is_isinstalling)
				WaitForSingleObjectEx (config.end_evt, 5000, FALSE);

			if (config.end_evt)
				CloseHandle (config.end_evt);

			if (config.stop_evt)
				CloseHandle (config.stop_evt);

			if (config.check_evt)
				CloseHandle (config.check_evt);

			if (config.download_evt)
				CloseHandle (config.download_evt);

			if (config.thread)
				CloseHandle (config.thread);

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
					app.LocaleMenu (hsubmenu, IDS_WEBSITE, IDM_TRAY_WEBSITE, false, nullptr);
					app.LocaleMenu (hsubmenu, IDS_ABOUT, IDM_TRAY_ABOUT, false, nullptr);
					app.LocaleMenu (hsubmenu, IDS_EXIT, IDM_TRAY_EXIT, false, nullptr);

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

				case IDM_WEBSITE:
				case IDM_TRAY_WEBSITE:
				{
					ShellExecute (hwnd, nullptr, _APP_WEBSITE_URL, nullptr, nullptr, SW_SHOWDEFAULT);
					break;
				}

				case IDM_ABOUT:
				case IDM_TRAY_ABOUT:
				{
					app.CreateAboutWindow (hwnd, app.LocaleString (IDS_DONATE, nullptr));
					break;
				}

				case IDC_START_BTN:
				{
					SetEvent (config.download_evt);
					break;
				}
			}

			break;
		}
	}

	return FALSE;
}

INT APIENTRY wWinMain (HINSTANCE, HINSTANCE, LPWSTR, INT)
{
	if (app.CreateMainWindow (IDD_MAIN, IDI_MAIN, &DlgProc, &initializer_callback))
	{
		MSG msg = {0};

		const HACCEL haccel = LoadAccelerators (app.GetHINSTANCE (), MAKEINTRESOURCE (IDA_MAIN));

		while (GetMessage (&msg, nullptr, 0, 0) > 0)
		{
			if (haccel)
				TranslateAccelerator (app.GetHWND (), haccel, &msg);

			if (!IsDialogMessage (app.GetHWND (), &msg))
			{
				TranslateMessage (&msg);
				DispatchMessage (&msg);
			}
		}

		if (haccel)
			DestroyAcceleratorTable (haccel);
	}

	return ERROR_SUCCESS;
}
