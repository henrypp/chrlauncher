#include <colorconstants.au3>
#include <fileconstants.au3>
#include <guiconstantsex.au3>
#include <inetconstants.au3>
#include <msgboxconstants.au3>
#include <progressconstants.au3>
#include <staticconstants.au3>
#include <windowsconstants.au3>
#include <winapifiles.au3>

#include <include\zip.au3>

#notrayicon

opt ('guioneventmode', 1);

; settings
global $directory_name = 'chrome-win32';
global $app_bin_path = @scriptdir & '\bin';
global $app_tmp_path = @scriptdir & '\temp';

global $dpi_ratio = _getdpi ();

global $download_state = 0;
global $download_handle = 0;

global $current_ver = filegetversion ($app_bin_path & '\chrome.exe', 'ProductVersion');

if @error == 1 then

	$current_ver = '<n/a>';
	
endif

; app definition
global $app_name = 'chrlauncher';
global $app_description = 'Portable Chromium launcher and updater.';
global $app_version = 'v1.0';
global $app_copyright = 'Copyright (c) 2015 Henry++';
global $app_website = 'www.henrypp.org';
global $app_github = 'github.com/henrypp';

; main window
global $main_dlg = guicreate ($app_name & ' ' & $app_version, 340 * $dpi_ratio, 142 * $dpi_ratio);
guiseticon (@scriptfullpath, -1, $main_dlg);

; create menu
global $file_menu = guictrlcreatemenu ('File');
global $cancel_item = guictrlcreatemenuitem ('Exit' & @TAB & 'Esc', $file_menu);

global $help_menu = guictrlcreatemenu ('Help');
global $about_item = guictrlcreatemenuitem ('About', $help_menu);

; create controls
global $status_ctrl = guictrlcreatelabel ('Checking for new version of Chromium...', 12 * $dpi_ratio, 12 * $dpi_ratio, 300 * $dpi_ratio, 14 * $dpi_ratio);
global $percent_ctrl = guictrlcreatelabel ('', 304 * $dpi_ratio, 12 * $dpi_ratio, 24 * $dpi_ratio, 14 * $dpi_ratio, $SS_RIGHT);

global $progress_ctrl = guictrlcreateprogress (12 * $dpi_ratio, 36 * $dpi_ratio, 316 * $dpi_ratio, 20 * $dpi_ratio, $PBS_SMOOTH);

global $current_ver_ctrl = guictrlcreatelabel ('Current:' & @tab & $current_ver, 12 * $dpi_ratio, 70 * $dpi_ratio, 150 * $dpi_ratio, 14 * $dpi_ratio);

global $new_ver_ctrl = guictrlcreatelabel ('Available:' & @tab & '<n/a>', 12 * $dpi_ratio, 90 * $dpi_ratio, 150 * $dpi_ratio, 14 * $dpi_ratio);

; create website links
global $website_ctrl = guictrlcreatelabel ($app_website, 178 * $dpi_ratio, 70 * $dpi_ratio, 150 * $dpi_ratio, 14 * $dpi_ratio, $SS_RIGHT);

guictrlsetfont ($website_ctrl, -1, -1, 4);
guictrlsetcolor ($website_ctrl, $COLOR_BLUE);
guictrlsetcursor ($website_ctrl, 0);

global $github_ctrl = guictrlcreatelabel ($app_github, 178 * $dpi_ratio, 90 * $dpi_ratio, 150 * $dpi_ratio, 14 * $dpi_ratio, $SS_RIGHT);

guictrlsetfont ($github_ctrl, -1, -1, 4);
guictrlsetcolor ($github_ctrl, $COLOR_BLUE);
guictrlsetcursor ($github_ctrl, 0);

; assign events
guisetonevent ($GUI_EVENT_CLOSE, 'onExit');

guictrlsetonevent ($cancel_item, 'onExit');
guictrlsetonevent ($website_ctrl, 'onWebsite');
guictrlsetonevent ($github_ctrl, 'onGithub');
guictrlsetonevent ($about_item, 'onAbout');

_run_application (0);

winclose ($main_dlg);

while 1

	sleep (100);

wend

func onExit ()

	if($download_state and msgbox ($MB_ICONQUESTION + $MB_YESNO, $app_name, 'Are you want to cancel download update?', 0, $main_dlg) == $IDNO) then

		return;

	endif

	if $download_handle <> 0 then

		inetclose ($download_handle);

	endif
	
	dirremove ($app_tmp_path, $DIR_REMOVE);

	exit;

endfunc

func onWebsite ()

	shellexecute ('http://' & $app_website);

endfunc

func onGithub ()

	shellexecute ('http://' & $app_github);

endfunc

func onAbout ()

	msgbox($MB_ICONINFORMATION, 'About', $app_name & ' ' & $app_version & @crlf & $app_copyright & @crlf & @crlf & $app_description & @crlf & @crlf & $app_website, 0, $main_dlg);

endfunc

func _run_application ($no_check)

	; start checking for updates
	if $no_check == 0 then

		_check_updates (0);

	endif

	$params = iniread (@scriptdir & '\' & $app_name & '.ini', $app_name, 'cmdline', '--user-data-dir=..\profile --no-default-browser-check') & ' ' & $cmdlineraw;

	run ('"' & $app_bin_path & '\chrome.exe" ' & $params, @scriptdir);

	if @error then

		if $no_check == 0 then
	
			_check_updates (1);
			_run_application (1);

		else

			msgbox ($MB_ICONERROR, $app_name, '"chrome.exe" cannot be found!');

		endif

	endif

endfunc

func _check_updates ($force)

	if iniread (@scriptdir & '\' & $app_name & '.ini', $app_name, 'no_check', 0) == 0 then

		if $force == 0 then

			$last_checking = iniread (@scriptdir & '\' & $app_name & '.ini', $app_name, 'last_checking', 0);

			if (_time () - $last_checking) <= 86400 then

				return

			endif

		endif

		; create temp directory
		dircreate ($app_tmp_path);

		$html_file = _download_file ('http://chromium.woolyss.com/download');

		local $f = fileread ($html_file);

		if $f then

			local $ver = stringregexp ($f, 'Version: ([0-9]{1,4}\.[0-9]{1,4}\.[0-9]{1,4}\.[0-9]{1,4})', $STR_REGEXPARRAYGLOBALMATCH);

			; on 64-bit system - download 64-bit chromium
			local $url_regexp = '';

			if @processorarch == 'X64' then

				$url_regexp = '(https://storage\.googleapis\.com/chromium-browser-continuous/Win_x64/[0-9]{1,6}/chrome-win32\.zip)';

			else

				$url_regexp = '(https://storage\.googleapis\.com/chromium-browser-continuous/Win/[0-9]{1,6}/chrome-win32\.zip)';

			endif

			local $url = stringregexp ($f, $url_regexp, $STR_REGEXPARRAYGLOBALMATCH);

			guictrlsetdata ($new_ver_ctrl, 'Available:' & @tab & $ver[0]);
			guictrlsetcolor ($current_ver_ctrl, $COLOR_RED);
			guictrlsetcolor ($new_ver_ctrl, $COLOR_GREEN);

			if $current_ver <> $ver[0] then

				$download_state = 1;

				guiswitch ($main_dlg);
				guisetstate (@sw_show);

				guictrlsetdata ($status_ctrl, 'Downloading new version...');

				$zip_file = _download_file ($url[0]);

				_installupdate ($zip_file);

				$download_state = 0;
				
			endif

		endif

		iniread (@scriptdir & '\' & $app_name & '.ini', $app_name, 'last_checking', _time ());

	endif

endfunc

func _installupdate ($file)

	guictrlsetdata ($status_ctrl, 'Installing update...');

	local $zip_file = $file & '.zip';
	filemove ($file, $zip_file, $FC_OVERWRITE + $FC_CREATEPATH);
	
	; clear directory first
	dirremove ($app_bin_path, $DIR_REMOVE);

	; unpack archive to folder
	_zip_unzip ($zip_file, $directory_name, $app_tmp_path, 1 + 16 + 256);

	; move update to binary path
	dirmove($app_tmp_path & '\' & $directory_name, $app_bin_path, $FC_OVERWRITE);

	if @error == 0 then

		guictrlsetdata ($status_ctrl, 'Successfully installed...');

	else

		msgbox ($MB_OK + $MB_ICONERROR, $app_name, 'Error #' & @error & ' occurred...');

	endif

endfunc

func _download_file ($url)

    local $path = _winapi_gettempfilename ($app_tmp_path);
	local $size = inetgetsize ($url);

    $download_handle = inetget ($url, $path, $INET_FORCERELOAD, $INET_DOWNLOADBACKGROUND);

    do
		$bytes_done = inetgetinfo ($download_handle, $INET_DOWNLOADREAD);

		if $size <> 0 then

			$progress = floor (($bytes_done / $size) * 100);

			guictrlsetdata ($progress_ctrl, $progress);
			guictrlsetdata ($percent_ctrl, $progress & '%');

		endif

		sleep (100);
		
	until inetgetinfo ($download_handle, $INET_DOWNLOADCOMPLETE);

	guictrlsetdata ($progress_ctrl, 0);
	guictrlsetdata ($percent_ctrl, '');

    inetclose ($download_handle);
	$download_handle = 0;

	return $path;

endfunc

func _getdpi ()

    local $hwnd = 0

    local $hdc = DllCall ('user32.dll', 'long', 'GetDC', 'long', $hwnd)
    local $ret = DllCall ('gdi32.dll', 'long', 'GetDeviceCaps', 'long', $hdc[0], 'long', 90);
	
    $hdc = DllCall ('user32.dll', 'long', 'ReleaseDC', 'long', $hwnd, 'long', $hdc)

    if $ret[0] = 0 then
		$ret[0] = 96
	endif

    return $ret[0] / 96

endfunc

func _time ()

	local $result = dllcall ('crtdll.dll', 'long:cdecl', 'time', 'ptr', 0);

	if @error then

		return 0;

	endif

	return $result[0];

endfunc
