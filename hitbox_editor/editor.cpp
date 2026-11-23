#include "editor.h"

#include <QDebug>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QMouseEvent>
#include <QCheckBox>
#include <QComboBox>
#include <QIntValidator>
#include <QPushButton>
#include <QStatusBar>
#include <QGraphicsPixmapItem>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>

static const int kMaxCustomGrid = 4;   // remembered typed sizes per axis

// Grid sizes must be positive multiples of 8 - the minor grid and hitbox snap are both 8px.
// Ties round up; anything under 8 (including a blank field) becomes 8.
static int roundGrid(int v) { return qMax(8, (v + 4) / 8 * 8); }

// Drops anything unusable from a remembered list (legacy settings, hand-edited files).
static QStringList validSizes(const QStringList &in)
{
    QStringList out;
    for (const QString &v : in)
        if (v.toInt() > 0 && v.toInt() % 8 == 0 && !out.contains(v)) out << v;
    while (out.size() > kMaxCustomGrid) out.removeLast();
    return out;
}

Editor::Editor(QWidget *p):QMainWindow(p){
    lastOpenedFolder = QSettings().value("lastFolder", QDir::homePath()).toString();

    scene=new GridScene(this);
    view=new QGraphicsView(scene,this);
    view->setRenderHints(QPainter::Antialiasing|QPainter::SmoothPixmapTransform);
    view->setDragMode(QGraphicsView::RubberBandDrag);

    QWidget *central=new QWidget;
    QVBoxLayout *layout=new QVBoxLayout(central);
    QHBoxLayout *tools=new QHBoxLayout;

    QPushButton *openBtn=new QPushButton("Open Image");
    QPushButton *saveBtn=new QPushButton("Save");
    QPushButton *loadBtn=new QPushButton("Load");
    QPushButton *zoomInBtn=new QPushButton("Zoom +");
    QPushButton *zoomOutBtn=new QPushButton("Zoom -");
    QLabel *typeLbl=new QLabel("Type:");
    typeCombo=new QComboBox; typeCombo->addItems({"Main","Attack","Special1","Special2"});

    tools->addWidget(openBtn); tools->addWidget(saveBtn); tools->addWidget(loadBtn);
    tools->addSpacing(20);
    tools->addWidget(zoomInBtn); tools->addWidget(zoomOutBtn);
    tools->addSpacing(20);
    tools->addWidget(typeLbl); tools->addWidget(typeCombo);
    tools->addStretch();

    hbList=new QListWidget;
    hbList->setMinimumHeight(100);
    QLabel *hbLabel = new QLabel("Hitboxes:");

    layout->addLayout(tools);
    layout->addWidget(view,1);
    layout->addWidget(hbLabel);
    layout->addWidget(hbList);

    setCentralWidget(central);

    status=new QStatusBar(this);
    setStatusBar(status);
    status->showMessage("Ready");

    connect(openBtn,&QPushButton::clicked,this,&Editor::onOpen);
    connect(saveBtn,&QPushButton::clicked,this,&Editor::onSave);
    connect(loadBtn,&QPushButton::clicked,this,&Editor::onLoad);
    connect(zoomInBtn,&QPushButton::clicked,this,[this]{view->scale(1.25,1.25); updateStatus();});
    connect(zoomOutBtn,&QPushButton::clicked,this,[this]{view->scale(0.8,0.8); updateStatus();});
    connect(hbList,&QListWidget::currentRowChanged,this,&Editor::onHitboxSelected);

    view->viewport()->installEventFilter(this);
    view->setFocusPolicy(Qt::StrongFocus);

    view->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(view, &QWidget::customContextMenuRequested,
            this, &Editor::onSceneContextMenu);

    // show/hide hitbox list
    QCheckBox *showListCheck = new QCheckBox("Show Hitbox List");
    showListCheck->setChecked(true); // visible by default
    tools->addWidget(showListCheck);
    connect(showListCheck, &QCheckBox::toggled, hbList, &QListWidget::setVisible);
    connect(showListCheck, &QCheckBox::toggled, hbLabel, &QListWidget::setVisible);


    QPushButton *importButton = new QPushButton("Import");
    tools->addWidget(importButton);

    connect(importButton, &QPushButton::clicked, this, &Editor::onImport);


    // Major grid size selector: presets from the list, or type any size (e.g. 104 x 96).
    // The last 4 custom sizes per axis are kept in the dropdown for reuse.
    customGridW = validSizes(QSettings().value("customGridW").toStringList());
    customGridH = validSizes(QSettings().value("customGridH").toStringList());

    gridSizeComboH = new QComboBox();
    gridSizeComboH->addItems(customGridW + sizes);
    gridSizeComboH->setCurrentText("64");
    gridSizeComboH->setEditable(true);
    gridSizeComboH->setInsertPolicy(QComboBox::NoInsert);
    gridSizeComboH->setValidator(new QIntValidator(1, 4096, gridSizeComboH));
    tools->addWidget(new QLabel("Grid:"));
    tools->addWidget(gridSizeComboH);

    gridSizeComboV = new QComboBox();
    gridSizeComboV->addItems(customGridH + sizes);
    gridSizeComboV->setCurrentText("64");
    gridSizeComboV->setEditable(true);
    gridSizeComboV->setInsertPolicy(QComboBox::NoInsert);
    gridSizeComboV->setValidator(new QIntValidator(1, 4096, gridSizeComboV));
    tools->addWidget(gridSizeComboV);


    // Background color selector
    QComboBox *bgColorCombo = new QComboBox();
    bgColorCombo->addItems({"White", "Dark Gray", "Light Gray",  "Dark Cyan"});
    tools->addWidget(new QLabel("Background:"));
    tools->addWidget(bgColorCombo);

    connect(gridSizeComboH, &QComboBox::currentTextChanged, this, &Editor::applyGridSize);
    connect(gridSizeComboV, &QComboBox::currentTextChanged, this, &Editor::applyGridSize);
    // Only commit a typed size when editing ends, so half-typed "1"/"10" are neither applied nor remembered.
    connect(gridSizeComboH->lineEdit(), &QLineEdit::editingFinished, this, &Editor::commitGridSizes);
    connect(gridSizeComboV->lineEdit(), &QLineEdit::editingFinished, this, &Editor::commitGridSizes);


    connect(bgColorCombo, &QComboBox::currentTextChanged, this, [this](const QString &text) {
        if (!scene) return;

        QColor c = QColor(40, 40, 40); // default dark gray
        if (text == "Dark Gray") c = QColor(40, 40, 40);
        else if (text == "Light Gray") c = QColor(200, 200, 200);
        else if (text == "White") c = QColor(255, 255, 255);
        else if (text == "Dark Cyan") c = QColor(0,139,139);

        scene->setBackgroundColor(c);
    });

}

void Editor::resizeEvent(QResizeEvent *e) {
    QMainWindow::resizeEvent(e);
    if(currentPixmapItem) view->fitInView(currentPixmapItem,Qt::KeepAspectRatio);
}

bool Editor::eventFilter(QObject *obj,QEvent *ev) {
    if(obj==view->viewport()){
        if(!currentPixmapItem) return false;
        switch(ev->type()){
        case QEvent::MouseButtonPress:{
            clearSelection();

            QMouseEvent *me=static_cast<QMouseEvent*>(ev);
            if(me->button()==Qt::LeftButton){
                startPos=view->mapToScene(me->pos());
                QRectF r(startPos,startPos);
                HitboxType t=(HitboxType)typeCombo->currentIndex();
                creatingRect=new HitboxItem(r,t); scene->addItem(creatingRect); creatingRect->setSelected(true);
                return true;
            }
            break;
        }
        case QEvent::MouseMove:{
            QMouseEvent *me=static_cast<QMouseEvent*>(ev);
            QRectF r(startPos,view->mapToScene(me->pos()));
            currPos = view->mapToScene(me->pos()).toPoint();
            if(creatingRect){
                creatingRect->setRect(r.normalized()); return true;
            }

            updateStatus();
            break;
        }
        case QEvent::MouseButtonRelease:{
            if(creatingRect){
                pushUndoState();
                HitboxDef def;
                def.rect=creatingRect->rect().toRect();
                def.type=creatingRect->boxType;
                QRect snapRect = snapToGrid(def.rect);
                if (snapRect != def.rect) {
                    creatingRect->setRect(snapRect.toRectF());
                    def.rect = snapRect;
                }
                if (def.rect.width() && def.rect.height()) {
                    // don't add empty hitbox
                    hitboxes.append(def);
                    updateHitboxList();
                    pushRedoClear();
                    // select last item in list
                    hbList->setCurrentRow(hbList->count() - 1);
                } else {
                    // Remove from scene
                    scene->removeItem(creatingRect);

                    // Delete the object
                    delete creatingRect;
                }
                creatingRect=nullptr;
                return true;
            }
            break;
        }
        default: break;
        }
    }
    return QMainWindow::eventFilter(obj,ev);
}

void Editor::keyPressEvent(QKeyEvent *e) {
    if(e->key()==Qt::Key_Delete){ deleteSelectedHitbox(); return; }
    if(e->matches(QKeySequence::Undo)){ undo(); return; }
    if(e->matches(QKeySequence::Redo)){ redo(); return; }
    if(auto hb=selectedHitboxItem()){
        QPointF move(0,0);
        if(e->key()==Qt::Key_Left) move.setX(-GRID);
        if(e->key()==Qt::Key_Right) move.setX(GRID);
        if(e->key()==Qt::Key_Up) move.setY(-GRID);
        if(e->key()==Qt::Key_Down) move.setY(GRID);
        if(move!=QPointF(0,0)){
            pushUndoState();
            hb->moveBy(move.x(),move.y());
            pushRedoClear();
            updateHitboxesFromScene();
            updateHitboxList();
        }
    }
}

void Editor::onOpen(){
    QString filePath=QFileDialog::getOpenFileName(this,"Open Image", lastOpenedFolder,"Images (*.png *.jpg *.bmp)");
    if(filePath.isEmpty())return;
    scene->clear(); hitboxes.clear(); currentPixmapItem=nullptr;
    currentPixmap=QPixmap(filePath);
    if(currentPixmap.isNull())return;
    imagePath = filePath;
    currentPixmapItem=scene->addPixmap(currentPixmap);
    view->fitInView(currentPixmapItem,Qt::KeepAspectRatio);
    updateHitboxList();
    pushUndoState();
    pushRedoClear();
    setWindowTitle(QFileInfo(imagePath).baseName());
    rememberFolder(QFileInfo(imagePath).dir().path());

    QFileInfo info(filePath);
    QString jsonPath = info.path() + "/" + info.completeBaseName() + ".json";
    QFile file(jsonPath);

    if (file.exists()) {
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            file.close();

            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isObject()) {
                QJsonObject root = doc.object();
                reloadData(root, jsonPath);
            }
        } else {
            QMessageBox::warning(this, "Warning", "Cannot open " + jsonPath + " for reading.");
        }
    }
    checkGridFits();   // no sibling json: the grid is still whatever it was before
}

void Editor::onSave(){
    QString jsonPath = "hitboxes.json";
    if (!imagePath.isEmpty()) {
        QFileInfo info(imagePath);
        jsonPath = info.path() + "/" + info.baseName() + ".json";
    }

    QString f=QFileDialog::getSaveFileName(this,"Save",jsonPath,"JSON (*.json)");
    if(f.isEmpty())return;
    if (!f.endsWith(".json"))
        f += ".json";
    QJsonArray arr;
    for(const HitboxDef &hb:hitboxes){
        QJsonObject o; o["x"]=hb.rect.x(); o["y"]=hb.rect.y(); o["w"]=hb.rect.width(); o["h"]=hb.rect.height(); o["type"]=(int)hb.type; arr.append(o);
    }
    QJsonObject root{{"imagePath",imagePath},{"hitboxes",arr}};
    commitGridSizes();   // guarantees both sizes below are positive multiples of 8
    int frameWidth = gridSizeComboH->currentText().toInt();
    int frameHeight = gridSizeComboV->currentText().toInt();
    checkGridFits();
    root["frame"] = QJsonObject{
        {"width", frameWidth},
        {"height", frameHeight },
        {"cols", currentPixmap.width() / frameWidth}
    };

    QFile file(f); if(file.open(QIODevice::WriteOnly)) file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    rememberFolder(QFileInfo(f).dir().path());
}

void Editor::onLoad(){
    QString f=QFileDialog::getOpenFileName(this,"Load", lastOpenedFolder,"JSON (*.json)");
    if(f.isEmpty())
        return;
    QFile file(f);
    if(!file.open(QIODevice::ReadOnly))return;
    QJsonDocument doc=QJsonDocument::fromJson(file.readAll());
    QJsonObject root=doc.object();
    QString path=root["imagePath"].toString();
    QPixmap pix(path);
    if (pix.isNull()) {   // stale/foreign path in the JSON: ask for the real file
        QDir dir = path.isEmpty() ? QDir(lastOpenedFolder) : QFileInfo(path).absoluteDir();
        QString picked = QFileDialog::getOpenFileName(this,
            "Find image for " + QFileInfo(f).fileName(),
            dir.exists() ? dir.path() : lastOpenedFolder,
            "Images (*.png *.jpg *.bmp)");
        if (picked.isEmpty()) return;                 // cancelled: keep the current image
        pix = QPixmap(picked);
        if (pix.isNull()) { QMessageBox::warning(this, "Load Failed", "Cannot load image:\n" + picked); return; }
        path = picked;
    }
    currentPixmap = pix;
    imagePath = path;
    scene->clear();
    hitboxes.clear();
    currentPixmapItem=scene->addPixmap(currentPixmap);

    reloadData(root, f);

    view->fitInView(currentPixmapItem,Qt::KeepAspectRatio);
    pushUndoState();
    pushRedoClear();
    rememberFolder(QFileInfo(f).dir().path());
}

void Editor::onImport()
{
    if (!currentPixmapItem || currentPixmapItem->pixmap().isNull()) {
        QMessageBox::warning(this, "No Image", "You must load an image before importing hitboxes.");
        return;
    }

    QString importImagePath = QFileDialog::getOpenFileName(
        this,
        "Select Image to Import Hitboxes From",
        lastOpenedFolder,
        "Images (*.png *.jpg *.bmp)"
        );

    if (importImagePath.isEmpty())
        return;

    rememberFolder(QFileInfo(importImagePath).dir().path());
    QFileInfo info(importImagePath);
    QString jsonPath = info.path() + "/" + info.completeBaseName() + ".json";

    QFile file(jsonPath);
    if (!file.exists()) {
        QMessageBox::warning(this, "Import Error", "No hitbox JSON found for:\n" + jsonPath);
        return;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Import Error", "Cannot open " + jsonPath);
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        QMessageBox::warning(this, "Import Error", "Invalid JSON format in:\n" + jsonPath);
        return;
    }

    QJsonObject root = doc.object();
    if (!root.contains("hitboxes") || !root["hitboxes"].isArray()) {
        QMessageBox::warning(this, "Import Error", "JSON file has no hitbox data.");
        return;
    }

    hitboxes.clear();
    scene->clear();
    currentPixmapItem=scene->addPixmap(currentPixmap);
    view->fitInView(currentPixmapItem,Qt::KeepAspectRatio);

    reloadData(root, jsonPath);
    pushUndoState();
    pushRedoClear();
    statusBar()->showMessage("Imported hitboxes from " + info.fileName(), 3000);
}

void Editor::onHitboxSelected(int r){
    QList<QGraphicsItem*> all=scene->items();
    for(auto *it:all){
        if(auto hb=dynamic_cast<HitboxItem*>(it))
            hb->setSelected(false);
    }
    if(r>=0 && r<hitboxes.size()){
        HitboxDef &hb=hitboxes[r];
        for(auto *it:all){
            if(auto hbi=dynamic_cast<HitboxItem*>(it)){
                if(hbi->rect().toRect()==hb.rect){
                   hbi->setSelected(true); view->centerOn(hbi); break;
                }
            }
        }
    }
    updateStatus();
}

void Editor::applyGridSize()
{
    if (!scene) return;
    scene->setMajorGrid(gridSizeComboH->currentText().toInt(), gridSizeComboV->currentText().toInt());
    checkGridFits();
}

void Editor::commitGridSizes()
{
    commitGridSize(gridSizeComboH, customGridW, "customGridW");
    commitGridSize(gridSizeComboV, customGridH, "customGridH");
    applyGridSize();
}

void Editor::commitGridSize(QComboBox *box, QStringList &list, const QString &key)
{
    const int raw = box->currentText().toInt();
    const int v = roundGrid(raw);
    if (v != raw) {
        box->setCurrentText(QString::number(v));   // fires currentTextChanged -> applyGridSize
        status->showMessage(QString("Grid %1 is not a positive multiple of 8 - using %2").arg(raw).arg(v), 5000);
    }
    rememberGridSize(box, list, key);
}

// Keeps the last kMaxCustomGrid typed sizes per axis, most recent first, in the dropdown and QSettings.
void Editor::rememberGridSize(QComboBox *box, QStringList &list, const QString &key)
{
    const QString value = box->currentText();
    if (value.isEmpty() || value.toInt() % 8) return;   // only valid sizes are remembered
    if (sizes.contains(value) || list.contains(value)) return;
    list.prepend(value);
    while (list.size() > kMaxCustomGrid) list.removeLast();
    QSettings().setValue(key, list);
    box->blockSignals(true);
    box->clear();
    box->addItems(list + sizes);
    box->setCurrentText(value);
    box->blockSignals(false);
}

void Editor::checkGridFits()
{
    const int w = gridSizeComboH->currentText().toInt();
    const int h = gridSizeComboV->currentText().toInt();
    if (currentPixmap.isNull() || w <= 0 || h <= 0) return;
    if (currentPixmap.width() % w || currentPixmap.height() % h)
        status->showMessage(QString("Warning: image %1x%2 is not divisible by grid %3x%4 - frame columns will be wrong")
            .arg(currentPixmap.width()).arg(currentPixmap.height()).arg(w).arg(h));
}

void Editor::rememberFolder(const QString &dir)
{
    lastOpenedFolder = dir;
    QSettings().setValue("lastFolder", dir);
}

void Editor::reloadData(QJsonObject& root, QString &jsonPath)
{
    if (root.contains("hitboxes") && root["hitboxes"].isArray()) {
        hitboxes.clear();
        QJsonArray arr = root["hitboxes"].toArray();
        for (const QJsonValue &v : arr) {
            QJsonObject o = v.toObject();
            HitboxDef hb;
            hb.rect = QRect(o["x"].toInt(), o["y"].toInt(), o["w"].toInt(), o["h"].toInt());
            hb.type = static_cast<HitboxType>(o["type"].toInt());
            hitboxes.append(hb);
        }
        rebuildSceneHitboxes();
        updateHitboxList();
        statusBar()->showMessage("Loaded hitboxes from " + jsonPath, 3000);
    }
    if (root.contains("frame")) {
        const QJsonObject frame = root["frame"].toObject();
        const int w = frame["width"].toInt();
        const int h = frame["height"].toInt();
        if (w > 0 && h > 0) {
            gridSizeComboH->setCurrentText(QString::number(w));
            gridSizeComboV->setCurrentText(QString::number(h));
            commitGridSizes();   // rounds to a multiple of 8, applies the grid, remembers the sizes
        }
    }
    checkGridFits();
}

QRect Editor::snapToGrid(const QRect &rect) {
    QRect tmp(GRID *  (rect.left() / GRID),
              GRID *  (rect.top() / GRID),
              GRID *  (rect.width() / GRID),
              GRID *  (rect.height() / GRID));
    if (rect.left() % GRID >= GRID/2) {
        tmp.setLeft(tmp.left() + GRID);
        tmp.setRight(tmp.right() + GRID);
    }
    if (rect.top() % GRID >= GRID/2) {
        tmp.setTop(tmp.top() +GRID);
        tmp.setBottom(tmp.bottom() +GRID);
    }
    if (rect.width() % GRID >= GRID/2) {
        tmp.setWidth(tmp.width() + GRID);
    }
    if (rect.height() % GRID >= GRID/2) {
        tmp.setHeight(tmp.height() +GRID);
    }
    return tmp;
}

void Editor::updateStatus(){
    qreal scale=view->transform().m11();
    QString msg=QString("Zoom: %1%  ").arg((int)(scale*100));
    if(auto hb=selectedHitboxItem()){
        QRectF r=hb->rect().translated(hb->pos());
        msg+=QString("Selected: (%1,%2,%3,%4) ").arg(r.x()).arg(r.y()).arg(r.width()).arg(r.height());
    }
    msg += QString(" [%1, %2]").arg(currPos.x()).arg(currPos.y());
    status->showMessage(msg);
}

void Editor::rebuildSceneHitboxes(){
    if(!currentPixmapItem) return;
    for(const HitboxDef &hb:hitboxes){
        HitboxItem *it=new HitboxItem(hb.rect,hb.type);
        scene->addItem(it);
    }
}

HitboxItem* Editor::selectedHitboxItem(){
    for(auto *it:scene->selectedItems()){
        if(auto hb=dynamic_cast<HitboxItem*>(it)) return hb;
    }
    return nullptr;
}

void Editor::clearSelection(){
    for(auto *it:scene->selectedItems()){
        it->setSelected(false);
    }
    hbList->setCurrentRow(-1);
}

void Editor::updateHitboxList(){
    hbList->clear();
    for(const HitboxDef &hb:hitboxes){
        QString s=QString("[%1,%2,%3,%4] - %5").arg(hb.rect.x()).arg(hb.rect.y()).arg(hb.rect.width()).arg(hb.rect.height()).arg(typeName(hb.type));
        QListWidgetItem *it=new QListWidgetItem(s); it->setBackground(typeColor(hb.type).lighter()); hbList->addItem(it);
    }
    updateStatus();
}

void Editor::updateHitboxesFromScene(){
    hitboxes.clear();
    for(auto *it:scene->items()){
        if(auto hb=dynamic_cast<HitboxItem*>(it)){
            HitboxDef d; QRectF r=hb->rect().translated(hb->pos());
            d.rect=r.toRect(); d.type=hb->boxType; hitboxes.append(d);
        }
    }
}

void Editor::deleteSelectedHitbox(){
    if(auto hb=selectedHitboxItem()){
        pushUndoState();
        scene->removeItem(hb);
        delete hb;
        updateHitboxesFromScene();
        updateHitboxList();
        pushRedoClear();
    }
}

void Editor::onSceneContextMenu(const QPoint &pos)
{
    // Map viewport coordinates to scene coordinates
    QPointF scenePos = view->mapToScene(pos);

    // Get the item under cursor (optional: multiple selection)
    QGraphicsItem *item = scene->itemAt(scenePos, view->transform());

    clearSelection();
    if (item && item != currentPixmapItem) {
        selectListItemForSceneItem(static_cast<HitboxItem*>(item));
    } else {
        return;
    }

    QMenu menu(this);

    QAction *deleteAction = menu.addAction("Delete Hitbox");
    QAction *changeTypeAction = menu.addAction("Change Type");
    QAction *selected = menu.exec(view->viewport()->mapToGlobal(pos));
    if (!selected) return;

    if (selected == deleteAction && item) {
        scene->removeItem(item);
        delete item;
        updateHitboxesFromScene();
        updateHitboxList();
    } else if (selected == changeTypeAction && item) {
        if (auto hb = dynamic_cast<HitboxItem*>(item)) {
            hb->boxType = static_cast<HitboxType>((hb->boxType + 1) % 4);
            hb->updateColor();
            updateHitboxesFromScene();
            updateHitboxList();
        }
    }
}

void Editor::selectListItemForSceneItem(HitboxItem *hb)
{
    if (!hb) return;
    QRect rect = hb->rect().translated(hb->pos()).toRect();
    for (int i = 0; i < hitboxes.size(); ++i) {
        const HitboxDef &def = hitboxes[i];
        if (def.rect == rect && def.type == hb->boxType) {
            hbList->setCurrentRow(i);
            return;
        }
    }
}

void Editor::pushUndoState(){ State s; s.hitboxes=hitboxes; undoStack.push(s); }
void Editor::pushRedoClear(){ while(!redoStack.empty()) redoStack.pop(); }
void Editor::undo(){ if(undoStack.empty())return; redoStack.push({hitboxes}); hitboxes=undoStack.top().hitboxes; undoStack.pop(); reloadScene(); }
void Editor::redo(){ if(redoStack.empty())return; undoStack.push({hitboxes}); hitboxes=redoStack.top().hitboxes; redoStack.pop(); reloadScene(); }

void Editor::reloadScene(){
    scene->clear();
    if(!currentPixmap.isNull()) currentPixmapItem=scene->addPixmap(currentPixmap);
    rebuildSceneHitboxes();
    updateHitboxList();
}
