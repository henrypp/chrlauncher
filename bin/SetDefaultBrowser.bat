@echo off

set "CHRLAUNCHER_PATH=%~dp0chrlauncher.exe"
set "CHRLAUNCHER_ICON=\"%CHRLAUNCHER_PATH%\",0"
set "CHRLAUNCHER_ARGS=\"%CHRLAUNCHER_PATH%\" \"%%1\""

if not exist "%CHRLAUNCHER_PATH%" (

	echo ERROR: "chrlauncher.exe" not found.

) else (

	reg add hklm /f>nul 2>&1

	if ERRORLEVEL 1 (

		echo ERROR: you have no privileges.

	) else (

		regedit /s "%~dp0RegistryCleaner.reg"

		reg add "HKLM\Software\Classes\chrlauncherHTML" /v "" /t REG_SZ /d "Chromium HTML Document" /f
		reg add "HKLM\Software\Classes\chrlauncherHTML\DefaultIcon" /v "" /t REG_SZ /d "%CHRLAUNCHER_ICON%" /f
		reg add "HKLM\Software\Classes\chrlauncherHTML\shell\open\command" /v "" /t REG_SZ /d "%CHRLAUNCHER_ARGS%" /f

		reg add "HKLM\Software\Classes\chrlauncherURL" /v "" /t REG_SZ /d "Chromium URL Protocol" /f
		reg add "HKLM\Software\Classes\chrlauncherURL" /v "EditFlags" /t REG_DWORD /d "2" /f
		reg add "HKLM\Software\Classes\chrlauncherURL" /v "FriendlyTypeName" /t REG_SZ /d "Chromium URL" /f
		reg add "HKLM\Software\Classes\chrlauncherURL" /v "URL Protocol" /t REG_SZ /d "" /f
		reg add "HKLM\Software\Classes\chrlauncherURL\DefaultIcon" /v "" /t REG_SZ /d "%CHRLAUNCHER_ICON%" /f
		reg add "HKLM\Software\Classes\chrlauncherURL\shell\open\command" /v "" /t REG_SZ /d "%CHRLAUNCHER_ARGS%" /f

		reg add "HKLM\Software\RegisteredApplications" /v "chrlauncher" /t REG_SZ /d "Software\Clients\StartMenuInternet\chrlauncher\Capabilities" /f

		reg add "HKLM\Software\Clients\StartMenuInternet\chrlauncher" /v "" /t REG_SZ /d "chrlauncher" /f

		reg add "HKLM\Software\Clients\StartMenuInternet\chrlauncher\DefaultIcon" /v "" /t REG_SZ /d "%CHRLAUNCHER_ICON%" /f

		reg add "HKLM\Software\Clients\StartMenuInternet\chrlauncher\shell\open\command" /v "" /t REG_SZ /d "\"%CHRLAUNCHER_PATH%\"" /f

		reg add "HKLM\Software\Clients\StartMenuInternet\chrlauncher\InstallInfo" /v "IconsVisible" /t REG_DWORD /d "1" /f

		reg add "HKLM\Software\Clients\StartMenuInternet\chrlauncher\Capabilities" /v "ApplicationIcon" /t REG_SZ /d "%CHRLAUNCHER_ICON%" /f
		reg add "HKLM\Software\Clients\StartMenuInternet\chrlauncher\Capabilities" /v "ApplicationName" /t REG_SZ /d "chrlauncher" /f
		reg add "HKLM\Software\Clients\StartMenuInternet\chrlauncher\Capabilities" /v "ApplicationDescription" /t REG_SZ /d "Chromium portable launcher and updater" /f

		reg add "HKLM\Software\Clients\StartMenuInternet\chrlauncher\Capabilities\FileAssociations" /v ".htm" /t REG_SZ /d "chrlauncherHTML" /f
		reg add "HKLM\Software\Clients\StartMenuInternet\chrlauncher\Capabilities\FileAssociations" /v ".html" /t REG_SZ /d "chrlauncherHTML" /f	
		reg add "HKLM\Software\Clients\StartMenuInternet\chrlauncher\Capabilities\FileAssociations" /v ".shtml" /t REG_SZ /d "chrlauncherHTML" /f
		reg add "HKLM\Software\Clients\StartMenuInternet\chrlauncher\Capabilities\FileAssociations" /v ".xht" /t REG_SZ /d "chrlauncherHTML" /f
		reg add "HKLM\Software\Clients\StartMenuInternet\chrlauncher\Capabilities\FileAssociations" /v ".xhtml" /t REG_SZ /d "chrlauncherHTML" /f

		reg add "HKLM\Software\Clients\StartMenuInternet\chrlauncher\Capabilities\StartMenu" /v "StartMenuInternet" /t REG_SZ /d "chrlauncher" /f

		reg add "HKLM\Software\Clients\StartMenuInternet\chrlauncher\Capabilities\URLAssociations" /v "ftp" /t REG_SZ /d "chrlauncherURL" /f
		reg add "HKLM\Software\Clients\StartMenuInternet\chrlauncher\Capabilities\URLAssociations" /v "http" /t REG_SZ /d "chrlauncherURL" /f
		reg add "HKLM\Software\Clients\StartMenuInternet\chrlauncher\Capabilities\URLAssociations" /v "https" /t REG_SZ /d "chrlauncherURL" /f
		reg add "HKLM\Software\Clients\StartMenuInternet\chrlauncher\Capabilities\URLAssociations" /v "mailto" /t REG_SZ /d "chrlauncherURL" /f
		reg add "HKLM\Software\Clients\StartMenuInternet\chrlauncher\Capabilities\URLAssociations" /v "webcal" /t REG_SZ /d "chrlauncherURL" /f
		reg add "HKLM\Software\Clients\StartMenuInternet\chrlauncher\Capabilities\URLAssociations" /v "urn" /t REG_SZ /d "chrlauncherURL" /f
		reg add "HKLM\Software\Clients\StartMenuInternet\chrlauncher\Capabilities\URLAssociations" /v "tel" /t REG_SZ /d "chrlauncherURL" /f
		reg add "HKLM\Software\Clients\StartMenuInternet\chrlauncher\Capabilities\URLAssociations" /v "smsto" /t REG_SZ /d "chrlauncherURL" /f
		reg add "HKLM\Software\Clients\StartMenuInternet\chrlauncher\Capabilities\URLAssociations" /v "sms" /t REG_SZ /d "chrlauncherURL" /f
		reg add "HKLM\Software\Clients\StartMenuInternet\chrlauncher\Capabilities\URLAssociations" /v "nntp" /t REG_SZ /d "chrlauncherURL" /f
		reg add "HKLM\Software\Clients\StartMenuInternet\chrlauncher\Capabilities\URLAssociations" /v "news" /t REG_SZ /d "chrlauncherURL" /f
		reg add "HKLM\Software\Clients\StartMenuInternet\chrlauncher\Capabilities\URLAssociations" /v "mms" /t REG_SZ /d "chrlauncherURL" /f
		reg add "HKLM\Software\Clients\StartMenuInternet\chrlauncher\Capabilities\URLAssociations" /v "irc" /t REG_SZ /d "chrlauncherURL" /f

	)
)

pause
