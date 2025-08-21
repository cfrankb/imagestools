#ifndef CUSTOMWIDGET_H
#define CUSTOMWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <unordered_map>

class CFrameSet;

class CustomWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CustomWidget(QWidget *parent = nullptr);
    ~CustomWidget() override;
    void applyColors(std::unordered_map<uint32_t, uint32_t> &m_colors);
    void setFrameSet(CFrameSet *frameSet);

private:
    void setupUi();
    void paintEvent(QPaintEvent *) override;
    CFrameSet *m_frameSet;
    std::unordered_map<uint32_t, uint32_t> m_colors;
};

#endif // CUSTOMWIDGET_H
