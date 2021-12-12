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



#ifndef VST_FUNC_H
#define VST_FUNC_H

#include <QLibrary>
#include "aeffectx.h"
#include "../mainwindow.h"

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QSlider>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QDesktopWidget>
#include <QTimer>
#include <QSemaphore>
#include <QMutex>
#include <QThread>

#define DELETE(x) {if(x) delete x; x= NULL;}

extern QWidget *main_widget;

extern QApplication *app;

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
    int winid;

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

} VST_preset_data_type;

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

    QTimer *time_updat;
    QSemaphore *semaf;
    bool _dis_change;

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
    void setBackColor(int sel);

public slots:

    void BackColor(int sel);
    void Save();
    void Reset();
    void Delete();
    void Set();
    void Dis();
    void Unset();
   /* void ChangePreset(int sel);*/
    void ChangeFastPresetI(int sel);
    void ChangeFastPresetI2(int sel);
    void ChangeFastPreset(int sel);

    void timer_update();

protected:
    void closeEvent(QCloseEvent *event) override;

};


class VST_IN : public QThread {
    Q_OBJECT
public:

    VST_IN(MainWindow *w);
    ~VST_IN();

    void run() override;

signals:
    void run_cmd_events();
    void force_exit();

private:
    MainWindow *win;
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
    static int VST_mix(float**in, int nchans, int samplerate, int nsamples);
    static int VST_mix2(float **in, int chan, int samplerate, int nsamples);
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

    static bool VST_isEnabled(int chan);

};

#if 0
class VST_chan: public QDialog
{
    Q_OBJECT

private:

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

public:

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

