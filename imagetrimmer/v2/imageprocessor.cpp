// ImageProcessor.cpp
#include "imageprocessor.h"
#include <algorithm>
#include <QPainter>

QImage ImageProcessor::trimAndCenter(const QImage &img, int finalSize) {
    if (img.isNull()) return img;

    int minX = img.width(), minY = img.height();
    int maxX = -1, maxY = -1;

    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            if (qAlpha(img.pixel(x, y)) > 0) {
                minX = std::min(minX, x);
                minY = std::min(minY, y);
                maxX = std::max(maxX, x);
                maxY = std::max(maxY, y);
            }
        }
    }

    if (maxX < minX || maxY < minY) {
        return QImage(finalSize, finalSize, QImage::Format_ARGB32_Premultiplied);
    }

    QImage cropped = img.copy(minX, minY, maxX - minX + 1, maxY - minY + 1);

    QImage finalImg(finalSize, finalSize, QImage::Format_ARGB32_Premultiplied);
    finalImg.fill(Qt::transparent);

    QPainter p(&finalImg);
    int offsetX = (finalSize - cropped.width()) / 2;
    int offsetY = (finalSize - cropped.height()) / 2;
    p.drawImage(offsetX, offsetY, cropped);
    p.end();

    return finalImg;
}
