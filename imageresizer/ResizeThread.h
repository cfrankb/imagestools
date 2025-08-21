#pragma once

#include <QtCore/QThread>
#include <QtCore/QMimeData>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDropEvent>
#include <QtGui/QPixmap>
#include <QList>
#include "ImageInfo.h"


class ResizeThread : public QThread
{
    Q_OBJECT

public:
    ResizeThread(const QList<ImageInfo> &imageItems, int targetWidth, int targetHeight, bool maintainAspect, const QString &savePath, QObject *parent = nullptr);

signals:
    void progress(int value);
    void finished(const QString &zipPath);
    void error(const QString &errorMessage);

protected:
    void run() override;

private:
    QList<ImageInfo> m_imageItems;
    int m_targetWidth;
    int m_targetHeight;
    bool m_maintainAspect;
    QString m_savePath;
};

