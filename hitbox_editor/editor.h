#ifndef EDITOR_H
#define EDITOR_H

#include "hitbox.h"

#include <QMainWindow>
#include <QPixmap>
#include <QList>
#include <QPoint>
#include <QPointF>
#include <QString>
#include <stack>

class QGraphicsView;
class QGraphicsPixmapItem;
class QComboBox;
class QListWidget;
class QStatusBar;

class Editor : public QMainWindow {
    Q_OBJECT
public:
    Editor(QWidget *p=nullptr);

protected:
    void resizeEvent(QResizeEvent *e) override;
    bool eventFilter(QObject *obj,QEvent *ev) override;
    void keyPressEvent(QKeyEvent *e) override;

private slots:
    void onOpen();
    void onSave();
    void onLoad();
    void onImport();
    void onHitboxSelected(int r);

private:
    void reloadData(QJsonObject& root, QString &jsonPath);
    void rememberFolder(const QString &dir);
    void applyGridSize();
    void commitGridSizes();
    void commitGridSize(QComboBox *box, QStringList &list, const QString &key);
    void rememberGridSize(QComboBox *box, QStringList &list, const QString &key);
    void checkGridFits();
    QRect snapToGrid(const QRect &rect);
    void updateStatus();
    void rebuildSceneHitboxes();
    HitboxItem* selectedHitboxItem();
    void clearSelection();
    void updateHitboxList();
    void updateHitboxesFromScene();
    void deleteSelectedHitbox();
    void onSceneContextMenu(const QPoint &pos);
    void selectListItemForSceneItem(HitboxItem *hb);

    struct State { QList<HitboxDef> hitboxes; };
    std::stack<State> undoStack, redoStack;

    void pushUndoState();
    void pushRedoClear();
    void undo();
    void redo();
    void reloadScene();

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
    QStringList customGridW;
    QStringList customGridH;
};

#endif // EDITOR_H
