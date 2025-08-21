#include "customwidget.h"
#include <QDebug>
#include <QPainter>
#include "shared/FrameSet.h"
#include "shared/Frame.h"
#include "shared/qtgui/qfilewrap.h"

CustomWidget::CustomWidget(QWidget *parent) : QWidget(parent)
{
    setupUi();
}

CustomWidget::~CustomWidget()
{
    // Cleanup handled by Qt's parent-child hierarchy
}

void CustomWidget::setupUi()
{
    m_frameSet = nullptr;
}

void CustomWidget::paintEvent(QPaintEvent *)
{
    if (m_frameSet == nullptr)
        return;
    if (m_frameSet->getSize() == 0)
        return;
    int zoom = 2;

    QPainter p(this);
    int i = 0;
    int ry = 0;
    for (int y = 0; y < 8; y++)
    {
        int rx = 0;
        int nry = 0;
        for (int x = 0; x < 8; x++)
        {
            CFrame &bitmap = *(*m_frameSet)[i];
            const size_t size = bitmap.len() * bitmap.hei() * sizeof(uint32_t);
            uint32_t *rgb = new uint32_t[size];
            nry = std::max(bitmap.hei(), nry);
            memcpy(rgb, bitmap.getRGB(), size);
            for (size_t j = 0; j < size; ++j)
            {
                uint32_t &color = rgb[j];
                if (m_colors.count(color))
                    color = m_colors[color];
            }
            const QImage &img = QImage(reinterpret_cast<uint8_t *>(rgb), bitmap.len(), bitmap.hei(), QImage::Format_RGBX8888);
            const QPixmap &pixmap = QPixmap::fromImage(zoom > 1 ? img.scaled(QSize(bitmap.len() * zoom, bitmap.hei() * zoom)) : img);
            p.drawPixmap(rx, ry, pixmap);
            ++i;
            delete[] rgb;
            if (i >= m_frameSet->getSize())
                break;
            rx += bitmap.len() * zoom;
        }
        if (i >= m_frameSet->getSize())
            break;
        ry += zoom * nry;
    }
    p.end();
}

void CustomWidget::applyColors(std::unordered_map<uint32_t, uint32_t> &colors)
{
    m_colors = colors;
}

void CustomWidget::setFrameSet(CFrameSet *frameSet)
{
    m_frameSet = frameSet;
}
