#!/bin/sh
#
# -------------- This script is used to create a debian package. ---------- #
#
# Required environment variables:
#
# MIDIEDITOR_RELEASE_VERSION_ID=2
# MIDIEDITOR_RELEASE_VERSION_STRING=3.1.0
# MIDIEDITOR_PACKAGE_VERSION=1
# MIDIEDITOR_BINARY=relative/path/to/MidiEditor
#

ARCH=amd64
PACKAGE_DIR=midieditor_$MIDIEDITOR_RELEASE_VERSION_STRING-$MIDIEDITOR_PACKAGE_VERSION-$ARCH
BASEDIR=$(dirname "$0")
DATE='date %Y'
DATE='$DATE'
DEPENDS="libc6(>=2.19), qtbase5-dev, qtdeclarative5-dev, libqt5webkit5-dev, libsqlite3-dev, qt5-default, qtmultimedia5-dev, libqt5multimedia5, qttools5-dev-tools, libqt5multimedia5-plugins, libasound2, libgstreamer1.0-0, gstreamer1.0-plugins-base, gstreamer1.0-plugins-good, gstreamer1.0-plugins-bad, gstreamer1.0-plugins-ugly, gstreamer1.0-libav, gstreamer1.0-doc, gstreamer1.0-tools"

# Setup dir structure
mkdir $PACKAGE_DIR
mkdir $PACKAGE_DIR/bin
mkdir $PACKAGE_DIR/usr/
mkdir $PACKAGE_DIR/DEBIAN
mkdir $PACKAGE_DIR/usr/share
mkdir $PACKAGE_DIR/usr/share/applications
mkdir $PACKAGE_DIR/usr/share/pixmaps
mkdir $PACKAGE_DIR/usr/share/midieditor
mkdir $PACKAGE_DIR/usr/share/midieditor/assistant
mkdir $PACKAGE_DIR/usr/share/doc/
mkdir $PACKAGE_DIR/usr/share/doc/midieditor
mkdir $PACKAGE_DIR/usr/lib
mkdir $PACKAGE_DIR/usr/lib/midieditor

# Copy the binary and all runfiles to usr/lib/midieditor
cp $MIDIEDITOR_BINARY $PACKAGE_DIR/usr/lib/midieditor/MidiEditor

# /usr/bin contains midieditor executable
cp $BASEDIR/midieditor/midieditor $PACKAGE_DIR/bin/

# /usr/share/applications gets desktop entry
cp $BASEDIR/midieditor/MidiEditor.desktop $PACKAGE_DIR/usr/share/applications

# /usr/share/pixmaps gets the png file
cp $BASEDIR/midieditor/logo48.png $PACKAGE_DIR/usr/share/pixmaps/midieditor.png

# Copy metronome
cp -R packaging/metronome $PACKAGE_DIR/usr/share/midieditor/metronome

# copyright goes to /usr/share/doc/midieditor (fields will be replaced below)
cp $BASEDIR/midieditor/copyright $PACKAGE_DIR/usr/share/doc/midieditor/copyright

# copy DEBIAN/control (fields will be replaced below)
cp $BASEDIR/midieditor/control $PACKAGE_DIR/DEBIAN/control

# Update fields in DEBIAN/control file and in copyright
sed -i 's/{DATE}/'$(date +%Y-%m-%d)'/g' $PACKAGE_DIR/usr/share/doc/midieditor/copyright
sed -i 's/{VERSION}/'$MIDIEDITOR_RELEASE_VERSION_STRING'/g' $PACKAGE_DIR/DEBIAN/control
sed -i 's/{PACKAGE}/'$MIDIEDITOR_PACKAGE_VERSION'/g' $PACKAGE_DIR/DEBIAN/control
sed -i 's/{ARCHITECURE}/'$ARCH'/g' $PACKAGE_DIR/DEBIAN/control
sed -i 's/{DEPENDS}/'"$DEPENDS"'/g' $PACKAGE_DIR/DEBIAN/control
SIZE=$(du -sb $DIR | cut -f1)
SIZE=$(($SIZE / 1024))
sed -i 's/{SIZE}/'"$SIZE"'/g' $PACKAGE_DIR/DEBIAN/control

# permissions
find $PACKAGE_DIR -type d -exec chmod 755 {} \;
find $PACKAGE_DIR -type f -exec chmod 644 {} \;
chmod +x $PACKAGE_DIR/bin/midieditor
chmod +x $PACKAGE_DIR/usr/lib/midieditor/MidiEditor

fakeroot dpkg-deb --build $PACKAGE_DIR

mkdir releases
mv $PACKAGE_DIR.deb releases/$PACKAGE_DIR.deb
