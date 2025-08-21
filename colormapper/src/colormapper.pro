QT += widgets core
CONFIG += c++20
INCLUDEPATH += shared/
SOURCES += main.cpp \
    colormapper.cpp \
    shared/DotArray.cpp \
    shared/FileWrap.cpp \
    shared/Frame.cpp \
    shared/FrameSet.cpp \
    shared/PngMagic.cpp \
    shared/helper.cpp \
    shared/qtgui/qfilewrap.cpp \
    shared/qtgui/qthelper.cpp \
    customwidget.cpp

HEADERS += \
    colormapper.h \
    shared/CRC.h \
    shared/DotArray.h \
    shared/FileWrap.h \
    shared/Frame.h \
    shared/FrameSet.h \
    shared/IFile.h \
    shared/ISerial.h \
    shared/PngMagic.h \
    shared/glhelper.h \
    shared/helper.h \
    shared/qtgui/cheat.h \
    shared/qtgui/qfilewrap.h \
    shared/qtgui/qthelper.h \
    customwidget.h

LIBS += -lz
