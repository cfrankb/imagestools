#include "imagesplitter.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QCheckBox>
#include <QDebug>
#include <QHBoxLayout>
#include <QCryptographicHash>
#include <QBuffer>
#include <QHeaderView>

// Static member initialization
const QRegularExpression SavePreviewDialog::filenameSanitizer("[^\\w.-]");

SavePreviewDialog::SavePreviewDialog(const QList<QPair<QImage, QPair<int, int>>> &tiles, QWidget *parent)
    : QDialog(parent) {
    setupUi(tiles);
}

void SavePreviewDialog::setupUi(const QList<QPair<QImage, QPair<int, int>>> &tiles) {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    previewTable = new QTableWidget(this);
    saveButton = new QPushButton("Save", this);
    cancelButton = new QPushButton("Cancel", this);

    previewTable->setColumnCount(4); // Preview, Confirm, Filename, Coordinates
    previewTable->setHorizontalHeaderLabels({"Preview", "Confirm", "Filename", "Coordinates"});
    previewTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    //previewTable->verticalHeight()->setSectionResizeMode(QHeaderView::Fixed);
    previewTable->setRowCount(tiles.size());

    for (int i = 0; i < tiles.size(); ++i) {
        const QImage &tile = tiles[i].first;
        const QPair<int, int> &coords = tiles[i].second;

        // Preview
        QLabel *previewLabel = new QLabel;
        previewLabel->setPixmap(QPixmap::fromImage(tile));
        previewLabel->setAlignment(Qt::AlignCenter);
        previewTable->setCellWidget(i, 0, previewLabel);
        previewTable->setRowHeight(i, 40);

        // Confirmation Checkbox
        QCheckBox *confirmCheckbox = new QCheckBox;
        confirmCheckbox->setChecked(true); // Default to checked
        previewTable->setCellWidget(i, 1, confirmCheckbox);
        confirmCheckboxes.append(confirmCheckbox);

        // Filename
        QLineEdit *filenameEdit = new QLineEdit;
        filenameEdit->setText(QString("tile_%1_%2.png").arg(coords.first).arg(coords.second));
        previewTable->setCellWidget(i, 2, filenameEdit);
        filenameFields.append(filenameEdit);

        // Coordinates
        QLabel *coordLabel = new QLabel(QString("(%1, %2)").arg(coords.first).arg(coords.second));
        coordLabel->setAlignment(Qt::AlignCenter);
        previewTable->setCellWidget(i, 3, coordLabel);
    }

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addWidget(previewTable);
    mainLayout->addLayout(buttonLayout);

    connect(saveButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    setWindowTitle("Save Tiles Preview");
    resize(700, 400);
}

QList<QPair<QString, QImage>> SavePreviewDialog::getFilenamesAndImages() const {
    QList<QPair<QString, QImage>> result;
    for (int i = 0; i < previewTable->rowCount(); ++i) {
        QCheckBox *confirmCheckbox = qobject_cast<QCheckBox*>(previewTable->cellWidget(i, 1));
        QLineEdit *filenameEdit = qobject_cast<QLineEdit*>(previewTable->cellWidget(i, 2));
        QLabel *previewLabel = qobject_cast<QLabel*>(previewTable->cellWidget(i, 0));
        if (confirmCheckbox && filenameEdit && previewLabel && confirmCheckbox->isChecked()) {
            QString filename = filenameEdit->text().trimmed().replace(filenameSanitizer, "_");
            if (!filename.endsWith(".png", Qt::CaseInsensitive)) {
                filename += ".png";
            }
            const QImage image = previewLabel->pixmap().toImage();
            result.append(qMakePair(filename, image));
            //QSize size = {image.width()/2, image.height()/2};
            //result.append(qMakePair(filename, image.scaled(size)));
        }
    }
    return result;
}

ImageSplitter::ImageSplitter(QWidget *parent) : QMainWindow(parent) {
    setupUi();
}

ImageSplitter::~ImageSplitter() {}

void ImageSplitter::setupUi() {
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    layout = new QVBoxLayout(centralWidget);

    loadButton = new QPushButton("Load PNG Image", this);
    imageLabel = new QLabel(this);
    tileSizeCombo = new QComboBox(this);
    tileSizeCombo->addItem("16x16", 16);
    tileSizeCombo->addItem("24x24", 24);
    tileSizeCombo->addItem("32x32", 32);
    tileSizeCombo->addItem("48x48", 48);
    tileSizeCombo->addItem("64x64", 64);
    tileTable = new QTableWidget(this);
    saveButton = new QPushButton("Save Selected Tiles", this);
    selectAllButton = new QPushButton("Select All", this);
    deselectAllButton = new QPushButton("Deselect All", this);

    imageLabel->setAlignment(Qt::AlignCenter);
    tileTable->setSelectionMode(QAbstractItemView::NoSelection);
    tileTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    tileTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(selectAllButton);
    buttonLayout->addWidget(deselectAllButton);
    layout->addWidget(loadButton);
    layout->addWidget(imageLabel);
    layout->addWidget(tileSizeCombo);
    layout->addWidget(tileTable);
    layout->addLayout(buttonLayout);
    layout->addWidget(saveButton);

    connect(loadButton, &QPushButton::clicked, this, &ImageSplitter::loadImage);
    connect(saveButton, &QPushButton::clicked, this, &ImageSplitter::saveSelectedTiles);
    connect(tileSizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ImageSplitter::updateTileSize);
    connect(selectAllButton, &QPushButton::clicked, this, &ImageSplitter::selectAllTiles);
    connect(deselectAllButton, &QPushButton::clicked, this, &ImageSplitter::deselectAllTiles);

    setWindowTitle("ImageSplitter");
    resize(800, 600);
}

void ImageSplitter::updateTileSize(int index) {
    Q_UNUSED(index);
    currentTileSize = tileSizeCombo->currentData().toInt();
    if (!originalImage.isNull()) {
        splitImage();
    }
}

void ImageSplitter::loadImage() {
    QString filePath = QFileDialog::getOpenFileName(this, "Select PNG Image", "", "PNG Images (*.png)");
    if (filePath.isEmpty()) return;

    if (!originalImage.load(filePath)) {
        QMessageBox::critical(this, "Error", "Failed to load image: " + filePath);
        return;
    }

    int tileSize = currentTileSize;
    if (originalImage.width() % tileSize != 0 || originalImage.height() % tileSize != 0) {
        QMessageBox::warning(this, "Warning", QString("Image dimensions must be divisible by %1.").arg(tileSize));
        originalImage = QImage();
        imageLabel->clear();
        clearTable();
        return;
    }

    imageLabel->setPixmap(QPixmap::fromImage(originalImage).scaled(400, 400, Qt::KeepAspectRatio));
    splitImage();
}

void ImageSplitter::splitImage() {
    clearTable();
    if (originalImage.isNull()) return;

    int tileSize = currentTileSize;
    int cols = originalImage.width() / tileSize;
    int rows = originalImage.height() / tileSize;

    QMap<QByteArray, QPair<QImage, QPair<int, int>>> uniqueTiles;
    int duplicateCount = 0;

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            QImage tile = originalImage.copy(col * tileSize, row * tileSize, tileSize, tileSize);
            
            QByteArray imageData;
            QBuffer buffer(&imageData);
            buffer.open(QIODevice::WriteOnly);
            tile.save(&buffer, "PNG");
            QByteArray hash = QCryptographicHash::hash(imageData, QCryptographicHash::Sha256);

            if (!uniqueTiles.contains(hash)) {
                uniqueTiles.insert(hash, qMakePair(tile, qMakePair(row, col)));
            } else {
                duplicateCount++;
            }
        }
    }

    QList<QPair<QImage, QPair<int, int>>> uniqueTileList = uniqueTiles.values();
    int tileCount = uniqueTileList.size();
    if (tileCount == 0) return;

    int tableSize = qCeil(qSqrt(static_cast<qreal>(tileCount)));
    if (tableSize * tableSize < tileCount) tableSize++;

    tileTable->setRowCount(tableSize);
    tileTable->setColumnCount(tableSize);

    int index = 0;
    for (int row = 0; row < tableSize && index < tileCount; ++row) {
        for (int col = 0; col < tableSize && index < tileCount; ++col) {
            if (index >= tileCount) break;
            const QImage &tile = uniqueTileList[index].first;
            const QPair<int, int> &coords = uniqueTileList[index].second;

            QLabel *tileLabel = new QLabel;
            tileLabel->setPixmap(QPixmap::fromImage(tile).scaled(2*tileSize, 2*tileSize)); // Use original size without scaling
            tileLabel->setAlignment(Qt::AlignCenter);

            QCheckBox *checkBox = new QCheckBox;
            checkBox->setProperty("tileImage", QVariant::fromValue(tile));
            checkBox->setProperty("row", coords.first);
            checkBox->setProperty("col", coords.second);

            QWidget *cellWidget = new QWidget;
            QVBoxLayout *cellLayout = new QVBoxLayout(cellWidget);
            cellLayout->addWidget(tileLabel);
            cellLayout->addWidget(checkBox, 0, Qt::AlignCenter);
            cellLayout->setContentsMargins(0, 0, 0, 0);
            cellLayout->setSpacing(2);

            tileTable->setCellWidget(row, col, cellWidget);
            tileTable->setRowHeight(row, tileSize*2 + 20); // Match tile size + space for checkbox
            tileTable->setColumnWidth(col, tileSize*2 + 20); // Match tile size + space for checkbox

            ++index;
        }
    }

    if (duplicateCount > 0) {
        QMessageBox::information(this, "Duplicates Removed",
                                 QString("Removed %1 duplicate tiles. Displaying %2 unique tiles.")
                                     .arg(duplicateCount)
                                     .arg(tileCount));
    }
}

void ImageSplitter::saveSelectedTiles() {
    if (originalImage.isNull()) {
        QMessageBox::warning(this, "Warning", "No image loaded.");
        return;
    }

    QList<QPair<QImage, QPair<int, int>>> selectedTiles;
    for (int row = 0; row < tileTable->rowCount(); ++row) {
        for (int col = 0; col < tileTable->columnCount(); ++col) {
            QWidget *cellWidget = tileTable->cellWidget(row, col);
            if (!cellWidget) continue;
            QCheckBox *checkBox = cellWidget->findChild<QCheckBox*>();
            if (checkBox && checkBox->isChecked()) {
                QImage tile = checkBox->property("tileImage").value<QImage>();
                int tileRow = checkBox->property("row").toInt();
                int tileCol = checkBox->property("col").toInt();
                selectedTiles.append(qMakePair(tile, qMakePair(tileRow, tileCol)));
            }
        }
    }

    if (selectedTiles.isEmpty()) {
        QMessageBox::warning(this, "Warning", "No tiles selected.");
        return;
    }

    SavePreviewDialog previewDialog(selectedTiles, this);
    bool saved = false;
    while (!saved) {
        if (previewDialog.exec() != QDialog::Accepted) {
            return; // User canceled
        }

        QList<QPair<QString, QImage>> filesToSave = previewDialog.getFilenamesAndImages();

        QString dir = QFileDialog::getExistingDirectory(this, "Select Output Directory");
        if (dir.isEmpty()) return;

        QSet<QString> usedFilenames;
        bool hasDuplicates = false;
        for (const auto &pair : filesToSave) {
            if (usedFilenames.contains(pair.first)) {
                hasDuplicates = true;
                QMessageBox::warning(this, "Error", QString("Duplicate filename: %1. Please edit the filenames.").arg(pair.first));
                break;
            }
            usedFilenames.insert(pair.first);
        }

        if (!hasDuplicates) {
            int savedCount = 0;
            for (const auto &pair : filesToSave) {
                QString fileName = QString("%1/%2").arg(dir).arg(pair.first);
                if (!pair.second.save(fileName, "PNG")) {
                    qDebug() << "Failed to save tile:" << fileName;
                } else {
                    savedCount++;
                }
            }
            QMessageBox::information(this, "Success", QString("Saved %1 tiles to %2").arg(savedCount).arg(dir));
            saved = true;
        }
    }
}

void ImageSplitter::selectAllTiles() {
    for (int row = 0; row < tileTable->rowCount(); ++row) {
        for (int col = 0; col < tileTable->columnCount(); ++col) {
            QWidget *cellWidget = tileTable->cellWidget(row, col);
            if (cellWidget) {
                QCheckBox *checkBox = cellWidget->findChild<QCheckBox*>();
                if (checkBox) {
                    checkBox->setChecked(true);
                }
            }
        }
    }
}

void ImageSplitter::deselectAllTiles() {
    for (int row = 0; row < tileTable->rowCount(); ++row) {
        for (int col = 0; col < tileTable->columnCount(); ++col) {
            QWidget *cellWidget = tileTable->cellWidget(row, col);
            if (cellWidget) {
                QCheckBox *checkBox = cellWidget->findChild<QCheckBox*>();
                if (checkBox) {
                    checkBox->setChecked(false);
                }
            }
        }
    }
}

void ImageSplitter::clearTable() {
    tileTable->clear();
    tileTable->setRowCount(0);
    tileTable->setColumnCount(0);
}
