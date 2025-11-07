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
    ziphandler.cpp \
    archwrap.cpp

HEADERS += \
    imageviewer.h \
    mainwindow.h \
    thumbnailgrid.h \
    ziphandler.h \
    archwrap.h \
    imginfo.h

TARGET = ImageViewer
LIBS += -lz -lminizip -larchive
