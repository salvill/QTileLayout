#include "tile.h"
#include "qtilelayout.h"
#include <QDebug>

Tile::Tile(QTileLayout *tileLayout, int fromRow, int fromColumn, int rowSpan, int columnSpan, int verticalSpan, int horizontalSpan, QWidget *parent)
    : QLabel(parent),
    tileLayout(tileLayout),
    originTileLayout(tileLayout),
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
    layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    setAcceptDrops(true);
    setMouseTracking(true);
    setLayout(layout);

}

void Tile::updateSize(int fromRow, int fromColumn, int rowSpan, int columnSpan, int verticalSpan, int horizontalSpan) {
    this->fromRow = (fromRow != -1) ? fromRow : this->fromRow;
    this->fromColumn = (fromColumn != -1) ? fromColumn : this->fromColumn;
    this->rowSpan = (rowSpan != -1) ? rowSpan : this->rowSpan;
    this->columnSpan = (columnSpan != -1) ? columnSpan : this->columnSpan;
    this->verticalSpan = (verticalSpan != -1) ? verticalSpan : this->verticalSpan;
    this->horizontalSpan = (horizontalSpan != -1) ? horizontalSpan : this->horizontalSpan;
    updateSizeLimit();
}

void Tile::addWidget(QWidget *widget) {
    layout->addWidget(widget);
    this->widget = widget;
    filled = true;
}

int Tile::getFromRow() const {
    return fromRow;
}

int Tile::getFromColumn() const {
    return fromColumn;
}

int Tile::getRowSpan() const {
    return rowSpan;
}

int Tile::getColumnSpan() const {
    return columnSpan;
}

bool Tile::isFilled() const {
    return filled;
}

void Tile::changeColor(const QPalette &color) {
    setAutoFillBackground(true);
    setPalette(QPalette(color));
}

void Tile::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() == Qt::LeftButton) {
        // Adjust offset from clicked point to origin of widget
        if (mouseMovePos && !dragInProcess && lock == QPoint()) {
            QPoint globalPos = event->globalPosition().toPoint();
            QPoint lastPos = mapToGlobal(*mouseMovePos);
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
    } else if (lock == QPoint()) {
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

void Tile::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        mouseMovePos = new QPoint(event->pos());
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

        if (lock != QPoint()) {
            tileLayout->changeTilesColor("resize");
        }
    } else {
        delete mouseMovePos;
        mouseMovePos = nullptr;
    }

    QWidget::mousePressEvent(event);
}

void Tile::mouseReleaseEvent(QMouseEvent *event) {
    if (lock == QPoint()) {
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

    emit tileMoved(
        widget,
        dropData["id"].toString(),
        tileLayout->objectName(),
        dropData["from_row"].toInt(),
        dropData["from_column"].toInt(),
        fromRow - dropData["row_offset"].toInt(),
        fromColumn - dropData["column_offset"].toInt()
        );
    event->acceptProposedAction();
}

QDrag *Tile::prepareDropData(QMouseEvent *event) {
    QDrag *drag = new QDrag(this);
    QMimeData *dropData = new QMimeData();
    QJsonObject data;
    data["id"] = tileLayout->objectName();
    data["from_row"] = fromRow;
    data["from_column"] = fromColumn;
    data["row_span"] = rowSpan;
    data["column_span"] = columnSpan;
    data["row_offset"] = event->pos().y() / (verticalSpan + tileLayout->verticalSpacing());
    data["column_offset"] = event->pos().x() / (horizontalSpan + tileLayout->horizontalSpacing());

    QByteArray dataBytes = QJsonDocument(data).toJson();
    dropData->setData("TileData", dataBytes);
    drag->setMimeData(dropData);
    drag->setHotSpot(event->pos() - rect().topLeft());
    return drag;
}

void Tile::dragAndDropProcess(QDrag *drag) {
    dragInProcess = true;
    int previousRowSpan = rowSpan;
    int previousColumnSpan = columnSpan;

    tileLayout->setWidgetToDrop(widget);
    widget->clearFocus();
    tileLayout->removeWidget(widget);
    setVisible(false);

    QMap<QUuid, QTileLayout *> mapTileLayout = tileLayout->getLinkedLayout();

    for (auto it = mapTileLayout.begin(); it != mapTileLayout.end(); ++it) {
        if (it.value()->getDragAndDrop()) {
            it.value()->changeTilesColor("drag_and_drop");
        }
    }

    if (drag->exec() != Qt::IgnoreAction) {
        removeWidget();
        QWidget *widgetToDrop = tileLayout->getWidgetToDrop();
        tileLayout->addWidget(
            widgetToDrop,
            fromRow,
            fromColumn,
            previousRowSpan,
            previousColumnSpan
            );
        if (tileLayout->getFocus()) {
            widgetToDrop->setFocus();
        }
    }

    originTileLayout = tileLayout;
    setVisible(true);
    dragInProcess = false;
}

bool Tile::isDropPossible(QDropEvent *event) {
    QByteArray tileData = event->mimeData()->data("TileData");
    QJsonParseError jsonError;
    QJsonObject dropData = QJsonDocument::fromJson(tileData, &jsonError).object();

    if (jsonError.error != QJsonParseError::NoError) {
        return false;
    }

    QMap<QUuid, QTileLayout *> mapTileLayout = tileLayout->getLinkedLayout();

    if (!mapTileLayout.contains(dropData["id"].toString())) {
        return false;
    } else {
        originTileLayout = mapTileLayout.value(dropData["id"].toString());
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

int Tile::getResizeTileNumber(int x, int y) {
    int dirX = lock.x();
    int dirY = lock.y();

    int span = horizontalSpan * (dirX != 0) + verticalSpan * (dirY != 0);
    int tileSpan = columnSpan * (dirX != 0) + rowSpan * (dirY != 0);
    int spacing = tileLayout->verticalSpacing() * (dirX != 0) + tileLayout->horizontalSpacing() * (dirY != 0);

    return static_cast<int>((x * (dirX != 0) + y * (dirY != 0) + (span / 2) - span * tileSpan * ((dirX + dirY) == 1)) / (span + spacing));
}

void Tile::removeWidget() {
    layout->removeWidget(widget);
    delete widget;
    widget = nullptr;
    filled = false;
}

void Tile::updateSizeLimit() {
    setFixedHeight(rowSpan * verticalSpan + (rowSpan - 1) * layout->spacing());
    setFixedWidth(columnSpan * horizontalSpan + (columnSpan - 1) * layout->spacing());
}

void Tile::tileHasBeenMoved(QWidget *widget, const QString &from_layout_id, const QString &to_layout_id, int from_row, int from_column, int to_row, int to_column) {
    qDebug() << widget << " has been moved from position (" << from_row << ", " << from_column << ") to ("
             << to_row << ", " << to_column << ")";
}
