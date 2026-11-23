QT += core gui widgets


greaterThan(QT_MAJOR_VERSION, 4): QT += widgets


TARGET = HitboxEditor
TEMPLATE = app


SOURCES += main.cpp editor.cpp hitbox.cpp
HEADERS += editor.h hitbox.h


# Uncomment the following line if you use Qt6 and C++17 features
# QMAKE_CXXFLAGS += -std=c++17
