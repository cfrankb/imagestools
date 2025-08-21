// main.cpp
#include "ImageResizerApp.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    ImageResizerApp window;
    window.show();
    
    return app.exec();
}