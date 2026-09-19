#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QImage>
#include <QStringList>
#include <QMenu>
#include <QAction>
#include <QDockWidget>
#include <QFormLayout>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QCheckBox>
#include <QGridLayout>
#include <QToolBar>
#include <QWidget>
#include <QVBoxLayout>
#include <QGraphicsView>
#include <QSettings>
#include <QFileInfo>
#include <QCloseEvent>
#include <QMessageBox>
#include <QSharedMemory>

#include "tileitem.h"
#include "tilescene.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow();

private slots:
    void onGranularToggle(const uint8_t flag, bool on);
    void applyProperties();
    void exitSelectMode();
    void keyPressEvent(QKeyEvent *event) override;
    void loadImage();
    void setTileSize(int s);
    void showContextMenu(const QPoint &screenPos);
    void setZoomPreset(int percent);
    void saveJson();
    void loadJson();
    bool readJson(const QString &fn);
    void createToolbar();
    void createTiles();
    void updatePropertiesPanel();
    void updateRecentFilesMenu();
    void addRecentFile(const QString &path);
    void readSettings();
    void writeSettings();
    void closeEvent(QCloseEvent *event) override;

private:
    void createDockWidget();
    void setModified(bool modified);
    void refreshTitle();
    bool confirmDiscard();

    TileScene *m_scene;
    QGraphicsView *m_view;
    QImage m_image;
    QString m_imageFile;
    int m_tileSize = 16;
    int m_currentZoom = 100;

    QLabel *m_label;
    QComboBox *m_typeBox;
    QLineEdit *m_tagEdit;
    QSpinBox *m_nextSpin;
    QSpinBox *m_speedSpin;
    QSpinBox *m_weightSpin;
    QPushButton *m_applyButton;
    QPushButton *m_selectTileButton;

    bool m_selectMode = false;

    QCheckBox *m_granUL;
    QCheckBox *m_granUR;
    QCheckBox *m_granDL;
    QCheckBox *m_granDR;

    QString m_imageFolder;
    QString m_jsonFolder;
    QString m_jsonFile;
    QStringList m_recentFiles;
    QMenu *m_recentMenu;

    bool m_modified = false;
    QSharedMemory *m_sharedMemory = nullptr;
};

#endif // MAINWINDOW_H
