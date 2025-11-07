#include "archwrap.h"
// #include <minizip/unzip.h> // minizip/unzip.h
#include <archive.h>
#include <archive_entry.h>
#include <iostream>
#include <fstream>
#include <vector>

#include <QFileInfo>
#include <QBuffer>
#include <QImageReader>
#include <QDir>
#include <QTemporaryFile>
#include <QDebug>

static const QStringList IMAGE_EXTS = {"png", "jpg", "jpeg", "bmp", "gif", "webp", "zip", "svg"};

ArchWrap::ArchWrap(const QString &zipPath, int limit, QObject *parent)
    : QObject(parent), m_limit(limit), m_zipPath(zipPath)
{
    openZip();
}

ArchWrap::~ArchWrap()
{
    // remove temp files we kept (if any)
    for (const QString &p : m_tempFiles)
    {
        QFile::remove(p);
    }
    closeZip();
}

bool ArchWrap::openZip()
{
    auto extractTmp = [this](const char *entryName, const size_t size)
    {
        std::vector<char> buffer(size);
        archive_read_data(m_arch, buffer.data(), buffer.size());

        if (!buffer.size())
            return QString();

        QTemporaryFile tmp;
        tmp.setAutoRemove(false); // we will remove in destructor
        tmp.setFileTemplate(QDir::tempPath() + QDir::separator() + "zipimg_XXXXXX." + QFileInfo(entryName).suffix());
        if (!tmp.open())
        {
            qWarning() << "Could not create temp file";
            return QString();
        }
        tmp.write(buffer.data(), size);
        tmp.flush();
        tmp.close();
        QString path = tmp.fileName();
        m_tempFiles.append(path);
        return path;
    };

    if (m_arch)
        return true;

    m_arch = archive_read_new();
    archive_read_support_format_7zip(m_arch); // Enable 7z support
    archive_read_support_format_zip(m_arch);
    archive_read_support_filter_all(m_arch); // Auto-detect compression

    QByteArray pathUtf8 = m_zipPath.toUtf8();
    if (archive_read_open_filename(m_arch, pathUtf8.constData(), 10240) != ARCHIVE_OK)
    {
        std::cerr << "Failed to open archive: " << archive_error_string(m_arch) << "\n";
        return false;
    }

    int i = 0;
    struct archive_entry *entry;
    while (archive_read_next_header(m_arch, &entry) == ARCHIVE_OK)
    {
        const char *name = archive_entry_pathname(entry);
        QFileInfo fi(name);
        QString ext = fi.suffix().toLower();
        if (IMAGE_EXTS.contains(ext))
        {
            size_t size = archive_entry_size(entry);
            qDebug() << "Found: " << name << " (" << size << " bytes)\n";
            QString tmpFile = extractTmp(name, size);
            qDebug() << "tempFile: " << tmpFile << "\n";
            m_entries.append({name, (qint64)size});
            m_index.insert(name, tmpFile);
        }

        // m_index.insert(archive_entry_pathname(entry), archive_filter_bytes(m_arch));
        // archive_read_data_skip(m_arch);

        /*
        std::vector<char> buffer(size);
        archive_read_data(m_arch, buffer.data(), buffer.size());

        // Example: write extracted file
        std::ofstream out(name, std::ios::binary);
        out.write(buffer.data(), buffer.size());
        */
        ++i;
        if (m_limit > 0 && i >= m_limit)
            break;
    }

    return true;
}

void ArchWrap::closeZip()
{
    if (m_arch)
        archive_read_free(m_arch);
    m_arch = nullptr;
}

QList<ImgInfo> &ArchWrap::listImageEntries()
{
    return m_entries;
}

QImage ArchWrap::loadImage(const QString &entryName)
{
    if (!m_arch)
        return QImage();

    const auto &it = m_index.find(entryName);
    if (it != m_index.end())
    {
        return QImage(it.value());
    }
    else
    {
        return QImage();
    }

    /*


    QFile file(tmpName);
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "Cannot open file:" << file.errorString();
        return;
    }

    QByteArray data = file.readAll();
    qDebug() << "Read" << data.size() << "bytes from" << tmpName;

    // Example: show first few bytes in hex
    qDebug() << "First 16 bytes:" << data.left(16).toHex(' ');

    struct archive_entry *entry;

    std::vector<char> buffer(size);
    archive_read_data(m_arch, buffer.data(), buffer.size());

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
        */
}
