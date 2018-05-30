#!/bin/sh
qmake -qt=qt5 -project -v
qmake -qt=qt5 midieditor.pro
make
