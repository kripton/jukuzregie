#-------------------------------------------------
#
# Project created by QtCreator 2013-06-06T22:05:32
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = jukuzregie
TEMPLATE = app

QMAKE_CXXFLAGS_RELEASE += -Ofast -march=native -fopenmp -D_GLIBCXX_PARALLEL -fomit-frame-pointer
QMAKE_LFLAGS_RELEASE += -fopenmp

SOURCES += main.cpp\
        mainwindow.cpp \
    cambox.cpp \
    jackthread.cpp \
    videoappsink.cpp \
    audioappsink.cpp \
    audioappsrc.cpp \
    tcpappsrc.cpp \
    videoappsrc.cpp \
    videoplayer.cpp \
    mediasourcebase.cpp

HEADERS  += mainwindow.h \
    cambox.h \
    jackthread.h \
    nanoKontrol2.h \
    videoappsink.h \
    audioappsink.h \
    audioappsrc.h \
    tcpappsrc.h \
    videoappsrc.h \
    videoplayer.h \
    mediasourcebase.h

FORMS    += mainwindow.ui \
    cambox.ui \
    videoplayer.ui

# Tell qmake to use pkg-config to find QtGStreamer.and jack
CONFIG += link_pkgconfig

# Now tell qmake to link to QtGStreamer and also use its include path and Cflags.
contains(QT_VERSION, ^4\\..*) {
  PKGCONFIG += QtGStreamer-1.0 QtGStreamerUtils-1.0
}
contains(QT_VERSION, ^5\\..*) {
  PKGCONFIG += Qt5GStreamer-1.0 Qt5GStreamerUtils-1.0
}

PKGCONFIG += jack

OTHER_FILES += \
    settings.txt \
    notes
