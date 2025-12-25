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
//#include <QtWidgets/QDesktopWidget>
#include <QSemaphore>
#include <QMutex>
#include "../gui/MainWindow.h"
#if defined(__SSE2__)
#include <cmath>
#include <emmintrin.h> // SSE2
#endif

#define DELETE(x) {if(x) delete x; x= NULL;}

extern QWidget *main_widget;
extern QMutex * externalMux;

#define PRE_CHAN SYNTH_CHANS*2 // 48 chans * number of VST Plugins
#define VST_NAME_32BITS_BARRIER "<----- 32 bits VST ----->"

#define EXTERNAL_UPDATE_WINSETPRESET   0xABECE5
#define EXTERNAL_UPDATE_PRESET_BKCOLOR 0xABCE50
#define EXTERNAL_VST_SHOW              0xC0C0FE0
#define EXTERNAL_VST_MIDINOTEOFF       0xC0C0FE2
#define EXTERNAL_VST_DISABLEBUTTONS    0xCACAFEA
#define EXTERNAL_VST_HIDE              0xC0C0FE1

typedef struct {
    bool on;
    int type;
    int external;
    bool needIdle;
    bool needUpdate;
    bool needUpdateMix;

    int vstVersion;
    bool vstPowerOn;

    AEffect *vstEffect;
    QWidget *vstWidget;
    QLibrary *vstLib;

    QString filename;
    int uniqueID;
    int version;
    int numParams;
    int type_data;
    bool disabled;
    bool closed;
    int curr_preset;
    QByteArray factory;
    QByteArray preset[8];

    int send_preset;

    QMutex *mux;

    int external_winid;
    int external_winid2;

} VST_preset_data_type;

typedef struct {
    float frequency;       // 20Hz - 2000Hz
    float rotationSpeedBass;   // 0.1Hz - 10Hz
    float depthBass;           // 0.0 - 1.0 (amplitud)
    float rotationSpeedTreble;   // 0.1Hz - 10Hz
    float depthTreble;           // 0.0 - 1.0 (amplitud)
    float phaseBass;
    float phaseTreble;
    float lastSampleLB;
    float lastSampleRB;
    float lastSampleLT;
    float lastSampleRT;
    float lastInputLT;
    float lastInputRT;
} LeslieSpeaker;

typedef struct {

    int last_value;
    float comp;
    float sr;
    float freqMin;   // Low Freq (ej. 300 Hz)
    float freqMax;   // High Freq (ej. 2000 Hz)
    float currentFreq;
#if defined(__SSE2__)
    __m128 aa0, aa1, aa2, bb1, bb2;
#endif
    float a0, a1, a2, b1, b2;

    float xHistory[2][2];
    float yHistory[2][2];

} WahWahControl;

class VST_EXT : public QThread {
    Q_OBJECT
public:

    VST_EXT(MainWindow *w);
    ~VST_EXT();

    void run() override;

signals:
    void sendVSTDialog(int chan, int button, int val);

private:
    MainWindow *win;
};

class VSTDialog: public QDialog
{
    Q_OBJECT

private:

    QPushButton *pushButtonSave;
    QPushButton *pushButtonReset;
    QPushButton *pushButtonDelete;
    QPushButton *pushButtonSet;
    QPushButton *pushButtonDis;
    QPushButton *pushButtonUnset;
    QLabel *labelPreset;
    QWidget* _parent;

    QTimer *time_update;
    QSemaphore *semaf;
    bool _dis_change;

    bool in_use;

public:
    QGroupBox *groupBox;
    QSpinBox *SpinBoxPreset;
    QScrollArea *scrollArea;
    QWidget *subWindow;

    int channel;

    VSTDialog(QWidget* parent, int chan);
    ~VSTDialog();

signals:
    void setPreset(int preset);

public slots:
    void accept() override;
    void recVSTDialog(int chan, int button, int val);
    void Save();
    void Reset();
    void Delete();
    void Set();
    void Dis();
    void Unset();
    void ChangePreset(int sel);
    void ChangeFastPresetI(int sel);
    void ChangeFastPresetI2(int sel);
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

    static intptr_t dispatcher(int chan, int b, int c, intptr_t d, void * e, float f);
    static void process(int chan, float * * b, float * * c, int d);
    static void setParameter(int chan, int b, float c);
    static float getParameter(int chan, int b);
    static void processReplacing(int chan, float * * b, float * * c, int d);  

    static int VST_load(int chan, const QString pathModule);
    static int VST_unload(int chan);
    static void VST_Resize(int chan, int w, int h);
    static int VST_exit();
    static void VST_LeslieReset();
    static void VST_LeslieEffect(float *left, float *right, float samplerate, int nsamples, LeslieSpeaker *Leslie);
    static void VST_WahWahReset(int samplerate);
    static void Wah_Wah_Control(int ch, int value);
    static void VST_Wah_Wah(int ch, float *left, float *right, float samplerate, int nsamples);
    static int VST_mix(float**in, int nchans, int samplerate, int nsamples, int mode = 0);
    static int VST_isLoaded(int chan);
    static bool VST_mix_disable(bool disable);

    static bool VST_isMIDI(int chan);
    static void VST_MIDIcmd(int chan, int ms, QByteArray cmd);
    static void VST_MIDIvol(int chan, int vol);
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

    static bool VST_isEnabled(int chan);

    static void Logo(int mode, QString text);

    static int VST_external_load(int chan, const QString pathModule);
    static int VST_external_unload(int chan);
    static int VST_external_mix(int samplerate, int nsamples);
    static int VST_external_idle(int chan, int cmd);
    static int VST_external_save_preset(int chan, int preset, QByteArray data = 0);
    static QByteArray VST_external_load_preset(int chan, int preset);
    static int VST_external_show(int chan, int ms = 0);
    static void VST_external_MIDIcmd(int chan, int ms, QByteArray cmd);
    static void VST_external_send_message(int chan, int message, int data1 = 0, int data2 = 0);

    static bool leslieON[SYNTH_CHANS];
    static LeslieSpeaker leslie[SYNTH_CHANS];
    static WahWahControl wah[SYNTH_CHANS];

  private:
#if defined(__SSE2__)
    static void VST_LeslieEffect_SSE2(float *left, float *right, float samplerate, int nsamples, LeslieSpeaker *Leslie);
#endif
};

class VST_chan: public QDialog
{
    Q_OBJECT

private:

    QDialogButtonBox *buttonBox;
    QPushButton *pushVSTDirectory;
#ifdef __ARCH64__
    QPushButton *pushVSTDirectory2;
#endif
    QGroupBox *GroupBoxVST;
    QPushButton *pushButtonSetVST;
    QPushButton *pushButtonExportVSTData;
    QPushButton *pushButtonImportVSTData;
    QPushButton *viewVST;
    QPushButton *pushButtonDeleteVST;
    QLabel *labelinfo;
    QListWidget *listWidget;

    int curVST_index;
    int chan;
    int chan_loaded;

public:

    VST_chan(QWidget* parent, int channel, int flag);
    void Addfiles();

public slots:
    void load_plugin(QListWidgetItem* i);
    void setVSTDirectory();
#ifdef __ARCH64__
    void setVSTDirectory2();
#endif
    void SetVST();
    void ExportVSTData();
    void ImportVSTData();
    void DeleteVST();
    void viewVSTfun();
};

class VSTlogo: public QDialog
{
    Q_OBJECT

private:
    int _counterR;
    int _counterG;

public:
    QLabel *VSTlabel;
    QLabel *Text;
    QTimer *time_update;

    VSTlogo(QWidget* parent, QString text);

public slots:
    void timer_update();
};



class VSTExportDatas: public QDialog
{
    Q_OBJECT

private:

    QWidget* _parent;
    QByteArray _header;
    QByteArray _presets[8];
    int plugin;
    int channel;
    int channel2;

public:
    QDialogButtonBox *buttonBox;
    QGroupBox *groupBoxPresets;
    QCheckBox *checkBoxPress[8];
    QLabel *labelFilename;
    QGroupBox *groupBoxDestChan;
    QSpinBox *spinBoxChan;
    QPushButton *pushButtonExport1;
    QPushButton *pushButtonExport2;
    QPushButton *pushButtonExportFile;

    VSTExportDatas(QWidget* parent, int chan);
    void ExportVST();

signals:


public slots:
    //void accept() override;
    void ExportVST1();
    void ExportVST2();
    void ExportVSTfile();
    void ImportVSTfile();

};

#endif
#endif
