#include <QPainter>
#include "tileitem.h"

TileItem::TileItem(const QRectF &rect, const QPixmap &pix)
    : QGraphicsRectItem(rect), m_pix(pix), m_granular(0), m_type(0), m_next(-1), m_speed(1.0)
{
    setFlags(ItemIsSelectable | ItemIsFocusable);
    setAcceptHoverEvents(true);
}

void TileItem::setTileType(int t)
{
    m_type = t;
    update();
}
int TileItem::tileType() const { return m_type; }

void TileItem::setNext(int n) { m_next = n; }
int TileItem::next() const { return m_next; }

void TileItem::setSpeed(double s) { m_speed = s; }
int TileItem::speed() const { return m_speed; }

void TileItem::setTag(const QString &tag) { m_tag = tag; }
QString TileItem::tag() const { return m_tag; }

void TileItem::setWeight(const int w) { m_weight = w; }
int TileItem::weight() const { return m_weight; }

uint8_t TileItem::granular() const { return m_granular; }
void TileItem::setGranular(const uint8_t gr) { m_granular = gr; }

void TileItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    QRectF r = rect();
    if (!m_pix.isNull())
        painter->drawPixmap(r.toRect(), m_pix);
    else
        painter->fillRect(r, Qt::lightGray);

    QColor overlay;
    bool useOverlay = true;
    switch (m_type)
    {
    case TileType::Background:
        useOverlay = false;
        break;
    case TileType::Foreground:
        overlay = QColor(0, 0, 255, 90);
        break;
    case TileType::Solid:
        overlay = QColor(255, 165, 0, 90);
        break;
    case TileType::Deadly:
        overlay = QColor(255, 0, 0, 110);
        break;
    case TileType::Water:
        overlay = QColor(101, 101, 101, 110);
        break;
    default:
        useOverlay = false;
        break;
    }
    if (useOverlay)
        painter->fillRect(r, overlay);

    if (m_granular != 0)
    {
        painter->save();
        const QBrush purpleOverlay(QColor(128, 0, 128, 100));
        painter->setPen(Qt::NoPen);
        painter->setBrush(purpleOverlay);
        const qreal halfW = r.width() / 2.0;
        const qreal halfH = r.height() / 2.0;
        if (m_granular & GranualarUL)
            painter->fillRect(QRectF(r.x(), r.y(), halfW, halfH), purpleOverlay);
        if (m_granular & GranualarUR)
            painter->fillRect(QRectF(r.x() + halfW, r.y(), halfW, halfH), purpleOverlay);
        if (m_granular & GranualarDL)
            painter->fillRect(QRectF(r.x(), r.y() + halfH, halfW, halfH), purpleOverlay);
        if (m_granular & GranualarDR)
            painter->fillRect(QRectF(r.x() + halfW, r.y() + halfH, halfW, halfH), purpleOverlay);
        painter->restore();
    }

    if (isSelected())
    {
        QPen p(Qt::yellow);
        p.setWidth(2);
        painter->setPen(p);
        painter->drawRect(r.adjusted(1, 1, -1, -1));
    }
    else
    {
        QPen p(Qt::black);
        p.setWidth(0);
        painter->setPen(p);
        painter->drawRect(r);
    }

    if (!m_tag.isEmpty()) {
        painter->setPen(Qt::white);
        painter->setFont(QFont("Arial", 3));
        painter->drawText(r.adjusted(2,2,-2,-2), m_tag);
    }

    if (m_weight != 1) {
        painter->setPen(Qt::yellow);
        painter->setFont(QFont("Arial", 2));
        painter->drawText(r.adjusted(0,0,-2,-2),
                          Qt::AlignRight | Qt::AlignBottom,
                          QString::number(m_weight));
    }
}
