#!/bin/sh
BUILD_WINDOWS32=false
BUILD_LINUX32=true
BUILD_LINUX64=false

# setup paths - link to sourceforge repository
QT_WS_PATH="$(realpath ../../)"
echo "qt-workspace: $QT_WS_PATH"
MIDIEDITOR_SRC="/home/markus/midieditor"
echo "src: $MIDIEDITOR_SRC"
MIDIEDITOR_RUN_ENVIRONMENT="/home/markus/midieditor/run_environment"
echo "run environment: $MIDIEDITOR_RUN_ENVIRONMENT"
MIDIEDITOR_BUILD="$QT_WS_PATH/build/midieditor/"
echo "build: $MIDIEDITOR_BUILD"
XBUILD_PATH="$(realpath .)"
echo "x-build: $XBUILD_PATH"
sh build_manual.sh

# build windows 32
if [ "$BUILD_WINDOWS32" = true ]; then
  export OVERRIDE_ARCH=32
  # mxe is a cross-compiler which supports building static windows
  # binaries (32 bit) for qt applications on linux.
  # refer to http://mxe.cc/ for installation instructions.
  mkdir $MIDIEDITOR_BUILD/windows32_build
  cd $MIDIEDITOR_BUILD/windows32_build
  mkdir .build
  cd .build
  PATH_BEFORE="$PATH"
  MXE_DIR="/home/markus/mxe/mxe"
  echo "Building windows 32 binary"
  export PATH=$MXE_DIR/usr/bin:$PATH
  $MXE_DIR/usr/i686-w64-mingw32.static/qt/bin/qmake $MIDIEDITOR_SRC && make
  cd ../
  cp .build/bin/MidiEditor.exe MidiEditor.exe
  export PATH=$PATH_BEFORE
  echo "$MIDIEDITOR_RUN_ENVIRONMENT"
  cp -a $MIDIEDITOR_RUN_ENVIRONMENT/. $MIDIEDITOR_BUILD/windows32_build/
fi

# build linux 64
if [ "$BUILD_LINUX64" = true ]; then
  export OVERRIDE_ARCH=64
  # qmake is already 64 bit
  mkdir $MIDIEDITOR_BUILD/linux64_build
  cd $MIDIEDITOR_BUILD/linux64_build
  mkdir .build
  cd .build
  echo "Building linux 64 binary"
  qmake $MIDIEDITOR_SRC && make
  cd ../
  cp .build/MidiEditor ./MidiEditor
  mkdir graphics
  mkdir metronome
  mkdir assistant
  cp -a $MIDIEDITOR_RUN_ENVIRONMENT/graphics/. graphics
  cp -a $MIDIEDITOR_RUN_ENVIRONMENT/metronome/. metronome
  cp -a $MIDIEDITOR_RUN_ENVIRONMENT/assistant/midieditor-collection.qhc assistant/midieditor-collection.qhc
  cp -a $MIDIEDITOR_RUN_ENVIRONMENT/assistant/midieditor-manual.qch assistant/midieditor-manual.qch
  cp -a $MIDIEDITOR_RUN_ENVIRONMENT/midieditor.ico midieditor.ico
  cp -a $MIDIEDITOR_RUN_ENVIRONMENT/version_info.xml version_info.xml
fi

# build linux 32
if [ "$BUILD_LINUX32" = true ]; then
  mkdir $MIDIEDITOR_BUILD/linux32_build
  
  sudo chroot /var/chroot32 mkdir midieditor-lin32
  sudo chroot /var/chroot32 mkdir midieditor-lin32/build_environment
  sudo chroot /var/chroot32 mkdir midieditor-lin32/src
  sudo chroot /var/chroot32 mkdir midieditor-lin32/build
  sudo mount --bind $XBUILD_PATH /var/chroot32/midieditor-lin32/build_environment
  sudo mount --bind $MIDIEDITOR_SRC /var/chroot32/midieditor-lin32/src
  sudo mount --bind $MIDIEDITOR_BUILD /var/chroot32/midieditor-lin32/build
  
  echo "Building linux 32 binary"
  sudo chroot /var/chroot32 sh midieditor-lin32/build_environment/build_lin32_chroot.sh
  cp $MIDIEDITOR_BUILD/linux32_build/.build/MidiEditor $MIDIEDITOR_BUILD/linux32_build/MidiEditor
  mkdir $MIDIEDITOR_BUILD/linux32_build/graphics
  mkdir $MIDIEDITOR_BUILD/linux32_build/metronome
  mkdir $MIDIEDITOR_BUILD/linux32_build/assistant
  cp -a $MIDIEDITOR_RUN_ENVIRONMENT/graphics/. $MIDIEDITOR_BUILD/linux32_build/graphics
  cp -a $MIDIEDITOR_RUN_ENVIRONMENT/metronome/. $MIDIEDITOR_BUILD/linux32_build/metronome
  cp -a $MIDIEDITOR_RUN_ENVIRONMENT/assistant/midieditor-collection.qhc $MIDIEDITOR_BUILD/linux32_build/assistant/midieditor-collection.qhc
  cp -a $MIDIEDITOR_RUN_ENVIRONMENT/assistant/midieditor-manual.qch $MIDIEDITOR_BUILD/linux32_build/assistant/midieditor-manual.qch
  cp -a $MIDIEDITOR_RUN_ENVIRONMENT/midieditor.ico $MIDIEDITOR_BUILD/linux32_build/midieditor.ico
  cp -a $MIDIEDITOR_RUN_ENVIRONMENT/version_info.xml $MIDIEDITOR_BUILD/linux32_build/version_info.xml
fi