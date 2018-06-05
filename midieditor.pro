TEMPLATE = app
TARGET = MidiEditor
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT += core \
    gui \
    network \
    xml \
    multimedia
#DEFINES += ENABLE_REMOTE
HEADERS += $$files(**.h, true)
SOURCES += $$files(**.cpp, true)
FORMS += 
RESOURCES += resources.qrc
message(get arch)
message($$(OVERRIDE_ARCH))
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

unix: {
    DEFINES += __LINUX_ALSASEQ__
    DEFINES += __LINUX_ALSA__
    LIBS += -lasound
    CONFIG += release
    OBJECTS_DIR = .tmp
    MOC_DIR = .tmp
}

win32: {
    DEFINES += __WINDOWS_MM__
    LIBS += -lwinmm
    CONFIG += release\
	    assistant
    RC_FILE = midieditor.rc
    OBJECTS_DIR = .tmp
    MOC_DIR = .tmp
    Release:DESTDIR = bin
}

OTHER_FILES += \
    run_environment/version_info.xml
