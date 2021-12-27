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
#ifndef FLUIDDIALOG_H
#define FLUIDDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDial>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSlider>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QWidget>

#include <QFrame>
#include <QTimer>
#include <QDialog>
#include <QObject>
#include <qscrollbar.h>
#include <QFileDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QPainter>

#include "../gui/MainWindow.h"

#include "fluidsynth_proc.h"

#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/TextEvent.h"
#include "../midi/MidiChannel.h"
#include "../midi/MidiFile.h"
#include "../protocol/Protocol.h"
#include "../VST/VST.h"

int addInstrumentNamesFromSF2(const char *filename);

class QVLabel;

class FluidDialog: public QDialog
{
    Q_OBJECT
public:
    QWidget* _parent;
    int disable_mainmenu;

    QDialogButtonBox *buttonBox;
    QTabWidget *tabWidget;

    // tab 1
    QWidget *MainVolume;
    QScrollArea *scrollArea;
    QGroupBox *groupChan[16];
    QSlider *ChanVol[16];
    QSlider *BalanceSlider[16];
    QLabel *BalanceLabel[16];
    QLabel *Chan[16];
    QDial *chanGain[16];
    QLabel *label;
    QLabel *chanGainLabel[16];
    QVLabel *qv[16];
    QPushButton *wicon[32];

    QGroupBox *groupMainVol;
    QSlider *MainVol;
    QLabel *Main;

    QGroupBox *groupVUm;
    QFrame *line_l[25];
    QFrame *line_r[25];

    QGroupBox *groupM;
    QFrame *line[16];

    QGroupBox *groupE;
    QSpinBox *spinChan;
    QLabel *labelChan;

    QGroupBox *LowCutBox;
    QLabel *label_low_gain;
    QLabel *label_low_freq;
    QLabel *label_low_res;
    QLabel *label_low_disp;
    QDial *LowCutGain;
    QLabel *label_low_disp2;
    QDial *LowCutFreq;
    QLabel *label_low_disp3;
    QDial *LowCutRes;
    QPushButton *LowCutButton;
    QGroupBox *DistortionBox;
    QLabel *label_dist_gain;
    QLabel *label_distortion_disp;
    QDial *DistortionGain;
    QPushButton *DistortionButton;

    QGroupBox *HighCutBox;
    QLabel *label_high_gain;
    QLabel *label_high_freq;
    QLabel *label_high_res;
    QLabel *label_high_disp;
    QDial *HighCutGain;
    QLabel *label_high_disp2;
    QDial *HighCutFreq;
    QLabel *label_high_disp3;
    QDial *HighCutRes;
    QPushButton *HighCutButton;

    QGroupBox *PresetBox;
    QSpinBox *spinPreset;
    QPushButton *PresetLoadButton;
    QPushButton *PresetSaveButton;
    QPushButton *PresetResetButton;
    QPushButton *PresetLoadPButton;
    QPushButton *PresetStorePButton;
    QPushButton *PresetDeletePButton;
    QPushButton *PresetStoreButton;

    int channel_selected;
    int preset_selected;

    QTimer *time_updat;

    // tab 2
    QWidget *tabConfig;
    QComboBox *sf2Box;
    QToolButton *loadSF2Button;
    QToolButton *MIDIConnectbutton;
    QToolButton *MIDISaveList;

    QGroupBox *OutputAudio;
    QComboBox *OutputBox;
    QLabel *Outputlabel;
    QComboBox *OutputFreqBox;
    QLabel *OutputFreqlabel;
    QLabel *bufflabel;
    QSlider *bufferSlider;
    QLabel *buffdisp;
    QCheckBox *OutFloat;

    QGroupBox *WavAudio;
    QComboBox *WavFreqBox;
    QLabel *WavFreqlabel;
    QCheckBox *checkFloat;
    QLabel *vlabel2;
    QLabel *status_fluid;
    QLabel *status_audio;

    QGroupBox *MP3Audio;
    QComboBox *MP3BitrateBox;
    QLabel *MP3Bitratelabel;
    QRadioButton *MP3ModeButton;
    QRadioButton *MP3ModeButton_2;
    QLabel *MP3label_2;
    QCheckBox *MP3VBRcheckBox;
    QCheckBox *MP3HQcheckBox;
    QLineEdit *MP3_lineEdit_title;
    QLabel *MP3label_title;
    QLabel *MP3label_artist;
    QLabel *MP3label_album;
    QLabel *MP3label_genre;
    QLineEdit *MP3_lineEdit_artist;
    QLineEdit *MP3_lineEdit_album;
    QLineEdit *MP3_lineEdit_genre;
    QLabel *MP3label_year;
    QLabel *MP3label_track;
    QSpinBox *MP3spinBox_year;
    QSpinBox *MP3spinBox_track;
    QCheckBox *MP3checkBox_id3;
    QPushButton *MP3Button_save;


    FluidDialog(QWidget* parent);
    void tab_MainVolume(QDialog *FluidDialog);
    void tab_Config(QDialog */*FluidDialog*/);

public slots:

    void update_status();

    // tabMainVolume
    void distortion_clicked();
    void distortion_gain(int gain);

    void lowcut_clicked();
    void lowcut_gain(int gain);
    void lowcut_freq(int freq);
    void lowcut_res(int res);

    void highcut_clicked();
    void highcut_gain(int gain);
    void highcut_freq(int freq);
    void highcut_res(int res);

    void StorePressets();
    void LoadPressets();
    void DeletePressets();

    void StoreSelectedPresset();
    void LoadSelectedPresset(int current_tick);
    void ResetPresset();

    void SaveRPressets();
    void LoadRPressets();

    // config

    void load_SF2();
    void load_SF2(QString path);
    void MIDIConnectOnOff();
    void MIDISaveListfun();

    void setOutput(int index);
    void setOutputFreq(int index);
    void setWavFreq(int index);

    void save_MP3_settings();

    void timer_update();
    void reject();

    void changeVolBalanceGain(int index, int vol, int balance, int gain);
    void changeMainVolume(int vol);
    void changeFilterValue(int index, int channel);
  protected:
    void mousePressEvent(QMouseEvent* event);

  private:
    int _current_tick;
    int _edit_mode;
    int _save_mode;

    QList<QAudioDeviceInfo> dev_out;


};

extern int _cur_edit;
extern FluidDialog *fluid_control;

#endif
#endif

