#include "GuiTools.h"

#include <QDateTime>
#include "../midi/MidiFile.h"

void msDelay(int ms) {

    qint64 one = QDateTime::currentMSecsSinceEpoch();
    qint64 diff;

    do {

        QCoreApplication::processEvents();

        diff = QDateTime::currentMSecsSinceEpoch() - one;

    } while(diff < ms);

}

/***************************************************************************/
/* QPushButtonE2                                                           */
/***************************************************************************/

bool _QPushButtonE2::event(QEvent *event) {
    if(event->type() == QEvent::Leave) {

        is_enter = false;
    }


    if(event->type() == QEvent::Enter) {
        is_enter = true;
    }

    return QPushButton::event(event);
}

/***************************************************************************/
/* QDialE                                                                  */
/***************************************************************************/

QDialE::QDialE(QWidget* parent) : QDial(parent) {

    time_update = NULL;
    is_enter = false;
    but1 = new _QPushButtonE2(parent);
    but1->setObjectName(this->objectName() + QString::fromUtf8("-but1"));
    but1->setStyleSheet(QString::fromUtf8("background-color: #b0ffd0;\n"
                                          "border-style: outset;\n"
                                          "border-width: 2px;\n"
                                          "border-radius: 8px;\n"
                                          "border-color: #508f60;\n"
                                          "font: bold 12px;\n"
                                          ));
    but1->setText("-");
    but1->setAutoRepeat(true);

    but2 = new _QPushButtonE2(parent);
    but2->setObjectName(this->objectName() + QString::fromUtf8("-but2"));
    but2->setStyleSheet(QString::fromUtf8("background-color: #b0ffd0;\n"
                                          "border-style: outset;\n"
                                          "border-width: 2px;\n"
                                          "border-radius: 8px;\n"
                                          "border-color: #508f60;\n"
                                          "font: bold 12px;\n"
                                          ));
    but2->setText("+");
    but2->setAutoRepeat(true);

    but1->setVisible(false);
    but2->setVisible(false);

    connect(but1, &_QPushButtonE2::clicked, this, [=](bool)
            {
                setValue(value() - 1);
            });

    connect(but2, &_QPushButtonE2::clicked, this, [=](bool)
            {
                setValue(value() + 1);
            });
}

QDialE::~QDialE() {
    time_update->stop();
}

void QDialE::setGeometry(const QRect& r) {

    _r = r;

    but1->setGeometry(QRect(r.x() - 6, r.bottom() - 10, 18, 18));
    but2->setGeometry(QRect(r.right() - 14, r.bottom() - 10, 18, 18));

    QDial::setGeometry(r);

    if(!time_update) {
        time_update= new QTimer(this);
        time_update->setSingleShot(false);

        connect(time_update, &QTimer::timeout, this, [=]()
        {

            if((is_enter)
                || but1->is_enter || but2->is_enter) {
                time_update->stop();

                but1->setVisible(true);
                but2->setVisible(true);

                time_update->start(100);
            } else {
                time_update->stop();

                is_enter = false;
                but1->setVisible(false);
                but2->setVisible(false);

            }

        });

    }
}

void QDialE::setVisible(bool visible) {

    if(!visible) {
        but1->setVisible(visible);
        but2->setVisible(visible);
    }

    QDial::setVisible(visible);
}


void QDialE::mousePressEvent(QMouseEvent* event) {

    if(event->buttons() & Qt::RightButton) {
        emit mouseRightButton();
    }

}

bool QDialE::event(QEvent *event) {

    if(event->type() == QEvent::Leave) {
        time_update->stop();
        is_enter = false;
        time_update->start(100);
    }


    if(event->type() == QEvent::Enter) {
        time_update->stop();
        is_enter = true;
        time_update->start(100);

    }

    return QDial::event(event);
}

/***************************************************************************/
/* QPushButtonE                                                            */
/***************************************************************************/

void QPushButtonE::paintEvent(QPaintEvent* event) {
    QPushButton::paintEvent(event);

    if(_mode)
        return;

    // Estwald Color Changes
    QPainter *p = new QPainter(this);
    if(!p) return;

    if(this->isChecked())
        p->fillRect(8, (height() - 8) / 2, 8, 8, Qt::green);
    else
        p->fillRect(8, (height() - 8) / 2, 8, 8, Qt::red);


    p->setPen(0x808080);
    p->drawRect(8, (height() - 8) / 2, 8, 8);

    delete p;
}

void QPushButtonE::mousePressEvent(QMouseEvent* event) {
    if(event->buttons() & Qt::RightButton) {
        emit mouseRightButton();
    } else {
        QPushButton::mousePressEvent(event);
    }


}

/***************************************************************************/
/* QWidgetE                                                                */
/***************************************************************************/

void QWidgetE::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);

    // Estwald Color Changes
    static QBrush backgroundQ(QImage(":/run_environment/graphics/custom/background.png"));
    static QPixmap p1(":/run_environment/graphics/custom/Midicustom.png");
    static QPixmap p2(":/run_environment/graphics/icon.png");
    static int one = 1;

    if(one) {
        one = 0;

        p2 =  p2.scaled(70, 70);
    }

    // Estwald Color Changes
    QPainter *p = new QPainter(this);

    if(!p) return;


    p->setOpacity(0.5);
    p->fillRect(0, 0, width(), height() - 2, backgroundQ);
    p->setOpacity(1);

    if(logo) {

        int w = p->window().width() - 90;

        p->drawPixmap(w - 308, 30, p2);
        p->drawPixmap(w - 240, 20, p1);

        static QPixmap p3(":/run_environment/graphics/custom/drum.png");
        int x =  w - 400 + 30, y = height()/2;
        p->setOpacity(0.15);
        p->drawPixmap(x , y - 139, 400 , 300, p3);
        p->setOpacity(1);
    }

    delete p;
}


/***************************************************************************/
/* QLedBoxE                                                                 */
/***************************************************************************/

void QLedBoxE::paintEvent(QPaintEvent* event) {

    QWidget::paintEvent(event);


    // Estwald Color Changes
    QPainter *p = new QPainter(this);

    if(!p) return;

    QBrush b(led);

    p->setBrush(b);
    QPen pen(QColor(0x00, 0x40, 0x10, 0x80));
    pen.setWidth(2);
    p->setPen(pen);

    int rx = width();

    if(height() < rx)
        rx = height();

    rx >>= 1;
    rx--;

    p->drawEllipse(QPoint(width() / 2, height() / 2), rx, rx);


    delete p;
}

/***************************************************************************/
/* QPedalE                                                                */
/***************************************************************************/

QPedalE::QPedalE(QWidget* parent, QString title) : QGroupBox(parent) {

    on = false;

    setFixedSize(112, 135);
    setAlignment(Qt::AlignCenter);
    setTitle(title);

    PedalVal = new QLabel(this);
    PedalVal->setObjectName(QString::fromUtf8("PedalVal"));
    PedalVal->setGeometry(QRect(33, 18, 47, 21));
    PedalVal->setAlignment(Qt::AlignCenter);
    PedalVal->setStyleSheet("color: black; background: white;");
    PedalVal->setText("0");

    invCheck = new QCheckBox(this);
    invCheck->setObjectName(QString::fromUtf8("invCheck"));
    invCheck->setGeometry(QRect(23-8, 50, 47, 12));
    invCheck->setCheckable(true);

    connect(invCheck, &QCheckBox::clicked, this, [=](bool checked)
    {
        emit isChecked(checked);
    });

    QFont font;
    font.setPointSize(6);
    QLabel *invTCheck = new QLabel(this);
    invTCheck->setObjectName(QString::fromUtf8("invTCheck"));
    invTCheck->setGeometry(QRect(0, 62, 47, 21));
    invTCheck->setAlignment(Qt::AlignCenter);
    invTCheck->setFont(font);
    invTCheck->setText("INVERT");

    time_update = new QTimer(this);
    time_update->setSingleShot(true);

    connect(time_update, &QTimer::timeout, this, [=]()
    {
        on = false;
        update();

    });
}

void QPedalE::setVal(int val, bool setcheck) {

    invCheck->setChecked(setcheck);

    if(val < 0)
        return;

    if(val > 127)
        val = 127;

    PedalVal->setText(QString::number(val));

    on = true;

    update();

    time_update->stop();
    time_update->start(300);
}

void QPedalE::paintEvent(QPaintEvent* event) {

    QGroupBox::paintEvent(event);

    // Estwald Color Changes

    static QPixmap p_off(":/run_environment/graphics/custom/pedal_off.png");
    static QPixmap p_on(":/run_environment/graphics/custom/pedal_on.png");

    // Estwald Color Changes
    QPainter *p = new QPainter(this);

    if(!p) return;

    p->drawPixmap(32, 50, 48, height() - 54, on ? p_on : p_off);

    delete p;
}

/***************************************************************************/
/* chooseCC                                                                */
/***************************************************************************/

chooseCC::chooseCC(QDialog *parent, int val) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
    if (this->objectName().isEmpty())
        this->setObjectName(QString::fromUtf8("chooseCC"));
    this->setWindowTitle("Control Change Selector");
    this->setFixedSize(223 + 200, 126);

    value = val;

    label = new QLabel(this);
    label->setObjectName(QString::fromUtf8("label"));
    label->setGeometry(QRect(54, 15, 108 + 200, 24));
    label->setAlignment(Qt::AlignCenter);
    label->setText("Select CC");

    chooseBox = new QComboBox(this);
    chooseBox->setObjectName(QString::fromUtf8("chooseBox"));
    chooseBox->setGeometry(QRect(51, 54, 115 + 200, 24));

    for (int i = 1; i < 120; i++) {
        chooseBox->addItem("CC " + QString::number(i) + " - " +
                           MidiFile::controlChangeName(i), i);
    }

    for (int i = 0; i < chooseBox->count(); i++) {
        if(chooseBox->itemData(i) == value) {
            chooseBox->setCurrentIndex(i);
            break;
        }

    }

    connect(chooseBox, QOverload<int>::of(&QComboBox::currentIndexChanged),[=](int v)
    {
        value = chooseBox->itemData(v).toInt() & 127;

        done(0);

    });

}

