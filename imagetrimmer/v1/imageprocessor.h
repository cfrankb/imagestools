// ImageProcessor.h
#pragma once
#include <QImage>

class ImageProcessor {
public:
    static QImage trimAndCenter(const QImage &img, int finalSize = 16);
};
