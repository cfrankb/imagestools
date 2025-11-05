// imageviewer.cpp
#include "imageviewer.h"
#include <QMenuBar>
#include <QToolBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QImageReader>
#include <QImageWriter>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QScreen>
#include <QDebug>
#include <QMutex>
#include <minizip/unzip.h>
#include "shared/FrameSet.h"
#include "shared/Frame.h"
#include "shared/FileWrap.h"
#include "sheet.h"
#include "shared/logger.h"

QMutex mMutex;

ImageViewer::ImageViewer(QWidget *parent)
    : QMainWindow(parent), scaleFactor(1.0)
{
    // Create central widget with splitter
    splitter = new QSplitter(Qt::Horizontal, this);
    
    // Create image list (left panel)
    imageList = new QListWidget(this);
    imageList->setMaximumWidth(250);
    imageList->setMinimumWidth(150);
    connect(imageList, &QListWidget::itemClicked, this, &ImageViewer::onImageSelected);
    connect(imageList, &QListWidget::itemSelectionChanged, this, &ImageViewer::onSelectionChange);
    
    // Create middle container for image and info pane
    QWidget *middleContainer = new QWidget(this);
    QVBoxLayout *middleLayout = new QVBoxLayout(middleContainer);
    middleLayout->setContentsMargins(0, 0, 0, 0);
    middleLayout->setSpacing(0);

    // Create image display area (right panel)
    imageLabel = new QLabel;
    imageLabel->setBackgroundRole(QPalette::Base);
    imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabel->setScaledContents(false);
    imageLabel->setAlignment(Qt::AlignCenter);
    
    scrollArea = new QScrollArea;
    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setWidget(imageLabel);
    scrollArea->setVisible(true);
    

    // Create info pane (bottom panel)
    infoPane = new QWidget(this);
    infoPane->setMaximumHeight(300);
    infoPane->setMinimumHeight(200);

    QVBoxLayout *infoLayout = new QVBoxLayout(infoPane);
    infoLabel = new QLabel(tr("No image loaded"));
    infoLabel->setWordWrap(true);
    infoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    infoLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    infoLabel->setStyleSheet("QLabel { padding: 10px; background-color: palette(base); }");

    QScrollArea *infoScrollArea = new QScrollArea;
    infoScrollArea->setWidget(infoLabel);
    infoScrollArea->setWidgetResizable(true);
    infoScrollArea->setFrameShape(QFrame::StyledPanel);

    infoLayout->addWidget(infoScrollArea);
    infoLayout->setContentsMargins(0, 0, 0, 0);

    // Add to middle layout
    middleLayout->addWidget(scrollArea, 1);
    middleLayout->addWidget(infoPane);


    // Add widgets to splitter
    splitter->addWidget(imageList);
    splitter->addWidget(middleContainer);
    //splitter->addWidget(scrollArea);
    splitter->setStretchFactor(1, 1);
    
    setCentralWidget(splitter);
    
    createActions();
    createMenus();
    
    setWindowTitle(tr("Image Viewer"));
    resize(QGuiApplication::primaryScreen()->availableSize() * 3 / 5);
    
    imageList->hide(); // Hide list initially
    infoPane->show(); // Show info pane by default
}

ImageViewer::~ImageViewer()
{
}

void ImageViewer::createActions()
{
    openFileAct = new QAction(tr("Open &File..."), this);
    openFileAct->setShortcut(QKeySequence::Open);
    connect(openFileAct, &QAction::triggered, this, &ImageViewer::openFile);
    
    openFolderAct = new QAction(tr("Open F&older..."), this);
    openFolderAct->setShortcut(tr("Ctrl+Shift+O"));
    connect(openFolderAct, &QAction::triggered, this, &ImageViewer::openFolder);
    
    exportPngAct = new QAction(tr("Export Current to PNG..."), this);
    exportPngAct->setShortcut(tr("Ctrl+E"));
    exportPngAct->setEnabled(false);
    connect(exportPngAct, &QAction::triggered, this, &ImageViewer::exportToPng);
    
    exportSelectedPngAct = new QAction(tr("Export Selected to PNG..."), this);
    exportSelectedPngAct->setShortcut(tr("Ctrl+Shift+E"));
    exportSelectedPngAct->setEnabled(false);
    connect(exportSelectedPngAct, &QAction::triggered, this, &ImageViewer::exportSelectedToPng);
    
    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcut(tr("Ctrl+Q"));
    connect(exitAct, &QAction::triggered, this, &QWidget::close);
    
    zoomInAct = new QAction(tr("Zoom &In (25%)"), this);
    zoomInAct->setShortcut(QKeySequence::ZoomIn);
    zoomInAct->setEnabled(false);
    connect(zoomInAct, &QAction::triggered, this, &ImageViewer::zoomIn);
    
    zoomOutAct = new QAction(tr("Zoom &Out (25%)"), this);
    zoomOutAct->setShortcut(QKeySequence::ZoomOut);
    zoomOutAct->setEnabled(false);
    connect(zoomOutAct, &QAction::triggered, this, &ImageViewer::zoomOut);
    
    fitToWindowAct = new QAction(tr("&Fit to Window"), this);
    fitToWindowAct->setShortcut(tr("Ctrl+F"));
    fitToWindowAct->setEnabled(false);
    connect(fitToWindowAct, &QAction::triggered, this, &ImageViewer::fitToWindow);
    
    actualSizeAct = new QAction(tr("&Actual Size"), this);
    actualSizeAct->setShortcut(tr("Ctrl+0"));
    actualSizeAct->setEnabled(false);
    connect(actualSizeAct, &QAction::triggered, this, &ImageViewer::actualSize);

    toggleInfoAct = new QAction(tr("Show &Info Panel"), this);
    toggleInfoAct->setShortcut(tr("Ctrl+I"));
    toggleInfoAct->setCheckable(true);
    toggleInfoAct->setChecked(true);
    connect(toggleInfoAct, &QAction::triggered, this, &ImageViewer::toggleInfoPane);
}

void ImageViewer::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(openFileAct);
    fileMenu->addAction(openFolderAct);
    fileMenu->addSeparator();
    
    exportMenu = fileMenu->addMenu(tr("&Export"));
    exportMenu->addAction(exportPngAct);
    exportMenu->addAction(exportSelectedPngAct);
    
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);
    
    viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(zoomInAct);
    viewMenu->addAction(zoomOutAct);
    viewMenu->addSeparator();
    viewMenu->addAction(fitToWindowAct);
    viewMenu->addAction(actualSizeAct);

    viewMenu->addSeparator();
    viewMenu->addAction(toggleInfoAct);
}

void ImageViewer::openFile()
{
    QStringList mimeTypeFilters;
    const QList<QByteArray> supportedMimeTypes = QImageReader::supportedMimeTypes();
    for (const QByteArray &mimeTypeName : supportedMimeTypes)
        mimeTypeFilters.append(mimeTypeName);
    mimeTypeFilters.sort();
    
    QFileDialog dialog(this, tr("Open Image File"));
    dialog.setMimeTypeFilters(mimeTypeFilters);
    dialog.selectMimeTypeFilter("image/png");
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    
    if (dialog.exec() == QDialog::Accepted) {
        loadImage(dialog.selectedFiles().constFirst());
        imageList->hide();
        imageFiles.clear();
    }
}

void ImageViewer::openFolder()
{
    static QString curPath = QDir::homePath();
    QString dirPath = QFileDialog::getExistingDirectory(this, tr("Open Folder"),
                                                        curPath,
                                                        QFileDialog::ShowDirsOnly);
    if (!dirPath.isEmpty()) {
        curPath = dirPath;
        populateImageList(dirPath);
        imageList->show();
    }
}

void ImageViewer::populateImageList(const QString &dirPath)
{
    imageList->clear();
    imageFiles.clear();
    
    QDir dir(dirPath);
    QStringList nameFilters;
    const QList<QByteArray> supportedFormats = QImageReader::supportedImageFormats();
    for (const QByteArray &format : supportedFormats) {
        nameFilters << "*." + QString::fromUtf8(format);
    }

    QStringList customFilters{"*.mcx", "*.obl", "*.ima", "*.zip"};
    nameFilters += customFilters;

    imageFiles = dir.entryInfoList(nameFilters, QDir::Files, QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo &fileInfo : imageFiles) {
        QListWidgetItem *item = new QListWidgetItem(fileInfo.fileName());
        item->setData(Qt::UserRole, fileInfo.absoluteFilePath());
        imageList->addItem(item);
    }
    
    if (!imageFiles.isEmpty()) {
        imageList->setCurrentRow(0);
        loadImage(imageFiles.first().absoluteFilePath());
    }
    
    exportSelectedPngAct->setEnabled(!imageFiles.isEmpty());
}

void ImageViewer::onImageSelected(QListWidgetItem *item)
{
    if (item) {
        QString filePath = item->data(Qt::UserRole).toString();
        loadImage(filePath);
    }
}

void ImageViewer::onSelectionChange()
{
    auto item = imageList->currentItem();
    if (item) {
        QString filePath = item->data(Qt::UserRole).toString();
        loadImage(filePath);
    }
}

bool ImageViewer::isCustomFile(const QString &filepath)
{
    return filepath.endsWith(".obl", Qt::CaseInsensitive) || filepath.endsWith(".mcx", Qt::CaseInsensitive)
           || filepath.endsWith(".ima", Qt::CaseInsensitive) || filepath.endsWith(".png", Qt::CaseInsensitive)
            || filepath.endsWith(".zip", Qt::CaseInsensitive);
}

bool ImageViewer::isImageFile(const QString &filepath)
{
    return filepath.endsWith(".obl", Qt::CaseInsensitive) || filepath.endsWith(".mcx", Qt::CaseInsensitive)
    || filepath.endsWith(".ima", Qt::CaseInsensitive) || filepath.endsWith(".png", Qt::CaseInsensitive);
}

bool ImageViewer::isAllSameSize(CFrameSet &set)
{
    for (const auto&frame:set.frames()) {
        if (frame->width() != set[0]->width()
            || frame->height() != set[0]->height())
            return false;
    }
    return true;
}


bool ImageViewer::extractImages(const QString &filePath,  QImage &image, QString &error) {
    CFileWrap file;
    CFrameSet set;

    if (filePath.endsWith(".zip", Qt::CaseInsensitive))
    {
        std::string err;
        if (!readZipFile(filePath.toStdString(), set, err)) {
            error = err.c_str();
            return false;
        }
    }
    else if (file.open(filePath.toStdString(),"rb")) {
        set.clear();
        if (set.extract(file)) {
        }else {
            error = set.getLastError();
        }
        file.close();
    } else {
        error = QString("fail to open %1").arg(filePath);
    }

    if (set.getSize() == 1) {
        CFrame *bitmap = set[0];
        size_t size = bitmap->width()*bitmap->height()*sizeof(uint32_t);
        uint8_t *buf = new uint8_t[size];
        memcpy(buf, bitmap->getRGB().data(), size);
        image = QImage(reinterpret_cast<uint8_t*>(buf), bitmap->width(), bitmap->height(), QImage::Format_RGBX8888);
    } else if (set.getSize()>1){
        CFrame bitmap;
        toSpriteSheet(bitmap, set, isAllSameSize(set)? SpriteSheet::Flag::noflag: SpriteSheet::Flag::sorted);
        size_t size = bitmap.width()*bitmap.height()*sizeof(uint32_t);
        uint8_t *buf = new uint8_t[size];
        memcpy(buf, bitmap.getRGB().data(), size);
        image = QImage(reinterpret_cast<uint8_t*>(buf), bitmap.width(), bitmap.height(), QImage::Format_RGBX8888);
    }
    return error.isEmpty();
}

void ImageViewer::loadImage(const QString &filePath)
{
    QMutexLocker ml(&mMutex);
    QString error;
    QImage image;
    if (isCustomFile(filePath))  {
        extractImages(filePath, image, error);
    } else {
        QImageReader reader(filePath);
        reader.setAutoTransform(true);
        image = reader.read();
        error = reader.errorString();
    }

    if (image.isNull()) {
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
                                tr("Cannot load %1: %2")
                                .arg(QDir::toNativeSeparators(filePath), error));
        return;
    }

    currentImage = QPixmap::fromImage(image);
    currentFilePath = filePath;
    scaleFactor = 1.0;

    updateImageDisplay();
    updateInfoPane();
    
    exportPngAct->setEnabled(true);
    zoomInAct->setEnabled(true);
    zoomOutAct->setEnabled(true);
    fitToWindowAct->setEnabled(true);
    actualSizeAct->setEnabled(true);   

    setWindowTitle(tr("%1 - Image Viewer").arg(QFileInfo(filePath).fileName()));
}

QString ImageViewer::format(const QString &filepath)
{
    const uint8_t pngSig[] = {137, 80, 78, 71, 13, 10, 26, 10};
    const QStringList list{"OBL3", "OBL4", "OBL5", "IMC1", "GE96"};
    QString fmt;
    CFileWrap file;
    if (file.open(filepath.toStdString(),"rb")) {
        char buf[8];
        if (file.read(buf, sizeof(buf))==IFILE_OK) {
            for (const auto&item: list) {
                if (memcmp(buf, item.toStdString().c_str(), item.length())==0) {
                    fmt = item;
                    break;
                }
            }
            if (memcmp(buf, pngSig, sizeof(buf))==0) {
                fmt = "PNG";
            }
        }else {
            qDebug("fail to open %s", filepath.toStdString().c_str());
        }
        file.close();
    }
    return fmt;
}


void ImageViewer::updateInfoPane()
{
    if (currentFilePath.isEmpty() || currentImage.isNull()) {
        infoLabel->setText(tr("No image loaded"));
        return;
    }

    QFileInfo fileInfo(currentFilePath);
    QImageReader reader(currentFilePath);

    QString info;
    info += tr("<b>File Information</b><br>");
    info += tr("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━<br>");
    info += tr("<b>Name:</b> %1<br>").arg(fileInfo.fileName());
    info += tr("<b>Path:</b> %1<br>").arg(fileInfo.absolutePath());
    info += tr("<b>Size:</b> %1<br>").arg(formatFileSize(fileInfo.size()));
    info += tr("<b>Modified:</b> %1<br>").arg(fileInfo.lastModified().toString("yyyy-MM-dd hh:mm:ss"));
    //info += tr("<br>");

    //info += tr("<b>Image Properties</b><br>");
    //info += tr("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━<br>");
    info += tr("<b>Dimensions:</b> %1 × %2 pixels<br>").arg(currentImage.width()).arg(currentImage.height());
    //info += tr("<b>Aspect Ratio:</b> %1<br>").arg(QString::number(static_cast<double>(currentImage.width()) / currentImage.height(), 'f', 3));
    //info += tr("<b>Depth:</b> %1 bits per pixel<br>").arg(currentImage.depth());
    if (isCustomFile(currentFilePath)) {
        info += tr("<b>Format:</b> %1<br>").arg(format(currentFilePath));
    } else {
        info += tr("<b>Format:</b> %1<br>").arg(QString::fromUtf8(reader.format()).toUpper());
    }

    // Color information
    //if (currentImage.isGrayscale()) {
    //    info += tr("<b>Color Mode:</b> Grayscale<br>");
    //} else {
     //   info += tr("<b>Color Mode:</b> Color<br>");
   // }

//    if (currentImage.hasAlphaChannel()) {
  //      info += tr("<b>Alpha Channel:</b> Yes<br>");
   // } else {
    //    info += tr("<b>Alpha Channel:</b> No<br>");
   // }

    /*
    info += tr("<br>");
    info += tr("<b>Display Information</b><br>");
    info += tr("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━<br>");
    info += tr("<b>Current Zoom:</b> %1%<br>").arg(QString::number(scaleFactor * 100, 'f', 1));
    info += tr("<b>Display Size:</b> %1 × %2 pixels<br>")
                .arg(static_cast<int>(currentImage.width() * scaleFactor))
                .arg(static_cast<int>(currentImage.height() * scaleFactor));
    */

    infoLabel->setText(info);
}

QString ImageViewer::formatFileSize(qint64 bytes)
{
    const qint64 KB = 1024;
    const qint64 MB = KB * 1024;
    const qint64 GB = MB * 1024;

    if (bytes >= GB) {
        return QString::number(static_cast<double>(bytes) / GB, 'f', 2) + " GB";
    } else if (bytes >= MB) {
        return QString::number(static_cast<double>(bytes) / MB, 'f', 2) + " MB";
    } else if (bytes >= KB) {
        return QString::number(static_cast<double>(bytes) / KB, 'f', 2) + " KB";
    } else {
        return QString::number(bytes) + " bytes";
    }
}

void ImageViewer::toggleInfoPane()
{
    infoPane->setVisible(!infoPane->isVisible());
    toggleInfoAct->setText(infoPane->isVisible() ? tr("Hide &Info Panel") : tr("Show &Info Panel"));
}


void ImageViewer::updateImageDisplay()
{
    imageLabel->setPixmap(currentImage.scaled(
        currentImage.size() * scaleFactor,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation));
    imageLabel->adjustSize();
}

void ImageViewer::exportToPng()
{
    if (currentImage.isNull()) {
        QMessageBox::warning(this, tr("Export Error"), tr("No image loaded"));
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export to PNG"),
                                                   QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
                                                   tr("PNG Images (*.png)"));
    if (fileName.isEmpty())
        return;
    
    if (!fileName.endsWith(".png", Qt::CaseInsensitive))
        fileName += ".png";
    
    if (currentImage.save(fileName, "PNG")) {
        QMessageBox::information(this, tr("Export Success"),
                               tr("Image exported successfully to:\n%1").arg(fileName));
    } else {
        QMessageBox::warning(this, tr("Export Error"),
                           tr("Failed to export image"));
    }
}

void ImageViewer::exportSelectedToPng()
{
    QList<QListWidgetItem*> selectedItems = imageList->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, tr("Export Error"), tr("No images selected"));
        return;
    }
    
    QString dirPath = QFileDialog::getExistingDirectory(this, tr("Select Export Directory"),
                                                        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
    if (dirPath.isEmpty())
        return;
    
    int successCount = 0;
    for (QListWidgetItem *item : selectedItems) {
        QString sourcePath = item->data(Qt::UserRole).toString();
        QFileInfo fileInfo(sourcePath);
        QString destPath = dirPath + "/" + fileInfo.baseName() + ".png";
        
        QPixmap pixmap(sourcePath);
        if (!pixmap.isNull() && pixmap.save(destPath, "PNG")) {
            successCount++;
        }
    }
    
    QMessageBox::information(this, tr("Export Complete"),
                           tr("Exported %1 of %2 images successfully")
                           .arg(successCount).arg(selectedItems.size()));
}

void ImageViewer::zoomIn()
{
    scaleImage(1.25);
}

void ImageViewer::zoomOut()
{
    scaleImage(0.8);
}

void ImageViewer::fitToWindow()
{
    if (currentImage.isNull())
        return;
    
    QSize viewSize = scrollArea->viewport()->size();
    QSize imageSize = currentImage.size();
    
    double widthRatio = static_cast<double>(viewSize.width()) / imageSize.width();
    double heightRatio = static_cast<double>(viewSize.height()) / imageSize.height();
    
    scaleFactor = std::min(widthRatio, heightRatio);
    updateImageDisplay();
}

void ImageViewer::actualSize()
{
    scaleFactor = 1.0;
    updateImageDisplay();
}

void ImageViewer::scaleImage(double factor)
{
    scaleFactor *= factor;
    updateImageDisplay();
    updateInfoPane();
    
    zoomInAct->setEnabled(scaleFactor < 3.0);
    zoomOutAct->setEnabled(scaleFactor > 0.1);
}


/**
 * @brief Extraxt png images from zip archive
 *
 * @param zipPath
 * @param images
 * @return true
 * @return false
 */
bool ImageViewer::readZipFile(const std::string &zipPath, CFrameSet &images, std::string & error)
{
    unzFile zip = unzOpen(zipPath.c_str());
    if (!zip)
    {
        error = std::string("Cannot open ZIP file: ") + zipPath;
        return false;
    }

    if (unzGoToFirstFile(zip) != UNZ_OK)
    {
        error = "Cannot read first file in ZIP";
        unzClose(zip);
        return false;
    }

    do
    {
        char filename[256];
        unz_file_info fileInfo;

        if (unzGetCurrentFileInfo(zip, &fileInfo, filename, sizeof(filename), nullptr, 0, nullptr, 0) != UNZ_OK)
        {
            error = "Cannot get file info";
            return false;
        }

        bool isDir = (fileInfo.external_fa & 0x10) != 0;
        if (isDir)
            continue;


        if (!isImageFile(filename)) {
            LOGW("Skipping unsupported file:%s [flag=0x%.8lx] in archive %s", filename, fileInfo.flag, zipPath.c_str());
            continue;
        }

        if (fileInfo.compressed_size > 200 * 1024) {
            LOGW("Skipping large file:%s [%ld] in archive %s", filename, fileInfo.compressed_size, zipPath.c_str());
            continue;
        }

        // std::cout << "Found file: " << filename << " (" << fileInfo.uncompressed_size << " bytes)\n";
        if (unzOpenCurrentFile(zip) != UNZ_OK)
        {
            error = std::string("Cannot open file: ") + filename;
            return false;
        }

        std::vector<char> buffer(fileInfo.uncompressed_size);
        int bytesRead = unzReadCurrentFile(zip, buffer.data(), buffer.size());
        if (bytesRead < 0)
        {
            error = std::string("Error reading file: ") + filename;
            return false;
        }
        else
        {
            namespace fs = std::filesystem;
            fs::path tmpDir = fs::temp_directory_path();
            const std::string tmpFile = std::format("{0}/mcxz{1}.bin", tmpDir.c_str(), rand());
            CFileWrap file;
            if (file.open(tmpFile.c_str(), "wb"))
            {
                if (file.write(buffer.data(), buffer.size()) !=IFILE_OK) {
                    error = std::string("cannot write to tmpfile: ") + tmpFile;
                    return false;
                }
                file.close();
                if (file.open(tmpFile.c_str(), "rb"))
                {
                    if (!images.extract(file)) {
                        error = std::string("failed extract from tmpfile: ") + tmpFile;
                        return false;
                    }
                    file.close();
                } else {
                    error = std::string("fail to open tmpfile: ") + tmpFile;
                    return false;
                }
                fs::remove(tmpFile);
            } else {
                error = std::string("cannot create tmpfile: ") + tmpFile;
                return false;
            }
        }
        unzCloseCurrentFile(zip);
    } while (unzGoToNextFile(zip) == UNZ_OK);

    unzClose(zip);
    return images.getSize() != 0;
}
