QT += core gui widgets

# Uncomment if using QZipWriter from Qt5's QtGui module
# QT += gui

# For Qt6 with QZipWriter from Qt5Compat:
# QT += core5compat

CONFIG += c++17 console
CONFIG -= app_bundle

#TEMPLATE = app
TARGET = ImageTrimmer

#INCLUDEPATH += /usr/include/QuaZip-Qt6-1.5

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    ImageProcessor.cpp

HEADERS += \
    mainwindow.h \
    ImageProcessor.h

FORMS += \
    mainwindow.ui

LIBS += -lfontconfig  -lminizip #-lquazip1-qt6

# If you have resources (icons, etc.), add them here:
# RESOURCES += resources.qrc

# Enable warnings
QMAKE_CXXFLAGS += -Wall -Wextra -pedantic
