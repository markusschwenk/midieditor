#!/bin/sh
qmake -project -v
qmake midieditor.pro
make
status=$?
if [ $status -eq 0 ]; then
echo "build was successful"
else
echo "build failed"
exit 1
fi
