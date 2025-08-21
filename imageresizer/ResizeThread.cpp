// ResizeThread.cpp
#include "ResizeThread.h"
#include <QtCore/QTemporaryDir>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtGui/QPixmap>
#include <QtCore/QProcess>
#include <QtCore/QDebug>
#include <qbuffer.h>
#include <minizip/zip.h>  // Minizip header
#include <fstream>
#include <vector>
#include <string>


ResizeThread::ResizeThread(const QList<ImageInfo> &imageItems, int targetWidth, int targetHeight, bool maintainAspect, const QString &savePath,QObject *parent)
    : QThread(parent)
    , m_imageItems(imageItems)
    , m_targetWidth(targetWidth)
    , m_targetHeight(targetHeight)
    , m_maintainAspect(maintainAspect)
    , m_savePath(savePath)
{
}


int createZip(const std::string &zipName, const std::string &filePath) {
    zipFile zf = zipOpen(zipName.c_str(), APPEND_STATUS_CREATE);
    if (!zf) return 1;

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) return 2;

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    file.read(buffer.data(), size);

    zip_fileinfo zfi = {};
    std::string entryName = filePath.substr(filePath.find_last_of("/\\") + 1);

    int err = zipOpenNewFileInZip(zf, entryName.c_str(), &zfi,
                                  nullptr, 0, nullptr, 0,
                                  "File added via Minizip",
                                  Z_DEFLATED, Z_DEFAULT_COMPRESSION);
    if (err != ZIP_OK) return 3;

    err = zipWriteInFileInZip(zf, buffer.data(), buffer.size());
    if (err != ZIP_OK) return 4;

    zipCloseFileInZip(zf);
    zipClose(zf, nullptr);
    return 0;
}

void ResizeThread::run()
{
    // Create temporary directory
    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        emit error("Failed to create temporary directory");
        return;
    }
    
    QString zipPath = m_savePath;
    
    if (m_imageItems.isEmpty()) {
        emit error("No images selected for processing");
        return;
    }
    
    // Create list of files to zip
    QStringList filesToZip;
    
    for (int i = 0; i < m_imageItems.size(); ++i) {
        const auto & item = m_imageItems[i];
        const QPixmap &originalPixmap = item.pixmap;
        
        if (originalPixmap.isNull()) continue;
        
        // Calculate dimensions
        int newWidth, newHeight;
        if (m_maintainAspect) {
            double aspectRatio = static_cast<double>(originalPixmap.width()) / originalPixmap.height();
            
            if (static_cast<double>(m_targetWidth) / m_targetHeight > aspectRatio) {
                newHeight = m_targetHeight;
                newWidth = static_cast<int>(m_targetHeight * aspectRatio);
            } else {
                newWidth = m_targetWidth;
                newHeight = static_cast<int>(m_targetWidth / aspectRatio);
            }
        } else {
            newWidth = m_targetWidth;
            newHeight = m_targetHeight;
        }
        
        // Resize pixmap
        QPixmap resizedPixmap = originalPixmap.scaled(newWidth, newHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        
        // Save to temporary file
        QFileInfo fileInfo(item.filepath);
        QString tempImagePath = tempDir.path() + "/" + fileInfo.baseName() + "." + fileInfo.suffix();
        
        if (!resizedPixmap.save(tempImagePath)) {
            emit error(QString("Failed to save resized image: %1").arg(tempImagePath));
            return;
        }
        
        filesToZip.append(tempImagePath);
        
        // Update progress
        int progressValue = static_cast<int>((i + 1.0) / m_imageItems.size() * 100);
        emit progress(progressValue);
    }
    
    qDebug() << filesToZip;

    zipFile zf = zipOpen(zipPath.toStdString().c_str(), APPEND_STATUS_CREATE);
    if (!zf) {
        emit error( "Could not create file ZIP.");
        return;
    }

    for (const auto &filepath : filesToZip) {
        QFileInfo fileInfo(filepath);
        QString filename = fileInfo.fileName();
        QFile sfile(filepath);
        if (sfile.open( QIODevice::ReadOnly)) {
            QByteArray pngData = sfile.readAll();
            sfile.close();
            zip_fileinfo zfi = {};
            int err = zipOpenNewFileInZip(zf, filename.toStdString().c_str(), &zfi,
                                          nullptr, 0, nullptr, 0,
                                          "",//"File added via Minizip",
                                          Z_DEFLATED, Z_DEFAULT_COMPRESSION);
            if (err != ZIP_OK){
                emit error( "Could not zipOpenNewFileInZip.");
                return;
            }
            err = zipWriteInFileInZip(zf, pngData.constData(), pngData.size());
            if (err != ZIP_OK) {
                emit error( "Could not zipWriteInFileInZip.");
                return ;
            }
        } else {
            emit error( "Could not read file.");
            return;
        }
    }
    zipCloseFileInZip(zf);
    zipClose(zf, nullptr);
    qDebug() << "ZIP archive saved successfully.";
    emit finished(zipPath);
}
