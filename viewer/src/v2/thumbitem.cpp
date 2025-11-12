#include <QPainter>
#include <QGraphicsView>
#include "thumbitem.h"

static QString humanSize(qint64 bytes)
{
    double b = bytes;
    if (b < 1024) return QString::number(bytes) + " bytes";
    b /= 1024;
    if (b < 1024) return QString::number(b, 'f', 1) + " KB";
    b /= 1024;
    return QString::number(b, 'f', 1) + " MB";
}


ThumbItem::ThumbItem(const QPixmap &pix,
                     const QString &entryName,
                     qint64 fileSize,
                     const QSize &dimensions,
                     QGraphicsItem *parent)
    : QObject(), QGraphicsItem(parent),
    m_pix(pix),
    m_entry(entryName),
    m_fileSize(fileSize),
    m_dimensions(dimensions)
{
    setAcceptedMouseButtons(Qt::AllButtons);
    setFlag(ItemIsSelectable, true);
    setFlag(ItemIsFocusable, true);

    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
}

void ThumbItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_entry);
        event->accept();
        return;
    }
}

QRectF ThumbItem::boundingRect() const
{
    return QRectF(0, 0, 200, 240);
}

void ThumbItem::paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // ensure antialiasing / smooth transform for scaled pixmap
    p->setRenderHint(QPainter::SmoothPixmapTransform, true);

    // Build rectangle for thumbnail (top area)
    QRectF thumbRect(0, 0, 200, 200);

    // Use drawPixmap overload that accepts QRectF, QPixmap and source QRect
    // The third arg (sourceRect) is a QRect in pixmap coordinates; here we use the full pixmap
    p->drawPixmap(thumbRect, m_pix, m_pix.rect());

    // Prepare text
    QRectF textRect(0, 200, 200, 40);
    p->setFont(QFont("Sans", 9));
    p->setPen(Qt::blue);


    // selected state overlay
    if (isSelected()) {
        p->setPen(QPen(Qt::yellow, 3));
        p->setBrush(Qt::NoBrush);
        p->drawRect(boundingRect().adjusted(1,1,-1,-1));
    }

    QString elided = p->fontMetrics().elidedText(m_entry, Qt::ElideMiddle, int(textRect.width()));

    // Optional second line with dimensions/size
    QString info = QString("%1 x %2, %3")
                       .arg(m_dimensions.width())
                       .arg(m_dimensions.height())
                       .arg(humanSize(m_fileSize));

    // Draw filename and info on two lines, centered
    p->drawText(textRect, Qt::AlignHCenter | Qt::AlignVCenter, elided + "\n" + info);
}

void ThumbItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    QMenu menu;
    QAction *save = menu.addAction("Save as PNG...");
    QAction *act = menu.exec(event->screenPos());

    if (act == save)
        emit saveRequested(m_entry);

    event->accept(); // prevents propagation
}
