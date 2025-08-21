#pragma once
#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QCheckBox>
#include <QPixmap>
#include <QList>

class ImageItem : public QWidget
{
    Q_OBJECT

public:
    explicit ImageItem(const QString &filePath, QWidget *parent = nullptr);
    
    void updatePreview(int targetWidth, int targetHeight, bool maintainAspect);
    bool isSelected() const;
    void setSelected(bool selected);
    QString getFilePath() const { return m_filePath; }
    QPixmap getOriginalPixmap() const { return m_originalPixmap; }

private:
    void setupUI();
    void loadImage();

    QString m_filePath;
    QPixmap m_originalPixmap;
    QPixmap m_resizedPixmap;
    
    QCheckBox *m_checkBox;
    QLabel *m_originalLabel;
    QLabel *m_previewLabel;
    QLabel *m_originalSizeLabel;
    QLabel *m_previewSizeLabel;
};
