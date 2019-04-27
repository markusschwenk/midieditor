#ifndef COLOREDWIDGET_H
#define COLOREDWIDGET_H

#include <QWidget>
#include <QColor>

class ColoredWidget : public QWidget {

public:
    ColoredWidget(QColor color, QWidget* parent = 0);
    void setColor(QColor c)
    {
        _color = c;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event);

private:
    QColor _color;
};

#endif // COLOREDWIDGET_H
