@echo off

rem QT folder


if not exist "%QTENV%" (
	echo "QTENV environment not found. (example: set QTENV=C/Qt)"
	pause
	exit
)

setlocal EnableExtensions EnableDelayedExpansion

set "MY_QT=!%QTENV:/=\!"

PATH=%MY_QT%\msys64\mingw32\bin;%MY_QT%\msys64\usr\bin;%PATH%;

set "MY_PROJECT=./build-fluidsynth"

rem get absolute path of the project
for %%I in ("%MY_PROJECT%") do set MY_PROJECT_PATH=%%~fI

echo %MY_PROJECT_PATH%

set "MY_UQT=!MY_QT:\=/!"
set "MY_UPROJECT_PATH=!MY_PROJECT_PATH:\=/!"

:loop0
	cls
        echo Building with MSYS32 fluidsynth library
	echo.
	echo Select an option:
	echo.
	echo 1 - static build
	echo 2 - shared build
        echo 0 - Exit
	echo.
	choice /c 120 /n /m "Press 1, 2 or 0:"

	if ERRORLEVEL 3 (
		echo Goodbye !

	) else if ERRORLEVEL 2 (

	:loop1
		cls
        	echo Building with MSYS32 fluidsynth library
		echo.
		echo Select an option for shared build:
		echo.
		echo 1 - configure
		echo 2 - build
		echo 3 - install
		echo 4 - clean
        	echo 0 - Exit
		echo.
		choice /c 12340 /n /m "Press 1, 2, 3, 4 or 0:"

		if ERRORLEVEL 5 (

			echo Goodbye !

		) else if ERRORLEVEL 4 (

			%MY_QT%\msys64\usr\bin\sh.exe -c "%MY_UPROJECT_PATH%/build-fluidsynth32bits.sh %MY_UQT% ON clean"
			pause
			goto loop1

		) else if ERRORLEVEL 3 (

			%MY_QT%\msys64\usr\bin\sh.exe -c "%MY_UPROJECT_PATH%/build-fluidsynth32bits.sh %MY_UQT% ON install"
			pause
			goto loop1

		) else if ERRORLEVEL 2 (

			%MY_QT%\msys64\usr\bin\sh.exe -c "%MY_UPROJECT_PATH%/build-fluidsynth32bits.sh %MY_UQT% ON build"
			pause
			goto loop1

		) else if ERRORLEVEL 1 (

			%MY_QT%\msys64\usr\bin\sh.exe -c "%MY_UPROJECT_PATH%/build-fluidsynth32bits.sh %MY_UQT% ON config"
			pause
			goto loop1

		)
	)  else if ERRORLEVEL 1 (

	:loop2
		cls
        	echo Building with MSYS32 fluidsynth library
		echo.
		echo Select an option for static build:
		echo.
		echo 1 - configure
		echo 2 - build
		echo 3 - install
		echo 4 - clean
        	echo 0 - Return
		echo.
		choice /c 12340 /n /m "Press 1, 2, 3, 4 or 0:"

		if ERRORLEVEL 5 (

			goto loop0

		) else if ERRORLEVEL 4 (

			%MY_QT%\msys64\usr\bin\sh.exe -c "%MY_UPROJECT_PATH%/build-fluidsynth32bits.sh %MY_UQT% OFF clean"
			pause
			goto loop2

		) else if ERRORLEVEL 3 (

			%MY_QT%\msys64\usr\bin\sh.exe -c "%MY_UPROJECT_PATH%/build-fluidsynth32bits.sh %MY_UQT% OFF install"
			pause
			goto loop2

		) else if ERRORLEVEL 2 (

			%MY_QT%\msys64\usr\bin\sh.exe -c "%MY_UPROJECT_PATH%/build-fluidsynth32bits.sh %MY_UQT% OFF build"
			pause
			goto loop2

		) else if ERRORLEVEL 1 (

			%MY_QT%\msys64\usr\bin\sh.exe -c "%MY_UPROJECT_PATH%/build-fluidsynth32bits.sh %MY_UQT% OFF config"
			pause
			goto loop2

		)
	) else (
		echo error
	)

pause