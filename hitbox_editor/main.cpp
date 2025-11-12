/* ==========================
   main.cpp
   ========================== */

#include <QApplication>
#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QFileDialog>
#include <QToolBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>
#include <QComboBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QMessageBox>
#include <QMouseEvent>
#include <QStyleOptionGraphicsItem>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QStatusBar>
#include <QMainWindow>
#include <QMenu>
#include <QCheckBox>
#include <stack>
//#include <optional>

static const int GRID = 8;
const QStringList sizes = {"16", "24", "32", "48", "64", "128", "256"};
enum HitboxType { MainBox, AttackBox, Special1Box, Special2Box };
struct HitboxDef { QRect rect; HitboxType type; };

static QColor typeColor(HitboxType t) {
    switch(t){
    case MainBox: return QColor(0,255,0,80);
    case AttackBox: return QColor(255,0,0,80);
    case Special1Box: return QColor(0,0,255,80);
    case Special2Box: return QColor(128,0,128,80);
    }
    return QColor(200,200,200,80);
}

static QString typeName(HitboxType t){
    switch(t){
    case MainBox: return "Main";
    case AttackBox: return "Attack";
    case Special1Box: return "Special1";
    case Special2Box: return "Special2";
    }
    return "Unknown";
}

class GridScene : public QGraphicsScene {
public:
    GridScene(QObject* parent=nullptr) : QGraphicsScene(parent) {}

    void setMajorGrid(int sizeH, int sizeV) { majorGridH = sizeH; majorGridV = sizeV; update(); }
    void setBackgroundColor(const QColor &color) { backgroundColor = color; update(); }

protected:

    void drawBackground(QPainter *painter, const QRectF &rect)
    {
        // Fill background
        painter->fillRect(rect, backgroundColor);

        const int minorGrid = 8;
        //const int majorGrid = 64;

        QPen minorPen(Qt::lightGray);
        minorPen.setWidth(0);
        QPen majorPen(QColor(90, 90, 90));

        // Draw minor grid (every 8 px)
        qreal left = std::floor(rect.left());
        qreal top = std::floor(rect.top());
        qreal right = std::ceil(rect.right());
        qreal bottom = std::ceil(rect.bottom());

        // Snap to multiples of minor grid
        int startX = static_cast<int>(left) - (static_cast<int>(left) % minorGrid);
        int startY = static_cast<int>(top) - (static_cast<int>(top) % minorGrid);

        painter->setPen(minorPen);
        for (int x = startX; x < right; x += minorGrid)
            painter->drawLine(x, top, x, bottom);
        for (int y = startY; y < bottom; y += minorGrid)
            painter->drawLine(left, y, right, y);

        // Draw major grid (every 64 px)
        startX = static_cast<int>(left) - (static_cast<int>(left) % majorGridH);
        startY = static_cast<int>(top) - (static_cast<int>(top) % majorGridV);

        painter->setPen(majorPen);
        for (int x = startX; x < right; x += majorGridH)
            painter->drawLine(x, top, x, bottom);
        for (int y = startY; y < bottom; y += majorGridV)
            painter->drawLine(left, y, right, y);
    }

private:
    int majorGridH = 64;
    int majorGridV = 64;
    QColor backgroundColor = QColor(255, 255, 255);
};


class HitboxItem : public QObject, public QGraphicsRectItem {
    Q_OBJECT
public:
    HitboxType boxType;
    QPointF dragStart;
    bool dragging=false;

    HitboxItem(const QRectF &r, HitboxType t):QGraphicsRectItem(r),boxType(t){
        setFlags(ItemIsSelectable|ItemIsMovable|ItemSendsGeometryChanges);
        setAcceptHoverEvents(true);
        updateColor();
    }

    void updateColor(){
        QColor c=typeColor(boxType);
        setBrush(c);
        setPen(QPen(c.darker(),2));
    }

    static qreal snap(qreal v){return static_cast<int>(v/GRID) *GRID;}

    QVariant itemChange(GraphicsItemChange change,const QVariant &val) override {
        if(change==ItemPositionChange && scene()){
            QPointF p=val.toPointF();
            qDebug("x=%f x=%f", p.x(), p.y());
            p.setX(snap(p.x())); p.setY(snap(p.y()));
            qDebug("x=%f x=%f", p.x(), p.y());
            return p;
        }
        return QGraphicsRectItem::itemChange(change,val);
    }

    void mousePressEvent(QGraphicsSceneMouseEvent *e) override {
        dragStart = pos();
        dragging=true;
        QGraphicsRectItem::mousePressEvent(e);
    }
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *e) override {
        if(dragging){ dragging=false; emitMoved(); }
        QGraphicsRectItem::mouseReleaseEvent(e);
    }

signals:
    void moved();

private:
    void emitMoved(){ emit moved(); }

    void paint(QPainter *p,const QStyleOptionGraphicsItem *opt,QWidget *w) override {
        QGraphicsRectItem::paint(p,opt,w);
        if(isSelected()){
            p->save();
            p->setPen(QPen(Qt::black,1,Qt::DashLine));
            p->setBrush(Qt::NoBrush);
            p->drawRect(rect());
            p->restore();
        }
    }
};

class Editor : public QMainWindow {
    Q_OBJECT
public:
    Editor(QWidget *p=nullptr):QMainWindow(p){
        scene=new GridScene(this);// QGraphicsScene(this);
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


        // Major grid size selector
        gridSizeComboH = new QComboBox();
        gridSizeComboH->addItems(sizes);
        gridSizeComboH->setCurrentText("64");
        tools->addWidget(new QLabel("Grid:"));
        tools->addWidget(gridSizeComboH);

        gridSizeComboV = new QComboBox();
        gridSizeComboV->addItems(sizes);
        gridSizeComboV->setCurrentText("64");
        tools->addWidget(gridSizeComboV);


        // Background color selector
        QComboBox *bgColorCombo = new QComboBox();
        bgColorCombo->addItems({"White", "Dark Gray", "Light Gray",  "Dark Cyan"});
        tools->addWidget(new QLabel("Background:"));
        tools->addWidget(bgColorCombo);

        connect(gridSizeComboH, &QComboBox::currentTextChanged, this, [this](const QString &) {
            if (!scene) return;
            auto h = gridSizeComboH->currentText();
            auto v = gridSizeComboV->currentText();
            scene->setMajorGrid(h.toInt(), v.toInt());
        });

        connect(gridSizeComboV, &QComboBox::currentTextChanged, this, [this](const QString &) {
            if (!scene) return;
            auto h = gridSizeComboH->currentText();
            auto v = gridSizeComboV->currentText();
            scene->setMajorGrid(h.toInt(), v.toInt());
        });


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

protected:
    void resizeEvent(QResizeEvent *e) override {
        QMainWindow::resizeEvent(e);
        if(currentPixmapItem) view->fitInView(currentPixmapItem,Qt::KeepAspectRatio);
    }

    bool eventFilter(QObject *obj,QEvent *ev) override {
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
                        //creatingRect->rect() = tmp;
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

    void keyPressEvent(QKeyEvent *e) override {
        if(e->key()==Qt::Key_Delete){ deleteSelectedHitbox(); return; }
        if(e->matches(QKeySequence::Undo)){ undo(); return; }
        if(e->matches(QKeySequence::Redo)){ redo(); return; }
        if(auto hb=selectedHitboxItem()){
            QPointF move(0,0);
            if(e->key()==Qt::Key_Left) move.setX(-GRID);
            if(e->key()==Qt::Key_Right) move.setX(GRID);
            if(e->key()==Qt::Key_Up) move.setY(-GRID);
            if(e->key()==Qt::Key_Down) move.setY(GRID);
            if(e->key()==Qt::Key_Escape) {
               // selectedHitboxItem()
            }
            if(move!=QPointF(0,0)){
                pushUndoState();
                hb->moveBy(move.x(),move.y());
                pushRedoClear();
                updateHitboxesFromScene();
                updateHitboxList();
            }
        }
    }

private slots:
    void onOpen(){
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
        lastOpenedFolder = QFileInfo(imagePath).dir().path();

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
    }

    void onSave(){        
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
        int frameWidth = gridSizeComboH->currentText().toInt();
        int frameHeight = gridSizeComboV->currentText().toInt();
        root["frame"] = QJsonObject{
            {"width", frameWidth},
            {"height", frameHeight },
            {"cols", currentPixmap.width() / frameWidth}
        };

        QFile file(f); if(file.open(QIODevice::WriteOnly)) file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        lastOpenedFolder = QFileInfo(f).dir().path();
    }

    void onLoad(){
        QString f=QFileDialog::getOpenFileName(this,"Load", lastOpenedFolder,"JSON (*.json)");
        if(f.isEmpty())
            return;
        QFile file(f);
        if(!file.open(QIODevice::ReadOnly))return;
        QJsonDocument doc=QJsonDocument::fromJson(file.readAll());
        QJsonObject root=doc.object();
        QString path=root["imagePath"].toString();
        currentPixmap=QPixmap(path);
        imagePath = path;
        if(currentPixmap.isNull())return;
        scene->clear();
        hitboxes.clear();
        currentPixmapItem=scene->addPixmap(currentPixmap);

        reloadData(root, f);

        view->fitInView(currentPixmapItem,Qt::KeepAspectRatio);
        pushUndoState();
        pushRedoClear();
        lastOpenedFolder = QFileInfo(f).dir().path();
    }

    void onImport()
    {
        if (!currentPixmapItem || currentPixmapItem->pixmap().isNull()) {
            QMessageBox::warning(this, "No Image", "You must load an image before importing hitboxes.");
            return;
        }

        QString importImagePath = QFileDialog::getOpenFileName(
            this,
            "Select Image to Import Hitboxes From",
            "",
            "Images (*.png *.jpg *.bmp)"
            );

        if (importImagePath.isEmpty())
            return;

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


    void onHitboxSelected(int r){
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

private:

    void reloadData(QJsonObject& root, QString &jsonPath)
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
          //  qDebug("****hitboxes: %llu", hitboxes.size());
            rebuildSceneHitboxes();
            updateHitboxList();
            statusBar()->showMessage("Loaded hitboxes from " + jsonPath, 3000);
        }
        if (root.contains("frame")) {
            const QJsonObject frame = root["frame"].toObject();
            const int w = frame["width"].toInt();
            const int h = frame["height"].toInt();
            qDebug(">>>>> w: %d h: %d", w,h);
            scene->setMajorGrid(w,h);
            int i = 0;
            for (auto const &size: sizes) {
                if (size.toInt() == w) {
                    gridSizeComboH->setCurrentIndex(i);
                }
                if (size.toInt() == h) {
                    gridSizeComboV->setCurrentIndex(i);
                }
                ++i;
            }
        }
    }


    void calculateSpriteHitboxes(QJsonObject & rootObj, int frameWidth, int frameHeight)
    {
        // Calculate frame grid
        int columns = currentPixmap.width() / frameWidth;
        int rows = currentPixmap.height() / frameHeight;

        struct SpriteFrame {
            QRect frameRect;             // position of the frame in the spritesheet
            QVector<HitboxDef> hitboxes; // hitboxes local to this frame
        };

        QVector<SpriteFrame> frames;
        for (int y = 0; y < rows; ++y)
            for (int x = 0; x < columns; ++x)
                frames.append({ QRect(x * frameWidth, y * frameHeight, frameWidth, frameHeight), {} });

        // Assign hitboxes to frames
        for (const HitboxDef &hb : hitboxes) {
            QPoint center = hb.rect.center();
            for (SpriteFrame &frame : frames) {
                if (frame.frameRect.contains(center)) {
                    HitboxDef local = hb;
                    local.rect.translate(-frame.frameRect.topLeft());
                    frame.hitboxes.append(local);
                    break;
                }
            }
        }

        // Save per-frame hitbox data
        QJsonArray framesArray;
        for (const SpriteFrame &frame : frames) {
            QJsonObject frameObj;
            frameObj["rect"] = QJsonArray{ frame.frameRect.x(), frame.frameRect.y(), frame.frameRect.width(), frame.frameRect.height() };
            QJsonArray frameHitboxes;
            for (const HitboxDef &hb : frame.hitboxes) {
                QJsonObject hbObj;
                hbObj["x"] = hb.rect.x();
                hbObj["y"] = hb.rect.y();
                hbObj["w"] = hb.rect.width();
                hbObj["h"] = hb.rect.height();
                hbObj["type"] = hb.type;
                frameHitboxes.append(hbObj);
            }
            frameObj["hitboxes"] = frameHitboxes;
            framesArray.append(frameObj);
        }
        rootObj["frames"] = framesArray;
    }


    QRect snapToGrid(const QRect &rect) {
      //  qDebug("**************************");
     //   qDebug(">>>>HB  %d %d %d %d", rect.x(), rect.y(), rect.width(), rect.height());

        QRect tmp(GRID *  (rect.left() / GRID),
                  GRID *  (rect.top() / GRID),
                  GRID *  (rect.width() / GRID),
                  GRID *  (rect.height() / GRID));
       // qDebug(">>>>tmp %d %d %d %d", tmp.x(), tmp.y(), tmp.width(), tmp.height());
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
       // qDebug(">>>>tmp %d %d %d %d", tmp.x(), tmp.y(), tmp.width(), tmp.height());
        return tmp;
    }

    void updateStatus(){
        qreal scale=view->transform().m11();
        QString msg=QString("Zoom: %1%  ").arg((int)(scale*100));
        if(auto hb=selectedHitboxItem()){
            QRectF r=hb->rect().translated(hb->pos());
            msg+=QString("Selected: (%1,%2,%3,%4) ").arg(r.x()).arg(r.y()).arg(r.width()).arg(r.height());
        }
        msg += QString(" [%1, %2]").arg(currPos.x()).arg(currPos.y());
        status->showMessage(msg);
    }

    void rebuildSceneHitboxes(){
        if(!currentPixmapItem) return;
        for(const HitboxDef &hb:hitboxes){
            HitboxItem *it=new HitboxItem(hb.rect,hb.type);
            scene->addItem(it);
        }
    }

    HitboxItem* selectedHitboxItem(){
        for(auto *it:scene->selectedItems()){
            if(auto hb=dynamic_cast<HitboxItem*>(it)) return hb;
        }
        return nullptr;
    }

    void clearSelection(){
        for(auto *it:scene->selectedItems()){
            it->setSelected(false);
        }
        hbList->setCurrentRow(-1);
    }

    void updateHitboxList(){
        hbList->clear();
        for(const HitboxDef &hb:hitboxes){
            QString s=QString("[%1,%2,%3,%4] - %5").arg(hb.rect.x()).arg(hb.rect.y()).arg(hb.rect.width()).arg(hb.rect.height()).arg(typeName(hb.type));
            QListWidgetItem *it=new QListWidgetItem(s); it->setBackground(typeColor(hb.type).lighter()); hbList->addItem(it);
        }
        updateStatus();
    }

    void updateHitboxesFromScene(){
        hitboxes.clear();
        for(auto *it:scene->items()){
            if(auto hb=dynamic_cast<HitboxItem*>(it)){
                HitboxDef d; QRectF r=hb->rect().translated(hb->pos());
                d.rect=r.toRect(); d.type=hb->boxType; hitboxes.append(d);
            }
        }
    }

    void deleteSelectedHitbox(){
        if(auto hb=selectedHitboxItem()){
            pushUndoState();
            scene->removeItem(hb);
            delete hb;
            updateHitboxesFromScene();
            updateHitboxList();
            pushRedoClear();
        }
    }

    void onSceneContextMenu(const QPoint &pos)
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
            updateHitboxList(); // optional: update your hitbox list
        } else if (selected == changeTypeAction && item) {
            if (auto hb = dynamic_cast<HitboxItem*>(item)) {
                hb->boxType = static_cast<HitboxType>((hb->boxType + 1) % 4);
                hb->updateColor();
                updateHitboxesFromScene();
                updateHitboxList();
            }
        }
    }

    void selectListItemForSceneItem(HitboxItem *hb)
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

    struct State { QList<HitboxDef> hitboxes; };
    std::stack<State> undoStack, redoStack;

    void pushUndoState(){ State s; s.hitboxes=hitboxes; undoStack.push(s); }
    void pushRedoClear(){ while(!redoStack.empty()) redoStack.pop(); }
    void undo(){ if(undoStack.empty())return; redoStack.push({hitboxes}); hitboxes=undoStack.top().hitboxes; undoStack.pop(); reloadScene(); }
    void redo(){ if(redoStack.empty())return; undoStack.push({hitboxes}); hitboxes=redoStack.top().hitboxes; redoStack.pop(); reloadScene(); }

    void reloadScene(){
        scene->clear();
        if(!currentPixmap.isNull()) currentPixmapItem=scene->addPixmap(currentPixmap);
        rebuildSceneHitboxes();
        updateHitboxList();
    }

    QGraphicsView *view;
    GridScene *scene;
    QGraphicsPixmapItem *currentPixmapItem=nullptr;
    QPixmap currentPixmap;
    QList<HitboxDef> hitboxes;
    HitboxItem *creatingRect=nullptr;
    QPointF startPos;
    QComboBox *typeCombo;
    QListWidget *hbList;
    QStatusBar *status;
    QPoint currPos;
    QString imagePath;
    QString lastOpenedFolder;
    QComboBox *gridSizeComboH;
    QComboBox *gridSizeComboV;
};


int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QMainWindow win;
    Editor *ed = new Editor;
    win.setCentralWidget(ed);
    win.resize(1000, 700);
    win.setWindowTitle("Qt Hitbox Editor");
    win.show();
    return app.exec();
}

#include "main.moc"
