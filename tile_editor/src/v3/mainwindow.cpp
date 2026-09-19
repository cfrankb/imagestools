#include "mainwindow.h"
#include <QOverload>
#include <QStatusBar>
#include <QFileDialog>
#include <QInputDialog>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <QKeySequence>
#include <QMenuBar>

MainWindow::MainWindow()
{
    setWindowTitle("Qt Tile Editor");
    m_scene = new TileScene(this);
    connect(m_scene, &TileScene::requestContextMenu, this, &MainWindow::showContextMenu);
    connect(m_scene, &TileScene::exitSelectModeRequested, this, &MainWindow::exitSelectMode);

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
    createDockWidget();
    statusBar()->showMessage("Ready");

    // Build the File menu (must exist before readSettings() so m_recentMenu is valid)
    auto fileMenu = menuBar()->addMenu("File");
    QAction *openAct = fileMenu->addAction("Open Image");
    connect(openAct, &QAction::triggered, this, &MainWindow::loadImage);
    QAction *saveJsonAct = fileMenu->addAction("Save Tileset JSON");
    connect(saveJsonAct, &QAction::triggered, this, &MainWindow::saveJson);
    QAction *loadJsonAct = fileMenu->addAction("Load Tileset JSON");
    connect(loadJsonAct, &QAction::triggered, this, &MainWindow::loadJson);

    fileMenu->addSeparator();
    m_recentMenu = fileMenu->addMenu(tr("Recent Files"));
    fileMenu->addSeparator();

    QAction *exitAct = new QAction("Exit", this);
    exitAct->setShortcut(QKeySequence::Quit);   // Ctrl+Q
    connect(exitAct, &QAction::triggered, this, &QMainWindow::close);
    fileMenu->addAction(exitAct);

    readSettings();
}

void MainWindow::createToolbar()
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

void MainWindow::createDockWidget()
{
    QDockWidget *propDock = new QDockWidget("Tile Properties", this);
    propDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    QWidget *propWidget = new QWidget;
    QFormLayout *form = new QFormLayout(propWidget);

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
    weightSpin->setRange(0, 9999);

    QPushButton *selectTileButton = new QPushButton("Select Next TileID", this);
    // Only usable when there is already a selection
    selectTileButton->setEnabled(!m_scene->selectedItems().isEmpty());
    connect(selectTileButton, &QPushButton::clicked, this, [this, selectTileButton]() {
        selectTileButton->setCursor(Qt::PointingHandCursor);
        m_scene->setSelectMode(true);
    });

    QPushButton *applyButton = new QPushButton("Apply", this);
    connect(applyButton, &QPushButton::clicked, this, &MainWindow::applyProperties);

    QWidget *granWidget = new QWidget;
    QGridLayout *granGrid = new QGridLayout(granWidget);
    granGrid->setContentsMargins(0, 0, 0, 0);
    granGrid->setHorizontalSpacing(2);
    granGrid->setVerticalSpacing(2);
    QCheckBox *chkUL = new QCheckBox("UL");
    QCheckBox *chkUR = new QCheckBox("UR");
    QCheckBox *chkDL = new QCheckBox("DL");
    QCheckBox *chkDR = new QCheckBox("DR");
    granGrid->addWidget(chkUL, 0, 0);
    granGrid->addWidget(chkUR, 0, 1);
    granGrid->addWidget(chkDL, 1, 0);
    granGrid->addWidget(chkDR, 1, 1);
    connect(chkUL, &QCheckBox::toggled, this, [this](bool on) { onGranularToggle(GranualarUL, on); });
    connect(chkUR, &QCheckBox::toggled, this, [this](bool on) { onGranularToggle(GranualarUR, on); });
    connect(chkDL, &QCheckBox::toggled, this, [this](bool on) { onGranularToggle(GranualarDL, on); });
    connect(chkDR, &QCheckBox::toggled, this, [this](bool on) { onGranularToggle(GranualarDR, on); });

    form->addRow(label);
    form->addRow("Type:", typeBox);
    form->addRow("Tag:", tagEdit);
    form->addRow("Next Tile:", nextSpin);
    form->addRow("", selectTileButton);
    form->addRow("Speed:", speedSpin);
    form->addRow("Weight:", weightSpin);
    form->addRow("Granular:", granWidget);
    form->addRow(applyButton);

    propDock->setWidget(propWidget);
    addDockWidget(Qt::RightDockWidgetArea, propDock);

    m_label = label;
    m_typeBox = typeBox;
    m_tagEdit = tagEdit;
    m_nextSpin = nextSpin;
    m_speedSpin = speedSpin;
    m_weightSpin = weightSpin;
    m_applyButton = applyButton;
    m_selectTileButton = selectTileButton;
    m_granUL = chkUL;
    m_granUR = chkUR;
    m_granDL = chkDL;
    m_granDR = chkDR;

    connect(m_scene, &TileScene::selectionChanged,
            this, &MainWindow::updatePropertiesPanel);
    // Keep the "Select Next TileID" button in sync with the selection state
    connect(m_scene, &TileScene::selectionChanged, this, [this]() {
        m_selectTileButton->setEnabled(!m_scene->selectedItems().isEmpty());
    });
    // Copy the clicked tile's ID into the Next Tile field
    connect(m_scene, &TileScene::nextTileIDSelected, this,
            [this](int index) { m_nextSpin->setValue(index); });

    updatePropertiesPanel();
}

void MainWindow::setTileSize(int s)
        {
            m_tileSize = s;
            if (!m_image.isNull())
                    createTiles();
        }

        void MainWindow::applyProperties()
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


                    void MainWindow::loadImage()
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



                   void MainWindow::showContextMenu(const QPoint &screenPos)
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

                   void MainWindow::setZoomPreset(int percent)
                   {
                       if (percent <= 0)
                           return;
                       m_currentZoom = percent;
                       m_view->resetTransform();
                       double factor = percent / 100.0;
                       m_view->scale(factor, factor);
                       statusBar()->showMessage(QString("Zoom: %1%").arg(percent));
                   }

                   void MainWindow::saveJson()
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
                               t["granular"] = ti->granular();
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

                   void MainWindow::loadJson()
                   {
                       QString fn = QFileDialog::getOpenFileName(this, "Load Tileset JSON", m_jsonFolder, "JSON Files (*.json)");
                       if (fn.isEmpty())
                               return;
                       readJson(fn);
                   }

                   bool MainWindow::readJson(const QString &fn) {
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
                               int granular = o.contains("granular") ? o["granular"].toInt(0) : 0;
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
                                               ti->setGranular(static_cast<uint8_t>(granular));
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



                void MainWindow::createTiles()
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

                void MainWindow::updatePropertiesPanel()
                {
                    QList<QGraphicsItem *> selectedTiles = m_scene->selectedItems();
                    if (selectedTiles.isEmpty()) {
                            m_typeBox->setEnabled(false);
                            m_tagEdit->setEnabled(false);
                            m_nextSpin->setEnabled(false);
                            m_speedSpin->setEnabled(false);
                            m_applyButton->setEnabled(false);
                            m_weightSpin->setEnabled(false);
                            m_granUL->setEnabled(false);
                            m_granUR->setEnabled(false);
                            m_granDL->setEnabled(false);
                            m_granDR->setEnabled(false);
                            m_label->setText("TILE: --");
                            return;
                        }

                        m_typeBox->setEnabled(true);
                    m_tagEdit->setEnabled(true);
                    m_nextSpin->setEnabled(true);
                    m_speedSpin->setEnabled(true);
                    m_applyButton->setEnabled(true);
                    m_weightSpin->setEnabled(true);
                    m_granUL->setEnabled(true);
                    m_granUR->setEnabled(true);
                    m_granDL->setEnabled(true);
                    m_granDR->setEnabled(true);

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

                        // Populate the 2x2 granular quadrant checkboxes (block signals to avoid feedback loop)
                        {
                            QSignalBlocker b1(m_granUL), b2(m_granUR), b3(m_granDL), b4(m_granDR);
                            m_granUL->setChecked(it->granular() & GranualarUL);
                            m_granUR->setChecked(it->granular() & GranualarUR);
                            m_granDL->setChecked(it->granular() & GranualarDL);
                            m_granDR->setChecked(it->granular() & GranualarDR);
                        }
                }

                void MainWindow::updateRecentFilesMenu() {
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

                void MainWindow::addRecentFile(const QString &path) {
                    m_recentFiles.removeAll(path);        // avoid duplicates
                    m_recentFiles.prepend(path);          // newest first
                    while (m_recentFiles.size() > 10)     // limit to 10
                            m_recentFiles.removeLast();

                        updateRecentFilesMenu();
                }

                void MainWindow::readSettings() {
                    QSettings s;
                    m_recentFiles = s.value("recentFiles").toStringList();
                    updateRecentFilesMenu();
                }

                void MainWindow::writeSettings() {
                    QSettings s;
                    s.setValue("recentFiles", m_recentFiles);
                }

                void MainWindow::closeEvent(QCloseEvent *event) {
                    writeSettings();
                    QMainWindow::closeEvent(event);
                }

                void  MainWindow::onGranularToggle(const uint8_t flag, bool on)
                {
                    QList<QGraphicsItem *> selectedTiles = m_scene->selectedItems();
                    if (selectedTiles.isEmpty())
                        return;

                    for (QGraphicsItem *it : selectedTiles)
                    {
                        TileItem *ti = dynamic_cast<TileItem *>(it);
                        if (!ti) continue;
                        uint8_t v = ti->granular();
                        v = on ? (uint8_t)(v | flag) : (uint8_t)(v & ~flag);
                        ti->setGranular(v);
                    }

                    // Repaint highlighted tiles
                    m_scene->update();
                }

void MainWindow::exitSelectMode()
{
    // Exiting select mode: leave select mode and restore the button's default cursor
    m_scene->setSelectMode(false);
    if (m_selectTileButton)
        m_selectTileButton->setCursor(Qt::ArrowCursor);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    Q_UNUSED(event)
}