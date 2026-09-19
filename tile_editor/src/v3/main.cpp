#include <QApplication>
#include "mainwindow.h"

// Qt Tile Editor (single-file demo)
// - Open image and split into tiles (default 16x16)
// - Left-click-drag to select multiple tiles (Ctrl to add)
// - Right-click opens context menu for selection: set Type (0..3), set Next tile, set animation speed
// - Select Tile button in properties panel: enters a select mode where the user can click on a single tile from the view
// - Selected tiles highlighted; non-background tiles get colored overlay
// - Per-tile granular quadrant flags (2x2): semi-transparent purple overlay,
//   checkbox UI in the properties panel, persisted in the tileset JSON
// - Zoom presets: 100%, 200%, 400%
// - Save/Load tileset metadata (JSON): image path, tile size, per-tile type/next/speed

// Build with Qt6/Qt5 (qmake/CMake/Qt Creator)

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    MainWindow w;
    w.resize(900, 700);
    w.show();
    return app.exec();
}
