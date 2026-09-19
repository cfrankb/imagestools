#include "tilescene.h"
#include "qgraphicssceneevent.h"
#include "tileitem.h"
#include <QApplication>

TileScene::TileScene(QObject *parent) : QGraphicsScene(parent), m_rubber(nullptr) {}

void TileScene::setSelectMode(bool enable)
{
    m_selectMode = enable;
}

void TileScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (m_selectMode)
        {
            QPointF pos = event->scenePos();
            QGraphicsItem *it = itemAt(pos, QTransform());
            if (it && dynamic_cast<TileItem *>(it))
            {
                // Copy the clicked tile's ID to the Next Tile field without
                // modifying the current selection.
                emit nextTileIDSelected(it->data(TileItem::TypeRole).toInt());
                emit exitSelectModeRequested();
                event->accept();
                return;
            }
            else
            {
                emit exitSelectModeRequested();
                event->accept();
                return;
            }
        }
        else
        {
            m_origin = event->scenePos();
            if (!m_rubber)
            {
                m_rubber = addRect(QRectF(m_origin, QSizeF()), QPen(Qt::DashLine));
                m_rubber->setZValue(10000);
            }
            event->accept();
            emit selectionChanged();
            return;
        }
    }
    if (event->button() == Qt::RightButton)
    {
        QGraphicsItem *it = itemAt(event->scenePos(), QTransform());
        if (!it)
            clearSelection();
        event->accept();
        emit selectionChanged();
        return;
    }
    QGraphicsScene::mousePressEvent(event);
}

void TileScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_rubber)
    {
        QRectF r(m_origin, event->scenePos());
        r = r.normalized();
        m_rubber->setRect(r);
        event->accept();
        emit selectionChanged();
        return;
    }
    QGraphicsScene::mouseMoveEvent(event);
}

void TileScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_rubber)
    {
        QRectF r = m_rubber->rect();
        removeItem(m_rubber);
        delete m_rubber;
        m_rubber = nullptr;
        QList<QGraphicsItem *> itemsInRect = items(r, Qt::IntersectsItemShape);
        bool add = QApplication::keyboardModifiers() & Qt::ControlModifier;
        if (!add)
            clearSelection();
        for (QGraphicsItem *it : itemsInRect)
            it->setSelected(true);
        event->accept();
        emit selectionChanged();
        return;
    }
    if (event->button() == Qt::RightButton)
    {
        QPointF pos = event->scenePos();
        QGraphicsItem *clicked = itemAt(pos, QTransform());
        if (clicked && !clicked->isSelected())
        {
            clearSelection();
            clicked->setSelected(true);
        }
        emit requestContextMenu(event->screenPos());
        event->accept();
        emit selectionChanged();
        return;
    }
    QGraphicsScene::mouseReleaseEvent(event);
}
