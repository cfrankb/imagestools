/* ==========================
   main.cpp
   ========================== */

#include "editor.h"

#include <QApplication>
#include <QMainWindow>

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("cfrankb");   // QSettings location for remembered state
    QCoreApplication::setApplicationName("HitboxEditor");
    QMainWindow win;
    Editor *ed = new Editor;
    win.setCentralWidget(ed);
    win.resize(1000, 700);
    win.setWindowTitle("Qt Hitbox Editor");
    win.show();
    return app.exec();
}
