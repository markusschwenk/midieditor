#ifndef COLOREDWIDGET_H
#define COLOREDWIDGET_H

#include <QWidget>
#include <QColor>

class ColoredWidget : public QWidget {

    Q_OBJECT

public:
    ColoredWidget(QColor color, QWidget* parent = 0);
    void setColor(QColor c)
    {
        _color = c;
        update();
    }

signals:
    void doubleClick();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QColor _color;
};

#endif // COLOREDWIDGET_H
