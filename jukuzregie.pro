#-------------------------------------------------
#
# Project created by QtCreator 2013-06-06T22:05:32
#
#-------------------------------------------------

QT       += core gui phonon network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = jukuzregie
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    cambox.cpp \
    audiometer.cpp \
    jackthread.cpp \
    kradclient.cpp

HEADERS  += mainwindow.h \
    cambox.h \
    audiometer.h \
    jackthread.h \
    kradclient.h \
    nanoKontrol2.h

FORMS    += mainwindow.ui \
    cambox.ui

LIBS += -ljack
