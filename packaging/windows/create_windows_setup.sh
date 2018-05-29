#!/bin/sh
BUILD_VERSION=$1
INSTALLJAMMER=/home/markus/Downloads/installjammer
mkdir MidiEditor-win32
rm -rf MidiEditor-win32/win_root/
mkdir MidiEditor-win32/win_root/
cp -a ../windows32_build/. MidiEditor-win32/win_root/
rm -rf MidiEditor-win32/win_root/.build
$INSTALLJAMMER/installjammer  -DVersion $BUILD_VERSION --build-for-release windows-installer/windows-installer.mpi
cp -a windows-installer/output/MidiEditor-$BUILD_VERSION-Setup.exe MidiEditor-win32/MidiEditor-$BUILD_VERSION-Setup.exe