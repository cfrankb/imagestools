QT += core widgets

CONFIG += c++17

TARGET = ImageResizerApp
TEMPLATE = app

#INCLUDEPATH += /usr/include/QuaZip-Qt6-1.5

SOURCES += \
    main.cpp \
    ImageItem.cpp \
    ResizeThread.cpp \
    ImageResizerApp.cpp

HEADERS += \
    ImageResizerApp.h \
    ImageItem.h \
    ImageInfo.h \
    ResizeThread.h

#LIBS += -lfontconfig -lquazip1-qt6
LIBS += -lfontconfig -lminizip
