#include <QColorDialog>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <set>
#include <unordered_map>
#include "colormapper.h"
#include "shared/FrameSet.h"
#include "shared/Frame.h"
#include "shared/qtgui/qfilewrap.h"
#include "customwidget.h"

ColorMapperWindow::ColorMapperWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("Color Mapper");
    resize(500, 400);

    QTabWidget *tabWidget = new QTabWidget(this);
    setCentralWidget(tabWidget);

    // --- UI Setup ---
    QWidget *centralWidget = new QWidget(this);
    tabWidget->addTab(centralWidget, "Colors");
    QVBoxLayout *vlayout = new QVBoxLayout(centralWidget);
    QHBoxLayout *hlayout = new QHBoxLayout();

    table = new QTableWidget(this);
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels({"Original Color", "Replacement Color"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QPushButton *saveButton = new QPushButton("Save Mapping", this);
    QPushButton *loadButton = new QPushButton("Load Mapping", this);
    QPushButton *openButton = new QPushButton("Open Image", this);
    QPushButton *greyButton = new QPushButton("Grey Scale", this);
    QPushButton *applyButton = new QPushButton("Apply", this);
    QPushButton *patchButton = new QPushButton("Patch", this);

    vlayout->addWidget(table);
    hlayout->addWidget(loadButton);
    hlayout->addWidget(saveButton);
    hlayout->addWidget(applyButton);
    vlayout->addLayout(hlayout);
    vlayout->addWidget(openButton);
    vlayout->addWidget(patchButton);
    vlayout->addWidget(greyButton);

    // --- Connect Signals and Slots ---
    connect(table, &QTableWidget::cellClicked, this, &ColorMapperWindow::onCellClicked);
    connect(table, &QTableWidget::cellDoubleClicked, this, &ColorMapperWindow::onCellDoubleClicked);
    connect(saveButton, &QPushButton::clicked, this, &ColorMapperWindow::onSaveClicked);
    connect(loadButton, &QPushButton::clicked, this, &ColorMapperWindow::onLoadClicked);
    connect(openButton, &QPushButton::clicked, this, &ColorMapperWindow::onOpenClicked);
    connect(applyButton, &QPushButton::clicked, this, &ColorMapperWindow::onApplyClicked);
    connect(patchButton, &QPushButton::clicked, this, &ColorMapperWindow::onPatchClicked);
    connect(greyButton, &QPushButton::clicked, this, &ColorMapperWindow::onGreyClicked);

    m_tab2 = new CustomWidget(this);
    tabWidget->addTab(m_tab2, "Preview");
    m_frameSet = new CFrameSet;
}

// Slot to handle cell clicks in the table
void ColorMapperWindow::onCellClicked(int row, int column)
{
    // Only allow changing the "Replacement Color" column
    if (column == 1)
    {
        QTableWidgetItem *item = table->item(row, column);
        QColor currentColor = item->background().color();
        QColor newColor = QColorDialog::getColor(currentColor, this, "Choose a new replacement color");
        // If the user selected a new, valid color
        if (newColor.isValid() && newColor != currentColor)
        {
            // Update the background color of the table cell
            item->setBackground(QBrush(newColor));
            item->setText(newColor.name());
        }
    }
}

void ColorMapperWindow::onCellDoubleClicked(int row, int column)
{
    if (column == 0)
    {
        QTableWidgetItem *item = table->item(row, column);
        QColor currentColor = item->background().color();

        QTableWidgetItem *item2 = table->item(row, 1);
        item2->setBackground(QBrush(currentColor));
        item2->setText(currentColor.name());
    }
}

void ColorMapperWindow::onApplyClicked()
{
    std::unordered_map<uint32_t, uint32_t> colors;
    makeColorMap(colors);
    m_tab2->applyColors(colors);
}

void ColorMapperWindow::makeColorMap(std::unordered_map<uint32_t, uint32_t> &colors)
{
    for (int i = 0; i < table->rowCount(); ++i)
    {
        QTableWidgetItem *originalItem = table->item(i, 0);
        QTableWidgetItem *replacementItem = table->item(i, 1);

        if (originalItem && replacementItem)
        {
            // Store colors as hex strings for easy saving/loading
            const auto org = originalItem->background().color().name();
            const auto rep = replacementItem->background().color().name();
            uint32_t oColor = swapRxB(org.sliced(1).toUInt(nullptr, 16)) | ALPHA;
            uint32_t rColor = swapRxB(rep.sliced(1).toUInt(nullptr, 16)) | ALPHA;
            colors[oColor] = rColor;
        }
    }
}

void ColorMapperWindow::onPatchClicked()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Save Color Patch", "", "Ini Files (*.ini)");
    if (filePath.isEmpty())
    {
        return;
    }
    if (!filePath.endsWith(".ini"))
        filePath += ".ini";

    QFileWrap file;
    if (!file.open(filePath.toStdString().c_str(), "wb"))
    {
        qDebug("failed to create file");
        return;
    }
    std::unordered_map<uint32_t, uint32_t> colors;
    makeColorMap(colors);
    file += "[color-patch]\n";
    for (const auto &[k, v] : colors)
    {
        if (k != v)
        {
            file += QString("0x%1").arg(k | ALPHA, 8, 16, QChar('0')).toStdString();
            file += std::string(" ");
            file += QString("0x%1").arg(v | ALPHA, 8, 16, QChar('0')).toStdString();
            file += std::string("\n");
        }
    }
    file += "\n\n";

    file += "// color-patch\n";
    file += "std::unordered_map<uint32_t, uint32_t> colors={\n";
    for (const auto &[k, v] : colors)
    {
        if (k != v)
        {
            file += "    {";
            file += QString("0x%1").arg(k | ALPHA, 8, 16, QChar('0')).toStdString();
            file += std::string(", ");
            file += QString("0x%1").arg(v | ALPHA, 8, 16, QChar('0')).toStdString();
            file += std::string("},\n");
        }
    }
    file += "};\n\n";
    file.close();
}

// Slot to handle the "Save" button click
void ColorMapperWindow::onSaveClicked()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Save Color Mapping", "", "JSON Files (*.json)");
    if (filePath.isEmpty())
    {
        return;
    }
    if (!filePath.endsWith(".json"))
        filePath += ".json";

    QJsonArray jsonArray;
    for (int i = 0; i < table->rowCount(); ++i)
    {
        QTableWidgetItem *originalItem = table->item(i, 0);
        QTableWidgetItem *replacementItem = table->item(i, 1);
        if (originalItem && replacementItem)
        {
            QJsonObject jsonObject;
            // Store colors as hex strings for easy saving/loading
            jsonObject["original"] = originalItem->background().color().name();
            jsonObject["replacement"] = replacementItem->background().color().name();
            jsonArray.append(jsonObject);
        }
    }

    QJsonDocument jsonDoc(jsonArray);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream stream(&file);
        stream << jsonDoc.toJson();
        file.close();
        QMessageBox::information(this, "Success", "Color mapping saved successfully.");
    }
    else
    {
        QMessageBox::warning(this, "Error", "Could not save the file.");
    }
}

// Slot to handle the "Load" button click
void ColorMapperWindow::onLoadClicked()
{

    QString filePath = QFileDialog::getOpenFileName(this, "Load Color Mapping", "", "JSON Files (*.json)");
    if (filePath.isEmpty())
    {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "Error", "Could not open the file.");
        return;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isArray())
    {
        QMessageBox::warning(this, "Error", "Invalid JSON format in the file.");
        return;
    }

    table->clearContents();
    table->setRowCount(0);

    QJsonArray jsonArray = jsonDoc.array();
    for (const QJsonValue &value : jsonArray)
    {
        QJsonObject jsonObject = value.toObject();
        QColor originalColor(jsonObject["original"].toString());
        QColor replacementColor(jsonObject["replacement"].toString());

        if (originalColor.isValid() && replacementColor.isValid())
        {
            int row = table->rowCount();
            table->insertRow(row);

            // Original Color (read-only)
            QTableWidgetItem *originalItem = new QTableWidgetItem();
            originalItem->setBackground(QBrush(originalColor));
            originalItem->setFlags(originalItem->flags() & ~Qt::ItemIsEditable);
            originalItem->setText(originalColor.name());
            table->setItem(row, 0, originalItem);

            // Replacement Color (editable)
            QTableWidgetItem *replacementItem = new QTableWidgetItem();
            replacementItem->setBackground(QBrush(replacementColor));
            replacementItem->setText(replacementColor.name());
            table->setItem(row, 1, replacementItem);
        }
    }
    QMessageBox::information(this, "Success", "Color mapping loaded successfully.");
}

void ColorMapperWindow::onOpenClicked()
{
    QString fileName;
    QFileDialog *dlg = new QFileDialog(this, tr("Open"), "", tr("All Supported Images (*.obl *.png)"));
    dlg->setAcceptMode(QFileDialog::AcceptOpen);
    dlg->setFileMode(QFileDialog::ExistingFile);
    if (dlg->exec())
    {
        QStringList fileNames = dlg->selectedFiles();
        if (fileNames.count() > 0)
        {
            fileName = fileNames[0];
            if (!extractColors(fileName))
            {
                QMessageBox::warning(this, "Error", "Could not load the file.");
            }
        }
    }
    delete dlg;
}

// Helper function to populate the table
void ColorMapperWindow::populateColors(const QMap<QString, QString> &map)
{
    table->setRowCount(map.size());
    int row = 0;
    for (auto it = map.begin(); it != map.end(); ++it)
    {
        // Original Color item (read-only)
        QTableWidgetItem *originalItem = new QTableWidgetItem();
        originalItem->setText(it.key());

        auto br1 = QBrush(QColor(it.key()));
        originalItem->setBackground(br1);
        originalItem->setFlags(originalItem->flags() & ~Qt::ItemIsEditable);
        table->setItem(row, 0, originalItem);

        // Replacement Color item (editable)
        QTableWidgetItem *replacementItem = new QTableWidgetItem();
        auto br2 = QBrush(QColor(it.value()));
        replacementItem->setBackground(br2);
        replacementItem->setText(it.value());
        table->setItem(row, 1, replacementItem);

        row++;
    }
}

uint32_t ColorMapperWindow::swapRxB(const uint32_t color)
{
    return (color & 0xff) << 16 |
           (color & 0xff00) |
           (color & 0xff0000) >> 16;
}

bool ColorMapperWindow::extractColors(const QString &filename)
{
    QMap<QString, QString> map;
    CFrameSet &fs = *m_frameSet;
    QFileWrap file;
    std::set<uint32_t> colors;
    m_tab2->setFrameSet(nullptr);
    m_frameSet->forget();
    if (file.open(filename.toStdString().c_str(), "rb") && m_frameSet->extract(file))
    {
        for (int i = 0; i < fs.getSize(); ++i)
        {
            CFrame *frame = fs[i];
            for (int y = 0; y < frame->hei(); ++y)
            {
                for (int x = 0; x < frame->len(); ++x)
                {
                    const uint32_t rgba = frame->at(x, y);
                    if (!rgba)
                        continue;
                    if (!colors.contains(rgba))
                    {
                        colors.insert(rgba);
                    }
                }
            }
        }
        for (const auto &color : colors)
        {
            uint32_t rgb = swapRxB(color);
            QString key = QString("#%1").arg(rgb, 6, 16, QChar('0'));
            map[key] = key;
        }
        populateColors(map);
        m_tab2->setFrameSet(m_frameSet);
        return true;
    }
    else
    {
        return false;
    }
}

void ColorMapperWindow::onGreyClicked()
{
    const int pairs = 3;
    for (int i = 0; i < table->rowCount(); ++i)
    {
        QTableWidgetItem *originalItem = table->item(i, 0);
        QTableWidgetItem *replacementItem = table->item(i, 1);
        if (originalItem && replacementItem)
        {
            const QString rgb = originalItem->background().color().name();
            uint32_t total = 0;
            for (int j=0; j < pairs; ++j) {
                const auto color = rgb.mid(j*2 +1, 2);
                total += color.toUInt(nullptr, 16);
            }
            QString hexStr = "#";
            for (int j=0; j < pairs; ++j) {
                hexStr += QString("%1").arg(total/pairs, 2, 16, QChar('0'));
            }
            auto br2 = QBrush(QColor(hexStr));
            replacementItem->setBackground(br2);
            replacementItem->setText(hexStr);
        }
    }
}
