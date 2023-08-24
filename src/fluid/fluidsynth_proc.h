/*
 * MidiEditor
 * Copyright (C) 2010  Markus Schwenk
 *
 * fluidsynth_proc
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

#ifndef FLUID_FUNC_H
#define FLUID_FUNC_H

#include <QObject>
#include <QThread>
#include <QFile>
#include "fluidsynth.h"
#include <QAudioDeviceInfo>
#include <QAudioInput>
#include <QAudioOutput>
#include <QSettings>
#include "FluidDialog.h"
#include "../midi/MidiPlayer.h"

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QWidget>

#include "../midi/MidiFile.h"
#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/ProgChangeEvent.h"
#include "../MidiEvent/ControlChangeEvent.h"
#include <QMutex>
#include <QWaitCondition>
#include <QSystemSemaphore>
#include <QProcess>
#include <QSharedMemory>

extern QProcess *process;
extern QSharedMemory *sharedAudioBuffer;
extern QSharedMemory *sharedVSText;

extern QSystemSemaphore *sys_sema_in;
extern QSystemSemaphore *sys_sema_out;
extern QSystemSemaphore *sys_sema_inW;

// maximun samples loop Output buffer (minimum 512 = Input Low Latency)
#define FLUID_OUT_SAMPLES  2048
#define ECHO_MAX_SAMPLES 96000

#define FLUID_SYNTH_NAME "** Fluid Synth **"

#define PROC_FILTER_LOW_PASS 1
#define PROC_FILTER_HIGH_PASS 2
#define PROC_FILTER_DISTORTION 4

#define GET_FILTER_ON   0
#define GET_FILTER_FREQ 1
#define GET_FILTER_GAIN 2
#define GET_FILTER_RES  4

class PROC_filter;
class fluid_Thread;
class fluid_Input_Thread;
class QFileW_Thread;

int decode_sys_format(QDataStream &qd, void *data);
void encode_sys_format(QDataStream &qd, void *data);

class ProgressDialog: public QDialog
{
   Q_OBJECT
public:
    QProgressBar *progressBar;
    QLabel *label;

    ProgressDialog(QWidget *parent, QString text);

    QDialog *PBar;

public slots:
    void setBar(int num);
    void reject();

};

class fluidsynth_proc: public QObject
{
    Q_OBJECT

public:
    fluidsynth_proc();
    ~fluidsynth_proc();

    ProgressDialog *_bar;
    bool wavDIS;

    bool use_fluidsynt;
    int disabled;

    fluid_settings_t* settings;
    fluid_synth_t* synth;
    fluid_audio_driver_t* adriver;
    fluid_midi_driver_t* mdriver;

    void MidiClean();
    int SendMIDIEvent(QByteArray array);
    int change_synth(int freq, int flag);
    int MIDIconnect(int on);

    void setSynthChanVolume(int chan, int volume);
    int getSynthChanVolume(int chan);

    void setAudioGain(int chan, int gain);
    int getAudioGain(int chan);

    void setAudioBalance(int chan, int balance);
    int getAudioBalance(int balance);

    void setAudioMute(int chan, bool mute);
    bool getAudioMute(int chan);

    void setAudioDistortionFilter(int on, int chan, int gain);
    void setAudioLowPassFilter(int on, int chan, int freq, int gain, int res);
    void setAudioHighPassFilter(int on, int chan, int freq, int gain, int res);

    int get_param_filter(int type, int chan, int index);

    void setSynthGain(int gain);
    int getSynthGain();

    int MIDtoWAV(QFile *wav, QWidget *parent, MidiFile* file);

    void set_error(int level, const char * err);

    void stop_audio(bool play);
    void new_sound_thread(int freq);
    void wakeup_audio();

    QSettings* fluid_settings;
    QString *sf2_name;

    int wait_signal;
    int iswaiting_signal;

    int _player_status;
    QFile *_player_wav;
    int total_wav_write;

    int wav_is_float;

    fluid_Thread * mixer;
    fluid_Input_Thread * output_aud;

    int sf2_id;

    QAudioOutput *out_sound;
    QIODevice * out_sound_io;
    QAudioDeviceInfo current_sound;
    int output_float;
    int fluid_out_samples; // size of samples loop buffer

    QTime time_frame;
    int frames;
    int _sample_rate;
    int _wave_sample_rate;

    bool audio_chanmute[16];
    int isNoteOn[16];

    // preset datas
    float synth_gain;
    int synth_chanvolume[16];
    int audio_changain[16];
    int audio_chanbalance[16];

    bool filter_dist_on[16];
    float filter_dist_gain[16];

    bool filter_locut_on[16];
    float filter_locut_freq[16];
    float filter_locut_gain[16];
    float filter_locut_res[16];

    bool filter_hicut_on[16];
    float filter_hicut_freq[16];
    float filter_hicut_gain[16];
    float filter_hicut_res[16];

    float level_WaveModulator[16];
    float freq_WaveModulator[16];
// end preset datas

    int cleft, cright;

    short *buffer_in;
    // error
    char error_string[256];
    int it_have_error;

    int status_fluid_err;
    int status_audio_err;

    MidiFile* _file;

    QMutex lock_audio;
    QMutex mutex_fluid;
    QWaitCondition audio_waits;

signals:
    void pause_player();
    void message_timeout(QString title, QString message);
    void changeVolBalanceGain(int index, int vol, int balance, int gain);
    void changeMainVolume(int vol);
    void changeFilterValue(int index, int channel);
    void MidiIn_set_events_to_down(bool f);

private:

    static void fluid_log_function(int level, const char *message, void *data);

    int _float_is_supported;

    int block_midi_events;

};

extern fluidsynth_proc *fluid_output; // main.cpp fluidSynth procedure


class fluid_Thread : public QThread  {
    Q_OBJECT
public:
    fluid_Thread(fluidsynth_proc *proc);
    ~fluid_Thread();

    void run() override;

    PROC_filter *PROC[16];

private:
    void write_header(QFile *f, int size, int sample_rate, int bits, int channels);

    void recalculate_filters(int freq);

    fluidsynth_proc *_proc;

    float *fbuf;
    float **dry;
    float **fx;
    short *buffer;
    float *mix_buffer;

    int fluid_out_samples;  // size of samples loop buffer
    int _float_is_supported;
    int output_float;
};

class fluid_Thread_playerWAV : public QThread {

    Q_OBJECT

public:
    fluid_Thread_playerWAV(fluidsynth_proc *proc);
    void run() override;

    void write_header(QFile *f, int size, int sample_rate, int bits, int channels);

    int init_sequencer_player();
    int sequencer_player();
    int sendCommand(MidiEvent*event);
    int msOfTick(int tick);

private:
    QMultiMap<int, MidiEvent*> * events2;

    int sequencer_tick_pos;
    int sequencer_tick_end;
    fluid_sequencer_t* sequencer;
    fluid_seq_id_t synthSeqID, mySeqID;

    fluidsynth_proc *_proc;

signals:
    void setBar(int num);
    void endBar();
    void changeVolBalanceGain(int index, int vol, int balance, int gain);
    void changeMainVolume(int vol);
    void changeFilterValue(int index, int channel);

};

#undef MAX_DIST_DEF
#define MAX_DIST_DEF 16383

class PROC_filter: public QObject
{
    Q_OBJECT
public:
    PROC_filter(float freq, float freq2, float sample_rate, float gain, float gain2, float gain_dist);
    void PROC_Change_filter(float freq, float freq2,
                           float gain, float gain2, float gain_dist);

    void PROC_set_type(int type, float gain, float gain2);
    void PROC_unset_type(int type);
    int PROC_get_type();

    void PROC_samples(float *left, float *right);

    void PROC_set_sample_rate(int freq) {
       _sample_rate = freq;
    };

    float _resdB;
    float _resdB2;
    int _sample_rate;

private:

    float _Icoefs[3];
    float _Ocoefs[2];
    float _Isleft[3];
    float _Isright[3];
    float _Osleft[2];
    float _Osright[2];

    float _I2coefs[3];
    float _O2coefs[2];
    float _I2sleft[3];
    float _I2sright[3];
    float _O2sleft[2];
    float _O2sright[2];

    float _gain;
    float _gain2;
    float _gain3;

    int _type;


    float _distortion_tab[MAX_DIST_DEF+2];
    float _distortion_tab2[MAX_DIST_DEF+2];

};


// MICROPHONE

int read_header(QFile *f, int &size, int &sample_rate, int &bits, int &channels);
void write_header(QFile *f, int size, int sample_rate, int bits, int channels);

#endif // FLUID_FUNC_H

#endif
