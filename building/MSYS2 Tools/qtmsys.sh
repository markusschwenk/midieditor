#!/bin/sh

##################################################################################################################
## QTMSYS.SH DOWNLOAD AND BUILD UTILITY BY FRANCISCO MUNOZ AKA 'ESTWALD'
##################################################################################################################

#MYCMD=$1

clear
UPDATE="false"
echo
echo "Qtmsys utility - (c) 2025 Francisco Munoz 'Estwald'"
echo

if [[ "$QTENV" == "" ]]; then
        echo
	echo "QTENV not set. The Qt installation path must be specified in the QTENV environment variable (e.g., export QTENV=C:/QT)"
	exit 1
fi

QTENV_MSYS="${QTENV//\\//}"

export MYPATH="$QTENV_MSYS"

mkdir -p $MYPATH

if [ ! -d "$MYPATH" ]; then
	echo
	echo "invalid QTENV or directory does not exist. The Qt installation path must be specified in the QTENV environment variable (e.g., export QTENV=C:/QT)"
	exit 1
fi



##################################################################################################################
## DETECTION OF MINGW32 OR MINGW64
##################################################################################################################

if [[ "$MSYSTEM" == "MINGW64" ]]; then
    export MYMINGW=/mingw64
    set PATH=${MYPATH}/msys32/usr/bin;
    QTLIB_S=mingw_msys64_static
elif [[ "$MSYSTEM" == "MINGW32" ]]; then
    export MYMINGW=/mingw32
    set PATH=${MYPATH}/msys64/usr/bin;
    QTLIB_S=mingw_msys32_static
else
    echo "Unknown environment: $MSYSTEM"
    echo Use MINGW64 or MINGW32 bash
    exit 1
fi

##################################################################################################################
## ENVIRONMENT
##################################################################################################################

export PKG_CONFIG_PATH="${MYMINGW}/lib/pkgconfig-static:$PKG_CONFIG_PATH"


set CXXFLAGS=-static -static-libgcc -static-libstdc++ -DPCRE2_STATIC -DASSIMP_STATIC
set CFLAGS=-static -static-libgcc -msse4.2
set LDFLAGS=-lucrt -static-libgcc -static-libstdc++ 

set QMAKE_CXXFLAGS=-static-libgcc -static-libstdc++ -DPCRE2_STATIC -DASSIMP_STATIC
set QMAKE_LFLAGS=-lucrt -static-libgcc -static-libstdc++

cd  "${MYPATH}"

QT_VERSION=6.9.0
QT_BASENAME=qt-everywhere-src-${QT_VERSION}
QT_URL="https://download.qt.io/official_releases/qt/6.9/${QT_VERSION}/single/${QT_BASENAME}.tar.xz"

##################################################################################################################
## DOWNLOAD QT 6.9.0 FILES
##################################################################################################################

export FILE1=${MYPATH}/${QT_BASENAME}

if [ ! -d "$FILE1" ]; then
## source code not found!. Download

	UPDATE="true"

	if [ ! -f "${MYPATH}/${QT_BASENAME}.tar.xz" ]; then
		 
		echo "Qt ${QT_VERSION} sources were not found. Do you want to download them? (y/n)"
		read -r answer

		if [ "$answer" = "y" ] || [ "$answer" = "Y" ]; then

			echo
			echo "Downloading Qt ${QT_VERSION}..."

			curl -LO "$QT_URL"

			if [ $? -ne 0 ]; then

    	   			echo "Failed to download Qt!!."
				# I can't continue without the source code.
    				exit 1
			fi
		else
			# I can't continue without the source code.
			exit 1
		fi
	fi

	echo
	echo "Extracting Qt ${QT_VERSION}... (Wait patiently until it finishes too much files!)"

	TEMP_DIR="${QT_BASENAME}.tmp"

	if [[  -d "${TEMP_DIR}" ]]; then
		echo
		echo "Deleting old ${QT_BASENAME}.tmp"
		rm -rf "$TEMP_DIR"
	fi

	mkdir "$TEMP_DIR"
	echo
	echo "Extracting.."
	tar -xvf "${QT_BASENAME}.tar.xz" -C "$TEMP_DIR"
	

	if [ $? -eq 0 ]; then
		SUBDIR=$(find "$TEMP_DIR" -mindepth 1 -maxdepth 1 -type d)
		mv "$SUBDIR" "."
		rm -rf "$TEMP_DIR"
    		
		echo "Qt ${QT_VERSION} downloaded and extracted in: ${MYPATH}/$QT_BASENAME"
    
		# delete qtwebengine
        	echo "Deleting qtwebengine sources... (Wait patiently until it finishes, too much files!)"
		rm -rf "${MYPATH}/$QT_BASENAME/qtwebengine"
 
	else
    		echo "Failed to extract the archive."
		rm -rf "$TEMP_DIR"
    		exit 1
	fi

fi


##################################################################################################################
## PATCH QT 6.9.0 FILES
##################################################################################################################

FILE1=${MYPATH}/${QT_BASENAME}/qtquick3d/src/3rdparty/assimp/src/code/Common/DefaultIOStream.cpp

if ! grep -q '#define _CRTIMP' "${FILE1}"; then

UPDATE="true"
echo    
echo "Patch DefaultIOStream.cpp"

chmod u+rw "${FILE1}"	

if grep -q '#include <assimp/DefaultIOStream.h>' "${FILE1}"; then
    sed -i.bak '/#include <assimp\/DefaultIOStream.h>/ {
        s|#include <assimp/DefaultIOStream.h>|#undef _CRTIMP\
#define _CRTIMP //hack (use .a not .dll.a)\
\
#include <assimp/DefaultIOStream.h>|
    }' "${FILE1}"
    echo "DefaultIOStream.cpp Patched"
fi
fi

FILE1=${MYPATH}/${QT_BASENAME}/qtquick3d/tools/balsam/CMakeLists.txt

if ! grep -q 'target_link_libraries(${target_name} PRIVATE ucrt)' "${FILE1}"; then

UPDATE="true"
echo    
echo "Patch balsam/CMakeLists.txt"

chmod u+rw "${FILE1}"

if grep -q 'qt_internal_return_unless_building_tools()' "${FILE1}"; then
    sed -i.bak '/qt_internal_return_unless_building_tools()/ {
        s|qt_internal_return_unless_building_tools()|target_link_libraries(${target_name} PRIVATE ucrt)\
qt_internal_return_unless_building_tools()|
    }' "${FILE1}"
    echo "balsam/CMakeLists.txt Patched"
fi
fi

FILE1=${MYPATH}/${QT_BASENAME}/qtquick3d/tools/balsamui/CMakeLists.txt

if ! grep -q 'target_link_libraries(${target_name} PRIVATE ucrt)' "${FILE1}"; then

UPDATE="true"
echo    
echo "Patch balsamui/CMakeLists.txt"

chmod u+rw "${FILE1}"

if grep -q 'qt_internal_return_unless_building_tools()' "${FILE1}"; then
    sed -i.bak '/qt_internal_return_unless_building_tools()/ {
        s|qt_internal_return_unless_building_tools()|target_link_libraries(${target_name} PRIVATE ucrt)\
qt_internal_return_unless_building_tools()|
    }' "${FILE1}"
    echo "balsam/CMakeLists.txt Patched"
fi
fi

FILE1=${MYPATH}/${QT_BASENAME}/qtquick3d/tools/meshdebug/CMakeLists.txt

if ! grep -q 'target_link_libraries(${target_name} PRIVATE ucrt)' "${FILE1}"; then

UPDATE="true"
echo    
echo "Patch meshdebug/CMakeLists.txt"

chmod u+rw "${FILE1}"

if grep -q 'qt_internal_return_unless_building_tools()' "${FILE1}"; then
    sed -i.bak '/qt_internal_return_unless_building_tools()/ {
        s|qt_internal_return_unless_building_tools()|target_link_libraries(${target_name} PRIVATE ucrt)\
qt_internal_return_unless_building_tools()|
    }' "${FILE1}"
    echo "balsam/CMakeLists.txt Patched"
fi
fi

##################################################################################################################
## SELECT INPUT
##################################################################################################################


while true; do

	while read -t 0; do read -n 1 -s; done

	if [ "$UPDATE" = "true" ]; then
		echo "Press any key to continue..."
		read -n 1 -s
		clear
		echo
		echo "Qtmsys utility - (c) 2025 Francisco Munoz 'Estwald'"
		UPDATE="false" 
	
	fi

	echo
	echo
	echo "Choose an action:"
	echo 
	echo "1) Configure"
	echo "2) Build"
	echo "3) Install"
	echo "q) Quit"

	read -p "Enter your choice [1-3 or q]: " choice

	case "$choice" in 
		1)
        		MYCMD="config"
        		;;
    		2)
        		MYCMD="build"
        		;;
    		3)
        		MYCMD="install"
        		;;
    		q|Q)
        		echo "Exiting."
        		exit 0
        		;;
    		*)

        		echo "Invalid option."
        		MYCMD=""
        		;;
	esac

	echo "You chose: $MYCMD"

##################################################################################################################
## CONFIGURE SOURCES (build in build-qt6-static)
##################################################################################################################

	if [[ "$MYCMD" == "config" ]]; then

		UPDATE="true"

		# very important hack
		unset PKG_CONFIG_PATH
		unset PKG_CONFIG_LIBDIR

		mkdir -p ${QT_VERSION}
		rm -rf build-qt6-static
		mkdir build-qt6-static
		cd build-qt6-static

        	# move lib/cmake (hack to configure QT)
		if [[ ! -d "${MYMINGW}/lib/cmake_backup" && -d "${MYMINGW}/lib/cmake" ]]; then

			mv ${MYMINGW}/lib/cmake ${MYMINGW}/lib/cmake_backup
		fi

		cmake ../${QT_BASENAME} -G "Ninja" \
  		-DCMAKE_MODULE_PATH="" \
  		-DCMAKE_PREFIX_PATH="" \
  		-DCMAKE_BUILD_TYPE=Release \
  		-DBUILD_SHARED_LIBS=OFF \
  		-DCMAKE_INSTALL_PREFIX="${MYPATH}/${QT_VERSION}/${QTLIB_S}" \
  		-DFEATURE_static=ON \
  		-DQT_FEATURE_static=ON \
  		-DFEATURE_openssl_linked=ON \
  		-DFEATURE_openssl=ON \
  		-DQT_FEATURE_openssl_hash=ON \
  		-DFEATURE_freetype=OFF \
  		-DOPENSSL_USE_STATIC_LIBS=TRUE \
  		-DCMAKE_CXX_STANDARD=17 \
  		-DCMAKE_CXX_FLAGS="-static-libgcc -static-libstdc++ -static -DPCRE2_STATIC -DASSIMP_STATIC" \
  		-DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++" \
  		-DQT_FEATURE_sse2=ON \
  		-DQT_FEATURE_sse3=ON \
  		-DQT_FEATURE_avx=ON \
  		-DQT_BUILD_EXAMPLES=OFF -DBUILD_qttools=OFF\
  		-DQT_BUILD_TESTS=OFF \
  		-DQT_BUILD_TOOLS_BY_DEFAULT=OFF \
  		-DCMAKE_PREFIX_PATH=${MYMINGW} \
  		-DCMAKE_FIND_LIBRARY_SUFFIXES=".a" \
  		-DQT_BUILD_SUBMODULES="qtbase;qtdeclarative;qttools;qtmultimedia;qtsvg;qtshadertools;qt5compat" \
  		-DBUILD_qtdatavis3d=OFF \
  		-DBUILD_qtwebengine=OFF \
  		-DBUILD_qtlocation=OFF \
  		-DBUILD_qtpositioning=OFF \
  		-DQT_QMAKE_TARGET_MKSPEC=win32-g++

		# restore lib/cmake (hack to configure QT)
		if [[ -d "${MYMINGW}/lib/cmake_backup" && ! -d "${MYMINGW}/lib/cmake" ]]; then

			mv ${MYMINGW}/lib/cmake_backup ${MYMINGW}/lib/cmake
		fi


##################################################################################################################
## INSTALL QT 6.9.0 (build in build-qt6-static. Install in x:/Qt/6.9.0/mingw_msys32_static or mingw_msys64_static)
##################################################################################################################

	elif [[ "$MYCMD" == "install" ]]; then

   		UPDATE="true"
		cd build-qt6-static
   		ninja install

##################################################################################################################
## COMPILE QT 6.9.0 (build in build-qt6-static)
##################################################################################################################
	elif [[ "$MYCMD" == "build" ]]; then

		UPDATE="true"
   		cd build-qt6-static
   		ninja

	fi

#end of while
done
