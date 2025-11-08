#pragma once

#include <QObject>
#include <QString>
#include <QImage>
#include <QTemporaryFile>
#include <QMap>
#include <QProgressBar>

#include "imginfo.h"

class ArchWrap : public QObject
{
    Q_OBJECT
public:
    explicit ArchWrap(QObject *parent = nullptr);
    ~ArchWrap();

    bool openZip(QString filepath, QProgressBar *progressBar, int limit =-1);

    QList<ImgInfo> &listImageEntries();
    QImage loadImage(const QString &entryName); // loads full image (may extract to temp file if >100KB)
    void clear();


signals:
    void progressChanged(int percent);
    void finished();
    void errorOccurred(const QString &message);

private:
    bool renewZip();
    void closeZip();
    QByteArray readEntryToMemory(const QString &entryName, qint64 &outUncompressedSize);
    QString extractEntryToTempFile(const QString &entryName);

    QString m_zipPath;
    QMap<QString, QString> m_index;
    struct archive *m_arch;
    QList<QString> m_tempFiles; // keep track of extracted temp files to delete later
    QList<ImgInfo> m_entries;
};
