#include "ziphandler.h"
#include <minizip/unzip.h> // minizip/unzip.h
#include <QFileInfo>
#include <QBuffer>
#include <QImageReader>
#include <QDir>
#include <QTemporaryFile>
#include <QDebug>

static const QStringList IMAGE_EXTS = {"png", "jpg", "jpeg", "bmp", "gif", "webp", "zip", "svg"};

ZipHandler::ZipHandler(const QString &zipPath, int limit, QObject *parent)
    : QObject(parent), m_limit(limit), m_zipPath(zipPath)
{
    openZip();
}

ZipHandler::~ZipHandler()
{
    // remove temp files we kept (if any)
    for (const QString &p : m_tempFiles)
    {
        QFile::remove(p);
    }
    closeZip();
}

bool ZipHandler::openZip()
{
    if (m_unzHandle)
        return true;
    QByteArray pathUtf8 = m_zipPath.toUtf8();
    unzFile uf = unzOpen(pathUtf8.constData());
    if (!uf)
    {
        qWarning() << "Failed to open zip:" << m_zipPath;
        return false;
    }
    m_unzHandle = (void *)uf;

    // enumerate entries and cache image file names
    m_entries.clear();
    int i = 0;
    if (unzGoToFirstFile(uf) == UNZ_OK)
    {
        char filename_inzip[1024];
        unz_file_info64 fileInfo;
        do
        {
            if (unzGetCurrentFileInfo64(uf, &fileInfo, filename_inzip, sizeof(filename_inzip), nullptr, 0, nullptr, 0) == UNZ_OK)
            {
                QString name = QString::fromUtf8(filename_inzip);
                QFileInfo fi(name);
                QString ext = fi.suffix().toLower();
                if (IMAGE_EXTS.contains(ext))
                {
                    qint64 fileSize = fileInfo.uncompressed_size;
                    m_entries.append({name, fileSize});
                }
            }
            ++i;
            if (m_limit > 0 && i >= m_limit) break;
        } while (unzGoToNextFile(uf) == UNZ_OK);
    }
    return true;
}

void ZipHandler::closeZip()
{
    if (!m_unzHandle)
        return;
    unzFile uf = (unzFile)m_unzHandle;
    unzClose(uf);
    m_unzHandle = nullptr;
}

QList<ImgInfo> & ZipHandler::listImageEntries()
{
    return m_entries;
}

QByteArray ZipHandler::readEntryToMemory(const QString &entryName, qint64 &outUncompressedSize)
{
    outUncompressedSize = 0;
    QByteArray result;
    if (!m_unzHandle)
        return result;
    unzFile uf = (unzFile)m_unzHandle;
    if (unzLocateFile(uf, entryName.toUtf8().constData(), 0) != UNZ_OK)
    {
        qWarning() << "entry not found:" << entryName;
        return result;
    }
    unz_file_info64 fileInfo;
    if (unzGetCurrentFileInfo64(uf, &fileInfo, nullptr, 0, nullptr, 0, nullptr, 0) != UNZ_OK)
    {
        qWarning() << "could not get file info for" << entryName;
        return result;
    }
    outUncompressedSize = (qint64)fileInfo.uncompressed_size;

    if (unzOpenCurrentFile(uf) != UNZ_OK)
    {
        qWarning() << "could not open current file for reading:" << entryName;
        return result;
    }

    const int CHUNK = 16384;
    QByteArray buffer;
    buffer.reserve(qMin<qint64>(outUncompressedSize, 65536));
    char temp[CHUNK];
    int readBytes = 0;
    while ((readBytes = unzReadCurrentFile(uf, temp, CHUNK)) > 0)
    {
        buffer.append(temp, readBytes);
    }
    unzCloseCurrentFile(uf);
    return buffer;
}

QString ZipHandler::extractEntryToTempFile(const QString &entryName)
{
    qint64 uncompressedSize = 0;
    QByteArray data = readEntryToMemory(entryName, uncompressedSize);
    if (data.isEmpty())
        return QString();

    QTemporaryFile tmp;
    tmp.setAutoRemove(false); // we will remove in destructor
    tmp.setFileTemplate(QDir::tempPath() + QDir::separator() + "zipimg_XXXXXX." + QFileInfo(entryName).suffix());
    if (!tmp.open())
    {
        qWarning() << "Could not create temp file";
        return QString();
    }
    tmp.write(data);
    tmp.flush();
    tmp.close();
    QString path = tmp.fileName();
    m_tempFiles.append(path);
    return path;
}

QImage ZipHandler::loadImage(const QString &entryName)
{
    if (!m_unzHandle)
        return QImage();
    qint64 uncompressedSize = 0;
    // peek size by getting file info
    unzFile uf = (unzFile)m_unzHandle;
    if (unzLocateFile(uf, entryName.toUtf8().constData(), 0) != UNZ_OK)
    {
        qWarning() << "entry not found:" << entryName;
        return QImage();
    }
    unz_file_info64 fileInfo;
    if (unzGetCurrentFileInfo64(uf, &fileInfo, nullptr, 0, nullptr, 0, nullptr, 0) != UNZ_OK)
    {
        qWarning() << "could not get file info for" << entryName;
        return QImage();
    }
    uncompressedSize = (qint64)fileInfo.uncompressed_size;

    // threshold: 100 KB (102400 bytes)
    const qint64 THRESH = 100 * 1024;

    if (uncompressedSize > THRESH)
    {
        // extract to temp file and load from disk
        QString path = extractEntryToTempFile(entryName);
        if (path.isEmpty())
            return QImage();
        QImage img(path);
        return img;
    }
    else
    {
        qint64 dummySize = 0;
        QByteArray data = readEntryToMemory(entryName, dummySize);
        if (data.isEmpty())
            return QImage();
        QImageReader reader;
        QBuffer buffer(&data);
        buffer.open(QIODevice::ReadOnly);
        reader.setDevice(&buffer);
        QImage img = reader.read();
        buffer.close();
        return img;
    }
}

QImage ZipHandler::loadImageThumbnail(const QString &entryName, const QSize &maxSize)
{
    QImage img = loadImage(entryName);
    if (img.isNull())
        return QImage();
    return img.scaled(maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}


