#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QStackedWidget>
#include <QSplitter>
#include <QAction>
#include <QMenuBar>
#include <QToolBar>
#include "ziphandler.h"
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


    QListWidget *m_listWidget;
    QStackedWidget *m_rightStack;
    ImageViewer *m_imageViewer;
    ThumbnailGrid *m_thumbGrid;

    ZipHandler *m_zipHandler = nullptr;
    QString m_currentFolder;
    QString m_currentZip;
};

#endif // MAINWINDOW_H
