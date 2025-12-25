@echo off

rem QT folder

if not exist "%QTENV%" (
	echo "QTENV environment not found. (example: set QTENV=C/Qt)" properly.
	pause
	exit
)

setlocal EnableExtensions EnableDelayedExpansion

set "MY_QT=!QTENV:/=\!"

echo QTENV=%MY_QT%
rem MSYS2 tools, compilers

set "MY_TOOLS=%MY_QT%\msys64\usr\bin"
set "MY_COMPILER=%MY_QT%\msys64\mingw64\bin"

rem QT lib, project, build...

set "MY_QT_LIB=%MY_QT%\6.9.0\mingw_msys64_static"
set "MY_PROJECT=..\midieditor.pro"
set "MY_BUILD=../build/build-Midieditor-64bits-static"
set PARAMS="DEFINES += __ARCH64__" "STATIC_BUILD = 1"


if not exist "%MY_QT_LIB%" (
	echo QT lib not found. Please edit this file to configure it properly.
	pause
	exit
)

if not exist "%MY_COMPILER%" (
	echo Compiler not found. Please edit this file to configure it properly.
	pause
	exit
)

if not exist "%MY_PROJECT%" (
	echo Midieditor.pro not found. Please edit this file to configure it properly.
	pause
	exit
)

rem get absolute path of the project
for %%I in ("%MY_PROJECT%") do set MY_PROJECT_PATH=%%~fI

rem echo %MY_PROJECT_PATH%

set "MY_UCOMPILER=!MY_COMPILER:\=/!"
set "QTDIR=!MY_QT_LIB:\=/!"
set PATH=%MY_COMPILER%;%MY_QT_LIB%\bin;%MY_TOOLS%;%PATH%

:loop
	echo.
        echo.
        echo Building with MSYS %PARAM%
	echo.
	echo Select an option:
	echo.
	echo 1 - Configure
	echo 2 - Build
	echo 3 - Run
	echo 4 - Install
	echo 5 - Clean
        echo 0 - Exit
	echo.
	choice /c 123450 /n /m "Press 1, 2, 3, 4, 5 or 0:"

	if ERRORLEVEL 6 (
		echo Goodbye !

	) else if ERRORLEVEL 5 (

		pushd "%MY_BUILD%
		%MY_COMPILER%\mingw32-make distclean
                popd
		goto loop
        
	) else if ERRORLEVEL 4 (

		pushd "%MY_BUILD%
		%MY_COMPILER%\mingw32-make -f Makefile.Release packing -j8
                popd
                goto loop

	) else if ERRORLEVEL 3 (

		pushd "%MY_BUILD%
		%MY_COMPILER%\mingw32-make -f Makefile.Release run -j8
                
                popd
                goto loop

	) else if ERRORLEVEL 2 (

		pushd "%MY_BUILD%
		%MY_COMPILER%\mingw32-make -f Makefile.Release -j8
		popd
		goto loop

	)  else if ERRORLEVEL 1 (

                if not exist "%MY_BUILD%" mkdir "%MY_BUILD%"
		pushd "%MY_BUILD%"
		%MY_QT_LIB%\bin\qmake.exe "%MY_PROJECT_PATH%" -spec win32-g++ ^
		"CONFIG+=qtquickcompiler" %PARAMS% ^
		&& %MY_COMPILER%\mingw32-make.exe qmake_all
		popd
		goto loop
	) 



endlocal
pause
