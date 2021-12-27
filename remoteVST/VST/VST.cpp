/*
 * MidiEditor
 * Copyright (C) 2010  Markus Schwenk
 *
 * VST.cpp
 * Copyright (C) 2021 Francisco Munoz / "Estwald"
 * Copyright (C) Dominic Mazzoni
 * Copyright (C) Audacity: A Digital Audio Editor
 * This code is partially based in VSTEffect.cpp from Audacity
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "VST.h"
#include <QDebug>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QSharedMemory>
#include <QCloseEvent>
#include <QSystemSemaphore>

int flag_exit = 0;
QSharedMemory *sharedVSText = NULL;
QSharedMemory *sharedAudioBuffer = NULL;

QSystemSemaphore * sys_sema_in = NULL;
QSystemSemaphore * sys_sema_out = NULL;
QSemaphore *sema = NULL;
extern QSystemSemaphore * sys_sema_inW;
extern QSemaphore *sVST_EXT;

#define FLUID_OUT_SAMPLES 2048
int fluid_output_sample_rate = 44100;
int fluid_output_fluid_out_samples = 512;

QWidget *main_widget = NULL;

#define DEBUG_OUT(x) //qDebug("VST_PROC::%s", x)
#define DEBUG_OUT2(x) //qDebug("VST_CHAN::%s", x)

typedef AEffect *(*vstPluginMain)(audioMasterCallback audioMaster);

static int first_time = 1;

static int vst_mix_disable = 0;
static int _block_timer = 0;
static int _block_timer2 = 0;

static QWidget *_parentS = NULL;

static float **out_vst;

static QString VST_directory;

#define OUT_CH(x) (x & 15)

VST_preset_data_type *VST_preset_data[PRE_CHAN + 1];

#define VST_ON(x) (VST_preset_data[x] && VST_preset_data[x]->on)

static int _init_2 = 1;
static int waits = 0;

class VstEvents2
{
public:
   // 00
   int numEvents;
   // 04
   void *reserved;
   // 08
   VstEvent* events[16];

} ;

static VstEvents2 *vstEvents[PRE_CHAN + 1];


VSTDialog::VSTDialog(QWidget* parent, int chan) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint) {

    QDialog *Dialog = this;
    channel = chan;
    _parent = parent;
    _dis_change = false;

    semaf = new QSemaphore(1);
    if(!semaf) {
        qFatal("VSTDialog: new semaf fail\n");
        exit(-1);
    }

    Dialog->setStyleSheet(QString::fromUtf8("background-color: black;"));
    Dialog->setStyleSheet(QString::fromUtf8("background-color: #FF000040;"));

    if (Dialog->objectName().isEmpty())
        Dialog->setObjectName(QString::fromUtf8("VSTDialog"));
    Dialog->resize(400, 64);

    scrollArea = new QScrollArea(Dialog);
    scrollArea->setObjectName(QString::fromUtf8("scrollArea"));
    scrollArea->setGeometry(QRect(0, 41, 400, 80));
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    subWindow = new QWidget();
    subWindow->setObjectName(QString::fromUtf8("scrollAreaWidgetContents"));
    subWindow->setGeometry(QRect(0, 0, 400, 80));
    //subWindow->setStyleSheet(QString::fromUtf8("background-color: black;"));
    scrollArea->setWidget(subWindow);
    scrollArea->horizontalScrollBar()->setStyleSheet(QString::fromUtf8("background-color: #E5E5E5;"));
    scrollArea->verticalScrollBar()->setStyleSheet(QString::fromUtf8("background-color: #E5E5E5;"));

    groupBox = new QGroupBox(Dialog);
    if(chan == PRE_CHAN) groupBox->setEnabled(false);
    groupBox->setStyleSheet(QString::fromUtf8("background-color: #E5E5E5;"));
    groupBox->setObjectName(QString::fromUtf8("groupBox"));

    groupBox->setGeometry(QRect(10, 0, 440 + 180, 41));

    int x = 10, y = 10;
    pushButtonSave = new QPushButton(groupBox);
    pushButtonSave->setObjectName(QString::fromUtf8("pushButtonSave"));
    pushButtonSave->setGeometry(QRect(x, y, 75, 23));
    pushButtonSave->setText("Save");
    pushButtonSave->setToolTip("Save the current preset");
    x+=80;
    pushButtonReset = new QPushButton(groupBox);
    pushButtonReset->setObjectName(QString::fromUtf8("pushButtonReset"));
    pushButtonReset->setGeometry(QRect(x, y, 75, 23));
    pushButtonReset->setText("Reset");
    pushButtonReset->setToolTip("set factory preset");
    x+=80;
    pushButtonDelete = new QPushButton(groupBox);
    pushButtonDelete->setObjectName(QString::fromUtf8("pushButtonDelete"));
    pushButtonDelete->setGeometry(QRect(x, y, 75, 23));
    pushButtonDelete->setText("Delete");
    pushButtonDelete->setToolTip("delete the current preset");
    x+=100;
    pushButtonSet = new QPushButton(groupBox);
    pushButtonSet->setObjectName(QString::fromUtf8("pushButtonSet"));
    pushButtonSet->setGeometry(QRect(x, y, 75, 23));
    pushButtonSet->setText("Set Preset");
    pushButtonSet->setToolTip("Set the current preset in cursor position");
    x+=80;
    pushButtonDis = new QPushButton(groupBox);
    pushButtonDis->setObjectName(QString::fromUtf8("pushButtonDis"));
    pushButtonDis->setGeometry(QRect(x, y, 75, 23));
    pushButtonDis->setText("Dis Preset");
    pushButtonDis->setToolTip("Disable VST plugin from cursor position");
    x+=80;
    pushButtonUnset = new QPushButton(groupBox);
    pushButtonUnset->setObjectName(QString::fromUtf8("pushButtonUnset"));
    pushButtonUnset->setGeometry(QRect(x, y, 75, 23));
    pushButtonUnset->setText("Unset Preset");
    pushButtonUnset->setToolTip("Remove presets in cursor position");
    x+=80;
    labelPreset = new QLabel(groupBox);
    labelPreset->setObjectName(QString::fromUtf8("labelPreset"));
    labelPreset->setGeometry(QRect(x, y, 47, 21));
    x+=50;

    SpinBoxPreset = new QSpinBox(groupBox);
    SpinBoxPreset->setObjectName(QString::fromUtf8("SpinBoxPreset"));
    SpinBoxPreset->setGeometry(QRect(x, y - 10, 51, 41));
    QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(SpinBoxPreset->sizePolicy().hasHeightForWidth());
    SpinBoxPreset->setSizePolicy(sizePolicy);
    QFont font;
    font.setPointSize(10);
    SpinBoxPreset->setFont(font);
    SpinBoxPreset->setFocusPolicy(Qt::NoFocus);
    SpinBoxPreset->setStyleSheet(QString::fromUtf8("background-color: white;"));

#if QT_CONFIG(accessibility)
    SpinBoxPreset->setAccessibleName(QString::fromUtf8(""));
#endif // QT_CONFIG(accessibility)
    SpinBoxPreset->setLayoutDirection(Qt::LeftToRight);
    SpinBoxPreset->setAlignment(Qt::AlignCenter);
    SpinBoxPreset->setMaximum(7);
    SpinBoxPreset->setValue(VST_ON(channel) ? VST_preset_data[channel]->curr_preset : 0);
    SpinBoxPreset->setToolTip("Change the current preset");

    if(channel == PRE_CHAN)
        Dialog->setWindowTitle("VST Pluging pre-loaded (32 bits)");
    else
        Dialog->setWindowTitle("VST channel #" + QString().number(channel & 15) + " plugin " + QString().number(channel/16 + 1) + " (32 bits)");

    groupBox->setTitle(QString());

    labelPreset->setText("Preset:");

    connect(pushButtonSave, SIGNAL(clicked()), this, SLOT(Save()));
    connect(pushButtonReset, SIGNAL(clicked()), this, SLOT(Reset()));
    connect(pushButtonDelete, SIGNAL(clicked()), this, SLOT(Delete()));
    connect(pushButtonSet, SIGNAL(clicked()), this, SLOT(Set()));
    connect(pushButtonDis, SIGNAL(clicked()), this, SLOT(Dis()));
    connect(pushButtonUnset, SIGNAL(clicked()), this, SLOT(Unset()));

    connect(SpinBoxPreset, SIGNAL(valueChanged(int)), this, SLOT(ChangeFastPresetI(int)));
    connect(this, SIGNAL(setPreset(int)), this, SLOT(ChangeFastPresetI2(int)));
    connect(this, SIGNAL(setBackColor(int)), this, SLOT(BackColor(int)));
    QMetaObject::connectSlotsByName(Dialog);

    time_updat= new QTimer(this);
    time_updat->setSingleShot(false);

    connect(time_updat, SIGNAL(timeout()), this, SLOT(timer_update()), Qt::DirectConnection);
    time_updat->setSingleShot(false);
    time_updat->start(50);

}

void VSTDialog::Save() {
    sVST_EXT->acquire();

    int * dat = ((int *) sharedVSText->data());
    sharedVSText->lock();
    dat[0] = 0x666;
    dat[1] = channel;
    dat[2] = 1;
    dat[3] = VST_preset_data[channel]->curr_preset;
    sharedVSText->unlock();
    sys_sema_inW->release();
    QThread::msleep(50);
    sVST_EXT->release();
}

void VSTDialog::Reset() {

    sVST_EXT->acquire();
    int * dat = ((int *) sharedVSText->data());
    sharedVSText->lock();
    dat[0] = 0x666;
    dat[1] = channel;
    dat[2] = 2;
    dat[3] = VST_preset_data[channel]->curr_preset;
    sharedVSText->unlock();
    sys_sema_inW->release();
    QThread::msleep(50);
    sVST_EXT->release();
}

void VSTDialog::Delete() {

    if(!VST_ON(channel)) return;
    if(!VST_preset_data[channel]->vstEffect) return;

   // setStyleSheet(QString::fromUtf8("background-color: #E5E5E5;"));
    int r = QMessageBox::question(_parentS, "Delete preset", "Are you sure?                         ");
    setStyleSheet(QString::fromUtf8("background-color: #FF000040;"));
    if(r != QMessageBox::Yes) return;

    sVST_EXT->acquire();
    int * dat = ((int *) sharedVSText->data());
    sharedVSText->lock();
    dat[0] = 0x666;
    dat[1] = channel;
    dat[2] = 3;
    dat[3] = VST_preset_data[channel]->curr_preset;
    sharedVSText->unlock();
    sys_sema_inW->release();
    QThread::msleep(50);
    sVST_EXT->release();
}

void VSTDialog::Set() {
    sVST_EXT->acquire();
    int * dat = ((int *) sharedVSText->data());
    sharedVSText->lock();
    dat[0] = 0x666;
    dat[1] = channel;
    dat[2] = 4;
    dat[3] = VST_preset_data[channel]->curr_preset;
    sharedVSText->unlock();
    sys_sema_inW->release();
    QThread::msleep(50);
    sVST_EXT->release();
}

void VSTDialog::Dis() {
    sVST_EXT->acquire();
    int * dat = ((int *) sharedVSText->data());
    sharedVSText->lock();
    dat[0] = 0x666;
    dat[1] = channel;
    dat[2] = 5;
    dat[3] = VST_preset_data[channel]->curr_preset;
    sharedVSText->unlock();
    sys_sema_inW->release();
    QThread::msleep(50);
    sVST_EXT->release();
}

void VSTDialog::Unset() {
    sVST_EXT->acquire();
    int * dat = ((int *) sharedVSText->data());
    sharedVSText->lock();
    dat[0] = 0x666;
    dat[1] = channel;
    dat[2] = 6;
    dat[3] = VST_preset_data[channel]->curr_preset;
    sharedVSText->unlock();
    sys_sema_inW->release();
    QThread::msleep(50);
    sVST_EXT->release();
}

void VSTDialog::closeEvent(QCloseEvent *event) {

    if (event->spontaneous()) {
        int * dat = ((int *) sharedVSText->data());
        sharedVSText->lock();
        dat[0] = 0x666;
        dat[1] = channel;
        dat[2] = 255;
        sharedVSText->unlock();
        sys_sema_inW->release();
        QThread::msleep(50);
        sVST_EXT->release();
    } else {
        QWidget::closeEvent(event);
    }

}

void VSTDialog::BackColor(int sel) {
    if(sel < 0 || sel > 7)
        SpinBoxPreset->setStyleSheet(QString::fromUtf8("background-color: lightgray;"));
    else if(sel == 0)
        SpinBoxPreset->setStyleSheet(QString::fromUtf8("background-color: white;"));
    else
        SpinBoxPreset->setStyleSheet(QString::fromUtf8("background-color: #8000C000;"));

}

void VSTDialog::ChangeFastPresetI(int sel) {
    if(!VST_ON(channel)) return;
    if(_dis_change || !SpinBoxPreset->underMouse()) return;
    VST_preset_data[channel]->send_preset = -1;

    if(sel < 0 || sel > 7) {
        SpinBoxPreset->setStyleSheet(QString::fromUtf8("background-color: lightgray;"));
        return;
    } else {
        int clen = VST_preset_data[channel]->preset[sel].length();

        if(!clen)
            SpinBoxPreset->setStyleSheet(QString::fromUtf8("background-color: white;"));
        else
            SpinBoxPreset->setStyleSheet(QString::fromUtf8("background-color: #8000C000;"));

    }

    ChangeFastPreset(sel);

    sVST_EXT->acquire();
    int * dat = ((int *) sharedVSText->data());
    dat[0] = 0x666;
    dat[1] = channel;
    dat[2] = 7;
    dat[3] = sel;
    sys_sema_inW->release();
    QThread::msleep(50);
    sVST_EXT->release();
}


void VSTDialog::ChangeFastPresetI2(int sel) {

    if(!VST_ON(channel)) return;

    if(sel < 0 || sel > 7) {
        SpinBoxPreset->setStyleSheet(QString::fromUtf8("background-color: lightgray;"));
        return;
    } else {
        int clen = VST_preset_data[channel]->preset[sel].length();

        if(!clen) {
            SpinBoxPreset->setStyleSheet(QString::fromUtf8("background-color: white;"));
        }
        else {

            SpinBoxPreset->setStyleSheet(QString::fromUtf8("background-color: #8000C000;"));
        }

    }

    _dis_change = true;

    SpinBoxPreset->setValue(sel);

    _dis_change = false;
}

void VSTDialog::ChangeFastPreset(int sel) {

    if(sel < 0 || sel > 7) return;

    if(!VST_ON(channel)) return;
    if(channel == PRE_CHAN || !VST_preset_data[channel]->vstEffect) return;

    semaf->acquire(1);

    int chan = channel;

    qDebug("ChangeFastPreset(%i)", chan);

    VST_preset_data[chan]->curr_preset = sel;

    int clen = 0;

    int pre = VST_preset_data[chan]->curr_preset;

    char *data2 = VST_preset_data[chan]->preset[pre].data();
    clen = VST_preset_data[chan]->preset[pre].length();

    _block_timer2 = 1;

    if(!clen) {
        semaf->release(1);
        return;
    }

    if(VST_preset_data[chan]->type_data == 0) {

        VstPatchChunkInfo info;

        memset(&info, 0, sizeof(info));
        info.version = 1;
        info.pluginUniqueID = VST_preset_data[chan]->uniqueID;
        info.pluginVersion = VST_preset_data[chan]->version;
        info.numElements =  VST_preset_data[chan]->numParams;

        // Ask the effect if this is an acceptable program
        if (VST_proc::dispatcher(chan, effBeginLoadProgram, 0, 0, (void *) &info, 0.0) == -1) {
            _block_timer2 = 0;
            semaf->release(1);
            return;
        }

        VST_proc::dispatcher(chan, effBeginSetProgram, 0, 0, NULL, 0.0);
        VST_proc::dispatcher(chan, effSetChunk, 1 , clen, data2, 0.0);
        VST_proc::dispatcher(chan, effEndSetProgram, 0, 0, NULL, 0.0);

        VST_proc::dispatcher(chan, effEditIdle, 0, 0, NULL, 0.0f);

    } else {

        if(VST_preset_data[chan]->numParams != clen/4) {
            _block_timer2 = 0;
            semaf->release(1);
            return;
        }

        for (int i = 0; i < VST_preset_data[chan]->numParams; i++) {
            //float f;
            //memcpy(&f, &data2[i * 4], sizeof(float));
            VST_proc::setParameter(chan, i, /*f*/ *((float *) &data2[i * 4]));
        }

        VST_proc::dispatcher(channel, effEditIdle, 0, 0, NULL, 0.0f);

    }

    _block_timer2 = 0;
    semaf->release(1);

}


void VSTDialog::timer_update() {

    if(_block_timer || _block_timer2) return;
    if(!VST_ON(channel)) return;
    if(!VST_preset_data[channel]->vstEffect) return;
    if(VST_preset_data[channel]->disabled || VST_preset_data[channel]->closed) return;

    semaf->acquire(1);

    if(VST_preset_data[channel]->vstVersion >= 2 && VST_preset_data[channel]->needIdle) {
        int r = 0;

        r = VST_proc::dispatcher(channel, effIdle, 0, 0, NULL, 0.0f);
        if(!r) VST_preset_data[channel]->needIdle = false;
    }


    VST_proc::dispatcher(channel, effEditIdle, 0, 0, NULL, 0.0f);

    if(VST_preset_data[channel]->needUpdate)
        update();
    VST_preset_data[channel]->needUpdate = false;

    semaf->release(1);
}


VSTDialog::~VSTDialog() {

    time_updat->stop();
    delete time_updat;
    if(semaf) delete semaf;
    hide();

}

/////////////////////////////////////////////////////////////////////////////////////////
// VST_PROC


intptr_t AudioMaster(AEffect * effect,
                     int32_t opcode,
                     int32_t index,
                     intptr_t value,
                     void * ptr,
                     float opt){
    int mCurrentEffectID = 0;

    VST_preset_data_type * VSTpre = NULL;

    if(effect) {
        VSTpre = (VST_preset_data_type *) effect->ptr2;
    }

    // Handles operations during initialization...before VSTEffect has had a
    // chance to set its instance pointer.
    switch (opcode)
    {
    case audioMasterVersion:
        return (intptr_t) 2400;

    case audioMasterCurrentId:
        return mCurrentEffectID;

    case audioMasterGetVendorString:
        strcpy((char *) ptr, "midieditor");    // Do not translate, max 64 + 1 for null terminator
        return 1;

    case audioMasterGetProductString:
        strcpy((char *) ptr, "midieditor");         // Do not translate, max 64 + 1 for null terminator
        return 1;

    case audioMasterGetVendorVersion:
        return (intptr_t) (1 << 24 |
                           0 << 16 |
                           0 << 8 |
                           0);

        // Some (older) effects depend on an effIdle call when requested.  An
        // example is the Antress Modern plugins which uses the call to update
        // the editors display when the program (preset) changes.
    case audioMasterNeedIdle:

        if (VSTpre){
            VSTpre->needIdle = true;
            return 1;
        }

        return 0;

        // We would normally get this if the effect editor is dipslayed and something "major"
        // has changed (like a program change) instead of multiple automation calls.
        // Since we don't do anything with the parameters while the editor is displayed,
        // there's no need for us to do anything.

    case audioMasterUpdateDisplay:
        if (VSTpre){
            VSTpre->needUpdate = true;
            return 1;
        }
        return 0;

        // Return the current time info.
    case audioMasterGetTime:
        return 0;

        // Inputs, outputs, or initial delay has changed...all we care about is initial delay.
    case audioMasterIOChanged:

        return 0;

    case audioMasterGetSampleRate:

        return 0;
    case audioMasterIdle:
        QThread::usleep(1);

        return 1;

    case audioMasterGetCurrentProcessLevel:

        return 0;

    case audioMasterGetLanguage:
        return kVstLangEnglish;

        // We always replace, never accumulate
    case audioMasterWillReplaceOrAccumulate:
        return 1;

        // Resize the window to accommodate the effect size
    case audioMasterSizeWindow:
        // size index (w) value(h)
        if (VSTpre){
            VST_proc::VST_Resize(((VSTDialog *) VSTpre->vstWidget)->channel, index, value);
        }
        return 0;

    case audioMasterCanDo:
    {
        char *s = (char *) ptr;
        if (strcmp(s, "acceptIOChanges") == 0 ||
                strcmp(s, "sendVstTimeInfo") == 0 ||
                strcmp(s, "startStopProcess") == 0 ||
                strcmp(s, "shellCategory") == 0 ||
                strcmp(s, "sizeWindow") == 0)
        {
            return 1;
        }

        return 0;
    }

    case audioMasterBeginEdit:
    case audioMasterEndEdit:
        return 0;

    case audioMasterAutomate:

        return 0;

        // We're always connected (sort of)
    case audioMasterPinConnected:

        // We don't do MIDI yet
    case audioMasterWantMidi:
    case audioMasterProcessEvents:

        // Don't need to see any messages about these
        return 0;
    }


    return 0;
}

void VST_proc::VST_PowerOn(int chan)
{
    DEBUG_OUT("VST_PowerOn");

    if(!VST_preset_data[chan] || !VST_preset_data[chan]->vstEffect) return;

    _block_timer2 = 1;

    if (!VST_preset_data[chan]->vstPowerOn) {
        // Turn the power on
        VST_proc::dispatcher(chan, effMainsChanged, 0, 1, NULL, 0.0);

        // Tell the effect we're going to start processing
        if (VST_preset_data[chan]->vstVersion >= 2)
        {
           // qDebug("VST_PowerOn()");

            VST_proc::dispatcher(chan, effStartProcess, 0, 0, NULL, 0.0);
        }

        // Set state
        VST_preset_data[chan]->vstPowerOn = true;
    }

    _block_timer2 = 0;
}

void VST_proc::VST_PowerOff(int chan)
{
    DEBUG_OUT("VST_PowerOff");

    if(!VST_preset_data[chan] || !VST_preset_data[chan]->vstEffect) return;

    _block_timer2 = 1;

    if (VST_preset_data[chan]->vstPowerOn) {
        // Tell the effect we're going to stop processing
        if (VST_preset_data[chan]->vstVersion >= 2)
        {
            //qDebug("VST_PowerOff()");
            VST_proc::dispatcher(chan, effStopProcess, 0, 0, NULL, 0.0);
        }

        // Turn the power off
        VST_proc::dispatcher(chan, effMainsChanged, 0, 0, NULL, 0.0);

        // Set state
        VST_preset_data[chan]->vstPowerOn = false;
    }

    _block_timer2 = 0;
}

bool VST_proc::VST_isShow(int chan) {

    DEBUG_OUT("VST_isShow");

    if(VST_preset_data[chan] && VST_preset_data[chan]->vstEffect && VST_preset_data[chan]->vstWidget) {
        return VST_preset_data[chan]->vstWidget->isVisible();
    }

    return false;
}


void VST_proc::VST_show(int chan, bool show) {

    DEBUG_OUT("VST_show");

    if(!VST_preset_data[chan] ||
        !VST_preset_data[chan]->vstEffect ||
            !VST_preset_data[chan]->vstWidget) return;

    _block_timer2 = 1;

    if(VST_preset_data[chan]->vstEffect) {
        if(!VST_preset_data[chan]->closed)
            VST_proc::dispatcher(chan, effEditClose, 0, 0, (void *)  VST_preset_data[chan]->vstWidget->winId(), 0.0);
        VST_preset_data[chan]->closed = true;
        if(show) {
            VST_proc::dispatcher(chan, effEditOpen, 0, 0, (void *)  VST_preset_data[chan]->vstWidget->winId(), 0.0);
            VST_preset_data[chan]->closed = false;
            //VST_preset_data[chan]->vstWidget->show();
            VST_preset_data[chan]->vstWidget->showMinimized();
            VST_preset_data[chan]->vstWidget->raise();
            VST_preset_data[chan]->vstWidget->showNormal();
        } else
            VST_preset_data[chan]->vstWidget->close();

    }

    _block_timer2 = 0;

}

int VST_proc::VST_exit() {

    if(first_time) return 0;

    DEBUG_OUT("VST_exit");

    vst_mix_disable = 1;

    while(1) {

        if(!_block_timer) break;
        else QThread::msleep(50);
    }

    VST_proc::VST_MIDIend();

    for(int n = 0; n <= PRE_CHAN; n++) {
        VST_proc::VST_unload(n);
    }

    if(out_vst) {
        for(int n = 0; n < 16 * 2; n++) {
            if(out_vst[n]) delete out_vst[n];
            out_vst[n] = NULL;
        }

        delete out_vst;
        out_vst = NULL;
    }
    return 0;
}

void VST_proc::VST_MIDIend() {

    if(_init_2) return;

    DEBUG_OUT("VST_MIDIend");

    for(int n = 0; n < PRE_CHAN + 1; n++) {

        if(vstEvents[n]) {
            for(int n = 0; n < vstEvents[n]->numEvents; n++)
                delete vstEvents[n]->events[n];

            delete vstEvents[n];
            vstEvents[n] = NULL;
        }

    }

}

void VST_proc::VST_MIDInotesOff(int chan) {

    DEBUG_OUT("VST_MIDInotesOff");

    for(; chan < PRE_CHAN; chan+= 16) {

        if(VST_ON(chan) && (VST_preset_data[chan]->type & 1)) {

            QByteArray cmd;
            for(int m = 0; m < 127; m++) {

                cmd.clear();
                cmd.append((char ) 0x80);
                cmd.append((char ) m);
                cmd.append((char ) 0x0);

                VST_proc::VST_MIDIcmd(chan, 0, cmd);

                _block_timer2 = 1;

                if(vstEvents[chan]) {
                    vstEvents[PRE_CHAN] = vstEvents[chan];
                    vstEvents[chan] = NULL;
                    VST_proc::dispatcher(chan,  effProcessEvents, 0, 0, vstEvents[PRE_CHAN], 0);
                    VST_proc::dispatcher(chan, effEditIdle, 0, 0, NULL, 0.0f);
                    if(vstEvents[PRE_CHAN]) {
                        for(int n = 0; n < vstEvents[PRE_CHAN]->numEvents; n++)
                            delete vstEvents[PRE_CHAN]->events[n];

                        delete vstEvents[PRE_CHAN];
                        vstEvents[PRE_CHAN] = NULL;
                    }

                    VST_proc::processReplacing(chan, &out_vst[OUT_CH(chan) * 2], &out_vst[OUT_CH(chan) * 2], fluid_output_fluid_out_samples);
                }

                _block_timer2 = 0;

            }
        }
    }

}

bool VST_proc::VST_isMIDI(int chan) {

  //DEBUG_OUT("VST_isMIDI");
    for(; chan < PRE_CHAN; chan+= 16) {
        if(VST_ON(chan) && (VST_preset_data[chan]->type & 1)) return true;
    }

    return false;

}

void VST_proc::VST_MIDIcmd(int chan, int deltaframe, QByteArray cmd) {

    DEBUG_OUT("VST_MIDIcmd");

    if(_init_2) {
        _init_2 = 0;
        for(int n = 0; n < PRE_CHAN + 1; n++)
            vstEvents[n] = NULL;
    }

    for(; chan < PRE_CHAN; chan+= 16) {

        if(!VST_ON(chan) || !(VST_preset_data[chan]->type & 1)) continue;

        waits = 1;

        if(!vstEvents[chan]) {
            vstEvents[chan] = new VstEvents2();
            vstEvents[chan]->numEvents = 0;
            vstEvents[chan]->events[0] = NULL;
        }


        int pos = vstEvents[chan]->numEvents;
        if(pos > 15) {
            waits = 0;
            return; // no more notes
        }

        vstEvents[chan]->events[pos] = new VstEvent();

        VstMidiEvent *event = (VstMidiEvent*)vstEvents[chan]->events[pos];

        event->byteSize = sizeof(*event);
        event->type = 1;
        event->flags = 1;
        event->noteLength = 0;
        event->noteOffset = 0;
        event->detune = 0;
        // 1d?
        event->noteOffVelocity = 0;
        event->deltaFrames = deltaframe;
        event->midiData[0] = cmd.at(0); // note on
        event->midiData[1] = cmd.at(1);
        event->midiData[2] = cmd.at(2); // velocity
        event->midiData[3] = 0;
        vstEvents[chan]->numEvents++;
        waits = 0;
    }
}


intptr_t VST_proc::dispatcher(int chan, int b, int c, intptr_t d, void * e, float f) {
    if(!VST_preset_data[chan] || !VST_preset_data[chan]->vstEffect || !VST_preset_data[chan]->mux) return (intptr_t) NULL;

    AEffect *temp = VST_preset_data[chan]->vstEffect;

    QMutex *mux= VST_preset_data[chan]->mux;
    if(mux)
        mux->lock();
    else
        return (intptr_t) NULL;

    intptr_t r = temp ? temp->dispatcher(temp, b, c, d, e, f) : (intptr_t) NULL;

    mux->unlock();

    return r;
}

void VST_proc::process(int chan, float * * b, float * * c, int d) {
    if(!VST_preset_data[chan] || !VST_preset_data[chan]->vstEffect || !VST_preset_data[chan]->mux) return;

    AEffect *temp = VST_preset_data[chan]->vstEffect;

    QMutex *mux= VST_preset_data[chan]->mux;
    if(mux)
        mux->lock();
    else
        return;

    if(temp)
        temp->process(temp, b, c, d);

    mux->unlock();

}

void VST_proc::processReplacing(int chan, float * * b, float * * c, int d) {
    if(!VST_preset_data[chan] || !VST_preset_data[chan]->vstEffect || !VST_preset_data[chan]->mux) return;

    AEffect *temp = VST_preset_data[chan]->vstEffect;

    QMutex *mux= VST_preset_data[chan]->mux;
    if(mux)
        mux->lock();
    else
        return;

    if(temp)
        temp->processReplacing(temp, b, c, d);

    mux->unlock();
}

void VST_proc::setParameter(int chan, int b, float c) {
    if(!VST_preset_data[chan] || !VST_preset_data[chan]->vstEffect || !VST_preset_data[chan]->mux) return;

    AEffect *temp = VST_preset_data[chan]->vstEffect;

    QMutex *mux= VST_preset_data[chan]->mux;
    if(mux)
        mux->lock();
    else
        return;

    if(temp)
        temp->setParameter(temp, b, c);

    mux->unlock();
}

float VST_proc::getParameter(int chan, int b) {
    if(!VST_preset_data[chan] || !VST_preset_data[chan]->vstEffect || !VST_preset_data[chan]->mux) return 0.0f;

    AEffect *temp = VST_preset_data[chan]->vstEffect;

    QMutex *mux= VST_preset_data[chan]->mux;
    if(mux)
        mux->lock();
    else
        return 0.0f;

    float r = temp ? temp->getParameter(temp, b) : 0.0f;

    mux->unlock();

    return r;
}

int VST_proc::VST_unload(int chan) {

    DEBUG_OUT("VST_unload");

    if(!VST_ON(chan) || !VST_preset_data[chan]->vstEffect) return -1;


    VST_preset_data[chan]->on = false;
    VST_preset_data[chan]->disabled = true;
    QMutex *mux = VST_preset_data[chan]->mux;
    VST_preset_data[chan]->mux = NULL;
    if(mux) mux->lock(); // lock or wait to lock

    VST_preset_data_type * VST_temp = VST_preset_data[chan];
    VST_temp->vstEffect->ptr2 = NULL;


    if(VST_preset_data[chan]->vstEffect) {
        VST_PowerOff(chan);
    }

    _block_timer2 = 1;

    VST_preset_data[chan] = NULL;
#ifdef CACA2
    if(fluid_control && chan < PRE_CHAN)
        fluid_control->wicon[chan]->setVisible(false);
#endif

    if(VST_temp->vstEffect) {

        AEffect *temp = VST_temp->vstEffect;
        VST_temp->vstEffect = NULL;

        if(!VST_temp->closed)
           temp->dispatcher(temp, effEditClose, 0, 0, (void *) VST_temp->winid/*VST_temp->vstWidget*/, 0.0);

        VST_temp->closed = true;
        if(VST_temp->vstWidget) VST_temp->vstWidget->close();

        temp->dispatcher(temp, effClose, 0, 0, NULL, 0.0);
    }

    if(mux) {
        mux->unlock();
        delete mux;
    }

    if(VST_temp->vstLib) {

        VST_temp->vstLib->blockSignals(true);

        VST_temp->vstLib->unload(); //it crash in some VST modules

        delete VST_temp->vstLib;
        VST_temp->vstLib = NULL;
    }

    if(VST_temp->vstWidget) {
        VST_temp->vstWidget->close();
        delete VST_temp->vstWidget;
        VST_temp->vstWidget = NULL;
    }


    if(VST_temp) delete VST_temp;

    _block_timer2 = 0;

    return 0;
}

int VST_proc::VST_isLoaded(int chan) {

    DEBUG_OUT("VST_isLoaded");

    if(VST_ON(chan)
        && VST_preset_data[chan]->vstWidget
        && VST_preset_data[chan]->vstLib) return 1;

    return 0;
}

int VST_proc::VST_load(int chan, const QString pathModule) {

    DEBUG_OUT("VST_load");

    vst_mix_disable = 1;

    while(1) {

        if(!_block_timer) break;
        else QThread::msleep(50);
    }

    if(first_time) {

        if(VST_directory == "") {
            VST_directory = QDir::homePath();
            /*
            if(fluid_output->fluid_settings->value("VST_directory").toString() != "")
                VST_directory = fluid_output->fluid_settings->value("VST_directory").toString();
                */

        }

        out_vst = new float*[16 * 2];
        for(int n = 0; n < 16 * 2; n++) {
            out_vst[n] = new float[FLUID_OUT_SAMPLES + 256];
        }

        for(int n = 0; n < PRE_CHAN + 1; n++) {

            VST_preset_data[n] = NULL;
        }

        first_time = 0;
    }

    if(!_parentS) return -1;

    VST_proc::VST_unload(chan);

    if(VST_preset_data[chan]) {
        qFatal("Error!nVST_preset_data[%i] defined", chan);
        delete VST_preset_data[chan];
        VST_preset_data[chan] = NULL;
    }

    VST_preset_data_type *VST_temp = new VST_preset_data_type;
    VST_temp->on = false;

    VST_temp->vstPowerOn = false;
    VST_temp->vstVersion = 0;
    VST_temp->vstEffect = NULL;
    VST_temp->vstWidget = NULL;
    VST_temp->vstLib = NULL;
    VST_temp->send_preset = -1;
    VST_temp->mux = NULL;

    VST_temp->vstLib = new QLibrary(pathModule, _parentS);

    if(!VST_temp->vstLib) {
        vst_mix_disable = 0;
        return -1;
    }

    //VST_preset_data[chan]->vstLib->loadHints();

    if (!VST_temp->vstLib->load()) {
        delete VST_temp->vstLib;
        delete VST_temp;
        vst_mix_disable = 0;
        return -2;
    }

    vstPluginMain pluginMain = (vstPluginMain) VST_temp->vstLib->resolve("VSTPluginMain");
    if (!pluginMain) pluginMain = (vstPluginMain) VST_temp->vstLib->resolve("main");
    if (!pluginMain) {
        VST_temp->vstLib->unload();
        delete VST_temp->vstLib;
        delete VST_temp;
        vst_mix_disable = 0;
        return -3;
    }

    _block_timer2 = 1;

    if (pluginMain) {

        VST_temp->curr_preset = 0;
        VST_temp->disabled = false;
        VST_temp->closed = false;
        VST_temp->needIdle = false;
        VST_temp->needUpdate = false;
        VST_temp->vstPowerOn = false;
        VST_temp->mux = new QMutex(QMutex::NonRecursive);

        VST_preset_data[chan] = VST_temp;

        VST_preset_data[chan]->vstEffect = pluginMain(AudioMaster);

        // Was it successful?
        if (VST_preset_data[chan]->vstEffect) {
            // Save a reference to ourselves
            //
            // Note:  Some hosts use "user" and some use "ptr2/resvd2".  It might
            //        be worthwhile to check if user is NULL before using it and
            //        then falling back to "ptr2/resvd2".
            VST_preset_data[chan]->vstEffect->ptr2 = VST_preset_data[chan];

            _block_timer2 = 1;

            // Give the plugin an initial sample rate and blocksize
            VST_preset_data[chan]->vstEffect->dispatcher(VST_preset_data[chan]->vstEffect, effSetSampleRate, 0, 0, NULL, 44100.0);
            VST_preset_data[chan]->vstEffect->dispatcher(VST_preset_data[chan]->vstEffect, effSetBlockSize, 0, 512, NULL, 0);

            VST_preset_data[chan]->vstEffect->dispatcher(VST_preset_data[chan]->vstEffect, effIdentify, 0, 0, NULL, 0);

            // Open the plugin
            VST_preset_data[chan]->vstEffect->dispatcher(VST_preset_data[chan]->vstEffect, effOpen, 0, 0, NULL, 0.0);
            // Get the VST version the plugin understands
            VST_preset_data[chan]->vstVersion = VST_preset_data[chan]->vstEffect->dispatcher(VST_preset_data[chan]->vstEffect, effGetVstVersion, 0, 0, NULL, 0);
            // Give the plugin an initial sample rate and blocksize
            VST_preset_data[chan]->vstEffect->dispatcher(VST_preset_data[chan]->vstEffect, effSetSampleRate, 0, 0, NULL, (float) 44100);
            VST_preset_data[chan]->vstEffect->dispatcher(VST_preset_data[chan]->vstEffect, effSetBlockSize, 0, 2048, NULL, 0);

            //
            // Ensure that it looks like a plugin and can deal with ProcessReplacing
            // calls.  Also exclude synths for now.
            int type = 0;

            if (VST_preset_data[chan]->vstEffect->magic == kEffectMagic &&
                    !(VST_preset_data[chan]->vstEffect->flags & effFlagsIsSynth) &&
                    VST_preset_data[chan]->vstEffect->flags & effFlagsCanReplacing) {

                // Make sure we start out with a valid program selection
                // I've found one plugin (SoundHack +morphfilter) that will
                // crash Audacity when saving the initial default parameters
                // with this.
                VST_preset_data[chan]->vstEffect->dispatcher(VST_preset_data[chan]->vstEffect, effBeginSetProgram, 0, 0, NULL, 0.0);
                VST_preset_data[chan]->vstEffect->dispatcher(VST_preset_data[chan]->vstEffect, effSetProgram, 0, 0 /*index*/, NULL, 0.0);
                VST_preset_data[chan]->vstEffect->dispatcher(VST_preset_data[chan]->vstEffect, effEndSetProgram, 0, 0, NULL, 0.0);

            }

            //int mAudioIns = VST_preset_data[chan]->vstEffect->numInputs;
            //int mAudioOuts = VST_preset_data[chan]->vstEffect->numOutputs;
            //qDebug("VST in %i out %i", mAudioIns, mAudioOuts);

            if(VST_preset_data[chan]->vstEffect->flags & effFlagsIsSynth) type = 1;
            VST_preset_data[chan]->type = type;

            _block_timer2 = 0;

            VST_PowerOn(chan);

            _block_timer2 = 1;

            VstRect *rect;

            // Some effects like to have us get their rect before opening them.
            VST_preset_data[chan]->vstEffect->dispatcher(VST_preset_data[chan]->vstEffect, effEditGetRect, 0, 0, &rect, 0.0);

            VST_preset_data[chan]->vstWidget = new VSTDialog(_parentS, chan);
            VST_preset_data[chan]->vstWidget->move(0, 0);

            ((VSTDialog *) VST_preset_data[chan]->vstWidget)->subWindow->resize(rect->right, rect->bottom);

            VST_preset_data[chan]->vstEffect->dispatcher(VST_preset_data[chan]->vstEffect, effEditOpen, 0, 0, (void *)((VSTDialog *) VST_preset_data[chan]->vstWidget)->subWindow->winId(), 0.0);

            // Get the final bounds of the effect GUI
            VST_preset_data[chan]->vstEffect->dispatcher(VST_preset_data[chan]->vstEffect, effEditGetRect, 0, 0, &rect, 0.0);

            VST_proc::VST_Resize(chan, rect->right - rect->left, rect->bottom - rect->top);
            _block_timer2 = 1;

            QFile mfile(VST_preset_data[chan]->vstLib->fileName());
            QString path = QFileInfo(mfile).dir().path();
            QString name = QFileInfo(mfile).fileName();

            if(name.length() > 79) {
                vst_mix_disable = 0;
                return -1; // file name too long...
            }

            VST_preset_data[chan]->filename = QFileInfo(mfile).fileName();

            VST_preset_data[chan]->uniqueID = VST_preset_data[chan]->vstEffect->uniqueID;
            VST_preset_data[chan]->version = VST_preset_data[chan]->vstEffect->version;
            VST_preset_data[chan]->numParams = VST_preset_data[chan]->vstEffect->numParams;
            VST_preset_data[chan]->needUpdate = true;
            VST_preset_data[chan]->on = true;

            // store factory settings
            if (VST_preset_data[chan]->vstEffect->flags & effFlagsProgramChunks) {

                void *chunk = NULL;
                int clen = (int) VST_preset_data[chan]->vstEffect->dispatcher(VST_preset_data[chan]->vstEffect, effGetChunk, 1, 0, &chunk, 0.0);
                if (clen <= 0) goto nochunk;

                VST_temp->type_data = 0;

                VST_temp->factory.append((char *) chunk, clen);

            } else {
            nochunk:

                VST_temp->type_data = 1;

                for (int i = 0; i < VST_preset_data[chan]->numParams; i++) {

                    float f = VST_preset_data[chan]->vstEffect->getParameter(VST_preset_data[chan]->vstEffect, i);
                    VST_temp->factory.append((char *) &f, sizeof(float));
                }

            }

            VST_preset_data[chan] = VST_temp;

            VST_preset_data[chan]->vstEffect->ptr2 = VST_preset_data[chan];
/*
            if(fluid_control && chan < PRE_CHAN) {
                fluid_control->wicon[chan]->setVisible(true);
            }
*/
            _block_timer2 = 0;

            vst_mix_disable = 0;
            return 0;
        }
    }

    _block_timer2 = 0;
    vst_mix_disable = 0;

    return -1;

}

void VST_proc::VST_Resize(int chan, int w, int h) {

    if(!VST_preset_data[chan] || !VST_preset_data[chan]->vstWidget) return;

    ((VSTDialog *) VST_preset_data[chan]->vstWidget)->subWindow->resize(w, h);

    QDesktopWidget* d = QApplication::desktop();

    QRect clientRect = d->availableGeometry();

    int width = w;
    int height = h;

    int winWidth = width;
    int opH = ((VSTDialog *) VST_preset_data[chan]->vstWidget)->groupBox->height();
    int winHeight = height + opH;

    int scrBarH = ((VSTDialog *) VST_preset_data[chan]->vstWidget)->scrollArea->horizontalScrollBar()->height();
    int scrWidth = winWidth;
    int scrHeight = winHeight - opH;

    if(winWidth >= clientRect.width()) {
        winWidth = clientRect.width();
        scrWidth = winWidth;
    }

    if(winHeight >= clientRect.height()) {
        winHeight = clientRect.height();
        scrHeight = winHeight - opH - scrBarH;
    }


    int width2 = ((VSTDialog *) VST_preset_data[chan]->vstWidget)->groupBox->width();

    if(width2 > winWidth) {
        winWidth = width2;
        ((VSTDialog *) VST_preset_data[chan]->vstWidget)->groupBox->move(0,0);
    } else
        ((VSTDialog *) VST_preset_data[chan]->vstWidget)->groupBox->move((winWidth-width2)/2, 0);

    ((VSTDialog *) VST_preset_data[chan]->vstWidget)->resize(winWidth, winHeight);
    VST_preset_data[chan]->vstWidget->setMinimumSize(winWidth, winHeight);
    VST_preset_data[chan]->vstWidget->setFixedSize(winWidth, winHeight);

    ((VSTDialog *) VST_preset_data[chan]->vstWidget)->subWindow->setMinimumSize(width, height);
    ((VSTDialog *) VST_preset_data[chan]->vstWidget)->scrollArea->setGeometry(
                (winWidth > scrWidth) ? (winWidth - scrWidth) / 2 : 0 , opH, scrWidth, scrHeight);

    // Must get the size again since SetPeer() could cause it to change
    VST_preset_data[chan]->vstWidget->showMinimized();
    VST_preset_data[chan]->vstWidget->raise();
    VST_preset_data[chan]->vstWidget->showNormal();

}

int VST_proc::VST_mix(float**in, int nchans, int samplerate, int nsamples) {


    if(vst_mix_disable) return 0;

    _block_timer = 1;

    if(_init_2) {
        _init_2 = 0;
        for(int n = 0; n < PRE_CHAN + 1; n++)
            vstEvents[n] = NULL;
    }

    for(int chan = 0; chan < nchans; chan++) {

        if(out_vst) {
            if(!out_vst[OUT_CH(chan)]) continue;
        } else {
            _block_timer = 0;
            return -1;
        }

        if(VST_ON(chan) && VST_preset_data[chan]->vstEffect && VST_preset_data[chan]->vstPowerOn) {

            if(VST_preset_data[chan]->disabled && !VST_proc::VST_isMIDI(chan)) continue;

  #if 1
            int send_preset = VST_preset_data[chan]->send_preset;

            if(send_preset >= 0 && VST_preset_data[chan]->vstWidget) {

                //((VSTDialog *) VST_preset_data[chan]->vstWidget)->ChangeFastPreset(send_preset);
                ((VSTDialog *) VST_preset_data[chan]->vstWidget)->ChangeFastPreset(send_preset);

                VST_preset_data[chan]->send_preset = -1;
                VST_preset_data[chan]->needUpdate = true;

            }
#endif

            VST_proc::dispatcher(chan, effSetSampleRate, 0, 0, NULL, (float) samplerate);
            VST_proc::dispatcher(chan, effSetBlockSize, 0, nsamples, NULL, 0);

            if(in && !VST_preset_data[chan]->disabled && VST_proc::VST_isMIDI(chan)) {
                memset(in[OUT_CH(chan) * 2], 0, nsamples * sizeof(float));
                memset(in[OUT_CH(chan) * 2 + 1], 0, nsamples * sizeof(float));

            }

            while (1) {
                if(!waits) break;
                QThread::msleep(1);
            }

            if(VST_proc::VST_isMIDI(chan) && vstEvents[chan]) {

                vstEvents[PRE_CHAN] = vstEvents[chan];
                vstEvents[chan] = NULL;
                VST_proc::dispatcher(chan, effProcessEvents, 0, 0, vstEvents[PRE_CHAN], 0);

                if(vstEvents[PRE_CHAN]) {
                    for(int n = 0; n < vstEvents[PRE_CHAN]->numEvents; n++)
                        delete vstEvents[PRE_CHAN]->events[n];

                    delete vstEvents[PRE_CHAN];
                    vstEvents[PRE_CHAN] = NULL;
                }
            }

            memset(out_vst[OUT_CH(chan) * 2], 0, nsamples * sizeof(float));
            memset(out_vst[OUT_CH(chan) * 2 + 1], 0, nsamples * sizeof(float));
            if(!in) in = out_vst;

            if(VST_preset_data[chan]->vstEffect->flags & effFlagsCanReplacing)
                VST_proc::processReplacing(chan, &in[OUT_CH(chan) * 2], &out_vst[OUT_CH(chan) * 2], nsamples );

            else
                VST_proc::process(chan, &in[OUT_CH(chan) * 2], &out_vst[OUT_CH(chan) * 2], nsamples );

            if(VST_preset_data[chan]->disabled) continue;

            memcpy(in[OUT_CH(chan) * 2], out_vst[OUT_CH(chan) * 2], nsamples * sizeof(float));
            memcpy(in[OUT_CH(chan) * 2 + 1], out_vst[OUT_CH(chan) * 2 + 1], nsamples * sizeof(float));

        }
    }

    _block_timer = 0;

    return 0;
}

void VST_proc::VST_DisableButtons(int chan, bool disable) {

    if(!VST_preset_data[chan] || !VST_preset_data[chan]->vstWidget) return;
    ((VSTDialog *) VST_preset_data[chan]->vstWidget)->groupBox->setEnabled(!disable);

}

int VST_proc::VST_LoadParameterStream(QByteArray array) {


    char id[4]= {0x0, 0x66, 0x66, 'W'};
    int chan = array[2];
    int sel = 0;

    if(array[3] == id[1] && array[4] == id[2] && array[5] == 'W') {
       sel = array[6];
    } else return 0;

    if(!VST_ON(chan)) return -1;
    if(!VST_preset_data[chan]->vstEffect) return -1;
    if(!VST_preset_data[chan]->vstWidget) return -1;

    if(sel > 7) {
        VST_preset_data[chan]->disabled = true;
        VST_preset_data[chan]->needUpdate = true;
        VST_preset_data[chan]->send_preset = -1;

        emit ((VSTDialog *) VST_preset_data[chan]->vstWidget)->setPreset(0x7f);
        return 0;
    }

    VST_preset_data[chan]->disabled = false;
    VST_preset_data[chan]->needUpdate = true;

    VST_preset_data[chan]->send_preset = sel;

    emit ((VSTDialog *) VST_preset_data[chan]->vstWidget)->setPreset(sel);
    return 0;
}

int VST_proc::VST_LoadfromMIDIfile() {

    return 0;
}


int VST_proc::VST_UpdatefromMIDIfile() {

    return 0;
}

bool VST_proc::VST_isEnabled(int chan) {

    if(!VST_preset_data[chan]) return false;
    return !VST_preset_data[chan]->disabled;

}

int VST_proc::VST_SaveParameters(int chan)
{
    return 0;
}


void VST_proc::VST_setParent(QWidget *parent) {
    _parentS = parent;
}

VST_proc::VST_proc() {

}

VST_proc::~VST_proc() {

}


VST_IN::VST_IN(MainWindow *w) {

    win = w;

    if(!sharedVSText) exit(-11);

    sema = new QSemaphore(0);
}


VST_IN::~VST_IN() {

    DELETE(sema)
}

void VST_IN::run() {

    while(1) {
        if(flag_exit) break;
        if(!sys_sema_in->acquire()) break;
        if(flag_exit) break;
        sharedVSText->lock();
        int * dat = ((int *) sharedVSText->data()) + 0x40;

        // thread in real time

        if(dat[0] == 0x70) { // VST_MIX()
            dat[0] = 0;
            int sample_rate = dat[1];
            int nsamples = dat[2];

            int chan = 0;
            for(; chan < PRE_CHAN; chan++)
                if(VST_ON(chan)) break; // test chan active

            if(chan < PRE_CHAN) {
                float **in = new float*[32];
                for(int n = 0; n < 32; n++)
                    in[n] = ((float *) sharedAudioBuffer->data()) + nsamples * n;

               // memset((float *) sharedVSText->data(), 0, nsamples * 8);

                dat[0] = VST_proc::VST_mix(in, PRE_CHAN, sample_rate, nsamples);


                delete[] in;
            }

            sys_sema_out->release();
            sharedVSText->unlock();
            continue;
        }  else if(dat[0] == (int) 0xCACABACA) { // force exit
            dat[0] = 0;
            dat[0xFC0] = (int) 0xCACABACA;
            sharedVSText->unlock();
            msleep(200); // wait a time to Midieditor to read this
            emit force_exit();
            exit(0);
        }

        emit run_cmd_events(); // to MainWindow event to process the cmds

        sema->acquire();
        sharedVSText->unlock();
        if(!sys_sema_out->release()) break;
        if(flag_exit) break;

    }

    flag_exit = 1;
    emit run_cmd_events(); // to MainWindow event to process the cmds
}

#include "mainwindow.h"
extern QMainWindow * mainW;

void MainWindow::run_cmd_events() {

    // from VST_IN emit

    // sys_sema_in->acquire();

    if(flag_exit) {
        this->deleteLater();
        return;
    }

    int * dat = ((int *) sharedVSText->data()) + 0x40;
    sharedVSText->lock();
    int cmd = dat[0];

    dat[0] = 0; // delete old cmd

    if(cmd == 0x10) { // vst load

        int chan = dat[1];
        dat[1] = VST_proc::VST_load(chan, (char *) &dat[3]);

        if(!dat[1]) {

            dat[2] = ((VSTDialog *) VST_preset_data[chan]->vstWidget)->subWindow->winId();
            dat[3] = VST_preset_data[chan]->vstWidget->winId();
            dat[4] = VST_preset_data[chan]->version;
            dat[5] = VST_preset_data[chan]->uniqueID;
            dat[6] = VST_preset_data[chan]->type;
            dat[7] = VST_preset_data[chan]->type_data;
            dat[8] = VST_preset_data[chan]->numParams;
            QByteArray file = VST_preset_data[chan]->filename.toUtf8();
            dat[9] = file.length();
            char * name = (char *) &dat[10];
            memcpy(name, file.data(), file.length());
            name[file.length()] = 0;
        }

        sharedVSText->unlock();

    } else if(cmd == 0x11) { // vst unload
        int chan = dat[1];
        dat[1] = VST_proc::VST_unload(chan);
        sharedVSText->unlock();
    } else if(cmd == 0x12) { //get datas
        int chan = dat[1];
        if(VST_preset_data[chan]) {
            dat[1] = 0;
            dat[2] = VST_preset_data[chan]->version;
            dat[3] = VST_preset_data[chan]->uniqueID;
            dat[4] = VST_preset_data[chan]->type;
            dat[5] = VST_preset_data[chan]->type_data;
            dat[6] = VST_preset_data[chan]->numParams;
        } else
            dat[1] = -1;
        sharedVSText->unlock();
    } else if(cmd == 0x14) { //idle's cmds
        dat[1] = VST_proc::dispatcher(dat[1], dat[2], 0, 0, NULL, 0.0f);
        sharedVSText->unlock();
    } else if(cmd == 0x18) { // reset win
        int chan = dat[1];

        if(!VST_preset_data[chan]->closed)
            VST_proc::dispatcher(chan, effEditClose, 0, 0, (void *)((VSTDialog *) VST_preset_data[chan]->vstWidget)->subWindow->winId(), 0.0);
        VST_preset_data[chan]->closed = true;

        VST_proc::dispatcher(chan, effEditOpen, 0, 0, (void *)  ((VSTDialog *) VST_preset_data[chan]->vstWidget)->subWindow->winId(), 0.0);
        VST_preset_data[chan]->closed = false;

        VST_preset_data[chan]->vstWidget->showMinimized();
        VST_preset_data[chan]->vstWidget->raise();
        VST_preset_data[chan]->vstWidget->showNormal();
        sharedVSText->unlock();
    } else if(cmd == 0x19) { // SetVST() do all in external channel

        int chan = dat[1];

        VST_preset_data[PRE_CHAN]->on = false;
        VST_preset_data[PRE_CHAN]->vstWidget->close();

        VST_preset_data[chan] = VST_preset_data[PRE_CHAN];
        if(VST_preset_data[chan]->vstEffect) VST_preset_data[chan]->vstEffect->ptr2 = VST_preset_data[chan];

        VST_preset_data[PRE_CHAN] = NULL;

        ((VSTDialog *) VST_preset_data[chan]->vstWidget)->channel = chan;
        ((VSTDialog *) VST_preset_data[chan]->vstWidget)->setWindowTitle("VST channel #" + QString().number(chan & 15) + " plugin " + QString().number(chan/16 + 1) + " (32 bits)");
        VST_preset_data[chan]->on = true;
        ((VSTDialog *) VST_preset_data[chan]->vstWidget)->groupBox->setEnabled(true);

        if(VST_preset_data[chan]->closed || VST_preset_data[chan]->vstWidget->isHidden()) {

            if(!VST_preset_data[chan]->closed)
                VST_proc::dispatcher(chan, effEditClose, 0, 0, (void *)((VSTDialog *) VST_preset_data[chan]->vstWidget)->subWindow->winId(), 0.0);
            VST_proc::dispatcher(chan, effEditOpen, 0, 0, (void *)  ((VSTDialog *) VST_preset_data[chan]->vstWidget)->subWindow->winId(), 0.0);

            VST_preset_data[chan]->closed = false;
        }

        VST_preset_data[chan]->vstWidget->update();
        VST_preset_data[chan]->vstWidget->showMinimized();
        VST_preset_data[chan]->vstWidget->raise();
        VST_preset_data[chan]->vstWidget->showNormal();
        VST_preset_data[chan]->needUpdate = true;
        sharedVSText->unlock();

    } else if(cmd == 0x20) { // get preset datas
        int chan = dat[1];
        int preset = dat[2];

        if(VST_preset_data[chan]) {
            dat[1] = 0;

            if(preset < 0 || preset > 7) { // get factory
                dat[3] = VST_preset_data[chan]->factory.length();
                memcpy(&dat[4], VST_preset_data[chan]->factory.data(), dat[3]);
            } else {

                VST_preset_data[chan]->preset[preset].clear();

                // get preset settings
                if (VST_preset_data[chan]->vstEffect->flags & effFlagsProgramChunks) {

                    void *chunk = NULL;
                    int clen = (int) VST_proc::dispatcher(chan, effGetChunk, 1, 0, &chunk, 0.0);
                    if (clen <= 0) goto nochunk;

                    VST_preset_data[chan]->type_data = 0;

                    VST_preset_data[chan]->preset[preset].append((char *) chunk, clen);

                } else {
                    nochunk:

                    VST_preset_data[chan]->type_data = 1;

                    for (int i = 0; i < VST_preset_data[chan]->numParams; i++) {

                       float f = VST_proc::getParameter(chan, i);
                       VST_preset_data[chan]->preset[preset].append((char *) &f, sizeof(float));
                    }

                }

                dat[3] = VST_preset_data[chan]->preset[preset].length();
                memcpy(&dat[4], VST_preset_data[chan]->preset[preset].data(), dat[3]);
            }
        } else
            dat[1] = -1;
        sharedVSText->unlock();

    } else if(cmd == 0x21) { // set preset datas

        _block_timer2 = 1;

        int chan = dat[1];
        if(VST_preset_data[chan]) {

            dat[1] = 0;
            int preset = dat[2];
            QByteArray data;
            data.append((char *) &dat[4], dat[3]);

            if(preset < 0 || preset > 7) { // set factory
                preset = 0x7F;
                data = VST_preset_data[chan]->factory;

            } else {

                VST_preset_data[chan]->preset[preset] = data;
            }

            int sel = (data.length()!=0 && preset != 0x7F) ? 1 : 0;

            if(VST_preset_data[chan]->vstWidget && (VST_preset_data[chan]->curr_preset == preset || preset == 0x7F))
                emit ((VSTDialog *) VST_preset_data[chan]->vstWidget)->setBackColor(sel);

            if((VST_preset_data[chan]->curr_preset == preset || preset == 0x7F) &&
                    data.length()) {
                if (!VST_preset_data[chan]->type_data) {

                    VstPatchChunkInfo info;

                    memset(&info, 0, sizeof(info));
                    info.version = 1;
                    info.pluginUniqueID = VST_preset_data[chan]->uniqueID;
                    info.pluginVersion = VST_preset_data[chan]->version;
                    info.numElements = VST_preset_data[chan]->numParams;


                    // Ask the effect if this is an acceptable program
                    if (VST_proc::dispatcher(chan, effBeginLoadProgram, 0, 0, (void *) &info, 0.0) == -1) {
                        _block_timer2 = 0;
                        goto skip;
                    }

                    VST_proc::dispatcher(chan, effBeginSetProgram, 0, 0, NULL, 0.0);
                    VST_proc::dispatcher(chan, effSetChunk, 1, data.length(), data.data(), 0.0);
                    VST_proc::dispatcher(chan, effEndSetProgram, 0, 0, NULL, 0.0);

                    VST_proc::dispatcher(chan, effEditIdle, 0, 0, NULL, 0.0f);


                } else {

                    VST_preset_data[chan]->type_data = 1;

                    char *p = data.data();

                    if(VST_preset_data[chan]->numParams != data.length()/4) {

                        _block_timer2 = 0;
                        goto skip;
                    }

                    for (int i = 0; i < VST_preset_data[chan]->numParams; i++) {

                        float f;
                        memcpy(&f, &p[i * 4], sizeof(float));

                        VST_proc::setParameter(chan, i, f);
                    }

                    VST_proc::dispatcher(chan, effEditIdle, 0, 0, NULL, 0.0f);
                }

            }
skip:
            _block_timer2 = 0;
        } else
            dat[1]= -1;

        sharedVSText->unlock();
    } else if(cmd == 0x33) { // VST_external_show

        int chan = dat[1];
        dat[1] = 0;

        int ms = dat[2];

        sharedVSText->unlock();

        QThread::msleep(ms);

        if(chan < 0 || chan > PRE_CHAN) {
            int flag = 1;

            for(int n = 0; n < PRE_CHAN; n++) {
                if(VST_preset_data[n] && VST_preset_data[n]->vstWidget) {
                    if(flag) {
                        if(!this->isMinimized())
                            this->showMinimized(); // all windows visible
                        this->raise();
                        this->showNormal();
                        this->setVisible(false);
                        flag = 0;
                    }

                }
            }


        } else {
            if(VST_preset_data[chan] && VST_preset_data[chan]->vstWidget) {
                if(!VST_preset_data[chan]->vstWidget->isMinimized())
                    VST_preset_data[chan]->vstWidget->showMinimized();
                VST_preset_data[chan]->vstWidget->raise();
                VST_preset_data[chan]->vstWidget->showNormal();

            }// else dat[1] = -1;

        }



    } else if(cmd == 0x34) { // vstWidget_external_close()

        int chan = dat[1];

        dat[1] = -1;

        if(chan < 0 || chan > PRE_CHAN) {

        } else {
            if(VST_preset_data[chan]) {

                VST_preset_data[chan]->vstWidget->close();

            } else dat[1] = -1;

        }

        sharedVSText->unlock();

    }  else if(cmd == 0x35) { // VST_external_send_message

        int chan = dat[1];

        int message = dat[2];
        int sel = dat[3];

        if(message == 0xAD105) {
            flag_exit = 1;
            sema->release();
            for(int n = 0; n <= PRE_CHAN; n++) {
                VST_proc::VST_unload(n);
            }
            this->deleteLater();
            return;
        }

        VST_preset_data[chan]->disabled = dat[8];
        VST_preset_data[chan]->needUpdate = dat[9];

        if(message == 0xABECE5) {
            if(sel < 0) sel = 0x7F;
            VST_preset_data[chan]->send_preset = sel;//dat[10];
            sharedVSText->unlock();
            if(VST_preset_data[chan]->vstWidget)
                emit ((VSTDialog *) VST_preset_data[chan]->vstWidget)->setPreset(sel);
        } else if(message == 0xABCE50) {
            if(sel < 0) sel = 0x7F;
            sharedVSText->unlock();
            if(VST_preset_data[chan]->vstWidget)
                emit ((VSTDialog *) VST_preset_data[chan]->vstWidget)->setBackColor(sel);
        } else if(message == 0xCACAFEA) {
            sharedVSText->unlock();
            VST_proc::VST_DisableButtons(chan, sel ? true : false);
        } else if(message == 0xC0C0FE0) {
            sharedVSText->unlock();
            VST_proc::VST_show(chan, sel ? true : false);
        } else
            sharedVSText->unlock();

    } else if(cmd == 0x40) {
        int chan = dat[1];
        int ms = dat[2];
        QByteArray cmd;
        cmd.append((char *) &dat[4], dat[3]);
        sharedVSText->unlock();

        VST_proc::VST_MIDIcmd(chan, ms, cmd);
    }

#if 0
    else if(cmd == 0x70) { // VST_MIX()

        int sample_rate = dat[1];
        int nsamples = dat[2];

        int chan = 0;
        for(; chan < PRE_CHAN; chan++)
            if(VST_ON(chan)) break; // test chan active

        if(chan < PRE_CHAN && sys_sema_out && sys_sema_in) {
            float **in = new float*[32];
            for(int n = 0; n < 32; n++)
                in[n] = ((float *) sharedAudioBuffer->data()) + nsamples * n;


            // memset((float *) sharedVSText->data(), 0, nsamples * 8);

            dat[1] = VST_proc::VST_mix(in, 16, sample_rate, nsamples);


            delete[] in;
        }

    }
#endif

    sema->release();

}


