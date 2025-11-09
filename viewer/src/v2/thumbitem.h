#pragma once

#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>

#include <QMenu>


class ThumbItem : public QObject, public QGraphicsItem {
    Q_OBJECT
public:
    ThumbItem(const QPixmap &pix,
              const QString &entryName,
              qint64 fileSize,
              const QSize &dimensions,
              QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *) override;

    QString entry() const { return m_entry; }
    qint64 size() const { return m_fileSize; }
    QSize dimensions() const { return m_dimensions; }


    QPainterPath shape() const override  {
        QPainterPath path;
        path.addRect(boundingRect());
        return path;
    }


protected:
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;


signals:
    void clicked(const QString &entryName);
    void saveRequested(const QString &entryName);
    void emitClicked(QGraphicsSceneMouseEvent *event);

private:
    QPixmap m_pix;
    QString m_entry;
    qint64 m_fileSize;
    QSize m_dimensions;
};
