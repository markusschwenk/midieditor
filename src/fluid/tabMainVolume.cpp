/*
 * MidiEditor
 * Copyright (C) 2010  Markus Schwenk
 *
 * tabMainVolume (FluidDialog)
 * Copyright (C) 2021 Francisco Munoz / "Estwald"
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.+
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef USE_FLUIDSYNTH

#include "FluidDialog.h"
#include "../MidiEvent/SysExEvent.h"
#include "../midi/MidiFile.h"
#include "../midi/MidiChannel.h"
#include "../protocol/Protocol.h"
#include "../tool/Selection.h"
#include "../tool/EventTool.h"

#include <QMessageBox>
#include <QDataStream>
#include <QIODevice>

class QVLabel : public QLabel
{


public:
    explicit QVLabel(const QString &text, QWidget *parent);

protected:
    void paintEvent(QPaintEvent*);

};

QVLabel::QVLabel(const QString &text, QWidget *parent=0)
        : QLabel(text, parent){
}

void QVLabel::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setClipping(false);
    painter.translate(8, this->height());
    painter.rotate(270);

    painter.drawText(0, 0, text());
}

void FluidDialog::tab_MainVolume(QDialog */*FluidDialog*/){

    int n;
    int area_size_y = 407;
    int buttons_y = 588;

    channel_selected = 0;

    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(tabWidget->sizePolicy().hasHeightForWidth());

    MainVolume = new QWidget();
    MainVolume->setObjectName(QString::fromUtf8("MainVolume"));

    if(disable_mainmenu) MainVolume->setDisabled(true);

    scrollArea = new QScrollArea(MainVolume);
    scrollArea->setObjectName(QString::fromUtf8("scrollArea"));
    scrollArea->setGeometry(QRect(4, 15, 554+60, area_size_y));
    sizePolicy.setHeightForWidth(scrollArea->sizePolicy().hasHeightForWidth());
    scrollArea->setSizePolicy(sizePolicy);
    scrollArea->setFocusPolicy(Qt::NoFocus);
    scrollArea->setContextMenuPolicy(Qt::NoContextMenu);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);

    scrollArea->setWidgetResizable(true);
    scrollArea->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

    QScrollBar * s=scrollArea->horizontalScrollBar();

    s->setMinimum(0);
    s->setMaximum(100);

    scrollArea->setStyleSheet(QString::fromUtf8("color: white; background-color: #80103040;"));

    groupM = new QGroupBox(scrollArea);
    groupM->setObjectName(QString::fromUtf8("groupM"));
    groupM->setStyleSheet(QString::fromUtf8("QGroupBox QToolTip {color: black;} \n"));
    groupM->setGeometry(QRect(0, 0, 2000, 600));

    groupE = new QGroupBox(MainVolume);
    groupE->setObjectName(QString::fromUtf8("groupE"));
    groupE->setGeometry(QRect(0, area_size_y+20, 752, buttons_y - area_size_y-20));
    groupE->setStyleSheet(QString::fromUtf8("QGroupBox QToolTip {color: black;} \n"));


    QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
    int x = 14;

    for(n = 0; n < 16; n++) {
        groupChan[n] = new QGroupBox(groupM/*scrollArea*/);
        if(n == channel_selected) {
            groupChan[n]->setStyleSheet(QString::fromUtf8("background-color: #80303060;"));
        }

        groupChan[n]->setObjectName(QString::fromUtf8("groupChan")+QString::number(n));
        groupChan[n]->setGeometry(QRect(x, 2, 53, 371));
        groupChan[n]->setFocusPolicy(Qt::NoFocus);

        x+=60;

        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(groupChan[n]->sizePolicy().hasHeightForWidth());

        groupChan[n]->setLayoutDirection(Qt::LeftToRight);
        groupChan[n]->setAlignment(Qt::AlignCenter);
        groupChan[n]->setFlat(false);
        groupChan[n]->setCheckable(true);
        groupChan[n]->setChecked(!fluid_output->getAudioMute(n));
        groupChan[n]->setToolTip("<html><head/><body><p>Mute/Unmute</p>"
                                                  "<p>the channel</p></body></html>");

        line[n] = new QFrame(groupChan[n]);
        line[n]->setObjectName(QString::fromUtf8("line"));
        line[n]->setGeometry(QRect(27 + 4, 2, 16, 20));
        line[n]->setFrameShadow(QFrame::Plain);
        line[n]->setLineWidth(8);
        line[n]->setFrameShape(QFrame::HLine);

        ChanVol[n] = new QSlider(groupChan[n]);
        ChanVol[n]->setObjectName(QString::fromUtf8("ChanVol")+QString::number(n));
        ChanVol[n]->setGeometry(QRect(12 + 4, 37, 22, 160));
        ChanVol[n]->setMouseTracking(false);
        ChanVol[n]->setMaximum(100);
        ChanVol[n]->setValue(fluid_output->getSynthChanVolume(n)*100/127);
        ChanVol[n]->setOrientation(Qt::Vertical);
        ChanVol[n]->setTickPosition(QSlider::TicksBothSides);
        ChanVol[n]->setTickInterval(25);
        ChanVol[n]->setToolTip("<html><head/><body><p>Synth Expression Volume</p>"
                                                  "<p>of the channel</p></body></html>");

        BalanceSlider[n] = new QSlider(groupChan[n]);
        BalanceSlider[n]->setObjectName(QString::fromUtf8("BalanceSlider")+QString::number(n));
        BalanceSlider[n]->setGeometry(QRect(8 + 4, 212, 30, 22));
        BalanceSlider[n]->setInputMethodHints(Qt::ImhNone);
        BalanceSlider[n]->setMinimum(-100);
        BalanceSlider[n]->setMaximum(100);
        BalanceSlider[n]->setValue(fluid_output->getAudioBalance(n));
        BalanceSlider[n]->setTracking(true);
        BalanceSlider[n]->setOrientation(Qt::Horizontal);
        BalanceSlider[n]->setTickPosition(QSlider::TicksAbove);
        BalanceSlider[n]->setTickInterval(25);
        BalanceSlider[n]->setToolTip("<html><head/><body><p>Output balance control</p>"
                                          "<p>of the channel</p></body></html>");

        BalanceLabel[n] = new QLabel(groupChan[n]);
        BalanceLabel[n]->setObjectName(QString::fromUtf8("BalanceLabel")+QString::number(n));
        BalanceLabel[n]->setGeometry(QRect(5 + 4, 243, 36, 22));
        BalanceLabel[n]->setToolTip("<html><head/><body><p>Output balance control</p>"
                                                  "<p>of the channel</p></body></html>");

        BalanceLabel[n]->setInputMethodHints(Qt::ImhNone);
        BalanceLabel[n]->setFrameShape(QFrame::Box);
        BalanceLabel[n]->setFrameShadow(QFrame::Raised);
        BalanceLabel[n]->setTextFormat(Qt::AutoText);
        BalanceLabel[n]->setAlignment(Qt::AlignCenter);
        BalanceLabel[n]->setWordWrap(true);

        Chan[n] = new QLabel(groupChan[n]);
        Chan[n]->setObjectName(QString::fromUtf8("Chan")+QString::number(n));
        Chan[n]->setGeometry(QRect(2 + 4, 17, 40, 13));
        Chan[n]->setAlignment(Qt::AlignCenter);

        chanGain[n] = new QDial(groupChan[n]);
        chanGain[n]->setObjectName(QString::fromUtf8("chanGain")+QString::number(n));
        chanGain[n]->setGeometry(QRect(-3 + 4, 270, 50, 91));
        chanGain[n]->setMinimum(-1000);
        chanGain[n]->setMaximum(1000);
        chanGain[n]->setValue(fluid_output->getAudioGain(n));
        chanGain[n]->setPageStep(50);
        chanGain[n]->setNotchTarget(100.0);
        chanGain[n]->setNotchesVisible(true);
        chanGain[n]->setToolTip("<html><head/><body><p>Output control gain</p>"
                                  "<p>of the channel</p></body></html>");

        label = new QLabel(groupChan[n]);
        label->setObjectName(QString::fromUtf8("label")+QString::number(n));
        label->setGeometry(QRect(7 + 4, 270, 31, 16));
        label->setAlignment(Qt::AlignCenter);

        chanGainLabel[n] = new QLabel(groupChan[n]);
        chanGainLabel[n]->setObjectName(QString::fromUtf8("chanGainLabel")+QString::number(n));
        chanGainLabel[n]->setGeometry(QRect(6 + 4, 340, 33, 21));
        chanGainLabel[n]->setFrameShape(QFrame::Box);
        chanGainLabel[n]->setFrameShadow(QFrame::Raised);
        chanGainLabel[n]->setAlignment(Qt::AlignCenter);
        chanGainLabel[n]->setToolTip("<html><head/><body><p>Output control gain</p>"
                                  "<p>of the channel</p></body></html>");

        QIcon ico(":/run_environment/graphics/channelwidget/vst_effect.png");
        wicon[n] = new QPushButton(groupM);
        wicon[n]->setObjectName(QString::asprintf("wicon %i", n));
        wicon[n]->setGeometry(QRect(x - 60 + 18 - 10, 374, 16, 16));
        wicon[n]->setStyleSheet(QString::fromUtf8("background-color: white;"));
        wicon[n]->setIcon(ico);

        if(VST_proc::VST_isLoaded(n))
            wicon[n]->setVisible(true);
        else
            wicon[n]->setVisible(false);

        wicon[n + 16] = new QPushButton(groupM);
        wicon[n + 16]->setObjectName(QString::asprintf("wicon %i", n + 16));
        wicon[n + 16]->setGeometry(QRect(x - 60 + 18 + 10, 374, 16, 16));
        wicon[n + 16]->setStyleSheet(QString::fromUtf8("background-color: white;"));
        wicon[n + 16]->setIcon(ico);

        if(VST_proc::VST_isLoaded(n + 16))
            wicon[n + 16]->setVisible(true);
        else
            wicon[n + 16]->setVisible(false);

        groupChan[n]->setTitle(QString());
        const QString str= "Ch "+QString::number(n);

        Chan[n]->setText(str);
        BalanceLabel[n]->setText(QString().setNum(((float)(fluid_output->getAudioBalance(n) / 10)/10.0), 'f', 2));
        label->setText("Gain");
        chanGainLabel[n]->setText(QString().setNum(((float)(fluid_output->getAudioGain(n) / 10))/10.0, 'f', 2));

        int _bank = Bank_MIDI[n];
        int _instrument = Prog_MIDI[n];

        QString s;
        if(n != 9) s = MidiFile::instrumentName( _bank, _instrument);
        else s = MidiFile::drumName(_instrument);

        qv[n] = new QVLabel(s, groupChan[n]);
        qv[n]->setObjectName(QString::fromUtf8("chanInstrum")+QString::number(n));
        qv[n]->setGeometry(QRect(4, 37, 12, 160));
    }

    groupMainVol = new QGroupBox(MainVolume);
    groupMainVol->setObjectName(QString::fromUtf8("groupMainVol"));
    groupMainVol->setStyleSheet(QString::fromUtf8("QGroupBox QToolTip {color: black;} \n"));
    groupMainVol->setGeometry(QRect(590+50, 15, 45, 275));
    groupMainVol->setLayoutDirection(Qt::LeftToRight);
    groupMainVol->setAlignment(Qt::AlignCenter);
    groupMainVol->setFlat(false);
    groupMainVol->setCheckable(true);
    groupMainVol->setTitle(QString());

    MainVol = new QSlider(groupMainVol);
    MainVol->setObjectName(QString::fromUtf8("MainVol"));
    MainVol->setGeometry(QRect(12, 37, 22, 200));
    MainVol->setMouseTracking(false);
    MainVol->setMaximum(200);
    MainVol->setMinimum(0);
    MainVol->setValue(fluid_output->getSynthGain());
    MainVol->setOrientation(Qt::Vertical);
    MainVol->setTickPosition(QSlider::TicksBothSides);
    MainVol->setTickInterval(25);
    MainVol->setToolTip("<html><head/><body><p>Synth Gain</p></body></html>");

    groupVUm = new QGroupBox(MainVolume);
    groupVUm->setObjectName(QString::fromUtf8("groupVUm"));
    groupVUm->setStyleSheet(QString::fromUtf8("QGroupBox QToolTip {color: black;} \n"));
    groupVUm->setGeometry(QRect(590+100, 51, 45, 206));
    groupVUm->setLayoutDirection(Qt::LeftToRight);
    groupVUm->setAlignment(Qt::AlignCenter);
    groupVUm->setFlat(false);
    groupVUm->setCheckable(false);
    groupVUm->setTitle(QString());
    groupVUm->setToolTip("<html><head/><body><p>VU Meter</p></body></html>");

    for(n = 0; n < 25; n++) {
        line_l[n] = new QFrame(groupVUm);
        line_l[n]->setObjectName(QString::fromUtf8("line_l")+QString::number(n));
        line_l[n]->setGeometry(QRect(2, 4+200*n/25, 20, 5));
        line_l[n]->setStyleSheet(QString::fromUtf8("color: #408000;"));
        line_l[n]->setFrameShadow(QFrame::Plain);
        line_l[n]->setLineWidth(18);
        line_l[n]->setFrameShape(QFrame::VLine);

        line_r[n] = new QFrame(groupVUm);
        line_r[n]->setObjectName(QString::fromUtf8("line_r")+QString::number(n));
        line_r[n]->setGeometry(QRect(23, 4+200*n/25, 20, 5));
        line_r[n]->setStyleSheet(QString::fromUtf8("color: #608000;"));
        line_r[n]->setFrameShadow(QFrame::Plain);
        line_r[n]->setLineWidth(18);
        line_r[n]->setFrameShape(QFrame::VLine);
    }


    Main = new QLabel(groupMainVol);
    Main->setObjectName(QString::fromUtf8("Main"));
    Main->setGeometry(QRect(2, 20, 41, 16));
    Main->setAlignment(Qt::AlignCenter);
    Main->setText("Main");

// effects

    labelChan = new QLabel(groupE);
    labelChan->setObjectName(QString::fromUtf8("labelChan"));
    labelChan->setGeometry(QRect(9, 14, 48, 16));
    labelChan->setAutoFillBackground(true);
    labelChan->setFrameShape(QFrame::Panel);
    labelChan->setAlignment(Qt::AlignCenter);
    labelChan->setText(QString("Chan ")+QString().setNum(channel_selected));
    labelChan->setToolTip("<html><head/><body><p>Channel selected</p></body></html>");

    spinChan = new QSpinBox(groupE);
    spinChan->setObjectName(QString::fromUtf8("spinChan"));
    spinChan->setToolTip("<html><head/><body><p>Current chan for Distortion, Low/High Cut...</p></body></html>");
    spinChan->setGeometry(QRect(11, 40, 45, 49));
    QFont font2;
    font2.setPointSize(16);
    spinChan->setFont(font2);
    spinChan->setAlignment(Qt::AlignCenter);
    spinChan->setMaximum(15);
    spinChan->setValue(channel_selected);
    spinChan->setAutoFillBackground(true);
    spinChan->setStyleSheet(QString::fromUtf8("color: black; background-color: white;"));

    DistortionBox = new QGroupBox(groupE);
    DistortionBox->setObjectName(QString::fromUtf8("DistortionBox"));
    DistortionBox->setGeometry(QRect(75, 4, 58, 127));
    DistortionBox->setLayoutDirection(Qt::LeftToRight);
    DistortionBox->setAlignment(Qt::AlignCenter);
    DistortionBox->setFlat(false);
    label_dist_gain = new QLabel(DistortionBox);
    label_dist_gain->setObjectName(QString::fromUtf8("label_dist_gain"));
    label_dist_gain->setGeometry(QRect(12, 35, 31, 16));
    label_dist_gain->setAlignment(Qt::AlignCenter);
    labelChan->setToolTip("<html><head/><body><p>Distortion Input Gain</p>"
                            "<p>of the current channel</p></body></html>");

    label_distortion_disp = new QLabel(DistortionBox);
    label_distortion_disp->setObjectName(QString::fromUtf8("label_distortion_disp"));
    label_distortion_disp->setGeometry(QRect(12, 100, 33, 21));
    label_distortion_disp->setFrameShape(QFrame::Box);
    label_distortion_disp->setFrameShadow(QFrame::Raised);
    label_distortion_disp->setAlignment(Qt::AlignCenter);
    label_distortion_disp->setToolTip("<html><head/><body><p>Distortion Input Gain</p>"
                                        "<p>of the current channel</p></body></html>");

    DistortionGain = new QDial(DistortionBox);
    DistortionGain->setObjectName(QString::fromUtf8("DistortionGain"));
    DistortionGain->setGeometry(QRect(3, 41, 50, 71));
    DistortionGain->setMinimum(0);
    DistortionGain->setMaximum(300);
    DistortionGain->setValue(
                fluid_output->get_param_filter(PROC_FILTER_DISTORTION,
                channel_selected, GET_FILTER_GAIN));
    DistortionGain->setNotchTarget(10.0);
    DistortionGain->setNotchesVisible(true);
    DistortionGain->setToolTip("<html><head/><body><p>Distortion Input Gain</p>"
                                 "<p>of the current channel</p></body></html>");

    DistortionButton = new QPushButton(DistortionBox);
    DistortionButton->setObjectName(QString::fromUtf8("DistortionButton"));
    DistortionButton->setGeometry(QRect(15, 18, 29, 16));
    DistortionButton->setCheckable(true);
    DistortionButton->setAutoDefault(false);
    DistortionButton->setFlat(false);
    DistortionButton->setDefault(true);
    DistortionButton->setChecked(
               (fluid_output->get_param_filter(PROC_FILTER_DISTORTION,
                channel_selected, GET_FILTER_ON)!=0) ? true : false);
    DistortionButton->setStyleSheet(QString::fromUtf8(
        "QPushButton{\n"
        "background-image: url(:/run_environment/graphics/fluid/icon_button_off.png);\n"
        "background-position: center center;}\n"
        "QPushButton::checked {\n"
        "background-image: url(:/run_environment/graphics/fluid/icon_button_on.png);\n"
        "}"));
    DistortionButton->setToolTip("<html><head/><body><p>Distortion On/Off</p>"
                                   "<p>of the current channel</p></body></html>");

    DistortionBox->setTitle("Distortion");
    label_dist_gain->setText("Level");
    label_distortion_disp->setText(QString().setNum(fluid_output->filter_dist_gain[channel_selected]));
    DistortionButton->setText(QString());

    LowCutBox = new QGroupBox(groupE);
    LowCutBox->setObjectName(QString::fromUtf8("LowCutBox"));
    LowCutBox->setGeometry(QRect(75+69, 4, 174, 127));
    LowCutBox->setLayoutDirection(Qt::LeftToRight);
    LowCutBox->setAlignment(Qt::AlignCenter);
    LowCutBox->setFlat(false);

    label_low_gain = new QLabel(LowCutBox);
    label_low_gain->setObjectName(QString::fromUtf8("label_low_gain"));
    label_low_gain->setGeometry(QRect(12, 35, 31, 16));
    label_low_gain->setAlignment(Qt::AlignCenter);
    label_low_gain->setToolTip("<html><head/><body><p>Low Cut Filter Gain</p>"
                                 "<p>of the current channel</p></body></html>");

    label_low_freq = new QLabel(LowCutBox);
    label_low_freq->setObjectName(QString::fromUtf8("label_low_freq"));
    label_low_freq->setGeometry(QRect(71, 35, 31, 16));
    label_low_freq->setAlignment(Qt::AlignCenter);
    label_low_freq->setToolTip("<html><head/><body><p>Low Cut Filter Frequency</p>"
                                     "<p>of the current channel</p></body></html>");

    label_low_res = new QLabel(LowCutBox);
    label_low_res->setObjectName(QString::fromUtf8("label_low_res"));
    label_low_res->setGeometry(QRect(76+57, 35, 31, 16));
    label_low_res->setToolTip("<html><head/><body><p>Low Cut Filter Resonance</p>"
                                     "<p>of the current channel</p></body></html>");

    label_low_disp = new QLabel(LowCutBox);
    label_low_disp->setObjectName(QString::fromUtf8("label_low_disp"));
    label_low_disp->setGeometry(QRect(12, 100, 33, 21));
    label_low_disp->setFrameShape(QFrame::Box);
    label_low_disp->setFrameShadow(QFrame::Raised);
    label_low_disp->setAlignment(Qt::AlignCenter);

    LowCutGain = new QDial(LowCutBox);
    LowCutGain->setObjectName(QString::fromUtf8("LowCutGain"));
    LowCutGain->setGeometry(QRect(3, 41, 50, 71));
    LowCutGain->setMinimum(0);
    LowCutGain->setMaximum(200);
    LowCutGain->setValue(
                fluid_output->get_param_filter(PROC_FILTER_LOW_PASS,
                channel_selected, GET_FILTER_GAIN));
    LowCutGain->setNotchTarget(10.0);
    LowCutGain->setNotchesVisible(true);
    LowCutGain->setToolTip("<html><head/><body><p>Low Cut Filter Gain</p>"
                                 "<p>of the current channel</p></body></html>");


    label_low_disp2 = new QLabel(LowCutBox);
    label_low_disp2->setObjectName(QString::fromUtf8("label_low_disp2"));
    label_low_disp2->setGeometry(QRect(69, 100, 33, 21));
    label_low_disp2->setFrameShape(QFrame::Box);
    label_low_disp2->setFrameShadow(QFrame::Raised);
    label_low_disp2->setAlignment(Qt::AlignCenter);

    LowCutFreq = new QDial(LowCutBox);
    LowCutFreq->setObjectName(QString::fromUtf8("LowCutFreq"));
    LowCutFreq->setGeometry(QRect(60, 41, 50, 71));
    LowCutFreq->setMinimum(50);
    LowCutFreq->setMaximum(2500);
    LowCutFreq->setValue(fluid_output->get_param_filter(PROC_FILTER_LOW_PASS,
                                                        channel_selected, GET_FILTER_FREQ));
    LowCutFreq->setNotchTarget(50.0);
    LowCutFreq->setNotchesVisible(true);
    LowCutFreq->setToolTip("<html><head/><body><p>Low Cut Filter Frequency</p>"
                                         "<p>of the current channel</p></body></html>");

    label_low_disp3 = new QLabel(LowCutBox);
    label_low_disp3->setObjectName(QString::fromUtf8("label_low_disp3"));
    label_low_disp3->setGeometry(QRect(69+57, 100, 33, 21));
    label_low_disp3->setFrameShape(QFrame::Box);
    label_low_disp3->setFrameShadow(QFrame::Raised);
    label_low_disp3->setAlignment(Qt::AlignCenter);

    LowCutRes = new QDial(LowCutBox);
    LowCutRes->setObjectName(QString::fromUtf8("LowCutRes"));
    LowCutRes->setGeometry(QRect(60+57, 41, 50, 71));
    LowCutRes->setMinimum(0);
    LowCutRes->setMaximum(250);
    LowCutRes->setValue(fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS,
                                                        channel_selected, GET_FILTER_RES));
    LowCutRes->setNotchTarget(14.0);
    LowCutRes->setNotchesVisible(true);
    LowCutRes->setToolTip("<html><head/><body><p>Low Cut Filter Resonance</p>"
                                         "<p>of the current channel</p></body></html>");

    LowCutButton = new QPushButton(LowCutBox);
    LowCutButton->setObjectName(QString::fromUtf8("LowCutButton"));

    LowCutButton->setGeometry(QRect(69, 18, 29, 16));
    LowCutButton->setCheckable(true);
    LowCutButton->setAutoDefault(false);
    LowCutButton->setDefault(true);
    LowCutButton->setChecked(
               (fluid_output->get_param_filter(PROC_FILTER_LOW_PASS,
                channel_selected, GET_FILTER_ON)!=0) ? true : false);
    LowCutButton->setText(QString());
    LowCutButton->setStyleSheet(QString::fromUtf8(
        "QPushButton{\n"
        "background-image: url(:/run_environment/graphics/fluid/icon_button_off.png);\n"
        "background-position: center center;}\n"
        "QPushButton::checked {\n"
        "background-image: url(:/run_environment/graphics/fluid/icon_button_on.png);\n"
        "}"));
    LowCutButton->setToolTip("<html><head/><body><p>Low Cut Filter On/Off</p>"
                                         "<p>of the current channel</p></body></html>");

    LowCutBox->setTitle("Low Cut");
    label_low_gain->setText("Gain");
    label_low_freq->setText("Freq");
    label_low_res->setText("Res");
    label_low_disp->setText(QString().setNum(((float)(fluid_output->filter_locut_gain[channel_selected] / 10)/10.0), 'f', 2));

    label_low_disp2->setText(QString().setNum(LowCutFreq->value()));
    label_low_disp3->setText(QString().setNum(((float)(fluid_output->filter_locut_res[channel_selected] / 10)/10.0), 'f', 2));

    HighCutBox = new QGroupBox(groupE);
    HighCutBox->setObjectName(QString::fromUtf8("HighCutBox"));
    HighCutBox->setGeometry(QRect(75+185+69, 4, 174, 127));
    HighCutBox->setLayoutDirection(Qt::LeftToRight);
    HighCutBox->setAlignment(Qt::AlignCenter);
    HighCutBox->setFlat(false);

    label_high_gain = new QLabel(HighCutBox);
    label_high_gain->setObjectName(QString::fromUtf8("label_high_gain"));
    label_high_gain->setGeometry(QRect(12, 35, 31, 16));
    label_high_gain->setAlignment(Qt::AlignCenter);
    label_high_gain->setToolTip("<html><head/><body><p>High Cut Filter Gain</p>"
                                 "<p>of the current channel</p></body></html>");

    label_high_freq = new QLabel(HighCutBox);
    label_high_freq->setObjectName(QString::fromUtf8("label_high_freq"));
    label_high_freq->setGeometry(QRect(69, 35, 31, 16));
    label_high_freq->setAlignment(Qt::AlignCenter);
    label_high_freq->setToolTip("<html><head/><body><p>High Cut Filter Frequency</p>"
                                             "<p>of the current channel</p></body></html>");

    label_high_res = new QLabel(HighCutBox);
    label_high_res->setObjectName(QString::fromUtf8("label_high_res"));
    label_high_res->setGeometry(QRect(69+57, 35, 31, 16));
    label_high_res->setAlignment(Qt::AlignCenter);
    label_high_res->setToolTip("<html><head/><body><p>High Cut Filter Resonance</p>"
                                             "<p>of the current channel</p></body></html>");

    label_high_disp = new QLabel(HighCutBox);
    label_high_disp->setObjectName(QString::fromUtf8("label_high_disp"));
    label_high_disp->setGeometry(QRect(12, 100, 33, 21));
    label_high_disp->setFrameShape(QFrame::Box);
    label_high_disp->setFrameShadow(QFrame::Raised);
    label_high_disp->setAlignment(Qt::AlignCenter);

    HighCutGain = new QDial(HighCutBox);
    HighCutGain->setObjectName(QString::fromUtf8("HighCutGain"));
    HighCutGain->setGeometry(QRect(3, 41, 50, 71));
    HighCutGain->setMinimum(0);
    HighCutGain->setMaximum(200);
    HighCutGain->setValue(
                fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS,
                channel_selected, GET_FILTER_GAIN));
    HighCutGain->setNotchTarget(10.0);
    HighCutGain->setNotchesVisible(true);
    HighCutGain->setToolTip("<html><head/><body><p>High Cut Filter Gain</p>"
                                     "<p>of the current channel</p></body></html>");


    label_high_disp2 = new QLabel(HighCutBox);
    label_high_disp2->setObjectName(QString::fromUtf8("label_high_disp2"));
    label_high_disp2->setGeometry(QRect(69, 100, 33, 21));
    label_high_disp2->setFrameShape(QFrame::Box);
    label_high_disp2->setFrameShadow(QFrame::Raised);
    label_high_disp2->setAlignment(Qt::AlignCenter);

    HighCutFreq = new QDial(HighCutBox);
    HighCutFreq->setObjectName(QString::fromUtf8("HighCutFreq"));
    HighCutFreq->setGeometry(QRect(60, 41, 50, 71));
    HighCutFreq->setMinimum(500);
    HighCutFreq->setMaximum(5000);
    HighCutFreq->setValue(fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS,
                                                        channel_selected, GET_FILTER_FREQ));
    HighCutFreq->setNotchTarget(50.0);
    HighCutFreq->setNotchesVisible(true);
    HighCutFreq->setToolTip("<html><head/><body><p>High Cut Filter Frequency</p>"
                              "<p>of the current channel</p></body></html>");


    label_high_disp3 = new QLabel(HighCutBox);
    label_high_disp3->setObjectName(QString::fromUtf8("label_high_disp3"));
    label_high_disp3->setGeometry(QRect(69+57, 100, 33, 21));
    label_high_disp3->setFrameShape(QFrame::Box);
    label_high_disp3->setFrameShadow(QFrame::Raised);
    label_high_disp3->setAlignment(Qt::AlignCenter);

    HighCutRes = new QDial(HighCutBox);
    HighCutRes->setObjectName(QString::fromUtf8("HighCutRes"));
    HighCutRes->setGeometry(QRect(60+57, 41, 50, 71));
    HighCutRes->setMinimum(0);
    HighCutRes->setMaximum(250);
    HighCutRes->setValue(fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS,
                                                        channel_selected, GET_FILTER_RES));
    HighCutRes->setNotchTarget(14.0);
    HighCutRes->setNotchesVisible(true);
    HighCutRes->setToolTip("<html><head/><body><p>High Cut Filter Resonance</p>"
                                                 "<p>of the current channel</p></body></html>");

    HighCutButton = new QPushButton(HighCutBox);
    HighCutButton->setObjectName(QString::fromUtf8("HighCutButton"));
    HighCutButton->setGeometry(QRect(70, 18, 29, 16));
    HighCutButton->setCheckable(true);
    HighCutButton->setAutoDefault(false);
    HighCutButton->setDefault(true);
    HighCutButton->setChecked((fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS,
               channel_selected, GET_FILTER_ON)!=0) ? true : false);
    HighCutButton->setToolTip("<html><head/><body><p>High Cut Filter On/Off</p>"
                                             "<p>of the current channel</p></body></html>");

    HighCutBox->setTitle("High Cut");

    label_high_gain->setText("Gain");
    label_high_freq->setText("Freq");
    label_high_res->setText("Res");
    label_high_disp->setText(QString().setNum(((float)(fluid_output->filter_hicut_gain[channel_selected] / 10)/10.0), 'f', 2));
    label_high_disp2->setText(QString().setNum(HighCutFreq->value()));
    label_high_disp3->setText(QString().setNum(((float)(fluid_output->filter_locut_gain[channel_selected] / 10)/10.0), 'f', 2));

    HighCutButton->setText(QString());
    HighCutButton->setStyleSheet(QString::fromUtf8(
        "QPushButton{\n"
        "background-image: url(:/run_environment/graphics/fluid/icon_button_off.png);\n"
        "background-position: center center;}\n"
        "QPushButton::checked {\n"
        "background-image: url(:/run_environment/graphics/fluid/icon_button_on.png);\n"
        "}"));

    PresetBox = new QGroupBox(groupE);
    PresetBox->setObjectName(QString::fromUtf8("PresetBox"));
    PresetBox->setGeometry(QRect(586-70, 4, 161+70, 128));
    PresetBox->setLayoutDirection(Qt::LeftToRight);
    PresetBox->setAlignment(Qt::AlignCenter);
    PresetBox->setFlat(false);
    PresetBox->setTitle("Presets");

    QFont font;
    font.setPointSize(16);
    spinPreset = new QSpinBox(PresetBox);
    spinPreset->setObjectName(QString::fromUtf8("spinPreset"));
    spinPreset->setToolTip("<html><head/><body><p>Current Preset for Load/Save</p></body></html>");
    spinPreset->setGeometry(QRect(10, 14, 45, 49));
    spinPreset->setFont(font);
    spinPreset->setAlignment(Qt::AlignCenter);
    spinPreset->setMaximum(15);
    spinPreset->setValue(preset_selected);
    spinPreset->setStyleSheet(QString::fromUtf8("color: black; background-color: white;"));

    PresetLoadButton = new QPushButton(PresetBox);
    PresetLoadButton->setObjectName(QString::fromUtf8("PresetLoadButton"));
    PresetLoadButton->setGeometry(QRect(10, 70, 63, 26));
    PresetLoadButton->setCheckable(false);
    PresetLoadButton->setAutoDefault(false);
    PresetLoadButton->setFlat(false);
    PresetLoadButton->setDefault(true);
    PresetLoadButton->setText("Load");
    PresetLoadButton->setStyleSheet(QString::fromUtf8("background-color: #80002F5F;\n"));
    PresetLoadButton->setToolTip("<html><head/><body><p>Load all presets</p>"
                                             "<p>from the indexed register</p></body></html>");

    PresetSaveButton = new QPushButton(PresetBox);
    PresetSaveButton->setObjectName(QString::fromUtf8("PresetSaveButton"));
    PresetSaveButton->setGeometry(QRect(10, 100, 63, 26));
    PresetSaveButton->setCheckable(false);
    PresetSaveButton->setAutoDefault(false);
    PresetSaveButton->setFlat(false);
    PresetSaveButton->setDefault(true);
    PresetSaveButton->setText("Save");
    PresetSaveButton->setStyleSheet(QString::fromUtf8("background-color: #80002F5F;\n"));
    PresetSaveButton->setToolTip("<html><head/><body><p>Save all presets</p>"
                                                 "<p>to the indexed register</p></body></html>");

    PresetResetButton = new QPushButton(PresetBox);
    PresetResetButton->setObjectName(QString::fromUtf8("PresetResetButton"));
    PresetResetButton->setGeometry(QRect(90, 70, 63, 26));
    PresetResetButton->setCheckable(false);
    PresetResetButton->setAutoDefault(false);
    PresetResetButton->setFlat(false);
    PresetResetButton->setDefault(true);
    PresetResetButton->setText("Reset");
    PresetResetButton->setStyleSheet(QString::fromUtf8("background-color: #80002F5F;\n"));
    PresetResetButton->setToolTip("<html><head/><body><p>Reset the presets</p>"
                                                     "<p>to the default values</p></body></html>");

    PresetLoadPButton = new QPushButton(PresetBox);
    PresetLoadPButton->setObjectName(QString::fromUtf8("PresetLoadPButton"));
    PresetLoadPButton->setGeometry(QRect(59, 14, 52, 51));
    PresetLoadPButton->setCheckable(false);
    PresetLoadPButton->setAutoDefault(false);
    PresetLoadPButton->setFlat(false);
    PresetLoadPButton->setDefault(true);
    PresetLoadPButton->setText("Get #0\nPresets\nin MIDI");
    PresetLoadPButton->setStyleSheet(QString::fromUtf8("background-color: #80002F5F;\n"));
    PresetLoadPButton->setToolTip("<html><head/><body><p>Load (if it exits)</p>"
                                         "<p>the systemEx event with all presets </p>"
                                         "<p>from the MIDI file position 0</p></body></html>");

    PresetStorePButton = new QPushButton(PresetBox);
    PresetStorePButton->setObjectName(QString::fromUtf8("PresetStorePButton"));
    PresetStorePButton->setGeometry(QRect(59+53, 14, 52, 51));
    PresetStorePButton->setCheckable(false);
    PresetStorePButton->setAutoDefault(false);
    PresetStorePButton->setFlat(false);
    PresetStorePButton->setDefault(true);
    PresetStorePButton->setText("Store All\nPresets\nin MIDI");
    PresetStorePButton->setStyleSheet(QString::fromUtf8("background-color: #80002F5F;\n"));
    PresetStorePButton->setToolTip("<html><head/><body><p>Store all presets in the MIDI File</p>"
                                     "<p>from the current position using </p>"
                                     "<p>a SystemEx event</p></body></html>");

    PresetDeletePButton = new QPushButton(PresetBox);
    PresetDeletePButton->setObjectName(QString::fromUtf8("PresetDeletePButton"));
    PresetDeletePButton->setGeometry(QRect(59+53*2, 70, 52, 51));
    PresetDeletePButton->setCheckable(false);
    PresetDeletePButton->setAutoDefault(false);
    PresetDeletePButton->setFlat(false);
    PresetDeletePButton->setDefault(true);
    PresetDeletePButton->setText("Delete\nPresets\nin MIDI");
    PresetDeletePButton->setStyleSheet(QString::fromUtf8("background-color: #40002F5F;\n"));
    PresetDeletePButton->setDisabled(true);
    PresetDeletePButton->setToolTip("<html><head/><body><p>Deletes the MIDI presets</p>"
                                      "<p>edited from the current position</p></body></html>");

    PresetStoreButton = new QPushButton(PresetBox);
    PresetStoreButton->setObjectName(QString::fromUtf8("PresetStoreButton"));
    PresetStoreButton->setGeometry(QRect(59+53*2, 14, 52, 51));
    PresetStoreButton->setCheckable(false);
    PresetStoreButton->setAutoDefault(false);
    PresetStoreButton->setFlat(false);
    PresetStoreButton->setDefault(true);
    PresetStoreButton->setText("Store Ch\nPreset\nin MIDI");
    PresetStoreButton->setStyleSheet(QString::fromUtf8("background-color: #80002F5F;\n"));
    PresetStoreButton->setToolTip("<html><head/><body><p>Store the preset of the current</p>"
                                    "<p>channel into the MIDI File</p>"
                                      "<p>from the current position</p></body></html>");

    for(n = 0; n < 16; n++) {

        connect(wicon[n], QOverload<bool>::of(&QPushButton::clicked), [=](bool)
        {
            VST_chan vst(main_widget, n, 1);
        });

        connect(wicon[n + 16], QOverload<bool>::of(&QPushButton::clicked), [=](bool)
        {
            VST_chan vst(main_widget, n + 16, 1);
        });

        connect(groupChan[n], QOverload<bool>::of(&QGroupBox::clicked), [=](bool checked)
        {
            fluid_output->setAudioMute(n, !checked);

        });

        connect(ChanVol[n], QOverload<int>::of(&QDial::valueChanged), [=](int num)
        {
            fluid_output->setSynthChanVolume(n, num*127/100);
        });

        connect(BalanceSlider[n], QOverload<int>::of(&QDial::valueChanged), [=](int num)
        {
            BalanceLabel[n]->setText(QString().setNum(((float)(num / 10)/10.0), 'f', 2));
            fluid_output->setAudioBalance(n, num);
        });

        connect(chanGain[n], QOverload<int>::of(&QDial::valueChanged), [=](int num)
        {
            chanGainLabel[n]->setText(QString().setNum(((float)(num / 10))/10.0, 'f', 2));
            fluid_output->setAudioGain(n, num);
            groupE->update();
        });

    }

    connect(MainVol, QOverload<int>::of(&QDial::valueChanged), [=](int num)
    {
        fluid_output->setSynthGain(num);
    });


    // effects
    QObject::connect(DistortionGain, SIGNAL(valueChanged(int)), label_distortion_disp, SLOT(setNum(int)));
    QObject::connect(LowCutFreq, SIGNAL(valueChanged(int)), label_low_disp2, SLOT(setNum(int)));
    connect(LowCutGain, QOverload<int>::of(&QDial::valueChanged), [=](int num)
    {
        label_low_disp->setText(QString().setNum(((float)(num / 10)/10.0), 'f', 2));

    });

    connect(LowCutRes, QOverload<int>::of(&QDial::valueChanged), [=](int num)
    {
        label_low_disp3->setText(QString().setNum(((float)(num / 10)/10.0), 'f', 2));

    });

    QObject::connect(HighCutFreq, SIGNAL(valueChanged(int)), label_high_disp2, SLOT(setNum(int)));

    connect(HighCutGain, QOverload<int>::of(&QDial::valueChanged), [=](int num)
    {
        label_high_disp->setText(QString().setNum(((float)(num / 10)/10.0), 'f', 2));

    });

    connect(HighCutRes, QOverload<int>::of(&QDial::valueChanged), [=](int num)
    {
        label_high_disp3->setText(QString().setNum(((float)(num / 10)/10.0), 'f', 2));

    });

    connect(spinChan, QOverload<int>::of(&QSpinBox::valueChanged), [=](int num)
    {
        scrollArea->horizontalScrollBar()->setValue(num*7);
        groupM->setGeometry(QRect((num<9) ? 0: (9-num)*60, 0, 2000, 600));

        labelChan->setText(QString("Chan ")+QString().setNum(num));

        groupChan[channel_selected]->setStyleSheet(QString::fromUtf8("background-color: #80103040;"));
        channel_selected = num;
        groupChan[num]->setStyleSheet(QString::fromUtf8("background-color: #80303060;"));

        DistortionGain->setValue(
                    fluid_output->get_param_filter(PROC_FILTER_DISTORTION,
                    channel_selected, GET_FILTER_GAIN));

        LowCutGain->setValue(
                    fluid_output->get_param_filter(PROC_FILTER_LOW_PASS,
                    channel_selected, GET_FILTER_GAIN));
        LowCutFreq->setValue(fluid_output->get_param_filter(PROC_FILTER_LOW_PASS,
                                                           channel_selected, GET_FILTER_FREQ));
        LowCutRes->setValue(fluid_output->get_param_filter(PROC_FILTER_LOW_PASS,
                                                           channel_selected, GET_FILTER_RES));


        HighCutGain->setValue(
                    fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS,
                    channel_selected, GET_FILTER_GAIN));
        HighCutFreq->setValue(fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS,
                     channel_selected, GET_FILTER_FREQ));
        HighCutRes->setValue(fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS,
                     channel_selected, GET_FILTER_RES));


        DistortionButton->setChecked(false);
        DistortionButton->setChecked(
                   (fluid_output->get_param_filter(PROC_FILTER_DISTORTION,
                    channel_selected, GET_FILTER_ON)!=0) ? true : false);


        LowCutButton->setChecked(false);
        LowCutButton->setChecked(
                   (fluid_output->get_param_filter(PROC_FILTER_LOW_PASS,
                    channel_selected, GET_FILTER_ON)!=0) ? true : false);


        HighCutButton->setDefault(true);
        HighCutButton->setChecked(
                   (fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS,
                    channel_selected, GET_FILTER_ON)!=0) ? true : false);

    });

    QObject::connect(DistortionButton, SIGNAL(clicked()), this, SLOT(distortion_clicked()));
    QObject::connect(DistortionGain, SIGNAL(valueChanged(int)), this, SLOT(distortion_gain(int)));

    QObject::connect(LowCutButton, SIGNAL(clicked()), this, SLOT(lowcut_clicked()));
    QObject::connect(LowCutGain, SIGNAL(valueChanged(int)), this, SLOT(lowcut_gain(int)));
    QObject::connect(LowCutFreq, SIGNAL(valueChanged(int)), this, SLOT(lowcut_freq(int)));
    QObject::connect(LowCutRes, SIGNAL(valueChanged(int)), this, SLOT(lowcut_res(int)));

    QObject::connect(HighCutButton, SIGNAL(clicked()), this, SLOT(highcut_clicked()));
    QObject::connect(HighCutGain, SIGNAL(valueChanged(int)), this, SLOT(highcut_gain(int)));
    QObject::connect(HighCutFreq, SIGNAL(valueChanged(int)), this, SLOT(highcut_freq(int)));
    QObject::connect(HighCutRes, SIGNAL(valueChanged(int)), this, SLOT(highcut_res(int)));

    QObject::connect(PresetSaveButton, SIGNAL(clicked()), this, SLOT(SaveRPressets()));
    QObject::connect(PresetLoadButton, SIGNAL(clicked()), this, SLOT(LoadRPressets()));
    QObject::connect(PresetDeletePButton, SIGNAL(clicked()), this, SLOT(DeletePressets()));
    QObject::connect(PresetStorePButton, SIGNAL(clicked()), this, SLOT(StorePressets()));
    QObject::connect(PresetLoadPButton, SIGNAL(clicked()), this, SLOT(LoadPressets()));
    QObject::connect(PresetStoreButton, SIGNAL(clicked()), this, SLOT(StoreSelectedPresset()));
    QObject::connect(PresetResetButton, SIGNAL(clicked()), this, SLOT(ResetPresset()));

    connect(spinPreset, QOverload<int>::of(&QSpinBox::valueChanged), [=](int num)
    {
        preset_selected = num;

    });

    // scroll
    connect(scrollArea->horizontalScrollBar(), QOverload<int>::of(&QDial::valueChanged), [=](int num)
    {
        groupM->setGeometry(QRect(-num*58/16, 0, 2000, 600));
    });



}

void FluidDialog::mousePressEvent(QMouseEvent* /*event*/) {

    if(MainVolume == NULL || disable_mainmenu) return;

    for(int n= 0; n < 16; n++) {
        if(groupChan[n]->hasFocus()) {
            groupChan[channel_selected]->setStyleSheet(QString::fromUtf8("background-color: #80103040;"));
            channel_selected = n;
            groupChan[n]->setStyleSheet(QString::fromUtf8("background-color: #80303060;"));
            spinChan->setValue(n);
            break;

        }
    }

}

void FluidDialog::distortion_clicked()
{

    int gain = fluid_output->get_param_filter(PROC_FILTER_DISTORTION, channel_selected, GET_FILTER_GAIN);

    if(fluid_output->get_param_filter(PROC_FILTER_DISTORTION, channel_selected, GET_FILTER_ON)==0)
        fluid_output->setAudioDistortionFilter(1, channel_selected, gain);
    else fluid_output->setAudioDistortionFilter(0, channel_selected, gain);
}

void FluidDialog::distortion_gain(int gain)
{
    bool check = (fluid_output->get_param_filter(PROC_FILTER_DISTORTION, channel_selected, GET_FILTER_ON)!=0) ? true : false;

    fluid_output->setAudioDistortionFilter((check) ? 1: 0, channel_selected, gain);
}

void FluidDialog::lowcut_clicked()
{

    int gain = fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel_selected, GET_FILTER_GAIN);
    int freq = fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel_selected, GET_FILTER_FREQ);
    int res = fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel_selected, GET_FILTER_RES);

    if(fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel_selected, GET_FILTER_ON)==0)
        fluid_output->setAudioLowPassFilter(1, channel_selected, freq, gain, res);
    else fluid_output->setAudioLowPassFilter(0, channel_selected, freq, gain, res);
}

void FluidDialog::lowcut_gain(int gain)
{
    bool check = (fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel_selected, GET_FILTER_ON)!=0) ? true : false;

    int freq = fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel_selected, GET_FILTER_FREQ);
    int res = fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel_selected, GET_FILTER_RES);

    fluid_output->setAudioLowPassFilter((check) ? 1: 0, channel_selected, freq, gain, res);
}

void FluidDialog::lowcut_freq(int freq)
{
    bool check = (fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel_selected, GET_FILTER_ON)!=0) ? true : false;

    int gain = fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel_selected, GET_FILTER_GAIN);
    int res = fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel_selected, GET_FILTER_RES);

    fluid_output->setAudioLowPassFilter((check) ? 1: 0, channel_selected, freq, gain, res);
}

void FluidDialog::lowcut_res(int res)
{
    bool check = (fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel_selected, GET_FILTER_ON)!=0) ? true : false;

    int gain = fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel_selected, GET_FILTER_GAIN);
    int freq = fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel_selected, GET_FILTER_FREQ);

    fluid_output->setAudioLowPassFilter((check) ? 1: 0, channel_selected, freq, gain, res);
}

void FluidDialog::highcut_clicked()
{

    int gain = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel_selected, GET_FILTER_GAIN);
    int freq = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel_selected, GET_FILTER_FREQ);
    int res = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel_selected, GET_FILTER_RES);

    if(fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel_selected, GET_FILTER_ON)==0)
        fluid_output->setAudioHighPassFilter(1, channel_selected, freq, gain, res);
    else fluid_output->setAudioHighPassFilter(0, channel_selected, freq, gain, res);
}

void FluidDialog::highcut_gain(int gain)
{
    bool check = (fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel_selected, GET_FILTER_ON)!=0) ? true : false;

    int freq = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel_selected, GET_FILTER_FREQ);
    int res = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel_selected, GET_FILTER_RES);

    fluid_output->setAudioHighPassFilter((check) ? 1: 0, channel_selected, freq, gain, res);
}

void FluidDialog::highcut_freq(int freq)
{
    bool check = (fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel_selected, GET_FILTER_ON)!=0) ? true : false;

    int gain = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel_selected, GET_FILTER_GAIN);
    int res = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel_selected, GET_FILTER_RES);

    fluid_output->setAudioHighPassFilter((check) ? 1: 0, channel_selected, freq, gain, res);
}

void FluidDialog::highcut_res(int res)
{
    bool check = (fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel_selected, GET_FILTER_ON)!=0) ? true : false;

    int gain = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel_selected, GET_FILTER_GAIN);
    int freq = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel_selected, GET_FILTER_FREQ);

    fluid_output->setAudioHighPassFilter((check) ? 1: 0, channel_selected, freq, gain, res);
}

void FluidDialog::timer_update(){

    if(fluid_output== NULL || MainVolume == NULL || disable_mainmenu) return;

    for(int n = 0; n < 16; n++) {
        if(groupChan[n]) {
            if(groupChan[n]->isChecked() && fluid_output->getAudioMute(n))
                groupChan[n]->setChecked(false);
            if(!groupChan[n]->isChecked() && !fluid_output->getAudioMute(n))
                groupChan[n]->setChecked(true);
        }
    }

    if(_cur_edit == -666 && _edit_mode) {
        _edit_mode = 0;
        PresetDeletePButton->setStyleSheet(QString::fromUtf8("background-color: #40002F5F;\n"));
        PresetDeletePButton->setDisabled(true);
    } else if(_cur_edit < 0 && _edit_mode) {
        _cur_edit = _current_tick;
        MainWindow *MWin = ((MainWindow *) _parent); // get MainWindow :D
        MWin->update();
    }

    // VU meter
    int vl = fluid_output->cleft;
    int vr = fluid_output->cright;

    for(int n = 0; n < 25;n++) {
        int flag= 25 - vl / 8;
        if(n>=flag) {
            if(n < 5) line_l[n]->setStyleSheet(QString::fromUtf8("color: #FF6000;"));
            else line_l[n]->setStyleSheet(QString::fromUtf8("color: #60FF00;"));
        } else
            line_l[n]->setStyleSheet(QString::fromUtf8("color: #408000;"));
        flag= 25 - vr /8;
        if(n >= flag) {
            if(n < 5) line_r[n]->setStyleSheet(QString::fromUtf8("color: #FF8000;"));
            else line_r[n]->setStyleSheet(QString::fromUtf8("color: #80FF00;"));
        } else
            line_r[n]->setStyleSheet(QString::fromUtf8("color: #608000;"));
    }

    // channel animation
    for(int n = 0; n < 16; n++) {
        if(fluid_output->isNoteOn[n]) {
            if(!fluid_output->getAudioMute(n))
                line[n]->setStyleSheet(QString::fromUtf8("color: green;"));
            else
                line[n]->setStyleSheet(QString::fromUtf8("color: red;"));
        } else
            line[n]->setStyleSheet(QString::fromUtf8("color: #00000000;"));

        if(fluid_output->isNoteOn[n]) fluid_output->isNoteOn[n]--;
    }

    for(int n = 0; n < 16; n++) {
        int _bank = Bank_MIDI[n];
        int _instrument = Prog_MIDI[n];

        QString s;
        if(n != 9) s = MidiFile::instrumentName( _bank, _instrument);
        else s = MidiFile::drumName(_instrument);

        qv[n]->setText(s);
    }
    update();
}


static void encode_sys_format(QDataStream &qd, void *data) {

    unsigned char *a= (unsigned char *) data;
    unsigned char b[5];

    b[0]=(a[0]>>1);
    b[1]=(a[1]>>1);
    b[2]=(a[2]>>1);
    b[3]=(a[3]>>1);

    b[4]= (a[0] & 1) | ((a[1] & 1)<<1) | ((a[2] & 1)<<2) |
            ((a[3] & 1)<<3);

   if(qd.writeRawData((const char *) b, 5)<0) return ;

}

static int decode_sys_format(QDataStream &qd, void *data) {

    unsigned char *a= (unsigned char *) data;
    unsigned char b[5];

    int ret =qd.readRawData((char *) b, 5);
    if(ret<0) b[0]=b[1]=b[2]=b[3]=b[4]=0;

    a[0]=(b[0]<<1) | ((b[4]) & 1);
    a[1]=(b[1]<<1) | ((b[4]>>1) & 1);
    a[2]=(b[2]<<1) | ((b[4]>>2) & 1);
    a[3]=(b[3]<<1) | ((b[4]>>3) & 1);

    return ret;
}

void FluidDialog::DeletePressets() // store or delete
{
    QByteArray b;

    char id[4]=
    {0x0, 0x66, 0x66, 'P'};

    MainWindow *MWin = ((MainWindow *) _parent); // get MainWindow :D

    if(_edit_mode) {// delete preset

        MWin->getFile()->protocol()->startNewAction("SYSex Presets Deleted");
        int dtick= MWin->getFile()->tick(150);
        int current_tick = _current_tick;
        foreach (MidiEvent* event,
                 *(MWin->getFile()->eventsBetween(current_tick-dtick, current_tick+dtick))) {

            SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

            if(sys) {
                b = sys->data();
                if((b[0]==id[0] || b[3]=='R') && b[1]==id[1] && b[2]==id[2] &&
                        (b[3]=='R' || b[3]=='P' || b[3]==(char) (0x70+channel_selected))){
                    MWin->getFile()->channel(16)->removeEvent(sys);
                }
            }

        }

        MWin->getFile()->protocol()->endAction();

        return;
    }
}
void FluidDialog::StorePressets() // store or delete
{

    QByteArray b[16], c;

    char id[4]=
    {0x0, 0x66, 0x66, 'R'};
    int entries = 13;
    int BOOL;

    MainWindow *MWin = ((MainWindow *) _parent); // get MainWindow :D

    for(int n = 0; n < 16; n++) {

        char id2[4]=
        {0x0, 0x66, 0x66, 'R'};
        QDataStream qd(&b[n],
                       QIODevice::WriteOnly); // save header

        id2[0] = n;

        // write the header
        if(qd.writeRawData((const char *) id2, 4)<0) return;

        if(n == 0)
            entries = 13 + 1;
        else
            entries = 13;

        encode_sys_format(qd, (void *) &entries);

        if(n == 0)
            encode_sys_format(qd, (void *) &fluid_output->synth_gain);

        encode_sys_format(qd, (void *) &fluid_output->synth_chanvolume[n]);
        encode_sys_format(qd, (void *) &fluid_output->audio_changain[n]);
        encode_sys_format(qd, (void *) &fluid_output->audio_chanbalance[n]);

        if(fluid_output->filter_dist_on[n]) BOOL=1; else BOOL=0;
        encode_sys_format(qd, (void *) &BOOL);
        encode_sys_format(qd, (void *) &fluid_output->filter_dist_gain[n]);

        if(fluid_output->filter_locut_on[n]) BOOL=1; else BOOL=0;
        encode_sys_format(qd, (void *) &BOOL);
        encode_sys_format(qd, (void *) &fluid_output->filter_locut_freq[n]);
        encode_sys_format(qd, (void *) &fluid_output->filter_locut_gain[n]);
        encode_sys_format(qd, (void *) &fluid_output->filter_locut_res[n]);

        if(fluid_output->filter_hicut_on[n]) BOOL=1; else BOOL=0;
        encode_sys_format(qd, (void *) &BOOL);
        encode_sys_format(qd, (void *) &fluid_output->filter_hicut_freq[n]);
        encode_sys_format(qd, (void *) &fluid_output->filter_hicut_gain[n]);
        encode_sys_format(qd, (void *) &fluid_output->filter_hicut_res[n]);

        if(_save_mode) {
            QString entry=QString("presets_audio_index")+QString().setNum(preset_selected)+QString("")+QString().setNum(n);
            fluid_output->fluid_settings->setValue(entry, b[n]);

        }
    }

    if(_save_mode) return;

    MWin->getFile()->protocol()->startNewAction("SYSex Presets stored");
    int dtick= MWin->getFile()->tick(150);

    int current_tick = _current_tick;

    foreach (MidiEvent* event,
             *(MWin->getFile()->eventsBetween(current_tick-dtick, current_tick+dtick))) {

        SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

        if(sys) {
            c = sys->data();
            // delete for individual chans
            if(c[0]==id[0] && c[1]==id[1] && c[2]==id[2] && (c[3] & 0xF0)==(char) 0x70){
                MWin->getFile()->channel(event->channel())->removeEvent(sys);
            }
            if(c[0]==id[0] && c[1]==id[1] && c[2]==id[2] && c[3]=='P'){
                MWin->getFile()->channel(16)->removeEvent(sys);
            }
            if(c[1]==id[1] && c[2]==id[2] && c[3]=='R'){
                MWin->getFile()->channel(16)->removeEvent(sys);
            }

        }

    }

    for(int n = 0; n < 16; n++) {
        SysExEvent *sys_event = new SysExEvent(16, b[n], MWin->getFile()->track(0));
        MWin->getFile()->channel(16)->insertEvent(sys_event, current_tick);
    }

    MWin->getFile()->protocol()->endAction();

}

void FluidDialog::LoadPressets()
{
    QByteArray b[16], d;

    char id2[4]= {0x0, 0x66, 0x66, 'P'};
    int found= 0;
    int BOOL;

    MainWindow *MWin = ((MainWindow *) _parent); // get MainWindow :D

    if(_save_mode) {
        for(int n = 0; n < 16; n++) {
            QString entry=QString("presets_audio_index")+QString().setNum(preset_selected)+QString("")+QString().setNum(n);
            b[n]= fluid_output->fluid_settings->value(entry).toByteArray();
        }

        found = 2;
    } else {
        foreach (MidiEvent* event,
                 *(MWin->getFile()->eventsBetween(0, 50))) {

            SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

            if(sys) {
                d = sys->data();
                if(d[1]!=id2[1] || d[2]!=id2[2] ) continue;
                if(d[0]==id2[0] && d[3]=='P') {b[0] = d; found=1;}
                if(d[3]=='R') {b[d[0] & 0xF] = d; found=2;}

            }
        }

    }

    if(!found) return;

    int entries = 13 * 16 + 1;
    char id[4];

    for(int m = 0; m < 16; m++) {

        if(b[m].size() < 4) continue;

        QDataStream qd(&b[m],
                       QIODevice::ReadOnly);
        qd.startTransaction();

        qd.readRawData((char *) id, 4);

        if(id[1]==id2[1] && id[2]==id2[2] &&  ((id[0]==id2[0] && id[3] == 'P')
                                               || id[3] == 'R')) {

            if(id[3] == 'R') found = 2;
            else found =1;

            if(decode_sys_format(qd, (void *) &entries)<0) {

                continue;
            }

            if((found == 1 && entries != 13 * 16 + 1) &&
                    (found == 2 && entries != (13 +(m==0)))) {

                continue;
            }

            if(m == 0)
                decode_sys_format(qd, (void *) &fluid_output->synth_gain);

            int k = 0, l = 16;

            if(found == 2) {
                k = m;
                l = m + 1;
            }

            for(int n = k; n < l; n++) {
                fluid_output->audio_chanmute[n]= false;
                groupChan[n]->setChecked(!fluid_output->getAudioMute(n));
                decode_sys_format(qd, (void *) &fluid_output->synth_chanvolume[n]);
                decode_sys_format(qd, (void *) &fluid_output->audio_changain[n]);
                decode_sys_format(qd, (void *) &fluid_output->audio_chanbalance[n]);

                decode_sys_format(qd, (void *) &BOOL);
                fluid_output->filter_dist_on[n]= (BOOL) ? true : false;
                decode_sys_format(qd, (void *) &fluid_output->filter_dist_gain[n]);

                decode_sys_format(qd, (void *) &BOOL);
                fluid_output->filter_locut_on[n]= (BOOL) ? true : false;
                decode_sys_format(qd, (void *) &fluid_output->filter_locut_freq[n]);
                decode_sys_format(qd, (void *) &fluid_output->filter_locut_gain[n]);
                decode_sys_format(qd, (void *) &fluid_output->filter_locut_res[n]);

                decode_sys_format(qd, (void *) &BOOL);
                fluid_output->filter_hicut_on[n]= (BOOL) ? true : false;
                decode_sys_format(qd, (void *) &fluid_output->filter_hicut_freq[n]);
                decode_sys_format(qd, (void *) &fluid_output->filter_hicut_gain[n]);
                decode_sys_format(qd, (void *) &fluid_output->filter_hicut_res[n]);

                ChanVol[n]->setValue(fluid_output->getSynthChanVolume(n)*100/127);
                BalanceSlider[n]->setValue(fluid_output->getAudioBalance(n));
                chanGain[n]->setValue(fluid_output->getAudioGain(n));

            }

        }
    }

    MainVol->setValue(fluid_output->getSynthGain());
    channel_selected=0;
    spinChan->valueChanged(channel_selected);
    update();
}

void FluidDialog::StoreSelectedPresset()
{
    QByteArray b;

    char id[4]=
    {0x0, 0x66, 0x66, 0x70};
    int entries = 13 * 1;

    int BOOL;

    id[3]|= (channel_selected & 15);

    QDataStream qd(&b,
                   QIODevice::WriteOnly); // save header

    // write the header
    if(qd.writeRawData((const char *) id, 4)<0) return;

    encode_sys_format(qd, (void *) &entries);

    encode_sys_format(qd, (void *) &fluid_output->synth_gain);

    int n = channel_selected;

        encode_sys_format(qd, (void *) &fluid_output->synth_chanvolume[n]);
        encode_sys_format(qd, (void *) &fluid_output->audio_changain[n]);
        encode_sys_format(qd, (void *) &fluid_output->audio_chanbalance[n]);

        if(fluid_output->filter_dist_on[n]) BOOL=1; else BOOL=0;
        encode_sys_format(qd, (void *) &BOOL);
        encode_sys_format(qd, (void *) &fluid_output->filter_dist_gain[n]);

        if(fluid_output->filter_locut_on[n]) BOOL=1; else BOOL=0;
        encode_sys_format(qd, (void *) &BOOL);
        encode_sys_format(qd, (void *) &fluid_output->filter_locut_freq[n]);
        encode_sys_format(qd, (void *) &fluid_output->filter_locut_gain[n]);
        encode_sys_format(qd, (void *) &fluid_output->filter_locut_res[n]);

        if(fluid_output->filter_hicut_on[n]) BOOL=1; else BOOL=0;
        encode_sys_format(qd, (void *) &BOOL);
        encode_sys_format(qd, (void *) &fluid_output->filter_hicut_freq[n]);
        encode_sys_format(qd, (void *) &fluid_output->filter_hicut_gain[n]);
        encode_sys_format(qd, (void *) &fluid_output->filter_hicut_res[n]);


    MainWindow *MWin = ((MainWindow *) _parent); // get MainWindow :D
    MWin->getFile()->track(0);
    SysExEvent *sys_event = new SysExEvent(16, b, MWin->getFile()->track(0));

    MWin->getFile()->protocol()->startNewAction("SYSex Preset stored");
    int dtick= MWin->getFile()->tick(150);

    int current_tick =_current_tick;
    foreach (MidiEvent* event,
             *(MWin->getFile()->eventsBetween(current_tick-dtick, current_tick+dtick))) {

        SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

        if(sys) {
            b = sys->data();
            if(b[0]==id[0] && b[1]==id[1] && b[2]==id[2] && b[3]=='P'){

                StorePressets(); // use colective command
                return;
            }
            if(b[0]==id[0] && b[1]==id[1] && b[2]==id[2] && b[3]==id[3]){
                MWin->getFile()->channel(16)->removeEvent(sys);
            }
        }

    }
    MWin->getFile()->channel(16)->insertEvent(sys_event, current_tick);
    MWin->getFile()->protocol()->endAction();
}

void FluidDialog::LoadSelectedPresset(int current_tick)
{
    QByteArray b;

    char id2[4]= {0x0, 0x66, 0x66, 0x70};

    int BOOL;

    MainWindow *MWin = ((MainWindow *) _parent); // get MainWindow :D
    int dtick= MWin->getFile()->tick(150);

    //int current_tick = MWin->getFile()->cursorTick();
    foreach (MidiEvent* event,
             *(MWin->getFile()->eventsBetween(current_tick-dtick, current_tick+dtick))) {

        SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

        if(sys) {
            b = sys->data();

            // old
            if(b[0]==id2[0] && b[1]==id2[1] && b[2]==id2[2] &&
                    b[3]=='P') {
                int entries = 13 * 16 + 1;
                char id[4];

                QDataStream qd(&b,
                               QIODevice::ReadOnly);
                qd.startTransaction();

                qd.readRawData((char *) id, 4);

                if(id[0]!=id2[0] || id[1]!=id2[1] || id[2]!=id2[2]) {
                    return;
                }

                if(id[3]!='P') return;

                if(decode_sys_format(qd, (void *) &entries)<0) {

                    return;
                }

                if(entries!=13 * 16 + 1) {
                    QMessageBox::information(this, "Ehhh!", "entries differents");
                    return;
                }


                decode_sys_format(qd, (void *) &fluid_output->synth_gain);

                for(int n = 0; n < 16; n++) {
                    fluid_output->audio_chanmute[n]= false;
                    groupChan[n]->setChecked(!fluid_output->getAudioMute(n));
                    decode_sys_format(qd, (void *) &fluid_output->synth_chanvolume[n]);
                    decode_sys_format(qd, (void *) &fluid_output->audio_changain[n]);
                    decode_sys_format(qd, (void *) &fluid_output->audio_chanbalance[n]);

                    decode_sys_format(qd, (void *) &BOOL);
                    fluid_output->filter_dist_on[n]= (BOOL) ? true : false;
                    decode_sys_format(qd, (void *) &fluid_output->filter_dist_gain[n]);

                    decode_sys_format(qd, (void *) &BOOL);
                    fluid_output->filter_locut_on[n]= (BOOL) ? true : false;
                    decode_sys_format(qd, (void *) &fluid_output->filter_locut_freq[n]);
                    decode_sys_format(qd, (void *) &fluid_output->filter_locut_gain[n]);
                    decode_sys_format(qd, (void *) &fluid_output->filter_locut_res[n]);

                    decode_sys_format(qd, (void *) &BOOL);
                    fluid_output->filter_hicut_on[n]= (BOOL) ? true : false;
                    decode_sys_format(qd, (void *) &fluid_output->filter_hicut_freq[n]);
                    decode_sys_format(qd, (void *) &fluid_output->filter_hicut_gain[n]);
                    decode_sys_format(qd, (void *) &fluid_output->filter_hicut_res[n]);

                    ChanVol[n]->setValue(fluid_output->getSynthChanVolume(n)*100/127);
                    BalanceSlider[n]->setValue(fluid_output->getAudioBalance(n));
                    chanGain[n]->setValue(fluid_output->getAudioGain(n));

                }

                _edit_mode = 1;
                _current_tick = current_tick;

                PresetDeletePButton->setStyleSheet(QString::fromUtf8("background-color: #805f2F00;\n"));
                PresetDeletePButton->setDisabled(false);

                MainVol->setValue(fluid_output->getSynthGain());
                channel_selected=0;
                spinChan->valueChanged(channel_selected);
               // update();
            } else if(b[1] == id2[1] && b[2] == id2[2] && ((b[0] == id2[0] && (b[3] & 0xf0) == 0x70) ||
                                                       b[3] == 'R')){
                int entries = 13 * 1;
                char id[4];

                int flag = 1;

                int n = b[3] & 0xf;

                if(b[3] == 'R') {
                    flag = 2;
                    n = b[0] & 0xf;
                }

                QDataStream qd(&b,
                               QIODevice::ReadOnly);
                qd.startTransaction();

                qd.readRawData((char *) id, 4);

                if(flag == 1 && (id[0]!=id2[0] || id[1]!=id2[1] || id[2]!=id2[2] || (id[3] & 0xF0) != 0x70)) {
                    continue;
                }

                if(flag == 2 && (id[1]!=id2[1] || id[2]!=id2[2] || id[3] != 'R')) {
                    continue;
                }

                if(decode_sys_format(qd, (void *) &entries)<0) {

                    continue;
                }

                if(flag == 2 && n == 0 && entries != (13 + 1)) {
                    QMessageBox::information(this, "Ehhh!", "wentries differents");
                    continue;
                }

                if((n != 0 || flag == 1) && entries != 13) {
                    QMessageBox::information(this, "Ehhh!", "entries differents");
                    continue;
                }

                if((flag == 2 && n == 0) || flag == 1)
                    decode_sys_format(qd, (void *) &fluid_output->synth_gain);

                fluid_output->audio_chanmute[n]= false;
                groupChan[n]->setChecked(!fluid_output->getAudioMute(n));
                decode_sys_format(qd, (void *) &fluid_output->synth_chanvolume[n]);
                decode_sys_format(qd, (void *) &fluid_output->audio_changain[n]);
                decode_sys_format(qd, (void *) &fluid_output->audio_chanbalance[n]);

                decode_sys_format(qd, (void *) &BOOL);
                fluid_output->filter_dist_on[n]= (BOOL) ? true : false;
                decode_sys_format(qd, (void *) &fluid_output->filter_dist_gain[n]);

                decode_sys_format(qd, (void *) &BOOL);
                fluid_output->filter_locut_on[n]= (BOOL) ? true : false;
                decode_sys_format(qd, (void *) &fluid_output->filter_locut_freq[n]);
                decode_sys_format(qd, (void *) &fluid_output->filter_locut_gain[n]);
                decode_sys_format(qd, (void *) &fluid_output->filter_locut_res[n]);

                decode_sys_format(qd, (void *) &BOOL);
                fluid_output->filter_hicut_on[n]= (BOOL) ? true : false;
                decode_sys_format(qd, (void *) &fluid_output->filter_hicut_freq[n]);
                decode_sys_format(qd, (void *) &fluid_output->filter_hicut_gain[n]);
                decode_sys_format(qd, (void *) &fluid_output->filter_hicut_res[n]);

                ChanVol[n]->setValue(fluid_output->getSynthChanVolume(n)*100/127);
                BalanceSlider[n]->setValue(fluid_output->getAudioBalance(n));
                chanGain[n]->setValue(fluid_output->getAudioGain(n));

                _edit_mode = 1;
                _current_tick = current_tick;

                PresetDeletePButton->setStyleSheet(QString::fromUtf8("background-color: #805f2F00;\n"));
                PresetDeletePButton->setDisabled(false);

                if(flag == 1) {
                    channel_selected = n;
                    spinChan->setValue(n);
                    spinChan->valueChanged(n);
                    channel_selected = n;
                } else {

                    MainVol->setValue(fluid_output->getSynthGain());
                    channel_selected=0;
                    spinChan->valueChanged(channel_selected);
                }

            }

        } // sys
    }

}

void FluidDialog::ResetPresset() {

    fluid_output->synth_gain= 1.0;

    for(int n = 0; n < 16; n++) {
        fluid_output->audio_chanmute[n]= false;
        groupChan[n]->setChecked(!fluid_output->getAudioMute(n));
        fluid_output->synth_chanvolume[n]= 127.0;
        fluid_output->audio_changain[n]= 0.0;
        fluid_output->audio_chanbalance[n]= 0.0;

        fluid_output->filter_dist_on[n]= false;
        fluid_output->filter_dist_gain[n]= 0.0;

        fluid_output->filter_locut_on[n]= false;
        fluid_output->filter_locut_freq[n]= 500.0;
        fluid_output->filter_locut_gain[n]= 0.0;
        fluid_output->filter_locut_res[n]= 0.0;

        fluid_output->filter_hicut_on[n]= false;
        fluid_output->filter_hicut_freq[n]= 2500.0;
        fluid_output->filter_hicut_gain[n]= 0.0;
        fluid_output->filter_hicut_res[n]= 0.0;

        ChanVol[n]->setValue(fluid_output->getSynthChanVolume(n)*100/127);
        BalanceSlider[n]->setValue(fluid_output->getAudioBalance(n));
        chanGain[n]->setValue(fluid_output->getAudioGain(n));

    }

    MainVol->setValue(fluid_output->getSynthGain());
    channel_selected=0;
    spinChan->valueChanged(channel_selected);
    update();
}

void FluidDialog::SaveRPressets() {
    _save_mode = 1;
    StorePressets();
    _save_mode = 0;
}
void FluidDialog::LoadRPressets() {
    _save_mode = 1;
    LoadPressets();
    _save_mode = 0;
}

#endif
