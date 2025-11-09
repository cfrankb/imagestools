#ifndef THUMBNAILGRID_H
#define THUMBNAILGRID_H

#include <QWidget>
#include <QScrollArea>
#include <QGridLayout>
#include <QMap>
#include <QLabel>
#include <QPixmap>

class ThumbnailGrid : public QScrollArea
{
    Q_OBJECT
public:
    explicit ThumbnailGrid(QWidget *parent = nullptr);
    void addThumbnail(const QString &entryName, const QImage &img, qint64 fileSize);
    void clear();
    void setBackgroundColor(const QColor &color);



private:
    QWidget *m_container;
    QGridLayout *m_layout;
    QMap<QString, QLabel *> m_labelMap;
    int m_columns = 3;
    void onLabelClicked();

signals:
    void thumbnailClicked(const QString &entryName);
    void requestSaveOriginal(const QString &entryName);


private slots:
    void saveThumbnailAsPng(const QString &entryName);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
};

#endif // THUMBNAILGRID_H
