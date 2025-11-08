#include "archwrap.h"
#include <archive.h>
#include <archive_entry.h>
#include <iostream>
#include <vector>

#include <QFileInfo>
#include <QBuffer>
#include <QImageReader>
#include <QDir>
#include <QTemporaryFile>
#include <QDebug>

static const QStringList IMAGE_EXTS = {"png", "jpg", "jpeg", "bmp", "gif", "webp", "zip", "svg"};

ArchWrap::ArchWrap(QObject *parent)
    : QObject(parent)
{
    m_arch = nullptr;
}

ArchWrap::~ArchWrap()
{
    m_zipPath = "";
    clear();
    closeZip();
}

bool ArchWrap::renewZip()
{
    m_arch = archive_read_new();
    if (!m_arch) {
        qDebug("archive_read_new failed");
        return false;
    }
    archive_read_support_format_7zip(m_arch); // Enable 7z support
    archive_read_support_format_zip(m_arch);
    archive_read_support_filter_all(m_arch); // Auto-detect compression
    archive_read_support_format_rar(m_arch);
    archive_read_support_format_rar5(m_arch);
    return true;
}

bool ArchWrap::openZip(QString filepath, QProgressBar *progressBar, int limit)
{
    auto extractTmp = [this](const char *entryName, const size_t size)
    {
        //qDebug("extract tmp: %s", entryName);
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


    if (filepath == m_zipPath)
        return true;

    if (m_arch) {
        closeZip();
    }

    if (!renewZip()) {
        return false;;
    }

    struct archive_entry *entry;
    m_zipPath = filepath;
    QByteArray pathUtf8 = m_zipPath.toUtf8();

    if (archive_read_open_filename(m_arch, pathUtf8.constData(), 10240) != ARCHIVE_OK)
    {
        std::cerr << "Failed to open archive: " << archive_error_string(m_arch) << "\n";
        return false;
    }

    qint64 totalBytes = 0;
    while (archive_read_next_header(m_arch, &entry) == ARCHIVE_OK) {
        totalBytes += archive_entry_size(entry);
        archive_read_data_skip(m_arch);
    }

    archive_read_free(m_arch);
    if (!renewZip()) {
        return false;;
    }

    qint64 processedBytes = 0;
    qint64 lastReported = 0;

    qDebug("new ArchWrap: %s", m_zipPath.toStdString().c_str());

    m_entries.clear();
    m_index.clear();

    if (archive_read_open_filename(m_arch, pathUtf8.constData(), 10240) != ARCHIVE_OK)
    {
        std::cerr << "Failed to open archive: " << archive_error_string(m_arch) << "\n";
        return false;
    }

    int i = 0;

    while (archive_read_next_header(m_arch, &entry) == ARCHIVE_OK)
    {
        const char *name = archive_entry_pathname(entry);
        QFileInfo fi(name);
        QString ext = fi.suffix().toLower();
        if (IMAGE_EXTS.contains(ext))
        {
            size_t size = archive_entry_size(entry);
            //  qDebug() << "Found: " << name << " (" << size << " bytes)";
            QString tmpFile = extractTmp(name, size);
            // qDebug() << "tempFile: " << tmpFile << "\n";
            if (tmpFile.isEmpty()) {
                qDebug("**********failed to create tmpFile");
                continue;
            }
            m_entries.append({name, (qint64)size});
            m_index.insert(name, tmpFile);

            processedBytes += size;
            if (totalBytes > 0 && processedBytes - lastReported > 64 * 1024) {
                int percent = int((processedBytes * 100) / totalBytes);
                emit progressChanged(percent);
                if (progressBar)
                    progressBar->setValue(percent);
                lastReported = processedBytes;
            }

        }
        ++i;
        if (limit > 0 && i >= limit)
            break;
    }
    emit progressChanged(100);
    if (progressBar)
        progressBar->setValue(100);

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
}

void ArchWrap::clear()
{
    // remove temp files we kept (if any)
    for (const QString &p : m_tempFiles)
    {
        QFile::remove(p);
    }
    m_entries.clear();
    m_index.clear();
}
