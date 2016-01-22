@echo off

set "CHRLAUNCHER_PATH=%~dp0chrlauncher.exe"
set "CHRLAUNCHER_ICON=\"%CHRLAUNCHER_PATH%\",0"
set "CHRLAUNCHER_ARGS=\"%CHRLAUNCHER_PATH%\" /url \"%%1\""

if not exist %CHRLAUNCHER_PATH% (

	echo ERROR: "chrlauncher.exe" not found.

) else (

	reg add hklm /f>nul 2>&1

	if ERRORLEVEL 1 (

		echo ERROR: you have no privileges.

	) else (

		reg add "HKCR\chrlauncherHTML\DefaultIcon" /v "" /t REG_SZ /d "%CHRLAUNCHER_ICON%" /f
		reg add "HKCR\chrlauncherHTML\shell\open\command" /v "" /t REG_SZ /d "%CHRLAUNCHER_ARGS%" /f

		reg add "HKCR\chrlauncherURL\DefaultIcon" /v "" /t REG_SZ /d "%CHRLAUNCHER_ICON%" /f
		reg add "HKCR\chrlauncherURL\shell\open\command" /v "" /t REG_SZ /d "%CHRLAUNCHER_ARGS%" /f

		reg add "HKCR\ftp\DefaultIcon" /v "" /t REG_SZ /d "%CHRLAUNCHER_ICON%" /f
		reg add "HKCR\ftp\shell\open\command" /v "" /t REG_SZ /d "%CHRLAUNCHER_ARGS%" /f

		reg add "HKCR\http\DefaultIcon" /v "" /t REG_SZ /d "%CHRLAUNCHER_ICON%" /f
		reg add "HKCR\http\shell\open\command" /v "" /t REG_SZ /d "%CHRLAUNCHER_ARGS%" /f

		reg add "HKCR\https\DefaultIcon" /v "" /t REG_SZ /d "%CHRLAUNCHER_ICON%" /f
		reg add "HKCR\https\shell\open\command" /v "" /t REG_SZ /d "%CHRLAUNCHER_ARGS%" /f

		reg add "HKCR\.htm" /v "" /t REG_SZ /d "chrlauncherHTML" /f
		reg add "HKCR\.html" /v "" /t REG_SZ /d "chrlauncherHTML" /f
		reg add "HKCR\.shtml" /v "" /t REG_SZ /d "chrlauncherHTML" /f
		reg add "HKCR\.xht" /v "" /t REG_SZ /d "chrlauncherHTML" /f
		reg add "HKCR\.xhtml" /v "" /t REG_SZ /d "chrlauncherHTML" /f

		reg add "HKCU\Software\Microsoft\Windows\Shell\Associations\UrlAssociations\ftp\UserChoice" /v "ProgId" /t REG_SZ /d "chrlauncherHTML" /f
		reg add "HKCU\Software\Microsoft\Windows\Shell\Associations\UrlAssociations\http\UserChoice" /v "ProgId" /t REG_SZ /d "chrlauncherHTML" /f
		reg add "HKCU\Software\Microsoft\Windows\Shell\Associations\UrlAssociations\https\UserChoice" /v "ProgId" /t REG_SZ /d "chrlauncherHTML" /f

		reg add "HKCU\Software\Microsoft\Windows\Shell\Associations\MIMEAssociations\text/html\UserChoice" /v "ProgId" /t REG_SZ /d "chrlauncherHTML" /f

		reg add "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.htm\UserChoice" /v "ProgId" /t REG_SZ /d "chrlauncherHTML" /f
		reg add "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.html\UserChoice" /v "ProgId" /t REG_SZ /d "chrlauncherHTML" /f
		reg add "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.shtml\UserChoice" /v "ProgId" /t REG_SZ /d "chrlauncherHTML" /f
		reg add "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.xht\UserChoice" /v "ProgId" /t REG_SZ /d "chrlauncherHTML" /f
		reg add "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.xhtml\UserChoice" /v "ProgId" /t REG_SZ /d "chrlauncherHTML" /f

	)
)

pause

