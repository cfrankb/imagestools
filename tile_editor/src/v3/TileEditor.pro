QT += core gui widgets

CONFIG += c++17
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app
TARGET = TileEditor

SOURCES += \
    main.cpp \
    tileitem.cpp \
    tilescene.cpp \
    mainwindow.cpp

HEADERS += mainwindow.h \
    tileitem.h \
    tilescene.h
