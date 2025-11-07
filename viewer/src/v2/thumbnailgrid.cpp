#include "thumbnailgrid.h"
#include <QMouseEvent>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>

ThumbnailGrid::ThumbnailGrid(QWidget *parent)
    : QScrollArea(parent)
{
    m_container = new QWidget;
    m_layout = new QGridLayout;
    m_container->setLayout(m_layout);
    setWidget(m_container);
    setWidgetResizable(true);
}

void ThumbnailGrid::addThumbnail(const QString &entryName, const QImage &img,  qint64 fileSize)
{
    auto formatSize = [](qint64 bytes){
        if (bytes < 1024)
            return QString("%1 bytes").arg(bytes);
        else if (bytes < 1024 * 1024)
            return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
        else if (bytes < 1024LL * 1024LL * 1024LL)
            return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
        else
            return QString("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
    };

    QLabel *lbl = new QLabel;
    lbl->setPixmap(QPixmap::fromImage(img.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setFixedSize(220, 220);
    lbl->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    lbl->setProperty("entry", entryName);
    lbl->installEventFilter(this);

    // Create info text
    QString info = QString("%1\n%2 x %3\n%4")
                       .arg(QFileInfo(entryName).fileName())
                       .arg(img.width())
                       .arg(img.height())
                       .arg(formatSize(fileSize));

    QLabel *infoLabel = new QLabel(info);
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setWordWrap(true);

    // Stack the image and text vertically
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setContentsMargins(2, 2, 2, 2);
    vbox->addWidget(lbl);
    vbox->addWidget(infoLabel);

    QWidget *container = new QWidget;
    container->setLayout(vbox);


    int count = m_labelMap.size();
    int row = count / m_columns;
    int col = count % m_columns;
    //m_layout->addWidget(lbl, row, col);
    m_layout->addWidget(container, row, col);
    m_labelMap.insert(entryName, lbl);
}

void ThumbnailGrid::clear()
{
    qDeleteAll(m_labelMap);
    m_labelMap.clear();
    QLayoutItem *child;
    while ((child = m_layout->takeAt(0)) != nullptr)
    {
        delete child->widget();
        delete child;
    }
}

bool ThumbnailGrid::eventFilter(QObject *watched, QEvent *event)
{
    QLabel *lbl = qobject_cast<QLabel*>(watched);
    if (!lbl)
        return QScrollArea::eventFilter(watched, event);

    QString entry = lbl->property("entry").toString();
    if (entry.isEmpty())
        return QScrollArea::eventFilter(watched, event);

    if (event->type() == QEvent::MouseButtonRelease) {
        auto *me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
            emit thumbnailClicked(entry);
            return true;
        } else if (me->button() == Qt::RightButton) {
            // context menu
            QMenu menu;
            //QAction *saveAct = menu.addAction("Save thumbnail...");
            //QAction *chosen = menu.exec(me->globalPosition().toPoint());
            //if (chosen == saveAct)
              //  saveThumbnailAsPng(entry);
            QAction *saveAct = menu.addAction("Save as PNG...");
            QAction *chosen = menu.exec(me->globalPosition().toPoint());
            if (chosen == saveAct)
                emit requestSaveOriginal(entry);

            return true;
        }
    }

    return QScrollArea::eventFilter(watched, event);
}

void ThumbnailGrid::saveThumbnailAsPng(const QString &entryName)
{
    QLabel *lbl = m_labelMap.value(entryName, nullptr);
    if (!lbl)
        return;

    QPixmap pix = lbl->pixmap();
    if (pix.isNull()) {
        QMessageBox::warning(this, "Save Thumbnail", "No thumbnail image found.");
        return;
    }

    QString defaultName = QFileInfo(entryName).baseName() + "_thumb.png";
    QString filePath = QFileDialog::getSaveFileName(this, "Save Thumbnail As", defaultName, "PNG Images (*.png)");
    if (filePath.isEmpty())
        return;

    if (!pix.save(filePath, "PNG")) {
        QMessageBox::warning(this, "Save Thumbnail", "Failed to save PNG file.");
    }
}

void ThumbnailGrid::setBackgroundColor(const QColor &color)
{
    QPalette pal = this->palette();
    pal.setColor(QPalette::Window, color);
    this->setAutoFillBackground(true);
    this->setPalette(pal);
    this->update();
}

