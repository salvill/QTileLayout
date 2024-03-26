 #include "qtilelayout.h"
#include "qdebug.h"

QTileLayout::QTileLayout(int rowNumber, int columnNumber, int verticalSpan, int horizontalSpan,
                         int verticalSpacing, int horizontalSpacing, QWidget *parent)
    : QGridLayout(parent), rowNumber(rowNumber), columnNumber(columnNumber),
    verticalSpan(verticalSpan), horizontalSpan(horizontalSpan),
    minVerticalSpan(verticalSpan), minHorizontalSpan(horizontalSpan),
    dragAndDrop(true), resizable(true), focus(false), widgetToDrop(nullptr)
{
    // Set spacing
    QGridLayout::setVerticalSpacing(verticalSpacing);
    QGridLayout::setHorizontalSpacing(horizontalSpacing);

    // Logic parameters
    dragAndDrop = true;
    resizable = true;
    focus = false;
    widgetToDrop = nullptr;

    tileMap = {};
    widgetTileCouple = {
        { "widget", QList<QWidget *>() },
        { "tile"  , QList<QWidget *>() }
    };

    id = QUuid::createUuid();
    // id = QUuid::createUuid().toString();
    linkedLayout[id] = this;

    // Design parameters
    cursorIdle = Qt::ArrowCursor;
    cursorGrab = Qt::OpenHandCursor;
    cursorResizeHorizontal = Qt::SizeHorCursor;
    cursorResizeVertical = Qt::SizeVerCursor;

    colorMap = {
        {"drag_and_drop",   QColor(255, 211, 211)},
        {"idle",            QColor(240, 240, 240)},
        {"resize",          QColor(211, 255, 211)},
        {"empty_check",     QColor(150, 150, 150)}
    };

    setRowStretch(rowNumber, 1);
    setColumnStretch(columnNumber, 1);

    createTileMap();

}

//adds a widget in the layout: works like the addWidget method in a gridLayout
void QTileLayout::addWidget(QWidget *widget, int fromRow, int fromColumn, int rowSpan, int columnSpan)
{
    Q_ASSERT(!widgetList().contains(widget));
    Q_ASSERT(isAreaEmpty(fromRow, fromColumn, rowSpan, columnSpan));
    // if( !widgetTileCouple["widget"].contains(widget)
    //     && isAreaEmpty(fromRow, fromColumn, rowSpan, columnSpan))
    {
        // Gets the tile at the specified position
        Tile* tile = tileMap[fromRow][fromColumn];
        // Adds the widget and tile to the widgetTileCouple
        widgetTileCouple["widget"].append(widget);
        widgetTileCouple["tile"].append(tile);

        widgetToDrop = widget;
        widget->setMouseTracking(true);
        tile->addWidget(widget);

        // If the widget is on more than 1 tile, the tiles must be merged
        if (rowSpan > 1 || columnSpan > 1) {
            QList<QPoint> tilesToMerge;
            for (int row = 0; row < rowSpan; ++row)
                for (int column = 0; column < columnSpan; ++column)
                    tilesToMerge.append(QPoint(fromRow + row, fromColumn + column));

            mergeTiles(tile, fromRow, fromColumn, rowSpan, columnSpan, tilesToMerge.mid(1));
        }
        // qDebug() << "Widget created: " << widget->objectName() << "row: "<< fromRow << "col: " <<fromColumn;
    }


}

// Removes the given widget
void QTileLayout::removeWidget(QWidget *widget) {
    // Q_ASSERT(widgetList().contains(widget));
    if (widgetList().contains(widget))
    {
        int index = widgetTileCouple["widget"].indexOf(widget);
        Tile *tile = dynamic_cast<Tile*>(widgetTileCouple.value("tile").at(index));

        int fromRow = tile->getFromRow();
        int fromColumn = tile->getFromColumn();
        int rowSpan = tile->getRowSpan();
        int columnSpan = tile->getColumnSpan();

        QList<QPoint> tilesToSplit;
        for (int row = 0; row < rowSpan; ++row)
            for (int column = 0; column < columnSpan; ++column)
                tilesToSplit.append(QPoint(fromRow + row, fromColumn + column));

        widget->setMouseTracking(false);
        splitTiles(tile, fromRow, fromColumn, rowSpan, columnSpan, tilesToSplit);
        hardSplitTiles(fromRow, fromColumn, tilesToSplit);
        widgetTileCouple["widget"].removeAt(index);
        widgetTileCouple["tile"].removeAt(index);
        changeTilesColor("idle");

        qDebug() << "Widget removed: " << widget->objectName() << "row: "<< fromRow << "col: " <<fromColumn;
    }

}

void QTileLayout::addRows(int rowNumber) {
    // Q_ASSERT(rowNumber > 0);
    if(rowNumber > 0)
    {
        setRowStretch(this->rowNumber, 0);

        for (int row = this->rowNumber; row < this->rowNumber + rowNumber; ++row) {
            tileMap.append(QList<Tile*>());
            for (int column = 0; column < columnNumber; ++column) {
                Tile *tile = createTile(row, column);
                tileMap.last().append(tile);
            }
        }

        this->rowNumber += rowNumber;
        setRowStretch(this->rowNumber, 1);
    }
}

// Adds columns at the right of the layout
void QTileLayout::addColumns(int columnNumber) {
    // Q_ASSERT(columnNumber > 0);
    if(columnNumber > 0)
    {
        setColumnStretch(this->columnNumber, 0);

        for (int row = 0; row < rowNumber; ++row) {
            for (int column = this->columnNumber; column < this->columnNumber + columnNumber; ++column) {
                Tile *tile = createTile(row, column);
                tileMap[row].append(tile);
            }
        }

        this->columnNumber += columnNumber;
        setColumnStretch(this->columnNumber, 1);
        QGridLayout::update();
        qDebug() << "Added column";
    }
}

void QTileLayout::removeRows(int rowNumber) {
    // Q_ASSERT(isAreaEmpty(this->rowNumber - rowNumber, 0, rowNumber, columnNumber));
    if (isAreaEmpty(this->rowNumber - rowNumber, 0, rowNumber, this->columnNumber))
    {
        for (int row = this->rowNumber - rowNumber; row < this->rowNumber; ++row)
        {
            for (int column = 0; column < columnNumber; ++column) {
                QGridLayout::removeWidget(tileMap[row][column]);
                tileMap[row][column]->deleteLater();
            }
            setRowMinimumHeight(row, 0);
            setRowStretch(row, 0);
        }

        this->rowNumber -= rowNumber;
        tileMap.erase(tileMap.begin() + this->rowNumber, tileMap.end());
    }

}

void QTileLayout::removeColumns(int columnNumber) {
    // Q_ASSERT(isAreaEmpty(0, this->columnNumber - columnNumber, rowNumber, columnNumber));
    if (isAreaEmpty(0, this->columnNumber - columnNumber, this->rowNumber, columnNumber))
    {
        for (int column = this->columnNumber - columnNumber; column < this->columnNumber; ++column) {
            for (int row = 0; row < rowNumber; ++row) {
                QGridLayout::removeWidget(tileMap[row][column]);
                tileMap[row][column]->deleteLater();
            }

            setColumnMinimumWidth(column, 0);
            setColumnStretch(column, 0);
        }

        this->columnNumber -= columnNumber;
        for (int row = 0; row < rowNumber; ++row) {
            tileMap[row].erase(tileMap[row].begin() + this->columnNumber, tileMap[row].end());
        }
    }

}

void QTileLayout::acceptDragAndDrop(bool value) {
    dragAndDrop = value;
}

void QTileLayout::acceptResizing(bool value) {
    resizable = value;
}

void QTileLayout::setCursorIdle(Qt::CursorShape value) {
    cursorIdle = value;
}

void QTileLayout::setCursorGrab(Qt::CursorShape value) {
    cursorGrab = value;
}

void QTileLayout::setCursorResizeHorizontal(Qt::CursorShape value) {
    cursorResizeHorizontal = value;
}

void QTileLayout::setCursorResizeVertical(Qt::CursorShape value) {
    cursorResizeVertical = value;
}

void QTileLayout::setColorIdle(QColor color) {
    colorMap["idle"] = color;
    changeTilesColor("idle");
}

void QTileLayout::setColorResize(QColor color) {
    colorMap["resize"] = color;
}

void QTileLayout::setColorDragAndDrop(QColor color) {
    colorMap["drag_and_drop"] = color;
}

void QTileLayout::setColorEmptyCheck(QColor color) {
    colorMap["empty_check"] = color;
}

int QTileLayout::rowCount() const {
    return rowNumber;
}

int QTileLayout::columnCount() const {
    return columnNumber;
}

QRect QTileLayout::tileRect(int row, int column) const {
    return tileMap[row][column]->rect();
}

int QTileLayout::rowsMinimumHeight() const {
    return minVerticalSpan;
}

int QTileLayout::columnsMinimumWidth() const {
    return minHorizontalSpan;
}

void QTileLayout::setRowsMinimumHeight(int height) {
    minVerticalSpan = height;
    if (minVerticalSpan > verticalSpan) {
        verticalSpan = minVerticalSpan;
        updateAllTiles();
    }
}

void QTileLayout::setColumnsMinimumWidth(int width)
{
    minHorizontalSpan = width;
    if (minHorizontalSpan > horizontalSpan) {
        horizontalSpan = minHorizontalSpan;
        updateAllTiles();
    }
}

void QTileLayout::setRowsHeight(int height)
{
    // Q_ASSERT(minVerticalSpan <= height);
    if (minVerticalSpan <= height)
    {
        verticalSpan = height;
        updateAllTiles();
    }
}

void QTileLayout::setColumnsWidth(int width)
{
    // Q_ASSERT(minHorizontalSpan <= width);
    if (minHorizontalSpan <= width)
    {
        horizontalSpan = width;
        updateAllTiles();
    }
}

void QTileLayout::setVerticalSpacing(int spacing)
{
    QGridLayout::setVerticalSpacing(spacing);
    updateAllTiles();
}

void QTileLayout::setHorizontalSpacing(int spacing)
{
    QGridLayout::setHorizontalSpacing(spacing);
    updateAllTiles();
}

QString QTileLayout::getId() const
{
    return id.toString();
}

void QTileLayout::activateFocus(bool focus)
{
    this->focus = focus;
}

QList<QWidget*> QTileLayout::widgetList() const
{
    return widgetTileCouple["widget"];
}

void QTileLayout::linkLayout(QTileLayout *layout)
{
    // Q_ASSERT(layout != nullptr);
    // Q_ASSERT(layout->id != id);
    if (layout != nullptr && layout->id != id)
    {
        linkedLayout.insert(layout->id, layout);
        layout->linkedLayout.insert(id, this);
    }

}

void QTileLayout::unLinkLayout(QTileLayout *layout)
{
    // Q_ASSERT(layout != nullptr);
    // Q_ASSERT(layout->id != id);
    if (layout != nullptr && layout->id != id)
    {
        linkedLayout.remove(layout->id);
        layout->linkedLayout.remove(id);
    }
}

void QTileLayout::highlightTiles(QPoint direction, int fromRow, int fromColumn, int tileNumber)
{

    Tile *tile = tileMap[fromRow][fromColumn];
    QList<QPoint> tilesToMerge;
    bool increase;
    int rowSpan, columnSpan;
    std::tie(tilesToMerge, increase, fromRow, fromColumn, rowSpan, columnSpan) = getTilesToBeResized(
        tile, direction, fromRow, fromColumn, tileNumber
        );

    if (!tilesToMerge.isEmpty()) {
        changeTilesColor("empty_check", QPoint(fromRow, fromColumn), QPoint(rowSpan, columnSpan));
    }
}

void QTileLayout::resizeTile(QPoint direction, int fromRow, int fromColumn, int tileNumber)
{

    Tile *tile = tileMap[fromRow][fromColumn];
    QList<QPoint> tilesToMerge;
    bool increase;
    int rowSpan, columnSpan;
    std::tie(tilesToMerge, increase, fromRow, fromColumn, rowSpan, columnSpan) = getTilesToBeResized(
        tile, direction, fromRow, fromColumn, tileNumber
        );

    if (!tilesToMerge.isEmpty()) {
        if (increase) {
            mergeTiles(tile, fromRow, fromColumn, rowSpan, columnSpan, tilesToMerge);
        } else {
            splitTiles(tile, fromRow, fromColumn, rowSpan, columnSpan, tilesToMerge);
        }
        int index = widgetTileCouple["tile"].indexOf(tile);
        QWidget *widget = widgetTileCouple["widget"].at(index);
        emit tileResized(widget, fromRow, fromColumn, rowSpan, columnSpan);
    }
}

// Splits the tiles and return the new one at (fromRow, fromColumn)
Tile* QTileLayout::hardSplitTiles(int fromRow, int fromColumn, QList<QPoint> tilesToSplit) {
    Q_ASSERT(tilesToSplit.contains(QPoint(fromRow, fromColumn)));
    // if (tilesToSplit.contains(QPoint(fromRow, fromColumn)))
    {
        QSet<Tile*> tilesToRecycle = {};

        for (const QPoint &point : qAsConst(tilesToSplit)) {
            tilesToRecycle.insert(tileMap[point.x()][point.y()]);
            createTile(point.x(), point.y(), 1, 1, true);
        }

        for (Tile *tile : qAsConst(tilesToRecycle)) {
            QGridLayout::removeWidget(tile);
            tile->deleteLater();
        }

        Tile *tile = tileMap[fromRow][fromColumn];
        QGridLayout::addWidget(tile, fromRow, fromColumn);
        return tile;
    }
}

// Checks if the given space is free from widgets
bool QTileLayout::isAreaEmpty(int fromRow, int fromColumn, int rowSpan, int columnSpan, QString color) {
    bool isEmpty = true;

    if (!color.isEmpty() && colorMap.contains(color)) {
        changeTilesColor(color);
    }

    if ((fromRow + rowSpan > rowNumber) || (fromColumn + columnSpan > columnNumber) || (fromRow < 0) || (fromColumn < 0)) {
        isEmpty = false;
    }
    else
    {
        for (int row = 0; row < rowSpan; ++row) {
            for (int column = 0; column < columnSpan; ++column) {
                if (tileMap[fromRow + row][fromColumn + column]->isFilled()) {
                    isEmpty = false;
                    break;
                }
            }
            if (!isEmpty) {
                break;
            }
        }
    }

    if (isEmpty && !color.isEmpty() && colorMap.contains(color)) {
        changeTilesColor("empty_check", QPoint(fromRow, fromColumn), QPoint(rowSpan, columnSpan));
    }

    return isEmpty;
}

QWidget* QTileLayout::getWidgetToDrop() {
    QWidget *widget = widgetToDrop;
    widgetToDrop = nullptr;
    return widget;
}

void QTileLayout::setWidgetToDrop(QWidget *widget)
{

    widgetToDrop = widget;
}

// Changes the color of all tiles
void QTileLayout::changeTilesColor(QString colorChoice, QPoint fromTile, QPoint toTile) {
    QPalette * palette = new QPalette();
    palette->setColor(QPalette::Window, colorMap.value(colorChoice));
    QPalette * paletteIdle = new QPalette();
    paletteIdle->setColor(QPalette::Window, colorMap.value("idle"));
    QPoint toTileAdjusted = toTile.isNull() ? QPoint(rowNumber, columnNumber) : toTile;

    for (int row = fromTile.x(); row < fromTile.x() + toTileAdjusted.x(); ++row)
    {
        for (int column = fromTile.y(); column < fromTile.y() + toTileAdjusted.y(); ++column)
        {
            if (!tileMap[row][column]->isFilled())
            {
                tileMap[row][column]->changeColor(*palette);
            }
            else
            {
                tileMap[row][column]->changeColor(*paletteIdle);
            }
        }
    }
}

// returns a 1D list given a 2D list
QList<QVariant> QTileLayout::flattenList(const QList<QList<QVariant>>& toFlatten) {
    QList<QVariant> flattenedList;
    for (const auto& aList : toFlatten) {
        for (const auto& item : aList) {
            flattenedList.append(item);
        }
    }
    return flattenedList;
}

// Update the size of the layout
void QTileLayout::updateGlobalSize(QResizeEvent *newSize)
{

    int verticalMargins = contentsMargins().top() + contentsMargins().bottom();
    int horizontalMargins = contentsMargins().left() + contentsMargins().right();

    verticalSpan = qMax(
        minVerticalSpan,
        static_cast<int>((newSize->size().height() - (rowNumber - 1) * verticalSpacing() - verticalMargins) / rowNumber)
        );

    horizontalSpan = qMax(
        minHorizontalSpan,
        static_cast<int>((newSize->size().width() - (columnNumber - 1) * horizontalSpacing() - horizontalMargins) / columnNumber)
        );

    updateAllTiles();
}

// Merges the tilesToMerge with tile
void QTileLayout::mergeTiles(Tile *tile, int fromRow, int fromColumn, int rowSpan, int columnSpan, QList<QPoint> tilesToMerge)
{

    for (const QPoint &point : std::as_const(tilesToMerge)) {
        // if (tileMap[point.x()][point.y()])
        {
            QGridLayout::removeWidget(tileMap[point.x()][point.y()]);
            delete tileMap[point.x()][point.y()];
        }
        tileMap[point.x()][point.y()] = tile;
    }

    QGridLayout::removeWidget(tile);
    QGridLayout::addWidget(tile, fromRow, fromColumn, rowSpan, columnSpan);
    tile->updateSize(fromRow, fromColumn, rowSpan, columnSpan);
}

// Splits the tilesToSplit from tile
void QTileLayout::splitTiles(Tile *tile, int fromRow, int fromColumn, int rowSpan, int columnSpan, QList<QPoint> tilesToSplit) {

    for (const QPoint &point : std::as_const(tilesToSplit)) {
        createTile(point.x(), point.y(), 1, 1,  true);
    }

    QGridLayout::removeWidget(tile);
    QGridLayout::addWidget(tile, fromRow, fromColumn, rowSpan, columnSpan);
    tile->updateSize(fromRow, fromColumn, rowSpan, columnSpan);
}

// Creates a tile: a tile is basically a place holder that can contain a widget or not
Tile* QTileLayout::createTile(int fromRow, int fromColumn, int rowSpan, int columnSpan, bool updateTileMap) {

    Tile *tile = new Tile(
        this,
        fromRow,
        fromColumn,
        rowSpan,
        columnSpan,
        verticalSpan,
        horizontalSpan
        );

    QGridLayout::addWidget(tile, fromRow, fromColumn, rowSpan, columnSpan);

    if (updateTileMap) {
        QVector<QPoint> tilePositions;
        for (int row = 0; row < rowSpan; ++row) {
            for (int column = 0; column < columnSpan; ++column) {
                tilePositions.append(QPoint(fromRow + row, fromColumn + column));
            }
        }

        for (const auto& position : tilePositions) {
            int row = position.x();
            int column = position.y();
            tileMap[row][column] = tile;
        }
    }

    // connect(tile, &Tile::doubleClicked, this, &QTileLayout::tileDoubleClicked);
    // connect(tile, &Tile::clicked, this, &QTileLayout::tileClicked);
    // connect(tile, &Tile::dragged, this, &QTileLayout::tileDragged);
    // connect(tile, &Tile::dragEnded, this, &QTileLayout::tileDragEnded);
    // connect(tile, &Tile::resized, this, &QTileLayout::tileResized);

    return tile;
}

std::tuple<QList<QPoint>, bool, int, int, int, int> QTileLayout::getTilesToBeResized(Tile *tile, QPoint direction, int fromRow, int fromColumn, int tileNumber) {
    // Recovers the tiles that will be merged or split during resizing
    int rowSpan = tile->getRowSpan();
    int columnSpan = tile->getColumnSpan();
    bool increase = true;
    int dirX = direction.x();
    int dirY = direction.y();
    QList<QPoint> tilesToMerge;
    int newTileNumber; // Nuova variabile per memorizzare tileNumber

    // Increase tile size
    if (tileNumber * (dirX + dirY) > 0) {
        auto res = getTilesToMerge(direction, fromRow, fromColumn, tileNumber);
        newTileNumber = std::get<0>(res); // Assegna il valore di tileNumber dalla tupla restituita
        tilesToMerge = std::get<1>(res);
    } else { // Decrease tile size
        std::tie(newTileNumber, tilesToMerge) = getTilesToSplit(direction, fromRow, fromColumn, tileNumber);
        increase = false;
    }

    columnSpan += newTileNumber * dirX; // Usa newTileNumber
    fromColumn += newTileNumber * (dirX == -1); // Usa newTileNumber
    rowSpan += newTileNumber * dirY; // Usa newTileNumber
    fromRow += newTileNumber * (dirY == -1); // Usa newTileNumber

    return std::make_tuple(tilesToMerge, increase, fromRow, fromColumn, rowSpan, columnSpan);
}

// Finds the tiles to split when a tile is decreased
std::tuple<int, QList<QPoint> > QTileLayout::getTilesToSplit(QPoint direction, int fromRow, int fromColumn, int tileNumber) {
    Tile* tile = tileMap[fromRow][fromColumn];
    int rowSpan = tile->getRowSpan();
    int columnSpan = tile->getColumnSpan();
    int dirX = direction.x();
    int dirY = direction.y();

    int newTileNumber = (tileNumber <= (-tileNumber * (dirX + dirY)) < (columnSpan * (dirX != 0)) + (rowSpan * (dirY != 0))) ?
                            tileNumber : ((1 - columnSpan) * dirX) + ((1 - rowSpan) * dirY);

    QList<QPoint> tilesToMerge;
    for (int row = (-newTileNumber * dirY) + (rowSpan * (dirX != 0)); row > 0; --row) {
        for (int column = (-newTileNumber * dirX) + (columnSpan * (dirY != 0)); column > 0; --column) {
            tilesToMerge.append(QPoint(fromRow + row + (rowSpan - 2 * row - 1) * (dirY == 1),
                                       fromColumn + column + (columnSpan - 2 * column - 1) * (dirX == 1)));
        }
    }

    return std::make_tuple(newTileNumber, tilesToMerge);
}

// Finds the tiles to merge when a tile is increased
std::tuple<int, QList<QPoint>> QTileLayout::getTilesToMerge(QPoint direction, int fromRow, int fromColumn, int tileNumber) {
    Tile* tile = tileMap[fromRow][fromColumn];
    int rowSpan = tile->getRowSpan();
    int columnSpan = tile->getColumnSpan();
    int tileNumberAvailable = 0;
    QList<QPoint> tilesToMerge;
    int dirX = direction.x();
    int dirY = direction.y();

    if (dirX + dirY == -1) {
        tileNumber = qMax(tileNumber, -fromColumn * (dirX != 0) - fromRow * (dirY != 0));
    } else {
        int minTileNumber = ((columnNumber - fromColumn - columnSpan) * (dirX != 0) +
                             (rowNumber - fromRow - rowSpan) * (dirY != 0));
        tileNumber = qMin(tileNumber, minTileNumber);
    }

    // Ovest o Est
    if (dirX != 0) {
        for (int column = 0; column < tileNumber * dirX; ++column) {
            QList<QPoint> tilesToCheck;
            int columnDelta = (columnSpan + column) * (dirX == 1) + (-column - 1) * (dirX == -1);
            for (int row = 0; row < rowSpan; ++row) {
                tilesToCheck.append(QPoint(fromRow + row, fromColumn + columnDelta));
                if (tileMap[fromRow + row][fromColumn + columnDelta]->isFilled()) {
                    return std::make_tuple(tileNumberAvailable, tilesToMerge);
                }
            }
            tileNumberAvailable += dirX;
            tilesToMerge.append(tilesToCheck);
        }
    }
    // Nord o Sud
    else {
        for (int row = 0; row < tileNumber * dirY; ++row) {
            QList<QPoint> tilesToCheck;
            int rowDelta = (rowSpan + row) * (dirY == 1) + (-row - 1) * (dirY == -1);
            for (int column = 0; column < columnSpan; ++column) {
                tilesToCheck.append(QPoint(fromRow + rowDelta, fromColumn + column));
                if (tileMap[fromRow + rowDelta][fromColumn + column]->isFilled()) {
                    return std::make_tuple(tileNumberAvailable, tilesToMerge);
                }
            }
            tileNumberAvailable += dirY;
            tilesToMerge.append(tilesToCheck);
        }
    }
    return std::make_tuple(tileNumberAvailable, tilesToMerge);
}


// Creates a map to be able to locate each tile on the grid
void QTileLayout::createTileMap() {
    for (int row = 0; row < rowNumber; ++row) {
        QList<Tile*> rowTiles;
        for (int column = 0; column < columnNumber; ++column) {
            Tile* tile = createTile(row, column);
            rowTiles.push_back(tile);
        }
        tileMap.push_back(rowTiles);
    }
}

// Forces the tiles to update their geometry
void QTileLayout::updateAllTiles() {
    if (!tileMap.isEmpty()) {
        for (int row = 0; row < rowNumber; ++row) {
            for (int column = 0; column < columnNumber; ++column) {
                tileMap[row][column]->updateSize(row, column, -1, -1, verticalSpan, horizontalSpan);
            }
        }
    }
}

QMap<QUuid, QTileLayout *> QTileLayout::getLinkedLayout() const {
    return linkedLayout;
}

Qt::CursorShape QTileLayout::getCursorResizeVertical() const {
    return cursorResizeVertical;
}

Qt::CursorShape QTileLayout::getCursorResizeHorizontal() const {
    return cursorResizeHorizontal;
}

Qt::CursorShape QTileLayout::getCursorGrab() const {
    return cursorGrab;
}

Qt::CursorShape QTileLayout::getCursorIdle() const {
    return cursorIdle;
}

bool QTileLayout::getFocus() const {
    return focus;
}

bool QTileLayout::getResizable() const {
    return resizable;
}

bool QTileLayout::getDragAndDrop() const {
    return dragAndDrop;
}



// void QTileLayout::setTileVisible(bool visible)
// {
//     for (int row = 0; row < rowNumber; ++row) {
//         for (int column = 0; column < columnNumber; ++column) {
//             tileMap[row][column]->setVisible(visible);
//         }
//     }
// }

// QList<QTileLayout*> QTileLayout::getLinkedLayouts() const
// {
//     return linkedLayout.values();
// }

// QList<Tile*> QTileLayout::getTileList() const
// {
//     QList<Tile*> tiles;
//     for (int row = 0; row < rowNumber; ++row) {
//         for (int column = 0; column < columnNumber; ++column) {
//             tiles.append(tileMap[row][column]);
//         }
//     }
//     return tiles;
// }
