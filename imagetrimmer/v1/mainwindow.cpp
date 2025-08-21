#include "mainwindow.h"

#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QMimeData>
#include <QImageReader>
#include <QFileDialog>
#include <QPainter>
#include <QBuffer>
#include <QMessageBox>
#include <QDragEnterEvent>

#include <quazip/quazip.h>
#include <quazip/quazipfile.h>
#include <quazip/quazipnewinfo.h>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    QWidget *central = new QWidget;
    setCentralWidget(central);

    // Left: thumb list
    thumbList = new QListWidget;
    thumbList->setViewMode(QListView::IconMode);
    thumbList->setIconSize(QSize(64,64));
    thumbList->setResizeMode(QListView::Adjust);
    thumbList->setMovement(QListView::Static);
    thumbList->setMaximumWidth(200);

    // Right: split previews (Before / After)
    beforePreview = new QLabel;
    beforePreview->setAlignment(Qt::AlignCenter);
    beforePreview->setMinimumSize(300,300);
    beforePreview->setStyleSheet("background: #222;");

    afterPreview = new QLabel;
    afterPreview->setAlignment(Qt::AlignCenter);
    afterPreview->setMinimumSize(300,300);
    afterPreview->setStyleSheet("background: #222;");

    QVBoxLayout *previewCol = new QVBoxLayout;
    QLabel *lblBefore = new QLabel("Before");
    lblBefore->setAlignment(Qt::AlignCenter);
    QLabel *lblAfter = new QLabel("After");
    lblAfter->setAlignment(Qt::AlignCenter);

    QHBoxLayout *topRow = new QHBoxLayout;
    topRow->addWidget(lblBefore);
    topRow->addWidget(lblAfter);

    QHBoxLayout *previewRow = new QHBoxLayout;
    previewRow->addWidget(beforePreview);
    previewRow->addWidget(afterPreview);

    previewCol->addLayout(topRow);
    previewCol->addLayout(previewRow);

    // Buttons
    processBtn = new QPushButton("Process");
    saveBtn = new QPushButton("Save ZIP");
    saveBtn->setEnabled(false);
    showAfterCheckbox = new QCheckBox("Show Processed (After)");
    showAfterCheckbox->setChecked(true);

    QHBoxLayout *controls = new QHBoxLayout;
    controls->addWidget(processBtn);
    controls->addWidget(saveBtn);
    controls->addWidget(showAfterCheckbox);
    controls->addStretch();

    // Main layout
    QHBoxLayout *mainRow = new QHBoxLayout;
    mainRow->addWidget(thumbList);
    QWidget *rightWidget = new QWidget;
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->addLayout(previewCol);
    rightLayout->addLayout(controls);
    mainRow->addWidget(rightWidget);

    central->setLayout(mainRow);

    setAcceptDrops(true);

    connect(processBtn, &QPushButton::clicked, this, &MainWindow::processImages);
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::saveZip);
    connect(showAfterCheckbox, &QCheckBox::toggled, this, [this](bool checked){
        showAfter = checked;
        updatePreview();
    });

    // clicking a thumbnail shows that image in previews
    connect(thumbList, &QListWidget::currentRowChanged, this, [this](int row){
        if (row < 0 || row >= originals.size()) return;
        QImage img = originals[row];
        beforePreview->setPixmap(QPixmap::fromImage(img).scaled(beforePreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        if (!processed.isEmpty() && row < processed.size()) {
            afterPreview->setPixmap(QPixmap::fromImage(processed[row]).scaled(afterPreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            afterPreview->clear();
        }
    });
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event) {
    for (const QUrl &u : event->mimeData()->urls()) {
        QString path = u.toLocalFile();
        if (path.isEmpty()) continue;
        QImageReader reader(path);
        reader.setAutoTransform(true);
        QImage img = reader.read();
        if (!img.isNull()) {
            QImage rgba = img.convertToFormat(QImage::Format_RGBA8888);
            originals.append(rgba);
            QListWidgetItem *it = new QListWidgetItem(QIcon(QPixmap::fromImage(rgba).scaled(64,64,Qt::KeepAspectRatio,Qt::SmoothTransformation)),
                                                      QFileInfo(path).fileName());
            thumbList->addItem(it);
        }
    }
    if (!originals.isEmpty()) {
        thumbList->setCurrentRow(0);
    }
    updatePreview();
}

QImage MainWindow::trimAndCenter(const QImage &src) {
    // Find bounding box of non-transparent pixels
    int w = src.width(), h = src.height();
    int minX = w, minY = h, maxX = -1, maxY = -1;
    for (int y=0;y<h;++y) {
        const QRgb *row = reinterpret_cast<const QRgb*>(src.constScanLine(y));
        for (int x=0;x<w;++x) {
            if (qAlpha(row[x]) > 0) {
                minX = qMin(minX, x);
                minY = qMin(minY, y);
                maxX = qMax(maxX, x);
                maxY = qMax(maxY, y);
            }
        }
    }

    QImage result(16, 16, QImage::Format_RGBA8888);
    result.fill(Qt::transparent);
    if (maxX < minX || maxY < minY) {
        // empty -> return transparent 16x16
        return result;
    }

    QRect crop(minX, minY, maxX - minX + 1, maxY - minY + 1);
    QImage trimmed = src.copy(crop);

    // Scale trimmed to fit within 16x16 but preserve aspect ratio
    QSize scaled = trimmed.size();
    scaled.scale(QSize(16,16), Qt::KeepAspectRatio);

    QPainter p(&result);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    QPoint pos((16 - scaled.width())/2, (16 - scaled.height())/2);
    QRect target(pos, scaled);
    p.drawImage(target, trimmed);
    p.end();
    return result;
}

void MainWindow::processImages() {
    processed.clear();
    processed.reserve(originals.size());
    for (const QImage &img : originals) {
        processed.append(trimAndCenter(img));
    }
    saveBtn->setEnabled(!processed.isEmpty());
    updatePreview();
}

void MainWindow::updatePreview() {
    int row = thumbList->currentRow();
    if (row < 0 && !originals.isEmpty()) {
        thumbList->setCurrentRow(0);
        row = 0;
    }
    // If a current row exists, show that image; otherwise, if multiple, show first combined
    if (row >= 0 && row < originals.size()) {
        QImage b = originals[row];
        beforePreview->setPixmap(QPixmap::fromImage(b).scaled(beforePreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        if (!processed.isEmpty() && row < processed.size()) {
            if (showAfter) {
                afterPreview->setPixmap(QPixmap::fromImage(processed[row]).scaled(afterPreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            } else {
                afterPreview->clear();
            }
        } else {
            afterPreview->clear();
        }
    } else {
        // If no selection, show first images concatenated vertically
        if (!originals.isEmpty()) {
            QImage b = originals.front();
            beforePreview->setPixmap(QPixmap::fromImage(b).scaled(beforePreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else beforePreview->clear();

        if (!processed.isEmpty() && showAfter) {
            afterPreview->setPixmap(QPixmap::fromImage(processed.front()).scaled(afterPreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else afterPreview->clear();
    }
}

void MainWindow::saveZip() {
    if (processed.isEmpty()) {
        QMessageBox::information(this, "No images", "No processed images to save. Press Process first.");
        return;
    }

    QString out = QFileDialog::getSaveFileName(this, "Save ZIP", QString(), "ZIP Archive (*.zip)");
    if (out.isEmpty()) return;

    QuaZip zip(out);
    if (!zip.open(QuaZip::mdCreate)) {
        QMessageBox::critical(this, "ZIP Error", "Failed to create ZIP archive.");
        return;
    }

    for (int i=0;i<processed.size();++i) {
        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        processed[i].save(&buffer, "PNG");
        buffer.close();

        QString fname = QString("image_%1.png").arg(i+1);
        QuaZipFile zf(&zip);
        QuaZipNewInfo zi(fname);//, QDateTime::currentDateTime());
        if (!zf.open(QIODevice::WriteOnly, zi)) {
            QMessageBox::warning(this, "ZIP Error", QString("Failed to add %1 to zip").arg(fname));
            continue;
        }
        zf.write(ba);
        zf.close();
    }

    zip.close();
    if (zip.getZipError() != UNZ_OK) {
        QMessageBox::warning(this, "ZIP Error", "Some error occurred while writing the zip (check permissions).");
        return;
    }
    QMessageBox::information(this, "Saved", "ZIP archive created successfully.");
}
