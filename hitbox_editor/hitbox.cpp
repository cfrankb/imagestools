#include "hitbox.h"

#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <cmath>

const QStringList sizes = {"16", "24", "32", "48", "64", "128", "256"};

QColor typeColor(HitboxType t) {
    switch(t){
    case MainBox: return QColor(0,255,0,80);
    case AttackBox: return QColor(255,0,0,80);
    case Special1Box: return QColor(0,0,255,80);
    case Special2Box: return QColor(128,0,128,80);
    }
    return QColor(200,200,200,80);
}

QString typeName(HitboxType t){
    switch(t){
    case MainBox: return "Main";
    case AttackBox: return "Attack";
    case Special1Box: return "Special1";
    case Special2Box: return "Special2";
    }
    return "Unknown";
}

void GridScene::drawBackground(QPainter *painter, const QRectF &rect)
{
    // Fill background
    painter->fillRect(rect, backgroundColor);

    const int minorGrid = 8;

    QPen minorPen(Qt::lightGray);
    minorPen.setWidth(0);
    QPen majorPen(QColor(90, 90, 90));

    // Draw minor grid (every 8 px)
    qreal left = std::floor(rect.left());
    qreal top = std::floor(rect.top());
    qreal right = std::ceil(rect.right());
    qreal bottom = std::ceil(rect.bottom());

    // Snap to multiples of minor grid
    int startX = static_cast<int>(left) - (static_cast<int>(left) % minorGrid);
    int startY = static_cast<int>(top) - (static_cast<int>(top) % minorGrid);

    painter->setPen(minorPen);
    for (int x = startX; x < right; x += minorGrid)
        painter->drawLine(x, top, x, bottom);
    for (int y = startY; y < bottom; y += minorGrid)
        painter->drawLine(left, y, right, y);

    // Draw major grid
    startX = static_cast<int>(left) - (static_cast<int>(left) % majorGridH);
    startY = static_cast<int>(top) - (static_cast<int>(top) % majorGridV);

    painter->setPen(majorPen);
    for (int x = startX; x < right; x += majorGridH)
        painter->drawLine(x, top, x, bottom);
    for (int y = startY; y < bottom; y += majorGridV)
        painter->drawLine(left, y, right, y);
}

HitboxItem::HitboxItem(const QRectF &r, HitboxType t):QGraphicsRectItem(r),boxType(t){
    setFlags(ItemIsSelectable|ItemIsMovable|ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
    updateColor();
}

void HitboxItem::updateColor(){
    QColor c=typeColor(boxType);
    setBrush(c);
    setPen(QPen(c.darker(),2));
}

QVariant HitboxItem::itemChange(GraphicsItemChange change,const QVariant &val) {
    if(change==ItemPositionChange && scene()){
        QPointF p=val.toPointF();
        p.setX(snap(p.x())); p.setY(snap(p.y()));
        return p;
    }
    return QGraphicsRectItem::itemChange(change,val);
}

void HitboxItem::mousePressEvent(QGraphicsSceneMouseEvent *e) {
    dragStart = pos();
    dragging=true;
    QGraphicsRectItem::mousePressEvent(e);
}

void HitboxItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *e) {
    if(dragging){ dragging=false; emitMoved(); }
    QGraphicsRectItem::mouseReleaseEvent(e);
}

void HitboxItem::paint(QPainter *p,const QStyleOptionGraphicsItem *opt,QWidget *w) {
    QGraphicsRectItem::paint(p,opt,w);
    if(isSelected()){
        p->save();
        p->setPen(QPen(Qt::black,1,Qt::DashLine));
        p->setBrush(Qt::NoBrush);
        p->drawRect(rect());
        p->restore();
    }
}
