// imageviewer.h
#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QMainWindow>
#include <QLabel>
#include <QScrollArea>
#include <QListWidget>
#include <QSplitter>
#include <QPixmap>
#include <QDir>
#include <QFileInfoList>

class CFrameSet;

class ImageViewer : public QMainWindow
{
    Q_OBJECT

public:
    explicit ImageViewer(QWidget *parent = nullptr);
    ~ImageViewer();

private slots:
    void openFile();
    void openFolder();
    void onImageSelected(QListWidgetItem *item);
    void onSelectionChange();
    void exportToPng();
    void exportSelectedToPng();
    void zoomIn();
    void zoomOut();
    void fitToWindow();
    void actualSize();
     void toggleInfoPane();

private:
    void createActions();
    void createMenus();
    void loadImage(const QString &filePath);
    void updateImageDisplay();
    void updateInfoPane();
    void populateImageList(const QString &dirPath);
    void scaleImage(double factor);
    QString formatFileSize(qint64 bytes);
    bool isCustomFile(const QString &filepath);
    bool isImageFile(const QString &filepath);
    QString format(const QString &filepath);
    bool isAllSameSize(CFrameSet &set);
    bool readZipFile(const std::string &zipPath, CFrameSet &images, std::string & error);
    bool extractImages(const QString &filePath, QImage &image, QString &error);

    QLabel *imageLabel;
    QScrollArea *scrollArea;
    QListWidget *imageList;
    QSplitter *splitter;
    QWidget *infoPane;
    QLabel *infoLabel;
    
    QPixmap currentImage;
    QString currentFilePath;
    QFileInfoList imageFiles;
    double scaleFactor;
    
    QMenu *fileMenu;
    QMenu *viewMenu;
    QMenu *exportMenu;
    
    QAction *openFileAct;
    QAction *openFolderAct;
    QAction *exportPngAct;
    QAction *exportSelectedPngAct;
    QAction *exitAct;
    QAction *zoomInAct;
    QAction *zoomOutAct;
    QAction *fitToWindowAct;
    QAction *actualSizeAct;
    QAction *toggleInfoAct;
};

#endif // IMAGEVIEWER_H
