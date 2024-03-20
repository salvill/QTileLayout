#ifndef QTILELAYOUT_H
#define QTILELAYOUT_H

#include "tile.h"
#include <QWidget>
#include <QGridLayout>
#include <QUuid>
#include <QPalette>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>

class QTileLayout : public QGridLayout {
    Q_OBJECT

public:
    explicit QTileLayout(int rowNumber, int columnNumber, int verticalSpan, int horizontalSpan,
                int verticalSpacing = 5, int horizontalSpacing = 5, QWidget *parent = nullptr);

    void addWidget(QWidget *widget, int fromRow, int fromColumn, int rowSpan = 1, int columnSpan = 1);
    void removeWidget(QWidget *widget);
    void addRows(int rowNumber);
    void addColumns(int columnNumber);
    void removeRows(int rowNumber);
    void removeColumns(int columnNumber);
    void acceptDragAndDrop(bool value);
    void acceptResizing(bool value);
    void setCursorIdle(Qt::CursorShape value);
    void setCursorGrab(Qt::CursorShape value);
    void setCursorResizeHorizontal(Qt::CursorShape value);
    void setCursorResizeVertical(Qt::CursorShape value);
    void setColorIdle(QColor color);
    void setColorResize(QColor color);
    void setColorDragAndDrop(QColor color);
    void setColorEmptyCheck(QColor color);
    int rowCount() const;
    int columnCount() const;
    QRect tileRect(int row, int column) const;
    int rowsMinimumHeight() const;
    int columnsMinimumWidth() const;
    void setRowsMinimumHeight(int height);
    void setColumnsMinimumWidth(int width);
    void setRowsHeight(int height);
    void setColumnsWidth(int width);
    void setVerticalSpacing(int spacing);
    void setHorizontalSpacing(int spacing);
    QString getId() const;
    void activateFocus(bool focus);
    QList<QWidget*> widgetList() const;
    void linkLayout(QTileLayout *layout);
    void unLinkLayout(QTileLayout *layout);
    void highlightTiles(QPoint direction, int fromRow, int fromColumn, int tileNumber);
    void resizeTile(QPoint direction, int fromRow, int fromColumn, int tileNumber);
    Tile* hardSplitTiles(int fromRow, int fromColumn, QList<QPoint> tilesToSplit);
    bool isAreaEmpty(int fromRow, int fromColumn, int rowSpan, int columnSpan, QString color = "");
    QWidget* getWidgetToDrop();
    void setWidgetToDrop(QWidget *widget);
    void changeTilesColor(QString colorChoice, QPoint fromTile = QPoint(0, 0), QPoint toTile = QPoint());

    bool getDragAndDrop() const;
    bool getResizable() const;
    bool getFocus() const;

    Qt::CursorShape getCursorIdle() const;
    Qt::CursorShape getCursorGrab() const;
    Qt::CursorShape getCursorResizeHorizontal() const;
    Qt::CursorShape getCursorResizeVertical() const;

    QMap<QUuid, QTileLayout *> getLinkedLayout() const;
    void updateGlobalSize(QResizeEvent *newSize);

signals:
    void tileResized(QWidget *widget, int fromRow, int fromColumn, int rowSpan, int columnSpan);
    void tileMoved(QWidget *widget, QString str, QString str2, int fromRow, int fromColumn, int rowSpan, int columnSpan);

protected:
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);

private:
    void mergeTiles(Tile *tile, int fromRow, int fromColumn, int rowSpan, int columnSpan, QList<QPoint> tilesToMerge);
    void splitTiles(Tile *tile, int fromRow, int fromColumn, int rowSpan, int columnSpan, QList<QPoint> tilesToSplit);
    Tile* createTile(int fromRow, int fromColumn, int rowSpan = 1, int columnSpan = 1, bool updateTileMap = false);
    // QSharedPointer<Tile> createTile(int fromRow, int fromColumn, int rowSpan = 1, int columnSpan = 1, bool updateTileMap = false);
    std::tuple<QList<QPoint>, bool, int, int, int, int> getTilesToBeResized(Tile* tile, QPoint direction, int fromRow, int fromColumn, int tileNumber);
    std::tuple<int, QList<QPoint> > getTilesToSplit(QPoint direction, int fromRow, int fromColumn, int tileNumber);
    std::tuple<int, QList<QPoint> > getTilesToMerge(QPoint direction, int fromRow, int fromColumn, int tileNumber);

    void createTileMap();
    void updateAllTiles();

private:
    int rowNumber;
    int columnNumber;
    int verticalSpan;
    int horizontalSpan;
    int minVerticalSpan;
    int minHorizontalSpan;
    bool dragAndDrop;
    bool resizable;
    bool focus;
    QWidget *widgetToDrop;
    QList<QList<Tile*>> tileMap;
    // QList<QList<QSharedPointer<Tile>>> tileMap;
    QMap<QString, QList<QWidget *>> widgetTileCouple;
    QMap<QUuid, QTileLayout*> linkedLayout;
    QUuid id;
    Qt::CursorShape cursorIdle;
    Qt::CursorShape cursorGrab;
    Qt::CursorShape cursorResizeHorizontal;
    Qt::CursorShape cursorResizeVertical;
    QMap<QString, QColor> colorMap;
static QVector<int> flattenList(const QVector<QVector<int>>& toFlatten);
};

#endif // QTILELAYOUT_H
