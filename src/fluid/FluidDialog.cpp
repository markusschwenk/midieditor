/*
 * MidiEditor
 * Copyright (C) 2010  Markus Schwenk
 *
 * FluidDialog
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

FluidDialog::FluidDialog(QWidget* parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint) {

    QDialog *FluidDialog = this;
    _parent = parent;

    MainVolume = NULL;
    disable_mainmenu = 0;

    MainWindow *MWin = ((MainWindow *) _parent); // get MainWindow :D
    _current_tick = (MWin->getFile()) ? MWin->getFile()->cursorTick() : 0;
    _edit_mode = 0;
    _save_mode = 0;

    int buttons_y= 588;

    channel_selected= 0;
    preset_selected = 0;

    if (FluidDialog->objectName().isEmpty())
        FluidDialog->setObjectName(QString::fromUtf8("FluidDialog"));
    FluidDialog->resize(753 + 125, buttons_y);
    FluidDialog->setFixedSize(753 + 125, buttons_y);

    QPalette palette;
    QBrush brush(QColor(255, 255, 255, 255));
    brush.setStyle(Qt::SolidPattern);
    palette.setBrush(QPalette::Active, QPalette::Base, brush);

    QBrush brush1(QColor(15, 16, 21, 255));
    brush1.setStyle(Qt::SolidPattern);

    palette.setBrush(QPalette::Active, QPalette::Window, brush1);
    palette.setBrush(QPalette::Inactive, QPalette::Base, brush);
    palette.setBrush(QPalette::Inactive, QPalette::Window, brush1);
    palette.setBrush(QPalette::Disabled, QPalette::Base, brush1);
    palette.setBrush(QPalette::Disabled, QPalette::Window, brush1);
    FluidDialog->setPalette(palette);

    QFont font;
    font.setPointSize(8);
    FluidDialog->setFont(font);

    tabWidget = new QTabWidget(FluidDialog);
    tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
    tabWidget->setGeometry(QRect(-2, 0, 720 + 90 + 125, buttons_y));

    tabWidget->setFocusPolicy(Qt::NoFocus);
    tabWidget->setAutoFillBackground(true);

    if((fluid_output->it_have_error == 2) || fluid_output->status_fluid_err)
        disable_mainmenu = 1;

    tab_MainVolume(FluidDialog);
    MainVolume->setStyleSheet(QString::fromUtf8("color: white; background-color: #103030;  "));

    if(!disable_mainmenu) {
        tabWidget->addTab(MainVolume, QString());
        tabWidget->setTabText(tabWidget->indexOf(MainVolume), "Main Volume");
    }


    tabConfig = new QWidget();
    tabConfig->setObjectName(QString::fromUtf8("tabConfig"));
    tab_Config(FluidDialog);
    tabWidget->addTab(tabConfig, QString());

    FluidDialog->setWindowTitle("Fluid Synth Control");

    tabWidget->setTabText(tabWidget->indexOf(tabConfig), "Config");
    tabWidget->setCurrentIndex(0);
    tabWidget->update();

    time_updat= new QTimer(this);
    time_updat->setSingleShot(false);

    connect(time_updat, SIGNAL(timeout()), this, SLOT(timer_update()), Qt::DirectConnection);
    time_updat->setSingleShot(false);
    time_updat->start(50);

    connect(fluid_output, SIGNAL(changeVolBalanceGain(int , int , int , int)), this, SLOT(changeVolBalanceGain(int , int , int , int)), Qt::QueuedConnection);
    connect(fluid_output, SIGNAL(changeMainVolume(int)), this, SLOT(changeMainVolume(int)), Qt::QueuedConnection);
    connect(fluid_output, SIGNAL(changeFilterValue(int , int)), this, SLOT(changeFilterValue(int , int)), Qt::QueuedConnection);

    QMetaObject::connectSlotsByName(FluidDialog);

}

void FluidDialog::reject() {
    time_updat->stop();
    delete time_updat;
    if(fluid_output->fluid_settings->value("mp3_artist").toString() != "")
        fluid_output->fluid_settings->setValue("mp3_artist", MP3_lineEdit_artist->text());
    hide();
    _cur_edit = -1;
}

void FluidDialog::changeVolBalanceGain(int index, int vol, int balance, int gain) {
    int fl = (index >> 4) & 3;

    if(fl == 0 || fl == 1) {
        emit ChanVol[index & 15]->valueChanged(vol);
        ChanVol[index & 15]->setValue(vol);
    }

    if(fl == 0 || fl == 2) {
        emit BalanceSlider[index & 15]->valueChanged(balance);
        BalanceSlider[index & 15]->setValue(balance);
    }

    if(fl == 0 || fl == 3) {
        emit chanGain[index & 15]->valueChanged(gain);
        chanGain[index & 15]->setValue(gain);
    }

    emit spinChan->valueChanged(15);
    emit spinChan->valueChanged(0);
    spinChan->setValue(0);
    update();
}

void FluidDialog::changeMainVolume(int vol) {

    MainVol->setValue(vol);
    emit spinChan->valueChanged(15);
    emit spinChan->valueChanged(0);
    spinChan->setValue(0);
    update();
}

void FluidDialog::changeFilterValue(int index, int channel) {

    if(index == 0) {

        emit DistortionGain->valueChanged(fluid_output->get_param_filter(PROC_FILTER_DISTORTION,
                    channel, GET_FILTER_GAIN));

        emit DistortionButton->toggled(fluid_output->get_param_filter(PROC_FILTER_DISTORTION,
                                                                      channel, GET_FILTER_ON));
        DistortionButton->setChecked(fluid_output->get_param_filter(PROC_FILTER_DISTORTION,
                                                                         channel, GET_FILTER_ON));
        DistortionGain->setValue(fluid_output->get_param_filter(PROC_FILTER_DISTORTION,
                                                                     channel, GET_FILTER_GAIN));

    } else if(index == 1) {

        emit LowCutGain->valueChanged(fluid_output->get_param_filter(PROC_FILTER_LOW_PASS,
                    channel, GET_FILTER_GAIN));
        emit LowCutFreq->valueChanged(fluid_output->get_param_filter(PROC_FILTER_LOW_PASS,
                                                            channel, GET_FILTER_FREQ));

        emit LowCutRes->valueChanged(fluid_output->get_param_filter(PROC_FILTER_LOW_PASS,
                                                            channel, GET_FILTER_RES));

        emit LowCutButton->toggled(fluid_output->get_param_filter(PROC_FILTER_LOW_PASS,
                                                                      channel, GET_FILTER_ON));
        LowCutButton->setChecked(fluid_output->get_param_filter(PROC_FILTER_LOW_PASS,
                                                                      channel, GET_FILTER_ON));

        LowCutGain->setValue(fluid_output->get_param_filter(PROC_FILTER_LOW_PASS,
                    channel, GET_FILTER_GAIN));

        LowCutFreq->setValue(fluid_output->get_param_filter(PROC_FILTER_LOW_PASS,
                                                            channel, GET_FILTER_FREQ));

        LowCutRes->setValue(fluid_output->get_param_filter(PROC_FILTER_LOW_PASS,
                                                            channel, GET_FILTER_RES));


    } else if(index == 2) {

        emit HighCutGain->valueChanged(
                    fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS,
                    channel, GET_FILTER_GAIN));

        emit HighCutFreq->valueChanged(fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS,
                                                            channel, GET_FILTER_FREQ));

        emit HighCutRes->valueChanged(fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS,
                                                            channel, GET_FILTER_RES));

        emit HighCutButton->toggled(fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS,
                                                                      channel, GET_FILTER_ON));

        HighCutButton->setChecked(fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS,
                                                                      channel, GET_FILTER_ON));
        HighCutGain->setValue(fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS,
                    channel, GET_FILTER_GAIN));

        HighCutFreq->setValue(fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS,
                                                            channel, GET_FILTER_FREQ));

        HighCutRes->setValue(fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS,
                                                            channel, GET_FILTER_RES));  
    }

    emit spinChan->valueChanged(channel);
    spinChan->setValue(channel);
    update();
}

#endif
