#ifndef TILESCENE_H
#define TILESCENE_H

#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsItem>
#include <QPointF>
#include <QList>
#include <QSizeF>

class TileItem;

class TileScene : public QGraphicsScene
{
    Q_OBJECT
public:
    TileScene(QObject *parent = nullptr);

    void setSelectMode(bool enable);

signals:
    void requestContextMenu(const QPoint &screenPos);
    void selectionChanged();
    void exitSelectModeRequested();
    void nextTileIDSelected(int index);

private:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

    QPointF m_origin;
    QGraphicsRectItem *m_rubber;
    bool m_selectMode = false;
};

#endif // TILESCENE_H
