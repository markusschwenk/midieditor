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
#include <QSplitter>

#define MAX_TOOLTIP_TIME 5000
#define TOOLTIP(c, t) {c->setToolTip(t); c->setToolTipDuration(MAX_TOOLTIP_TIME);}
#define TOOLTIP2(c, t, time) {c->setToolTip(t); c->setToolTipDuration(time * 1000);}

extern QString GroupBoxChecked1;
extern QString GroupBoxChecked2;

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

enum qdiale_type
{
    QDIALE_DEFAULT = 0,
    QDIALE_GRAY_NOTCH_BLACKDOT,
    QDIALE_GRAY_NOTCH_BLACKLINE,
    QDIALE_BLACK_NOTCH_BLACKDOT,
    QDIALE_BLACK_NOTCH_BLACKLINE,
    QDIALE_GREEN_NOTCH_BLACKLINE,
    QDIALE_GRAY_NOTCH_WHITEDOT,
    QDIALE_GRAY_NOTCH_WHITELINE,
    QDIALE_BLACK_NOTCH_WHITEDOT,
    QDIALE_BLACK_NOTCH_WHITELINE
};


class QDialE : public QDial {

    Q_OBJECT

public:

    _QPushButtonE2 *but1;
    _QPushButtonE2 *but2;
    QTimer *time_update;

    bool is_enter;

    QRect _r;

    QDialE(QWidget* parent, qdiale_type type = QDIALE_DEFAULT, int steps = 1);
    ~QDialE();

    void setGeometry(const QRect& r);
    void setVisible(bool visible)  override;

signals:
    void mouseRightButton();

protected:

    void mousePressEvent(QMouseEvent* event) override;
    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QPixmap m_background;
    bool flag_back = false;
    int set_notch = 0;

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

    QPedalE(QWidget* parent, QString title, bool use_scalex2 = false);


public slots:

    void setVal(int val, bool setcheck = false, bool setscalex2 = false);

signals:

    void isChecked(bool checked);
    void scalex2Checked(bool checked);

protected:

    void paintEvent(QPaintEvent* event) override;

private:

    QLabel *PedalVal;
    QTimer *time_update;
    QCheckBox *invCheck;
    QCheckBox *scalex2Check;

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


enum qslider_type
{
    QSLIDER_DEFAULT = 0,
    QSLIDER_DEFAULT_TINY,
    QSLIDER_BLUE,
    QSLIDER_BLUE_TINY,
    QSLIDER_RED,
    QSLIDER_RED_TINY,
    QSLIDER_GREEN,
    QSLIDER_GREEN_TINY,
    QSLIDER_YELLOW,
    QSLIDER_YELLOW_TINY,

};

class QSliderE : public QSlider {

    Q_OBJECT

public:

    QSliderE(QWidget* parent, int nticks = 0, qslider_type type = QSLIDER_DEFAULT);

    void setGeometry(const QRect& r);

    void setOrientation(Qt::Orientation o);


protected:

    void paintEvent(QPaintEvent *) override;

private:

    QPixmap m_knob;
    QPixmap m_knobh;
    int type;
    int nticks;

};

int MessageBoxinformation(QWidget *w, const QString& title, const QString& text, const QString Accept, const QString Destruct, const QString Cancel);
int MessageBoxQuestion(QWidget *w, const QString& title, const QString& text, const QString Accept, const QString Destruct, const QString Cancel);

#endif // GUITOOLS_H
