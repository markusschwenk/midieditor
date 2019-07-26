#!/bin/sh
#
# -------------- This script is used to create the windows installer. ---------- #
#
# Required environment variables:
#
# MIDIEDITOR_RELEASE_VERSION_ID=2
# MIDIEDITOR_RELEASE_VERSION_STRING=3.1.0
# MIDIEDITOR_PACKAGE_VERSION=1
# MIDIEDITOR_BINARY_WINDOWS=relative/path/to/MidiEditor.exe
# INSTALLJAMMER=/path/to/installjammer
#

# Setup folder structure
mkdir MidiEditor-win32
mkdir MidiEditor-win32/win_root/

# Copy binary
cp $MIDIEDITOR_BINARY_WINDOWS MidiEditor-win32/win_root/MidiEditor.exe

# Copy metronome
cp -R packaging/metronome MidiEditor-win32/win_root/metronome

cp -R packaging/windows/windows-installer/. MidiEditor-win32

cd MidiEditor-win32

sh ../$INSTALLJAMMER -DVersion $MIDIEDITOR_RELEASE_VERSION_STRING --build-for-release windows-installer.mpi

cd ..

mkdir releases
cp -a MidiEditor-win32/output/MidiEditor-$MIDIEDITOR_RELEASE_VERSION_STRING-Setup.exe releases/MidiEditor-$MIDIEDITOR_RELEASE_VERSION_STRING-Setup.exe
