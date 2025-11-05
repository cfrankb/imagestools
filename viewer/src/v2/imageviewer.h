#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QWidget>
#include <QLabel>
#include <QScrollArea>
#include <QImage>

class ImageViewer : public QWidget
{
    Q_OBJECT
public:
    ImageViewer(QWidget *parent = nullptr);
    void setImage(const QImage &img);
    void clear();

private:
    QLabel *m_imageLabel;
    QScrollArea *m_scrollArea;
};

#endif // IMAGEVIEWER_H
