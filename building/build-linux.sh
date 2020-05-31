#!/bin/sh
qmake -project -v -qt=qt5
qmake -makefile midieditor.pro
make -j $(nproc)
status=$?
if [ $status -eq 0 ]; then
echo "build was successful"
else
echo "build failed"
exit 1
fi
