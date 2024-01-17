#ifndef GUITOOLS_H
#define GUITOOLS_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QDial>
#include <QSettings>
#include <QTimer>
#include <QPainter>
#include <QListWidget>
#include <QMouseEvent>

#define MAX_TOOLTIP_TIME 5000
#define TOOLTIP(c, t) {c->setToolTip(t); c->setToolTipDuration(MAX_TOOLTIP_TIME);}
#define TOOLTIP2(c, t, time) {c->setToolTip(t); c->setToolTipDuration(time * 1000);}

void msDelay(int ms);

class _QPushButtonE2 : public QPushButton {

    Q_OBJECT

public:

    bool is_enter;

    _QPushButtonE2(QWidget* parent) : QPushButton(parent) {

        is_enter = false;
    }

protected:

    bool event(QEvent *event) override;


};

class QDialE : public QDial {

    Q_OBJECT

public:

    _QPushButtonE2 *but1;
    _QPushButtonE2 *but2;
    QTimer *time_update;

    bool is_enter;

    QRect _r;

    QDialE(QWidget* parent);
    ~QDialE();

    void setGeometry(const QRect& r);
    void setVisible(bool visible)  override;

signals:
    void mouseRightButton();

protected:

    void mousePressEvent(QMouseEvent* event) override;
    bool event(QEvent *event) override;

};

class QPushButtonE : public QPushButton {

    Q_OBJECT

public:

    QPushButtonE(QWidget* parent, int mode = 0) : QPushButton(parent) {
        _mode = mode;
    }

signals:
    void mouseRightButton();

protected:

    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:

    int _mode;

};

class QWidgetE : public QWidget {

    Q_OBJECT

public:

    bool logo;

    QWidgetE(QWidget* parent, bool logo = false) : QWidget(parent) {
        this->logo = logo;
    }

protected:

    void paintEvent(QPaintEvent* event) override;

};

class QLedBoxE : public QWidget {

    Q_OBJECT

public:

    QLedBoxE(QWidget* parent) : QWidget(parent) {
        led = QColor(0x00, 0x40, 0x10);
    }

    void setLed(QColor color) {
        led = color;
        update();
    }

protected:

    QColor led;
    void paintEvent(QPaintEvent* event) override;

};

class QPedalE : public QGroupBox {

    Q_OBJECT

public:

    QPedalE(QWidget* parent, QString title);


public slots:

    void setVal(int val, bool setcheck = false);

signals:

    void isChecked(bool checked);

protected:

    void paintEvent(QPaintEvent* event) override;

private:

    QLabel *PedalVal;
    QTimer *time_update;
    QCheckBox *invCheck;

    bool on;
};

class chooseCC : public QDialog {

    Q_OBJECT

public:

    QComboBox *chooseBox;
    QLabel *label;

    int value;

    chooseCC(QDialog *parent, int val);
};

#endif // GUITOOLS_H
