#include "thumbnailview.h"
#include <QResizeEvent>


ThumbItem * ThumbnailView::addThumbnail(const QString &entryName, const QPixmap &pix, qint64 size)
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

    ThumbItem *item = new ThumbItem(
        pix.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation),
        entryName,
        size,
        pix.size()
        );

    item->setToolTip(
        QString("%1\n%2 x %3 px\n%4")
            .arg(entryName)
            .arg(pix.width())
            .arg(pix.height())
            .arg(formatSize(size))
        );

    m_items.append(item);
    scene()->addItem(item);

    repositionItems();
    return item;
}

void ThumbnailView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);

    int thumbW = 200;
    int spacing = 10;

    int available = event->size().width();
    int newColumns = qMax(1, available / (thumbW + spacing));

    if (newColumns != m_columns) {
        m_columns = newColumns;
        repositionItems();
    }
}

void ThumbnailView::repositionItems()
{
    const int thumbW = 200;
    const int thumbH = 240;
    const int spacing = 10;

    for (int i = 0; i < m_items.size(); ++i) {
        int row = i / m_columns;
        int col = i % m_columns;

        qreal x = col * (thumbW + spacing);
        qreal y = row * (thumbH + spacing);

        m_items[i]->setPos(x, y);
    }

    // Update scene size so scrollbars adjust correctly
    int rows = (m_items.size() + m_columns - 1) / m_columns;

    qreal sceneW = m_columns * (thumbW + spacing);
    qreal sceneH = rows * (thumbH + spacing);

    scene()->setSceneRect(0, 0, sceneW, sceneH);
}

void ThumbnailView::setBackgroundColor(const QColor &color)
{
    setBackgroundBrush(color);
}

void ThumbnailView::clear()
{
    // Remove all items from the scene
    for (auto *item : m_items) {
        m_scene->removeItem(item);
        delete item;
    }

    // Reset the list
    m_items.clear();

    // Reset the scene rect
    m_scene->setSceneRect(0, 0, 0, 0);

    // Optional: ensure view updates
    viewport()->update();
}
