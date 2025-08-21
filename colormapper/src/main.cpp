#include <QApplication>
#include "colormapper.h"


int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    ColorMapperWindow window;
    window.show();
    return app.exec();
}
