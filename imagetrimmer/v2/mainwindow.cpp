#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDragEnterEvent>
#include <QMimeData>
#include <QFileDialog>
#include <QMessageBox>
#include <QBuffer>
#include <QListWidgetItem>
#include <QPainter>
#include <minizip/zip.h>  // Minizip header
//#include <quazip/quazip.h>
//#include <quazip/quazipfile.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setAcceptDrops(true);

    connect(ui->processButton, &QPushButton::clicked, this, &MainWindow::processImages);
    connect(ui->saveButton, &QPushButton::clicked, this, &MainWindow::saveZip);
    connect(ui->selectAllButton, &QPushButton::clicked, this, &MainWindow::selectAll);
    connect(ui->selectNoneButton, &QPushButton::clicked, this, &MainWindow::selectNone);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event) {
    for (const QUrl &url : event->mimeData()->urls()) {
        QImage img(url.toLocalFile());
        if (!img.isNull()) {
            ImagePair pair;
            pair.fileName = QFileInfo(url.toLocalFile()).fileName();
            pair.original = img;
            images.append(pair);
        }
    }
    updatePreview();
}

QImage MainWindow::trimAndCenter(const QImage &img) {
    int left = 0, right = img.width() - 1, top = 0, bottom = img.height() - 1;

    // Trim transparent rows/columns
    auto isRowTransparent = [&](int y) {
        for (int x = 0; x < img.width(); ++x)
            if (qAlpha(img.pixel(x, y)) > 0) return false;
        return true;
    };
    auto isColTransparent = [&](int x) {
        for (int y = 0; y < img.height(); ++y)
            if (qAlpha(img.pixel(x, y)) > 0) return false;
        return true;
    };

    while (top <= bottom && isRowTransparent(top)) ++top;
    while (bottom >= top && isRowTransparent(bottom)) --bottom;
    while (left <= right && isColTransparent(left)) ++left;
    while (right >= left && isColTransparent(right)) --right;

    QImage cropped = img.copy(left, top, right - left + 1, bottom - top + 1);

    // Scale to 16x16 centered
    QImage result(16, 16, QImage::Format_ARGB32);
    result.fill(Qt::transparent);
    QImage scaled = cropped.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    int x = (16 - scaled.width()) / 2;
    int y = (16 - scaled.height()) / 2;

    QPainter p(&result);
    p.drawImage(x, y, scaled);
    p.end();

    return result;
}

void MainWindow::processImages() {
    for (auto &pair : images)
        pair.processed = trimAndCenter(pair.original);
    updatePreview();
}


void MainWindow::saveZip() {
    QString zipPath = QFileDialog::getSaveFileName(this, "Save ZIP", "processed_images.zip", "ZIP Files (*.zip)");
    if (zipPath.isEmpty()) return;

    zipFile zf = zipOpen(zipPath.toStdString().c_str(), APPEND_STATUS_CREATE);
    if (!zf)
    //QuaZip zip(zipPath);
    //if (!zip.open(QuaZip::mdCreate)) {
    {
        QMessageBox::warning(this, "Error", "Could not create ZIP archive.");
        return;
    }

    for (const auto &pair : images) {
        if (!pair.selected) continue;

        zip_fileinfo zfi = {};
        std::string entryName = pair.fileName.toStdString(); //filePath.substr(filePath.find_last_of("/\\") + 1);

        int err = zipOpenNewFileInZip(zf, entryName.c_str(), &zfi,
                                      nullptr, 0, nullptr, 0,
                                      "File added via Minizip",
                                      Z_DEFLATED, Z_DEFAULT_COMPRESSION);
        if (err != ZIP_OK) {
            QMessageBox::warning(this, "Error", "Could not zipOpenNewFileInZip.");
            return;
        }

        QByteArray pngData;
        QBuffer buffer(&pngData);
       // buffer.open(QIODevice::WriteOnly);
        pair.processed.save(&buffer, "PNG");

        err = zipWriteInFileInZip(zf, buffer.data(), buffer.size());
        if (err != ZIP_OK) {
            QMessageBox::warning(this, "Error", "Could not zipWriteInFileInZip.");
            return ;
        }
        //QuaZipFile file(&zip);
//        if (!file.open(QIODevice::WriteOnly, QuaZipNewInfo(pair.fileName + ".png"))) {
  //          QMessageBox::warning(this, "Error", "Could not add file to ZIP.");
    //        continue;
      //  }


//        file.write(pngData);
  //      file.close();
    }
    //zip.close();

    zipCloseFileInZip(zf);
    zipClose(zf, nullptr);
    QMessageBox::information(this, "Done", "ZIP archive saved successfully.");
}

void MainWindow::selectAll() {
    for (auto &pair : images) pair.selected = true;
    updatePreview();
}

void MainWindow::selectNone() {
    for (auto &pair : images) pair.selected = false;
    updatePreview();
}

void MainWindow::updatePreview() {
    ui->listWidget->clear();
    for (int i = 0; i < images.size(); ++i) {
        QListWidgetItem *item = new QListWidgetItem(images[i].fileName);
        item->setCheckState(images[i].selected ? Qt::Checked : Qt::Unchecked);
        ui->listWidget->addItem(item);
    }

    connect(ui->listWidget, &QListWidget::itemChanged, this, [this](QListWidgetItem *item) {
        int idx = ui->listWidget->row(item);
        images[idx].selected = (item->checkState() == Qt::Checked);
    });
}
