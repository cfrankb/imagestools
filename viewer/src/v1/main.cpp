
// main.cpp
#include "imageviewer.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("Image Viewer");
    QCoreApplication::setOrganizationName("MyCompany");
    QCoreApplication::setApplicationVersion("1.0");
    
    ImageViewer viewer;
    viewer.show();
    
    return app.exec();
}

