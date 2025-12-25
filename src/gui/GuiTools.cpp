#include "GuiTools.h"
#include <math.h>

#include <QDateTime>
#include "../midi/MidiFile.h"

QString GroupBoxChecked1 = QString::fromUtf8("QGroupBox::indicator:unchecked {image: url(:/run_environment/graphics/custom/unchecked.png);}\n"
    "QGroupBox::indicator:checked {image: url(:/run_environment/graphics/custom/checked.png);}\n"
    "QGroupBox::title {subcontrol-origin: margin; left: 6px;\n"
    "subcontrol-position: top left; padding: 0 2px; }");

QString GroupBoxChecked2 = QString::fromUtf8("QGroupBox::indicator:unchecked {image: url(:/run_environment/graphics/custom/unchecked.png);}\n"
    "QGroupBox::indicator:checked {image: url(:/run_environment/graphics/custom/checked.png);}\n"
    "QGroupBox::title {subcontrol-origin: margin; left: 0px;\n"
    "subcontrol-position: top center; padding: 0 2px; }");

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

QDialE::QDialE(QWidget* parent, qdiale_type type, int steps) : QDial(parent) {

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
                                          "color: black;"
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
                                          "color: black;"
                                          ));
    but2->setText("+");
    but2->setAutoRepeat(true);

    but1->setVisible(false);
    but2->setVisible(false);

    connect(but1, &_QPushButtonE2::clicked, this, [=](bool)
    {
        setValue(value() - steps);
    });

    connect(but2, &_QPushButtonE2::clicked, this, [=](bool)
    {
        setValue(value() + steps);
    });

    flag_back = false;
    set_notch = 0;

    switch(type) {
      case QDIALE_GRAY_NOTCH_BLACKDOT:
      case QDIALE_GRAY_NOTCH_BLACKLINE:
        flag_back = true;
        set_notch = (type == QDIALE_GRAY_NOTCH_BLACKDOT) ? 1 : 2;
        m_background = QPixmap(":/run_environment/graphics/custom/dial1.png");
        break;
      case QDIALE_BLACK_NOTCH_BLACKDOT:
      case QDIALE_BLACK_NOTCH_BLACKLINE:
        flag_back = true;
        set_notch = (type == QDIALE_BLACK_NOTCH_BLACKDOT) ? 1 : 2;
        m_background = QPixmap(":/run_environment/graphics/custom/dial2.png");
        break;
      case QDIALE_GREEN_NOTCH_BLACKLINE:
        flag_back = true;
        set_notch = 2;
        m_background = QPixmap(":/run_environment/graphics/custom/dial3.png");
        break;

      case QDIALE_GRAY_NOTCH_WHITEDOT:
      case QDIALE_GRAY_NOTCH_WHITELINE:
        flag_back = true;
        set_notch = (type == QDIALE_GRAY_NOTCH_WHITEDOT) ? 3 : 4;
        m_background = QPixmap(":/run_environment/graphics/custom/dial1.png");
        break;
      case QDIALE_BLACK_NOTCH_WHITEDOT:
      case QDIALE_BLACK_NOTCH_WHITELINE:
        flag_back = true;
        set_notch = (type == QDIALE_BLACK_NOTCH_WHITEDOT) ? 3 : 4;
        m_background = QPixmap(":/run_environment/graphics/custom/dial2.png");
        break;
      default:
        break;
    }
    //setBackgroundImage(QPixmap(":/run_environment/graphics/custom/dial2.png"));

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
        if(time_update)
            time_update->stop();
        is_enter = false;
        if(time_update)
            time_update->start(100);
    }

    if(event->type() == QEvent::Enter) {
        if(time_update)
            time_update->stop();
        is_enter = true;
        if(time_update)
            time_update->start(100);

    }


    return QDial::event(event);
}

void QDialE::paintEvent(QPaintEvent *event)
{

    if(!flag_back) {
        QDial::paintEvent(event);
        return;
    }


    float alpha = this->isEnabled() ? 1.0f : 0.5f;

    int centerx= width() / 2;
    int centery= height() / 2;

    int v = value() - minimum();
    int rank = maximum() - minimum();
    if(rank == 0) rank = 1;
    if(v < 0) v = 0;
    if(v > rank) v = rank;
    rank++;

    #define PIX2 6.28318530f

    float PI45 = PIX2 / 12.0f;

    double step1 = (double) (((int) ((((double) rank) / this->notchTarget()) + 0.5)));

    double step = ((PIX2 - PI45 * 2.0f)/ step1);

    QPainter painter(this);
    painter.setOpacity(alpha);

    if(set_notch < 3) {
        painter.setPen(QColor(0x20, 0x20, 0x20));
        painter.setBrush(QColor(0x20, 0x20, 0x20));
    } else {
        painter.setPen(QColor(0xd0, 0xd0, 0xd0));
        painter.setBrush(QColor(0xd0, 0xd0, 0xd0));
    }

    int size1 = (width() < height() ? width() : height()) *93/100;

    float rad1 = (float) (size1/2);
    float rad2 = rad1 - 4.0f;

    if(set_notch != 0) {

        for(double a = PI45; a <= PIX2; a+= step) {

            bool bk = false;

            if(a >= (PIX2 - PI45)) {
                a = PIX2 - PI45;
                bk = true;
            }

            float sin1 = -sinf(a);
            float cos1 = cosf(a);
            int x1 = rad1 * sin1 + (float) centerx;
            int y1 = rad1 * cos1 + (float) centery;
            int x2 = rad2 * sin1 + (float) centerx;
            int y2 = rad2 * cos1 + (float) centery;
            painter.setRenderHint(QPainter::Antialiasing);

            if(set_notch & 1)
                painter.drawEllipse(x1, y1, 2, 2);
            else
                painter.drawLine(x1,y1,x2,y2);


            if(bk) break;
        }

    }

    painter.translate(centerx, centery);

    painter.rotate(30 + 300 *v/rank);

    int size2 = (width() < height() ? width() : height()) *80/100;
    if(size2 & 1) size2++;


    painter.drawPixmap(-size2/2, -size2/2, size2, size2, m_background);

    painter.rotate(0);



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

    float alpha = this->isEnabled() ? 1.0f : 0.5f;
    p->setOpacity(alpha);

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

QPedalE::QPedalE(QWidget* parent, QString title, bool use_scalex2) : QGroupBox(parent) {

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

    scalex2Check = new QCheckBox(this);
    scalex2Check->setObjectName(QString::fromUtf8("scalex2Check"));
    scalex2Check->setGeometry(QRect(85, 50, 47, 12));
    scalex2Check->setCheckable(true);

    if(!use_scalex2)
        scalex2Check->setVisible(false);

    connect(scalex2Check, &QCheckBox::clicked, this, [=](bool checked)
    {
        emit scalex2Checked(checked);
    });

    QFont font;
    font.setPointSize(6);
    QLabel *invTCheck = new QLabel(this);
    invTCheck->setObjectName(QString::fromUtf8("invTCheck"));
    invTCheck->setGeometry(QRect(0, 62, 47, 21));
    invTCheck->setAlignment(Qt::AlignCenter);
    invTCheck->setFont(font);
    invTCheck->setText("INVERT");

    QLabel *scalex2TCheck = new QLabel(this);
    scalex2TCheck->setObjectName(QString::fromUtf8("scalex2TCheck"));
    scalex2TCheck->setGeometry(QRect(85-15, 62, 47, 21));
    scalex2TCheck->setAlignment(Qt::AlignCenter);
    scalex2TCheck->setFont(font);
    scalex2TCheck->setText("SCALEx2");
    if(!use_scalex2)
        scalex2TCheck->setVisible(false);

    time_update = new QTimer(this);

    time_update->setSingleShot(true);

    connect(time_update, &QTimer::timeout, this, [=]()
    {
        on = false;
        update();

    });
}

void QPedalE::setVal(int val, bool setcheck, bool setscalex2) {

    invCheck->setChecked(setcheck);
    scalex2Check->setChecked(setscalex2);

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

/***************************************************************************/
/* QSliderE                                                                */
/***************************************************************************/

QSliderE::QSliderE(QWidget* parent, int nticks, qslider_type type) : QSlider(parent) {

    this->type = type;
    this->nticks = nticks;

    QColor color;

    switch(type / 2) {
        case 1:
            color = QColor(0x0, 0x50, 0xff);
            break;
        case 2:
            color = QColor(0xff, 0x30, 0x0);
            break;
        case 3:
            color = QColor(0x40, 0xff, 0x40);
            break;
        case 4:
            color = QColor(0xff, 0xff, 0x40);
            break;
        default:
            color = QColor(0xff, 0xff, 0xff);
            break;
    }


    m_knob = QPixmap(":/run_environment/graphics/custom/slider1.png");

    QPainter painter(&m_knob);
    painter.setPen(color);
    painter.setBrush(color);
    painter.drawRect(0, m_knob.height()/2 - 9, m_knob.width(), 15);
    painter.end();
    m_knobh = m_knob.transformed(QTransform().rotate(90));

}


void QSliderE::setGeometry(const QRect& r) {
    QSlider::setGeometry(r);

    setStyleSheet(QString::asprintf("QSlider::groove { background: transparent; width: %ipx; }\nQSlider::handle {width:%ipx;height:%ipx;}\n",
                                      width(),width(),width()));
}

void QSliderE::setOrientation(Qt::Orientation o) {

    QSlider::setOrientation(o);

    if(o == Qt::Horizontal) {

        setStyleSheet(QString::asprintf("QSlider::groove { background: transparent; height: %ipx; }\nQSlider::handle {width:%ipx;height:%ipx;}\n",
                                         height(), height(), height()));

    } else {

        int w = (type & 1) ? width() * 2 / 3 : width();
        setStyleSheet(QString::asprintf("QSlider::groove { background: transparent; width: %ipx; height:%ipx;}\nQSlider::handle {width:%ipx;height:%ipx;}\n",
                                        width(), height(), width(), w));
    }

    update();

}

void QSliderE::paintEvent(QPaintEvent *) {

    QPainter painter(this);

    if(nticks < 3)
      nticks = 3;

    if(!(nticks & 1))
        nticks++;

    int alpha = this->isEnabled() ? 255 : 128;
    painter.setOpacity(((float) alpha)/255.0f);

    if(orientation() != Qt::Horizontal) {

        int knobw = width();
        int knobh = (type & 1) ? width() * 2 / 3 : width();

        int len = (this->maximum() - this->minimum() + 1);

        float step = ((float) (height() - knobh)) / ((float) (nticks - 1));

        if(len) {

            int y =  knobh / 2;
            int ymid =  knobh / 2 + ((float) (nticks / 2)) * step;
            int x1 = 0;
            int x2 = width() - 4;

            int pen = 0;

            painter.setPen(QColor(0xc0, 0xc0, 0xc0));

            for(int n = 0; n < nticks; n++) {

                int y1 = y + ((int) (step * ((float) n)));

                if(y1>= (ymid - 1) && y1 <= (ymid + 1)) {

                    if(!pen) painter.setPen(QColor(0xf0, 0xc0, 0x00));
                    pen = 1;

                } else {

                    if(pen) painter.setPen(QColor(0xc0, 0xc0, 0xc0));
                    pen = 0;

                }

                painter.drawLine(x1, y1, x1 + 4, y1);
                painter.drawLine(x2, y1, x2 + 4, y1);

            }

        }

        painter.setPen(QColor(0x20, 0x20, 0x20));
        painter.setBrush(QColor(0x20, 0x20, 0x20));
        painter.drawRect(width()/2 - 3, 0, 5, height());
        painter.setBrush(QColor(0x60, 0x60, 0x60));
        painter.drawRect(width()/2 - 1, 0, 2, height());

        int y = (height() - knobh) * (len - (value() - minimum())) / len;
        painter.drawPixmap(0, y, knobw, knobh, m_knob);

    } else {

        int knobw = (type & 1) ? height() * 2 / 3 : height();

        int knobh = height() * 3 / 4;

        int large = ((width() - knobw) / 2) * 2;

        int len = (this->maximum() - this->minimum() + 1);

        float step = ((float) (width() - knobw)) /  ((float) (nticks - 1));

        if(len) {

            int x = knobw / 2;
            int xmid =  knobw / 2 + (nticks / 2) * step;
            int y1 = height() - 4;

            int pen = 0;

            painter.setPen(QColor(0xc0, 0xc0, 0xc0));

            for(int n = 0; n < nticks; n++) {

                int x1 = x + ((int) (step * ((float) n)));

                if(x1>= (xmid - 1) && x1 <= (xmid + 1)) {

                    if(!pen) painter.setPen(QColor(0xf0, 0xc0, 0x00));
                    pen = 1;

                } else {

                    if(pen) painter.setPen(QColor(0xc0, 0xc0, 0xc0));
                    pen = 0;

                }

                painter.drawLine(x1, y1 , x1, y1 + 4);

            }

        }

        int hh = height() * 3 / 8;

        painter.setPen(QColor(0x20, 0x20, 0x20));
        painter.drawRect(0, hh - 3, width(), 6);
        painter.setBrush(QColor(0x60, 0x60, 0x60));
        painter.drawRect(0, hh - 1, width(), 2);

        int x = (large) * (len - (maximum() - value())) / len;
        painter.drawPixmap(x, 0, knobw, knobh, m_knobh);
    }

}


int MessageBoxinformation(QWidget *w, const QString& title, const QString& text, const QString Accept, const QString Destruct, const QString Cancel) {
    QMessageBox msgBox(w);
    msgBox.setWindowTitle(title);
    msgBox.setText(text);
    msgBox.setIcon(QMessageBox::Information);

    QPushButton *saveButton = Accept.isEmpty() ? NULL : msgBox.addButton(Accept, QMessageBox::AcceptRole);
    QPushButton *discardButton = Destruct.isEmpty() ? NULL : msgBox.addButton(Destruct, QMessageBox::DestructiveRole);
    QPushButton *cancelButton = Cancel.isEmpty() ? NULL : msgBox.addButton(Cancel, QMessageBox::RejectRole);

    msgBox.setDefaultButton(cancelButton);

    msgBox.exec();

    if (msgBox.clickedButton() == saveButton) {
        return 0;
    } else if (msgBox.clickedButton() == discardButton) {
        return 1;
    }

    return 2;

}


int MessageBoxQuestion(QWidget *w, const QString& title, const QString& text, const QString Accept, const QString Destruct, const QString Cancel) {
    QMessageBox msgBox(w);
    msgBox.setWindowTitle(title);
    msgBox.setText(text);
    msgBox.setIcon(QMessageBox::Question);

    QPushButton *saveButton = Accept.isEmpty() ? NULL : msgBox.addButton(Accept, QMessageBox::AcceptRole);
    QPushButton *discardButton = Destruct.isEmpty() ? NULL : msgBox.addButton(Destruct, QMessageBox::DestructiveRole);
    QPushButton *cancelButton = Cancel.isEmpty() ? NULL : msgBox.addButton(Cancel, QMessageBox::RejectRole);

    msgBox.setDefaultButton(cancelButton);

    msgBox.exec();

    if (msgBox.clickedButton() == saveButton) {
        return 0;
    } else if (msgBox.clickedButton() == discardButton) {
        return 1;
    }

    return 2;

}
