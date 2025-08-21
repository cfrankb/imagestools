#include <QApplication>
#include <fontconfig/fontconfig.h>
#include "imagesplitter.h"

// Static initialization to ensure FcInit is called before any Qt usage
class FontconfigInitializer {
public:
    FontconfigInitializer() {
        if (!FcInit()) {
            qWarning("Failed to initialize Fontconfig");
        }
    }
    ~FontconfigInitializer() {
        FcFini();
    }
};

static FontconfigInitializer fontconfigInit;

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    ImageSplitter splitter;
    splitter.show();

    int result = app.exec();
    return result;
}
