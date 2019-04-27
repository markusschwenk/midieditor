#include "ColoredWidget.h"

#include <QPainter>
#include <QPaintEvent>

ColoredWidget::ColoredWidget(QColor color, QWidget* parent)
    : QWidget(parent)
{
    _color = color;
    setFixedWidth(30);
    setContentsMargins(0, 0, 0, 0);
}

void ColoredWidget::paintEvent(QPaintEvent* event)
{
    QPainter p;
    int l = width() - 1;
    int x = 0;
    int y = (height() - 1 - l) / 2;
    if (l > height() - 1) {
        l = height() - 1;
        y = 0;
        x = (width() - 1 - l) / 2;
    }

    p.begin(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(0, 0, width(), height(), Qt::white);
    p.setPen(Qt::lightGray);
    p.setBrush(_color);
    p.drawRoundRect(x, y, l, l, 30, 30);
    p.end();
}
