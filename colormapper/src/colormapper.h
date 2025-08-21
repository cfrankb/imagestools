#pragma once
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QPushButton>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QWidget>

class CustomWidget;
class CFrameSet;

class ColorMapperWindow : public QMainWindow
{
    Q_OBJECT

public:
    ColorMapperWindow(QWidget *parent = nullptr);

private slots:
    void onCellClicked(int row, int column);
    void onCellDoubleClicked(int row, int column);
    void onOpenClicked();
    void onSaveClicked();
    void onLoadClicked();
    void onApplyClicked();
    void onPatchClicked();
    void onGreyClicked();

private:
    QTableWidget *table;
    CustomWidget *m_tab2;
    CFrameSet *m_frameSet;

    void populateColors(const QMap<QString, QString> &map);
    bool extractColors(const QString &filename);
    void makeColorMap(std::unordered_map<uint32_t, uint32_t> &map);
    uint32_t swapRxB(const uint32_t color);
    enum
    {
        ALPHA = 0xff000000
    };
};
