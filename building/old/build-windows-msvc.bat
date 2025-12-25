@echo off

rem Make sure that we build in 32-bit.
set OVERRIDE_ARCH=32

set MIDIEDITOR_RELEASE_DATE=%date:~4%

qmake -project -v || exit /b 1
qmake midieditor.pro || exit /b 1
nmake /nologo

