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

#include "fluidsynth_proc.h"
#include "../VST/VST.h"
//#include "../fluid/FluidDialog.h"
#include "../MidiEvent/TempoChangeEvent.h"
#include "../midi/MidiChannel.h"
#include "../midi/MidiInControl.h"
#include "math.h"
#include <QMessageBox>
#include <QSemaphore>
#include <QDateTime>

#if 0
#include <windows.h>

int sys_mem() {
    MEMORYSTATUSEX memory_status;
    ZeroMemory(&memory_status, sizeof(MEMORYSTATUSEX));
    memory_status.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memory_status)) {

            return memory_status.ullAvailVirtual / (1024 * 1024);

    }
   return -1;
}
#endif

#define MSG_ERR qWarning
#define MSG_OUT qDebug

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

static int handle_midi_event(void* /*data*/, fluid_midi_event_t* event)
{

    if(!fluid_output->synth) return 0;

    // TO DO: alternative MIDI Input from fluidsynth

    switch(fluid_midi_event_get_type(event)) {

        case 0x90: // note on
            fluid_output->isNoteOn[0]=4;
            return fluid_synth_noteon(fluid_output->synth, 0, fluid_midi_event_get_key(event), fluid_midi_event_get_velocity(event));
        case 0x80: // note off
            return fluid_synth_noteoff(fluid_output->synth, 0, fluid_midi_event_get_key(event));
    }

    return 0;
}

int addInstrumentNamesFromSF2(const char *filename);

int fluidsynth_proc::change_synth(int freq, int flag) {

    int disabled = 0;

    mutex_fluid.lock();

    if(mdriver) delete_fluid_midi_driver(mdriver);
    mdriver = NULL;

    int id = sf2_id;
    sf2_id = -1;

    if(id >= 0 && synth) {
        //fluid_synth_program_reset(synth);
        for(int n = 0; n < 16; n++)
            fluid_synth_unset_program(synth, n);

        MSG_OUT("unloading the soundfont %i\n", id);
        fluid_synth_sfunload(synth, id, 0);
        QThread::msleep(100);

        sf2_id = -1;

    }

    if(synth) delete_fluid_synth(synth);
    synth = NULL;

    if(settings) delete_fluid_settings(settings);

    settings = new_fluid_settings();
    if(settings) {
        fluid_settings_setnum(settings, "synth.gain", synth_gain);
        fluid_settings_setint(settings, "synth.audio-channels", 16);
        fluid_settings_setint(settings, "synth.audio-groups", 16);
        fluid_settings_setint(settings, "synth.effects-groups", 16);
        fluid_settings_setint(settings, "synth.chorus.active", 1);
        fluid_settings_setint(settings, "synth.reverb.active", 1);
        fluid_settings_setnum(settings, "synth.sample-rate", freq);
        fluid_settings_setint(settings, "synth.cpu-cores", 4);
       // fluid_settings_setint(settings, "midi.realtime-prio", 90);
        fluid_settings_setint(settings, "midi.autoconnect", 0);

        synth = new_fluid_synth(settings);
        if(!synth) {
            status_fluid_err = 2;
            MSG_ERR("FATAL ERROR: error in new_fluid_synth()");
            set_error(666, "FATAL ERROR: error in new_fluid_synth()");
            mutex_fluid.unlock();
            return -1;
        }

        if(!disabled) {

            if(!sf2_name) {
                if (fluid_settings && fluid_settings->value("sf2_path").toString() != "") {
                    sf2_name = new QString(fluid_settings->value("sf2_path").toString());
                }
            }

            if (sf2_name) {

                MSG_OUT("changesynth: New Sound font %s\n", (const char *) sf2_name->toUtf8());
                id = fluid_synth_sfload(synth,  (const char *) sf2_name->toUtf8(), 1);
                if(id >= 0) addInstrumentNamesFromSF2((const char *) sf2_name->toUtf8());

            } else id = FLUID_FAILED;

            if(id == FLUID_FAILED) {
                disabled = 2;
                status_fluid_err = 3;
                MSG_ERR("error loading SF2 file");
                set_error(0, "error loading SF2 file");

            }

        }
    } else {
        status_fluid_err = 1; disabled = 1;
        set_error(666, "FATAL ERROR: error in new_fluid_settings()");
    }

    if(!disabled && flag == 1) {
        //qWarning("send volume...\n");
        for(int n=0; n < 16; n++) {
            fluid_synth_cc(synth, n , 11, synth_chanvolume[n]);
        }
    }

    if(!disabled) {
        MSG_OUT("Fluidsynth changed at %i Hz\n", freq);
        status_fluid_err = 0;
    }

    sf2_id = id;
    mutex_fluid.unlock();
    return disabled;
}

void fluidsynth_proc::fluid_log_function(int level, const char *message, void *data) {
    if(message) {
        if(strstr(message, "Ringbuffer") && strstr(message, "full")) {
            if (MidiPlayer::isPlaying())
                emit fluid_output->pause_player();
        }
    } else fluid_default_log_function	(level, message, data);
}

fluidsynth_proc::fluidsynth_proc()
{
    disabled = 0;
    settings = NULL;
    synth = NULL;
    adriver =  NULL;
    use_fluidsynt = true;
    _bar = NULL;
    sf2_id = -1;
    mdriver = NULL;
    wavDIS = true;

    status_fluid_err = 255;
    status_audio_err = 255;
    block_midi_events = 0;

    out_sound = NULL;
    out_sound_io = NULL;

    mixer = NULL;

    fluid_set_log_function(FLUID_WARN, 	(fluid_log_function_t )fluid_log_function, NULL);

    fluid_settings = new QSettings(QString("Fluid_MidiEditor"), QString("NONE"));

    fluid_settings->setValue("mp3_title", "");
    //fluid_settings->setValue("mp3_artist", "");
    fluid_settings->setValue("mp3_album", "");
    fluid_settings->setValue("mp3_genre", "");
    if(fluid_settings->value("mp3_bitrate").toInt() == 0)
        fluid_settings->setValue("mp3_bitrate", 5);
    fluid_settings->setValue("mp3_mode", true);
    fluid_settings->setValue("mp3_vbr", false);
    fluid_settings->setValue("mp3_hq", false);
    fluid_settings->setValue("mp3_id3", true);
    fluid_settings->setValue("mp3_year", QDate::currentDate().year());
    fluid_settings->setValue("mp3_track", 1);

    sf2_name = NULL;

    _player_status = 0;
    _player_wav =NULL;

    wait_signal = 0;
    iswaiting_signal = 0;

    set_error(0, NULL);
    sf2_id = FLUID_FAILED;

    synth_gain = 1.0;

    for(int n = 0; n < 16; n++) {
        synth_chanvolume[n] = 127; // initialize expresion
        audio_chanbalance[n] = 0;
        audio_changain[n] = 0;
        audio_chanmute[n] = false;
        isNoteOn[n] = 0;

        filter_dist_on[n] = false;
        filter_dist_gain[n] = 0.0;

        filter_locut_on[n] = false;
        filter_locut_freq[n] = 500;
        filter_locut_gain[n] = 0.0;
        filter_locut_res[n] = 0.0;

        filter_hicut_on[n] = false;
        filter_hicut_freq[n] = 2500;
        filter_hicut_gain[n] = 0.0;
        filter_hicut_res[n] = 0.0;

    }

    // audio device

    QString device_name;
    const QAudioDeviceInfo &defaultDeviceInfo = QAudioDeviceInfo::defaultOutputDevice();

    if (fluid_settings->value("Audio Device").toString() != "") {
        device_name = fluid_settings->value("Audio Device").toString();
    }

    _sample_rate = fluid_settings->value("Audio Freq" , 44100).toInt();
    _wave_sample_rate = fluid_settings->value("Wav Freq" , 44100).toInt();
    fluid_out_samples = fluid_settings->value("Out Samples" , 512).toInt();


    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);

    int max_out = devices.count();
    int def = -1;
    for(int n = 0; n < max_out; n++) {

        if(devices.at(n).deviceName() == device_name) {
            def = n; break;
        }

        if(def < 0 && devices.at(n).deviceName() == defaultDeviceInfo.deviceName()) def = n;
    }

    if(max_out < 1) {
        MSG_ERR("ERROR: Audio Device not found\n");
        QMessageBox::critical(NULL, "Fluidsynth_proc Error", "Audio Device not found\n.");
        return;

    }

    if(def < 0) def = 0; // get by default

    if(def >= 0) {
        QList<int> s = devices.at(def).supportedSampleRates();

        int m = s.count();
        int freq = -1;

        for(int n = 0; n < m; n++) {
            if(_sample_rate == s.at(n)) {
                freq = _sample_rate;
                break;
            }

        }

        if(freq < 0) _sample_rate = 44100;
    }

    output_float = fluid_settings->value("Out use float" , 0).toInt();
    _float_is_supported = 0;
    QAudioFormat format;

    format.setSampleRate(_sample_rate);
    format.setChannelCount(2);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    if(def >= 0) {

        QAudioDeviceInfo info(devices.at(def));

        format.setSampleSize(32);
        format.setSampleType(QAudioFormat::Float);

        info.isFormatSupported(format);
        if(!info.isFormatSupported(format)) {
            format.setSampleSize(16);
            format.setSampleType(QAudioFormat::SignedInt);
            output_float = 0;
        } else {
            _float_is_supported = 1;
            if(!output_float) {
                format.setSampleSize(16);
                format.setSampleType(QAudioFormat::SignedInt);
            }
        }
    }

    current_sound = devices.at(def);

    MSG_OUT("audio output:\n    sample_rate: %i use float %i\n", _sample_rate, output_float);

    //for(int n = 0; n < 32; n++) // test if memory leak of fluid synth (version < 2.1.6)
    disabled = change_synth(_sample_rate, 0);

    if(disabled == -1) return;

    if(!disabled) {
        status_fluid_err = 0;

        for(int n = 0; n < 16; n++) {
            if(!disabled) fluid_synth_cc(synth, n , 11, synth_chanvolume[n]);         
        }

        if(def >= 0)
            out_sound= new QAudioOutput(devices.at(def), format);

        if(out_sound) {

            out_sound_io = out_sound->start();

            if(!out_sound_io) {
                status_audio_err = 2;
                MSG_ERR("FATAL ERROR: error in QAudioOutput()");
            } else status_audio_err = 0;

        } else {
            status_audio_err = 1;
            MSG_ERR("FATAL ERROR: error in QAudioOutput()");
        }

        if(!status_fluid_err)
            mixer = new fluid_Thread(this);
        else
            mixer = NULL;

        if(!mixer)  {
            status_fluid_err = 4;
            set_error(666, "FATAL ERROR: error creating fluid_Thread()"); return;
        } else
            status_fluid_err = 0;

        mixer->start(QThread::TimeCriticalPriority);


    } else {
        disabled = 1;
       // status_fluid_err = 1;
       // set_error(666, "FATAL ERROR: error in new_fluid_settings()"); return;
    }
}

int fluidsynth_proc::MIDIconnect(int on) {

    if(on) {
        if(synth) mdriver= new_fluid_midi_driver(settings, handle_midi_event, NULL);
        if(!synth || !mdriver) return -1;
    } else
        if(mdriver) {delete_fluid_midi_driver(mdriver); mdriver = NULL;}

return 0;
}

void fluidsynth_proc::MidiClean()
{
    QByteArray a;
    a.clear();
    a.append((char) 0);
    a.append((char) 0);
    SendMIDIEvent(a);
}

fluidsynth_proc::~fluidsynth_proc()
{
    if(mixer) {
        int *p = &iswaiting_signal;
        wait_signal = 2;
        *p = 0;
        audio_waits.wakeOne();
        while(*p != 2) QThread::msleep(5); // waits to sound thread receives the signal
        lock_audio.lock();
        while(!mixer->isFinished()) QThread::msleep(5);
        QThread::msleep(5);
        mixer->terminate();
        lock_audio.unlock();
        mixer->wait(1000);
        delete mixer;
    }

    if(mdriver) delete_fluid_midi_driver(mdriver);
    if(adriver) delete_fluid_audio_driver(adriver);
    if(synth) delete_fluid_synth(synth);
    if(settings) delete_fluid_settings(settings);

    if(out_sound_io) out_sound_io->close();
    if(out_sound) {
        out_sound->stop();
        out_sound->disconnect(this);
        delete out_sound;
    }

    if(sf2_name) delete sf2_name;
    if(fluid_settings) delete fluid_settings;

    qWarning("fluidsynth_proc() destructor");

}

void fluidsynth_proc::set_error(int level, const char * err) {

    if(!err) { // reset
        error_string[0] = 0;
        it_have_error = 0;
        return;
    }

    if(it_have_error == 2) return; // ignore others errors

    error_string[0] = 0;
    strcpy(error_string, err);

    if(level == 666)
        it_have_error = 2;
    else
        it_have_error = 1;
}

int strange = 0;

int fluidsynth_proc::SendMIDIEvent(QByteArray array)
{
    int type= array[0] & 0xf0;
    int channel = array[0] & 0xf;
    int ret = 0;

    if(block_midi_events)
        return 0;
    if(sf2_id == FLUID_FAILED || !synth)
        return -1;

    wakeup_audio();

    switch(type) {

        case 0x90: // note on

            isNoteOn[channel] = 4;
            ret = 0;

            if(strange) array[1] = array[1]/3 * 3 + 3 - (array[1] % 3);

            if(VST_proc::VST_isMIDI(channel) && mixer) VST_proc::VST_MIDIcmd(channel,
                                                            ((time_frame.msecsSinceStartOfDay() - frames) *  _sample_rate) / 1000
                                                            , array);

            ret = fluid_synth_noteon(synth, channel, array[1], array[2]);
            break;

        case 0x80: // note off

            ret = 0;

            if(strange) array[1] = array[1]/3 * 3 + 3 - (array[1] % 3);

            if(VST_proc::VST_isMIDI(channel) && mixer) VST_proc::VST_MIDIcmd(channel,
                                                            ((time_frame.msecsSinceStartOfDay() - frames) *  _sample_rate) /1000
                                                            , array);

            ret = fluid_synth_noteoff(synth, channel, array[1]);
            break;

        case 0xB0: // control

            if((unsigned) array[1] == 11)  //ignore song expresion
                array[2]= synth_chanvolume[channel];

            else if((unsigned) array[1] == 121) {

                fluid_synth_cc(synth, channel, array[1], array[2]);
                array[1] = 11; // set expresion
                array[2] = synth_chanvolume[channel];

            } else if((unsigned) array[1] == 124) // omni off skip
                return 0;

            // Fluid Synth MIDI custom CC

            else if((unsigned) array[1] == 20) { // change volume of channel

                synth_chanvolume[channel] = array[2];
                setSynthChanVolume(channel, synth_chanvolume[channel]);

                if(fluid_control && !fluid_control->disable_mainmenu) {

                    emit changeVolBalanceGain(channel | (1<<4), getSynthChanVolume(channel) * 100 / 127,
                                              getAudioBalance(channel), getAudioGain(channel));
                }

                return 0;

            } else if((unsigned) array[1] == 21) { // change balance of channel

                audio_chanbalance[channel] = array[2] * 200 / 127 - 100;

                if(fluid_control && !fluid_control->disable_mainmenu) {

                    emit changeVolBalanceGain(channel | (2<<4), getSynthChanVolume(channel)* 100 / 127,
                                              getAudioBalance(channel), getAudioGain(channel));
                }

                return 0;

            } else if((unsigned) array[1] == 22) { // change gain  of channel

                audio_changain[channel] = array[2] * 2000 / 127 - 1000;

                if(fluid_control && !fluid_control->disable_mainmenu) {

                    emit changeVolBalanceGain(channel | (3<<4), getSynthChanVolume(channel)* 100 / 127,
                                              getAudioBalance(channel), getAudioGain(channel));
                }

                return 0;

            } else if((unsigned) array[1] == 23) { // distortion gain  of channel

                int gain = (unsigned) array[2] * 300 / 127;

                fluid_output->setAudioDistortionFilter( (gain > 0), channel, gain);

                if(fluid_control && !fluid_control->disable_mainmenu) {

                    emit changeFilterValue(0, channel);
                }

                return 0;

            } else if((unsigned) array[1] == 24) { // Low Cut gain  of channel

                int gain = (unsigned) array[2] * 200 / 127;

                //gain = get_param_filter(PROC_FILTER_LOW_PASS, channel, GET_FILTER_GAIN);
                int freq = get_param_filter(PROC_FILTER_LOW_PASS, channel, GET_FILTER_FREQ);
                int res = get_param_filter(PROC_FILTER_LOW_PASS, channel, GET_FILTER_RES);

                setAudioLowPassFilter((gain > 0), channel, freq, gain, res);

                if(fluid_control && !fluid_control->disable_mainmenu) {

                    emit changeFilterValue(1, channel);
                }

                return 0;

            } else if((unsigned) array[1] == 25) { // Low Cut freq  of channel

                int freq = 50 + ((unsigned) array[2] * 2450 / 127);

                int gain = get_param_filter(PROC_FILTER_LOW_PASS, channel, GET_FILTER_GAIN);
                //int freq = get_param_filter(PROC_FILTER_LOW_PASS, channel, GET_FILTER_FREQ);
                int res = get_param_filter(PROC_FILTER_LOW_PASS, channel, GET_FILTER_RES);

                setAudioLowPassFilter(1, channel, freq, gain, res);

                if(fluid_control && !fluid_control->disable_mainmenu) {

                    emit changeFilterValue(1, channel);
                }

                return 0;

            } else if((unsigned) array[1] == 26) { // High Cut gain  of channel

                int gain = (unsigned) array[2] * 200 / 127;

                //gain = get_param_filter(PROC_FILTER_HIGH_PASS, channel, GET_FILTER_GAIN);
                int freq = get_param_filter(PROC_FILTER_HIGH_PASS, channel, GET_FILTER_FREQ);
                int res = get_param_filter(PROC_FILTER_HIGH_PASS, channel, GET_FILTER_RES);

                setAudioHighPassFilter((gain > 0), channel, freq, gain, res);

                if(fluid_control && !fluid_control->disable_mainmenu) {

                    emit changeFilterValue(2, channel);
                }

                return 0;

            } else if((unsigned) array[1] == 27) { // High Cut freq  of channel

                int freq = 500 + ((unsigned) array[2] )* 4500 / 127;

                int gain = get_param_filter(PROC_FILTER_HIGH_PASS, channel, GET_FILTER_GAIN);
                //int freq = get_param_filter(PROC_FILTER_HIGH_PASS, channel, GET_FILTER_FREQ);
                int res = get_param_filter(PROC_FILTER_HIGH_PASS, channel, GET_FILTER_RES);

                setAudioHighPassFilter(1, channel, freq, gain, res);

                if(fluid_control && !fluid_control->disable_mainmenu) {

                    emit changeFilterValue(2, channel);
                }

                return 0;

            } else if((unsigned) array[1] == 28) { // Low Cut Resonance of channel

                int res = (unsigned) array[2] * 250 / 127;

                int gain = get_param_filter(PROC_FILTER_LOW_PASS, channel, GET_FILTER_GAIN);
                int freq = get_param_filter(PROC_FILTER_LOW_PASS, channel, GET_FILTER_FREQ);
                //int res = get_param_filter(PROC_FILTER_LOW_PASS, channel, GET_FILTER_RES);

                setAudioLowPassFilter(1, channel, freq, gain, res);

                if(fluid_control && !fluid_control->disable_mainmenu) {

                    emit changeFilterValue(1, channel);
                }

                return 0;

            } else if((unsigned) array[1] == 29) { // High Cut Resonance of channel

                int res = (unsigned) array[2] * 250 / 127;

                int gain = get_param_filter(PROC_FILTER_HIGH_PASS, channel, GET_FILTER_GAIN);
                int freq = get_param_filter(PROC_FILTER_HIGH_PASS, channel, GET_FILTER_FREQ);
                //int res = get_param_filter(PROC_FILTER_HIGH_PASS, channel, GET_FILTER_RES);

                setAudioHighPassFilter(1, channel, freq, gain, res);

                if(fluid_control && !fluid_control->disable_mainmenu) {

                    emit changeFilterValue(2, channel);
                }

                return 0;

            } else if((unsigned) array[1] == 30) { // Main Volume

                synth_gain = ((float) ((unsigned) array[2] * 2000 / 127)) / 1000.0;

                fluid_synth_set_gain(synth, (float) synth_gain);

                if(fluid_control && !fluid_control->disable_mainmenu) {

                    emit changeMainVolume(getSynthGain());
                }

                return 0;

            } else if((unsigned) array[1] == 31) { // switch effect events up/down

                MidiInControl::set_events_to_down((unsigned) array[2] >= (unsigned) 64);

                emit MidiIn_set_events_to_down((unsigned) array[2] >= (unsigned) 64);

                return 0;

            }


            return fluid_synth_cc(synth, channel, array[1], array[2]);

        case 0xC0: // program

            ret = fluid_synth_program_change(synth, channel, array[1]);
            break;

        case 0xE0: // pitch bend

            ret = fluid_synth_pitch_bend(synth, channel, (array[1] & 0x7f) | (array[2]<<7));
            break;

        case 0xF0: // systemEX
        {
            char id2[4] = {0x0, 0x66, 0x66, 'P'}; // global mixer

            if(fluid_control) { // anti-crash!
                fluid_control->disable_mainmenu = true;
                fluid_control->deleteLater();
                fluid_control = NULL;
            }

            // new sysEx old compatibility
            if(array[2] == id2[0] && array[3] == id2[1] && array[4] == id2[2] && array[5] == 'P') {

                int entries = 13 * 16 + 1;
                char id[4];
                int BOOL;

                QDataStream qd(&array,
                               QIODevice::ReadOnly);
                qd.startTransaction();

                qd.readRawData((char *) id, 1); // new sysEx old compatibility
                qd.readRawData((char *) id, 1);
                qd.readRawData((char *) id, 4);

                if(id[0] != id2[0] || id[1] != id2[1] || id[2] != id2[2]) {
                    return 0;
                }


                if(id[3] != 'P') return 0;

                if(decode_sys_format(qd, (void *) &entries) < 0) {

                    return 0;
                }

                if(entries != 13 * 16 + 1) {
                    return 0;
                }


                decode_sys_format(qd, (void *) &synth_gain);

                for(int n = 0; n < 16; n++) {

                    audio_chanmute[n]= false;
                    decode_sys_format(qd, (void *) &synth_chanvolume[n]);
                    decode_sys_format(qd, (void *) &audio_changain[n]);
                    decode_sys_format(qd, (void *) &audio_chanbalance[n]);

                    decode_sys_format(qd, (void *) &BOOL);
                    filter_dist_on[n]= (BOOL) ? true : false;
                    decode_sys_format(qd, (void *) &filter_dist_gain[n]);

                    decode_sys_format(qd, (void *) &BOOL);
                    filter_locut_on[n]= (BOOL) ? true : false;
                    decode_sys_format(qd, (void *) &filter_locut_freq[n]);
                    decode_sys_format(qd, (void *) &filter_locut_gain[n]);
                    decode_sys_format(qd, (void *) &filter_locut_res[n]);

                    decode_sys_format(qd, (void *) &BOOL);
                    filter_hicut_on[n]= (BOOL) ? true : false;
                    decode_sys_format(qd, (void *) &filter_hicut_freq[n]);
                    decode_sys_format(qd, (void *) &filter_hicut_gain[n]);
                    decode_sys_format(qd, (void *) &filter_hicut_res[n]);

                    setSynthChanVolume(n, synth_chanvolume[n]);

                    int gain = get_param_filter(PROC_FILTER_DISTORTION, n, GET_FILTER_GAIN);

                    if(get_param_filter(PROC_FILTER_DISTORTION, n, GET_FILTER_ON)!=0)
                        setAudioDistortionFilter(1, n, gain);
                    else
                        setAudioDistortionFilter(0, n, gain);

                    gain = get_param_filter(PROC_FILTER_LOW_PASS, n, GET_FILTER_GAIN);
                    int freq = get_param_filter(PROC_FILTER_LOW_PASS, n, GET_FILTER_FREQ);
                    int res = get_param_filter(PROC_FILTER_LOW_PASS, n, GET_FILTER_RES);

                    if(get_param_filter(PROC_FILTER_LOW_PASS, n, GET_FILTER_ON) != 0)
                        setAudioLowPassFilter(1, n, freq, gain, res);
                    else
                        setAudioLowPassFilter(0, n, freq, gain, res);

                    gain = get_param_filter(PROC_FILTER_HIGH_PASS, n, GET_FILTER_GAIN);
                    freq = get_param_filter(PROC_FILTER_HIGH_PASS, n, GET_FILTER_FREQ);
                    res = get_param_filter(PROC_FILTER_HIGH_PASS, n, GET_FILTER_RES);

                    if(get_param_filter(PROC_FILTER_HIGH_PASS, n, GET_FILTER_ON) != 0)
                        setAudioHighPassFilter(1, n, freq, gain, res);
                    else
                        setAudioHighPassFilter(0, n, freq, gain, res);

                    if(fluid_control && !fluid_control->disable_mainmenu) {

                        emit changeVolBalanceGain(channel, getSynthChanVolume(n) * 100 / 127,
                                                  getAudioBalance(n), getAudioGain(n));
                    }
                }

                fluid_synth_set_gain(synth, (float) synth_gain);

                if(fluid_control && !fluid_control->disable_mainmenu) {

                    emit changeMainVolume(getSynthGain());
                }

            } else
                // new sysEx2 old compatibility
                if((array[2] == id2[0] || array[5] == 'R') && array[3] == id2[1]
                      && array[4] == id2[2] && ((array[5] & 0xf0) == 0x70 || array[5] == 'R')) {

                int entries = 13;
                char id[4];
                int BOOL;

                QDataStream qd(&array,
                               QIODevice::ReadOnly);
                qd.startTransaction();

                // new sysEx old compatibility
                qd.readRawData((char *) id, 1);
                qd.readRawData((char *) id, 1);
                qd.readRawData((char *) id, 4);

                if(id[1] != id2[1] || id[2] != id2[2]) {
                    return 0;
                }

                if(!((id[0] == id2[0] && (id[3] & 0xF0) == 0x70) || id[3] == 'R')) {
                    return 0;
                }

                int flag = 1;
                int n = id[3] & 15;

                if(id[3] == 'R') {
                    flag = 2;
                    n = id[0] & 15;
                }

                if(decode_sys_format(qd, (void *) &entries) < 0) {

                    return 0;
                }

                if((flag == 1 && entries != 13) ||
                    (flag == 2 && entries != 13 + (n == 0))) {
                    return 0;
                }

                if(flag == 1 || (flag == 2 && n == 0))
                    decode_sys_format(qd, (void *) &synth_gain);

                audio_chanmute[n]= false;
                decode_sys_format(qd, (void *) &synth_chanvolume[n]);
                decode_sys_format(qd, (void *) &audio_changain[n]);
                decode_sys_format(qd, (void *) &audio_chanbalance[n]);

                decode_sys_format(qd, (void *) &BOOL);
                filter_dist_on[n]= (BOOL) ? true : false;
                decode_sys_format(qd, (void *) &filter_dist_gain[n]);

                decode_sys_format(qd, (void *) &BOOL);
                filter_locut_on[n]= (BOOL) ? true : false;
                decode_sys_format(qd, (void *) &filter_locut_freq[n]);
                decode_sys_format(qd, (void *) &filter_locut_gain[n]);
                decode_sys_format(qd, (void *) &filter_locut_res[n]);

                decode_sys_format(qd, (void *) &BOOL);
                filter_hicut_on[n]= (BOOL) ? true : false;
                decode_sys_format(qd, (void *) &filter_hicut_freq[n]);
                decode_sys_format(qd, (void *) &filter_hicut_gain[n]);
                decode_sys_format(qd, (void *) &filter_hicut_res[n]);

                setSynthChanVolume(n, synth_chanvolume[n]);

                int gain = get_param_filter(PROC_FILTER_DISTORTION, n, GET_FILTER_GAIN);
                if(get_param_filter(PROC_FILTER_DISTORTION, n, GET_FILTER_ON) != 0)
                    setAudioDistortionFilter(1, n, gain);
                else setAudioDistortionFilter(0, n, gain);

                gain = get_param_filter(PROC_FILTER_LOW_PASS, n, GET_FILTER_GAIN);
                int freq = get_param_filter(PROC_FILTER_LOW_PASS, n, GET_FILTER_FREQ);
                int res = get_param_filter(PROC_FILTER_LOW_PASS, n, GET_FILTER_RES);

                if(get_param_filter(PROC_FILTER_LOW_PASS, n, GET_FILTER_ON) != 0)
                    setAudioLowPassFilter(1, n, freq, gain, res);
                else
                    setAudioLowPassFilter(0, n, freq, gain, res);

                gain = get_param_filter(PROC_FILTER_HIGH_PASS, n, GET_FILTER_GAIN);
                freq = get_param_filter(PROC_FILTER_HIGH_PASS, n, GET_FILTER_FREQ);
                res = get_param_filter(PROC_FILTER_HIGH_PASS, n, GET_FILTER_RES);

                if(get_param_filter(PROC_FILTER_HIGH_PASS, n, GET_FILTER_ON) != 0)
                    setAudioHighPassFilter(1, n, freq, gain, res);
                else
                    setAudioHighPassFilter(0, n, freq, gain, res);

                if(fluid_control && !fluid_control->disable_mainmenu) {

                    emit changeVolBalanceGain(channel, getSynthChanVolume(n)* 100 / 127,
                                              getAudioBalance(n), getAudioGain(n));
                }

                if(fluid_control && !fluid_control->disable_mainmenu) {

                    emit changeMainVolume(getSynthGain());

                }

            } else if(/*array[2] == id2[0] &&*/ array[3] == id2[1]
                 && array[4] == id2[2] && (array[5] == 'V' || array[5] == 'W')) {

                    VST_proc::VST_LoadParameterStream(array);
            }
        //

        }
        return 0;
    }
#if 0
    if(0 && ret < 0) {
        if (MidiPlayer::isPlaying()) {
            emit pause_player(); // error
            static int no_re = 0;
            if(!no_re) {
                no_re = 1;
                emit message_timeout("fluidsynth_proc Error", QString::asprintf("SendMIDIEvent: 0x%x", type));
                no_re = 0;
            }
        }
    }
#endif
    return ret;
}

void fluidsynth_proc::new_sound_thread(int freq) {

    if(mixer) {

        qWarning("error: mixer is defined\n");

        exit(-1);
    }

    disabled = change_synth(freq, 1);

    if(!disabled) {

        wait_signal = 0;

        int *p = &iswaiting_signal;
        *p = 0;
        set_error(0, NULL);

        mixer = new fluid_Thread(this);

        if(!mixer)  {
            status_fluid_err = 4;
            set_error(666, "FATAL ERROR: error creating fluid_Thread()"); return;
        }

        if(out_sound && out_sound_io)
            status_audio_err = 0;

        mixer->start(QThread::TimeCriticalPriority);
        while(*p != 555) QThread::msleep(5);
        *p = 0;
    }

    QThread::msleep(150);

}
void fluidsynth_proc::stop_audio(bool stop) {

    if(stop) { // stops the audio loop

        block_midi_events = 1;

        if(mixer) {

            int *p = &iswaiting_signal;

            wait_signal = 2;   
            *p = 0;
            audio_waits.wakeOne();

            while(*p != 2) QThread::msleep(5); // waits to sound thread receives the signal

            lock_audio.lock();

            while(!mixer->isFinished()) QThread::msleep(5);
            QThread::msleep(5);

            delete mixer;
            mixer = NULL;
            disabled = 0;

        } else {

            lock_audio.lock();

            wait_signal = 0;
            iswaiting_signal = 0;
            disabled = 0;
        }

    } else { // release lock audio

        lock_audio.unlock();
        block_midi_events = 0;
    }
}

void fluidsynth_proc::wakeup_audio() {

    audio_waits.wakeOne();
}

/***************************************************************************/
// fluid_Thread
/***************************************************************************/

fluid_Thread::fluid_Thread(fluidsynth_proc *proc) : QThread(proc){

    _proc = proc;
    fbuf = NULL;
    dry = NULL;
    fx = NULL;
    buffer = NULL;
    mix_buffer = NULL;

    for(int n = 0; n < 16; n++) PROC[n] = NULL;
}

fluid_Thread::~fluid_Thread() {

    if(fbuf && !sharedAudioBuffer) delete[] fbuf;
    if(dry) delete[] dry;
    if(fx) delete[] fx; 
    delete[] buffer;
    delete[] mix_buffer;

    for(int n = 0; n < 16; n++) delete PROC[n];

    qWarning("fluid_Thread() destructor");

}

void fluid_Thread::run()
{

    if(!_proc->synth) {

        MSG_ERR("Error! fluid_Thread: !_proc->synth\n");
        _proc->iswaiting_signal = 2;
        exit(0);
    }

    int n_aud_chan = fluid_synth_count_audio_channels(_proc->synth);
    //MSG_OUT("audio channels: %i\n", n_aud_chan);
    int n_fx_chan = fluid_synth_count_effects_channels(_proc->synth);
    int n_fx_groups = fluid_synth_count_effects_groups(_proc->synth);
    //MSG_OUT("effects channels: %i\n", n_fx_chan);
    //MSG_OUT("effects groups channels: %i\n", n_fx_groups);

    fluid_out_samples = _proc->fluid_out_samples;
    output_float = _proc->output_float;
    MSG_OUT("fluid_Thread: fluid_out_samples: %i\n", fluid_out_samples);

    int szbuf = FLUID_OUT_SAMPLES * 2 * (n_aud_chan + n_fx_chan);

    if(sharedAudioBuffer)
        fbuf = (float *) sharedAudioBuffer->data();
    else
        fbuf = new float[szbuf];

    if(!fbuf) {

        MSG_ERR("Error! fluid_Thread: !fbuf\n");
        _proc->set_error(666, "FATAL ERROR: out of memory creating fbuf"); return;

    } else
        memset((void *) fbuf, 0, sizeof(float) * szbuf);

    // array of buffers
    dry = new float*[n_aud_chan * 2];
    fx = new float*[n_fx_chan * n_fx_groups * 2];

    if(dry) memset(dry, 0, sizeof(float*) * n_aud_chan * 2);
    if(fx) memset(fx, 0, sizeof(float*) * n_fx_chan * n_fx_groups * 2);

    if(!dry || !fx) {
        MSG_ERR("Error! fluid_Thread: !dry || !fx\n");
        _proc->set_error(666, "FATAL ERROR: out of memory creating dry/fx"); return;
    }

     // setup buffers to mix stereo audio per MIDI Channel (16 channels)

     for(int i = 0; i < n_aud_chan * 2; i++) {
         dry[i] = &fbuf[i * fluid_out_samples];
     }

     // Map fx channels to mix with voices

     for(int i = 0, g = 0; i < n_fx_groups  * 2; i+= 2, g+= n_fx_chan * 2){
         for(int h = 0; h < n_fx_chan; h++){
             fx[h * 2 + g]     = dry[i];
             fx[h * 2 + g + 1] = dry[i + 1];
         }

     }

     for(int n = 0; n < 16; n++) {

         PROC[n] = new PROC_filter(100, 2500, _proc->_sample_rate, 0.0, 0.0, 0.0);

         if(!PROC[n]) {

             MSG_ERR("Error! fluid_Thread: !PROC[%i]\n", n);
             _proc->set_error(666, "FATAL ERROR: out of memory creating PROC[n]");

             return;
         }

     }

     recalculate_filters(_proc->_sample_rate);

     buffer= new short[FLUID_OUT_SAMPLES * 2];

     mix_buffer= new float[FLUID_OUT_SAMPLES * 2];

     if(!buffer || !mix_buffer) {
         MSG_ERR("Error! fluid_Thread: !buffer || !mix_buffer\n");
         _proc->set_error(666, "FATAL ERROR: out of memory creating buffer/mix_buffer (out sound)"); return;
     }

     memset(buffer, 0, sizeof(short) * FLUID_OUT_SAMPLES * 2);
     memset(mix_buffer, 0, sizeof(float) * FLUID_OUT_SAMPLES * 2);

     int *sem = &_proc->wait_signal;

     _proc->cleft = _proc->cright = 0;
     _proc->wav_is_float = _proc->fluid_settings->value("Wav to Float" , 0).toInt();
     _proc->iswaiting_signal = 555;

     QMutex mutex;
     mutex.lock();

     int lock_thread = 1;

     MSG_OUT("fluid_Thread: loop");
     _proc->frames = 0;

     while(1) {

         if(_proc->_player_status == 2) {
             _proc->iswaiting_signal = 1;
             //MSG_OUT("waiting signal..\n");
             *sem = 0;
             lock_thread = 1;
         }

         if(lock_thread) {
             //MSG_OUT("waiting...\n");
             _proc->audio_waits.wait(&mutex);
             //MSG_OUT("running free..\n");
             lock_thread = 0;
         }

         QThread::usleep(1);

         // test if out buffer changed
         if((sharedAudioBuffer && sharedAudioBuffer->data() != fbuf) || fluid_out_samples != _proc->fluid_out_samples) {

             fluid_out_samples = _proc->fluid_out_samples;

             if(sharedAudioBuffer && sharedAudioBuffer->data() != fbuf) {
                 free(fbuf);
                 fbuf = (float *) sharedAudioBuffer->data();
             }

             memset((void *) fbuf, 0, sizeof(float) * szbuf);
             memset(buffer, 0, sizeof(short) * FLUID_OUT_SAMPLES * 2);
             memset(mix_buffer, 0, sizeof(float) * FLUID_OUT_SAMPLES * 2);

             // setup buffers to mix stereo audio per MIDI Channel (16 channels)

             for(int i = 0; i < n_aud_chan * 2; i++) {
                 dry[i] = &fbuf[i * fluid_out_samples];
             }

             // Map fx channels to mix with voices

             for(int i = 0, g = 0; i < n_fx_groups  * 2; i+= 2, g+= n_fx_chan * 2){
                 for(int h = 0; h < n_fx_chan; h++){
                     fx[h * 2 + g]     = dry[i];
                     fx[h * 2 + g + 1] = dry[i + 1];
                 }

             }

             MSG_OUT("fluid_Thread: change buffer: %i\n", fluid_out_samples);

         }

         //if(!_float_is_supported) _proc->output_float = 0;

         if(output_float != _proc->output_float) {

             QAudioOutput *out_sound2;
             QIODevice * out_sound_io2;
             QAudioFormat format;

             int outf =_proc->output_float;

             format.setSampleRate(_proc->_sample_rate);
             format.setChannelCount(2);
             format.setCodec("audio/pcm");
             format.setByteOrder(QAudioFormat::LittleEndian);

             if(outf) {
                 format.setSampleSize(32);
                 format.setSampleType(QAudioFormat::Float);
             } else {
                 format.setSampleSize(16);
                 format.setSampleType(QAudioFormat::SignedInt);
             }

             out_sound2 = new QAudioOutput(_proc->current_sound, format);

             if(out_sound2) {

                 out_sound_io2 = out_sound2->start();

                 if(out_sound_io2) {

                     if(_proc->out_sound_io) _proc->out_sound_io->close();

                     if(_proc->out_sound) {
                         _proc->out_sound->stop();
                         _proc->out_sound->disconnect(this);
                         delete _proc->out_sound;
                     }

                     _proc->out_sound = out_sound2;
                     _proc->out_sound_io = out_sound_io2;

                     output_float = outf;

                     _proc->fluid_settings->setValue("Out use float", outf);
                     MSG_OUT("fluid_Thread: audio output:\n    sample_rate: %i use float %i\n", _proc->_sample_rate, output_float);

                 } else {

                     delete out_sound2;
                     _proc->output_float = output_float;
                 }

             } else
                 _proc->output_float = output_float;

         }

        /*
         static int loop = 1;
         MSG_OUT("loop %i\n", loop);
         loop++;
        */

         if(*sem == 2) { // thread exit
             *sem = 0;
             break;
         }

         if(_proc->sf2_id == FLUID_FAILED || _proc->disabled == 2
                 || *sem == 1 || _proc->out_sound_io == NULL || _proc->out_sound == NULL) {

             if(*sem) {
                 _proc->iswaiting_signal = 1;
                 lock_thread = 1;

                 *sem = 0;
                 continue;
             }

             QThread::msleep(20);

             continue; // waits SF2 is loaded
         }

         _proc->lock_audio.lock();

         // reset synth buffer
         memset(fbuf, 0, szbuf*sizeof(float));

         _proc->mutex_fluid.lock();

         int err =-1;

         if(_proc->synth) {
             err = fluid_synth_process(_proc->synth, fluid_out_samples, n_fx_chan * n_fx_groups * 2, fx, n_aud_chan * 2, dry);
         }
         _proc->mutex_fluid.unlock();
         if(err) {
             MSG_ERR("Error! fluid_Thread: fluid_synth_process()");
             _proc->lock_audio.unlock();
             continue;
         }

         // reset audio buffer
         memset(mix_buffer, 0, fluid_out_samples * 2 * sizeof(float));
         int n, m;

         //
         VST_proc::VST_mix(dry, /*n_aud_chan*/PRE_CHAN, (_proc->_player_wav) ? _proc->_wave_sample_rate : _proc->_sample_rate, fluid_out_samples);

         if(sharedAudioBuffer)
            VST_proc::VST_external_mix((_proc->_player_wav) ? _proc->_wave_sample_rate : _proc->_sample_rate, fluid_out_samples);

         _proc->frames = _proc->time_frame.msecsSinceStartOfDay();


         // mix audio channels
         for(m = 0; m < (n_aud_chan * 2); m += 2) {

             if(_proc->audio_chanmute[m >> 1]) //skip channel
                 continue;

             float *leftz = dry[m];
             float *rightz = dry[m + 1];

             PROC_filter *filter = NULL;

             if(PROC[m >> 1] && PROC[m >> 1]->PROC_get_type() != 0)
                 filter = PROC[m >> 1];

             int gain = _proc->audio_changain[m >> 1] / 100;
             float gainl = 1.0;

             if(gain > 0) gainl = 1.0 * ((float) gain);
             if(gain < 0) gainl = 1.0 / ((float) -gain);

             float gainr = gainl;
             float ball = ((float) (100 - _proc->audio_chanbalance[m >> 1])) / 200.0;
             float balr = 1.0f - ball;

             gainl *= ball;
             gainr *= balr;

             for(n = 0; n <  fluid_out_samples; n++) {
                 float left = *(leftz++), right = *(rightz++);
                 float *out = &mix_buffer[(n << 1)];

                 if(filter) filter->PROC_samples(&left, &right);

                 left  *= gainl;
                 right *= gainr;
                 out[0] += left;
                 out[1] += right;
             }
         }

         // convert and clip samples to output buffer
         float *f = mix_buffer;

         float addl = 0.0, addr = 0.0;

         for(n = 0; n < fluid_out_samples * 2; n++) {

             if(*f < -1.0f) *f = -1.0f;
             else if(*f  > 1.0f) *f = 1.0f;

             if(n & 1) {
                 if(*f < 0) addr-= *f; else addr+= *f;
             } else {
                 if(*f < 0) addl-= *f; else addl+= *f;
             }

             buffer[n] = 32767.0f * (*(f++));

         }

         _proc->cleft = 40.0 * log(addl * 200.0 / 512.0);
         _proc->cright = 40.0 * log(addr * 200.0 / 512.0);

         if(_proc->cleft > 200)  _proc->cleft = 200;
         if(_proc->cright > 200)  _proc->cright = 200;

         // write samples to audio device
         int pos = 0;
         int total = ((_proc->_player_wav && _proc->wav_is_float) ||
                     (!_proc->_player_wav && _proc->output_float))
                 ? fluid_out_samples * 8
                 : fluid_out_samples * 4;
         int len;

         char *buff = (_proc->wav_is_float) ?  (char *) mix_buffer : (char *) buffer;

         char *buff2 = (_proc->output_float) ?  (char *) mix_buffer : (char *) buffer;


         qint64 _system_time = QDateTime::currentMSecsSinceEpoch();


         while (total > 0) {

             if(_proc->_player_wav) { // alternative for WAV files

                 if(_proc->_player_status == 2)
                     break;

                 if(_proc->_player_status == 666)

                     len = -1;

                 else {

                     if(_proc->wavDIS) {
                         len = 0;
                         break;
                     } else
                         len = _proc->_player_wav->write(((const char  *) buff + pos), total);
                 }

                 if(len < 0) {

                     _proc->_player_status = 666;

                 } else
                     _proc->total_wav_write += len;

             } else // write audio stream
                 len= _proc->out_sound_io->write(((const char  *) buff2 + pos), total);

             int stat = _proc->out_sound->state();

             if((len < 0 || _proc->out_sound->error() || stat == QAudio::SuspendedState ||
                stat == QAudio::InterruptedState ||
                stat == QAudio::StoppedState) && MidiPlayer::isPlaying()) {

                    emit _proc->pause_player();
                    QThread::usleep(2);
                }

             if(len == 0) {

                 QThread::usleep(2);

                 // timeout at 2 seconds
                 if(((int) (QDateTime::currentMSecsSinceEpoch() - _system_time)) >= 2000) {
                     if (MidiPlayer::isPlaying())
                        emit _proc->pause_player();
                     QThread::usleep(2);
                    // MSG_OUT("time out\n");
                     break;
                 }
             }

             if(len < 0) break;
             pos+= len;
             total-= len;

         }

         _proc->lock_audio.unlock();

     }

     for(int n = 0; n < 100; n++) {
         if(_proc->synth) { // discard synth events
             int err = fluid_synth_process(_proc->synth, fluid_out_samples, n_fx_chan * n_fx_groups * 2, fx, n_aud_chan * 2, dry);
             if(err < 0) break;
             QThread::msleep(2);
         }
     }

     _proc->iswaiting_signal = 2;
     mutex.unlock();

}

void fluid_Thread::recalculate_filters(int freq) {


    for(int chan = 0; chan < 16; chan++) {

        if(!PROC[chan]) continue;

        PROC[chan]->PROC_set_sample_rate(freq);

        if(_proc->filter_dist_on[chan])
            PROC[chan]->PROC_set_type(PROC_FILTER_DISTORTION, _proc->filter_dist_gain[chan], 0);

        if(_proc->filter_locut_on[chan]) {

            PROC[chan]->PROC_Change_filter(_proc->filter_locut_freq[chan], 0,
                                                0, 0, _proc->filter_dist_gain[chan]);
            PROC[chan]->PROC_set_type(PROC_FILTER_LOW_PASS, _proc->filter_locut_gain[chan], 0);
        }

        if(_proc->filter_hicut_on[chan]) {

            PROC[chan]->PROC_Change_filter(0, _proc->filter_hicut_freq[chan],
                                                0, 0, _proc->filter_dist_gain[chan]);
            PROC[chan]->PROC_set_type(PROC_FILTER_HIGH_PASS, 0, _proc->filter_hicut_gain[chan]);
        }
    }

}


// PROC_filter
void PROC_filter::PROC_set_type(int type, float gain, float gain2) {

    if(type & PROC_FILTER_DISTORTION) {
        _type |= PROC_FILTER_DISTORTION;

        PROC_Change_filter(0, 0, 0, 0, gain);
        return;
    }

    if(type & PROC_FILTER_LOW_PASS) {
        _type|= PROC_FILTER_LOW_PASS; _gain = gain + 1.0;
    }

    if(type & PROC_FILTER_HIGH_PASS) {
        _type|= PROC_FILTER_HIGH_PASS; _gain2 = gain2 + 1.0;
    }


}

void PROC_filter::PROC_unset_type(int type) {

    _type &= ~(type);
}
int PROC_filter::PROC_get_type() {
    return _type;
}

void PROC_filter::PROC_Change_filter(float freq, float freq2,
                       float gain, float gain2, float gain_dist) {

    if(_type & PROC_FILTER_DISTORTION) {

        _gain3 = 1.0 + gain_dist / 10.0f;

        for(int n = 0; n <= MAX_DIST_DEF; n++) {
            _distortion_tab[n] = (1.0 - exp(-(_gain3 * (float) n) / MAX_DIST_DEF)) * (1.0 / ((1.0 - exp(-_gain3))));
        }
    }

    if(freq) {
        _gain = gain + 1.0;
    }

    if(freq2) {
        _gain2 = gain2 + 1.0;
    }

    float c, csq, resonance, q;
/*
 Source from:

https://www.musicdsp.org/en/latest/Filters/180-cool-sounding-lowpass-with-decibel-measured-resonance.html

*/
    //PROC_FILTER_LOW_PASS

    if(freq) {

        _type|= PROC_FILTER_LOW_PASS;

        resonance = 2.0 * powf(10.0f, -(_resdB * 0.05f));
        q = sqrt(2.0f) * resonance;

        c = 1.0f / (tanf(M_PI * (freq / _sample_rate)));

        csq = c * c;

        _Icoefs[0] = 1.0f / (1.0f + (q * c) + (csq));
        _Icoefs[1] = 2.0f * _Icoefs[0];
        _Icoefs[2] = _Icoefs[0];
        _Ocoefs[0]= (2.0f * _Icoefs[0]) * (1.0f - csq);
        _Ocoefs[1]= _Icoefs[0] * (1.0f - (q * c) + csq);
    }

   // PROC_FILTER_HIGH_PASS

    if(freq2) {
        _type|= PROC_FILTER_HIGH_PASS;

        resonance = 2.0 * powf(10.0f, -(_resdB2 * 0.05f));

        q = sqrt(2.0f) * resonance;

        c = tanf(M_PI * (freq2 / _sample_rate));

        csq = c * c;

        _I2coefs[0] = 1.0f / (1.0f + (q * c) + (csq));
        _I2coefs[1] = -2.0f * _I2coefs[0];
        _I2coefs[2] = _I2coefs[0];
        _O2coefs[0]= (2.0f * _I2coefs[0]) * (csq - 1.0f );
        _O2coefs[1]= _I2coefs[0] * (1.0f - (q * c) + csq);
    }

}

PROC_filter::PROC_filter(float freq, float freq2, float sample_rate,
                       float gain, float gain2, float gain_dist) {
    _resdB = 0;
    _sample_rate = sample_rate;

    _Isleft[0]=_Isright[0] = 0.0f;
    _Isleft[1]=_Isright[1] = 0.0f;
    _Isleft[2]=_Isright[2] = 0.0f;
    _Osleft[0]=_Osright[0] = 0.0f;
    _Osleft[1]=_Osright[1] = 0.0f;

    _I2sleft[0]=_I2sright[0] = 0.0f;
    _I2sleft[1]=_I2sright[1] = 0.0f;
    _I2sleft[2]=_I2sright[2] = 0.0f;
    _O2sleft[0]=_O2sright[0] = 0.0f;
    _O2sleft[1]=_O2sright[1] = 0.0f;

    _type= PROC_FILTER_DISTORTION;
    PROC_Change_filter(freq, freq2, gain, gain2, gain_dist);
    _type = 0; // only create it
}


void PROC_filter::PROC_samples(float *left, float *right){

    float accul = 0.0f;
    float accur = 0.0f;
    float _left = *left;
    float _right = *right;

    // distorsion
    if(_type & PROC_FILTER_DISTORTION) {

        if(_left < -1.0f)
            _left = -1.0f;
        else if(_left > 1.0f)
            _left = 1.0f;

        if(_right < -1.0f)
            _right = -1.0f;
        else if(_right > 1.0f)
            _right = 1.0f;

        if(_left < 0.0)
            _left = -_distortion_tab[(int) (-_left * 255.0f) & MAX_DIST_DEF] + _left / 255.0;
        else
            _left = _distortion_tab[(int) (_left * 255.0f) & MAX_DIST_DEF] + _left / 255.0;

        if(_right < 0.0)
            _right = -_distortion_tab[(int) (-_right * 255.0f) & MAX_DIST_DEF] + _right / 255.0;
        else
            _right = _distortion_tab[(int) (_right * 255.0f) & MAX_DIST_DEF] + _right / 255.0;

        if(_left < -1.0f)
            _left = -1.0f;
        else if(_left > 1.0f)
            _left = 1.0f;

        if(_right < -1.0f)
            _right = -1.0f;
        else if(_right > 1.0f)
            _right = 1.0f;

    }


    float _left2 = _left;
    float _right2 = _right;

    if(_type & PROC_FILTER_LOW_PASS) {

        _Isleft[2] = _Isleft[1];
        _Isleft[1] = _Isleft[0];
        _Isleft[0] = _left;

        _Isright[2] = _Isright[1];
        _Isright[1] = _Isright[0];
        _Isright[0] = _right;

        accul =  _Isleft[0] * _Icoefs[0];
        accul += _Isleft[1] * _Icoefs[1] - _Osleft[0] * _Ocoefs[0];
        accul += _Isleft[2] * _Icoefs[2] - _Osleft[1] * _Ocoefs[1];

        accur  = _Isright[0] *_Icoefs[0];
        accur += _Isright[1] *_Icoefs[1] - _Osright[0] * _Ocoefs[0];
        accur += _Isright[2] *_Icoefs[2] - _Osright[1] * _Ocoefs[1];

        _Osleft[1] = _Osleft[0];
        _Osleft[0] = accul;

        _Osright[1] = _Osright[0];
        _Osright[0] = accur;

        _left =  accul * _gain;
        _right = accur * _gain ;

    } else {

        _left = _right = 0.0;
    }

    if(_type & PROC_FILTER_HIGH_PASS) {

        _I2sleft[2] = _I2sleft[1];
        _I2sleft[1] = _I2sleft[0];
        _I2sleft[0] = _left2;

        _I2sright[2] = _I2sright[1];
        _I2sright[1] = _I2sright[0];
        _I2sright[0] = _right2;

        accul =  _I2sleft[0] * _I2coefs[0];
        accul += _I2sleft[1] * _I2coefs[1] - _O2sleft[0] * _O2coefs[0];
        accul += _I2sleft[2] * _I2coefs[2] - _O2sleft[1] * _O2coefs[1];

        accur =  _I2sright[0] * _I2coefs[0];
        accur += _I2sright[1] * _I2coefs[1] - _O2sright[0] * _O2coefs[0];
        accur += _I2sright[2] * _I2coefs[2] - _O2sright[1] * _O2coefs[1];

        _O2sleft[1] = _O2sleft[0];
        _O2sleft[0] = accul;

        _O2sright[1] = _O2sright[0];
        _O2sright[0] = accur;

        _left2  = accul * _gain2;
        _right2 = accur * _gain2 ;

    }  else {

        if((_type & 7) != PROC_FILTER_DISTORTION) {
            _left2 =_right2 = 0.0;
        }
    }

    _left  += _left2;
    _right += _right2;

    if((_type & 3) == 3) {

       _left  /= 2.0;
       _right /= 2.0;
    }

    *left  = _left;
    *right = _right;

}

int fluidsynth_proc::getSynthChanVolume(int chan) {

    return synth_chanvolume[chan];
}


void fluidsynth_proc::setSynthChanVolume(int chan, int volume) {

    synth_chanvolume[chan] = volume & 127;
    fluid_synth_cc(synth, chan , 11, volume & 127);

}

void fluidsynth_proc::setAudioGain(int chan, int gain){

    audio_changain[chan & 15] = gain;
}

int fluidsynth_proc::getAudioGain(int chan){

    return audio_changain[chan & 15];
}

void fluidsynth_proc::setAudioBalance(int chan, int balance){

    audio_chanbalance[chan & 15] = balance;
}

int fluidsynth_proc::getAudioBalance(int chan){

    return audio_chanbalance[chan & 15];
}

void fluidsynth_proc::setAudioMute(int chan, bool mute){

    audio_chanmute[chan & 15] = mute;
}

bool fluidsynth_proc::getAudioMute(int chan){

    return audio_chanmute[chan & 15];
}

void fluidsynth_proc::setSynthGain(int gain){
    synth_gain = ((float) gain) / 100.0;
    fluid_synth_set_gain(synth, (float) synth_gain);
}

int fluidsynth_proc::getSynthGain(){
    return (int) (synth_gain * 100.0);
}

void fluidsynth_proc::setAudioDistortionFilter(int on, int chan, int gain) {

    chan &= 15;

    filter_dist_gain[chan] = ((float) gain);

    if(on) {

        filter_dist_on[chan] = true;
        mixer->PROC[chan]->PROC_set_type(PROC_FILTER_DISTORTION, filter_dist_gain[chan], 0);

    } else {

        filter_dist_on[chan] = false;
        mixer->PROC[chan]->PROC_unset_type(PROC_FILTER_DISTORTION);

    }
}

void fluidsynth_proc::setAudioLowPassFilter(int on, int chan, int freq, int gain, int res) {

    chan &= 15;

    filter_locut_freq[chan] = (float) freq;
    filter_locut_gain[chan] = ((float) gain) / 100.0;
    filter_locut_res[chan]  = ((float) res)  / 10.0;

    mixer->PROC[chan]->_resdB = filter_locut_res[chan];

    if(on) {

        filter_locut_on[chan] = true;

        mixer->PROC[chan]->PROC_Change_filter(filter_locut_freq[chan], 0,
                               0, 0, filter_dist_gain[chan]);
        mixer->PROC[chan]->PROC_set_type(PROC_FILTER_LOW_PASS, filter_locut_gain[chan], 0);

    } else {

        filter_locut_on[chan] = false;
        mixer->PROC[chan]->PROC_unset_type(PROC_FILTER_LOW_PASS);
    }
}

void fluidsynth_proc::setAudioHighPassFilter(int on, int chan, int freq, int gain, int res) {

    chan &= 15;

    filter_hicut_freq[chan] = (float) freq;
    filter_hicut_gain[chan] = ((float) gain) / 100.0;
    filter_hicut_res[chan]  = ((float) res)  / 10.0;

    mixer->PROC[chan]->_resdB2 = filter_hicut_res[chan];

    if(on) {

        filter_hicut_on[chan] = true;

        mixer->PROC[chan]->PROC_Change_filter(0, filter_hicut_freq[chan],
                               0, 0, filter_dist_gain[chan]);

        mixer->PROC[chan]->PROC_set_type(PROC_FILTER_HIGH_PASS, 0, filter_hicut_gain[chan]);

    } else {

        filter_hicut_on[chan] = false;
        mixer->PROC[chan]->PROC_unset_type(PROC_FILTER_HIGH_PASS);
    }
}

int fluidsynth_proc::get_param_filter(int type, int chan, int index) {

    chan &= 15;

    if(type & PROC_FILTER_DISTORTION) {

        switch(index) {

        case GET_FILTER_ON:

            if(filter_dist_on[chan])
                return 1;
            else
                return 0;

        case GET_FILTER_GAIN:

            return (int) (filter_dist_gain[chan]);
        }

        return 0;
    }

    if(!(type & PROC_FILTER_HIGH_PASS)) {

        switch(index) {

        case GET_FILTER_ON:

            if(filter_locut_on[chan])
                return 1;
            else
                return 0;

        case GET_FILTER_FREQ:

            return (int) filter_locut_freq[chan];

        case GET_FILTER_GAIN:

            return (int) (filter_locut_gain[chan] * 100.0);

        case GET_FILTER_RES:

            return (int) (filter_locut_res[chan] * 10.0);

        default:

            return 0;
        }

    } else {

        switch(index) {

        case GET_FILTER_ON:

            if(filter_hicut_on[chan])
                return 1;
            else
                return 0;

        case GET_FILTER_FREQ:

            return (int) filter_hicut_freq[chan];

        case GET_FILTER_GAIN:

            return (int) (filter_hicut_gain[chan] * 100.0);

        case GET_FILTER_RES:

            return (int) (filter_hicut_res[chan] * 10.0);

        default:

            return 0;
        }


    }

    return 0;
}

int fluidsynth_proc::MIDtoWAV(QFile *wav, QWidget *parent, MidiFile* file) {

    if(_player_status)
        return -1;

    fluid_output->wavDIS = true;

    _file = file;

    _file->setPauseTick(0);
    _file->setCursorTick(0);
    _file->preparePlayerData(0);

    if(_bar) delete _bar;

    for(int n = 0; n < PRE_CHAN; n++) {

        VST_proc::VST_show(n, false);
    }

    _bar = new ProgressDialog(parent, QString("Exporting to WAVE File..."));
    _bar->setModal(true);

    stop_audio(true);
    _player_wav = wav;
    _player_status = 0;

    fluid_Thread_playerWAV *player = new fluid_Thread_playerWAV(this);

    if(!player) {
        _player_wav = NULL;
        wav->close();
        new_sound_thread(_sample_rate);
        stop_audio(false);
        fluid_output->MidiClean();
        return -1;
    }

    new_sound_thread(_wave_sample_rate);

    stop_audio(false);

    if(fluid_output->status_fluid_err) {

        emit message_timeout("Error!", "FluidSynth building error");

        fluid_output->wavDIS = true;

        stop_audio(true);
        _player_wav = NULL;
        new_sound_thread(_sample_rate);
        stop_audio(false);
        fluid_output->MidiClean();

        wav->close();
        return 0;
    }

    for(int n = 0; n < PRE_CHAN; n++) {

        VST_proc::VST_DisableButtons(n, true);

        QByteArray vst_sel;

        vst_sel.append((char) 0xF0);
        vst_sel.append((char) 0x6);
        vst_sel.append((char) n);
        vst_sel.append((char) 0x66);
        vst_sel.append((char) 0x66);
        vst_sel.append((char) 'W');
        vst_sel.append((char) 0x0);
        vst_sel.append((char) 0xF7);

        SendMIDIEvent(vst_sel);

    }

    player->start(QThread::NormalPriority);

    MSG_OUT("Converting MID to WAV file:\n");

    _bar->exec();

    delete _bar;
    _bar = NULL;

    int *p = &_player_status;

    if(*p == 1) { // progress is canceled

        *p = 666;
        //QMessageBox::critical(NULL, "Error!", "Aborted!");
        emit message_timeout("Error!", "Aborted!");
    }

    while(*p != 0) {

        QThread::msleep(50); // waits WAV thread ends
    }

    if(player) {

        player->terminate();
        player->wait(1000);
    }

    fluid_output->wavDIS = true;

    stop_audio(true);
    _player_wav = NULL;
    new_sound_thread(_sample_rate);
    stop_audio(false);

    fluid_output->MidiClean();

    for(int n = 0; n < PRE_CHAN; n++) {

        VST_proc::VST_MIDInotesOff(n);

        VST_proc::VST_DisableButtons(n, false);
    }

    VST_proc::VST_MIDIend();

    return 0;
}

ProgressDialog::ProgressDialog(QWidget *parent, QString text)
    : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{

    PBar= this;

    if (PBar->objectName().isEmpty())
        PBar->setObjectName(QString::fromUtf8("ProgressBar"));

    PBar->resize(446, 173);
    PBar->setWindowTitle(QCoreApplication::translate("ProgressBar", "Fluid Synth Proc", nullptr));

    progressBar = new QProgressBar(PBar);
    progressBar->setObjectName(QString::fromUtf8("progressBar"));
    progressBar->setGeometry(QRect(100, 100, 251, 23));
    progressBar->setValue(0);

    label = new QLabel(PBar);
    label->setObjectName(QString::fromUtf8("label"));
    label->setGeometry(QRect(100, 50, 221, 21));
    label->setAlignment(Qt::AlignCenter);
    label->setText(text);

    QMetaObject::connectSlotsByName(PBar);
}

void ProgressDialog::setBar(int num) {

   progressBar->setValue(num);
}

void ProgressDialog::reject() {

    hide();
}


/***************************************************************************/
// fluid_Thread_playerWAV
/***************************************************************************/


static QSemaphore *semaf;

/* sequencer callback */
static void seq_callback(unsigned int /*time*/, fluid_event_t* /*event*/, fluid_sequencer_t* /*seq*/, void* /*data*/) {

    semaf->release(1);
}

#define sequence_command(x) {fluid_event_t *evt = new_fluid_event();\
    fluid_event_set_source(evt, -1);\
    fluid_event_set_dest(evt, synthSeqID);\
    x ;\
    fluid_res = fluid_sequencer_send_at(sequencer, evt, msOfTick(event_ticks), 1);\
    if(fluid_res < 0) MSG_ERR("Error in sequence_command ev %d err: %i\n", event_ticks, fluid_res);\
    delete_fluid_event(evt);}

#define sequence_callback(x) {unsigned int cevent_ticks = msOfTick(x);\
    fluid_event_t *evt = new_fluid_event();\
    fluid_event_set_source(evt, -1);\
    fluid_event_set_dest(evt, mySeqID);\
    fluid_event_timer(evt, NULL);\
    fluid_res = fluid_sequencer_send_at(sequencer, evt, cevent_ticks, 1);\
    if(fluid_res < 0) MSG_ERR("Error in sequence_callback ev %d err: %i\n", cevent_ticks, fluid_res);\
    delete_fluid_event(evt);}

fluid_Thread_playerWAV::fluid_Thread_playerWAV(fluidsynth_proc *proc) {

    _proc = proc;
    sequencer_tick_pos = 0;
    sequencer_tick_end = 0;

}

int fluid_Thread_playerWAV::msOfTick(int tick)
{
    if(_proc->_file->ticksPerQuarter() == 192) return tick;

    QList<MidiEvent*> * events = NULL;
    int msOfFirstEventInList = 0;

    if (!events) {
        events = new QList<MidiEvent*>(_proc->_file->channel(17)->eventMap()->values());

        if (!events) {
            return 0;
        }
    }

    // timeMs holds the time of the current tick
    double timeMs = 0;
    int count = 0;

    // event is the previous TempoChangeEvent in the list, ev the current
    TempoChangeEvent* event = 0;
    for (int i = 0; i < events->length(); i++) {
        TempoChangeEvent* ev = dynamic_cast<TempoChangeEvent*>(events->at(i));
        if (!ev) {
            continue;
        }

        count++;

        if (!event || ev->midiTime() <= tick) {
            if (!event) {
                // first event in the list at time msOfFirstEventInList
                timeMs = msOfFirstEventInList;
            } else {
                // any event before the endTick
                timeMs += event->msPerTick() * (ev->midiTime() - event->midiTime());

            }

            event = ev;

        } else {

            // end: ev is later than the endTick

            break;
        }
    }

    if (!event) {
        delete events;
        return 0;
    }

    timeMs += event->msPerTick() * (tick - event->midiTime());
    delete events;
    return (int) timeMs;
}

int fluid_Thread_playerWAV::init_sequencer_player(){

    sequencer = new_fluid_sequencer2(0);
    if(!sequencer) {
        MSG_OUT("fail sequencer new\n");
        exit(-1);
    }

    if(!_proc->synth) {
        MSG_OUT("no synth!\n");
        exit(-1);
    }

    // register synth as first destination
    synthSeqID = fluid_sequencer_register_fluidsynth(sequencer, _proc->synth);

    // register myself as second destination
    mySeqID = fluid_sequencer_register_client(sequencer, "MidiEditorWAV", seq_callback, NULL);

    semaf = new QSemaphore(1);
    if(!semaf) {
        MSG_OUT("new semaf fail\n");
        exit(-1);
    }

    fluid_sequencer_set_time_scale(sequencer, 1000);

    for(int n = 0; n < 16; n++) {
        int fluid_res;
        int event_ticks = 0;
        sequence_command(fluid_event_control_change(evt, n, 123, 127))
    }

    events2 = _proc->_file->playerData();
    sequencer_tick_end = 0;

    QMultiMap<int, MidiEvent*>::iterator it = events2->lowerBound(0);

    int n_events = 0;

    while (it != events2->end()) {
        int sendPosition = it.key();
        if(sendPosition > sequencer_tick_end) sequencer_tick_end = sendPosition;
        it++; n_events++;
    }

    MSG_OUT("init_sequencer_player()\n    n_events: %i\n    last tick: %i\n", n_events, sequencer_tick_end);

    return 0;
}

#define SEQ_FRAME 50

int fluid_Thread_playerWAV::sequencer_player(){

    int last_time = sequencer_tick_pos+SEQ_FRAME;

    semaf->acquire(1);

    QMultiMap<int, MidiEvent*>::iterator it = events2->lowerBound(sequencer_tick_pos);

    while (it != events2->end() && it.key() < (sequencer_tick_pos + SEQ_FRAME)) {

        // save events for the given tick
        QList<MidiEvent *> onEv, offEv;
        int sendPosition = it.key();

        do {
            if(it.value()->midiTime() > last_time)
                last_time = it.value()->midiTime();

            if (it.value()->isOnEvent()) {
                onEv.append(it.value());
            } else {
                offEv.append(it.value());
            }

            it++;
        } while (it != events2->end() && it.key() == sendPosition);

        foreach (MidiEvent* event, offEv) {
            sendCommand(event);
        }

        foreach (MidiEvent* event, onEv) {

            sendCommand(event);
        }

    }


    int fluid_res;

    sequence_callback(sequencer_tick_pos + SEQ_FRAME);
    if(fluid_res < 0) return fluid_res;

    sequencer_tick_pos += SEQ_FRAME;

    if(it == events2->end()) {
        MSG_OUT("sequencer_player() ends\n    tick: %i", sequencer_tick_pos);

        sequencer_tick_end = last_time;

        semaf->acquire(1);

        for(int n = 0; n < 16; n++) {

            int event_ticks= sequencer_tick_end + 1;
            sequence_command(fluid_event_control_change(evt, n, 123, 127))
            if(fluid_res < 0) return fluid_res;
        }

        sequence_callback(sequencer_tick_end + 1);

        if(fluid_res < 0) return fluid_res;

        semaf->acquire(1);
        return 1;
    }
    return 0;
}

void fluid_Thread_playerWAV::run()
{
    _proc->_player_status = 1;
    _proc->total_wav_write= 0;

    fluid_output->wavDIS = false;

    // write fake header
    write_header(_proc->_player_wav, 0, 0, 0, 0);

    init_sequencer_player();

    connect(this, SIGNAL(setBar(int)), _proc->_bar, SLOT(setBar(int)));
    connect(this, SIGNAL(endBar()), _proc->_bar, SLOT(hide()));

    while(1) {

        if(_proc->_player_status == 1) { // playing

            int ret = sequencer_player();

            if(ret == 1) {
                _proc->_player_status = 2; break; // end of file
            }
            if(ret < 0) {
                _proc->_player_status = 666; break; // error!
            }
        }

        if(_proc->_player_status == 666) break; // external error

        if(_proc->_bar) {
            emit setBar(100 * sequencer_tick_pos / sequencer_tick_end);
        }

        QThread::usleep(1);
    }

    if(_proc->_player_status == 2) { // ok
        int *p = &_proc->iswaiting_signal;
        while(*p != 1) QThread::msleep(5);
        _proc->_player_wav->seek(0);
        fluid_output->wavDIS = true;
        QFile *wav = _proc->_player_wav;
        //_proc->_player_wav = NULL;
        write_header(wav, _proc->total_wav_write, _proc->_wave_sample_rate, (_proc->wav_is_float) ? 32 : 16, 2);
        wav->close();

        _proc->_player_status = 0;

        if( _proc->_bar) {
            emit endBar();
        }

    } else {
        fluid_output->wavDIS = true;
        QFile *wav = _proc->_player_wav;
        //_proc->_player_wav = NULL;
        wav->remove();
        _proc->_player_status = 0;

        if( _proc->_bar) {
            emit endBar();
        }
    }

    fluid_output->wavDIS = true;

    QThread::msleep(1000);

    for(int n = 0; n < 16; n++){
        fluid_synth_all_sounds_off(_proc->synth, n);
        fluid_synth_all_notes_off(_proc->synth, n);
    }

    delete_fluid_sequencer(sequencer);
    delete semaf;

}

#define u32 unsigned int

int fluid_Thread_playerWAV::sendCommand(MidiEvent*event) {

    QByteArray data = event->save();

    int type = data[0] & 0xf0;
    int channel = data[0] & 0xf;
    int fluid_res;
    int event_ticks = event->midiTime();

    _proc->wakeup_audio();

    if(type == 0xB0) {  // control

        if((unsigned) data[1] == 11)  //ignore song expresion

            data[2] = _proc->synth_chanvolume[channel];

        else if((unsigned) data[1] == 121) {

            sequence_command(fluid_event_control_change(evt, (u32) channel, (u32) data[1], (u32) data[2]))
            if(fluid_res < 0) return fluid_res;

            data[1] = 11; // set expresion
            data[2]= _proc->synth_chanvolume[channel];
        } else if((unsigned) data[1] == 124) {// ignore Omni off
            return 0;

            // Fluid Synth MIDI custom CC

        } else if((unsigned) data[1] == 20) { // change volume of channel

                fluid_output->synth_chanvolume[channel] = data[2];
                fluid_output->setSynthChanVolume(channel, fluid_output->synth_chanvolume[channel]);

                if(fluid_control && !fluid_control->disable_mainmenu) {

                    emit fluid_output->changeVolBalanceGain(channel | (1<<4), fluid_output->getSynthChanVolume(channel) * 100 / 127,
                                              fluid_output->getAudioBalance(channel), fluid_output->getAudioGain(channel));
                }

                return 0;

            } else if((unsigned) data[1] == 21) { // change balance of channel

                fluid_output->audio_chanbalance[channel] = data[2] * 200 / 127 - 100;

                if(fluid_control && !fluid_control->disable_mainmenu) {

                    emit fluid_output->changeVolBalanceGain(channel | (2<<4), fluid_output->getSynthChanVolume(channel)* 100 / 127,
                                              fluid_output->getAudioBalance(channel), fluid_output->getAudioGain(channel));
                }

                return 0;

            } else if((unsigned) data[1] == 22) { // change gain  of channel

                fluid_output->audio_changain[channel] = data[2] * 2000 / 127 - 1000;

                if(fluid_control && !fluid_control->disable_mainmenu) {

                    emit fluid_output->changeVolBalanceGain(channel | (3<<4), fluid_output->getSynthChanVolume(channel) * 100 / 127,
                                             fluid_output->getAudioBalance(channel), fluid_output->getAudioGain(channel));
                }

                return 0;

            } else if((unsigned) data[1] == 23) { // distortion gain  of channel

                int gain = (unsigned) data[2] * 300 / 127;

                fluid_output->setAudioDistortionFilter( (gain > 0), channel, gain);

                if(fluid_control && !fluid_control->disable_mainmenu) {

                    emit fluid_output->changeFilterValue(0, channel);
                }

                return 0;

            } else if((unsigned) data[1] == 24) { // Low Cut gain  of channel

                int gain = (unsigned) data[2] * 200 / 127;

                //gain = get_param_filter(PROC_FILTER_LOW_PASS, channel, GET_FILTER_GAIN);
                int freq = fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel, GET_FILTER_FREQ);
                int res = fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel, GET_FILTER_RES);

                fluid_output->setAudioLowPassFilter((gain > 0), channel, freq, gain, res);

                if(fluid_control && !fluid_control->disable_mainmenu) {

                    emit fluid_output->changeFilterValue(1, channel);
                }

                return 0;

            } else if((unsigned) data[1] == 25) { // Low Cut freq  of channel

                int freq = 50 + ((unsigned) data[2] * 2450 / 127);

                int gain = fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel, GET_FILTER_GAIN);
                //int freq = fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel, GET_FILTER_FREQ);
                int res = fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel, GET_FILTER_RES);

                fluid_output->setAudioLowPassFilter(1, channel, freq, gain, res);

                if(fluid_control && !fluid_control->disable_mainmenu) {

                    emit fluid_output->changeFilterValue(1, channel);
                }

                return 0;

            } else if((unsigned) data[1] == 26) { // High Cut gain  of channel

                int gain = (unsigned) data[2] * 200 / 127;

                //gain = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel, GET_FILTER_GAIN);
                int freq = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel, GET_FILTER_FREQ);
                int res = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel, GET_FILTER_RES);

                fluid_output->setAudioHighPassFilter((gain > 0), channel, freq, gain, res);

                if(fluid_control && !fluid_control->disable_mainmenu) {

                    emit fluid_output->changeFilterValue(2, channel);
                }

                return 0;

        } else if((unsigned) data[1] == 27) { // High Cut freq  of channel

            int freq = 500 + ((unsigned) data[2] )* 4500 / 127;

            int gain = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel, GET_FILTER_GAIN);
            //int freq = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel, GET_FILTER_FREQ);
            int res = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel, GET_FILTER_RES);

            fluid_output->setAudioHighPassFilter(1, channel, freq, gain, res);

            if(fluid_control && !fluid_control->disable_mainmenu) {

                emit fluid_output->changeFilterValue(2, channel);
            }

            return 0;

        } else if((unsigned) data[1] == 28) { // Low Cut Resonance of channel

            int res = (unsigned) data[2] * 250 / 127;

            int gain = fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel, GET_FILTER_GAIN);
            int freq = fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel, GET_FILTER_FREQ);
            //int res = fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel, GET_FILTER_RES);

            fluid_output->setAudioLowPassFilter(1, channel, freq, gain, res);

            if(fluid_control && !fluid_control->disable_mainmenu) {

                emit fluid_output->changeFilterValue(1, channel);
            }

            return 0;

        } else if((unsigned) data[1] == 29) { // High Cut Resonance of channel

            int res = (unsigned) data[2] * 250 / 127;

            int gain = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel, GET_FILTER_GAIN);
            int freq = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel, GET_FILTER_FREQ);
            //int res = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel, GET_FILTER_RES);

            fluid_output->setAudioHighPassFilter(1, channel, freq, gain, res);

            if(fluid_control && !fluid_control->disable_mainmenu) {

                emit fluid_output->changeFilterValue(2, channel);
            }

            return 0;

        } else if((unsigned) data[1] == 30) { // Main Volume

            fluid_output->synth_gain = ((float) ((unsigned) data[2] * 2000 / 127)) / 1000.0;

            fluid_synth_set_gain(fluid_output->synth, (float) fluid_output->synth_gain);

            if(fluid_control && !fluid_control->disable_mainmenu) {

                emit fluid_output->changeMainVolume(fluid_output->getSynthGain());
            }

            return 0;

        } else if((unsigned) data[1] == 31) { // switch effect events up/down

            MidiInControl::set_events_to_down((unsigned) data[2] >= (unsigned) 64);
            emit fluid_output->MidiIn_set_events_to_down((unsigned) data[2] >= (unsigned) 64);

            return 0;

        }

        sequence_command(fluid_event_control_change(evt, (u32) channel, (u32) data[1], (unsigned int) data[2]))
                if(fluid_res < 0) return fluid_res;

    } else if(type == 0x90) { // note on

        if(VST_proc::VST_isMIDI(channel))
            VST_proc::VST_MIDIcmd(channel,((msOfTick(event_ticks) % 1000 * (_proc->fluid_out_samples / _proc->_wave_sample_rate)) * _proc->_wave_sample_rate) / 1000, data);

        sequence_command(fluid_event_noteon(evt, (u32) channel, (u32) data[1], (u32) data[2]))
        if(fluid_res < 0) return fluid_res;

    } else if(type == 0x80) { // note off

        if(VST_proc::VST_isMIDI(channel))
            VST_proc::VST_MIDIcmd(channel,((msOfTick(event_ticks) % 1000 * (_proc->fluid_out_samples / _proc->_wave_sample_rate)) * _proc->_wave_sample_rate) / 1000 , data);

        sequence_command(fluid_event_noteoff(evt, (u32) channel, (u32) data[1]))
        if(fluid_res < 0) return fluid_res;

    } else if(type == 0xC0) { // program

        sequence_command(fluid_event_program_change(evt, (u32) channel, (u32)  data[1]))
        if(fluid_res < 0) return fluid_res;

    } else if(type==0xE0) { // pitch bend
        sequence_command(fluid_event_pitch_bend(evt, channel, (data[1] & 0x7f) | (data[2]<<7)))
        if(fluid_res < 0) return fluid_res;
    }
    // system EX for fluid mixer
    if(data[0] == (char) 0xF0) // systemEX
    {
        char id2[4]= {0x0, 0x66, 0x66, 'P'}; // global mixer

        if(fluid_control) { // anti-crash!
            fluid_control->disable_mainmenu = true;
            fluid_control->deleteLater();
            fluid_control = NULL;
        }

        // new sysEx old compatibility
        if(data[2] == id2[0] && data[3] == id2[1] && data[4] == id2[2] && data[5] == 'P') {

            int fluid_res;
            sequence_callback(event_ticks); // I needs to send previous commands
            if(fluid_res < 0) return fluid_res;

            semaf->acquire(1);

            int entries = 13 * 16 + 1;
            char id[4];
            int BOOL;

            QDataStream qd(&data,
                           QIODevice::ReadOnly);
            qd.startTransaction();

            // new sysEx old compatibility
            qd.readRawData((char *) id, 1);
            qd.readRawData((char *) id, 1);
            qd.readRawData((char *) id, 4);


            if(id[0] != id2[0] || id[1] != id2[1] || id[2] != id2[2]) {

                goto skip;
            }

            if(id[3] != 'P')

                goto skip;

            if(decode_sys_format(qd, (void *) &entries)<0) {

                goto skip;
            }


            if(entries != 13 * 16 + 1){

                goto skip;
            }


            decode_sys_format(qd, (void *) &fluid_output->synth_gain);

            for(int n = 0; n < 16; n++) {

                fluid_output->audio_chanmute[n] = false;
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

                fluid_output->setSynthChanVolume(n, fluid_output->synth_chanvolume[n]);

                int gain = fluid_output->get_param_filter(PROC_FILTER_DISTORTION, n, GET_FILTER_GAIN);
                if(fluid_output->get_param_filter(PROC_FILTER_DISTORTION, n, GET_FILTER_ON) != 0)
                    fluid_output->setAudioDistortionFilter(1, n, gain);
                else fluid_output->setAudioDistortionFilter(0, n, gain);

                gain = fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, n, GET_FILTER_GAIN);
                int freq =fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, n, GET_FILTER_FREQ);
                int res =fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, n, GET_FILTER_RES);

                if(fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, n, GET_FILTER_ON) != 0)
                    fluid_output->setAudioLowPassFilter(1, n, freq, gain, res);
                else fluid_output->setAudioLowPassFilter(0, n, freq, gain, res);

                gain = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, n, GET_FILTER_GAIN);
                freq =fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, n, GET_FILTER_FREQ);
                res =fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, n, GET_FILTER_RES);

                if(fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, n, GET_FILTER_ON) != 0)
                    fluid_output->setAudioHighPassFilter(1, n, freq, gain, res);
                else
                    fluid_output->setAudioHighPassFilter(0, n, freq, gain, res);

                if(fluid_control && !fluid_control->disable_mainmenu) {

                    emit changeVolBalanceGain(channel, fluid_output->getSynthChanVolume(n) * 100 / 127,
                                              fluid_output->getAudioBalance(n), fluid_output->getAudioGain(n));
                }
            }

            fluid_synth_set_gain(fluid_output->synth, (float) fluid_output->synth_gain);

            if(fluid_control && !fluid_control->disable_mainmenu) {
                emit changeMainVolume(fluid_output->getSynthGain());
            }

        } else
            // new sysEx2 old compatibility
            if((data[2] == id2[0] || data[5] == 'R') && data[3] == id2[1]
                      && data[4] == id2[2] && ((data[5] & 0xf0) == 0x70 || data[5] == 'R')) {

            int fluid_res;
            sequence_callback(event_ticks); // I needs to send previous commands
            if(fluid_res < 0) return fluid_res;
            semaf->acquire(1);

            int entries = 13;
            char id[4];
            int BOOL;

            QDataStream qd(&data,
                           QIODevice::ReadOnly);
            qd.startTransaction();

            // new sysEx old compatibility
            qd.readRawData((char *) id, 1);
            qd.readRawData((char *) id, 1);
            qd.readRawData((char *) id, 4);

            if(id[1] != id2[1] || id[2] != id2[2]) {
                 goto skip;
            }

            if(!((id[0] == id2[0] && (id[3] & 0xF0) == 0x70) || id[3] == 'R')) {
                 goto skip;
            }

            int flag = 1;
            int n = id[3] & 15;

            if(id[3] == 'R') {
                flag = 2;
                n = id[0] & 15;
            }


            if(decode_sys_format(qd, (void *) &entries)<0) {

                goto skip;
            }

            if((flag == 1 && entries != 13) ||
                (flag == 2 && entries != 13 + (n == 0))) {
                goto skip;
            }

            if((flag == 1) || (flag == 2 && n == 0)) {
                decode_sys_format(qd, (void *) &fluid_output->synth_gain);
            }

                fluid_output->audio_chanmute[n]= false;
                decode_sys_format(qd, (void *) &fluid_output->synth_chanvolume[n]);
                decode_sys_format(qd, (void *) &fluid_output->audio_changain[n]);
                decode_sys_format(qd, (void *) &fluid_output->audio_chanbalance[n]);

                decode_sys_format(qd, (void *) &BOOL);
                fluid_output->filter_dist_on[n] = (BOOL) ? true : false;
                decode_sys_format(qd, (void *) &fluid_output->filter_dist_gain[n]);

                decode_sys_format(qd, (void *) &BOOL);
                fluid_output->filter_locut_on[n] = (BOOL) ? true : false;
                decode_sys_format(qd, (void *) &fluid_output->filter_locut_freq[n]);
                decode_sys_format(qd, (void *) &fluid_output->filter_locut_gain[n]);
                decode_sys_format(qd, (void *) &fluid_output->filter_locut_res[n]);

                decode_sys_format(qd, (void *) &BOOL);
                fluid_output->filter_hicut_on[n] = (BOOL) ? true : false;
                decode_sys_format(qd, (void *) &fluid_output->filter_hicut_freq[n]);
                decode_sys_format(qd, (void *) &fluid_output->filter_hicut_gain[n]);
                decode_sys_format(qd, (void *) &fluid_output->filter_hicut_res[n]);

                fluid_output->setSynthChanVolume(n, fluid_output->synth_chanvolume[n]);

                int gain = fluid_output->get_param_filter(PROC_FILTER_DISTORTION, n, GET_FILTER_GAIN);

                if(fluid_output->get_param_filter(PROC_FILTER_DISTORTION, n, GET_FILTER_ON) != 0)
                    fluid_output->setAudioDistortionFilter(1, n, gain);
                else
                    fluid_output->setAudioDistortionFilter(0, n, gain);

                gain = fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, n, GET_FILTER_GAIN);
                int freq =fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, n, GET_FILTER_FREQ);
                int res =fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, n, GET_FILTER_RES);

                if(fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, n, GET_FILTER_ON)!=0)
                    fluid_output->setAudioLowPassFilter(1, n, freq, gain, res);
                else fluid_output->setAudioLowPassFilter(0, n, freq, gain, res);

                gain = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, n, GET_FILTER_GAIN);
                freq = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, n, GET_FILTER_FREQ);
                res =  fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, n, GET_FILTER_RES);

                if(fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, n, GET_FILTER_ON) != 0)
                    fluid_output->setAudioHighPassFilter(1, n, freq, gain, res);
                else fluid_output->setAudioHighPassFilter(0, n, freq, gain, res);

                if(fluid_control && !fluid_control->disable_mainmenu) {

                    emit changeVolBalanceGain(channel, fluid_output->getSynthChanVolume(n) * 100 / 127,
                                              fluid_output->getAudioBalance(n), fluid_output->getAudioGain(n));

                }

            if(fluid_control && !fluid_control->disable_mainmenu) {

                emit changeMainVolume(fluid_output->getSynthGain());
            }

        }  else if(/*data[2] == id2[0] &&*/ data[3] == id2[1]
                   && data[4] == id2[2] && (data[5] == 'V' || data[5] == 'W')) {

                    if(data[5] == 'W') {

                        int fluid_res;

                        sequence_callback(event_ticks); // I needs to send previous commands
                        if(fluid_res < 0) return fluid_res;

                        semaf->acquire(1);
                    }

                    VST_proc::VST_LoadParameterStream(data);

              }
    skip: ;
    }

    return 0;
}

void fluid_Thread_playerWAV::write_header(QFile *f, int size, int sample_rate, int bits, int channels) {

    char riff[]="RIFF";
    char wave[]="WAVE";
    char fmt[]="fmt ";
    char d[]="data";
    int dat;
    short dat2;

    if(bits == 0)
        riff[0] = riff[1] = riff[2] = riff[3] = 0;

    f->write((const char  *) riff, 4);

    dat = size + 44;

    f->write((const char  *) &dat, 4);
    f->write((const char  *) wave, 4);
    f->write((const char  *) fmt, 4);

    dat = 16;
    f->write((const char  *) &dat, 4);

    dat2 = (bits == 32) ? 3 : 1; // tipo
    f->write((const char  *) &dat2, 2);

    dat2 = channels; // channels
    f->write((const char  *) &dat2, 2);

    f->write((const char  *) &sample_rate, 4);

    dat = sample_rate * bits / 8 * channels; // bits per sec
    f->write((const char  *) &dat, 4);

    dat2= channels * bits / 8; // align
    f->write((const char  *) &dat2, 2);

    f->write((const char  *) &bits, 2); // bits per sample
    f->write((const char  *) d, 4);
    f->write((const char  *) &size, 4);

}

#endif
