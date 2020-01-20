#!/bin/sh
QMAKE=`which qmake-qt5 2> /dev/null`
QMAKE_ARGS=""
if [ ! -x "$QMAKE" ];then
    QMAKE=qmake
    QMAKE_ARGS="-qt=qt5"
fi
"$QMAKE" $QMAKE_ARGS -project -v
"$QMAKE" $QMAKE_ARGS midieditor.pro
make
status=$?
if [ $status -eq 0 ]; then
echo "build was successful"
else
echo "build failed"
exit 1
fi
