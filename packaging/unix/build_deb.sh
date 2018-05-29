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
DEPENDS="libc6(>=2.19), libqtcore4(>=4\.8\.5), libqtgui4(>=4\.8\.5), libqt4-network(>=4\.8\.5), libqt4-xml(>=4\.8\.5), libasound2, qt4-dev-tools, libsfml-dev(>=2\.1)"

# Setup dir structure
mkdir $PACKAGE_DIR
mkdir $PACKAGE_DIR/bin
mkdir $PACKAGE_DIR/usr/
mkdir $PACKAGE_DIR/DEBIAN
mkdir $PACKAGE_DIR/usr/share
mkdir $PACKAGE_DIR/usr/share/applications
mkdir $PACKAGE_DIR/usr/share/pixmaps
mkdir $PACKAGE_DIR/usr/share/midieditor
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

# copy version_info.xml to usr/share/midieditor
cp $BASEDIR/midieditor/version_info.xml $PACKAGE_DIR/usr/share/midieditor/version_info.xml

# copyright goes to /usr/share/doc/midieditor (fields will be replaced below)
cp $BASEDIR/midieditor/copyright $PACKAGE_DIR/usr/share/doc/midieditor/copyright

# copy DEBIAN/control (fields will be replaced below)
cp $BASEDIR/midieditor/control $PACKAGE_DIR/DEBIAN/control

# permissions
find $PACKAGE_DIR -type d -exec chmod 755 {} \;
find $PACKAGE_DIR -type f -exec chmod 644 {} \;
chmod +x $PACKAGE_DIR/bin/midieditor
chmod +x $PACKAGE_DIR/usr/lib/midieditor/MidiEditor

# Update fields in DEBIAN/control file and in copyright
sed -i 's/{DATE}/'$(date +%Y-%m-%d)'/g' $PACKAGE_DIR/usr/share/doc/midieditor/copyright
sed -i 's/{VERSION}/'$MIDIEDITOR_RELEASE_VERSION_STRING'/g' $PACKAGE_DIR/DEBIAN/control
sed -i 's/{PACKAGE}/'$MIDIEDITOR_PACKAGE_VERSION'/g' $PACKAGE_DIR/DEBIAN/control
sed -i 's/{ARCHITECURE}/'$ARCH'/g' $PACKAGE_DIR/DEBIAN/control
sed -i 's/{DEPENDS}/'"$DEPENDS"'/g' $PACKAGE_DIR/DEBIAN/control
SIZE=$(du -sb $DIR | cut -f1)
SIZE=$(($SIZE / 1024))
sed -i 's/{SIZE}/'"$SIZE"'/g' $PACKAGE_DIR/DEBIAN/control

# Update fields in version_info.xml
sed -i 's/{VERSION_ID}/'$MIDIEDITOR_RELEASE_VERSION_ID'/g' $PACKAGE_DIR/usr/share/midieditor/version_info.xml
sed -i 's/{VERSION_STRING}/'$MIDIEDITOR_RELEASE_VERSION_STRING'/g' $PACKAGE_DIR/usr/share/midieditor/version_info.xml
sed -i 's/{VERSION_DATE}/'$(date +%Y-%m-%d)'/g' $PACKAGE_DIR/usr/share/midieditor/version_info.xml

fakeroot dpkg-deb --build $PACKAGE_DIR
