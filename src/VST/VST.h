/*
 * MidiEditor
 * Copyright (C) 2010  Markus Schwenk
 *
 * VST.h
 * Copyright (C) 2021 Francisco Munoz / "Estwald"
 * Copyright (C) Dominic Mazzoni
 * Copyright (C) Audacity: A Digital Audio Editor
 * This code is partially based in VSTEffect.cpp from Audacity
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
#ifndef VST_FUNC_H
#define VST_FUNC_H
#include "../fluid/fluidsynth_proc.h"
#include <QLibrary>
#include "aeffectx.h"

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QSlider>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QDesktopWidget>

#define PRE_CHAN 32 // 16 chans * number of VST Plugins

typedef struct {
    bool on;
    int type;
    bool needIdle;
    bool needUpdate;

    int vstVersion;
    bool vstPowerOn;

    AEffect *vstEffect;
    QWidget *vstWidget;
    QLibrary *vstLib;

    QString filename;
    unsigned int uniqueID;
    unsigned int version;
    unsigned int numParams;
    int type_data;
    bool disabled;
    bool closed;
    int curr_preset;
    QByteArray factory;
    QByteArray preset[8];

    int send_preset;

} VST_preset_data_type;

class VSTDialog: public QDialog
{
    Q_OBJECT
public:
    QGroupBox *groupBox;
    QPushButton *pushButtonSave;
    QPushButton *pushButtonReset;
    QPushButton *pushButtonDelete;
    QPushButton *pushButtonSet;
    QPushButton *pushButtonDis;
    QPushButton *pushButtonUnset;
    QLabel *labelPreset;
    QSpinBox *SpinBoxPreset;

    QScrollArea *scrollArea;
    QWidget *subWindow;

    QWidget* _parent;

    int channel;

    QTimer *time_updat;

    VSTDialog(QWidget* parent, int chan);
    ~VSTDialog();

public slots:
    void Save();
    void Reset();
    void Delete();
    void Set();
    void Dis();
    void Unset();
    void ChangePreset(int sel);
    void ChangeFastPresetI(int sel);
    void ChangeFastPreset(int sel);
    void timer_update();

};

class VST_proc: public QObject
{
    Q_OBJECT

public:
    VST_proc();
    ~VST_proc();

    static void VST_setParent(QWidget *parent);

    static int VST_load(int chan, const QString pathModule);
    static int VST_unload(int chan);
    static void VST_Resize(int chan, int w, int h);
    static int VST_exit();
    static int VST_mix(float**in, int nchans, int deltaframe, int nsamples);
    static int VST_isLoaded(int chan);

    static bool VST_isMIDI(int chan);
    static void VST_MIDIcmd(int chan, int ms, QByteArray cmd);
    static void VST_MIDIend();
    static void VST_MIDInotesOff(int chan);

    static void VST_PowerOn(int chan);
    static void VST_PowerOff(int chan);

    static bool VST_isShow(int chan);
    static void VST_show(int chan, bool show);

    static void VST_DisableButtons(int chan, bool disable);
    static int VST_LoadParameterStream(QByteArray array);

    static int VST_SaveParameters(int chan);

    static int VST_LoadfromMIDIfile();
    static int VST_UpdatefromMIDIfile();


};


class VST_chan: public QDialog
{
    Q_OBJECT
public:
    QDialogButtonBox *buttonBox;
    QPushButton *pushVSTDirectory;
    QGroupBox *GroupBoxVST;
    QPushButton *pushButtonSetVST;
    QPushButton *viewVST;
    QPushButton *pushButtonDeleteVST;
    QLabel *labelinfo;
    QListWidget *listWidget;

    int curVST_index;
    int chan;
    int chan_loaded;

    VST_chan(QWidget* parent, int channel, int flag);

public slots:
    void load_plugin(QListWidgetItem* i);
    void setVSTDirectory();
    void SetVST();
    void DeleteVST();
    void viewVSTfun();

};
#endif
#endif