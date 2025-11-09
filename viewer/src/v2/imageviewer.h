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
    void setImage(const QImage &img, const QString &path, bool fromZip);
    void clear();
    void setBackgroundColor(const QColor &color);


private:
    bool m_fromZip;
    QString m_path;
    QLabel *m_imageLabel;
    QScrollArea *m_scrollArea;

signals:
    void requestSaveOriginal(const QString &entryName, bool fromZip);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
};

#endif // IMAGEVIEWER_H
