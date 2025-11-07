#pragma once

#include <QObject>
#include <QString>
#include <QImage>
#include <QTemporaryFile>

#include "imginfo.h"

class ZipHandler : public QObject
{
    Q_OBJECT
public:
    explicit ZipHandler(const QString &zipPath, int limit = -1, QObject *parent = nullptr);
    ~ZipHandler();

    QList<ImgInfo> &listImageEntries();
    QImage loadImage(const QString &entryName);                                // loads full image (may extract to temp file if >100KB)
    QImage loadImageThumbnail(const QString &entryName, const QSize &maxSize); // scaled thumbnail

private:
    int m_limit = -1;
    bool openZip();
    void closeZip();
    QByteArray readEntryToMemory(const QString &entryName, qint64 &outUncompressedSize);
    QString extractEntryToTempFile(const QString &entryName);

    QString m_zipPath;
    void *m_unzHandle = nullptr; // opaque pointer to unzFile; we cast as needed
    QList<QString> m_tempFiles;  // keep track of extracted temp files to delete later
    QList<ImgInfo> m_entries;
};
