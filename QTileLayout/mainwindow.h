#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QScrollArea>
#include <QLabel>
#include "QTileLayout.h"

namespace Ui {
class MainWindow;
}

class Label : public QLabel {
    Q_OBJECT

public:
    Label(QWidget *parent = nullptr);
    virtual ~Label();

protected:
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void tileHasBeenMoved(QWidget *widget, const QString &from_layout_id, const QString &to_layout_id,
                          int from_row, int from_column, int to_row, int to_column);
    void tileHasBeenResized(QWidget *widget, int from_row, int from_column, int row_span, int column_span);
    void centralWidgetResize(QResizeEvent *event);

private:
    QLabel* spawnWidget();
    QPalette spawnColor();
    // QWidget *central_widget;
    QTileLayout *tile_layout;
    QFont font;
    QScrollArea *scroll;
    Ui::MainWindow *ui;

};

#endif // MAINWINDOW_H
