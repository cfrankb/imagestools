#include "imageviewer.h"
#include <QVBoxLayout>
#include <QEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QFileInfo>
#include <QMessageBox>
#include <QFileDialog>

ImageViewer::ImageViewer(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *l = new QVBoxLayout(this);
    m_imageLabel = new QLabel;
   // m_imageLabel->setBackgroundRole(QPalette::Base);
    m_imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_imageLabel->setScaledContents(true);
    m_imageLabel->installEventFilter(this);
    m_scrollArea = new QScrollArea;
    m_scrollArea->setWidget(m_imageLabel);
    m_scrollArea->setWidgetResizable(true);
    l->addWidget(m_scrollArea);
    //m_imageLabel->setAttribute(Qt::WA_TranslucentBackground);
    //m_imageLabel->setStyleSheet("background: transparent;");
}

void ImageViewer::setImage(const QImage &img, const QString &path, bool fromZip)
{
    m_imageLabel->setPixmap(QPixmap::fromImage(img));
    m_imageLabel->adjustSize();
    m_path = path;
    m_fromZip = fromZip;
}

void ImageViewer::clear()
{
    m_imageLabel->clear();
}

bool ImageViewer::eventFilter(QObject *watched, QEvent *event)
{
    if (!m_imageLabel)
        return QWidget::eventFilter(watched, event);

    QPixmap pix = m_imageLabel->pixmap();
    if (pix.isNull())
        return QWidget::eventFilter(watched, event);

    if (event->type() == QEvent::MouseButtonRelease) {
        auto *me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
         //   emit thumbnailClicked(entry);
            return true;
        } else if (me->button() == Qt::RightButton) {
            // context menu
            QMenu menu;
            QAction *saveAct = menu.addAction("Save as PNG...");
            QAction *chosen = menu.exec(me->globalPosition().toPoint());
            if (chosen == saveAct)
                emit requestSaveOriginal(m_path, m_fromZip);

            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}


void ImageViewer::setBackgroundColor(const QColor &color)
{
    QPalette pal = this->palette();
    pal.setColor(QPalette::Window, color);
    this->setAutoFillBackground(true);
    this->setPalette(pal);
    this->update();
}

