#include "mainwindow.h"
#include <QFileDialog>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QApplication>
#include <QKeyEvent>
#include <QComboBox>
#include <QToolBar>


static const QStringList IMAGE_EXTS = {"png", "jpg", "jpeg", "bmp", "gif", "webp", "zip", "svg", "7z", "rar"};

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

    // Inside MainWindow constructor:
    QToolBar *toolbar = addToolBar("View");
    QComboBox *bgCombo = new QComboBox(this);
    bgCombo->addItems({ "White", "Dark Gray", "Dark Cyan", "Light Gray", "Black" });
    toolbar->addWidget(bgCombo);

    // Connect selection to slot
    connect(bgCombo, &QComboBox::currentTextChanged, this, [this](const QString &colorName) {
        QColor color;
        if (colorName == "White") color = Qt::white;
        else if (colorName == "Dark Gray") color = Qt::darkGray;
        else if (colorName == "Dark Cyan") color = Qt::darkCyan;
        else if (colorName == "Light Gray") color = Qt::lightGray;
        else if (colorName == "Black") color = Qt::black;
        else color = Qt::white;
        m_thumbGrid->setBackgroundColor(color);
    });

    m_thumbGrid->setBackgroundColor(Qt::white);

    m_zipHandler = new ArchWrap(this);

}

MainWindow::~MainWindow()
{
    clearState();
    delete m_zipHandler;
}

void MainWindow::clearState()
{
    m_zipHandler->clear();
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
    for (const QFileInfo &fi : entries)
    {
        QString ext = fi.suffix().toLower();
        if (IMAGE_EXTS.contains(ext))
        {
            QListWidgetItem *it = new QListWidgetItem(fi.fileName());
            it->setData(Qt::UserRole, fi.absoluteFilePath());
            m_listWidget->addItem(it);
        }
    }
}

/// when you open a zipfile (in zipfile mode)
void MainWindow::listFilesFromZip(const QString &zipPath)
{
    if (!m_zipHandler->openZip(zipPath, -1)) {
        qDebug("cannot open zip: %s", zipPath.toStdString().c_str());
        return;
    }

    const QList<ImgInfo> &images = m_zipHandler->listImageEntries();
    for (const ImgInfo &entry : images)
    {
        QListWidgetItem *it = new QListWidgetItem(entry.filename);
        it->setData(Qt::UserRole, entry.filename); // store entry name
        m_listWidget->addItem(it);
    }
    // populate thumbnail grid
    m_thumbGrid->clear();
    for (const ImgInfo &entry : images)
    {
        QImage img = m_zipHandler->loadImage(entry.filename);
        if (!img.isNull())
        {
            m_thumbGrid->addThumbnail(entry.filename, img, entry.fileSize);
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
        QStringList ARCH_EXT = {"zip", "7z", "rar"};
        QString path = item->data(Qt::UserRole).toString();
        // add grid logic here
        QFileInfo fi(path);
        if (ARCH_EXT.contains(fi.suffix().toLower())) {
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
    if (!m_zipHandler->openZip(zipPath, 500)) {
        qDebug("cannot open zip: %s", zipPath.toStdString().c_str());
        return;
    }

    const QList<ImgInfo> &images = m_zipHandler->listImageEntries();

    // populate thumbnail grid
    m_thumbGrid->clear();
    for (const ImgInfo &entry : images)
    {
        QImage img = m_zipHandler->loadImage(entry.filename);
        if (!img.isNull())
        {
            m_thumbGrid->addThumbnail(entry.filename, img, entry.fileSize);
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
