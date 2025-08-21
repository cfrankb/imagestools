QT += core gui widgets
TARGET = ImageSplitter
#INCLUDEPATH += /usr/include/QuaZip-Qt6-1.5
SOURCES += main.cpp \
    imagesplitter.cpp
HEADERS += imagesplitter.h
LIBS += -lfontconfig
QMAKE_CFLAGS += -fstack-protector
QMAKE_CXXFLAGS += -fstack-protector

#FORMS += \
#    ../imagetrimmer/mainwindow-.ui
