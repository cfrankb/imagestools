// ImageItem.cpp
#include <QtCore/QFileInfo>
#include <QtGui/QPixmap>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QBoxLayout>
#include <QGroupBox>
#include "ImageItem.h"

ImageItem::ImageItem(const QString &filePath, QWidget *parent)
    : QWidget(parent)
    , m_filePath(filePath)
{
    setupUI();
    loadImage();
}

void ImageItem::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout;
    
    // Checkbox for selection
    QFileInfo fileInfo(m_filePath);
    m_checkBox = new QCheckBox(fileInfo.fileName());
    m_checkBox->setChecked(true);
    layout->addWidget(m_checkBox);
    
    // Container for images
    QHBoxLayout *imagesLayout = new QHBoxLayout;
    
    // Original image
    QGroupBox *originalGroup = new QGroupBox("Original");
    QVBoxLayout *originalLayout = new QVBoxLayout;
    m_originalLabel = new QLabel;
    m_originalLabel->setFixedSize(200, 150);
    m_originalLabel->setScaledContents(true);
    m_originalLabel->setStyleSheet("border: 1px solid gray");
    originalLayout->addWidget(m_originalLabel);
    m_originalSizeLabel = new QLabel("Size: Loading...");
    originalLayout->addWidget(m_originalSizeLabel);
    originalGroup->setLayout(originalLayout);
    
    // Preview image
    QGroupBox *previewGroup = new QGroupBox("Preview");
    QVBoxLayout *previewLayout = new QVBoxLayout;
    m_previewLabel = new QLabel;
    m_previewLabel->setFixedSize(200, 150);
    m_previewLabel->setScaledContents(true);
    m_previewLabel->setStyleSheet("border: 1px solid gray");
    previewLayout->addWidget(m_previewLabel);
    m_previewSizeLabel = new QLabel("Size: No preview");
    previewLayout->addWidget(m_previewSizeLabel);
    previewGroup->setLayout(previewLayout);
    
    imagesLayout->addWidget(originalGroup);
    imagesLayout->addWidget(previewGroup);
    
    layout->addLayout(imagesLayout);
    setLayout(layout);
}

void ImageItem::loadImage()
{
    m_originalPixmap = QPixmap(m_filePath);
    
    if (!m_originalPixmap.isNull()) {
        m_originalLabel->setPixmap(m_originalPixmap);
        m_originalSizeLabel->setText(QString("Size: %1 x %2")
                                   .arg(m_originalPixmap.width())
                                   .arg(m_originalPixmap.height()));
    } else {
        m_originalLabel->setText("Error loading image");
    }
}

void ImageItem::updatePreview(int targetWidth, int targetHeight, bool maintainAspect)
{
    if (m_originalPixmap.isNull()) return;
    
    int newWidth, newHeight;
    
    if (maintainAspect) {
        // Calculate aspect ratio preserving dimensions
        double aspectRatio = static_cast<double>(m_originalPixmap.width()) / m_originalPixmap.height();
        
        if (static_cast<double>(targetWidth) / targetHeight > aspectRatio) {
            // Height is the limiting factor
            newHeight = targetHeight;
            newWidth = static_cast<int>(targetHeight * aspectRatio);
        } else {
            // Width is the limiting factor
            newWidth = targetWidth;
            newHeight = static_cast<int>(targetWidth / aspectRatio);
        }
    } else {
        newWidth = targetWidth;
        newHeight = targetHeight;
    }
    
    // Create resized pixmap
    m_resizedPixmap = m_originalPixmap.scaled(newWidth, newHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    m_previewLabel->setPixmap(m_resizedPixmap);
    
    // Update size label
    m_previewSizeLabel->setText(QString("Size: %1 x %2").arg(newWidth).arg(newHeight));
}

bool ImageItem::isSelected() const
{
    return m_checkBox->isChecked();
}

void ImageItem::setSelected(bool selected)
{
    m_checkBox->setChecked(selected);
}
