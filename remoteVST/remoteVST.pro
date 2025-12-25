QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
isEqual(QT_MAJOR_VERSION, 5): DEFINES += IS_QT5
isEqual(QT_MAJOR_VERSION, 6): DEFINES += IS_QT6

CONFIG += c++11

message(Using static build)

QMAKE_CXXFLAGS += -static-libgcc -static-libstdc++
QMAKE_LFLAGS += -static -static-libgcc -static-libstdc++

QMAKE_CXXFLAGS_RELEASE += -static-libgcc -static-libstdc++
QMAKE_LFLAGS_RELEASE += -static -static-libgcc -static-libstdc++

QMAKE_CXXFLAGS_DEBUG += -static-libgcc -static-libstdc++
QMAKE_LFLAGS_DEBUG += -static -static-libgcc -static-libstdc++

CONFIG += static

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    VST/VST.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    VST/VST.h \
    VST/aeffectx.h \
    mainwindow.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES +=

RESOURCES += \
    resources/resources.qrc
