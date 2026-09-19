#ifndef TILEITEM_H
#define TILEITEM_H

#include <QGraphicsRectItem>
#include <QPixmap>
#include <QString>
#include <QRectF>

enum TileType {
    Background,
    Foreground,
    Solid,
    Deadly,
    Water
};

const uint8_t GranualarUL=1; // TOP LEFT
const uint8_t GranualarUR=2; // TOP RIGHT
const uint8_t GranualarDL=4; // BOTTOM LEFT
const uint8_t GranualarDR=8; // BOTTOM RIGHT

class TileItem : public QGraphicsRectItem
{
public:
    enum
    {
        TypeRole = UserType + 1
    };

    TileItem(const QRectF &rect, const QPixmap &pix);

    void setTileType(int t);
    int tileType() const;

    void setNext(int n);
    int next() const;

    void setSpeed(double s);
    int speed() const;

    void setTag(const QString &tag);
    QString tag() const;

    void setWeight(const int w);
    int weight() const;

    uint8_t granular() const;
    void setGranular(const uint8_t gr);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

private:
    QPixmap m_pix;
    uint8_t m_granular;
    int m_type;
    int m_next;
    int m_speed;
    QString m_tag;
    int m_weight = 1;
};

#endif // TILEITEM_H
