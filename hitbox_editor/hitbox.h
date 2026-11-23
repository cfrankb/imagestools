#ifndef HITBOX_H
#define HITBOX_H

#include <QColor>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QRect>
#include <QStringList>

static const int GRID = 8;
extern const QStringList sizes;

enum HitboxType { MainBox, AttackBox, Special1Box, Special2Box };
struct HitboxDef { QRect rect; HitboxType type; };

QColor typeColor(HitboxType t);
QString typeName(HitboxType t);

class GridScene : public QGraphicsScene {
public:
    GridScene(QObject* parent=nullptr) : QGraphicsScene(parent) {}

    void setMajorGrid(int sizeH, int sizeV) { majorGridH = sizeH; majorGridV = sizeV; update(); }
    void setBackgroundColor(const QColor &color) { backgroundColor = color; update(); }

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;

private:
    int majorGridH = 64;
    int majorGridV = 64;
    QColor backgroundColor = QColor(255, 255, 255);
};

class HitboxItem : public QObject, public QGraphicsRectItem {
    Q_OBJECT
public:
    HitboxType boxType;
    QPointF dragStart;
    bool dragging=false;

    HitboxItem(const QRectF &r, HitboxType t);

    void updateColor();

    static qreal snap(qreal v){return static_cast<int>(v/GRID) *GRID;}

    QVariant itemChange(GraphicsItemChange change,const QVariant &val) override;

    void mousePressEvent(QGraphicsSceneMouseEvent *e) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *e) override;

signals:
    void moved();

private:
    void emitMoved(){ emit moved(); }

    void paint(QPainter *p,const QStyleOptionGraphicsItem *opt,QWidget *w) override;
};

#endif // HITBOX_H
