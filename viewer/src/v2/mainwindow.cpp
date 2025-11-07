#include "mainwindow.h"
#include <QFileDialog>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QApplication>
#include <QKeyEvent>

static const QStringList IMAGE_EXTS = {"png", "jpg", "jpeg", "bmp", "gif", "webp", "zip", "svg"};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    resize(1000, 700);

    auto central = new QWidget(this);
    auto layout = new QHBoxLayout(central);

    QSplitter *splitter = new QSplitter(central);
    m_listWidget = new QListWidget(splitter);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listWidget->setMinimumWidth(250);

    m_rightStack = new QStackedWidget(splitter);
    m_imageViewer = new ImageViewer;
    m_thumbGrid = new ThumbnailGrid;
    m_rightStack->addWidget(m_imageViewer); // index 0
    m_rightStack->addWidget(m_thumbGrid);   // index 1

    splitter->addWidget(m_listWidget);
    splitter->addWidget(m_rightStack);
    splitter->setStretchFactor(1, 1);

    layout->addWidget(splitter);
    setCentralWidget(central);

    // actions
    QAction *actOpenFolder = new QAction("Open Folder", this);
    QAction *actOpenZip = new QAction("Open ZIP", this);
    menuBar()->addAction(actOpenFolder);
    menuBar()->addAction(actOpenZip);

    connect(actOpenFolder, &QAction::triggered, this, &MainWindow::openFolder);
    connect(actOpenZip, &QAction::triggered, this, &MainWindow::openZip);
//    connect(m_listWidget, &QListWidget::itemActivated, this, &MainWindow::onListItemActivated);
    connect(m_listWidget, &QListWidget::itemClicked, this, &MainWindow::onListItemActivated);

    connect(m_thumbGrid, &ThumbnailGrid::thumbnailClicked, this, &MainWindow::showImageFromZip);

    connect(m_thumbGrid, &ThumbnailGrid::requestSaveOriginal, this, [this](const QString &entryName) {
        if (!m_zipHandler) return;

        QImage fullImg = m_zipHandler->loadImage(entryName);
        if (fullImg.isNull()) {
            QMessageBox::warning(this, "Save Image", "Failed to load original image from ZIP.");
            return;
        }

        QString defaultName = QFileInfo(entryName).fileName();
        QString filePath = QFileDialog::getSaveFileName(this, "Save Original Image As", defaultName, "PNG Images (*.png)");
        if (filePath.isEmpty()) return;

        if (!fullImg.save(filePath, "PNG")) {
            QMessageBox::warning(this, "Save Image", "Failed to save PNG file.");
        }
    });


    connect(m_listWidget, &QListWidget::currentRowChanged, this, &MainWindow::onImageSelected);


}

MainWindow::~MainWindow()
{
    clearState();
}

void MainWindow::clearState()
{
    delete m_zipHandler;
    m_zipHandler = nullptr;
    m_currentFolder.clear();
    m_currentZip.clear();
    m_listWidget->clear();
    m_thumbGrid->clear();
    m_imageViewer->clear();
}

void MainWindow::openFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Open Folder", m_currentFolder,
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir.isEmpty())
        return;
    clearState();
    m_currentFolder = dir;
    listFilesFromFolder(dir);
    m_rightStack->setCurrentWidget(m_imageViewer);

    m_mode = Mode::FolderMode;
}

void MainWindow::openZip()
{
    QString zip = QFileDialog::getOpenFileName(this, "Open ZIP File", m_currentFolder, "Zip files (*.zip)");
    if (zip.isEmpty())
        return;
    qDebug("zip: %s", zip.toStdString().c_str());
    clearState();
    m_currentZip = zip;
    QFileInfo fi(zip);
    m_currentFolder = fi.dir().path();
    listFilesFromZip(zip);
    // Show thumbnails by default when a ZIP is opened.
    m_rightStack->setCurrentWidget(m_thumbGrid);

    m_mode = Mode::ZipMode;
}

void MainWindow::listFilesFromFolder(const QString &folderPath)
{
    QDir d(folderPath);
    QFileInfoList entries = d.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    m_imageFiles.clear();
    for (const QFileInfo &fi : entries)
    {
        m_imageFiles.append(fi.absoluteFilePath());
        QString ext = fi.suffix().toLower();
        if (IMAGE_EXTS.contains(ext))
        {
            QListWidgetItem *it = new QListWidgetItem(fi.fileName());
            it->setData(Qt::UserRole, fi.absoluteFilePath());
            m_listWidget->addItem(it);
        }
    }
}

void MainWindow::listFilesFromZip(const QString &zipPath)
{
    m_zipHandler = new ZipHandler(zipPath, -1, this);
    const QStringList &images = m_zipHandler->listImageEntries();
    m_imageFiles = images;
    for (const QString &entry : images)
    {
    //   qDebug("entry:%s", entry.toStdString().c_str());
        QListWidgetItem *it = new QListWidgetItem(entry);
        it->setData(Qt::UserRole, entry); // store entry name
        m_listWidget->addItem(it);
    }
    // populate thumbnail grid
    m_thumbGrid->clear();
    for (const QString &entry : images)
    {
        QImage thumb = m_zipHandler->loadImageThumbnail(entry, QSize(200, 200));
        if (!thumb.isNull())
        {
            m_thumbGrid->addThumbnail(entry, QPixmap::fromImage(thumb),0);
        }
    }
}

void MainWindow::onListItemActivated(QListWidgetItem *item)
{
    if (!item)
        return;
    if (!m_currentZip.isEmpty() && m_zipHandler)
    {
        QString entry = item->data(Qt::UserRole).toString();
        QImage img = m_zipHandler->loadImage(entry);
        if (!img.isNull())
        {
            m_imageViewer->setImage(img);
            m_rightStack->setCurrentWidget(m_imageViewer);
        }
        else
        {
            QMessageBox::warning(this, "Image load failed", "Could not load image from ZIP: " + entry);
        }
    }
    else if (!m_currentFolder.isEmpty())
    {

        QString path = item->data(Qt::UserRole).toString();

        // add grid logic here
        QFileInfo fi(path);
        if (fi.suffix().toLower() == "zip") {
            previewZip(path);
            return;
        }

        QImage img(path);
        if (!img.isNull())
        {
            m_imageViewer->setImage(img);
            m_rightStack->setCurrentWidget(m_imageViewer);
        }
        else
        {
            QMessageBox::warning(this, "Image load failed", "Could not load image: " + path);
        }
    }
}

/// show zip content in preview pane (on the right)
/// when you select a zip file in the list on the left
void MainWindow::previewZip(const QString &zipPath)
{
    if (m_zipHandler)
        delete m_zipHandler;

    //std::unique_ptr<ZipHandler> zipHandler(new ZipHandler(zipPath, 500, this));
    m_zipHandler = new ZipHandler(zipPath, 500, this);;
    const QStringList &images = m_zipHandler->listImageEntries();
    /*
    for (const QString &entry : images)
    {
        QListWidgetItem *it = new QListWidgetItem(entry);
        it->setData(Qt::UserRole, entry); // store entry name
        m_listWidget->addItem(it);
    }*/

    // populate thumbnail grid
    m_thumbGrid->clear();
    for (const QString &entry : images)
    {
        QImage thumb = m_zipHandler->loadImageThumbnail(entry, QSize(200, 200));
        if (!thumb.isNull())
        {
            m_thumbGrid->addThumbnail(entry, QPixmap::fromImage(thumb),0);
        }
    }

    // Show thumbnails by default when a ZIP is opened.
    m_rightStack->setCurrentWidget(m_thumbGrid);

}


void MainWindow::showImageFromZip(const QString &entryName)
{
    if (!m_zipHandler)
        return;
    QImage img = m_zipHandler->loadImage(entryName);
    if (!img.isNull())
    {
        m_imageViewer->setImage(img);
        m_rightStack->setCurrentWidget(m_imageViewer);
        // select corresponding item in list if present
        for (int i = 0; i < m_listWidget->count(); ++i)
        {
            QListWidgetItem *it = m_listWidget->item(i);
            if (it->data(Qt::UserRole).toString() == entryName)
            {
                m_listWidget->setCurrentItem(it);
                break;
            }
        }
    }
    else
    {
        QMessageBox::warning(this, "Image load failed", "Could not load image from ZIP: " + entryName);
    }
}

void MainWindow::onImageSelected(int row)
{
    if (row < 0) return;
    m_listWidget->setCurrentRow(row);
    QListWidgetItem* item = m_listWidget->currentItem();
    onListItemActivated(item);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Left:
    case Qt::Key_Right:
        QApplication::sendEvent(m_listWidget, event);
        return;
    default:
        QMainWindow::keyPressEvent(event);
    }
}
