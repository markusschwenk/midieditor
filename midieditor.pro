
## Obtain release version info. We get this from the environment variables.
## To set them, use
##
## $ export MIDIEDITOR_RELEASE_VERSION_STRING=3.1.4
## $ export MIDIEDITOR_RELEASE_VERSION_ID=4
## $ export MIDIEDITOR_RELEASE_DATE=1970-01-01
##

# use "mingw32-make.exe packing" to create the package

MIDIEDITOR_RELEASE_DATE="2025/12"
MIDIEDITOR_RELEASE_VERSION_ID= "0"
MIDIEDITOR_RELEASE_VERSION_STRING="CUSTOM14b"

#WITH_FLUIDSYNTH=USE_FLUIDSYNTH_STATIC   # static Fluidsynth
WITH_FLUIDSYNTH=USE_FLUIDSYNTH         # dynamic Fluidsynth

# use qmake ... "STATIC_BUILD=1" for static builds
# use qmake ... "DEFINES += __ARCH64__"  for 64bits compilers
# use qmake ... "DEFINES += __ARCH32__"  for 32bits compilers

isEmpty(STATIC_BUILD): STATIC_BUILD = 0
equals(STATIC_BUILD, 1) {
    message(Using static build)

    QMAKE_CXXFLAGS += -static-libgcc -static-libstdc++
    QMAKE_LFLAGS += -static -static-libgcc -static-libstdc++

    QMAKE_CXXFLAGS_RELEASE += -static-libgcc -static-libstdc++
    QMAKE_LFLAGS_RELEASE += -static -static-libgcc -static-libstdc++

    QMAKE_CXXFLAGS_DEBUG += -static-libgcc -static-libstdc++
    QMAKE_LFLAGS_DEBUG += -static -static-libgcc -static-libstdc++

    CONFIG += static
}

TEMPLATE = app
TARGET = MidiEditor

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
isEqual(QT_MAJOR_VERSION, 5): DEFINES += IS_QT5
isEqual(QT_MAJOR_VERSION, 6): DEFINES += IS_QT6

#CONFIG += c++11
CONFIG += c++17

QT += core \
    gui \
    network \
    xml \
    multimedia

#DEFINES += ENABLE_REMOTE
HEADERS += $$files(src/*.h, true) $$files(fluidsynth/*.h, true)

SOURCES += $$files(src/*.cpp, true)

FORMS +=
RESOURCES += resources.qrc

QMAKE_CXXFLAGS += -Wno-overloaded-virtual
QMAKE_CXXFLAGS += -msse2 # added SSE2 support

#message(get arch)
#message($$(OVERRIDE_ARCH))
ARCH_FORCE = $$(OVERRIDE_ARCH)
contains(ARCH_FORCE, 64){
    DEFINES += __ARCH64__
    message(arch forced to 64 bit)
} else {
    contains(ARCH_FORCE, 32){
	message(arch forced to 32 bit)
    } else {
	contains(QMAKE_HOST.arch, x86_64):{
            DEFINES += __ARCH64__
            message(arch recognized as 64 bit)
	} else {
	    message(arch recognized as 32 bit)
        }
    }
}


MIDIEDITOR_RELEASE_VERSION_STRING_QMAKE=$$MIDIEDITOR_RELEASE_VERSION_STRING
DEFINES += MIDIEDITOR_RELEASE_VERSION_STRING_DEF=$$MIDIEDITOR_RELEASE_VERSION_STRING_QMAKE
MIDIEDITOR_RELEASE_VERSION_ID_QMAKE=$$MIDIEDITOR_RELEASE_VERSION_ID
DEFINES += MIDIEDITOR_RELEASE_VERSION_ID_DEF=$$MIDIEDITOR_RELEASE_VERSION_ID_QMAKE
MIDIEDITOR_RELEASE_DATE_QMAKE=$$MIDIEDITOR_RELEASE_DATE
DEFINES += MIDIEDITOR_RELEASE_DATE_DEF=$$MIDIEDITOR_RELEASE_DATE_QMAKE

message(Release version is set to $$MIDIEDITOR_RELEASE_VERSION_STRING_QMAKE)

unix:!macx {
    DEFINES += __LINUX_ALSASEQ__
    DEFINES += __LINUX_ALSA__ CUSTOM_MIDIEDITOR
    LIBS += -lasound
    CONFIG += release
    OBJECTS_DIR = .tmp
    MOC_DIR = .tmp
}

win32: {

    # use "mingw32-make.exe packing" to create the package
    packing.target = packing

    DEFINES += __WINDOWS_MM__
    LIBS += -lwinmm -lole32
    DEFINES += GLIB_STATIC_COMPILATION

    CONFIG += release
    DEFINES += $$WITH_FLUIDSYNTH CUSTOM_MIDIEDITOR #VISIBLE_VST_SYSEX

    ## 64 bits compilers
    contains(DEFINES, __ARCH64__): {

        contains(DEFINES, USE_FLUIDSYNTH_STATIC): {

            DEFINES += USE_FLUIDSYNTH
            LIBS +=  -L$$PWD/lib64/windows_static
            LIBS +=  -lfluidsynth-3 -linstpatch-2 -lgobject-2.0 -lglib-2.0 -lpcre2-8 -lintl -lffi -liconv
            LIBS +=  -lsndfile -lFLAC -lvorbis -logg -lvorbisenc -lmpg123 -lmp3lame -lopus

            packing.target = packing
            packing.commands = $$PWD/packaging/windows/create_windows64-static.bat \
                $$OUT_PWD $$PWD $$MIDIEDITOR_RELEASE_VERSION_STRING
            run.target = run
            run.commands = "$$OUT_PWD/bin/$$TARGET".exe
            QMAKE_EXTRA_TARGETS += packing run

        } else {

            LIBS += $$PWD/lib64/windows/libfluidsynth-3.dll.a

            message("Copying the Fluidsynth DLLs to the destination folder. $$OUT_PWD/bin")

            QMAKE_POST_LINK += $$system("mkdir \"$$OUT_PWD\\bin\" 2>nul")
            contains(DEFINES, USE_FLUIDSYNTH): {
                QMAKE_POST_LINK += $$system("copy /Y \"$$PWD\\lib64\\windows\\*.dll\" \"$$OUT_PWD\\bin\\\" > nul")
            }

            packing.target = packing
            packing.commands = $$PWD/packaging/windows/create_windows64.bat \
                $$OUT_PWD $$PWD $$MIDIEDITOR_RELEASE_VERSION_STRING
            run.target = run
            run.commands = "$$OUT_PWD/bin/$$TARGET".exe
            QMAKE_EXTRA_TARGETS += packing run

        }


    } else {

        contains(DEFINES, USE_FLUIDSYNTH_STATIC): {

            DEFINES += USE_FLUIDSYNTH
            LIBS +=  -L$$PWD/lib/windows_static
            LIBS +=  -lfluidsynth-3 -linstpatch-2 -lgobject-2.0 -lglib-2.0 -lpcre2-8 -lintl -lffi -liconv
            LIBS +=  -lsndfile -lFLAC -lvorbis -logg -lvorbisenc -lmpg123 -lmp3lame -lopus

            packing.target = packing
            packing.commands = $$PWD/packaging/windows/create_windows32-static.bat \
                $$OUT_PWD $$PWD $$MIDIEDITOR_RELEASE_VERSION_STRING
            run.target = run
            run.commands = "$$OUT_PWD/bin/$$TARGET".exe
            QMAKE_EXTRA_TARGETS += packing run

        } else {

            LIBS += $$PWD/lib/windows/libfluidsynth-3.dll.a

            message("Copying the Fluidsynth DLLs to the destination folder. $$OUT_PWD/bin")

            QMAKE_POST_LINK += $$system("mkdir \"$$OUT_PWD\\bin\" 2>nul")
            contains(DEFINES, USE_FLUIDSYNTH): {
                QMAKE_POST_LINK += $$system("copy /Y \"$$PWD\\lib\\windows\\*.dll\" \"$$OUT_PWD\\bin\\\" > nuL")
            }

            packing.target = packing
            packing.commands = $$PWD/packaging/windows/create_windows32.bat \
                $$OUT_PWD $$PWD $$MIDIEDITOR_RELEASE_VERSION_STRING
            run.target = run
            run.commands = "$$OUT_PWD/bin/$$TARGET".exe
            QMAKE_EXTRA_TARGETS += packing run
        }
    }

    RC_FILE = midieditor.rc
    OBJECTS_DIR = .tmp
    MOC_DIR = .tmp
    Release:DESTDIR = bin
}

macx: {
    DEFINES += __MACOSX_CORE__ CUSTOM_MIDIEDITOR
    LIBS += -framework CoreMidi -framework CoreAudio -framework CoreFoundation
    CONFIG += release
    OBJECTS_DIR = .tmp
    MOC_DIR = .tmp
    ICON = midieditor.icns
}

DISTFILES += \
    run_environment/graphics/channelwidget/decouple.png \
    run_environment/graphics/custom/Midicustom2.png
