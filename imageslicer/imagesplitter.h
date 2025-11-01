#ifndef IMAGESPLITTER_H
#define IMAGESPLITTER_H

#include <QMainWindow>
#include <QImage>
#include <QTableWidget>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QRegularExpression>

class SavePreviewDialog : public QDialog {
    Q_OBJECT

public:
    explicit SavePreviewDialog(const QList<QPair<QImage, QPair<int, int>>> &tiles, QWidget *parent = nullptr);
    QList<QPair<QString, QImage>> getFilenamesAndImages() const;

private:
    void setupUi(const QList<QPair<QImage, QPair<int, int>>> &tiles);
    QTableWidget *previewTable;
    QPushButton *saveButton;
    QPushButton *cancelButton;
    QList<QCheckBox*> confirmCheckboxes;
    QList<QLineEdit*> filenameFields;

    // Static member to avoid temporary QRegularExpression
    static const QRegularExpression filenameSanitizer;
};

class ImageSplitter : public QMainWindow {
    Q_OBJECT

public:
    explicit ImageSplitter(QWidget *parent = nullptr);
    ~ImageSplitter() override;

private slots:
    void loadImage();
    void saveSelectedTiles();
    void updateTileSize(int index);
    void selectAllTiles();
    void deselectAllTiles();

private:
    void setupUi();
    void splitImage();
    void clearTable();

    QImage originalImage;
    QLabel *imageLabel;
    QTableWidget *tileTable;
    QPushButton *loadButton;
    QPushButton *saveButton;
    QPushButton *selectAllButton;
    QPushButton *deselectAllButton;
    QComboBox *tileSizeCombo;
    QCheckBox *uniqueCheckbox;
    QVBoxLayout *layout;
    QWidget *centralWidget;
    int currentTileSize = 16;
};

#endif // IMAGESPLITTER_H
