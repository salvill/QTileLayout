#ifndef TILE_H
#define TILE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QMimeData>
#include <QByteArray>
#include <QDrag>
#include <QMouseEvent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QtWidgets>
#include <QtWidgets/qlistview.h>

class QTileLayout;

class Tile : public QWidget {
    Q_OBJECT

public:
    explicit Tile(QTileLayout *tileLayout, int fromRow, int fromColumn, int rowSpan, int columnSpan, int verticalSpan, int horizontalSpan, QWidget *parent = nullptr);

    void updateSize(int fromRow = -1, int fromColumn = -1, int rowSpan = -1, int columnSpan = -1, int verticalSpan = -1, int horizontalSpan = -1);
    void addWidget(QWidget *widget);
    int getFromRow() const;
    int getFromColumn() const;
    int getRowSpan() const;
    int getColumnSpan() const;
    QString getInfo();
    bool isFilled() const;
    void changeColor(const QPalette &color);
    void releaseWidget();

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    QTileLayout *tileLayout;
    QTileLayout *originTileLayout;
    int fromRow;
    int fromColumn;
    int rowSpan;
    int columnSpan;
    int verticalSpan;
    int horizontalSpan;
    int resizeMargin;
    bool filled;
    QWidget *widget;
    QPoint mouseMovePos;
    QVBoxLayout *layout;
    QPoint lock;
    bool dragInProcess;
    int currentTileNumber;

    // Other member variables and functions...

    QDrag *prepareDropData(QMouseEvent *event);
    void dragAndDropProcess(QDrag *drag);
    void removeWidget();
    int getResizeTileNumber(int x, int y);
    bool isDropPossible(QDropEvent *event);
    void updateSizeLimit();

signals:
    void tileMoved(QWidget *widget, const QString &from_layout_id, const QString &to_layout_id,
                          int from_row, int from_column, int to_row, int to_column);
public slots:
    void tileHasBeenMoved(QWidget *widget, const QString &from_layout_id, const QString &to_layout_id,
                          int from_row, int from_column, int to_row, int to_column);
};

#endif // TILE_H
