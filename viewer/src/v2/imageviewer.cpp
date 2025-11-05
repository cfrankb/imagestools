#include "imageviewer.h"
#include <QVBoxLayout>

ImageViewer::ImageViewer(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *l = new QVBoxLayout(this);
    m_imageLabel = new QLabel;
    m_imageLabel->setBackgroundRole(QPalette::Base);
    m_imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_imageLabel->setScaledContents(true);
    m_scrollArea = new QScrollArea;
    m_scrollArea->setWidget(m_imageLabel);
    m_scrollArea->setWidgetResizable(true);
    l->addWidget(m_scrollArea);
}

void ImageViewer::setImage(const QImage &img)
{
    m_imageLabel->setPixmap(QPixmap::fromImage(img));
    m_imageLabel->adjustSize();
}

void ImageViewer::clear()
{
    m_imageLabel->clear();
}
