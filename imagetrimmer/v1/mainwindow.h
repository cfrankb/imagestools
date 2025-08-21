#pragma once
#include <QMainWindow>
#include <QImage>
#include <QVector>

class QListWidget;
class QLabel;
class QPushButton;
class QCheckBox;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void processImages();
    void saveZip();
    void updatePreview();

private:
    QImage trimAndCenter(const QImage &src);

    QListWidget *thumbList = nullptr;
    QLabel *beforePreview = nullptr;
    QLabel *afterPreview = nullptr;
    QPushButton *processBtn = nullptr;
    QPushButton *saveBtn = nullptr;
    QCheckBox *showAfterCheckbox = nullptr;

    QVector<QImage> originals;
    QVector<QImage> processed;
    bool showAfter = true;
};
