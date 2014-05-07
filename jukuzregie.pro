#-------------------------------------------------
#
# Project created by QtCreator 2013-06-06T22:05:32
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = jukuzregie
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    cambox.cpp \
    jackthread.cpp

HEADERS  += mainwindow.h \
    cambox.h \
    jackthread.h \
    nanoKontrol2.h

FORMS    += mainwindow.ui \
    cambox.ui

LIBS += -ljack

INCLUDEPATH += /usr/include/QtGStreamer
LIBS += -I/usr/include/QtGStreamer -lQtGStreamer-1.0 -lQtGLib-2.0 -lQtGStreamerUi-1.0

OTHER_FILES += \
    settings.txt
