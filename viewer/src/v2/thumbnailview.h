#pragma once
#include <QGraphicsView>
#include <QGraphicsScene>
#include "thumbitem.h"


class ThumbnailView : public QGraphicsView {
    Q_OBJECT
public:
    ThumbnailView(QWidget *parent = nullptr)
        : QGraphicsView(parent), m_columns(6) {
        m_scene = new QGraphicsScene(this);
        setScene(m_scene);
        setAlignment(Qt::AlignTop | Qt::AlignLeft);
    }

    void clear();
    void setBackgroundColor(const QColor &color);
    ThumbItem * addThumbnail(const QString &entryName, const QPixmap &pix, qint64 size, bool reposition);
    void repositionItems();

protected:
    void resizeEvent(QResizeEvent *event);


private:
    QGraphicsScene *m_scene;
    QVector<ThumbItem*> m_items;
    int m_columns;
};
