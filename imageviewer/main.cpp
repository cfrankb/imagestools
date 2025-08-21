#include <QApplication>
#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QPixmap>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>
#include <QPushButton>
#include <QDir>

class ImageGridViewer : public QWidget {
public:
    ImageGridViewer() {
        auto* layout = new QVBoxLayout(this);
        auto* button = new QPushButton("Load PNG Folder");
        connect(button, &QPushButton::clicked, this, &ImageGridViewer::loadImages);

        scrollArea = new QScrollArea;
        scrollWidget = new QWidget;
        gridLayout = new QGridLayout(scrollWidget);
        scrollWidget->setLayout(gridLayout);
        scrollArea->setWidget(scrollWidget);
        scrollArea->setWidgetResizable(true);

        layout->addWidget(button);
        layout->addWidget(scrollArea);
        setLayout(layout);
        resize(1000, 700);
    }

private:
    QScrollArea* scrollArea;
    QWidget* scrollWidget;
    QGridLayout* gridLayout;

    void loadImages() {
        QString dirPath = QFileDialog::getExistingDirectory(this, "Select Folder with PNGs");
        if (dirPath.isEmpty()) return;

        QDir dir(dirPath);
        QStringList pngFiles = dir.entryList(QStringList() << "*.png", QDir::Files);

        // Clear previous images
        QLayoutItem* item;
        while ((item = gridLayout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }

        int row = 0, col = 0;
        const int maxCols = 4;

        for (const QString& fileName : pngFiles) {
            QString fullPath = dir.filePath(fileName);
            QPixmap pixmap(fullPath);
            QSize size = pixmap.size();

            auto* imageLabel = new QLabel;
            imageLabel->setPixmap(pixmap.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            imageLabel->setAlignment(Qt::AlignCenter);

            auto* filenameLabel = new QLabel(fileName);
            filenameLabel->setAlignment(Qt::AlignCenter);

            auto* sizeLabel = new QLabel(QString("%1 × %2").arg(size.width()).arg(size.height()));
            sizeLabel->setAlignment(Qt::AlignCenter);

            auto* container = new QWidget;
            auto* vbox = new QVBoxLayout(container);
            vbox->addWidget(imageLabel);
            vbox->addWidget(filenameLabel);
            vbox->addWidget(sizeLabel);
            container->setLayout(vbox);

            gridLayout->addWidget(container, row, col);
            col++;
            if (col >= maxCols) {
                col = 0;
                row++;
            }
        }
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    ImageGridViewer viewer;
    viewer.setWindowTitle("Qt6 PNG Grid Viewer");
    viewer.show();
    return app.exec();
}
