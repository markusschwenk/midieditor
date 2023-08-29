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

#ifdef USE_FLUIDSYNTH
#include "VST.h"
#include <QDebug>
#include "../MidiEvent/SysExEvent.h"
#include <QMessageBox>
#include "../fluid/FluidDialog.h"
#include <qscreen.h>

QWidget *main_widget = NULL;

extern VST_EXT *vst_ext;

#define DEBUG_OUT(x) //qDebug("VST_PROC::%s", x)
#define DEBUG_OUT2(x) qDebug("VST_CHAN::%s", x)

typedef AEffect *(*vstPluginMain)(audioMasterCallback audioMaster);

static int first_time = 1;

static int vst_mix_disable = 1;
static int _block_timer = 0;
static int _block_timer2 = 0;

static QWidget *_parentS = NULL;

static float **out_vst;

QString VST_directory;
#ifdef __ARCH64__
QString VST_directory32bits;
#endif

#define OUT_CH(x) (x & 15)

static VST_preset_data_type *VST_preset_data[PRE_CHAN + 1];

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

VSTlogo *VSTlogo_control= NULL;

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

    Dialog->setStyleSheet(QString::fromUtf8("background-color: #FF000040;"));

    if (Dialog->objectName().isEmpty())
        Dialog->setObjectName("VSTDialog" + QString::number(chan));
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
    pushButtonSave->setFocusPolicy(Qt::NoFocus);
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
    //SpinBoxPreset->setFocusPolicy(Qt::NoFocus);
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
        Dialog->setWindowTitle("VST Pluging pre-loaded");
    else
        Dialog->setWindowTitle("VST channel #" + QString().number(channel & 15) + " plugin " + QString().number(channel/16 + 1));

    groupBox->setTitle(QString());

    labelPreset->setText("Preset:");

    connect(pushButtonSave, SIGNAL(clicked()), this, SLOT(Save()));
    connect(pushButtonReset, SIGNAL(clicked()), this, SLOT(Reset()));
    connect(pushButtonDelete, SIGNAL(clicked()), this, SLOT(Delete()));
    connect(pushButtonSet, SIGNAL(clicked()), this, SLOT(Set()));
    connect(pushButtonDis, SIGNAL(clicked()), this, SLOT(Dis()));
    connect(pushButtonUnset, SIGNAL(clicked()), this, SLOT(Unset()));
    connect(SpinBoxPreset, SIGNAL(valueChanged(int)), this, SLOT(ChangeFastPresetI(int)));
    connect(this, SIGNAL(setPreset(int)), this, SLOT(ChangeFastPresetI2(int)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(this, SIGNAL(rejected()), this, SLOT(accept()));


    if(vst_ext)
        connect(vst_ext, SIGNAL(sendVSTDialog(int, int, int)), this, SLOT(recVSTDialog(int, int, int)));

    QMetaObject::connectSlotsByName(Dialog);

    time_updat= new QTimer(this);
    time_updat->setSingleShot(false);

    connect(time_updat, SIGNAL(timeout()), this, SLOT(timer_update()), Qt::DirectConnection);
    time_updat->setSingleShot(false);
    time_updat->start(50);

}

void VSTDialog::accept() {

    VST_proc::VST_show(channel, false);

}

void VSTDialog::recVSTDialog(int chan, int button, int val) {
    qWarning("recVSTDialog %i %i %i", chan, button, val);
    if(chan == channel) {
        switch(button) {
        case 255:
            close();
            break;
        case 1:
            VST_preset_data[channel]->curr_preset = val;
            Save();
            break;
        case 2:
            VST_preset_data[channel]->curr_preset = val;
            Reset();
            break;
        case 3:
            VST_preset_data[channel]->curr_preset = val;
            Delete();
            break;
        case 4:
            VST_preset_data[channel]->curr_preset = val;
            Set();
            break;
        case 5:
            VST_preset_data[channel]->curr_preset = val;
            Dis();
            break;
        case 6:
            VST_preset_data[channel]->curr_preset = val;
            Unset();
            break;
       case 7:
            VST_preset_data[channel]->send_preset = -1;
            VST_preset_data[channel]->curr_preset = val;
            ChangeFastPresetI2(val);
            if(val >= 0) ChangeFastPreset(val);
            break;

        }

    }
}

void VSTDialog::timer_update() {
    if(!VST_ON(channel)) return;
    if(VST_preset_data[channel]->external) {
        if(_block_timer || _block_timer2) return;
        //VST_proc::VST_external_show(-1);
        return; // skip remoteVST
    }

    if(_block_timer || _block_timer2) return;

    if(!VST_preset_data[channel]->vstEffect && !VST_preset_data[channel]->external) return;
    if(VST_preset_data[channel]->disabled || VST_preset_data[channel]->closed) return;

    semaf->acquire(1);

    if(VST_preset_data[channel]->vstVersion >= 2 && VST_preset_data[channel]->needIdle) {
        int r = 0;
        if(VST_preset_data[channel]->external) {
            r = VST_proc::VST_external_idle(channel, effIdle);

        } else
            r = VST_proc::dispatcher(channel, effIdle, 0, 0, NULL, 0.0f);
        if(!r) VST_preset_data[channel]->needIdle = false;
    }

    if(VST_preset_data[channel]->external) {

        VST_proc::VST_external_idle(channel, effEditIdle);

    } else {
        if(isVisible())
            VST_proc::dispatcher(channel, effEditIdle, 0, 0, NULL, 0.0f);
    }

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

void VSTDialog::ChangePreset(int sel) {

    if(!VST_ON(channel)) return;
    if(!VST_preset_data[channel]->vstEffect) return;

    int chan = channel;

    VST_preset_data[chan]->curr_preset = sel;

    MainWindow *MWin = ((MainWindow *) _parentS);
    MidiFile* file = MWin->getFile();

    char id[4]= {0x0, 0x66, 0x66, 'V'};
    id[0] = chan;

    int clen = 0;

    int pre = VST_preset_data[chan]->curr_preset;

    int dtick= file->tick(150);

    QByteArray c;

    int last = -1;

    char *flag = NULL;
    
    int current_tick = 0;

    char *data2 = NULL;

    foreach (MidiEvent* event,
             *(file->eventsBetween(current_tick-dtick, current_tick+dtick))) {

        SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

        if(sys) {

            c = sys->data();

            if(c[0]==id[0] && c[1]==id[1] && c[2]==id[2] && c[3]==id[3]){
                QDataStream qd(&c,
                               QIODevice::ReadOnly);
                qd.startTransaction();

                char head[4];
                unsigned short head2[2];
                qd.readRawData((char *) head, 4);

                int dat;

                qd.readRawData((char *) head, 2);

                if(head[0] == 8 + pre) { // preset

                    decode_sys_format(qd, (void *) head2);
                    decode_sys_format(qd, (void *) &dat);
                    if(dat != VST_preset_data[chan]->uniqueID) continue;
                    decode_sys_format(qd, (void *) &dat);
                    if(dat != VST_preset_data[chan]->version) continue;
                    decode_sys_format(qd, (void *) &dat);
                    if(dat != VST_preset_data[chan]->numParams) continue;

                    int ind = head2[0];
                    int len = head[1];

                    if(ind == 0) last = head2[1];
                    if(!data2) data2 = (char *) malloc(head2[1] * 80 + 84);
                    if(!data2) return; // out of memory

                    if(!flag) {
                        flag =  (char *) malloc(head2[1]);
                        memset(flag, 0, head2[1]);
                        if(!flag) return;
                    }

                    for(int n = 0; n < len; n+= 4) {
                        decode_sys_format(qd, (void *) &data2[ind * 80 + n]);
                    }

                    clen+= len;

                    flag[ind] = 1;

                }

                if(head[0] == 16 + pre) { // preset new
                    clen = 0;
                    decode_sys_format(qd, (void *) &clen);

                    qWarning("sys VST pre %i len %i %i", pre, c.size(), clen);

                    decode_sys_format(qd, (void *) &dat);
                    if(dat != VST_preset_data[chan]->uniqueID) continue;
                    decode_sys_format(qd, (void *) &dat);
                    if(dat != VST_preset_data[chan]->version) continue;
                    decode_sys_format(qd, (void *) &dat);
                    if(dat != VST_preset_data[chan]->numParams) continue;

                    if(data2) free(data2);
                    data2 = (char *) malloc(clen + 4);
                    if(!data2) return;

                    memset(data2, 0, clen + 4);

                    for(int n = 0; n < clen; n+= 4) {
                        decode_sys_format(qd, (void *) &data2[n]);
                    }

                    last = 0x666;

                    break;

                }


            }

        }

    }

    if(last < 0 || !data2 || clen == 0) {
        /*
        // not found, default factory...
        clen = VST_preset_data[chan]->factory.length();
        if(!data2) data2 = (char *) malloc(clen);
        if(!data2) return; // out of memory
        memcpy(data2, VST_preset_data[chan]->factory.data(), clen);
        */

        if(VST_preset_data[channel]->external)
            VST_proc::VST_external_send_message(channel, EXTERNAL_UPDATE_PRESET_BKCOLOR, 0);
        else
            SpinBoxPreset->setStyleSheet(QString::fromUtf8("background-color: white;"));

        if(data2) free(data2);
        if(flag) free(flag);
        return;
    } else {

        for(int n = 0; n <= last; n++) {
            if(!flag || !flag[n]) { // incomplete
                if(VST_preset_data[channel]->external)
                    VST_proc::VST_external_send_message(channel, EXTERNAL_UPDATE_PRESET_BKCOLOR, 0);
                else
                    SpinBoxPreset->setStyleSheet(QString::fromUtf8("background-color: white;"));
                if(data2) free(data2);
                if(flag) free(flag);
                return;
            }
        }

        if(VST_preset_data[channel]->external)
            VST_proc::VST_external_send_message(channel, EXTERNAL_UPDATE_PRESET_BKCOLOR, 1);
        else
            SpinBoxPreset->setStyleSheet(QString::fromUtf8("background-color: #8000C000;"));
    }

    if(flag) free(flag);

    if(data2) {

        VST_preset_data[chan]->preset[pre].clear();
        VST_preset_data[chan]->preset[pre].append(data2, clen);
    }

    if(VST_preset_data[chan]->type_data == 0) {

        VstPatchChunkInfo info;

        memset(&info, 0, sizeof(info));
        info.version = 1;
        info.pluginUniqueID = VST_preset_data[chan]->uniqueID;
        info.pluginVersion = VST_preset_data[chan]->version;
        info.numElements =  VST_preset_data[chan]->numParams;

        // Ask the effect if this is an acceptable program
        if (VST_proc::dispatcher(chan, effBeginLoadProgram, 0, 0, (void *) &info, 0.0) == -1)
        {
            if(data2) free(data2);
            return;
        }

        VST_proc::dispatcher(chan, effBeginSetProgram, 0, 0, NULL, 0.0);
        VST_proc::dispatcher(chan, effSetChunk, 1 , clen, data2, 0.0);
        VST_proc::dispatcher(chan, effEndSetProgram, 0, 0, NULL, 0.0);

        if(VST_preset_data[chan]->vstWidget->isVisible())
            VST_proc::dispatcher(chan, effEditIdle, 0, 0, NULL, 0.0f);
    } else {
        if(VST_preset_data[chan]->numParams != clen/4) {
            if(data2) free(data2);
            return;
        }

        for (int i = 0; i < VST_preset_data[chan]->numParams; i++) {
            float f;
            memcpy(&f, &data2[i * 4], sizeof(float));
            VST_proc::setParameter(chan, i, f);
        }
        if(VST_preset_data[chan]->vstWidget->isVisible())
            VST_proc::dispatcher(chan, effEditIdle, 0, 0, NULL, 0.0f);
    }

    if(data2) free(data2);
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
}

void VSTDialog::ChangeFastPresetI2(int sel) {

    if(!VST_ON(channel)) return;
    if(VST_preset_data[channel]->external) {


        _dis_change = true;

        SpinBoxPreset->setValue(sel);

        _dis_change = false;

        VST_proc::VST_external_send_message(channel, EXTERNAL_UPDATE_WINSETPRESET, sel);
        return ;
    }

    if(sel < 0 || sel > 7) {
        if(VST_preset_data[channel]->external)
            VST_proc::VST_external_send_message(channel, EXTERNAL_UPDATE_PRESET_BKCOLOR, -1);
        else
            SpinBoxPreset->setStyleSheet(QString::fromUtf8("background-color: lightgray;"));
        return;
    } else {
        int clen = VST_preset_data[channel]->preset[sel].length();

        if(!clen) {
            if(VST_preset_data[channel]->external)
                VST_proc::VST_external_send_message(channel, EXTERNAL_UPDATE_PRESET_BKCOLOR, 0);
            else
                SpinBoxPreset->setStyleSheet(QString::fromUtf8("background-color: white;"));
        }
        else {
            if(VST_preset_data[channel]->external)
                VST_proc::VST_external_send_message(channel, EXTERNAL_UPDATE_PRESET_BKCOLOR, 1);
            else
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
    if(channel == PRE_CHAN || (!VST_preset_data[channel]->vstEffect && !VST_preset_data[channel]->external)) return;


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
        _block_timer2 = 0;
        semaf->release(1);
        return;
    }

    if(VST_preset_data[channel]->external) {

        _block_timer2 = 0;
        semaf->release(1);
        return;
    }

    if(VST_preset_data[chan]->type_data == 0) {

        qWarning("Change preset type_data 0");

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
        if(isVisible())
            VST_proc::dispatcher(chan, effEditIdle, 0, 0, NULL, 0.0f);

    } else {

        qWarning("Change preset type_data 1");

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

        if(isVisible())
            VST_proc::dispatcher(channel, effEditIdle, 0, 0, NULL, 0.0f);

    }

    _block_timer2 = 0;
    semaf->release(1);

}

void VSTDialog::Save() {

    int chan = channel;

    if(!VST_ON(chan)) return;
    if(!VST_preset_data[chan]->vstEffect && !VST_preset_data[chan]->external) return;

    MainWindow *MWin = ((MainWindow *) _parentS);
    MidiFile* file = MWin->getFile();

    QByteArray b;

    char id[4]= {0x0, 0x66, 0x66, 'V'};
    id[0] = chan;

    char head[4] = {0, 0, 0, 0};
    unsigned short head2[2] = {0, 0};

    unsigned dat;

    int clen = 0;

    int pre = VST_preset_data[chan]->curr_preset;

    DEBUG_OUT((QString("save ") + QString().number(chan) + QString(" pre ") + QString().number(pre)).toLocal8Bit().data());

    file->protocol()->startNewAction(QString().asprintf("SYSex VST #%i pluging %i Preset (%i) data stored", chan & 15, chan/16 + 1, pre));

    VST_preset_data[chan]->preset[pre].clear();

    _block_timer2 = 1;


    if(VST_preset_data[chan]->external) {

        VST_preset_data[chan]->preset[pre] = VST_proc::VST_external_load_preset(chan, pre);

        //qWarning("ext pre %i %i", pre, VST_preset_data[chan]->preset[pre].length());
    } else {

        // store preset settings
        if (VST_preset_data[chan]->vstEffect->flags & effFlagsProgramChunks) {

            void *chunk = NULL;
            int clen = (int) VST_proc::dispatcher(chan, effGetChunk, 1, 0, &chunk, 0.0);
            if (clen <= 0) goto nochunk;

            VST_preset_data[chan]->type_data = 0;

            VST_preset_data[chan]->preset[pre].append((char *) chunk, clen);

        } else {
            nochunk:

            VST_preset_data[chan]->type_data = 1;

            for (int i = 0; i < VST_preset_data[chan]->numParams; i++) {

               float f = VST_proc::getParameter(chan, i);
               VST_preset_data[chan]->preset[pre].append((char *) &f, sizeof(float));
            }

        }
    }

    _block_timer2 = 0;

    if(!VST_preset_data[chan]->preset[pre].length())
        return; // is void data..

    ///-> filename

    {
        // delete old sysEx events
        if(1) {

            int dtick = file->tick(150);

            QByteArray c;
            int current_tick = 0;

            foreach (MidiEvent* event,
                     *(file->eventsBetween(current_tick-dtick, current_tick+dtick))) {

                SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

                if(sys) {

                    c = sys->data();
                    // delete for individual chans

                    if(c[0]==id[0] && c[1]==id[1] && c[2]==id[2] && c[3]==id[3]){
                        QDataStream qd(&c,
                                       QIODevice::ReadOnly);
                        qd.startTransaction();

                        char head[4];

                        qd.readRawData((char *) head, 4);
                        qd.readRawData((char *) head, 2);

                        if(head[0] == 0)  // filename
                            file->channel(16)->removeEvent(sys);

                    }

                }

            }
        }


        QDataStream qd(&b,
                       QIODevice::WriteOnly);

        if(qd.writeRawData((const char *) id, 4)<0) goto skip0;
        head[0] = 0; // filename
        QByteArray name = VST_preset_data[chan]->filename.toUtf8();
        clen = name.length();
        head[1] = clen;
        head2[0] = head2[1] = 0;
        if(qd.writeRawData((const char *) head, 2)<0) goto skip0;
        encode_sys_format(qd, (void *) head2);
        dat = VST_preset_data[chan]->uniqueID;
        encode_sys_format(qd, (void *) &dat);
        dat = VST_preset_data[chan]->version;
        encode_sys_format(qd, (void *) &dat);
        dat = VST_preset_data[chan]->numParams;
        encode_sys_format(qd, (void *) &dat);
        char data2[clen + 4];
        memset(data2, 0, clen + 4);
        memcpy(data2, name.data(), clen);
        for(int n = 0; n < clen; n+= 4) {
            encode_sys_format(qd, (void *) &data2[n]);
        }

        SysExEvent *sys_event = new SysExEvent(16, b, file->track(0));
        file->channel(16)->insertEvent(sys_event, 0);

    }
skip0:
    // delete old sysEx events

    int dtick= file->tick(150);

    QByteArray c;
    int current_tick = 0;

    foreach (MidiEvent* event,
             *(file->eventsBetween(current_tick-dtick, current_tick+dtick))) {

        SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

        if(sys) {

            c = sys->data();
            // delete for individual chans

            if(c[0]==id[0] && c[1]==id[1] && c[2]==id[2] && c[3]==id[3]){
                QDataStream qd(&c,
                               QIODevice::ReadOnly);
                qd.startTransaction();

                char head[4];
                qd.readRawData((char *) head, 4);

                qd.readRawData((char *) head, 2);

                if(head[0] == 8 + pre || head[0] == 16 + pre) { // preset
                    file->channel(16)->removeEvent(sys);

                }
            }

        }

    }

    {

        clen = VST_preset_data[chan]->preset[pre].length();

        if(clen > 126 * 521) {
            QMessageBox::critical(_parentS, "Savings Preset", "data string too long for SysEx");
            goto skip;
        }

        ///////////////////////////
        b.clear();
        QDataStream qd(&b,
                       QIODevice::WriteOnly);
        if(qd.writeRawData((const char *) id, 4)<0) goto skip;

        head[0] = 16 + pre; // preset (new preset saves);
        head[1] = 0;

        char *cdata = new char[clen + 4];

        if(!cdata) goto skip; // no memory !

        memset(cdata, 0, clen + 4);
        memcpy(cdata, VST_preset_data[chan]->preset[pre].data(), clen);

        if(qd.writeRawData((const char *) head, 2)<0) goto skip;
        encode_sys_format(qd, (void *) &clen);
        dat = VST_preset_data[chan]->uniqueID;
        encode_sys_format(qd, (void *) &dat);
        dat = VST_preset_data[chan]->version;
        encode_sys_format(qd, (void *) &dat);
        dat = VST_preset_data[chan]->numParams;
        encode_sys_format(qd, (void *) &dat);

        for(int n = 0; n < clen; n+= 4) {
            encode_sys_format(qd, (void *) &cdata[n]);
        }

        SysExEvent *sys_event = new SysExEvent(16, b, file->track(0));
        file->channel(16)->insertEvent(sys_event, 0);

        ///////////////////////////////

#if 0
        int ind = 0;
        int lastind = clen/80;

        char data2[clen + 4];
        memset(data2, 0, clen + 4);
        memcpy(data2, VST_preset_data[chan]->preset[pre].data(), clen);

        while(clen > 0) {

            int len = (clen > 80) ? 80 : clen;
            clen-= len;

            b.clear();
            QDataStream qd(&b,
                           QIODevice::WriteOnly);
            if(qd.writeRawData((const char *) id, 4)<0) goto skip;

            head[0] = 8 + pre; // preset
            head[1] = len;
            head2[0] = ind;
            head2[1] = lastind;
            if(qd.writeRawData((const char *) head, 2)<0) goto skip;
            encode_sys_format(qd, (void *) head2);
            dat = VST_preset_data[chan]->uniqueID;
            encode_sys_format(qd, (void *) &dat);
            dat = VST_preset_data[chan]->version;
            encode_sys_format(qd, (void *) &dat);
            dat = VST_preset_data[chan]->numParams;
            encode_sys_format(qd, (void *) &dat);

            for(int n = 0; n < len; n+= 4) {
                encode_sys_format(qd, (void *) &data2[ind * 80 + n]);
            }

            if(b.length() > 126) goto skip;

            //qDebug("save2 ch %i pre %i %i", (int) id[0], pre, b.length());
            SysExEvent *sys_event = new SysExEvent(16, b, file->track(0));
            file->channel(16)->insertEvent(sys_event, 0);
            ind++;
        }
#endif
        VST_proc::VST_external_send_message(channel, EXTERNAL_UPDATE_PRESET_BKCOLOR, 1);
        SpinBoxPreset->setStyleSheet(QString::fromUtf8("background-color: #8000C000;"));

    }
skip:

    file->protocol()->endAction();

    return;

}

void VSTDialog::Reset() {

    int chan = channel;

    if(!VST_ON(chan)) return;

    if(!VST_preset_data[chan]->vstEffect && !VST_preset_data[chan]->external) return;

    if(VST_preset_data[chan]->external) {

        _block_timer2 = 1;

        VST_proc::VST_external_save_preset(chan, -1); // factory

        int cur_pre = (SpinBoxPreset) ? SpinBoxPreset->value() : -1;
        if(cur_pre > 7) cur_pre = -1;

        VST_proc::VST_external_send_message(chan, EXTERNAL_UPDATE_PRESET_BKCOLOR, (cur_pre == -1 || VST_preset_data[chan]->preset[cur_pre].count() == 0) ? 0 : 1);

        _block_timer2 = 0;

        VST_preset_data[chan]->needUpdate = true;
        return;
    }

    _block_timer2 = 1;

    // restore factory settings
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
            return;
        }

        VST_proc::dispatcher(chan, effBeginSetProgram, 0, 0, NULL, 0.0);
        VST_proc::dispatcher(chan, effSetChunk, 1, VST_preset_data[chan]->factory.length(), VST_preset_data[chan]->factory.data(), 0.0);
        VST_proc::dispatcher(chan, effEndSetProgram, 0, 0, NULL, 0.0);

        if(isVisible())
            VST_proc::dispatcher(chan, effEditIdle, 0, 0, NULL, 0.0f);

    } else {

        VST_preset_data[chan]->type_data = 1;

        char *p = VST_preset_data[chan]->factory.data();

        if(VST_preset_data[chan]->numParams != VST_preset_data[chan]->factory.length()/4) {

            _block_timer2 = 0;
            return;
        }

        for (int i = 0; i < VST_preset_data[chan]->numParams; i++) {

           float f;
           memcpy(&f, &p[i * 4], sizeof(float));

           VST_proc::setParameter(chan, i, f);
        }

        if(isVisible())
            VST_proc::dispatcher(chan, effEditIdle, 0, 0, NULL, 0.0f);

    }

    _block_timer2 = 0;
}

void VSTDialog::Delete() {

    int chan = channel;

    if(!VST_ON(channel)) return;
    if(!VST_preset_data[chan]->vstEffect && !VST_preset_data[chan]->external) return;

    if(!VST_preset_data[chan]->external) {

        setStyleSheet(QString::fromUtf8("background-color: #E5E5E5;"));
        int r = QMessageBox::question(this, "MidiEditor", "Delete preset\nAre you sure?                         ");

        setStyleSheet(QString::fromUtf8("background-color: #FF000040;"));
        if(r != QMessageBox::Yes) return;

    }


    MainWindow *MWin = ((MainWindow *) _parentS);
    MidiFile* file = MWin->getFile();

    QByteArray b;

    char id[4]= {0x0, 0x66, 0x66, 'V'};
    id[0] = chan;

    int pre = VST_preset_data[chan]->curr_preset;

    file->protocol()->startNewAction(QString().asprintf("SYSex VST #%i pluging %i Preset (%i) deleted", chan & 15, chan/16 + 1, pre));

    VST_preset_data[chan]->preset[pre].clear();

    // delete old sysEx events

    int dtick= file->tick(150);

    QByteArray c;
    int current_tick = 0;//file->cursorTick();

    foreach (MidiEvent* event,
             *(file->eventsBetween(current_tick-dtick, current_tick+dtick))) {

        SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

        if(sys) {

            c = sys->data();
            // delete for individual chans

            if(c[0]==id[0] && c[1]==id[1] && c[2]==id[2] && c[3]==id[3]){
                QDataStream qd(&c,
                               QIODevice::ReadOnly);
                qd.startTransaction();

                char head[4];
                qd.readRawData((char *) head, 4);

                qd.readRawData((char *) head, 2);

                if(head[0] == 8 + pre || head[0] == 16 + pre) { // preset
                    file->channel(16)->removeEvent(sys);

                    if(SpinBoxPreset)
                            SpinBoxPreset->setStyleSheet(QString::fromUtf8("background-color: white;"));
                    //
                    if(VST_preset_data[chan]->external) {


                        _block_timer2 = 1;

                        VST_proc::VST_external_save_preset(chan, pre, VST_preset_data[chan]->preset[pre]);

                        _block_timer2 = 0;
                    }


                    /////
                }
            }

        }

    }

    file->protocol()->endAction();

    return;
}


void VSTDialog::Set() {

    int chan = channel;

    if(!VST_ON(chan)) return;
    if(!VST_preset_data[chan]->vstEffect && !VST_preset_data[chan]->external) return;


    MainWindow *MWin = ((MainWindow *) _parentS);
    MidiFile* file = MWin->getFile();

    char id[4]= {0x0, 0x66, 0x66, 'W'};
    id[0] = chan;

    file->protocol()->startNewAction("SYSex VST Set Presets");

    int dtick= file->tick(150);

    QByteArray c;
    int current_tick = file->cursorTick();

    foreach (MidiEvent* event,
             *(file->eventsBetween(current_tick-dtick, current_tick+dtick))) {

        SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

        if(sys) {
            c = sys->data();

            if(c[0] == id[0] && c[1] == id[1] && c[2] == id[2] && c[3] == 'W'){

                file->channel(16)->removeEvent(sys);
            }

        }

    }

    QByteArray b;
    b.append(id, 4);
    b.append((char) VST_preset_data[chan]->curr_preset);

    SysExEvent *sys_event = new SysExEvent(16, b, file->track(0));
    file->channel(16)->insertEvent(sys_event, current_tick);

    file->protocol()->endAction();

}

void VSTDialog::Dis() {

    int chan = channel;

    if(!VST_ON(chan)) return;
    if(!VST_preset_data[chan]->vstEffect && !VST_preset_data[chan]->external) return;

    MainWindow *MWin = ((MainWindow *) _parentS);
    MidiFile* file = MWin->getFile();

    char id[4]= {0x0, 0x66, 0x66, 'W'};
    id[0] = chan;

    file->protocol()->startNewAction("SYSex VST Set Presets");

    int dtick= file->tick(150);

    QByteArray c;
    int current_tick = file->cursorTick();

    foreach (MidiEvent* event,
             *(file->eventsBetween(current_tick-dtick, current_tick+dtick))) {

        SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

        if(sys) {

            c = sys->data();

            if(c[0] == id[0] && c[1] == id[1] && c[2] == id[2] && c[3] == 'W'){

                file->channel(16)->removeEvent(sys);
            }

        }

    }

    QByteArray b;

    b.append(id, 4);
    b.append((char) 127); // disable

    SysExEvent *sys_event = new SysExEvent(16, b, file->track(0));
    file->channel(16)->insertEvent(sys_event, current_tick);

    file->protocol()->endAction();

}

void VSTDialog::Unset() {

    int chan = channel;

    if(!VST_ON(chan)) return;
    if(!VST_preset_data[chan]->vstEffect && !VST_preset_data[chan]->external) return;

    MainWindow *MWin = ((MainWindow *) _parentS);
    MidiFile* file = MWin->getFile();

    QByteArray b;

    char id[4]= {0x0, 0x66, 0x66, 'W'};
    id[0] = chan;

    file->protocol()->startNewAction("SYSex VST Unset Presets");

    int dtick= file->tick(150);

    QByteArray c;
    int current_tick = file->cursorTick();

    foreach (MidiEvent* event,
             *(file->eventsBetween(current_tick-dtick, current_tick+dtick))) {

        SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

        if(sys) {
            c = sys->data();

            if(c[0] == id[0] && c[1] == id[1] && c[2] == id[2] && c[3] == 'W'){

                file->channel(16)->removeEvent(sys);
            }

        }

    }

    file->protocol()->endAction();

}


/////////////////////////////////////////////////////////////////////////////////////////
// VST_PROC


intptr_t AudioMaster(AEffect * effect,
                     int32_t opcode,
                     int32_t index,
                     intptr_t value,
                     void * ptr,
                     float/* opt*/){
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

void VST_proc::Logo(int mode, QString text) {

    if(mode) {

        if(!VSTlogo_control) {

            VSTlogo_control= new VSTlogo(main_widget, text);
            if(!VSTlogo_control) return;
            ((QWidget *) VSTlogo_control->parent())->setEnabled(false);
            VSTlogo_control->open();
            /*
            VSTlogo_control->setModal(false);
            VSTlogo_control->show();
            VSTlogo_control->raise();
            VSTlogo_control->activateWindow();
            */

            for(int n = 0; n < 50; n++) {
                QCoreApplication::processEvents();
                QThread::msleep(5);
            }

        }

    } else if(VSTlogo_control) {

        QThread::msleep(1000);
        //VSTlogo_control->setVisible(false);
        VSTlogo_control->hide();
        ((QWidget *) VSTlogo_control->parent())->setEnabled(true);
        delete VSTlogo_control;
        VSTlogo_control = NULL;

    }
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
            qDebug("VST_PowerOn()");

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
            qDebug("VST_PowerOff()");
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
        (!VST_preset_data[chan]->vstEffect && !VST_preset_data[chan]->external) ||
            !VST_preset_data[chan]->vstWidget) return;

    _block_timer2 = 1;

    if(VST_preset_data[chan]->external) {
        VST_proc::VST_external_send_message(chan, EXTERNAL_VST_SHOW, show ? 1 : 0);
    }


    if(VST_preset_data[chan]->vstEffect) {
        if(!VST_preset_data[chan]->closed)
            VST_proc::dispatcher(chan, effEditClose, 0, 0, (void *)((VSTDialog *) VST_preset_data[chan]->vstWidget)->subWindow->winId(), 0.0);
        VST_preset_data[chan]->closed = true;
        if(show) {
            VST_preset_data[chan]->vstWidget->setVisible(true);
            VST_proc::dispatcher(chan, effEditOpen, 0, 0, (void *)((VSTDialog *) VST_preset_data[chan]->vstWidget)->subWindow->winId(), 0.0);
            VST_preset_data[chan]->closed = false;
            VST_preset_data[chan]->vstWidget->showMinimized();
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
        else {QCoreApplication::processEvents();QThread::msleep(50);}
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

    if(VST_ON(chan) && (VST_preset_data[chan]->type & 1) && VST_preset_data[chan]->external) {
        VST_proc::VST_external_send_message(chan, EXTERNAL_VST_MIDINOTEOFF, 0); //external VST_MIDInotesoff()
        return;
    }

    for(; chan < PRE_CHAN; chan+= 16) {

        if(VST_ON(chan) && (VST_preset_data[chan]->type & 1)) {

            VST_preset_data[chan]->type |= 128; // little hack for multithread

            QByteArray cmd;
            for(int m = 0; m < 127; m++) {

                cmd.clear();
                cmd.append((char ) 0x80);
                cmd.append((char ) m);
                cmd.append((char ) 0x0);

                VST_proc::VST_MIDIcmd(chan, -666 /* hack */, cmd);

                _block_timer2 = 1;


                if(vstEvents[chan]) {
                    VstEvents2 *vstEvents2 = vstEvents[chan];

                   // vstEvents[PRE_CHAN] =
                    vstEvents[chan] = NULL;

                    VST_proc::dispatcher(chan,  effProcessEvents, 0, 0, vstEvents2, 0);
                    if(VST_preset_data[chan]->vstWidget->isVisible())
                        VST_proc::dispatcher(chan, effEditIdle, 0, 0, NULL, 0.0f);


                    if(vstEvents2) {
                        for(int n = 0; n < vstEvents2->numEvents; n++)
                            delete vstEvents2->events[n];

                        delete vstEvents2;
                        vstEvents2 = NULL;
                    }

                    VST_proc::processReplacing(chan, &out_vst[OUT_CH(chan) * 2], &out_vst[OUT_CH(chan) * 2], fluid_output->fluid_out_samples);
                }


                _block_timer2 = 0;

            }

            VST_preset_data[chan]->type &= ~128; // restore flag

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

    if(_block_timer2) return;

    if(_init_2) {
        _init_2 = 0;
        for(int n = 0; n < PRE_CHAN + 1; n++)
            vstEvents[n] = NULL;
    }

    for(; chan < PRE_CHAN; chan+= 16) {

        if(!VST_ON(chan) || !(VST_preset_data[chan]->type & 1) || ((VST_preset_data[chan]->type & 128) && deltaframe != -666)) continue;

        waits = 1; 
/*
        if(_block_timer2) {
            waits = 0;
            return;
        }
*/
        if(VST_preset_data[chan]->external) {
            VST_proc::VST_external_MIDIcmd(chan, (deltaframe == -666) ? 0 : deltaframe, cmd);
            waits = 0;
            if(deltaframe == -666) break;
            continue;
        }
/*
        if(_block_timer2) {
            waits = 0;
            return;
        }
*/
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
/*
        if(_block_timer2) {
            waits = 0;
            return;
        }
*/
/*
        vstEvents[chan]->events[pos] = new VstEvent();


*/

        VstEvent *ev = new VstEvent();
        VstMidiEvent *event = (VstMidiEvent*) ev;

        event->byteSize = sizeof(*event);
        event->type = 1;
        event->flags = 1;
        event->noteLength = 0;
        event->noteOffset = 0;
        event->detune = 0;
        // 1d?
        event->noteOffVelocity = 0;
        event->deltaFrames = (deltaframe == -666) ? 0 : deltaframe;
        event->midiData[0] = cmd.at(0); // note on
        event->midiData[1] = cmd.at(1);
        event->midiData[2] = cmd.at(2); // velocity
        event->midiData[3] = 0;
/*
        if(_block_timer2) {
            DELETE(ev);
            waits = 0;
            return;
        }
*/
        vstEvents[chan]->events[pos] = ev;
        vstEvents[chan]->numEvents++;

        waits = 0;

        if(deltaframe == -666) break;
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
    vst_mix_disable = 1;

    if(VST_preset_data[chan] && VST_preset_data[chan]->on == false  && VST_preset_data[chan]->external) {
        qWarning("WTF");
        VST_preset_data[chan] = NULL;
    }
    if(VST_ON(chan) && VST_preset_data[chan]->external) {

        DEBUG_OUT("VST_unload VST is 32 bits");
        VST_preset_data[chan]->on = false;
        VST_preset_data[chan]->disabled = true;
        VST_preset_data_type * VST_temp = VST_preset_data[chan];

       // VST_proc::VST_external_unload(chan);

        QMutex *mux = VST_preset_data[chan]->mux;
        VST_preset_data[chan]->mux = NULL;
        if(mux) mux->lock(); // lock or wait to lock

        //VST_preset_data_type * VST_temp = VST_preset_data[chan];

        _block_timer2 = 1;

        VST_preset_data[chan] = NULL;


        if(fluid_control && chan < PRE_CHAN)
            fluid_control->wicon[chan]->setVisible(false);

        if(mux) {
            mux->unlock();
            delete mux;
        }

        if(VST_temp->vstWidget) {
            ((VSTDialog *) VST_temp->vstWidget)->subWindow->close();

        }
        VST_proc::VST_external_unload(chan);
        //emit ((VSTDialog *) VST_temp->vstWidget)->update();

        if(VST_temp->vstWidget) {
            ((VSTDialog *) VST_temp->vstWidget)->close();
            //((VSTDialog *) VST_temp->vstWidget)->deleteLater();
            //VST_temp->vstWidget->close();
            delete ((VSTDialog *) VST_temp->vstWidget);
            VST_temp->vstWidget = NULL;
        }


        if(VST_temp) delete VST_temp;

        _block_timer2 = 0;
        vst_mix_disable = 0;
        emit ((MainWindow *) main_widget)->ToggleViewVST(chan, false);
        return 0;
    }

    if(!VST_ON(chan) || !VST_preset_data[chan]->vstEffect) {
        _block_timer2 = 0;
        vst_mix_disable = 0;
        return -1;
    }

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

    if(fluid_control && chan < PRE_CHAN)
        fluid_control->wicon[chan]->setVisible(false);


    if(VST_temp->vstEffect) {

        AEffect *temp = VST_temp->vstEffect;
        VST_temp->vstEffect = NULL;

        if(!VST_temp->closed)
            temp->dispatcher(temp, effEditClose, 0, 0, (void *)  (void *)((VSTDialog *) VST_temp->vstWidget)->subWindow->winId(), 0.0);

        VST_temp->closed = true;
        if(VST_temp->vstWidget) VST_temp->vstWidget->close();

        temp->dispatcher(temp, effClose, 0, 0, NULL, 0.0);
    }

    mux->unlock();
    delete mux;

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

   // VST_temp->vstVersion = 0;
   // VST_temp->vstPowerOn = 0;

    _block_timer2 = 0;
    vst_mix_disable = 0;

    emit ((MainWindow *)main_widget)->ToggleViewVST(chan, false);

    return 0;
}

int VST_proc::VST_isLoaded(int chan) {

    DEBUG_OUT("VST_isLoaded");

    if(VST_ON(chan)
        && VST_preset_data[chan]->vstWidget
        && (VST_preset_data[chan]->vstLib || VST_preset_data[chan]->external)) return 1;

    return 0;
}

bool VST_proc::VST_mix_disable(bool disable) {

    bool ret = false;

    if(vst_mix_disable)
        ret = true;

    vst_mix_disable = (disable) ? 1 : 0;

    return ret;
}

int test_DLL(const QString pathModule) {
    int type = -1; // error
    QFile file(pathModule);

    if(!file.open(QIODevice::ReadOnly)) return type;

    QByteArray dat = file.read(0x40);

    if(dat.length() == 0x40) {
        if(dat[0] == 'M' && dat[1] == 'Z' // DLL signature
                && dat[2] == (char) 0x90 && dat[3] == (char) 0x0) {
            unsigned index;

            index = ((unsigned char) dat[0x3c]) | (((unsigned char) dat[0x3d])<<8)
                     | (((unsigned char) dat[0x3e])<<16) | (((unsigned char) dat[0x3f])<<24);

            if(file.seek(index)) {

                dat = file.read(0x6);

                if(dat.length() == 0x6) {

                    if(dat[0] == 'P' && dat[1] == 'E' // PE format
                            && dat[2] == (char) 0x0 && dat[3] == (char) 0x0) {
                        unsigned short machine = ((unsigned) dat[4]) | (((unsigned) dat[5])<<8);

                        if(machine == 0x8664)
                            type = 2; // 64 bits (Intel x64)
                        else if(machine == 0x14c)
                            type = 1; // 32 bits (Intel 386)
                        else
                            type = 0; // other
                    }

                }
            }

        }
    }

    qWarning("DLL type: %i", type);

    return type;
}


int VST_proc::VST_load(int chan, const QString pathModule) {

    DEBUG_OUT("VST_load");

    vst_mix_disable = 1;

    while(1) {

        if(!_block_timer) break;
        else {QCoreApplication::processEvents();QThread::msleep(50);}
    }


    if(first_time) {

        if(VST_directory == "") {
            VST_directory = QDir::homePath();
#ifdef __ARCH64__
            if(fluid_output->fluid_settings->value("VST_directory").toString() != "")
                VST_directory = fluid_output->fluid_settings->value("VST_directory").toString();
#else
            if(fluid_output->fluid_settings->value("VST_directory_32bits").toString() != "")
                VST_directory = fluid_output->fluid_settings->value("VST_directory_32bits").toString();
#endif

        }
#ifdef __ARCH64__
        if(VST_directory32bits == "") {
            VST_directory32bits = QDir::homePath();
            if(fluid_output->fluid_settings->value("VST_directory_32bits").toString() != "")
                VST_directory32bits = fluid_output->fluid_settings->value("VST_directory_32bits").toString();

        }
#endif

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
        qWarning("Error! VST_preset_data[%i] defined after VST_unload()", chan);
        delete VST_preset_data[chan];
        VST_preset_data[chan] = NULL;
    }

    int machine = test_DLL(pathModule);

    if(machine == 0) {
        QMessageBox::information(_parentS, "VST Load", "Unknown plataform for VST plugin");
        return -1;
    }

#ifndef __ARCH64__
    if(machine == 2) {
        QMessageBox::information(_parentS, "VST Load", "64 bits VST plugin under 32 bits Midieditor application");
        return -1;
    }

#endif


    VST_preset_data_type *VST_temp = new VST_preset_data_type;
    VST_temp->on = false;

    VST_temp->vstPowerOn = false;
    VST_temp->vstVersion = 0;
    VST_temp->vstEffect = NULL;
    VST_temp->vstWidget = NULL;
    VST_temp->vstLib = NULL;
    VST_temp->send_preset = -1;
    VST_temp->mux = NULL;
    VST_temp->external = 0;

#ifdef __ARCH64__
    VST_temp->external = (machine == 1) ? 1 : 0;

    if(machine == 1) {
        if(!sys_sema_in) {
            ((MainWindow *) main_widget)->remote_VST();
            QThread::msleep(200); // wait a time...
        }

        if(sys_sema_in) {

            VST_temp->external = 1;

            _block_timer2 = 1;
            int ret = VST_proc::VST_external_load(chan, pathModule);
            qDebug("VST_external_load() ret: %i", ret);

            if(!ret) {

                delete VST_temp;

                if(fluid_control && chan < PRE_CHAN) {
                    fluid_control->wicon[chan]->setVisible(true);
                }

                _block_timer2 = 0;
                vst_mix_disable = 0;

                if(chan < PRE_CHAN) emit ((MainWindow *)main_widget)->ToggleViewVST(chan, true);
                return 0;
            }


        } else {
            DELETE(VST_temp)
                    vst_mix_disable = 0;
            return -1;
        }

    }

#endif

    VST_temp->vstLib = new QLibrary(pathModule, _parentS);

    if(!VST_temp->vstLib) {
        DELETE(VST_temp)
        _block_timer2 = 0;
        vst_mix_disable = 0;
        return -1;
    }

    if (!VST_temp->vstLib->load()) {

        delete VST_temp->vstLib;

        delete VST_temp;
        _block_timer2 = 0;
        vst_mix_disable = 0;

        return -2;

    }

    vstPluginMain pluginMain;

    pluginMain = (vstPluginMain) VST_temp->vstLib->resolve("VSTPluginMain");

    if (!pluginMain) pluginMain = (vstPluginMain) VST_temp->vstLib->resolve("main");

    if (!pluginMain) {
        VST_temp->vstLib->unload();
        delete VST_temp->vstLib;
        delete VST_temp;
        _block_timer2 = 0;
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
            VST_preset_data[chan]->vstEffect->dispatcher(VST_preset_data[chan]->vstEffect, effSetSampleRate, 0, 0, NULL, (float) fluid_output->_sample_rate);
            VST_preset_data[chan]->vstEffect->dispatcher(VST_preset_data[chan]->vstEffect, effSetBlockSize, 0, fluid_output->fluid_out_samples, NULL, 0);

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

            VstRect *rect =NULL;

            // Some effects like to have us get their rect before opening them.

            VST_preset_data[chan]->vstEffect->dispatcher(VST_preset_data[chan]->vstEffect, effEditGetRect, 0, 0, &rect, 0.0);

            VST_preset_data[chan]->vstWidget = new VSTDialog(_parentS, chan);
            VST_preset_data[chan]->vstWidget->move(0, 0);

            if(rect && rect->right !=0 && rect->bottom !=0)  {
                ((VSTDialog *) VST_preset_data[chan]->vstWidget)->subWindow->resize(rect->right, rect->bottom);

            }

            VST_preset_data[chan]->vstEffect->dispatcher(VST_preset_data[chan]->vstEffect, effEditOpen, 0, 0, (void *)((VSTDialog *) VST_preset_data[chan]->vstWidget)->subWindow->winId(), 0.0);

            // Get the final bounds of the effect GUI
            VST_preset_data[chan]->vstEffect->dispatcher(VST_preset_data[chan]->vstEffect, effEditGetRect, 0, 0, &rect, 0.0);

            VST_proc::VST_Resize(chan, rect->right - rect->left, rect->bottom - rect->top);
            _block_timer2 = 1;

            QFile mfile(VST_preset_data[chan]->vstLib->fileName());
            //QString path = QFileInfo(mfile).dir().path();
            QString name = QFileInfo(mfile).fileName();

            if(name.length() > 79) {
                vst_mix_disable = 0;
                _block_timer2 = 0;
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

                VST_preset_data[chan]->type_data = 0;

                VST_preset_data[chan]->factory.append((char *) chunk, clen);

            } else {
nochunk:

                VST_preset_data[chan]->type_data = 1;

                for (int i = 0; i < VST_preset_data[chan]->numParams; i++) {

                    float f = VST_preset_data[chan]->vstEffect->getParameter(VST_preset_data[chan]->vstEffect, i);
                    VST_preset_data[chan]->factory.append((char *) &f, sizeof(float));
                }

            }

            //VST_preset_data[chan] = VST_temp;

            VST_preset_data[chan]->vstEffect->ptr2 = VST_preset_data[chan];

            if(fluid_control && chan < PRE_CHAN) {
                fluid_control->wicon[chan]->setVisible(true);
            }

            _block_timer2 = 0;
            vst_mix_disable = 0;

            if(chan < PRE_CHAN) emit ((MainWindow *)main_widget)->ToggleViewVST(chan, true);
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

    /*
    QDesktopWidget* d = QApplication::desktop();
    QRect clientRect = d->availableGeometry();
    */

    QRect clientRect = QGuiApplication::primaryScreen()->availableGeometry();



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
    VST_preset_data[chan]->vstWidget->minimumSize();
    VST_preset_data[chan]->vstWidget->showMinimized();
    VST_preset_data[chan]->vstWidget->showNormal();

}

typedef struct {
    char type;
    int in_bytes;
    int out_bytes;

} vst_message;

#include <QProcess>

extern QProcess *process;

int VST_proc::VST_mix(float**in, int nchans, int samplerate, int nsamples) {

    if(vst_mix_disable/* || _block_timer2*/) return 0;

    _block_timer = 1;

    static int _samplerate = -1;
    static int _nsamples = -1;

    if(_samplerate != samplerate || _nsamples != nsamples) {
        _samplerate = samplerate;
        _nsamples = nsamples;

        for(int chan = 0; chan < nchans; chan++) {
            if(VST_ON(chan) && VST_preset_data[chan]->external) {
                continue;
            }
            if(VST_ON(chan) && VST_preset_data[chan]->vstEffect) {
                VST_preset_data[chan]->needUpdateMix = true;
            }
        }
    }

    if(_init_2) {
        _init_2 = 0;
        for(int n = 0; n < PRE_CHAN + 1; n++)
            vstEvents[n] = NULL;
    }

    for(int chan = 0; chan < nchans; chan++) {

        if(vst_mix_disable) {
            _block_timer = 0;
            return 0;
        }

        if(out_vst) {
            if(!out_vst[OUT_CH(chan)]) continue;
        } else {
            _block_timer = 0;
            return -1;
        }

        if(VST_ON(chan) && VST_preset_data[chan]->external) {
            continue;
        }

        if(vst_mix_disable) {
            _block_timer = 0;
            return 0;
        }

        if(VST_ON(chan) && VST_preset_data[chan]->vstEffect && VST_preset_data[chan]->vstPowerOn) {

            if(VST_preset_data[chan]->disabled && !VST_proc::VST_isMIDI(chan)) continue;

            int send_preset = VST_preset_data[chan]->send_preset;
            if(send_preset >= 0 && VST_preset_data[chan]->vstWidget) {


                ((VSTDialog *) VST_preset_data[chan]->vstWidget)->ChangeFastPreset(send_preset);

                VST_preset_data[chan]->send_preset = -1;
                VST_preset_data[chan]->needUpdate = true;

            }


            if(VST_preset_data[chan]->needUpdateMix) {
                VST_proc::dispatcher(chan, effSetSampleRate, 0, 0, NULL, (float) samplerate);
                VST_proc::dispatcher(chan, effSetBlockSize, 0, nsamples, NULL, 0);
                VST_preset_data[chan]->needUpdateMix = false;
            }

            if(in && !VST_preset_data[chan]->disabled && VST_proc::VST_isMIDI(chan)) {
                memset(in[OUT_CH(chan) * 2], 0, nsamples * sizeof(float));
                memset(in[OUT_CH(chan) * 2 + 1], 0, nsamples * sizeof(float));

            }

            while (1) {
                if(!waits) break;
                QThread::msleep(1);
            }

            if(vst_mix_disable) {
                _block_timer = 0;
                return 0;
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

            // only VST 1 can use midi ch
            if(chan < 16 && VST_proc::VST_isMIDI(chan) && fluid_output->synth_chanvolume[chan] < 127) {
                float vol = ((float) (fluid_output->synth_chanvolume[chan] & 127)) / 127.0f;

                float *f1 = in[OUT_CH(chan) * 2];
                float *o1 = out_vst[OUT_CH(chan) * 2];
                float *f2 = in[OUT_CH(chan) * 2 + 1];
                float *o2 = out_vst[OUT_CH(chan) * 2 + 1];
                for(int n = 0; n < nsamples; n++) {
                    f1[n] = o1[n] * vol;
                    f2[n] = o2[n] * vol;
                }
            } else {

                memcpy(in[OUT_CH(chan) * 2], out_vst[OUT_CH(chan) * 2], nsamples * sizeof(float));
                memcpy(in[OUT_CH(chan) * 2 + 1], out_vst[OUT_CH(chan) * 2 + 1], nsamples * sizeof(float));
            }

        }
    }

    _block_timer = 0;

    return 0;
}

void VST_proc::VST_DisableButtons(int chan, bool disable) {
    if(!VST_preset_data[chan] || !VST_preset_data[chan]->vstWidget) return;

    ((VSTDialog *) VST_preset_data[chan]->vstWidget)->groupBox->setEnabled(!disable);
    if(VST_preset_data[chan]->external)
        VST_proc::VST_external_send_message(chan, EXTERNAL_VST_DISABLEBUTTONS, disable ? 1 : 0);

}

int VST_proc::VST_LoadParameterStream(QByteArray array) {

    char id[4]= {0x0, 0x66, 0x66, 'W'};

    int sel = 0;

    // sysEX uses VLQ as length! (https://en.wikipedia.org/wiki/Variable-length_quantity)

    unsigned int length = 0;

    int ind = 1;
    while(array[ind] & 128) {
        length = (length + (array[ind] & 127)) * 128;
        ind++;
    }
    length += array[ind];

    int chan = array[1+ind];

    if(array[2+ind] == id[1] && array[3+ind] == id[2] && array[4+ind] == 'W') {
       sel = array[5+ind];
    } else return 0;
/*
qWarning("sasasa %x %x %x %x %x", (unsigned int) array[1+ind], (unsigned int) array[2+ind],
        (unsigned int) array[3+ind], (unsigned int) array[4+ind], (unsigned int) array[5+ind]);
        */
    if(!VST_ON(chan)) return -1;
    if(!VST_preset_data[chan]->vstEffect && !VST_preset_data[chan]->external) return -1;
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

    bool proto = false;

    bool logo = false;


    MainWindow *MWin = ((MainWindow *) _parentS);
    MidiFile* file = MWin->getFile();

    bool list_chan[PRE_CHAN + 1];

    // list of channels loaded
    for(int n = 0; n < PRE_CHAN; n++) {
        list_chan[n] = false;
    }

#ifdef __ARCH64__
    if(fluid_output->fluid_settings->value("VST_directory").toString() != "")
        VST_directory = fluid_output->fluid_settings->value("VST_directory").toString();
#else
    if(fluid_output->fluid_settings->value("VST_directory_32bits").toString() != "")
        VST_directory = fluid_output->fluid_settings->value("VST_directory_32bits").toString();
#endif

#ifdef __ARCH64__
    if(fluid_output->fluid_settings->value("VST_directory_32bits").toString() != "")
        VST_directory32bits = fluid_output->fluid_settings->value("VST_directory_32bits").toString();
#endif

    int chan;

    char id[4]= {0x0, 0x66, 0x66, 'V'};

    int dtick= file->tick(150);

    QByteArray c;

    int current_tick = 0;

    char *data2 = NULL;

    for(int i = 0; i <= PRE_CHAN; i++)
           VST_proc::VST_unload(i);

    foreach (MidiEvent* event,
             *(file->eventsBetween(current_tick-dtick, current_tick+dtick))) {

        SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

        if(sys) {

            c = sys->data();

            if(c[1] == id[1] && c[2] == id[2] && c[3] == id[3]){

                chan = c[0];
                id[0] = c[0];

                QDataStream qd(&c,
                               QIODevice::ReadOnly);
                qd.startTransaction();

                char head[4];
                unsigned short head2[2];
                qd.readRawData((char *) head, 4);

                if(!(head[0] == id[0] && head[1] == id[1] && // re-test header
                        head[2] == id[2] && head[3] == id[3])) continue;


                qd.readRawData((char *) head, 2);
                decode_sys_format(qd, (void *) &head2[0]);

                if(head[0] == 0) { // filename

                    if(logo == false) {
                        Logo(1, QString());
                        logo = true;
                    }

                    //qWarning("sys VST Head len %i", c.size());

                    int _uniqueID;
                    int _version;
                    int _numParams;

                    if(list_chan[chan] == true) {
                        qWarning("sys VST (%i) Head duplicated!", chan);
                        continue;
                    }

                    list_chan[chan] = true;

                    decode_sys_format(qd, (void *) &_uniqueID);
                    decode_sys_format(qd, (void *) &_version);
                    decode_sys_format(qd, (void *) &_numParams);

                    int len = head[1];

                    data2 = (char *) malloc(len + 4);
                    if(!data2) {
                        if(proto)
                            file->protocol()->endAction();
                        Logo(0, QString());
                        return -1; // out of memory
                    }
                    memset(data2, 0, len + 4);

                    for(int n = 0; n < len; n+= 4) {
                        decode_sys_format(qd, (void *) &data2[n]);
                    }

                    QString filename;
                    filename.append(data2);
                    free(data2); data2 = NULL;

                    QString s = VST_directory + QString("/") +  filename;

#ifdef __ARCH64__
                    if(!QFile::exists(s)) // if not exits use 32 bits path
                        s = VST_directory32bits + QString("/") +  filename;
#endif

                    DEBUG_OUT2((QString("VST_load ") + QString::number(chan) + QString(" ") + filename).toLocal8Bit().data());

                    if(VST_proc::VST_load(chan, s)==0) {

                        if(!VST_ON(chan)) continue;
                        if(!VST_preset_data[chan]->vstEffect && !VST_preset_data[chan]->external) continue;

                        if(VST_preset_data[chan]->external) {
                            VST_proc::VST_external_send_message(chan, EXTERNAL_VST_HIDE, 0);
                        } else {
                            if(VST_preset_data[chan]->vstWidget)
                                VST_preset_data[chan]->vstWidget->hide();
                        }

                        QCoreApplication::processEvents();

                        //VST_proc::VST_show(chan, false);


                        for(int pre = 0; pre < 8; pre++) {

                            int flag_olds_vst = false;
                            int flag_new_vst = false;

                            int clen = 0;

                            int dtick= file->tick(150);

                            QByteArray c;

                            int last = -1;

                            char *flag = NULL;

                            int current_tick = 0;

                            char *data2 = NULL;

                            int num_index = -1;

                            foreach (MidiEvent* event,
                                     *(file->eventsBetween(current_tick-dtick, current_tick+dtick))) {

                                SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

                                if(sys) {

                                    c = sys->data();

                                    if(c[0]==id[0] && c[1]==id[1] && c[2]==id[2] && c[3]==id[3]){

                                        QDataStream qd(&c,
                                                       QIODevice::ReadOnly);
                                        qd.startTransaction();


                                        char head[4];
                                        unsigned short head2[2];
                                        qd.readRawData((char *) head, 4);

                                        int dat;

                                        qd.readRawData((char *) head, 2);                                   

                                        if(head[0] == 8 + pre) {
                                            flag_olds_vst = true;

                                            // preset

                                            decode_sys_format(qd, (void *) head2);

                                            //qWarning("sys VST pre %i len %i", pre, c.size());

                                            decode_sys_format(qd, (void *) &dat);
                                            if(dat != VST_preset_data[chan]->uniqueID) continue;
                                            if(dat != _uniqueID) continue;
                                            decode_sys_format(qd, (void *) &dat);
                                            if(dat != VST_preset_data[chan]->version) continue;
                                            if(dat != _version) continue;
                                            decode_sys_format(qd, (void *) &dat);
                                            if(dat != VST_preset_data[chan]->numParams) continue;
                                            if(dat != _numParams) continue;

                                            DEBUG_OUT2((QString("VST_load pre") + QString::number(pre)).toLocal8Bit().data());

                                            int ind = head2[0];
                                            int len = head[1];

                                            if(num_index < 0) {
                                               num_index =  head2[1];
                                            } else if( num_index !=  head2[1]) { // corrupted!
                                                qWarning("data corruption in VST_load pre %i", pre);
                                                last = -1;
                                                break;
                                            }


                                            if(!flag) {
                                                flag =  (char *) malloc(num_index);
                                                if(!flag) {
                                                    if(proto)
                                                        file->protocol()->endAction();
                                                    Logo(0, QString());
                                                    return -1;
                                                }
                                                memset(flag, 0, head2[1]);

                                            }

                                            if(ind == 0) last = head2[1];
                                            if(!data2) data2 = (char *) malloc(head2[1] * 80 + 84);
                                            if(!data2) {
                                                if(proto)
                                                    file->protocol()->endAction();
                                                Logo(0, QString());
                                                return -1; // out of memory
                                            }

                                            for(int n = 0; n < len; n+= 4) {
                                                decode_sys_format(qd, (void *) &data2[ind * 80 + n]);
                                            }

                                            clen+= len;
                                            flag[ind] = 1;

                                        }

                                        if(head[0] == 16 + pre) { // preset new
                                            flag_new_vst = true;
                                            clen = 0;
                                            decode_sys_format(qd, (void *) &clen);

                                            if(clen == 0) {
                                                if(!proto) {

                                                    file->protocol()->startNewAction("SYSEx old VST events deleted");
                                                    proto = true;
                                                }

                                                file->channel(16)->removeEvent(sys);
                                                continue;
                                            }

                                            qWarning("sys VST pre %i len %i %i", pre, c.size(), clen);

                                            decode_sys_format(qd, (void *) &dat);
                                            if(dat != VST_preset_data[chan]->uniqueID) continue;
                                            if(dat != _uniqueID) continue;
                                            decode_sys_format(qd, (void *) &dat);
                                            if(dat != VST_preset_data[chan]->version) continue;
                                            if(dat != _version) continue;
                                            decode_sys_format(qd, (void *) &dat);
                                            if(dat != VST_preset_data[chan]->numParams) continue;
                                            if(dat != _numParams) continue;

                                            if(data2) free(data2);
                                            data2 = (char *) malloc(clen + 4);
                                            if(!data2) {
                                                if(proto)
                                                    file->protocol()->endAction();
                                                Logo(0, QString());
                                                return -1;
                                            }

                                            memset(data2, 0, clen + 4);

                                            for(int n = 0; n < clen; n+= 4) {
                                                decode_sys_format(qd, (void *) &data2[n]);
                                            }

                                            last = 0x666;

                                            break;


                                        }
                                    }

                                }

                            }

                            if(last < 0 || !data2) { // not found, default factory...
                                if(pre == VST_preset_data[chan]->curr_preset) {
                                    clen = VST_preset_data[chan]->factory.length();
                                    if(data2) free(data2);
                                    data2 = (char *) malloc(clen);
                                    if(!data2) {
                                        if(proto)
                                            file->protocol()->endAction();
                                        Logo(0, QString());
                                        return -1; // out of memory
                                    }
                                    memcpy(data2, VST_preset_data[chan]->factory.data(), clen);
                                    ((VSTDialog *) VST_preset_data[chan]->vstWidget)->SpinBoxPreset->setStyleSheet(QString::fromUtf8("background-color: white;"));
                                } else {
                                    if(flag) free(flag);
                                    goto skip;
                                }

                            } else {

                                if(last != 0x666) {
                                    for(int n = 0; n <= last; n++) {
                                        if(!flag[n]) { // incomplete
                                            if(pre == VST_preset_data[chan]->curr_preset)
                                                ((VSTDialog *) VST_preset_data[chan]->vstWidget)->SpinBoxPreset->setStyleSheet(QString::fromUtf8("background-color: white;"));
                                            if(flag) free(flag);
                                            goto skip;
                                        }
                                    }

                                }
                                ((VSTDialog *) VST_preset_data[chan]->vstWidget)->SpinBoxPreset->setStyleSheet(QString::fromUtf8("background-color: #8000C000;"));
                            }

                            if(data2) {

                                if(flag_new_vst == true || flag_olds_vst) {
                                    if(!proto) {

                                        if(flag_new_vst == false)
                                            file->protocol()->startNewAction("SYSEx old VST events replaced");
                                        else
                                            file->protocol()->startNewAction("SYSEx old VST events deleted");
                                        //mf->protocol()->addEmptyAction("SYSEx old events replaced");

                                        proto = true;
                                    }

                                    foreach (MidiEvent* event,
                                             *(file->eventsBetween(current_tick-dtick, current_tick+dtick))) {

                                        SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

                                        if(sys) {
                                            c = sys->data();
                                            // delete for individual chans

                                            if(c[0]==id[0] && c[1]==id[1] && c[2]==id[2] && c[3]==id[3]){
                                                QDataStream qd(&c,
                                                               QIODevice::ReadOnly);
                                                qd.startTransaction();

                                                char head[4];
                                                qd.readRawData((char *) head, 4);
                                                qd.readRawData((char *) head, 2);

                                                if(head[0] == 8 + pre)  // preset
                                                    file->channel(16)->removeEvent(sys);
                                            }

                                        }

                                    }

                                    if(flag_new_vst == false) {
                                        QByteArray b;
                                        b.clear();
                                        QDataStream qd(&b,
                                                       QIODevice::WriteOnly);
                                        if(qd.writeRawData((const char *) id, 4)<0) goto skip;

                                        head[0] = 16 + pre; // preset (new preset saves);
                                        head[1] = 0;


                                        if(qd.writeRawData((const char *) head, 2)<0) goto skip;
                                        encode_sys_format(qd, (void *) &clen);
                                        int dat = VST_preset_data[chan]->uniqueID;
                                        encode_sys_format(qd, (void *) &dat);
                                        dat = VST_preset_data[chan]->version;
                                        encode_sys_format(qd, (void *) &dat);
                                        dat = VST_preset_data[chan]->numParams;
                                        encode_sys_format(qd, (void *) &dat);

                                        for(int n = 0; n < clen; n+= 4) {
                                            encode_sys_format(qd, (void *) &data2[n]);
                                        }

                                        SysExEvent *sys_event = new SysExEvent(16, b, file->track(0));
                                        file->channel(16)->insertEvent(sys_event, 0);

                                    }

                                }


                                VST_preset_data[chan]->preset[pre].clear();
                                VST_preset_data[chan]->preset[pre].append(data2, clen);
                            }

                            if(pre != VST_preset_data[chan]->curr_preset
                                    && VST_preset_data[chan]->external) {
                                _block_timer2 = 1;

                                VST_external_save_preset(chan, pre, VST_preset_data[chan]->preset[pre]);

                                _block_timer2 = 0;
                            }

                            if(pre == VST_preset_data[chan]->curr_preset) {

                                if(VST_preset_data[chan]->external) {
                                    _block_timer2 = 1;

                                    VST_external_save_preset(chan, pre, VST_preset_data[chan]->preset[pre]);

                                    _block_timer2 = 0;
                                } else {
                                    if(VST_preset_data[chan]->type_data == 0) {

                                        VstPatchChunkInfo info;

                                        _block_timer2 = 1;

                                        memset(&info, 0, sizeof(info));
                                        info.version = 1;
                                        info.pluginUniqueID = VST_preset_data[chan]->uniqueID;
                                        info.pluginVersion = VST_preset_data[chan]->version;
                                        info.numElements =  VST_preset_data[chan]->numParams;

                                        // Ask the effect if this is an acceptable program
                                        if(VST_proc::dispatcher(chan, effBeginLoadProgram, 0, 0, (void *) &info, 0.0) == -1) {
                                            if(data2) free(data2);
                                            _block_timer2 = 0;
                                            if(proto)
                                                file->protocol()->endAction();
                                            Logo(0, QString());
                                            return -1;
                                        }

                                        VST_proc::dispatcher(chan, effBeginSetProgram, 0, 0, NULL, 0.0);
                                        VST_proc::dispatcher(chan, effSetChunk, 1 , clen, data2, 0.0);
                                        VST_proc::dispatcher(chan, effEndSetProgram, 0, 0, NULL, 0.0);

                                        if(VST_preset_data[chan]->vstWidget->isVisible())
                                            VST_proc::dispatcher(chan, effEditIdle, 0, 0, NULL, 0.0f);

                                        _block_timer2 = 0;

                                    } else {

                                        if(VST_preset_data[chan]->numParams != clen/4) {
                                            if(data2) free(data2);
                                            if(proto)
                                                file->protocol()->endAction();
                                            Logo(0, QString());
                                            return -1;

                                        }

                                        _block_timer2 = 1;

                                        for (int i = 0; i < VST_preset_data[chan]->numParams; i++) {
                                            float f;
                                            memcpy(&f, &data2[i * 4], sizeof(float));
                                            VST_proc::setParameter(chan, i, f);
                                        }
                                        if(VST_preset_data[chan]->vstWidget->isVisible())
                                            VST_proc::dispatcher(chan, effEditIdle, 0, 0, NULL, 0.0f);
                                        _block_timer2 = 0;
                                    }
                                }

                                if(data2) free(data2);
                                data2 = NULL;
                                if(flag) free(flag);
                            }

                        skip:
                            if(data2) free(data2);
                            data2 = NULL;
                        }

                        /*
                        if(VST_preset_data[chan]->external) {
                            _block_timer2 = 1;
                            int pre = VST_preset_data[chan]->curr_preset;

                            VST_external_save_preset(chan, pre, VST_preset_data[chan]->preset[pre]);

                            VST_proc::VST_external_send_message(chan, EXTERNAL_UPDATE_PRESET_BKCOLOR, 1);
                            _block_timer2 = 0;
                        }
                        */

                    } else {
                        QMessageBox::critical(_parentS, "Load VST from MIDI file", "MIDI file countain VST Plugin datas but cannot load the plugin (wrong directory?)");

                    }

                }
            }

        }

    }

    if(proto)
        file->protocol()->endAction();

    if(data2) free(data2);
    Logo(0, QString());
    return 0;
}


int VST_proc::VST_UpdatefromMIDIfile() {

    MainWindow *MWin = ((MainWindow *) _parentS);
    MidiFile* file = MWin->getFile();

#ifdef __ARCH64__
    if(fluid_output->fluid_settings->value("VST_directory").toString() != "")
        VST_directory = fluid_output->fluid_settings->value("VST_directory").toString();
#else
    if(fluid_output->fluid_settings->value("VST_directory_32bits").toString() != "")
        VST_directory = fluid_output->fluid_settings->value("VST_directory_32bits").toString();
#endif

#ifdef __ARCH64__
    if(fluid_output->fluid_settings->value("VST_directory_32bits").toString() != "")
        VST_directory32bits = fluid_output->fluid_settings->value("VST_directory_32bits").toString();
#endif

    bool chan_test[PRE_CHAN];

    for(int n = 0; n < PRE_CHAN; n++)
        chan_test[n] = false;

    int chan;

    char id[4]= {0x0, 0x66, 0x66, 'V'};

    int dtick= file->tick(150);

    QByteArray c;

    int current_tick = 0;

    char *data2 = NULL;

    bool logo = false;

    foreach (MidiEvent* event,
             *(file->eventsBetween(current_tick-dtick, current_tick+dtick))) {

        SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

        if(sys) {

            c = sys->data();

            if(c[1]==id[1] && c[2]==id[2] && c[3]==id[3]){

                chan = c[0];
                id[0] = c[0];

                QDataStream qd(&c,
                               QIODevice::ReadOnly);
                qd.startTransaction();

                char head[4];
                unsigned short head2[2];
                qd.readRawData((char *) head, 4);

                if(!(head[0] == id[0] && head[1] == id[1] && // re-test header
                        head[2] == id[2] && head[3] == id[3])) continue;

                qd.readRawData((char *) head, 2);
                decode_sys_format(qd, (void *) &head2[0]);

                if(head[0] == 0) { // filename

                    int _uniqueID;
                    int _version;
                    int _numParams;

                    chan_test[chan] = true;

                    decode_sys_format(qd, (void *) &_uniqueID);
                    decode_sys_format(qd, (void *) &_version);
                    decode_sys_format(qd, (void *) &_numParams);

                    int len = head[1];

                    data2 = (char *) malloc(len + 4);
                    if(!data2) return -1; // out of memory
                    memset(data2, 0, len + 4);

                    for(int n = 0; n < len; n+= 4) {
                        decode_sys_format(qd, (void *) &data2[n]);
                    }
                    QString filename;
                    filename.append(data2);
                    free(data2); data2 = NULL;

                    int r = -1;

                    if(!VST_ON(chan) || filename != VST_preset_data[chan]->filename) {

                        bool show = VST_proc::VST_isShow(chan);
                        //xxx VST_proc::VST_unload(chan);
                        QString s = VST_directory + QString("/") +  filename;
#ifdef __ARCH64__
                        if(!QFile::exists(s)) // if not exits use 32 bits path
                            s = VST_directory32bits + QString("/") +  filename;
#endif
                        if(logo == false) {
                            Logo(1, "Loading VST Plugings");
                            logo = true;
                        }

                        r = VST_proc::VST_load(chan, s);

                        if(!show) {
                            //VST_proc::VST_show(chan, false);
                            if(VST_ON(chan)) {
                                if(VST_preset_data[chan]->external) {
                                    VST_proc::VST_external_send_message(chan, EXTERNAL_VST_HIDE, 0);
                                } else {
                                    if(VST_preset_data[chan]->vstWidget)
                                        VST_preset_data[chan]->vstWidget->hide();
                                }
                            }

                        }

                        QCoreApplication::processEvents();

                    } else {
                        r = 0;

                        if(VST_ON(chan)) {
                            for (int n = 0; n < 8; n++)
                                VST_preset_data[chan]->preset[n].clear();

                            if(VST_preset_data[chan]->vstEffect || VST_preset_data[chan]->external) {

                                for (int pre = 0; pre < 8; pre++) {

                                    int c = VST_preset_data[chan]->curr_preset;

                                    if(VST_preset_data[chan]->vstWidget) {

                                        VST_preset_data[chan]->curr_preset = pre;
                                        ((VSTDialog *) VST_preset_data[chan]->vstWidget)->Reset();
                                    }

                                    VST_preset_data[chan]->curr_preset = c;

#if 0

                                    if(pre == VST_preset_data[chan]->curr_preset) {

                                        if(VST_preset_data[chan]->external) {
                                            _block_timer2 = 1;

                                            VST_external_save_preset(chan, pre);

                                            _block_timer2 = 0;

                                            VST_proc::VST_external_send_message(chan, EXTERNAL_UPDATE_PRESET_BKCOLOR, 0);

                                        } else
                                            ((VSTDialog *) VST_preset_data[chan]->vstWidget)->SpinBoxPreset->setStyleSheet(QString::fromUtf8("background-color: white;"));

                                    } else {

                                        if(VST_preset_data[chan]->external) {
                                            _block_timer2 = 1;

                                            VST_external_save_preset(chan, pre);

                                            _block_timer2 = 0;
                                        }

                                    }
#endif


                                }

                            }

                        }
                    }



                    if(r == 0) {

                        if(!VST_ON(chan)) continue;
                        if(!VST_preset_data[chan]->vstEffect && !VST_preset_data[chan]->external) continue;

                        for(int pre = 0; pre < 8; pre++) {

                            int clen = 0;

                            int dtick= file->tick(150);

                            QByteArray c;

                            int last = -1;

                            char *flag = NULL;

                            int current_tick = 0;

                            char *data2 = NULL;

                            foreach (MidiEvent* event,
                                     *(file->eventsBetween(current_tick-dtick, current_tick+dtick))) {

                                SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

                                if(sys) {

                                    c = sys->data();

                                    if(c[0]==id[0] && c[1]==id[1] && c[2]==id[2] && c[3]==id[3]){

                                        QDataStream qd(&c,
                                                       QIODevice::ReadOnly);
                                        qd.startTransaction();

                                        char head[4];
                                        unsigned short head2[2];
                                        qd.readRawData((char *) head, 4);

                                        int dat;

                                        qd.readRawData((char *) head, 2);

                                        if(head[0] == 8 + pre) { // preset
                                            decode_sys_format(qd, (void *) head2);

                                            decode_sys_format(qd, (void *) &dat);
                                            if(dat != VST_preset_data[chan]->uniqueID) continue;
                                            if(dat != _uniqueID) continue;
                                            decode_sys_format(qd, (void *) &dat);
                                            if(dat != VST_preset_data[chan]->version) continue;
                                            if(dat != _version) continue;
                                            decode_sys_format(qd, (void *) &dat);
                                            if(dat != VST_preset_data[chan]->numParams) continue;
                                            if(dat != _numParams) continue;

                                            int ind = head2[0];
                                            int len = head[1];

                                            if(!flag) {
                                                flag =  (char *) malloc(head2[1]); 
                                                if(!flag) {
                                                    Logo(0, QString());
                                                    return -1;
                                                }
                                                memset(flag, 0, head2[1]);
                                            }

                                            if(ind == 0) last = head2[1];
                                            if(!data2) data2 = (char *) malloc(head2[1] * 80 + 84);
                                            if(!data2) {
                                                if(flag) free(flag);
                                                Logo(0, QString());
                                                return -1; // out of memory
                                            }

                                            for(int n = 0; n < len; n+= 4) {
                                                decode_sys_format(qd, (void *) &data2[ind * 80 + n]);
                                            }

                                            clen+= len;
                                            flag[ind] = 1;

                                        }
                                        if(head[0] == 16 + pre) { // preset new
                                            clen = 0;
                                            decode_sys_format(qd, (void *) &clen);

                                            qWarning("sys VST pre %i len %i %i", pre, c.size(), clen);

                                            decode_sys_format(qd, (void *) &dat);
                                            if(dat != VST_preset_data[chan]->uniqueID) continue;
                                            if(dat != _uniqueID) continue;
                                            decode_sys_format(qd, (void *) &dat);
                                            if(dat != VST_preset_data[chan]->version) continue;
                                            if(dat != _version) continue;
                                            decode_sys_format(qd, (void *) &dat);
                                            if(dat != VST_preset_data[chan]->numParams) continue;
                                            if(dat != _numParams) continue;

                                            if(data2) free(data2);
                                            data2 = (char *) malloc(clen + 4);
                                            if(!data2) {
                                                Logo(0, QString());
                                                return -1;
                                            }

                                            memset(data2, 0, clen + 4);

                                            for(int n = 0; n < clen; n+= 4) {
                                                decode_sys_format(qd, (void *) &data2[n]);
                                            }

                                            last = 0x666;
                                            break;

                                        }


                                    }

                                }

                            }

                            if(last < 0 || !data2) { // not found, default factory...

                                VST_preset_data[chan]->preset[pre].clear();

                                if(pre == VST_preset_data[chan]->curr_preset) {

                                    if(VST_preset_data[chan]->external) {
                                        _block_timer2 = 1;

                                        VST_external_save_preset(chan, pre);

                                        _block_timer2 = 0;

                                        VST_proc::VST_external_send_message(chan, EXTERNAL_UPDATE_PRESET_BKCOLOR, 0);

                                    } else
                                        ((VSTDialog *) VST_preset_data[chan]->vstWidget)->SpinBoxPreset->setStyleSheet(QString::fromUtf8("background-color: white;"));

                                    if(flag) free(flag);

                                    goto skip;
                                } else {

                                    if(VST_preset_data[chan]->external) {
                                        _block_timer2 = 1;

                                        VST_external_save_preset(chan, pre);

                                        _block_timer2 = 0;
                                    }



                                    if(flag) free(flag);
                                    goto skip;
                                }


                            } else {

                                if(last != 0x666) {
                                    for(int n = 0; n <= last; n++) {
                                        if(!flag[n]) { // incomplete
                                            if(pre == VST_preset_data[chan]->curr_preset)
                                                ((VSTDialog *) VST_preset_data[chan]->vstWidget)->SpinBoxPreset->setStyleSheet(QString::fromUtf8("background-color: white;"));

                                            if(flag) free(flag);

                                            goto skip;
                                        }
                                    }
                                }

                                ((VSTDialog *) VST_preset_data[chan]->vstWidget)->SpinBoxPreset->setStyleSheet(QString::fromUtf8("background-color: #8000C000;"));
                            }

                            if(data2) {
                                VST_preset_data[chan]->preset[pre].clear();
                                VST_preset_data[chan]->preset[pre].append(data2, clen);
                            }

                            if(pre != VST_preset_data[chan]->curr_preset
                                    && VST_preset_data[chan]->external) {
                                _block_timer2 = 1;

                                VST_external_save_preset(chan, pre, VST_preset_data[chan]->preset[pre]);

                                _block_timer2 = 0;
                            }

                            if(pre == VST_preset_data[chan]->curr_preset) {

                                if(VST_preset_data[chan]->external) {
                                    // do nothing

                                    _block_timer2 = 1;

                                    VST_external_save_preset(chan, pre, VST_preset_data[chan]->preset[pre]);

                                    _block_timer2 = 0;
                                } else {

                                    if(VST_preset_data[chan]->type_data == 0) {

                                        VstPatchChunkInfo info;

                                        _block_timer2 = 1;

                                        memset(&info, 0, sizeof(info));
                                        info.version = 1;
                                        info.pluginUniqueID = VST_preset_data[chan]->uniqueID;
                                        info.pluginVersion = VST_preset_data[chan]->version;
                                        info.numElements =  VST_preset_data[chan]->numParams;

                                        // Ask the effect if this is an acceptable program
                                        if (VST_proc::dispatcher(chan, effBeginLoadProgram, 0, 0, (void *) &info, 0.0) == -1) {

                                            if(data2) free(data2);
                                            _block_timer2 = 0;
                                            Logo(0, QString());
                                            return -1;
                                        }

                                        VST_proc::dispatcher(chan, effBeginSetProgram, 0, 0, NULL, 0.0);
                                        VST_proc::dispatcher(chan, effSetChunk, 1 , clen, data2, 0.0);
                                        VST_proc::dispatcher(chan, effEndSetProgram, 0, 0, NULL, 0.0);
                                        if(VST_preset_data[chan]->vstWidget->isVisible())
                                            VST_proc::dispatcher(chan, effEditIdle, 0, 0, NULL, 0.0f);

                                        _block_timer2 = 0;

                                    } else {

                                        if(VST_preset_data[chan]->numParams != clen/4) {

                                            if(data2) free(data2);
                                            Logo(0, QString());
                                            return -1;

                                        }

                                        _block_timer2 = 1;

                                        for (int i = 0; i < VST_preset_data[chan]->numParams; i++) {
                                            float f;
                                            memcpy(&f, &data2[i * 4], sizeof(float));
                                            VST_proc::setParameter(chan, i, f);
                                        }

                                        if(VST_preset_data[chan]->vstWidget->isVisible())
                                            VST_proc::dispatcher(chan, effEditIdle, 0, 0, NULL, 0.0f);

                                        _block_timer2 = 0;
                                    }

                                }

                                if(data2) free(data2);
                                data2 = NULL;
                                if(flag) free(flag);
                            }

                        skip:
                            if(data2) free(data2);
                            data2 = NULL;
                        }
                    }
                }
            }
        }
    }

    if(data2) free(data2);

    for(int chan = 0; chan < PRE_CHAN; chan++) {

        if(chan_test[chan] == false)
            VST_proc::VST_unload(chan);

        if(VST_ON(chan) && VST_preset_data[chan]->vstWidget &&
                !VST_preset_data[chan]->preset[VST_preset_data[chan]->curr_preset].length()) {
            if(VST_preset_data[chan]->external)
                VST_proc::VST_external_send_message(chan, EXTERNAL_UPDATE_PRESET_BKCOLOR, 0);
            else
                ((VSTDialog *) VST_preset_data[chan]->vstWidget)->SpinBoxPreset->setStyleSheet(QString::fromUtf8("background-color: white;"));
        }
    }

    Logo(0, QString());

    return 0;
}

bool VST_proc::VST_isEnabled(int chan) {

    if(!VST_preset_data[chan]) return false;
    return !VST_preset_data[chan]->disabled;

}

int VST_proc::VST_SaveParameters(int chan)
{
    if(!VST_preset_data[chan]
            || (!VST_preset_data[chan]->vstEffect && !VST_preset_data[chan]->external)
            || (!VST_preset_data[chan]->vstWidget && VST_preset_data[chan]->external)
            ) return -1;

    MainWindow *MWin = ((MainWindow *) _parentS);
    MidiFile* file = MWin->getFile();

    QByteArray b;

    char id[4]= {0x0, 0x66, 0x66, 'V'};
    id[0] = chan;

    char head[4] = {0, 0, 0, 0};
    unsigned short head2[2] = {0, 0};

    unsigned dat;

    int clen = 0;

    file->protocol()->startNewAction(QString().asprintf("SYSex VST #%i pluging %i Preset data stored", chan & 15, chan/16 + 1));

    ///-> filename

    {
        // delete old sysEx events
        if(1) {
            int dtick = file->tick(150);

            QByteArray c;
            int current_tick = 0;

            foreach (MidiEvent* event,
                     *(file->eventsBetween(current_tick-dtick, current_tick+dtick))) {

                SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

                if(sys) {
                    c = sys->data();
                    // delete for individual chans

                    if(c[0]==id[0] && c[1]==id[1] && c[2]==id[2] && c[3]==id[3]){
                        QDataStream qd(&c,
                                       QIODevice::ReadOnly);
                        qd.startTransaction();

                        char head[4];
                        qd.readRawData((char *) head, 4);
                        qd.readRawData((char *) head, 2);

                        if(head[0] == 0)  // filename
                            file->channel(16)->removeEvent(sys);

                    }

                }

            }
        }

        QDataStream qd(&b,
                       QIODevice::WriteOnly);

        if(qd.writeRawData((const char *) id, 4)<0) goto skip;
        head[0] = 0; // filename
        QByteArray name = VST_preset_data[chan]->filename.toUtf8();
        clen = name.length();
        head[1] = clen;
        head2[0] = head2[1] = 0;
        if(qd.writeRawData((const char *) head, 2)<0) goto skip;
        encode_sys_format(qd, (void *) head2);
        dat = VST_preset_data[chan]->uniqueID;
        encode_sys_format(qd, (void *) &dat);
        dat = VST_preset_data[chan]->version;
        encode_sys_format(qd, (void *) &dat);
        dat = VST_preset_data[chan]->numParams;
        encode_sys_format(qd, (void *) &dat);
        char data2[clen + 4];
        memset(data2, 0, clen + 4);
        memcpy(data2, name.data(), clen);
        for(int n = 0; n < clen; n+= 4) {
            encode_sys_format(qd, (void *) &data2[n]);
        }

        SysExEvent *sys_event = new SysExEvent(16, b, file->track(0));
        file->channel(16)->insertEvent(sys_event, 0);
    }


    ///-> preset

    for(int pre = 0; pre < 8; pre++)
    {

        clen = VST_preset_data[chan]->preset[pre].length();

        if(clen > 126 * 521) {
            QMessageBox::critical(_parentS, "Saving Preset", "data string too long for SysEx");
            break;
        }

        char data2[clen + 4];
        memset(data2, 0, clen + 4);
        memcpy(data2, VST_preset_data[chan]->preset[pre].data(), clen);

        // delete old sysEx events
        if(1/*clen > 0*/) {
            int dtick= file->tick(150);

            QByteArray c;
            int current_tick = 0;//file->cursorTick();

            foreach (MidiEvent* event,
                     *(file->eventsBetween(current_tick-dtick, current_tick+dtick))) {

                SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

                if(sys) {
                    c = sys->data();
                    // delete for individual chans

                    if(c[0]==id[0] && c[1]==id[1] && c[2]==id[2] && c[3]==id[3]){
                        QDataStream qd(&c,
                                       QIODevice::ReadOnly);
                        qd.startTransaction();

                        char head[4];
                        qd.readRawData((char *) head, 4);
                        qd.readRawData((char *) head, 2);

                        if(head[0] == 8 + pre || head[0] == 16 + pre)  // preset
                            file->channel(16)->removeEvent(sys);
                    }

                }

            }
        }

        ///////////////////////////

        if(clen > 0) {
            b.clear();
            QDataStream qd(&b,
                           QIODevice::WriteOnly);
            if(qd.writeRawData((const char *) id, 4)<0) goto skip;

            head[0] = 16 + pre; // preset (new preset saves);
            head[1] = 0;

            char *cdata = new char[clen + 4];

            if(!cdata) goto skip; // no memory !

            memset(cdata, 0, clen + 4);
            memcpy(cdata, VST_preset_data[chan]->preset[pre].data(), clen);

            if(qd.writeRawData((const char *) head, 2)<0) goto skip;
            encode_sys_format(qd, (void *) &clen);
            dat = VST_preset_data[chan]->uniqueID;
            encode_sys_format(qd, (void *) &dat);
            dat = VST_preset_data[chan]->version;
            encode_sys_format(qd, (void *) &dat);
            dat = VST_preset_data[chan]->numParams;
            encode_sys_format(qd, (void *) &dat);

            for(int n = 0; n < clen; n+= 4) {
                encode_sys_format(qd, (void *) &cdata[n]);
            }

            SysExEvent *sys_event = new SysExEvent(16, b, file->track(0));
            file->channel(16)->insertEvent(sys_event, 0);
        }

        ///////////////////////////////
#if 0
        // old sysEx...

        int ind = 0;
        int lastind = clen/80;

        while(clen > 0) {

            int len = (clen > 80) ? 80 : clen;
            clen-= len;

// 8 + pre
            b.clear();
            QDataStream qd(&b,
                           QIODevice::WriteOnly);
            if(qd.writeRawData((const char *) id, 4)<0) goto skip;

            head[0] = 8 + pre; // preset
            head[1] = len;
            head2[0] = ind;
            head2[1] = lastind;
            if(qd.writeRawData((const char *) head, 2)<0) goto skip;
            encode_sys_format(qd, (void *) head2);
            dat = VST_preset_data[chan]->uniqueID;
            encode_sys_format(qd, (void *) &dat);
            dat = VST_preset_data[chan]->version;
            encode_sys_format(qd, (void *) &dat);
            dat = VST_preset_data[chan]->numParams;
            encode_sys_format(qd, (void *) &dat);

            for(int n = 0; n < len; n+= 4) {
                encode_sys_format(qd, (void *) &data2[ind * 80 + n]);
            }

            if(b.length() > 126) goto skip;

            SysExEvent *sys_event = new SysExEvent(16, b, file->track(0));
            file->channel(16)->insertEvent(sys_event, 0);
            ind++;
        }
#endif
    }


skip:

    file->protocol()->endAction();

    return 0;
}


void VST_proc::VST_setParent(QWidget *parent) {
    _parentS = parent;
}

VST_proc::VST_proc() {

}

VST_proc::~VST_proc() {

}

/////////////////////////////////////////////////////////////////////////////////////////
// VST_chan

VST_chan::VST_chan(QWidget* parent, int channel, int flag) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint) {

    QDialog *VST_chan = this;

    curVST_index = -1;
    chan = channel;
    chan_loaded = chan;

    int VST_index = -1;


    if(VST_directory == "") {
        VST_directory = QDir::homePath();

#ifdef __ARCH64__
        if(fluid_output->fluid_settings->value("VST_directory").toString() != "")
            VST_directory = fluid_output->fluid_settings->value("VST_directory").toString();
#else
        if(fluid_output->fluid_settings->value("VST_directory_32bits").toString() != "")
            VST_directory = fluid_output->fluid_settings->value("VST_directory_32bits").toString();
#endif

    }

#ifdef __ARCH64__
    if(VST_directory32bits == "") {
        VST_directory32bits = QDir::homePath();
        if(fluid_output->fluid_settings->value("VST_directory_32bits").toString() != "")
            VST_directory32bits = fluid_output->fluid_settings->value("VST_directory_32bits").toString();
    }
#endif

#define AMP2 48

    if(!flag) {
        if (VST_chan->objectName().isEmpty())
            VST_chan->setObjectName(QString::fromUtf8("VST_chan"));
        VST_chan->resize(409, 458 + AMP2); //(409, 340);
        VST_chan->setFixedSize(409, 458 + AMP2);
        VST_chan->setWindowTitle("VST Plugin List");

        buttonBox = new QDialogButtonBox(VST_chan);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setGeometry(QRect(30, 410 + AMP2, 371, 32));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Ok);

        pushVSTDirectory = new QPushButton(VST_chan);
        pushVSTDirectory->setObjectName(QString::fromUtf8("pushVSTDirectory"));
        pushVSTDirectory->setGeometry(QRect(20, 70, 371, 23));
        pushVSTDirectory->setToolTip("VST plugin directory for all channels");
        pushVSTDirectory->setText("Set VST plugin Directory");
#ifdef __ARCH64__
        pushVSTDirectory2 = new QPushButton(VST_chan);
        pushVSTDirectory2->setObjectName(QString::fromUtf8("pushVSTDirectory2"));
        pushVSTDirectory2->setGeometry(QRect(20, 100, 371, 23));
        pushVSTDirectory2->setToolTip("VST plugin directory 32 bits for all channels");
        pushVSTDirectory2->setText("Set VST plugin Directory for 32 bits");
#endif
        GroupBoxVST = new QGroupBox(VST_chan);
        GroupBoxVST->setObjectName(QString::fromUtf8("GroupBoxVST"));
        GroupBoxVST->setGeometry(QRect(20, 150, 371, 251 + AMP2));
        GroupBoxVST->setTitle("VST Plugin");

        pushButtonSetVST = new QPushButton(GroupBoxVST);
        pushButtonSetVST->setObjectName(QString::fromUtf8("pushButtonSetVST"));
        pushButtonSetVST->setGeometry(QRect(10, 20, 101, 23));
        pushButtonSetVST->setToolTip("Save VST datas for this channel using SysEx");
        pushButtonSetVST->setText("Set VST Plugin");

        pushButtonExportVSTData = new QPushButton(GroupBoxVST);
        pushButtonExportVSTData->setObjectName(QString::fromUtf8("pushButtonExportVSTData"));
        pushButtonExportVSTData->setGeometry(QRect(10, 20 + 32, /*101*/205 - 35 - 10, 23));
        pushButtonExportVSTData->setToolTip("Export VST plugins and presets from this channel");
        pushButtonExportVSTData->setText("Export VST Presets to...");

        pushButtonImportVSTData = new QPushButton(GroupBoxVST);
        pushButtonImportVSTData->setObjectName(QString::fromUtf8("pushButtonImportVSTData"));
        pushButtonImportVSTData->setGeometry(QRect(10 + 205 - 40, 20 + 32, /*101*/205 - 35 + 10, 23));
        pushButtonImportVSTData->setToolTip("Import VST plugins and presets to this channel");
        pushButtonImportVSTData->setText("Import VST and Presets");
        pushButtonImportVSTData->setStyleSheet(QString::fromUtf8("background-color: #b0b0b0;"));

        viewVST = new QPushButton(GroupBoxVST);
        viewVST->setObjectName(QString::fromUtf8("pushButtonviewVST"));
        viewVST->setGeometry(QRect(140, 20, 75, 23));
        viewVST->setToolTip("View VST plugin window");
        viewVST->setText("view VST");

        listWidget = new QListWidget(GroupBoxVST);
        listWidget->setObjectName(QString::fromUtf8("listWidget"));
        listWidget->setGeometry(QRect(10, 60 + AMP2, 351, 171));
        listWidget->setToolTip("List VST plugins from current directory\n"
                  "Background Green for current plugin in channel\n"
                  "Click to Open Plugin Window");

        pushButtonDeleteVST = new QPushButton(GroupBoxVST);
        pushButtonDeleteVST->setObjectName(QString::fromUtf8("pushButtonDeleteVST"));
        pushButtonDeleteVST->setGeometry(QRect(244, 20, 111, 23));
        pushButtonDeleteVST->setToolTip("Delete all VST current  plugin datas (SysEx) in channel");
        pushButtonDeleteVST->setText("Delete VST datas");


        labelinfo = new QLabel(VST_chan);
        labelinfo->setObjectName(QString::fromUtf8("labelinfo"));
        labelinfo->setGeometry(QRect(20, 10, 200, 31));
        QFont font;
        font.setPointSize(12);
        labelinfo->setFont(font);
        //labelinfo->setText("Channel #" + QString::number(channel));
        labelinfo->setText("Channel #" + QString().number(channel & 15) + " plugin " + QString().number(channel/16 + 1));


        Addfiles();


    } else {
        int i = 0;

        QDir q(VST_directory);
        QStringList plugs = q.entryList(QStringList() << "*.dll" << "*.DLL" << "*.vst" << "*.VST",QDir::Files);
#ifdef __ARCH64__
        q.setPath(VST_directory32bits);

        plugs += q.entryList(QStringList() << "*.dll" << "*.DLL" << "*.vst" << "*.VST",QDir::Files);
#endif
        listWidget = new QListWidget();

        foreach(QString filename, plugs) {

            if(VST_ON(channel) && VST_preset_data[channel]->filename == filename) {

                VST_index = i;
                curVST_index = i;
                listWidget->clear();
                listWidget->addItem(filename);
                break;
            }

        }

        if(curVST_index == -1 && VST_ON(PRE_CHAN)) chan_loaded = PRE_CHAN;
        if(VST_index != -1)
            load_plugin(listWidget->item(0));
        if(flag && !VST_ON(chan)) {
            int r = QMessageBox::question(this, "MidiEditor", "VST preset do not exist.\nUnset preset? (it is deleted)   ");

            if(r == QMessageBox::Yes) {

                MainWindow *MWin = ((MainWindow *) _parentS);
                MidiFile* file = MWin->getFile();

                QByteArray b;

                char id[4]= {0x0, 0x66, 0x66, 'W'};
                id[0] = chan;

                file->protocol()->startNewAction("SYSex VST Unset Presets");

                int dtick= file->tick(150);

                QByteArray c;
                int current_tick = file->cursorTick();

                foreach (MidiEvent* event,
                         *(file->eventsBetween(current_tick-dtick, current_tick+dtick))) {

                    SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

                    if(sys) {
                        c = sys->data();

                        if(c[0] == id[0] && c[1] == id[1] && c[2] == id[2] && c[3] == 'W'){

                            file->channel(16)->removeEvent(sys);
                        }

                    }

                }

                file->protocol()->endAction();


            }
        }
    }


    if(!flag) {

        if(curVST_index == -1 && VST_ON(PRE_CHAN)) chan_loaded = PRE_CHAN;

        connect(buttonBox, SIGNAL(accepted()), VST_chan, SLOT(accept()));        
        connect(pushVSTDirectory, SIGNAL(clicked()), this, SLOT(setVSTDirectory()));
#ifdef __ARCH64__
        connect(pushVSTDirectory2, SIGNAL(clicked()), this, SLOT(setVSTDirectory2()));
#endif
        connect(pushButtonSetVST, SIGNAL(clicked()), this, SLOT(SetVST()));
        connect(pushButtonExportVSTData, SIGNAL(clicked()), this, SLOT(ExportVSTData()));
        connect(pushButtonImportVSTData, SIGNAL(clicked()), this, SLOT(ImportVSTData()));


        connect(pushButtonDeleteVST, SIGNAL(clicked()), this, SLOT(DeleteVST()));
        connect(listWidget, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(load_plugin(QListWidgetItem*)));
        connect(viewVST, SIGNAL(clicked()), this, SLOT(viewVSTfun()));

        QMetaObject::connectSlotsByName(VST_chan);

        VST_proc::VST_external_show(-2, 1000);

    }

}

void VST_chan::Addfiles() {

    QDir q(VST_directory);
    QStringList plugs = q.entryList(QStringList() << "*.dll" << "*.DLL" << "*.vst" << "*.VST",QDir::Files);

    int i = 0;

    foreach(QString filename, plugs) {
        listWidget->addItem(filename);
        listWidget->item(i)->setBackground(QBrush(Qt::white));
        if(VST_ON(chan) && VST_preset_data[chan]->filename == filename) {
            listWidget->setCurrentRow(i);
            listWidget->item(i)->setBackground(QBrush(0x8000C0C0));
            curVST_index = i;
        } else

        if(VST_ON(PRE_CHAN) && VST_preset_data[PRE_CHAN]->filename == filename) {
            listWidget->setCurrentRow(i);

        }

        i++;
    }

#ifdef __ARCH64__
    q.setPath(VST_directory32bits);
    QStringList plugs2 = q.entryList(QStringList() << "*.dll" << "*.DLL" << "*.vst" << "*.VST",QDir::Files);

    if(plugs2.count()) {
        listWidget->addItem(VST_NAME_32BITS_BARRIER);
        listWidget->item(i)->setForeground(QBrush(Qt::white));
        listWidget->item(i)->setBackground(QBrush(Qt::red));
        i++;
    }

    foreach(QString filename, plugs2) {

        int exist = 0;

        foreach(QString filename2, plugs) { // skip duplicate names
            if(filename == filename2) {
                exist = 1;
                break;
            }
        }

        if(exist) continue;

        listWidget->addItem(filename);
        listWidget->item(i)->setBackground(QBrush(Qt::white));

        if(VST_ON(chan) && VST_preset_data[chan]->filename == filename) {

            listWidget->setCurrentRow(i);
            listWidget->item(i)->setBackground(QBrush(0x8000C0C0));
            curVST_index = i;

        } else if(VST_ON(PRE_CHAN) && VST_preset_data[PRE_CHAN]->filename == filename) {

            listWidget->setCurrentRow(i);

        }

        i++;
    }

#endif
}

void VST_chan::viewVSTfun() {

    if(curVST_index == -1) return;

    if(VST_ON(chan) && VST_preset_data[chan]->filename == listWidget->item(curVST_index)->text())
        load_plugin(listWidget->item(curVST_index));

}

void VST_chan::setVSTDirectory() {

#ifdef __ARCH64__
    if(fluid_output->fluid_settings->value("VST_directory").toString() != "")
        VST_directory = fluid_output->fluid_settings->value("VST_directory").toString();
#else
    if(fluid_output->fluid_settings->value("VST_directory_32bits").toString() != "")
        VST_directory = fluid_output->fluid_settings->value("VST_directory_32bits").toString();
#endif

    QFileDialog d(this, "VST Plugin Directory - Open a VST file to select this directory", VST_directory, "VST Files (*.vst *.dll)");

    if(d.exec() == QDialog::Accepted)
        VST_directory = d.directory().absolutePath();//.getExistingDirectory(this, "VST Plugin Directory", VST_directory);

    if(VST_directory.length())
#ifdef __ARCH64__

        fluid_output->fluid_settings->setValue("VST_directory", VST_directory);
#else

        fluid_output->fluid_settings->setValue("VST_directory_32bits", VST_directory);
#endif

    VST_proc::VST_LoadfromMIDIfile();

    curVST_index = -1;
    listWidget->clear();
    listWidget->setCurrentRow(-1);

    VST_chan::Addfiles();

    listWidget->update();
}

#ifdef __ARCH64__

void VST_chan::setVSTDirectory2() {

    if(fluid_output->fluid_settings->value("VST_directory_32bits").toString() != "")
        VST_directory32bits = fluid_output->fluid_settings->value("VST_directory_32bits").toString();

    QFileDialog d(this, "VST Plugin Directory - Open a VST file to select this directory", VST_directory32bits, "VST Files (*.vst *.dll)");

    if(d.exec() == QDialog::Accepted)
        VST_directory32bits = d.directory().absolutePath();
    if(VST_directory32bits.length()) fluid_output->fluid_settings->setValue("VST_directory_32bits", VST_directory32bits);

    QDir q(VST_directory32bits);
    QStringList plugs = q.entryList(QStringList() << "*.dll" << "*.DLL"  << "*.vst" << "*.VST",QDir::Files);

    VST_proc::VST_LoadfromMIDIfile();

    curVST_index = -1;
    listWidget->clear();
    listWidget->setCurrentRow(-1);

    VST_chan::Addfiles();

    listWidget->update();
}
#endif

void VST_chan::SetVST() {

    if(VST_ON(PRE_CHAN) && (VST_preset_data[PRE_CHAN]->type & 1) && chan >= 16) {

        QMessageBox::information(_parentS, "VST Channel - plugin 2", "Please, load Synth VST plugins in this channel as Plugin 1");

        qDebug("synth module in plugin 2 of channel %i", chan & 15);

        return;
    }

    if(!(chan_loaded!= -1 && chan_loaded < PRE_CHAN && curVST_index!= -1)) {

        if(VST_ON(PRE_CHAN) && VST_preset_data[PRE_CHAN]->vstWidget &&
            (VST_preset_data[PRE_CHAN]->vstEffect || VST_preset_data[PRE_CHAN]->external)) {

            int r = QMessageBox::question(this, "MidiEditor", "This option delete current VST plugin and current presets\nAre you sure to set the new VST plugin?");

            if(r!=QMessageBox::Yes) return;
        }

        VST_proc::VST_unload(chan);

        if(!VST_ON(PRE_CHAN) || !VST_preset_data[PRE_CHAN]->vstWidget ||
            ((!VST_preset_data[PRE_CHAN]->vstEffect || !VST_preset_data[PRE_CHAN]->vstLib) &&
                !VST_preset_data[PRE_CHAN]->external)) return;

        if(externalMux && sys_sema_in && VST_preset_data[PRE_CHAN]->external) {

            VST_preset_data[PRE_CHAN]->on = false;
            if(VST_preset_data[PRE_CHAN]->vstWidget)
                VST_preset_data[PRE_CHAN]->vstWidget->close();

            VST_preset_data[chan] = VST_preset_data[PRE_CHAN];
            if(VST_preset_data[chan]->vstEffect)
                VST_preset_data[chan]->vstEffect->ptr2 = VST_preset_data[chan];

            externalMux->lock();
            int * dat = ((int *) sharedVSText->data()) + 0x40;
            sharedVSText->lock();
            dat[0] = 0x19; // SetVST() do all in external channel
            dat[1] = chan;
            dat[2] = PRE_CHAN;
            sharedVSText->unlock();
            sys_sema_in->release();
            sys_sema_out->acquire();
            externalMux->unlock();
        } else {
            VST_preset_data[PRE_CHAN]->on = false;
            VST_preset_data[PRE_CHAN]->vstWidget->close();

            VST_preset_data[chan] = VST_preset_data[PRE_CHAN];
            if(VST_preset_data[chan]->vstEffect)
                VST_preset_data[chan]->vstEffect->ptr2 = VST_preset_data[chan];
        }

        ((VSTDialog *) VST_preset_data[chan]->vstWidget)->channel = chan;
        ((VSTDialog *) VST_preset_data[chan]->vstWidget)->setWindowTitle("VST channel #" + QString().number(chan & 15) + " plugin " + QString().number(chan/16 + 1));
        VST_preset_data[chan]->on = true;
        ((VSTDialog *) VST_preset_data[chan]->vstWidget)->groupBox->setEnabled(true);

        if(fluid_control && chan < PRE_CHAN) {
            fluid_control->wicon[chan]->setVisible(true);
        }

        chan_loaded = chan;

        for(int i = 0; i < listWidget->count(); i++) {

            listWidget->item(i)->setBackground(QBrush(Qt::white));
            if(VST_preset_data[chan]->filename == listWidget->item(i)->text()) {
                listWidget->setCurrentRow(i);
                listWidget->item(i)->setBackground(QBrush(0x8000C0C0));
                curVST_index = i;
            }
        }

        VST_preset_data[PRE_CHAN] = NULL;

        VST_proc::VST_SaveParameters(chan);

        VST_preset_data[chan]->on = true;

        if(!VST_preset_data[chan]->external &&
                (VST_preset_data[chan]->closed || VST_preset_data[chan]->vstWidget->isHidden())) {

            if(!VST_preset_data[chan]->closed)
                VST_proc::dispatcher(chan, effEditClose, 0, 0, (void *)((VSTDialog *) VST_preset_data[chan]->vstWidget)->subWindow->winId(), 0.0);

            VST_preset_data[chan]->closed = true;
        }

        if(VST_preset_data[chan]->external)
            VST_preset_data[chan]->vstWidget->setVisible(false);
        else {
            if(!VST_preset_data[chan]->closed)
                VST_proc::dispatcher(chan, effEditClose, 0, 0, (void *)((VSTDialog *) VST_preset_data[chan]->vstWidget)->subWindow->winId(), 0.0);
            VST_proc::dispatcher(chan, effEditOpen, 0, 0, (void *)((VSTDialog *) VST_preset_data[chan]->vstWidget)->subWindow->winId(), 0.0);
            VST_preset_data[chan]->closed = false;
            VST_preset_data[chan]->vstWidget->showMinimized();
            VST_preset_data[chan]->vstWidget->showNormal();
            VST_preset_data[chan]->vstWidget->update();

            VST_preset_data[chan]->needUpdate = true;

        }

        emit ((MainWindow *)main_widget)->ToggleViewVST(chan, true);

        close();

    }
}

void VST_chan::ExportVSTData() {
    if(chan == PRE_CHAN || !VST_ON(chan)) return;

    VSTExportDatas * v = new VSTExportDatas(this, chan);
    if(!v) return;
    v->exec();
    delete v;

}

void VST_chan::ImportVSTData() {

    VSTExportDatas * v = new VSTExportDatas(this, chan);
    if(!v) return;
    v->setVisible(false);
    v->ImportVSTfile();
    delete v;
    hide();

}

void VST_chan::DeleteVST() {

    if(curVST_index!= -1) {

        int r = QMessageBox::question(this, "MidiEditor", "Delete VST plugin\nAre you sure?                         ");

        if(r!=QMessageBox::Yes) return;

        MainWindow *MWin = ((MainWindow *) _parentS);
        MidiFile* file = MWin->getFile();

        char id[4]= {0x0, 0x66, 0x66, 'V'};
        id[0] = chan;

        file->protocol()->startNewAction(QString().asprintf("SYSex VST #%i pluging %i Deleted", chan & 15, chan/16 + 1));

        int dtick = file->tick(150);

        QByteArray c;
        int current_tick = 0;

        foreach (MidiEvent* event,
                 *(file->eventsBetween(current_tick-dtick, current_tick+dtick))) {

            SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

            if(sys) {
                c = sys->data();

                if(c[0]==id[0] && c[1]==id[1] && c[2]==id[2] && c[3]==id[3]){

                        file->channel(16)->removeEvent(sys);

                }

            }

        }

        file->protocol()->endAction();

        VST_proc::VST_unload(chan);

        for(int i = 0; i < listWidget->count(); i++) {

            if(listWidget->item(i)->text() != QString(VST_NAME_32BITS_BARRIER))

                listWidget->item(i)->setBackground(QBrush(Qt::white));

        }
    }
    curVST_index = -1;
    listWidget->setCurrentRow(-1);

}


void VST_chan::load_plugin(QListWidgetItem* i) {

    if(curVST_index != -1 && VST_ON(chan) && VST_preset_data[chan]->filename == i->text()) {

        if(!VST_ON(chan) || (!VST_preset_data[chan]->vstEffect && !VST_preset_data[chan]->external)) return;
        chan_loaded = -1;

        qDebug("load_plugin(1) chan: %i", chan);

        if(VST_ON(PRE_CHAN)) VST_proc::VST_unload(PRE_CHAN);

        if(VST_preset_data[chan]->external) {

            if(externalMux && sys_sema_in) {
                externalMux->lock();
                int * dat = ((int *) sharedVSText->data()) + 0x40;
                sharedVSText->lock();
                dat[0] = 0x18; // reset external win
                dat[1] = chan;

                sharedVSText->unlock();
                sys_sema_in->release();
                sys_sema_out->acquire();

                externalMux->unlock();

                VST_preset_data[chan]->closed = false;
                //VST_preset_data[chan]->vstWidget->show();
                VST_preset_data[chan]->vstWidget->setVisible(false);
                VST_proc::VST_external_show(-1);
                VST_proc::VST_external_show(chan);
            }

        } else {
            if(!VST_preset_data[chan]->closed)
                VST_proc::dispatcher(chan, effEditClose, 0, 0, (void *)((VSTDialog *) VST_preset_data[chan]->vstWidget)->subWindow->winId(), 0.0);
            VST_preset_data[chan]->closed = true;

            VST_proc::dispatcher(chan, effEditOpen, 0, 0, (void *)  ((VSTDialog *) VST_preset_data[chan]->vstWidget)->subWindow->winId(), 0.0);
            VST_preset_data[chan]->closed = false;
            VST_preset_data[chan]->vstWidget->showMinimized();
            VST_preset_data[chan]->vstWidget->showNormal();
        }

        chan_loaded = chan;
        close();

    } else {

        chan_loaded = -1;

        qDebug("load_plugin(2) chan: %i", chan);

        QString s = VST_directory + QString("/") +  i->text();

#ifdef __ARCH64__
            if(!QFile::exists(s))
                s = VST_directory32bits + QString("/") +  i->text();
#endif
        VST_proc::Logo(1, "Pre-loading VST Plugin");

// close VST of current channel

        if(VST_ON(chan) && VST_preset_data[chan]->vstWidget) {


            if(VST_preset_data[chan]->external && externalMux && sys_sema_in) {


                externalMux->lock();
                int * dat = ((int *) sharedVSText->data()) + 0x40;
                sharedVSText->lock();
                dat[0] = 0x34; // vstWidget_external_close()
                dat[1] = chan;
                sharedVSText->unlock();
                sys_sema_in->release();
                sys_sema_out->acquire();

                externalMux->unlock();
            }

            VST_preset_data[chan]->vstWidget->close();

        }

        if(VST_proc::VST_load(PRE_CHAN, s) == 0) {
            chan_loaded = PRE_CHAN;
            QCoreApplication::processEvents();
        }

        VST_proc::Logo(0, QString());

    }

}

/************************************************************************************/
/* remoteVST external process functions                                             */
/************************************************************************************/

extern QSystemSemaphore *sys_sema_in;
extern QSystemSemaphore *sys_sema_out;
extern QSystemSemaphore *sys_sema_inW;

int VST_proc::VST_external_mix(int samplerate, int nsamples) {
    if(vst_mix_disable) {
        return 0;
    }

    int chan = 0;
    for(; chan < PRE_CHAN; chan++)
        if(VST_ON(chan) && VST_preset_data[chan]->external) break;
    if(chan >= PRE_CHAN) return 0;

    if(sys_sema_out && sys_sema_in && externalMux && sharedVSText) {
        externalMux->lock();

        int * dat = ((int *) sharedVSText->data()) + 0x40;
        sharedVSText->lock();
        dat[0] = 0x70; // VST_MIX
        dat[1] = samplerate;
        dat[2] = nsamples;
        for(int n = 0; n < 16; n++)
            dat[3 + n] = fluid_output->synth_chanvolume[n];

        sharedVSText->unlock();
        sys_sema_in->release();
        sys_sema_out->acquire();

        externalMux->unlock();
    }
    return 0;
}

int VST_proc::VST_external_unload(int chan) {
    if(externalMux && sys_sema_in) {
        externalMux->lock();
        int * dat = ((int *) sharedVSText->data()) + 0x40;
        sharedVSText->lock();
        dat[0] = 0x11; // unload VST
        dat[1] = chan;
        sharedVSText->unlock();
        sys_sema_in->release();
        sys_sema_out->acquire();
        int ret = dat[1];
        externalMux->unlock();
        return ret;
    }

    return -1;
}

int VST_proc::VST_external_idle(int chan, int cmd) {
    int r = -1;
    if(externalMux && sys_sema_in) {
        externalMux->lock();
        int * dat = ((int *) sharedVSText->data()) + 0x40;
        sharedVSText->lock();
        dat[0] = 0x14; // idle external
        dat[1] = chan;
        dat[2] = cmd;
        sharedVSText->unlock();
        sys_sema_in->release();
        sys_sema_out->acquire();
        r = dat[1];
        externalMux->unlock();
    }
    return r;
}


int VST_proc::VST_external_save_preset(int chan, int preset, QByteArray data) {

    int ret = -1;

    if(externalMux && sys_sema_in) {
        externalMux->lock();
        if(preset > 7) preset = -1;

        int * dat = ((int *) sharedVSText->data()) + 0x40;
        sharedVSText->lock();
        dat[0] = 0x21; // set factory datas
        dat[1] = chan;
        dat[2] = preset;
        dat[3] = data.length();
        memcpy(&dat[4], data.data(), dat[3]);
        sharedVSText->unlock();
        sys_sema_in->release();
        sys_sema_out->acquire();
        ret = dat[1];
        externalMux->unlock();
    }

    return ret;
}

QByteArray VST_proc::VST_external_load_preset(int chan, int preset) {

    QByteArray data;

    if(externalMux && sys_sema_in) {

        externalMux->lock();
        if(preset > 7) preset = -1;

        int * dat = ((int *) sharedVSText->data()) + 0x40;
        sharedVSText->lock();
        dat[0] = 0x20; // get factory datas
        dat[1] = chan;
        dat[2] = preset;
        sharedVSText->unlock();
        sys_sema_in->release();
        sys_sema_out->acquire();
        sharedVSText->lock();
        data.append((char *) &dat[4], dat[3]);
        sharedVSText->unlock();
        externalMux->unlock();
    }

    return data;
}

void VST_proc::VST_external_send_message(int chan, int message, int data1, int data2) {

    if((message == 0xAD105 || externalMux) && sys_sema_in) {
        if(message != 0xAD105)
            externalMux->lock();
        int * dat = ((int *) sharedVSText->data()) + 0x40;
        sharedVSText->lock();
        dat[0] = 0x35;
        dat[1] = chan;
        dat[2] = message;
        dat[3] = data1;
        dat[4] = data2;
        if(message != 0xAD105) {
            dat[8] = VST_preset_data[chan]->disabled;
            dat[9] = VST_preset_data[chan]->needUpdate;
            dat[10] = VST_preset_data[chan]->send_preset;
        }

        sharedVSText->unlock();
        sys_sema_in->release();

        if(message != 0xAD105) {
            sys_sema_out->acquire();
            externalMux->unlock();
        } else
            QThread::msleep(200);

    }

}

int VST_proc::VST_external_show(int chan, int ms) {

    int ret = -1;

    if(externalMux && sys_sema_in) {
        externalMux->lock();
        int * dat = ((int *) sharedVSText->data()) + 0x40;
        sharedVSText->lock();
        dat[0] = 0x33;
        dat[1] = chan;
        dat[2] = ms;
        sharedVSText->unlock();
        sys_sema_in->release();
        sys_sema_out->acquire();

        externalMux->unlock();

        ret = dat[1];

    }

    return ret;
}

void VST_proc::VST_external_MIDIcmd(int chan, int ms, QByteArray cmd) {
    if(externalMux && sys_sema_in) {
        externalMux->lock();
        int * dat = ((int *) sharedVSText->data()) + 0x40;

        dat[0] = 0x40;
        dat[1] = chan;
        dat[2] = ms;
        dat[3] = cmd.length();
        memcpy(&dat[4], cmd.data(), dat[3]);
        sys_sema_in->release();
        sys_sema_out->acquire();

        externalMux->unlock();

    }
}

int VST_proc::VST_external_load(int chan, const QString pathModule) {
    if(externalMux && sys_sema_in) {
        externalMux->lock();
        _block_timer2 = 1;
        vst_mix_disable = 1;

        VST_preset_data_type *VST_temp = new VST_preset_data_type;
        VST_temp->on = false;

        VST_temp->vstPowerOn = false;
        VST_temp->vstVersion = 0;
        VST_temp->vstEffect = NULL;
        VST_temp->vstWidget = NULL;
        VST_temp->vstLib = NULL;
        VST_temp->send_preset = -1;
        VST_temp->mux = NULL;
        VST_temp->external = 1;

        VST_temp->curr_preset = 0;
        VST_temp->disabled = false;
        VST_temp->closed = false;
        VST_temp->needIdle = false;
        VST_temp->needUpdate = false;
        VST_temp->needUpdateMix = true;

        VST_temp->vstWidget = new VSTDialog(_parentS, chan);
        VST_temp->vstWidget->move(0, 0);
        VST_temp->vstWidget->setVisible(false);
        ((VSTDialog *) VST_temp->vstWidget)->subWindow->resize(600, 100);


        VST_preset_data[chan] = VST_temp;
        int * dat = ((int *) sharedVSText->data()) + 0x40;
        sharedVSText->lock();
        dat[0] = 0x10; // load VST
        dat[1] = chan;

        memset((char *) &dat[3], 0, 1024);
        QByteArray s = pathModule.toUtf8();
        memcpy((char *) &dat[3], s.data(), s.length());
        sharedVSText->unlock();
        sys_sema_in->release();
        sys_sema_out->acquire();

        sharedVSText->lock();
        int ret = dat[1];

        if(ret == 0) {
            VST_preset_data[chan]->on = false;
            VST_preset_data[chan]->external_winid = dat[2];
            VST_preset_data[chan]->external_winid2 = dat[3];

            VST_preset_data[chan]->version = dat[4];
            VST_preset_data[chan]->uniqueID = dat[5];
            VST_preset_data[chan]->type = dat[6];
            VST_preset_data[chan]->type_data = dat[7];
            VST_preset_data[chan]->numParams = dat[8];

            VST_preset_data[chan]->filename = QString::fromUtf8((char *) &dat[10], dat[9]);

            //qDebug("filename from ext: %s",(char *) &dat[10]);

            VST_preset_data[chan]->mux = new QMutex(QMutex::NonRecursive);
            VST_preset_data[chan]->on = true;

            qDebug("VST_external_load() ret: %i - winid subwin: %i winid win: %i", ret, dat[2], dat[3]);


        }

        sharedVSText->unlock();
        externalMux->unlock();

        if(ret == 0) {
            VST_proc::VST_external_show(-1);
            VST_proc::VST_external_show(chan);
        }

        _block_timer2 = 0;
        vst_mix_disable = 0;

        return ret;
    }

    _block_timer2 = 0;
    vst_mix_disable = 0;
    return -1;
}


VST_EXT::VST_EXT(MainWindow *w) {

    win = w;

}


VST_EXT::~VST_EXT() {

    DELETE(sys_sema_inW)

}

void VST_EXT::run() {

    while(1) {

        if(!sys_sema_inW->acquire()) break;
        int * dat = ((int *) sharedVSText->data());
        sharedVSText->lock();
        if(dat[0] == 0x123) { // other Midieditor instance ask to me
            dat[0] = 0;
            dat[0x39] = (int) 0xCACABACA; // send a message and continue
            sharedVSText->unlock();
        } else if(dat[0] == 0x666) {
            dat[0] = 0;
            int chan = dat[1];
            int button = dat[2];
            int val = dat[3];
            sharedVSText->unlock();
            emit this->sendVSTDialog(chan, button, val);
        } else if(dat[0] == 0x1) {
            qDebug("VST_EXT ping");
            sharedVSText->unlock();
        }

    }
}

////////////////////////////////////////////////////////////////////////////


VSTlogo::VSTlogo(QWidget* parent, QString text) : QDialog(parent, Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowTitleHint /*| Qt::WindowCloseButtonHint*/) {

    QDialog *Dialog = this;

    if (Dialog->objectName().isEmpty())
        Dialog->setObjectName(QString::fromUtf8("VSTlogo"));
    Dialog->setStyleSheet(QString::fromUtf8("background-color: black;"));
    Dialog->resize(513, 300);
    VSTlabel = new QLabel(Dialog);
    VSTlabel->setObjectName(QString::fromUtf8("VSTlabel"));
    VSTlabel->setGeometry(QRect(132, 15, 250, 156));
    QFont font;
    font.setPointSize(72);
    font.setBold(true);
    font.setWeight(75);
    VSTlabel->setFont(font);
    VSTlabel->setAlignment(Qt::AlignCenter);
    _counterR = 0x40;
    _counterG = 0x20;
    VSTlabel->setStyleSheet(QString::asprintf("color: #%x;  ", 0x000020 + (_counterR << 16) + (_counterG << 8)));

    Text = new QLabel(Dialog);
    Text->setObjectName(QString::fromUtf8("Text"));
    Text->setGeometry(QRect(53, 174, 404, 51));
    QFont font1;
    font1.setPointSize(12);
    Text->setFont(font1);
    Text->setAlignment(Qt::AlignCenter);
    Text->setStyleSheet(QString::fromUtf8("color: white;"));

    Dialog->setWindowTitle("MidiEditor");
    VSTlabel->setText("VST");
    Text->setText((text.isEmpty()) ? "Loading VST Plugings" : text);

    QMetaObject::connectSlotsByName(Dialog);



    time_updat= new QTimer(this);
    time_updat->setSingleShot(false);

    connect(time_updat, SIGNAL(timeout()), this, SLOT(timer_update()), Qt::DirectConnection);
    time_updat->setSingleShot(false);
    time_updat->start(5);


}

void VSTlogo::timer_update() {

    QString s;

    s = QString::asprintf("color: #%x;  ", 0x000020 + (_counterR << 16) + (_counterG << 8));
    VSTlabel->setStyleSheet(s);
    VSTlabel->repaint();

    _counterR+= 5;
    if(_counterR > 255)
        _counterR = 255;
    _counterG+= 4;
    if(_counterG > 255)
        _counterG = 255;

}


VSTExportDatas::VSTExportDatas(QWidget* parent, int chan) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint) {

    QDialog *Dialog = this;
    channel = chan;
    _parent = parent;

    if(!VST_ON(chan)) return;

    _header.clear();
    for(int n = 0; n < 8; n++) {
        _presets[n].clear();
    }

    MainWindow *MWin = ((MainWindow *) _parentS);
    MidiFile* file = MWin->getFile();

    char id[4]= {0x0, 0x66, 0x66, 'V'};
    id[0] = chan;

    int dtick= file->tick(150);

    QByteArray c;

    int current_tick = 0;

    foreach (MidiEvent* event,
             *(file->eventsBetween(current_tick-dtick, current_tick+dtick))) {

        SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

        if(sys) {
            c = sys->data();

            if(id[0] == c[0] && c[1] == id[1] && c[2] == id[2] && c[3] == id[3]) {

                QDataStream qd(&c,
                               QIODevice::ReadOnly);
                qd.startTransaction();

                char head[4];
                unsigned short head2[2];
                qd.readRawData((char *) head, 4);

                if(!(head[0] == id[0] && head[1] == id[1] && // re-test header
                        head[2] == id[2] && head[3] == id[3])) continue;


                qd.readRawData((char *) head, 2);
                decode_sys_format(qd, (void *) &head2[0]);

                if(head[0] == 0) { // filename

                    //qWarning("sys VST Head len %i", c.size());

                    int _uniqueID;
                    int _version;
                    int _numParams;

                    decode_sys_format(qd, (void *) &_uniqueID);
                    decode_sys_format(qd, (void *) &_version);
                    decode_sys_format(qd, (void *) &_numParams);

                    _header = c;

                    for(int pre = 0; pre < 8; pre++) {

                        int clen = 0;

                        int dtick= file->tick(150);

                        QByteArray c;

                        int current_tick = 0;

                        foreach (MidiEvent* event,
                                 *(file->eventsBetween(current_tick-dtick, current_tick+dtick))) {

                            SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

                            if(sys) {

                                c = sys->data();

                                if(c[0]==id[0] && c[1]==id[1] && c[2]==id[2] && c[3]==id[3]){

                                    QDataStream qd(&c,
                                                   QIODevice::ReadOnly);
                                    qd.startTransaction();

                                    char head[4];

                                    qd.readRawData((char *) head, 4);

                                    int dat;

                                    qd.readRawData((char *) head, 2);

                                    if(head[0] == 8 + pre) {
                                        // ignore old presets: don?t supported here
                                        continue;
                                    }

                                    if(head[0] == 16 + pre) { // preset new
                                        //flag_new_vst = true;
                                        clen = 0;
                                        decode_sys_format(qd, (void *) &clen);
                                        if(clen == 0) continue; // ignore

                                        //qWarning("sys VST pre %i len %i %i", pre, c.size(), clen);

                                        decode_sys_format(qd, (void *) &dat);
                                        if(dat != VST_preset_data[chan]->uniqueID) continue;
                                        if(dat != _uniqueID) continue;
                                        decode_sys_format(qd, (void *) &dat);
                                        if(dat != VST_preset_data[chan]->version) continue;
                                        if(dat != _version) continue;
                                        decode_sys_format(qd, (void *) &dat);
                                        if(dat != VST_preset_data[chan]->numParams) continue;
                                        if(dat != _numParams) continue;

                                        _presets[pre] = c;

                                        break;

                                    }
                                }

                            }

                        }

                    }

                }

            }
        }

    }


    if (Dialog->objectName().isEmpty())
        Dialog->setObjectName(QString::fromUtf8("VSTExportDatas"));
    Dialog->setWindowTitle(QString::asprintf("VST Export Datas Channel #%i plugin %i" , channel & 15, 1 + channel / 16));

    Dialog->resize(489, 371);
    buttonBox = new QDialogButtonBox(Dialog);
    buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
    buttonBox->setGeometry(QRect(290, 330, 171, 32));
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Ok);

    groupBoxPresets = new QGroupBox(Dialog);
    groupBoxPresets->setObjectName(QString::fromUtf8("groupBoxPresets"));
    groupBoxPresets->setGeometry(QRect(20, 70, 221 + 10, 51));
    groupBoxPresets->setTitle("Presets");

    for(int n = 0; n < 8; n++) {
        checkBoxPress[n] = new QCheckBox(groupBoxPresets);

        if(_presets[n].count() == 0)
            checkBoxPress[n]->setEnabled(false);
        else
            checkBoxPress[n]->setChecked(true);

        checkBoxPress[n]->setObjectName(QString::fromUtf8("checkBoxPress") + QString::number(n));
        checkBoxPress[n]->setGeometry(QRect(10 + 27 * n, 20, 35, 17));
        checkBoxPress[n]->setText(QString::number(n));
    }

    // checkBoxPress1_2->setGeometry(QRect(40, 20, 35, 17));

    labelFilename = new QLabel(Dialog);
    labelFilename->setObjectName(QString::fromUtf8("labelFilename"));
    labelFilename->setGeometry(QRect(20, 30, 451, 16));
    QString filename = "Filename: " + VST_preset_data[chan]->filename;
    labelFilename->setText(filename);

    groupBoxDestChan = new QGroupBox(Dialog);
    groupBoxDestChan->setObjectName(QString::fromUtf8("groupBoxDestChan"));
    groupBoxDestChan->setGeometry(QRect(260, 70, 211, 121));
    groupBoxDestChan->setTitle("Channel destination");

    spinBoxChan = new QSpinBox(groupBoxDestChan);
    spinBoxChan->setObjectName(QString::fromUtf8("spinBoxChan"));
    spinBoxChan->setGeometry(QRect(10, 20, 41, 41));
    QFont font;
    font.setPointSize(10);
    font.setBold(true);
    font.setWeight(75);
    spinBoxChan->setFont(font);
    spinBoxChan->setAlignment(Qt::AlignCenter);
    spinBoxChan->setMinimum(0);
    spinBoxChan->setMaximum(15);

    pushButtonExport1 = new QPushButton(groupBoxDestChan);
    pushButtonExport1->setObjectName(QString::fromUtf8("pushButtonExport1"));
    pushButtonExport1->setGeometry(QRect(60, 20, 141, 31));
    pushButtonExport1->setText("Press to Export as Plugin 1");

    pushButtonExport2 = new QPushButton(groupBoxDestChan);
    pushButtonExport2->setObjectName(QString::fromUtf8("pushButtonExport2"));
    pushButtonExport2->setGeometry(QRect(60, 70, 141, 31));
    pushButtonExport2->setText("Press to Export as Plugin 2");

    pushButtonExportFile = new QPushButton(Dialog);
    pushButtonExportFile->setObjectName(QString::fromUtf8("pushButtonExportFile"));
    pushButtonExportFile->setGeometry(QRect(260, 210, 211, 31));
    pushButtonExportFile->setText("Press to Export file");

    QObject::connect(buttonBox, SIGNAL(accepted()), Dialog, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), Dialog, SLOT(reject()));
    QObject::connect(pushButtonExport1, SIGNAL(clicked()), Dialog, SLOT(ExportVST1()));
    QObject::connect(pushButtonExport2, SIGNAL(clicked()), Dialog, SLOT(ExportVST2()));
    QObject::connect(pushButtonExportFile, SIGNAL(clicked()), Dialog, SLOT(ExportVSTfile()));

    QMetaObject::connectSlotsByName(Dialog);


}

void VSTExportDatas::ExportVST1() {
    plugin = 0;
    channel2 = spinBoxChan->value();
    ExportVST();
}

void VSTExportDatas::ExportVST2() {
    plugin = 1;
    channel2 = spinBoxChan->value();
    ExportVST();
}

void VSTExportDatas::ExportVST() {

    int chan = channel;
    int chan2 = channel2;

    if(_header.count() == 0) return;

    if(VST_ON(chan) && (VST_preset_data[chan]->type & 1) && (plugin & 1)) {

        QMessageBox::information(_parentS, "VST Channel - plugin 2", "Please, export Synth VST plugins in this channel as Plugin 1");

        qDebug("synth module in plugin 2 of channel %i", chan2 & 15);

        return;
    }

    qWarning("exporting from channel %i to channel %i plugin %i", chan & 15, chan2, (plugin & 1)+ 1);

    MainWindow *MWin = ((MainWindow *) _parentS);
    MidiFile* file = MWin->getFile();

    file->protocol()->startNewAction(QString::asprintf("SYSex VST Export from #%i plugin %i to #%i ", chan & 15, 1 + (plugin & 1), chan2));

    char id[4]= {0x0, 0x66, 0x66, 'V'};
    id[0] = (chan2 & 15) | ((plugin & 1) ? 16 : 0);

    int dtick= file->tick(150);

    QByteArray c;

    int current_tick = 0;

    // deleting VST datas from chan2

    foreach (MidiEvent* event,
             *(file->eventsBetween(current_tick-dtick, current_tick+dtick))) {

        SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

        if(sys) {
            c = sys->data();

            if(id[0] == c[0] && c[1] == id[1] && c[2] == id[2] && c[3] == id[3]) {

                file->channel(16)->removeEvent(sys);

            }
        }
    }

    /*
    SysExEvent *sys_event = new SysExEvent(16, _header, file->track(0));
    file->channel(16)->insertEvent(sys_event, 0);
    */

    // unload/load

    char *data2 = NULL;

    c = _header;
    c[0] = id[0];

    if(c[1]==id[1] && c[2]==id[2] && c[3]==id[3]){

        chan = id[0];
        //id[0] = c[0];

        QDataStream qd(&c,
                       QIODevice::ReadOnly);
        qd.startTransaction();

        char head[4];
        unsigned short head2[2];
        qd.readRawData((char *) head, 4);

        qd.readRawData((char *) head, 2);
        decode_sys_format(qd, (void *) &head2[0]);

        if(head[0] == 0) { // filename

            int _uniqueID;
            int _version;
            int _numParams;

            decode_sys_format(qd, (void *) &_uniqueID);
            decode_sys_format(qd, (void *) &_version);
            decode_sys_format(qd, (void *) &_numParams);

            qWarning("_unique %x", _uniqueID);
            qWarning("_version %x", _version);
            qWarning("_numParams %x", _numParams);

            int len = head[1];

            data2 = (char *) malloc(len + 4);
            if(!data2) return; // out of memory
            memset(data2, 0, len + 4);

            for(int n = 0; n < len; n+= 4) {
                decode_sys_format(qd, (void *) &data2[n]);
            }
            QString filename;
            filename.append(data2);
            free(data2); data2 = NULL;

            int r = -1;

            bool show = VST_proc::VST_isShow(chan);
            //xxx VST_proc::VST_unload(chan);
            QString s = VST_directory + QString("/") +  filename;
#ifdef __ARCH64__
            if(!QFile::exists(s)) // if not exits use 32 bits path
                s = VST_directory32bits + QString("/") +  filename;
#endif

            VST_proc::Logo(1, "Loading VST Plugings");

            c[0] = chan;
            SysExEvent *sys_event = new SysExEvent(16, c, file->track(0));
            file->channel(16)->insertEvent(sys_event, 0);

            r = VST_proc::VST_load(chan, s);

            if(!show) {
                //VST_proc::VST_show(chan, false);
                if(VST_ON(chan)) {
                    if(VST_preset_data[chan]->external) {
                        VST_proc::VST_external_send_message(chan, EXTERNAL_VST_HIDE, 0);
                    } else {
                        if(VST_preset_data[chan]->vstWidget)
                            VST_preset_data[chan]->vstWidget->hide();
                    }
                }

            }

            QCoreApplication::processEvents();

            if(r == 0) {

                if(!VST_ON(chan)) goto end;
                if(!VST_preset_data[chan]->vstEffect && !VST_preset_data[chan]->external) goto end;

                for(int pre = 0; pre < 8; pre++) {

                    QByteArray c;

                    if(((plugin & 128) || checkBoxPress[pre]->isChecked() ) && _presets[pre].count()) {

                        c = _presets[pre];
                        c[0] = chan;
                        SysExEvent *sys_event = new SysExEvent(16, c, file->track(0));
                        file->channel(16)->insertEvent(sys_event, 0);
                    } else
                        continue; // ignore

                    int clen = 0;

                    int last = -1;

                    char *flag = NULL;

                    char *data2 = NULL;

                    if(c[0]==id[0] && c[1]==id[1] && c[2]==id[2] && c[3]==id[3]){

                        QDataStream qd(&c,
                                       QIODevice::ReadOnly);
                        qd.startTransaction();

                        char head[4];

                        qd.readRawData((char *) head, 4);

                        int dat;

                        qd.readRawData((char *) head, 2);

                        if(head[0] == 8 + pre) { // preset

                            continue; // ignore
                        }


                        if(head[0] == 16 + pre) { // preset new
                            clen = 0;
                            decode_sys_format(qd, (void *) &clen);

                            qWarning("asys VST pre %i len %i %i", pre, c.size(), clen);

                            decode_sys_format(qd, (void *) &dat);
                            if(dat != VST_preset_data[chan]->uniqueID) continue;
                            if(dat != _uniqueID) continue;
                            decode_sys_format(qd, (void *) &dat);

                            if(dat != VST_preset_data[chan]->version) continue;
                            if(dat != _version) continue;

                            decode_sys_format(qd, (void *) &dat);
                            if(dat != VST_preset_data[chan]->numParams) continue;
                            if(dat != _numParams) continue;

                            qWarning("bsys VST pre %i len %i %i", pre, c.size(), clen);


                            if(data2) free(data2);
                            data2 = (char *) malloc(clen + 4);
                            if(!data2) {
                                VST_proc::Logo(0, QString());
                                goto end;
                            }

                            memset(data2, 0, clen + 4);

                            for(int n = 0; n < clen; n+= 4) {
                                decode_sys_format(qd, (void *) &data2[n]);
                            }

                            last = 0x666;

                        }


                        if(last < 0 || !data2) { // not found, default factory...

                            VST_preset_data[chan]->preset[pre].clear();

                            if(pre == VST_preset_data[chan]->curr_preset) {

                                if(VST_preset_data[chan]->external) {
                                    _block_timer2 = 1;

                                    VST_proc::VST_external_save_preset(chan, pre);

                                    _block_timer2 = 0;

                                    VST_proc::VST_external_send_message(chan, EXTERNAL_UPDATE_PRESET_BKCOLOR, 0);

                                } else
                                    ((VSTDialog *) VST_preset_data[chan]->vstWidget)->SpinBoxPreset->setStyleSheet(QString::fromUtf8("background-color: white;"));

                                if(flag) free(flag);

                                goto skip;
                            } else {

                                if(VST_preset_data[chan]->external) {
                                    _block_timer2 = 1;

                                    VST_proc::VST_external_save_preset(chan, pre);

                                    _block_timer2 = 0;
                                }


                                if(flag) free(flag);
                                goto skip;
                            }


                        } else {

                            ((VSTDialog *) VST_preset_data[chan]->vstWidget)->SpinBoxPreset->setStyleSheet(QString::fromUtf8("background-color: #8000C000;"));
                        }

                        if(data2) {
                            VST_preset_data[chan]->preset[pre].clear();
                            VST_preset_data[chan]->preset[pre].append(data2, clen);
                        }

                        if(pre != VST_preset_data[chan]->curr_preset
                                && VST_preset_data[chan]->external) {
                            _block_timer2 = 1;

                            VST_proc::VST_external_save_preset(chan, pre, VST_preset_data[chan]->preset[pre]);

                            _block_timer2 = 0;
                        }

                        if(pre == VST_preset_data[chan]->curr_preset) {

                            if(VST_preset_data[chan]->external) {
                                // do nothing

                                _block_timer2 = 1;

                                VST_proc::VST_external_save_preset(chan, pre, VST_preset_data[chan]->preset[pre]);

                                _block_timer2 = 0;
                            } else {

                                if(VST_preset_data[chan]->type_data == 0) {

                                    VstPatchChunkInfo info;

                                    _block_timer2 = 1;

                                    memset(&info, 0, sizeof(info));
                                    info.version = 1;
                                    info.pluginUniqueID = VST_preset_data[chan]->uniqueID;
                                    info.pluginVersion = VST_preset_data[chan]->version;
                                    info.numElements =  VST_preset_data[chan]->numParams;

                                    // Ask the effect if this is an acceptable program
                                    if (VST_proc::dispatcher(chan, effBeginLoadProgram, 0, 0, (void *) &info, 0.0) == -1) {

                                        if(data2) free(data2);
                                        _block_timer2 = 0;
                                        VST_proc::Logo(0, QString());
                                        goto end;
                                    }

                                    VST_proc::dispatcher(chan, effBeginSetProgram, 0, 0, NULL, 0.0);
                                    VST_proc::dispatcher(chan, effSetChunk, 1 , clen, data2, 0.0);
                                    VST_proc::dispatcher(chan, effEndSetProgram, 0, 0, NULL, 0.0);
                                    if(VST_preset_data[chan]->vstWidget->isVisible())
                                        VST_proc::dispatcher(chan, effEditIdle, 0, 0, NULL, 0.0f);

                                    _block_timer2 = 0;

                                } else {

                                    if(VST_preset_data[chan]->numParams != clen/4) {

                                        if(data2) free(data2);
                                        VST_proc::Logo(0, QString());
                                        goto end;

                                    }

                                    _block_timer2 = 1;

                                    for (int i = 0; i < VST_preset_data[chan]->numParams; i++) {
                                        float f;
                                        memcpy(&f, &data2[i * 4], sizeof(float));
                                        VST_proc::setParameter(chan, i, f);
                                    }

                                    if(VST_preset_data[chan]->vstWidget->isVisible())
                                        VST_proc::dispatcher(chan, effEditIdle, 0, 0, NULL, 0.0f);

                                    _block_timer2 = 0;
                                }

                            }

                            if(data2) free(data2);
                            data2 = NULL;
                            if(flag) free(flag);
                        }

skip:
                        if(data2) free(data2);
                        data2 = NULL;
                    }
                }
            }

        }
    }


end:


    file->protocol()->endAction();
    if(data2) free(data2);

    VST_proc::Logo(0, QString());

    hide();

}

void VSTExportDatas::ExportVSTfile() {

    QString appdir = VST_directory;

    QString path = appdir+"/" + VST_preset_data[channel]->filename;

    if(path.endsWith(".dll")) {
        path = path.left(path.count() - 4);
        path += ".vst_exp";
    } else
        path += ".vst_exp";

    QString savePath = QFileDialog::getSaveFileName(this, "Export VST file",
                                                    path, "Export Files (*.vst_exp)");

    if(savePath.isEmpty()) {
        return; // canceled
    }

    if(!savePath.endsWith(".vst_exp")) {
        savePath += ".vst_exp";
    }

    QFile* f = new QFile(savePath);
    if (!f->open(QIODevice::WriteOnly)) {
        QMessageBox::information(this, "Error Creating", savePath);
        return;
    }

    char header[8] = "VSTEXP1";

    if(VST_preset_data[channel]->external)
        header[6] = '2';

    int size = _header.count();
    int len = size;

    for(int n = 0; n < 8; n++) {
        if(!checkBoxPress[n]->isChecked()) _presets[n].clear();
    }

    for(int n = 0; n < 8; n++) {
        size+= _presets[n].count();
    }


    if(f->write(header, 8) < 0) {
        QMessageBox::information(this, "Error Writting", savePath);
        goto error;
    }

    if(f->write((const char *) &size, 4) < 0) {
        QMessageBox::information(this, "Error Writting", savePath);
        goto error;
    }

    if(f->write((const char *) &len, 4) < 0) {
        QMessageBox::information(this, "Error Writting", savePath);
        goto error;
    }

    if(f->write((const char *) _header.constData(), len) < 0) {
        QMessageBox::information(this, "Error Writting", savePath);
        goto error;
    }

    for(int n = 0; n < 8; n++) {
        len= _presets[n].count();
        if(f->write((const char *) &len, 4) < 0) {
            QMessageBox::information(this, "Error Writting", savePath);
            goto error;
        }

        if(f->write((const char *) _presets[n].constData(), len) < 0) {
            QMessageBox::information(this, "Error Writting", savePath);
            goto error;
        }
    }

    f->close();
    hide();
    return;

error:

    f->close();
    f->remove();
    hide();
}

void VSTExportDatas::ImportVSTfile() {

    int chan = channel;

    QString appdir = VST_directory;
    QString filename;

    QString path = appdir;
    QString loadPath = QFileDialog::getOpenFileName(this, "Import VST file",
                                                    path, "Import Files (*.vst_exp)");

    if(loadPath.isEmpty()) {
        return; // canceled
    }

    if(!loadPath.endsWith(".vst_exp")) {
        loadPath += ".vst_exp";
    }

    int r = QMessageBox::question(this, "MidiEditor", "This operation deletes current VST plugin and current presets\nAre you sure?                         ");

    if(r != QMessageBox::Yes) return;


    QFile* f = new QFile(loadPath);
    if (!f->open(QIODevice::ReadOnly)) {
        QMessageBox::information(this, "Error Opening", loadPath);
        return;
    }


    char header2[8];
    int size;
    int size2 = 0;
    int len;
#ifndef __ARCH64__
    int is_32bits = 0;
#endif
    if(f->read(header2, 8) < 0) {
        QMessageBox::information(this, "Error Reading", loadPath);
        goto error;
    }

    if(!strncmp("VSTEXP1", header2, 8)) {
#ifndef __ARCH64__
        is_32bits = 1;
#endif
    } else if(strncmp("VSTEXP2", header2, 8)) {
        QMessageBox::information(this, "Error VST export unknown type", loadPath);
        goto error;
    }

 #ifndef __ARCH64__
    if(!is_32bits) {
        QMessageBox::information(this, "Error VST export 64bits unsupported", loadPath);
        goto error;
    }
 #endif

    if(f->read((char *) &size, 4) < 0) {
        QMessageBox::information(this, "Error Reading", loadPath);
        goto error;
    }

    if(f->read((char *) &len, 4) < 0) {
        QMessageBox::information(this, "Error Reading", loadPath);
        goto error;
    }

    size2+= len;
    if(size < size2) {
        QMessageBox::information(this, "Error file corrupted", loadPath);
        goto error;
    }

    _header = f->read(len);

    if(_header.count() != len) {
        QMessageBox::information(this, "Error Reading", loadPath);
        goto error;
    }

    for(int n = 0; n < 8; n++) {
        if(f->read((char *) &len, 4) < 0) {
            QMessageBox::information(this, "Error Reading", loadPath);
            goto error;
        }

        size2+= len;

        if(size < size2) {
            QMessageBox::information(this, "Error file corrupted", loadPath);
            goto error;
        }

        _presets[n] = f->read(len);
        if(len != _presets[n].count()) {
            QMessageBox::information(this, "Error Reading", loadPath);
            goto error;
        }

    }


    {

        QDataStream qd(&_header,
                       QIODevice::ReadOnly);
        qd.startTransaction();

        char head[4];
        unsigned short head2[2];
        qd.readRawData((char *) head, 4);

        qd.readRawData((char *) head, 2);
        decode_sys_format(qd, (void *) &head2[0]);

        if(head[0] == 0) { // filename

            int _uniqueID;
            int _version;
            int _numParams;

            decode_sys_format(qd, (void *) &_uniqueID);
            decode_sys_format(qd, (void *) &_version);
            decode_sys_format(qd, (void *) &_numParams);

            //qWarning("_unique %x", _uniqueID);
            //qWarning("_version %x", _version);
            //qWarning("_numParams %x", _numParams);

            int len = head[1];

            char *data2 = (char *) malloc(len + 4);
            if(!data2) {
                QMessageBox::information(this, "Error Reading", "Out of memory");
                goto error; // out of memory
            }
            memset(data2, 0, len + 4);

            for(int n = 0; n < len; n+= 4) {
                decode_sys_format(qd, (void *) &data2[n]);
            }

            filename.append(data2);
            free(data2); data2 = NULL;
        } else {
            QMessageBox::information(this, "Error Reading", loadPath);
            goto error;
        }

    }

    {

        QString s = VST_directory + QString("/") +  filename;

#ifdef __ARCH64__

        if(!QFile::exists(s))
            s = VST_directory32bits + QString("/") +  filename;

#endif

        f->close();

        VST_proc::Logo(1, "Loading imported VST Plugin");

        if(VST_proc::VST_load(PRE_CHAN, s) == 0) {

            QCoreApplication::processEvents();

            if(VST_ON(PRE_CHAN) && (VST_preset_data[PRE_CHAN]->type & 1) && (chan & 16)) {

                if(VST_ON(PRE_CHAN)) VST_proc::VST_unload(PRE_CHAN);
                VST_proc::Logo(0, QString());

                QMessageBox::information(_parentS, "VST Channel - plugin 2", "Please, export Synth VST plugins in this channel as Plugin 1");

                return;
            }

            if(VST_ON(PRE_CHAN)) VST_proc::VST_unload(PRE_CHAN);
            //if(VST_ON(chan)) VST_proc::VST_unload(chan);

            plugin = (chan & 16) ? 129 : 128;
            channel2 = (chan & 15);
            ExportVST();

            VST_proc::Logo(0, QString());


        }

    }

    return;


error:
    f->close();
}

#endif

