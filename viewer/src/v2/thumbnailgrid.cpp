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

void ThumbnailGrid::addThumbnail(const QString &entryName, const QPixmap &pix,  qint64 fileSize)
{
    QLabel *lbl = new QLabel;
    lbl->setPixmap(pix.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setFixedSize(220, 220);
    lbl->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    lbl->setProperty("entry", entryName);
    lbl->installEventFilter(this);




    // Create info text
    QString info = QString("%1\n%2 x %3")
                       .arg(QFileInfo(entryName).fileName())
                       .arg(pix.width())
                       .arg(pix.height())
                       ;

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

/*
bool ThumbnailGrid::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease)
    {
        QLabel *lbl = qobject_cast<QLabel *>(watched);
        if (lbl)
        {
            QString entry = lbl->property("entry").toString();
            if (!entry.isEmpty())
            {
                emit thumbnailClicked(entry);
                return true;
            }
        }
    }
    return QScrollArea::eventFilter(watched, event);
}
*/

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


