@echo off

set "CHRLAUNCHER_PATH=%~dp0chrlauncher.exe"
set "CHRLAUNCHER_ICON=\"%CHRLAUNCHER_PATH%\",0"
set "CHRLAUNCHER_ARGS=\"%CHRLAUNCHER_PATH%\" /url \"%%1\""

if not exist "%CHRLAUNCHER_PATH%" (

	echo ERROR: "chrlauncher.exe" not found.

) else (

	reg add hklm /f>nul 2>&1

	if ERRORLEVEL 1 (

		echo ERROR: you have no privileges.

	) else (

		regedit /s "%~dp0RegistryCleaner.reg"

		reg add "HKLM\Software\Classes\chrlauncherHTML" /v "" /t REG_SZ /d "chrlauncher document" /f
		reg add "HKLM\Software\Classes\chrlauncherHTML\DefaultIcon" /v "" /t REG_SZ /d "%CHRLAUNCHER_ICON%" /f
		reg add "HKLM\Software\Classes\chrlauncherHTML\shell\open\command" /v "" /t REG_SZ /d "%CHRLAUNCHER_ARGS%" /f

		reg add "HKLM\Software\Classes\chrlauncherURL" /v "" /t REG_SZ /d "chrlauncher document" /f
		reg add "HKLM\Software\Classes\chrlauncherURL" /v "URL Protocol" /t REG_SZ /d "" /f
		reg add "HKLM\Software\Classes\chrlauncherURL\DefaultIcon" /v "" /t REG_SZ /d "%CHRLAUNCHER_ICON%" /f
		reg add "HKLM\Software\Classes\chrlauncherURL\shell\open\command" /v "" /t REG_SZ /d "%CHRLAUNCHER_ARGS%" /f

		reg add "HKLM\Software\RegisteredApplications" /v "chrlauncher" /t REG_SZ /d "Software\Clients\StartMenuInternet\CHRLAUNCHER.EXE\Capabilities" /f

		reg add "HKLM\Software\Clients\StartMenuInternet\CHRLAUNCHER.EXE" /v "" /t REG_SZ /d "chrlauncher" /f

		reg add "HKLM\Software\Clients\StartMenuInternet\CHRLAUNCHER.EXE\DefaultIcon" /v "" /t REG_SZ /d "%CHRLAUNCHER_ICON%" /f

		reg add "HKLM\Software\Clients\StartMenuInternet\CHRLAUNCHER.EXE\shell\open\command" /v "" /t REG_SZ /d "\"%CHRLAUNCHER_PATH%\"" /f

		reg add "HKLM\Software\Clients\StartMenuInternet\CHRLAUNCHER.EXE\Capabilities" /v "ApplicationIcon" /t REG_SZ /d "%CHRLAUNCHER_ICON%" /f
		reg add "HKLM\Software\Clients\StartMenuInternet\CHRLAUNCHER.EXE\Capabilities" /v "ApplicationName" /t REG_SZ /d "chrlauncher" /f
		reg add "HKLM\Software\Clients\StartMenuInternet\CHRLAUNCHER.EXE\Capabilities" /v "ApplicationDescription" /t REG_SZ /d "Portable Chromium Launcher" /f

		reg add "HKLM\Software\Clients\StartMenuInternet\CHRLAUNCHER.EXE\Capabilities\FileAssociations" /v ".htm" /t REG_SZ /d "chrlauncherHTML" /f
		reg add "HKLM\Software\Clients\StartMenuInternet\CHRLAUNCHER.EXE\Capabilities\FileAssociations" /v ".html" /t REG_SZ /d "chrlauncherHTML" /f	
		reg add "HKLM\Software\Clients\StartMenuInternet\CHRLAUNCHER.EXE\Capabilities\FileAssociations" /v ".shtml" /t REG_SZ /d "chrlauncherHTML" /f
		reg add "HKLM\Software\Clients\StartMenuInternet\CHRLAUNCHER.EXE\Capabilities\FileAssociations" /v ".xht" /t REG_SZ /d "chrlauncherHTML" /f
		reg add "HKLM\Software\Clients\StartMenuInternet\CHRLAUNCHER.EXE\Capabilities\FileAssociations" /v ".xhtml" /t REG_SZ /d "chrlauncherHTML" /f

		reg add "HKLM\Software\Clients\StartMenuInternet\CHRLAUNCHER.EXE\Capabilities\URLAssociations" /v "ftp" /t REG_SZ /d "chrlauncherURL" /f
		reg add "HKLM\Software\Clients\StartMenuInternet\CHRLAUNCHER.EXE\Capabilities\URLAssociations" /v "http" /t REG_SZ /d "chrlauncherURL" /f
		reg add "HKLM\Software\Clients\StartMenuInternet\CHRLAUNCHER.EXE\Capabilities\URLAssociations" /v "https" /t REG_SZ /d "chrlauncherURL" /f
		reg add "HKLM\Software\Clients\StartMenuInternet\CHRLAUNCHER.EXE\Capabilities\URLAssociations" /v "mailto" /t REG_SZ /d "chrlauncherURL" /f

	)
)

pause
