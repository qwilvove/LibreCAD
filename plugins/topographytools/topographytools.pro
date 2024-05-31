#-------------------------------------------------
#
# Project created by QtCreator 2011-03-22T19:33:11
#
#-------------------------------------------------

QT       += widgets
TEMPLATE = lib
CONFIG += plugin
VERSION = 0.0.1
TARGET = $$qtLibraryTarget(topographytools)

GENERATED_DIR = ../../generated/plugin/topographytools
# Use common project definitions.
include(../../common.pri)

# For plugins
INCLUDEPATH    += ../../librecad/src/plugins

SOURCES += \
    topographytools.cpp \
    tt_dialogadd.cpp \
    tt_dialogedit.cpp \
    tt_dialogmain.cpp

HEADERS += \
    topographytools.h \
    tt_dialogadd.h \
    tt_dialogedit.h \
    tt_dialogmain.h

# Installation Directory
win32 {
        DESTDIR = ../../windows/resources/plugins
}
unix {
    macx {
        DESTDIR = ../../LibreCAD.app/Contents/Resources/plugins
    }
    else {
        DESTDIR = ../../unix/resources/plugins
    }
}

FORMS += \
    tt_dialogadd.ui \
    tt_dialogedit.ui \
    tt_dialogmain.ui

RESOURCES += \
    icons/icons.qrc
