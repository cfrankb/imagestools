#pragma once

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QGroupBox>
#include <QtCore/QThread>
#include <QtCore/QMimeData>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDropEvent>
#include <QtGui/QPixmap>
#include <QList>

class ImageItem;
class ResizeThread;

class ImageResizerApp : public QMainWindow
{
    Q_OBJECT

public:
    ImageResizerApp(QWidget *parent = nullptr);
    ~ImageResizerApp();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void addFiles();
    void updatePreviews();
    void selectAll();
    void selectNone();
    void exportImages();
    void exportFinished(const QString &zipPath);
    void exportError(const QString &errorMessage);
    void updateProgress(int value);
    void clearAll();

private:
    void setupUI();
    void addImageFiles(const QStringList &filePaths);
    bool isImageFile(const QString &filePath);
    void loadSettings();
    void saveSettings();

    QWidget *m_centralWidget;
    QVBoxLayout *m_mainLayout;
    QSpinBox *m_widthSpinBox;
    QSpinBox *m_heightSpinBox;
    QCheckBox *m_aspectCheckBox;
    QPushButton *m_selectAllBtn;
    QPushButton *m_selectNoneBtn;
    QPushButton *m_addFilesBtn;
    QPushButton *m_exportBtn;
    QPushButton *m_clearAllBtn;
    QLabel *m_dropLabel;
    QScrollArea *m_scrollArea;
    QWidget *m_scrollWidget;
    QVBoxLayout *m_scrollLayout;
    QProgressBar *m_progressBar;
    
    QList<ImageItem*> m_imageItems;
    ResizeThread *m_resizeThread;
};

