#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QStackedWidget>
#include <QSplitter>
#include <QAction>
#include <QMenuBar>
#include <QToolBar>
#include <QStringList>
#include "ziphandler.h"
#include "archwrap.h"
#include "imageviewer.h"
#include "thumbnailgrid.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void openFolder();
    void openZip();
    void onListItemActivated(QListWidgetItem *item);
    void showImageFromZip(const QString &entryName);

private:
    void listFilesFromFolder(const QString &folderPath);
    void listFilesFromZip(const QString &zipPath);
    void clearState();
    void previewZip(const QString &zipPath);
    void onImageSelected(int row);


    void keyPressEvent(QKeyEvent *event) override;

    QListWidget *m_listWidget;
    QStackedWidget *m_rightStack;
    ImageViewer *m_imageViewer;
    ThumbnailGrid *m_thumbGrid;

    //ZipHandler *m_zipHandler = nullptr;
    ArchWrap  *m_zipHandler = nullptr;
    QString m_currentFolder;
    QString m_currentZip;
    QStringList m_imageFiles;

    enum Mode {
        FolderMode,
        ZipMode
    };

    Mode m_mode;
};

#endif // MAINWINDOW_H
