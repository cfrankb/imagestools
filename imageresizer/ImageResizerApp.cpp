// ImageResizerApp.cpp
#include "ImageResizerApp.h"
#include <QtCore/QUrl>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtCore/QTemporaryDir>
#include <QtGui/QPixmap>
#include <QtWidgets/QApplication>
#include <QMimeData>
#include <QSettings>
#include "ImageInfo.h"
#include "ImageItem.h"
#include "ResizeThread.h"

ImageResizerApp::ImageResizerApp(QWidget *parent)
    : QMainWindow(parent)
    , m_resizeThread(nullptr)
{
    setupUI();
    setAcceptDrops(true);
    loadSettings();
}

ImageResizerApp::~ImageResizerApp()
{
    saveSettings();
    if (m_resizeThread && m_resizeThread->isRunning()) {
        m_resizeThread->quit();
        m_resizeThread->wait();
    }
}

void ImageResizerApp::loadSettings()
{
    QSettings settings("ImageResizerApp", "BatchImageResizer");

    // Load width and height settings
    int savedWidth = settings.value("targetWidth", 16).toInt();
    int savedHeight = settings.value("targetHeight", 16).toInt();
    bool savedMaintainAspect = settings.value("maintainAspect", true).toBool();

    // Apply saved values
    m_widthSpinBox->setValue(savedWidth);
    m_heightSpinBox->setValue(savedHeight);
    m_aspectCheckBox->setChecked(savedMaintainAspect);

    // Also restore window geometry if saved
    restoreGeometry(settings.value("geometry").toByteArray());
}

void ImageResizerApp::saveSettings()
{
    QSettings settings("ImageResizerApp", "BatchImageResizer");

    // Save width and height settings
    settings.setValue("targetWidth", m_widthSpinBox->value());
    settings.setValue("targetHeight", m_heightSpinBox->value());
    settings.setValue("maintainAspect", m_aspectCheckBox->isChecked());

    // Also save window geometry
    settings.setValue("geometry", saveGeometry());
}


void ImageResizerApp::setupUI()
{
    setWindowTitle("Qt6 C++ Batch Image Resizer");
    setGeometry(100, 100, 1000, 700);
    
    m_centralWidget = new QWidget;
    setCentralWidget(m_centralWidget);
    
    m_mainLayout = new QVBoxLayout;
    
    // Controls section
    QGroupBox *controlsGroup = new QGroupBox("Resize Settings");
    QGridLayout *controlsLayout = new QGridLayout;
    
    // Size controls
    controlsLayout->addWidget(new QLabel("Target Width:"), 0, 0);
    m_widthSpinBox = new QSpinBox;
    m_widthSpinBox->setRange(1, 10000);
    m_widthSpinBox->setValue(16);
    connect(m_widthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImageResizerApp::updatePreviews);
    controlsLayout->addWidget(m_widthSpinBox, 0, 1);
    
    controlsLayout->addWidget(new QLabel("Target Height:"), 0, 2);
    m_heightSpinBox = new QSpinBox;
    m_heightSpinBox->setRange(1, 10000);
    m_heightSpinBox->setValue(16);
    connect(m_heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImageResizerApp::updatePreviews);
    controlsLayout->addWidget(m_heightSpinBox, 0, 3);
    
    // Aspect ratio checkbox
    m_aspectCheckBox = new QCheckBox("Maintain aspect ratio");
    m_aspectCheckBox->setChecked(true);
    connect(m_aspectCheckBox, &QCheckBox::toggled, this, &ImageResizerApp::updatePreviews);
    controlsLayout->addWidget(m_aspectCheckBox, 1, 0, 1, 2);
    
    controlsGroup->setLayout(controlsLayout);
    m_mainLayout->addWidget(controlsGroup);
    
    // Selection controls
    QHBoxLayout *selectionLayout = new QHBoxLayout;
    
    m_selectAllBtn = new QPushButton("Select All");
    connect(m_selectAllBtn, &QPushButton::clicked, this, &ImageResizerApp::selectAll);
    selectionLayout->addWidget(m_selectAllBtn);
    
    m_selectNoneBtn = new QPushButton("Select None");
    connect(m_selectNoneBtn, &QPushButton::clicked, this, &ImageResizerApp::selectNone);
    selectionLayout->addWidget(m_selectNoneBtn);
    
    m_clearAllBtn = new QPushButton("Clear All");
    connect(m_clearAllBtn, &QPushButton::clicked, this, &ImageResizerApp::clearAll);
    selectionLayout->addWidget(m_clearAllBtn);


    selectionLayout->addStretch();
    
    // Add files button
    m_addFilesBtn = new QPushButton("Add Files");
    connect(m_addFilesBtn, &QPushButton::clicked, this, &ImageResizerApp::addFiles);
    selectionLayout->addWidget(m_addFilesBtn);
    
    // Export button
    m_exportBtn = new QPushButton("Export as ZIP");
    connect(m_exportBtn, &QPushButton::clicked, this, &ImageResizerApp::exportImages);
    m_exportBtn->setEnabled(false);
    selectionLayout->addWidget(m_exportBtn);
    
    m_mainLayout->addLayout(selectionLayout);
    
    // Drag and drop area
    m_dropLabel = new QLabel("Drag and drop images here or use 'Add Files' button");
    m_dropLabel->setAlignment(Qt::AlignCenter);
    m_dropLabel->setStyleSheet(
        "QLabel {"
        "border: 2px dashed #aaa;"
        "border-radius: 10px;"
        "padding: 20px;"
        "font-size: 14px;"
        "color: #666;"
        "min-height: 100px;"
        "}"
    );
    m_mainLayout->addWidget(m_dropLabel);
    
    // Scroll area for images
    m_scrollArea = new QScrollArea;
    m_scrollWidget = new QWidget;
    m_scrollLayout = new QVBoxLayout;
    m_scrollWidget->setLayout(m_scrollLayout);
    m_scrollArea->setWidget(m_scrollWidget);
    m_scrollArea->setWidgetResizable(true);
    m_mainLayout->addWidget(m_scrollArea);
    
    // Progress bar
    m_progressBar = new QProgressBar;
    m_progressBar->setVisible(false);
    m_mainLayout->addWidget(m_progressBar);
    
    m_centralWidget->setLayout(m_mainLayout);
}

void ImageResizerApp::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void ImageResizerApp::dropEvent(QDropEvent *event)
{
    QStringList files;
    const QList<QUrl> urls = event->mimeData()->urls();   
    for (const QUrl &url : urls) {
        QString filePath = url.toLocalFile();
        if (isImageFile(filePath)) {
            files.append(filePath);
        }
    }
    
    if (!files.isEmpty()) {
        addImageFiles(files);
    }
}

bool ImageResizerApp::isImageFile(const QString &filePath)
{
    QStringList validExtensions = {"jpg", "jpeg", "png", "bmp", "gif", "tiff", "webp"};
    QFileInfo fileInfo(filePath);
    return validExtensions.contains(fileInfo.suffix().toLower());
}

void ImageResizerApp::addFiles()
{
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        "Select Images",
        "",
        "Image files (*.jpg *.jpeg *.png *.bmp *.gif *.tiff *.webp)"
    );
    
    if (!files.isEmpty()) {
        addImageFiles(files);
    }
}

void ImageResizerApp::addImageFiles(const QStringList &filePaths)
{
    for (const QString &filePath : filePaths) {
        // Check if file already added
        bool alreadyExists = false;
        for (ImageItem *item : m_imageItems) {
            if (item->getFilePath() == filePath) {
                alreadyExists = true;
                break;
            }
        }
        
        if (alreadyExists) continue;
        
        ImageItem *imageItem = new ImageItem(filePath);
        m_imageItems.append(imageItem);
        m_scrollLayout->addWidget(imageItem);
    }
    
    // Hide drop label if we have images
    if (!m_imageItems.isEmpty()) {
        m_dropLabel->setVisible(false);
        m_exportBtn->setEnabled(true);
    }
    
    // Update previews
    updatePreviews();
}

void ImageResizerApp::updatePreviews()
{
    int targetWidth = m_widthSpinBox->value();
    int targetHeight = m_heightSpinBox->value();
    bool maintainAspect = m_aspectCheckBox->isChecked();
    
    for (ImageItem *item : m_imageItems) {
        item->updatePreview(targetWidth, targetHeight, maintainAspect);
    }
}

void ImageResizerApp::selectAll()
{
    for (ImageItem *item : m_imageItems) {
        item->setSelected(true);
    }
}

void ImageResizerApp::selectNone()
{
    for (ImageItem *item : m_imageItems) {
        item->setSelected(false);
    }
}

void ImageResizerApp::clearAll()
{
    // Remove all image items from layout
    for (ImageItem *item : m_imageItems) {
        m_scrollLayout->removeWidget(item);
        item->deleteLater();
    }

    // Clear the list
    m_imageItems.clear();

    // Show drop label again and disable export
    m_dropLabel->setVisible(true);
    m_exportBtn->setEnabled(false);
}

void ImageResizerApp::exportImages()
{
    int selectedCount = 0;
    for (ImageItem *item : m_imageItems) {
        if (item->isSelected()) {
            selectedCount++;
        }
    }
    
    if (selectedCount == 0) {
        QMessageBox::warning(this, "Warning", "No images selected for export!");
        return;
    }
    
    // Show progress bar
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    m_exportBtn->setEnabled(false);
    
    // Start resize thread  
    QString savePath = QFileDialog::getSaveFileName(
        this,
        "Save Resized Images",
        "resized_images.zip",
        "ZIP files (*.zip)"
        );

    qDebug() << "savePath:" << savePath ;
    if (!savePath.isEmpty()) {
        QList<ImageInfo> list;
        for (const auto &image: m_imageItems) {
            if (image->isSelected())
                list.append({image->getOriginalPixmap(), image->getFilePath()});
        }
        m_resizeThread = new ResizeThread(
            list,
            m_widthSpinBox->value(),
            m_heightSpinBox->value(),
            m_aspectCheckBox->isChecked(),
            savePath,
            this
            );
        connect(m_resizeThread, &ResizeThread::progress, this, &ImageResizerApp::updateProgress);
        connect(m_resizeThread, &ResizeThread::finished, this, &ImageResizerApp::exportFinished);
        connect(m_resizeThread, &ResizeThread::error, this, &ImageResizerApp::exportError);
        m_resizeThread->start();
    }
}

void ImageResizerApp::updateProgress(int value)
{
    m_progressBar->setValue(value);
}

void ImageResizerApp::exportFinished(const QString &zipPath)
{
    qDebug() << "zipPath:" << zipPath ;

    m_progressBar->setVisible(false);
    m_exportBtn->setEnabled(true);
    
    // Clean up thread
    m_resizeThread->deleteLater();
    m_resizeThread = nullptr;
}

void ImageResizerApp::exportError(const QString &errorMessage)
{
    m_progressBar->setVisible(false);
    m_exportBtn->setEnabled(true);
    QMessageBox::critical(this, "Error", QString("Export failed: %1").arg(errorMessage));
    
    // Clean up thread
    if (m_resizeThread) {
        m_resizeThread->deleteLater();
        m_resizeThread = nullptr;
    }
}

