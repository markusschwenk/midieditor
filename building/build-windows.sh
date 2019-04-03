#!/bin/sh

# Make sure that we build in 32-bit.
export OVERRIDE_ARCH=32

mkdir windows32_build
cd windows32_build
mkdir .build
cd .build

$MXE/usr/i686-w64-mingw32.static/qt5/bin/qmake ../../ && make
status=$?
if [ $status -eq 0 ]; then
echo "build was successful"
else
echo "build failed"
exit 1
fi

cd ../../
