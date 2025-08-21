#pragma once
#include <QMainWindow>
#include <QImage>
#include <QListWidgetItem>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct ImagePair {
    QString fileName;
    QImage original;
    QImage processed;
    bool selected = true;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void processImages();
    void saveZip();
    void selectAll();
    void selectNone();

private:
    Ui::MainWindow *ui;
    QList<ImagePair> images;

    QImage trimAndCenter(const QImage &img);
    void updatePreview();
};
