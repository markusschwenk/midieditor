#!/bin/sh

##################################################################################################################
## MSYSTOOL.SH CREATE BUILD ENVIRONMENT FOR QT 6.9.0 BY FRANCISCO MUNOZ AKA 'ESTWALD'
##################################################################################################################

#MYCMD=$1

##################################################################################################################
## DETECTION OF MINGW32 OR MINGW64
##################################################################################################################

if [[ "$MSYSTEM" == "MINGW64" ]]; then
    export MYMINGW=/mingw64
elif [[ "$MSYSTEM" == "MINGW32" ]]; then
    export MYMINGW=/mingw32
else
    echo "Unknown environment: $MSYSTEM"
    echo Use MINGW64 or MINGW32 bash
    exit 1
fi

#pacman -Syu --noconfirm
#pacman -Su
#./msys.sh pkg

##################################################################################################################
## SELECT INPUT
##################################################################################################################

UPDATE="false"
clear
echo
echo "Msystool utility - (c) 2025 Francisco Munoz 'Estwald'"

while true; do

	while read -t 0; do read -n 1 -s; done

	if [ "$UPDATE" = "true" ]; then
		echo "Press any key to continue..."
		read -n 1 -s
		clear
		echo
		echo "Msystool utility - (c) 2025 Francisco Munoz 'Estwald'"
		UPDATE="false" 
	
	fi

	echo
	echo
	echo "Choose an action:"
	echo 
	echo "1) Update pkg"
	echo "2) Perform static builds"
	echo "3) Build & Install libinstpatch"
	echo "q) Quit"

	read -p "Enter your choice [1-3 or q]: " choice

	case "$choice" in 
		1)
        		MYCMD="pkg"
        		;;
    		2)
        		MYCMD="static"
        		;;
    		3)
        		MYCMD="libinstpatch"
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



if [[ "$MYCMD" == "pkg" ]]; then

        UPDATE="true"
	set -e

	if [[ "$MYMINGW" == "/mingw64" ]]; then

		pacman -S --needed --noconfirm \
  			base-devel \
  			binutils \
  			bison \
  			diffutils \
  			flex \
  			git \
  			m4 \
  			make \
  			patch \
  			pkgconf \
  			texinfo \
  			unzip \
  			wget \
  			zstd

		pacman -S --needed --noconfirm \
  			mingw-w64-x86_64-gcc \
  			mingw-w64-x86_64-crt \
  			mingw-w64-x86_64-headers \
  			mingw-w64-x86_64-libarchive \
  			mingw-w64-x86_64-libuv \
  			mingw-w64-x86_64-cmake \
  			mingw-w64-x86_64-pkgconf \
  			mingw-w64-x86_64-zstd \
  			mingw-w64-x86_64-jsoncpp \
  			mingw-w64-x86_64-cppdap \
  			mingw-w64-x86_64-glib2 \
  			mingw-w64-x86_64-pcre \
  			mingw-w64-x86_64-gettext \
  			mingw-w64-x86_64-libiconv \
  			mingw-w64-x86_64-libffi \
  			mingw-w64-x86_64-make \
  			ninja cmake python perl

		pacman -S --needed --noconfirm mingw-w64-x86_64-openssl mingw-w64-x86_64-zlib mingw-w64-x86_64-pcre2 \
			mingw-w64-x86_64-ncurses mingw-w64-x86_64-sqlite3 mingw-w64-x86_64-libpng mingw-w64-x86_64-giflib mingw-w64-x86_64-fontconfig

		pacman -S --noconfirm mingw-w64-x86_64-libsndfile mingw-w64-x86_64-flac mingw-w64-x86_64-libvorbis mingw-w64-x86_64-libogg \
			mingw-w64-x86_64-mpg123 mingw-w64-x86_64-lame mingw-w64-x86_64-opus

	elif [[ "$MYMINGW" == "/mingw32" ]]; then

		pacman -S --needed --noconfirm \
  			base-devel \
  			binutils \
  			bison \
  			diffutils \
  			flex \
  			git \
  			m4 \
  			make \
  			patch \
  			pkgconf \
  			texinfo \
  			unzip \
  			wget \
  			zstd

		pacman -S --needed --noconfirm \
  			mingw-w64-i686-gcc \
  			mingw-w64-i686-crt \
  			mingw-w64-i686-headers \
  			mingw-w64-i686-libarchive \
  			mingw-w64-i686-libuv \
  			mingw-w64-i686-cmake \
  			mingw-w64-i686-pkgconf \
  			mingw-w64-i686-zstd \
  			mingw-w64-i686-jsoncpp \
  			mingw-w64-i686-cppdap \
  			mingw-w64-i686-glib2 \
  			mingw-w64-i686-pcre \
  			mingw-w64-i686-gettext \
  			mingw-w64-i686-libiconv \
  			mingw-w64-i686-libffi \
  			mingw-w64-i686-make \
  			ninja cmake python perl

		pacman -S --needed --noconfirm mingw-w64-i686-openssl mingw-w64-i686-zlib mingw-w64-i686-pcre2 \
			mingw-w64-i686-ncurses mingw-w64-i686-sqlite3 mingw-w64-i686-libpng mingw-w64-i686-giflib mingw-w64-i686-fontconfig

		pacman -S --noconfirm mingw-w64-i686-libsndfile mingw-w64-i686-flac mingw-w64-i686-libvorbis mingw-w64-i686-libogg \
			mingw-w64-i686-mpg123 mingw-w64-i686-lame mingw-w64-i686-opus
	else
		echo Use MINGW64 or MINGW32 bash
    		exit 1
	fi
	
elif [[ "$MYCMD" == "static" ]]; then

        UPDATE="true"
	
	# Clone the /lib directory the first time
	if [ ! -d ${MYMINGW}/lib-orig ]; then
		 echo cloning ${MYMINGW}/lib to ${MYMINGW}/lib-orig
		 cp -r ${MYMINGW}/lib ${MYMINGW}/lib-orig
	fi 

	echo "Moving all .dll.a files from ${MYMINGW}/lib to ${MYMINGW}/lib/sharedlibs"

	mkdir -p ${MYMINGW}/lib/sharedlibs

	find ${MYMINGW}/lib -type f -name "*.dll.a" -exec mv {} ${MYMINGW}/lib/sharedlibs/ \;

        mkdir -p ${MYMINGW}/lib/pkgconfig-static
        
        # hack share/pkgconfig
	mkdir -p ${MYMINGW}/share/pkgconfig

        echo "Copying glib-2.0.pc gobject-2.0.pc sndfile.pc from ${MYMINGW}/lib/pkgconfig to ${MYMINGW}/lib/pkgconfig-static/"

	cp ${MYMINGW}/lib/pkgconfig/glib-2.0.pc ${MYMINGW}/lib/pkgconfig-static/
	cp ${MYMINGW}/lib/pkgconfig/gobject-2.0.pc ${MYMINGW}/lib/pkgconfig-static/
	cp ${MYMINGW}/lib/pkgconfig/sndfile.pc ${MYMINGW}/lib/pkgconfig-static/

        echo "Patching glib-2.0.pc"
	sed -i 's|^Libs: -L${libdir} -lglib-2.0 -lintl$|Libs: -L${libdir} -lglib-2.0 -lpcre2-8 -lintl -liconv -lws2_32 -lole32 -lwinmm -lshlwapi -luuid -latomic -lm|' 	${MYMINGW}/lib/pkgconfig-static/glib-2.0.pc
	#sed -i 's|^Libs.private: -lws2_32|Libs.private: -lintl -liconv -lws2_32|' 	${MYMINGW}/lib/pkgconfig-static/glib-2.0.pc

	echo "Patching gobject-2.0.pc"
	sed -i 's|^Libs: -L${libdir} -lgobject-2.0$|Libs: -L${libdir} -lgobject-2.0 -lintl -liconv -lffi|' 	${MYMINGW}/lib/pkgconfig-static/gobject-2.0.pc

	echo "Patching sndfile.pc"
	sed -i 's|^Libs: -L${libdir} -lsndfile$|Libs: -L${libdir} -lsndfile -lflac -lvorbis -lvorbisenc -logg -lopus -lmpg123 -lmp3lame -lshlwapi|' 	${MYMINGW}/lib/pkgconfig-static/sndfile.pc

elif [[ "$MYCMD" == "libinstpatch" ]]; then  

	UPDATE="true"

        pushd .

	# hack share/pkgconfig
	mkdir -p ${MYMINGW}/share/pkgconfig

	cd ~
	if [ ! -d libinstpatch ]; then
        	echo Downloading libinstpatch
		if ! git clone https://github.com/swami/libinstpatch.git; then
			echo "Error: failed to clone repository." >&2
    			exit 1
		fi
		 
	fi 

	echo "Building libinstpatch"

	cd libinstpatch

	rm -rf build
	mkdir build && cd build

	export PKG_CONFIG_PATH="${MYMINGW}/lib/pkgconfig-static:$PKG_CONFIG_PATH"

	set -e

	cmake .. \
        -G"MSYS Makefiles" \
  	-DCMAKE_IGNORE_PATH="${MYMINGW}" \
  	-DCMAKE_BUILD_TYPE=Release \
  	-DCMAKE_INSTALL_PREFIX=${MYMINGW} \
  	-DBUILD_SHARED_LIBS=OFF \
  	-DCMAKE_C_FLAGS="-static-libgcc -static-libstdc++ -static -DGLIB_STATIC_COMPILATION -DGOBJECT_STATIC_COMPILATION -DGMODULE_STATIC_COMPILATION" \
  	-DCMAKE_CXX_FLAGS="-static-libgcc -static-libstdc++ -static -DGLIB_STATIC_COMPILATION -DGOBJECT_STATIC_COMPILATION -DGMODULE_STATIC_COMPILATION" \
  	-DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++ -static"

        echo Make libinstpatch
	make

	echo Install libinstpatch
	make install

	rm -rf build # no more shits!
	popd
	echo "Patching libinstpatch-1.0.pc"
	cp ${MYMINGW}/lib/pkgconfig/libinstpatch-1.0.pc ${MYMINGW}/lib/pkgconfig-static/
	sed -i 's|^Libs: -L${libdir} -linstpatch-2$|Libs: -L${libdir} -linstpatch-2 -lglib-2.0 -lgobject-2.0 -lgthread-2.0 -lsndfile|' 	${MYMINGW}/lib/pkgconfig-static/libinstpatch-1.0.pc

fi

#end of while
done


