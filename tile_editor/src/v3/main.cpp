#include <QApplication>
#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsPixmapItem>
#include <QFileDialog>
#include <QMenu>
#include <QInputDialog>
#include <QToolBar>
#include <QAction>
#include <QComboBox>
#include <QLabel>
#include <QStatusBar>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>
#include <QWidget>
#include <QImage>
#include <QPixmap>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QGraphicsSceneMouseEvent>
#include <QSpinBox>
#include <QDockWidget>
#include <QFormLayout>
#include <QPushButton>
#include <QMenuBar>
#include <QPushButton>
#include <QSettings>


// Qt Tile Editor (single-file demo)
// - Open image and split into tiles (default 16x16)
// - Left-click-drag to select multiple tiles (Ctrl to add)
// - Right-click opens context menu for selection: set Type (0..3), set Next tile, set animation speed
// - Selected tiles highlighted; non-background tiles get colored overlay
// - Zoom presets: 100%, 200%, 400%
// - Save/Load tileset metadata (JSON): image path, tile size, per-tile type/next/speed

// Build with Qt6/Qt5 (qmake/CMake/Qt Creator)

enum TileType {
    Background,
    Foreground,
    Solid,
    Deadly,
    Water,
};


class TileItem : public QGraphicsRectItem
{
public:
    enum
    {
        TypeRole = UserType + 1
    };

    TileItem(const QRectF &rect, const QPixmap &pix)
        : QGraphicsRectItem(rect), m_pix(pix), m_type(0), m_next(-1), m_speed(1.0)
    {
        setFlags(ItemIsSelectable | ItemIsFocusable);
        setAcceptHoverEvents(true);
    }

    void setTileType(int t)
    {
        m_type = t;
        update();
    }
    int tileType() const { return m_type; }

    void setNext(int n) { m_next = n; }
    int next() const { return m_next; }

    void setSpeed(double s) { m_speed = s; }
    int speed() const { return m_speed; }

    void setTag(const QString &tag) { m_tag = tag;}
    QString tag() { return m_tag;}

    void setWeight(const int w) {m_weight = w;}
    int weight() {return m_weight;}

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override
    {
        Q_UNUSED(option)
        Q_UNUSED(widget)
        QRectF r = rect();
        if (!m_pix.isNull())
            painter->drawPixmap(r.toRect(), m_pix);
        else
            painter->fillRect(r, Qt::lightGray);

        QColor overlay;
        bool useOverlay = true;
        switch (m_type)
        {
        case TileType::Background:
            useOverlay = false;
            break;
        case TileType::Foreground:
            overlay = QColor(0, 0, 255, 90);
            break;
        case TileType::Solid:
            overlay = QColor(255, 165, 0, 90);
            break;
        case TileType::Deadly:
            overlay = QColor(255, 0, 0, 110);
            break;
        case TileType::Water:
            overlay = QColor(101, 101, 101, 110);
            break;
        default:
            useOverlay = false;
            break;
        }
        if (useOverlay)
            painter->fillRect(r, overlay);

        if (isSelected())
        {
            QPen p(Qt::yellow);
            p.setWidth(2);
            painter->setPen(p);
            painter->drawRect(r.adjusted(1, 1, -1, -1));
        }
        else
        {
            QPen p(Qt::black);
            p.setWidth(0);
            painter->setPen(p);
            painter->drawRect(r);
        }

        if (!m_tag.isEmpty()) {
            painter->setPen(Qt::white);
            painter->setFont(QFont("Arial", 3));
            painter->drawText(r.adjusted(2,2,-2,-2), m_tag);
        }

        if (m_weight != 1) {
            painter->setPen(Qt::yellow);
            painter->setFont(QFont("Arial", 2));
            painter->drawText(r.adjusted(0,0,-2,-2),
                              Qt::AlignRight | Qt::AlignBottom,
                              QString::number(m_weight));
        }
    }

private:
    QPixmap m_pix;
    int m_type;
    int m_next;
    int m_speed;
    QString m_tag;
    int m_weight = 1;
};

class TileScene : public QGraphicsScene
{
    Q_OBJECT
public:
    TileScene(QObject *parent = nullptr) : QGraphicsScene(parent), m_rubber(nullptr) {}

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton)
        {
            m_origin = event->scenePos();
            if (!m_rubber)
            {
                m_rubber = addRect(QRectF(m_origin, QSizeF()), QPen(Qt::DashLine));
                m_rubber->setZValue(10000);
            }
            event->accept();
            emit selectionChanged();    // ← update panel
            return;
        }
        if (event->button() == Qt::RightButton)
        {
            QGraphicsItem *it = itemAt(event->scenePos(), QTransform());
            if (!it)
                clearSelection();
            event->accept();
            emit selectionChanged();    // ← update panel
            return;
        }
        QGraphicsScene::mousePressEvent(event);
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (m_rubber)
        {
            QRectF r(m_origin, event->scenePos());
            r = r.normalized();
            m_rubber->setRect(r);
            event->accept();
            emit selectionChanged();    // ← update panel
            return;
        }
        QGraphicsScene::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton && m_rubber)
        {
            QRectF r = m_rubber->rect();
            removeItem(m_rubber);
            delete m_rubber;
            m_rubber = nullptr;
            QList<QGraphicsItem *> itemsInRect = items(r, Qt::IntersectsItemShape);
            bool add = QApplication::keyboardModifiers() & Qt::ControlModifier;
            if (!add)
                clearSelection();
            for (QGraphicsItem *it : itemsInRect)
                it->setSelected(true);
            event->accept();
            emit selectionChanged();    // ← update panel
            return;
        }
        if (event->button() == Qt::RightButton)
        {
            QPointF pos = event->scenePos();
            QGraphicsItem *clicked = itemAt(pos, QTransform());
            if (clicked && !clicked->isSelected())
            {
                clearSelection();
                clicked->setSelected(true);
            }
            emit requestContextMenu(event->screenPos());
            event->accept();
            emit selectionChanged();    // ← update panel
            return;
        }
        QGraphicsScene::mouseReleaseEvent(event);
    }

signals:
    void requestContextMenu(const QPoint &screenPos);
    void selectionChanged();

private:
    QPointF m_origin;
    QGraphicsRectItem *m_rubber;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow()
    {
        setWindowTitle("Qt Tile Editor");
        m_scene = new TileScene(this);
        connect(m_scene, &TileScene::requestContextMenu, this, &MainWindow::showContextMenu);

        m_view = new QGraphicsView(m_scene);
        m_view->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        m_view->setDragMode(QGraphicsView::NoDrag);

        QWidget *central = new QWidget;
        QVBoxLayout *vlay = new QVBoxLayout;
        vlay->setContentsMargins(0, 0, 0, 0);
        vlay->addWidget(m_view);
        central->setLayout(vlay);
        setCentralWidget(central);

        createToolbar();
        statusBar()->showMessage("Ready");

        QDockWidget *propDock = new QDockWidget("Tile Properties", this);
        propDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

        QWidget *propWidget = new QWidget;
        QFormLayout *form = new QFormLayout(propWidget);

        // Create fields
        QComboBox *typeBox = new QComboBox;
        typeBox->addItems({
               "0 Background",
               "1 Foreground",
               "2 Solid",
               "3 Deadly",
               "4 Water"
        });

        QLabel *label = new QLabel("TILE: --", this);
        QLineEdit *tagEdit = new QLineEdit;

        QSpinBox *nextSpin = new QSpinBox;
        nextSpin->setMinimum(-1);
        nextSpin->setMaximum(255);
        QSpinBox *speedSpin = new QSpinBox;
        QSpinBox *weightSpin = new QSpinBox;
        weightSpin->setRange(0, 9999);  // or any range you prefer

        QPushButton *applyButton = new QPushButton("Apply", this);
        // Connect the button's clicked signal to our custom slot
        connect(applyButton, &QPushButton::clicked, this, &MainWindow::applyProperties);

        form->addRow(label);
        form->addRow("Type:", typeBox);
        form->addRow("Tag:", tagEdit);
        form->addRow("Next Tile:", nextSpin);
        form->addRow("Speed:", speedSpin);
        form->addRow("Weight:", weightSpin);
        form->addRow(applyButton);

        propDock->setWidget(propWidget);
        addDockWidget(Qt::RightDockWidgetArea, propDock);

        // store pointers as members
        m_label = label;
        m_typeBox = typeBox;
        m_tagEdit = tagEdit;
        m_nextSpin = nextSpin;
        m_speedSpin = speedSpin;
        m_weightSpin = weightSpin;
        m_applyButton = applyButton;

        connect(m_scene, &TileScene::selectionChanged,
                this, &MainWindow::updatePropertiesPanel);

        updatePropertiesPanel();

        // Add to the File menu
        auto fileMenu = menuBar()->addMenu("File");
        QAction *openAct = fileMenu->addAction("Open Image");
        connect(openAct, &QAction::triggered, this, &MainWindow::loadImage);
        QAction *saveJsonAct = fileMenu->addAction("Save Tileset JSON");
        connect(saveJsonAct, &QAction::triggered, this, &MainWindow::saveJson);
        QAction *loadJsonAct = fileMenu->addAction("Load Tileset JSON");
        connect(loadJsonAct, &QAction::triggered, this, &MainWindow::loadJson);

        // Separator
        fileMenu->addSeparator();
        // Recent files submenu
        m_recentMenu = fileMenu->addMenu(tr("Recent Files"));

        fileMenu->addSeparator();

        QAction *exitAct = new QAction("Exit", this);
        exitAct->setShortcut(QKeySequence::Quit);   // Ctrl+Q
        connect(exitAct, &QAction::triggered, this, &QMainWindow::close);
        fileMenu->addAction(exitAct);

        // Add to the View menu
        auto viewMenu = menuBar()->addMenu("View");
        viewMenu->addAction(propDock->toggleViewAction());

        readSettings(); // restore recent files, window geometry, etc.
    }


private slots:

    void applyProperties()
    {
        QList<QGraphicsItem *> selectedTiles = m_scene->selectedItems();
        if (selectedTiles.isEmpty()) return;

        for (QGraphicsItem *it : selectedTiles)
        {
            TileItem *ti = dynamic_cast<TileItem *>(it);
            if (!ti) continue;
            ti->setTileType(m_typeBox->currentIndex());
            ti->setTag(m_tagEdit->text());
            ti->setNext(m_nextSpin->value());
            ti->setSpeed(m_speedSpin->value());
            ti->setWeight(m_weightSpin->value());
        }

        // Repaint highlighted tiles
        m_scene->update();
    }


    void loadImage()
    {
        QString fn = QFileDialog::getOpenFileName(this, "Open Image", m_imageFolder, "Images (*.png *.jpg *.bmp *.gif)");
        if (fn.isEmpty())
            return;
        if (!m_image.load(fn))
        {
            statusBar()->showMessage("Failed to load image");
            return;
        }
        // save imagefile
        m_imageFile = fn;
        // save imagefolder
        m_imageFolder = QFileInfo(fn).absoluteFilePath();
        createTiles();
        statusBar()->showMessage(QString("Loaded %1 — %2x%3").arg(fn).arg(m_image.width()).arg(m_image.height()));
    }

    void setTileSize(int s)
    {
        m_tileSize = s;
        if (!m_image.isNull())
            createTiles();
    }

    void showContextMenu(const QPoint &screenPos)
    {
        QList<QGraphicsItem *> sel = m_scene->selectedItems();
        if (sel.isEmpty())
            return;

        QMenu menu;
        QMenu *typeMenu = menu.addMenu("Set Type");
        QAction *a0 = typeMenu->addAction("0 - Background");
        QAction *a1 = typeMenu->addAction("1 - Foreground (blue)");
        QAction *a2 = typeMenu->addAction("2 - Solid (orange)");
        QAction *a3 = typeMenu->addAction("3 - Deadly (red)");
        QAction *a4 = typeMenu->addAction("4 - Water (gray)");

        QAction *setNext = menu.addAction("Set Next Tile Index...");
        QAction *setSpeed = menu.addAction("Set Animation Speed...");
        QAction *setTagAct = menu.addAction("Set Tag");
        QAction *setWeightAct = menu.addAction("Set Weight");

        QAction *chosen = menu.exec(screenPos);
        if (!chosen)
            return;

        if (chosen == a0 || chosen == a1 || chosen == a2 || chosen == a3 || chosen == a4)
        {
            int t = TileType::Background;
            if (chosen == a1)
                t = TileType::Foreground;
            else if (chosen == a2)
                t = TileType::Solid;
            else if (chosen == a3)
                t = TileType::Deadly;
            else if (chosen == a4)
                t = TileType::Water;
            for (QGraphicsItem *it : sel)
            {
                TileItem *ti = dynamic_cast<TileItem *>(it);
                if (ti)
                    ti->setTileType(t);
            }
            m_scene->update();
        }
        else if (chosen == setNext)
        {
            bool ok;
            int val = QInputDialog::getInt(this, "Set Next Tile", "Next tile index (-1 = none):", -1, -1, 1000000, 1, &ok);
            if (!ok)
                return;
            for (QGraphicsItem *it : sel)
            {
                TileItem *ti = dynamic_cast<TileItem *>(it);
                if (ti)
                    ti->setNext(val);
            }
        }
        else if (chosen == setSpeed)
        {
            bool ok;
            int val = QInputDialog::getInt(this, "Set Speed", "Animation speed:", 0, 0, 1000, 2, &ok);
            if (!ok)
                return;
            for (QGraphicsItem *it : sel)
            {
                TileItem *ti = dynamic_cast<TileItem *>(it);
                if (ti)
                    ti->setSpeed(val);
            }
        } else if (chosen == setTagAct) {
            bool ok = false;
            QString tag = QInputDialog::getText(
                this, "Set Tag",
                "Enter tag:", QLineEdit::Normal,
                "", &ok);

            if (ok) {
                for (QGraphicsItem *it : sel) {
                    TileItem *ti = dynamic_cast<TileItem *>(it);
                    if (ti)
                        ti->setTag(tag);
                }
                //viewport()->update();   // redraw
            }
        } else if (chosen == setWeightAct)
        {
            bool ok;
            int val = QInputDialog::getInt(this, "Set Weight", "Weight:", 0, 0, 1000, 2, &ok);
            if (!ok)
                return;
            for (QGraphicsItem *it : sel)
            {
                TileItem *ti = dynamic_cast<TileItem *>(it);
                if (ti)
                    ti->setWeight(val);
            }
        }
    }

    void setZoomPreset(int percent)
    {
        if (percent <= 0)
            return;
        m_currentZoom = percent;
        m_view->resetTransform();
        double factor = percent / 100.0;
        m_view->scale(factor, factor);
        statusBar()->showMessage(QString("Zoom: %1%").arg(percent));
    }

    void saveJson()
    {
        if (m_imageFile.isEmpty())
        {
            statusBar()->showMessage("No image loaded to save.");
            return;
        }
        QString fn = QFileDialog::getSaveFileName(this, "Save Tileset JSON", m_jsonFolder, "JSON Files (*.json)");
        if (fn.isEmpty())
            return;
        if (!fn.endsWith(".json"))
            fn += ".json";

        QJsonObject root;
        root["imagePath"] = m_imageFile;
        root["tileSize"] = m_tileSize;
        QRectF sceneRect = m_scene->sceneRect();
        root["width"] = (int)sceneRect.width();
        root["height"] = (int)sceneRect.height();

        QJsonArray tilesArray;
        QList<QGraphicsItem *> allItems = m_scene->items(Qt::AscendingOrder);
        // items() returns in stacking order — filter TileItem
        // We'll collect by their stored index
        QMap<int, QJsonObject> byIndex;
        for (QGraphicsItem *it : allItems)
        {
            TileItem *ti = dynamic_cast<TileItem *>(it);
            if (!ti)
                continue;
            int idx = ti->data(TileItem::TypeRole).toInt();
            QJsonObject t;
            t["index"] = idx;
            t["type"] = ti->tileType();
            t["next"] = ti->next();
            t["speed"] = ti->speed();
            t["tag"] = ti->tag();
            t["w"] = ti->weight();   // ← NEW
            byIndex[idx] = t;
        }
        // write array in index order
        QList<int> keys = byIndex.keys();
        std::sort(keys.begin(), keys.end());
        for (int k : keys)
            tilesArray.append(byIndex[k]);

        root["tiles"] = tilesArray;

        QJsonDocument doc(root);
        QFile f(fn);
        if (!f.open(QIODevice::WriteOnly))
        {
            statusBar()->showMessage("Failed to open file for writing.");
            return;
        }
        f.write(doc.toJson(QJsonDocument::Indented));
        f.close();
        statusBar()->showMessage(QString("Saved tileset to %1").arg(fn));

        // save json folder
        m_jsonFolder = QFileInfo(fn).absoluteFilePath();

        // add filepath
        addRecentFile(fn);
    }

    void loadJson()
    {
        QString fn = QFileDialog::getOpenFileName(this, "Load Tileset JSON", m_jsonFolder, "JSON Files (*.json)");
        if (fn.isEmpty())
            return;
        readJson(fn);
    }

    bool readJson(const QString &fn) {
        QFile f(fn);
        if (!f.open(QIODevice::ReadOnly))
        {
            statusBar()->showMessage("Failed to open JSON file");
            return false;
        }
        QByteArray data = f.readAll();
        f.close();
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(data, &err);
        if (err.error != QJsonParseError::NoError)
        {
            statusBar()->showMessage("JSON parse error");
            return false;
        }
        QJsonObject root = doc.object();
        QString imagePath = root.value("imagePath").toString();
        int tileSize = root.value("tileSize").toInt(16);
        if (imagePath.isEmpty())
        {
            statusBar()->showMessage("JSON missing imagePath");
            return false;
        }
        if (!m_image.load(imagePath))
        {
            statusBar()->showMessage("Failed to load image from JSON: " + imagePath);
            return false;
        }
        m_imageFile = imagePath;
        m_tileSize = tileSize;
        // save json folder
        m_jsonFolder = QFileInfo(fn).absoluteFilePath();

        // create tiles then apply properties
        createTiles();

        QJsonArray tilesArray = root.value("tiles").toArray();
        for (const QJsonValue &v : tilesArray)
        {
            if (!v.isObject())
                continue;
            QJsonObject o = v.toObject();
            int idx = o.value("index").toInt(-1);
            if (idx < 0)
                continue;
            int type = o.value("type").toInt(0);
            int next = o.value("next").toInt(-1);
            double speed = o.value("speed").toDouble(1.0);
            QString tag = o.value("tag").toString();
            int weight = o.contains("w") ? o["w"].toInt() : 1;
            // find item by index
            QList<QGraphicsItem *> allItems = m_scene->items(Qt::AscendingOrder);
            for (QGraphicsItem *it : allItems)
            {
                TileItem *ti = dynamic_cast<TileItem *>(it);
                if (!ti)
                    continue;
                if (ti->data(TileItem::TypeRole).toInt() == idx)
                {
                    ti->setTileType(type);
                    ti->setNext(next);
                    ti->setSpeed(speed);
                    ti->setTag(tag);
                    ti->setWeight(weight);
                    break;
                }
            }
        }

        // update UI widgets that reflect tile size and zoom
        // try to find spinbox and set its value (simple approach)
        for (QObject *child : children())
        {
            QSpinBox *sp = qobject_cast<QSpinBox *>(child);
            if (sp)
                sp->setValue(m_tileSize);
        }
        statusBar()->showMessage(QString("Loaded tileset from %1").arg(fn));
        updatePropertiesPanel();

        // add filepath
        addRecentFile(fn);
        return true;
    }

private:
    void createToolbar()
    {
        QToolBar *tb = addToolBar("Main");
        QLabel *lbl = new QLabel("Tile size:");
        tb->addWidget(lbl);
        QSpinBox *spin = new QSpinBox;
        spin->setRange(4, 256);
        spin->setValue(16);
        connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::setTileSize);
        tb->addWidget(spin);

        tb->addSeparator();
        QLabel *zlbl = new QLabel("Zoom:");
        tb->addWidget(zlbl);
        QComboBox *zoomBox = new QComboBox;
        zoomBox->addItem("100%", 100);
        zoomBox->addItem("200%", 200);
        zoomBox->addItem("400%", 400);
        zoomBox->setCurrentIndex(0);
        connect(zoomBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, zoomBox](int)
                {
            int pct = zoomBox->currentData().toInt();
            setZoomPreset(pct); });
        tb->addWidget(zoomBox);

        tb->addSeparator();
        QAction *fit = tb->addAction("Fit View");
        connect(fit, &QAction::triggered, this, [this]()
                { m_view->fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio); m_currentZoom = 0; statusBar()->showMessage("Fit to view"); });

        QAction *clear = tb->addAction("Clear Selection");
        connect(clear, &QAction::triggered, m_scene, &QGraphicsScene::clearSelection);
    }

    void createTiles()
    {
        m_scene->clear();
        if (m_image.isNull())
            return;
        int w = m_image.width();
        int h = m_image.height();
        int ts = m_tileSize;
        int cols = (w + ts - 1) / ts;
        int rows = (h + ts - 1) / ts;

        int index = 0;
        for (int y = 0; y < rows; ++y)
        {
            for (int x = 0; x < cols; ++x)
            {
                int sx = x * ts;
                int sy = y * ts;
                int sw = qMin(ts, w - sx);
                int sh = qMin(ts, h - sy);
                QImage sub = m_image.copy(sx, sy, sw, sh);
                QPixmap pm = QPixmap::fromImage(sub);
                QRectF r(x * ts, y * ts, ts, ts);
                TileItem *ti = new TileItem(r, pm);
                ti->setData(TileItem::TypeRole, index);
                m_scene->addItem(ti);
                ++index;
            }
        }

        m_scene->setSceneRect(0, 0, cols * ts, rows * ts);
        m_view->setScene(m_scene);
        m_view->resetTransform();
        // apply current zoom preset if >0
        if (m_currentZoom > 0)
        {
            double factor = m_currentZoom / 100.0;
            m_view->scale(factor, factor);
        }
        m_view->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    }

    void updatePropertiesPanel()
    {
        QList<QGraphicsItem *> selectedTiles = m_scene->selectedItems();
        if (selectedTiles.isEmpty()) {
            m_typeBox->setEnabled(false);
            m_tagEdit->setEnabled(false);
            m_nextSpin->setEnabled(false);
            m_speedSpin->setEnabled(false);
            m_applyButton->setEnabled(false);
            m_weightSpin->setEnabled(false);
            m_label->setText("TILE: --");
            return;
        }

        m_typeBox->setEnabled(true);
        m_tagEdit->setEnabled(true);
        m_nextSpin->setEnabled(true);
        m_speedSpin->setEnabled(true);
        m_applyButton->setEnabled(true);
        m_weightSpin->setEnabled(true);

        // Show first selected tile's values
        const auto & it = dynamic_cast<TileItem *>(selectedTiles.first());

        int idx = it->data(TileItem::TypeRole).toInt();
        if (selectedTiles.size() == 1) {
            m_label->setText(QString("TILE: %1").arg(idx));
        }
        else {
            size_t count = selectedTiles.size() - 1;
            m_label->setText(QString("TILE: %1 [+ %2 other%3 selected]").arg(idx).arg(count).arg(count == 1? "": "s"));
        }

        m_typeBox->setCurrentIndex(it->tileType());
        m_tagEdit->setText(it->tag());
        m_nextSpin->setValue(it->next());
        m_speedSpin->setValue(it->speed());
        m_weightSpin->setValue(it->weight());
    }

    void updateRecentFilesMenu() {
        m_recentMenu->clear();

        for (const QString &path : m_recentFiles) {
            QFileInfo fileInfo(path);
            QString filename(fileInfo.fileName());
            QAction *act = m_recentMenu->addAction(filename);
            connect(act, &QAction::triggered, this, [this, path]() {
                readJson(path);
            });
            act->setStatusTip(path);
        }

        if (m_recentFiles.isEmpty()) {
            m_recentMenu->addAction(tr("(No recent files)"))->setEnabled(false);
        }
    }

    void addRecentFile(const QString &path) {
        m_recentFiles.removeAll(path);        // avoid duplicates
        m_recentFiles.prepend(path);          // newest first
        while (m_recentFiles.size() > 10)     // limit to 10
            m_recentFiles.removeLast();

        updateRecentFilesMenu();
    }

    void readSettings() {
        QSettings s;
        m_recentFiles = s.value("recentFiles").toStringList();
        updateRecentFilesMenu();
    }

    void writeSettings() {
        QSettings s;
        s.setValue("recentFiles", m_recentFiles);
    }

    void closeEvent(QCloseEvent *event) {
        writeSettings();
        QMainWindow::closeEvent(event);
    }

    QStringList m_recentFiles;
    QMenu *m_recentMenu;

    TileScene *m_scene;
    QGraphicsView *m_view;
    QImage m_image;
    QString m_imageFile;
    int m_tileSize = 16;
    int m_currentZoom = 100; // default 100%

    QLabel *m_label;
    QComboBox *m_typeBox;
    QLineEdit *m_tagEdit;
    QSpinBox *m_nextSpin;
    QSpinBox *m_speedSpin;
    QSpinBox *m_weightSpin;
    QPushButton *m_applyButton;

    QString m_imageFolder;
    QString m_jsonFolder;
};

#include "main.moc"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    MainWindow w;
    w.resize(900, 700);
    w.show();
    return app.exec();
}
