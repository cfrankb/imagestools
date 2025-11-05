# .pro file (if using qmake)

QT       += core gui widgets

CONFIG += c++20

INCLUDEPATH += ./

DEFINES += USE_QFILE

SOURCES += \
    imageviewer.cpp  \
    main.cpp  \
    mainwindow.cpp \
    thumbnailgrid.cpp \
    ziphandler.cpp

HEADERS += \
    imageviewer.h \
    mainwindow.h \
    thumbnailgrid.h \
    ziphandler.h

TARGET = ImageViewer
LIBS += -lz -lminizip
