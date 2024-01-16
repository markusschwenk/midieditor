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

void ColoredWidget::mouseDoubleClickEvent(QMouseEvent */*event*/)
{
    emit doubleClick();

}

void ColoredWidget::paintEvent(QPaintEvent*)
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
#ifdef CUSTOM_MIDIEDITOR_GUI
    // Estwald Color Changes
    p.fillRect(0, 0, width(), height(), Qt::transparent /*QColor(0xe0e0d0)*/);
#else
    p.fillRect(0, 0, width(), height(), Qt::white);
#endif
    p.setPen(Qt::lightGray);
    p.setBrush(_color);

    p.drawRoundedRect(x, y, l, l, 30, 30, Qt::RelativeSize);

    if(1) {

        QLinearGradient linearGrad(QPointF(x, y), QPointF(x + l, y + l));
        linearGrad.setColorAt(0, QColor(60, 60, 60, 0x40));
        linearGrad.setColorAt(0.5, QColor(0xcf, 0xcf, 0xcf, 0x70));
        linearGrad.setColorAt(1.0, QColor(0xff, 0xff, 0xff, 0x70));

        QBrush d(linearGrad);
        p.setBrush(d);
        p.drawRoundedRect(x, y, l, l, 30, 30, Qt::RelativeSize);
    }
    p.end();
}
