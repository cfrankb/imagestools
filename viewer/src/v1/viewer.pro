# .pro file (if using qmake)

QT       += core gui widgets

CONFIG += c++20

INCLUDEPATH += ./

DEFINES += USE_QFILE

SOURCES += \
    main.cpp \
    imageviewer.cpp \
    shared/DotArray.cpp \
    shared/FileMem.cpp \
    shared/FileWrap.cpp \
    shared/Frame.cpp \
    shared/FrameSet.cpp \
    shared/helper.cpp \
    shared/logger.cpp \
    shared/PngMagic.cpp \
    shared/qtgui/qfilewrap.cpp \
    sheet.cpp

HEADERS += \
    imageviewer.h \
    shared/DotArray.h \
    shared/FileMem.h \
    shared/FileWrap.h \
    shared/Frame.h \
    shared/FrameSet.h \
    shared/helper.h \
    shared/logger.h \
    shared/PngMagic.h \
    shared/qtgui/qfilewrap.h \
    sheet.h


TARGET = ImageViewer
LIBS += -lz -lminizip
