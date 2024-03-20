#include "MainWindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QKeyEvent>
#include <QRandomGenerator>

static     QVector<QColor> possible_colors = {
    QColor(255, 153, 51),  // orange
    QColor(153, 0, 153),   // purple
    QColor(204, 204, 0),   // yellow
    QColor(51, 102, 204),  // blue
    QColor(0, 204, 102),   // green
    QColor(153, 102, 51),  // brown
    QColor(255, 51, 51)    // red
};

static     QVector<QString> possible_text = {
    "Hello",
    "Salut",
    "Hallo",
    "Hola",
    "Ciao",
    "Ola",
    "Hej",
    "Saluton",
    "Szia"
};

Label::Label(QWidget *parent) : QLabel(parent) {
    // Your constructor implementation
}

Label::~Label() {
    // Your destructor implementation
}

void Label::focusInEvent(QFocusEvent *event) {
    qDebug() << "WIDGET FOCUS IN EVENT" << this;
    QLabel::focusInEvent(event);
}

void Label::focusOutEvent(QFocusEvent *event) {
    qDebug() << "WIDGET FOCUS OUT EVENT" << this;
    QLabel::focusOutEvent(event);
}

void Label::keyPressEvent(QKeyEvent *ev) {
    qDebug() << "WIDGET KEY PRESSED" << this;
    QLabel::keyPressEvent(ev);
}


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {

    ui->setupUi(this);
    font = QFont("Latin", 15);
    font.setBold(true);

    int row_number = 6;
    int column_number = 4;
    int vertical_span = 100;
    int horizontal_span = 150;
    int spacing = 5;
    bool static_layout = true;

    tile_layout = new QTileLayout(row_number, column_number, vertical_span, horizontal_span, spacing, spacing, this);

    tile_layout->acceptDragAndDrop(true);
    tile_layout->acceptResizing(true);

    tile_layout->setCursorIdle(Qt::ArrowCursor);
    tile_layout->setCursorGrab(Qt::OpenHandCursor);
    tile_layout->setCursorResizeHorizontal(Qt::SizeHorCursor);
    tile_layout->setCursorResizeVertical(Qt::SizeVerCursor);

    tile_layout->setColorIdle(QColor(240, 240, 240));
    tile_layout->setColorResize(QColor(211, 211, 211));
    tile_layout->setColorDragAndDrop(QColor(211, 211, 211));
    tile_layout->setColorEmptyCheck(QColor(150, 150, 150));

    for (int i_row = 0; i_row < row_number - 2; ++i_row) {
        for (int i_column = 0; i_column < column_number; ++i_column) {
            QLabel* wdg = spawnWidget();
            tile_layout->addWidget(wdg, i_row, i_column);
        }
    }

    tile_layout->addWidget(spawnWidget(), row_number - 2, 1, 2, 2);

    QLabel* last_widget = spawnWidget();
    tile_layout->addWidget(last_widget, row_number - 1, 0, 1, 1);
    tile_layout->removeWidget(last_widget);

    qDebug() << "Row number:" << tile_layout->rowCount();
    qDebug() << "Column number:" << tile_layout->columnCount();
    qDebug() << "One tile geometry:" << tile_layout->tileRect(row_number - 1, 1);
    qDebug() << "Tile height:" << tile_layout->rowsMinimumHeight();
    qDebug() << "Tile width:" << tile_layout->columnsMinimumWidth();
    qDebug() << "Layout vertical spacing:" << tile_layout->verticalSpacing();
    qDebug() << "Layout horizontal spacing:" << tile_layout->horizontalSpacing();
    qDebug() << "Number of widget:" << tile_layout->widgetList().size();

    tile_layout->setVerticalSpacing(5);
    tile_layout->setHorizontalSpacing(5);
    tile_layout->setRowsMinimumHeight(150);
    tile_layout->setColumnsMinimumWidth(150);
    tile_layout->setRowsHeight(100);
    tile_layout->setColumnsWidth(150);

    tile_layout->activateFocus(false);

    connect(tile_layout, &QTileLayout::tileResized, this, &MainWindow::tileHasBeenResized);
    connect(tile_layout, &QTileLayout::tileMoved, this, &MainWindow::tileHasBeenMoved);

    tile_layout->addRows(1);
    tile_layout->addColumns(1);
    tile_layout->removeRows(1);
    // tile_layout->removeColumns(1);

    ui->centralwidget = new QWidget;
    ui->centralwidget->setContentsMargins(0, 0, 0, 0);
    ui->centralwidget->setLayout(tile_layout);

    if (static_layout) {
        setCentralWidget(ui->centralwidget);
    } else {
        scroll = new QScrollArea;
        scroll->setWidgetResizable(true);
        scroll->setContentsMargins(0, 0, 0, 0);
        scroll->setWidget(ui->centralwidget);
        setCentralWidget(scroll);
        //connect(scroll, &QScrollArea::resizeEvent, this, &MainWindow::centralWidgetResize);

        int vertical_margins = tile_layout->contentsMargins().top() + tile_layout->contentsMargins().bottom();
        int horizontal_margins = tile_layout->contentsMargins().left() + tile_layout->contentsMargins().right();
        scroll->setMinimumHeight(row_number * vertical_span + (row_number - 1) * spacing + vertical_margins + 2);
        scroll->setMinimumWidth(column_number * horizontal_span + (column_number - 1) * spacing + horizontal_margins + 2);
    }

}

void MainWindow::tileHasBeenMoved(QWidget *widget, const QString &from_layout_id, const QString &to_layout_id,
                                  int from_row, int from_column, int to_row, int to_column) {
    qDebug() << widget << " has been moved from position (" << from_row << ", " << from_column << ") to ("
             << to_row << ", " << to_column << ")";
}

void MainWindow::tileHasBeenResized(QWidget *widget, int from_row, int from_column, int row_span, int column_span) {
    qDebug() << widget << " has been resized and is now at the position (" << from_row << ", " << from_column << ") "
             << "with a span of (" << row_span << ", " << column_span << ")";
}

void MainWindow::centralWidgetResize(QResizeEvent *event) {
    tile_layout->updateGlobalSize(event);
}

QLabel* MainWindow::spawnWidget() {
    Label *label = new Label(this);
    label->setText(QString(possible_text[QRandomGenerator::global()->bounded(possible_text.size())]));
    label->setObjectName(label->text());
    label->setFont(font);
    label->setAlignment(Qt::AlignCenter);
    label->setAutoFillBackground(true);
    label->setPalette(spawnColor());
    return label;
}

QPalette MainWindow::spawnColor() {
    QPalette palette;
    QColor color = possible_colors[QRandomGenerator::global()->bounded(possible_colors.size())];
    palette.setColor(QPalette::Window, color);
    return palette;
}
