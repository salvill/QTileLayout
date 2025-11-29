#include "tile.h"
#include <QPointer>
#include "customshadoweffect.h"
#include "qtilelayout.h"
#include <QDebug>

// The basic component of a tileLayout
Tile::Tile(QTileLayout *tileLayout, int fromRow, int fromColumn, int rowSpan, int columnSpan, int verticalSpan, int horizontalSpan, QWidget *parent)
    : QWidget(parent),
    tileLayout(tileLayout),
    originTileLayout(this->tileLayout),
    fromRow(fromRow),
    fromColumn(fromColumn),
    rowSpan(rowSpan),
    columnSpan(columnSpan),
    verticalSpan(verticalSpan),
    horizontalSpan(horizontalSpan),
    resizeMargin(5),
    filled(false),
    widget(nullptr),
    lock(QPoint()),
    dragInProcess(false),
    currentTileNumber(0)
{
    this->layout = new QVBoxLayout();
    this->layout->setSpacing(0);
    this->layout->setContentsMargins(0, 0, 0, 0);

    this->mouseMovePos = QPoint();
    this->updateSizeLimit();
    this->setAcceptDrops(true);
    this->setMouseTracking(true);
    this->setLayout(layout);

    // connect(this, &Tile::tileMoved, tileLayout, &QTileLayout::tileMoved);
    connect(this, &Tile::tileMoved, this, &Tile::tileHasBeenMoved);

}

// Changes the tile size
void Tile::updateSize(int fromRow, int fromColumn, int rowSpan, int columnSpan, int verticalSpan, int horizontalSpan) {
    this->fromRow = (fromRow != -1) ? fromRow : this->fromRow;
    this->fromColumn = (fromColumn != -1) ? fromColumn : this->fromColumn;
    this->rowSpan = (rowSpan != -1) ? rowSpan : this->rowSpan;
    this->columnSpan = (columnSpan != -1) ? columnSpan : this->columnSpan;
    this->verticalSpan = (verticalSpan != -1) ? verticalSpan : this->verticalSpan;
    this->horizontalSpan = (horizontalSpan != -1) ? horizontalSpan : this->horizontalSpan;
    this->updateSizeLimit();
}

// Adds a widget in the tile
void Tile::addWidget(QWidget *widget) {
    layout->addWidget(widget);
    this->widget = widget;
    filled = true;
}

// Returns the tile from row
int Tile::getFromRow() const {
    return fromRow;
}

// Returns the tile from column
int Tile::getFromColumn() const {
    return fromColumn;
}

// Returns the tile row span
int Tile::getRowSpan() const {
    return rowSpan;
}

// Returns the tile column span
int Tile::getColumnSpan() const {
    return columnSpan;
}

QString Tile::getInfo()
{
    return QString("TileInfo: FromRow %1, FromColumn %2, RowSpan %3, ColumnSpan %4")
        .arg(fromRow)
        .arg(fromColumn)
        .arg(rowSpan)
        .arg(columnSpan);
}

// Returns True if there is a widget in the tile, else False
bool Tile::isFilled() const {
    return filled;
}

// Changes the tile background color
void Tile::changeColor(const QPalette &color) {
    setAutoFillBackground(true);
    setPalette(QPalette(color));
}

// Actions to do when the mouse is moved
void Tile::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() == Qt::LeftButton) {
        // Adjust offset from clicked point to origin of widget
        if (!mouseMovePos.isNull() && !dragInProcess && lock.isNull()) {
            QPoint globalPos = event->globalPos();
            QPoint lastPos = mapToGlobal(mouseMovePos);
            // Calculate the difference when moving the mouse
            QPoint diff = globalPos - lastPos;

            if (diff.manhattanLength() > 3) {
                if (filled && tileLayout->getDragAndDrop()) {
                    QDrag *drag = prepareDropData(event);
                    dragAndDropProcess(drag);
                    QMap<QUuid, QTileLayout *> mapTileLayout = tileLayout->getLinkedLayout();
                    for (auto it = mapTileLayout.begin(); it != mapTileLayout.end(); ++it) {
                        it.value()->changeTilesColor("idle");
                    }
                }
                if (filled && tileLayout->getFocus()) {
                    widget->setFocus();
                }
            }
        }
    }

    if (!filled) {
        setCursor(QCursor(tileLayout->getCursorIdle()));
    } else if (lock.isNull()) {
        bool westCondition = 0 <= event->pos().x() && event->pos().x() < resizeMargin;
        bool eastCondition = width() >= event->pos().x() && event->pos().x() > width() - resizeMargin;
        bool northCondition = 0 <= event->pos().y() && event->pos().y() < resizeMargin;
        bool southCondition = height() >= event->pos().y() && event->pos().y() > height() - resizeMargin;

        if ((westCondition || eastCondition) && tileLayout->getResizable()) {
            setCursor(QCursor(tileLayout->getCursorResizeHorizontal()));
        } else if ((northCondition || southCondition) && tileLayout->getResizable()) {
            setCursor(QCursor(tileLayout->getCursorResizeVertical()));
        } else if (tileLayout->getDragAndDrop() && rect().contains(event->pos())) {
            setCursor(QCursor(tileLayout->getCursorGrab()));
        } else {
            setCursor(QCursor(tileLayout->getCursorIdle()));
        }
    } else {
        // highlight tiles that are going to be merged in the resizing
        int x = event->pos().x();
        int y = event->pos().y();
        int tileNumber = getResizeTileNumber(x, y);

        if (tileNumber != currentTileNumber) {
            currentTileNumber = tileNumber;
            tileLayout->changeTilesColor("resize");
            tileLayout->highlightTiles(lock, fromRow, fromColumn, tileNumber);
        }
    }

    QWidget::mouseMoveEvent(event);
}

// Actions to do when the mouse button is pressed
void Tile::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        // mouseMovePos = new QPoint(event->pos());
        mouseMovePos = event->pos();
        if (event->pos().x() < resizeMargin && tileLayout->getResizable()) {
            lock = QPoint(-1, 0);  // 'west'
        }
        else if (event->pos().x() > width() - resizeMargin && tileLayout->getResizable()) {
            lock = QPoint(1, 0);  // 'east'
        }
        else if (event->pos().y() < resizeMargin && tileLayout->getResizable()) {
            lock = QPoint(0, -1);  // 'north'
        }
        else if (event->pos().y() > height() - resizeMargin && tileLayout->getResizable()) {
            lock = QPoint(0, 1);  // 'south'
        }

        qDebug() << __FUNCTION__ << lock;

        if (!lock.isNull()) {
            tileLayout->changeTilesColor("resize");
        }
    } else {
        // delete mouseMovePos;
        mouseMovePos = QPoint();
    }

    QWidget::mousePressEvent(event);
}

// Actions to do when the mouse button is released
void Tile::mouseReleaseEvent(QMouseEvent *event) {
    if (lock.isNull()) {
        QWidget::mouseReleaseEvent(event);
        return;
    }

    int x = event->pos().x();
    int y = event->pos().y();
    int tileNumber = getResizeTileNumber(x, y);

    tileLayout->resizeTile(lock, fromRow, fromColumn, tileNumber);
    tileLayout->changeTilesColor("idle");
    currentTileNumber = 0;
    lock = QPoint();
    QWidget::mouseReleaseEvent(event);
}

void Tile::dragEnterEvent(QDragEnterEvent *event) {
    if (tileLayout->getDragAndDrop() && event->mimeData()->hasFormat("TileData") && isDropPossible(event)) {
        event->acceptProposedAction();
    }
}

void Tile::dropEvent(QDropEvent *event) {
    QJsonObject dropData = QJsonDocument::fromJson(event->mimeData()->data("TileData")).object();
    QWidget *widget = originTileLayout->getWidgetToDrop();

    tileLayout->addWidget(
        widget,
        fromRow - dropData["row_offset"].toInt(),
        fromColumn - dropData["column_offset"].toInt(),
        dropData["row_span"].toInt(),
        dropData["column_span"].toInt()
        );

    // emit tileMoved(
    //     widget,
    //     dropData["id"].toString(),
    //     tileLayout->getId(),
    //     dropData["from_row"].toInt(),
    //     dropData["from_column"].toInt(),
    //     fromRow - dropData["row_offset"].toInt(),
    //     fromColumn - dropData["column_offset"].toInt()
    //     );
    event->acceptProposedAction();
}

// Prepares data for the drag and drop process
QDrag *Tile::prepareDropData(QMouseEvent *event) {
    QDrag *drag = new QDrag(this);
    QMimeData *dropData = new QMimeData();
    QJsonObject data;
    data["id"] = tileLayout->getId(); // this is string than we convert it again in uuid
    data["from_row"] = fromRow;
    data["from_column"] = fromColumn;
    data["row_span"] = rowSpan;
    data["column_span"] = columnSpan;
    data["row_offset"] = event->pos().y() / (verticalSpan + tileLayout->verticalSpacing());
    data["column_offset"] = event->pos().x() / (horizontalSpan + tileLayout->horizontalSpacing());

    QByteArray dataBytes = QJsonDocument(data).toJson();
    dropData->setData("TileData", dataBytes);

    CustomShadowEffect *bodyShadow = new CustomShadowEffect(widget);
    double dist = 10.0;
    bodyShadow->setBlurRadius(20.0);
    bodyShadow->setDistance(dist);
    bodyShadow->setColor(QColor(0, 0, 0, 80));
    widget->setAutoFillBackground(true);
    widget->setGraphicsEffect(bodyShadow);

    QPixmap dragIcon = widget->grab(widget->rect().adjusted(-dist, -dist, dist, dist));

    drag->setPixmap(dragIcon);
    drag->setMimeData(dropData);
    drag->setHotSpot(event->pos() - rect().topLeft());

    return drag;
}

// Manages the drag and drop process
void Tile::dragAndDropProcess(QDrag *drag) {
    dragInProcess = true;
    int previousRowSpan = rowSpan;
    int previousColumnSpan = columnSpan;
    int previousFromRow = fromRow;
    int previousFromColumn = fromColumn;
    QTileLayout* previousTileLayout = tileLayout;
    QWidget* previousWidget = widget;

    tileLayout->setWidgetToDrop(widget);
    widget->clearFocus();
    
    // Use QPointer to track if 'this' is deleted
    QPointer<Tile> self(this);

    tileLayout->removeWidget(widget);
    
    if (self) {
        setVisible(false);

        QMap<QUuid, QTileLayout *> mapTileLayout = tileLayout->getLinkedLayout();

        for (auto it = mapTileLayout.begin(); it != mapTileLayout.end(); ++it) {
            if (it.value()->getDragAndDrop()) {
                it.value()->changeTilesColor("drag_and_drop");
            }
        }
    }

    if (drag->exec() == Qt::IgnoreAction) {
        // If the drag was ignored, we need to put the widget back.
        // We use the saved local variables because 'this' might be deleted or invalid if we rely on member access after removeWidget (though here we are in the same scope, removeWidget might have triggered deletion if not for the fact we are in a method of the object... wait, removeWidget calls deleteLater() on the tile? 
        // In QTileLayout::removeWidget:
        // tileMap[row][column]->deleteLater();
        // So yes, 'this' is scheduled for deletion. Accessing members is risky if the event loop spins. drag->exec() spins the event loop.
        
        QWidget *widgetNew = previousTileLayout->getWidgetToDrop();
        // If widgetNew is null, it might mean it was dropped somewhere else or something happened. 
        // But here we are in IgnoreAction, so it wasn't accepted.
        
        if (!widgetNew) widgetNew = previousWidget;

        previousTileLayout->addWidget(
            widgetNew,
            previousFromRow,
            previousFromColumn,
            previousRowSpan,
            previousColumnSpan
            );
        if (previousTileLayout->getFocus()) {
            widgetNew->setFocus();
        }
    }

    if (self) {
        originTileLayout = tileLayout;
        setVisible(true);
        dragInProcess = false;
    }
}

// Checks if this tile can accept the drop
bool Tile::isDropPossible(QDropEvent *event) {
    QByteArray tileData = event->mimeData()->data("TileData");
    QJsonDocument dropData;

    try {
        dropData = QJsonDocument::fromJson(tileData);
    } catch (const QJsonParseError &error) {
        qDebug() << "JSON parsing error:" << error.errorString();
        return false;
    }

    if (dropData.isNull() || !dropData.isObject())
        return false;

    QMap<QUuid, QTileLayout *> mapTileLayout = tileLayout->getLinkedLayout();

    if (!mapTileLayout.contains(QUuid(dropData["id"].toString()))) {
        return false;
    } else {
        originTileLayout = mapTileLayout.value(QUuid(dropData["id"].toString()));
    }

    QMap<QUuid, QTileLayout *> mapOriginTileLayout = originTileLayout->getLinkedLayout();

    for (auto it = mapOriginTileLayout.begin(); it != mapOriginTileLayout.end(); ++it) {
        if (it.value()->getDragAndDrop()) {
            it.value()->changeTilesColor("drag_and_drop");
        }
    }

    return tileLayout->isAreaEmpty(
        fromRow - dropData["row_offset"].toInt(),
        fromColumn - dropData["column_offset"].toInt(),
        dropData["row_span"].toInt(),
        dropData["column_span"].toInt(),
        "drag_and_drop"
        );
}

// Finds the tile number when resizing
int Tile::getResizeTileNumber(int x, int y) {
    int dirX = lock.x();
    int dirY = lock.y();

    int span = horizontalSpan * (dirX != 0) + verticalSpan * (dirY != 0);
    int tileSpan = columnSpan * (dirX != 0) + rowSpan * (dirY != 0);
    int spacing = tileLayout->verticalSpacing() * (dirX != 0) + tileLayout->horizontalSpacing() * (dirY != 0);

    int res = static_cast<int>((x * (dirX != 0) + y * (dirY != 0) + (span / 2) - span * tileSpan * ((dirX + dirY) == 1)) / (span + spacing));

    qDebug() << "getResizeTileNumber: " << res;
    return res;
}

// Removes the tile widget
void Tile::removeWidget() {
    layout->removeWidget(widget);
    delete widget;
    widget = nullptr;
    filled = false;
}

// Refreshes the tile size limit
void Tile::updateSizeLimit() {
    setFixedHeight((rowSpan * verticalSpan) + ((rowSpan - 1) * tileLayout->verticalSpacing()));
    setFixedWidth((columnSpan * horizontalSpan) + ((columnSpan - 1) * tileLayout->horizontalSpacing()));
}

void Tile::tileHasBeenMoved(QWidget *widget, const QString &from_layout_id, const QString &to_layout_id, int from_row, int from_column, int to_row, int to_column) {
    qDebug() << widget << " has been moved from position (" << from_row << ", " << from_column << ") to ("
             << to_row << ", " << to_column << ")";
}
