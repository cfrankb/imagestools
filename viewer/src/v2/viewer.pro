# .pro file (if using qmake)

QT       += core gui widgets

CONFIG += c++20

INCLUDEPATH += ./

DEFINES += USE_QFILE

SOURCES += \
    imageviewer.cpp  \
    main.cpp  \
    mainwindow.cpp \
    #thumbnailgrid.cpp \
    #ziphandler.cpp \
    archwrap.cpp \
    thumbitem.cpp \
    thumbnailview.cpp

HEADERS += \
    imageviewer.h \
    mainwindow.h \
    #thumbnailgrid.h \
    #ziphandler.h \
    archwrap.h \
    imginfo.h \
    thumbnailview.h \
    thumbitem.h

TARGET = ImageViewer
#LIBS += -lz -lminizip -larchive
LIBS += -lz -larchive
