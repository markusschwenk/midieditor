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
#include "../MidiEvent/SysExEvent.h"
#include "../MidiEvent/OffEvent.h"
#include "../MidiEvent/ProgChangeEvent.h"
#include "../MidiEvent/ControlChangeEvent.h"
#include "../midi/MidiOutput.h"
#include "../midi/MidiTrack.h"
#include "../midi/MidiPlayer.h"

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


QObject* fluidsynth_proc::first = nullptr;

#define MSG_ERR qWarning
#define MSG_OUT qDebug

int decode_sys_format(QDataStream &qd, void *data) {

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

void encode_sys_format(QDataStream &qd, void *data) {

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

MidiFile* fluidsynth_proc::_file = NULL;

void fluidsynth_proc::forceDrum(bool force) {

    if(force) {
        fluid_synth_set_channel_type(synth, 16 + 9, CHANNEL_TYPE_DRUM);
        fluid_synth_set_channel_type(synth, 32 + 9, CHANNEL_TYPE_DRUM);
    } else {
        fluid_synth_set_channel_type(synth, 16 + 9, CHANNEL_TYPE_MELODIC);
        fluid_synth_set_channel_type(synth, 32 + 9, CHANNEL_TYPE_MELODIC);
    }

}

int fluidsynth_proc::change_synth(int freq, int flag) {

    int disabled = 0;

    mutex_fluid.lock();

    if(mdriver) delete_fluid_midi_driver(mdriver);
    mdriver = NULL;

    int id = sf2_id;
    sf2_id = -1;

    if(id >= 0 && synth) {
        //fluid_synth_program_reset(synth);
        for(int n = 0; n < 48; n++)
            fluid_synth_unset_program(synth, n);

        MSG_OUT("unloading the soundfont %i\n", id);
        fluid_synth_sfunload(synth, id, 0);
        //QThread::msleep(100);
        msDelay(100);

        sf2_id = -1;

    }

    if(synth) delete_fluid_synth(synth);
    synth = NULL;

    if(settings) delete_fluid_settings(settings);

    settings = new_fluid_settings();
    if(settings) {
        fluid_settings_setnum(settings, "synth.gain", synth_gain);
        fluid_settings_setint(settings, "synth.audio-channels", SYNTH_CHANS);
        fluid_settings_setint(settings, "synth.audio-groups", SYNTH_CHANS);
        fluid_settings_setint(settings, "synth.effects-groups", SYNTH_CHANS);
        fluid_settings_setint(settings, "synth.midi-channels", SYNTH_CHANS);
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

#ifdef __WINDOWS_MM__

            if(!sf2_name) {
                if(QFile::exists("C:/WINDOWS/system32/drivers/gm.dls")) {
                     sf2_name = new QString("C:/WINDOWS/system32/drivers/gm.dls");

                }

            }
#endif
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
        for(int n=0; n < SYNTH_CHANS; n++) {
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

void fluidsynth_proc::msDelay(int ms) {

    qint64 one = QDateTime::currentMSecsSinceEpoch();
    qint64 diff;
    do {

        QCoreApplication::processEvents();

        diff = QDateTime::currentMSecsSinceEpoch() - one;

    } while(diff < ms);
}

fluidsynth_proc::fluidsynth_proc()
{
    disabled = 0;
    settings = NULL;
    synth = NULL;
    adriver =  NULL;
    _bar = NULL;
    sf2_id = -1;
    mdriver = NULL;
    wavDIS = true;

    status_fluid_err = 255;
    status_audio_err = 255;
    block_midi_events = 0;

    mixer = NULL;

    fluid_set_log_function(FLUID_WARN, 	(fluid_log_function_t )fluid_log_function, NULL);

    //fluid_settings = new QSettings(QString("Fluid_MidiEditor"), QString("NONE"));
    fluid_settings = new QSettings(QDir::homePath() + "/Midieditor/settings/FluidMidiEditor.ini", QSettings::IniFormat);
    //qWarning("home %s", QDir::homePath().toLocal8Bit().constData());
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

    _player_status = PLAYER_STATUS_WAV_INIT;
    _player_wav =NULL;

    wait_signal = 0;
    iswaiting_signal = 0;

    set_error(0, NULL);
    sf2_id = FLUID_FAILED;

    synth_gain = 1.0;

    for(int n = 0; n < SYNTH_CHANS; n++) {
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

        level_WaveModulator[n] = 0.0;
        freq_WaveModulator[n] = 0.0;

    }

    // inicio
    // audio device


#if 0
    QString device_name;
    const QAudioDeviceInfo &defaultDeviceInfo = QAudioDeviceInfo::defaultOutputDevice();

    if (fluid_settings->value("Audio Device").toString() != "") {
        device_name = fluid_settings->value("Audio Device").toString();
    }

    _sample_rate = fluid_settings->value("Audio Freq" , 44100).toInt();
    _wave_sample_rate = fluid_settings->value("Wav Freq" , 44100).toInt();
    fluid_out_samples = fluid_settings->value("Out Samples" , 512).toInt();

    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);

    devices.append(QAudioDeviceInfo::defaultOutputDevice());
    devices.move(devices.count() - 1, 0);

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

#endif

    QString device_name;

    if (fluid_settings->value("Audio Device").toString() != "") {
        device_name = fluid_settings->value("Audio Device").toString();
    }

    _sample_rate = fluid_settings->value("Audio Freq" , 44100).toInt();
    _wave_sample_rate = fluid_settings->value("Wav Freq" , 44100).toInt();
    fluid_out_samples = fluid_settings->value("Out Samples" , 512).toInt();
    output_float = fluid_settings->value("Out use float" , 0).toInt();

    MAudioDev param;
    param.device_name = device_name;
    param.sample_rate = _sample_rate;
    param.is_float = output_float ? true : false;

    if(current_sound)
        delete current_sound;

    current_sound = MAudio::MAudio_findDev(param);
    if(!current_sound) {
        MSG_ERR("ERROR: Creating Audio Device\n");
        QMessageBox::critical(NULL, "Fluidsynth_proc Error", "Error creating Audio Device\n.");
        return;
    }

    if(current_sound->err) {
        delete current_sound;
        current_sound = NULL;
        MSG_ERR("ERROR: Audio Device not found\n");
        QMessageBox::critical(NULL, "Fluidsynth_proc Error", "Audio Device not found\n.");
        return;
    }

    // update from device
    if(current_sound->is_float)
        output_float = 1;
    else
        output_float = 0;

    qWarning("is output_float %i", output_float);

    _sample_rate = current_sound->sample_rate;
    device_name = current_sound->device_name;

    MSG_OUT("audio output:\n    sample_rate: %i use float %i - %s\n", _sample_rate, output_float, device_name.toLocal8Bit().constData());

    //for(int n = 0; n < 32; n++) // test if memory leak of fluid synth (version < 2.1.6)
    disabled = change_synth(_sample_rate, 0);

    if(disabled == -1) return;

    if(!disabled) {
        status_fluid_err = 0;

        for(int n = 0; n < SYNTH_CHANS; n++) {
            if(!disabled) fluid_synth_cc(synth, n , 11, synth_chanvolume[n]);         
        }

        fluidsynth_proc::first = this;
        if(!status_fluid_err) {
            qWarning("first fluid_Thread");

            mixer = new fluid_Thread(this);
        } else
            mixer = NULL;

        if(!mixer)  {
            status_fluid_err = 4;
            set_error(666, "FATAL ERROR: error creating fluid_Thread()"); return;
        } else {
            status_fluid_err = 0;
        }


        mixer->start(QThread::TimeCriticalPriority);

        frames = time_frame.msecsSinceStartOfDay();

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
    SendMIDIEvent(a, 0);
}

fluidsynth_proc::~fluidsynth_proc()
{
    if(mixer) {
        int *p = &iswaiting_signal;
        wait_signal = 2;
        *p = 0;
        audio_waits.wakeOne();
        int count = 0;
        while(*p != 2) { // waits to sound thread receives the signal
            msDelay(5);
            count++;
            if(count > 200)
                break;
        }
        lock_audio.lock();
        count = 0;
        while(!mixer->isFinished()) {
            msDelay(5);
            count++;
            if(count > 500)
                break;
        }

        msDelay(5);
        mixer->terminate();
        lock_audio.unlock();
        mixer->wait(1000);
        delete mixer;
    }

    if(mdriver) delete_fluid_midi_driver(mdriver);
    if(adriver) delete_fluid_audio_driver(adriver);
    if(synth) delete_fluid_synth(synth);
    if(settings) delete_fluid_settings(settings);

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

int fluidsynth_proc::SendMIDIEvent(QByteArray array, int track)
{
    int type= array[0] & 0xf0;
    int channel = array[0] & 0xf;
    int desp_channel = track * 16;
    int desp_channelVST = track * 32;
    int ret = 0;

    if(block_midi_events)
        return 0;
    if(sf2_id == FLUID_FAILED || !synth)
        return -1;

    wakeup_audio();

    switch(type) {

        case 0x90: // note on

            isNoteOn[channel + desp_channel] = 4;
            ret = 0;
#ifdef ONLY9
            if(channel == 9 && _file->DrumUseCh9) { // drum for fluidsynth
                desp_channel = 0;
                desp_channelVST = 0;
            }
#endif
            if(strange) array[1] = array[1]/3 * 3 + 3 - (array[1] % 3);


            if(VST_proc::VST_isMIDI(channel + desp_channelVST) && mixer)
                VST_proc::VST_MIDIcmd(channel + desp_channelVST,
                                      ((time_frame.msecsSinceStartOfDay() - frames) *  _sample_rate) / 1000
                                      , array);

            ret = fluid_synth_noteon(synth, channel + desp_channel, array[1], array[2]);
            break;

        case 0x80: // note off

#ifdef ONLY9
            if(channel == 9 && _file->DrumUseCh9) {// drum for fluidsynth
                desp_channel = 0;
                desp_channelVST = 0;
            }
#endif

            ret = 0;

            if(strange) array[1] = array[1]/3 * 3 + 3 - (array[1] % 3);

            if(VST_proc::VST_isMIDI(channel + desp_channelVST) && mixer)
                VST_proc::VST_MIDIcmd(channel + desp_channelVST,
                                      ((time_frame.msecsSinceStartOfDay() - frames) *  _sample_rate) /1000
                                      , array);

            ret = fluid_synth_noteoff(synth, channel + desp_channel, array[1]);
            break;

    case 0xB0: // control

        if(VST_proc::VST_isMIDI(channel + desp_channelVST) && mixer) {

            if((unsigned) array[1] == 3)  //ignore displace notes
                return 0;
            else if((unsigned) array[1] == 7) { // change volume of channel

                VST_proc::VST_MIDIvol(channel + desp_channelVST, array[2]);
                return 0;
            } else if((unsigned) array[1] == 9) { // wah wah control of channel
                VST_proc::Wah_Wah_Control(channel + desp_channelVST, array[2]);
                return 0;
            } else if((unsigned) array[1] >= 20 && (unsigned) array[1] <= 31) {

            } else VST_proc::VST_MIDIcmd(channel + desp_channelVST,
                                  ((time_frame.msecsSinceStartOfDay() - frames) *  _sample_rate) / 1000
                                  , array);
        }

        if((unsigned) array[1] == 3)  //ignore displace notes
            return 0;
        else if((unsigned) array[1] == 7) { // change volume of channel

            VST_proc::VST_MIDIvol(channel + desp_channelVST, array[2]);

        } else if((unsigned) array[1] == 9) { // wah wah control of channel
            VST_proc::Wah_Wah_Control(channel + desp_channelVST, array[2]);
            return 0;
        } else if((unsigned) array[1] == 11)  //ignore song expresion
            array[2]= synth_chanvolume[channel + desp_channel];

        else if((unsigned) array[1] == 121) {

            fluid_synth_cc(synth, channel + desp_channel, array[1], array[2]);
            array[1] = 11; // set expresion
            array[2] = synth_chanvolume[channel + desp_channel];

        } else if((unsigned) array[1] == 124) // omni off skip
            return 0;

        // Fluid Synth MIDI custom CC

        else if((unsigned) array[1] == 20) { // change volume of channel

            synth_chanvolume[channel + desp_channel] = array[2];
            setSynthChanVolume(channel + desp_channel, synth_chanvolume[channel + desp_channel]);

            if(fluid_control && !fluid_control->disable_mainmenu) {

                emit changeVolBalanceGain((channel + desp_channel) | (1<<6), getSynthChanVolume(channel + desp_channel) * 100 / 127,
                                          getAudioBalance(channel + desp_channel), getAudioGain(channel + desp_channel));
            }

            return 0;

        } else if((unsigned) array[1] == 21) { // change balance of channel

            audio_chanbalance[channel + desp_channel] = array[2] * 200 / 127 - 100;

            if(fluid_control && !fluid_control->disable_mainmenu) {

                emit changeVolBalanceGain((channel + desp_channel) | (2<<6), getSynthChanVolume(channel + desp_channel)* 100 / 127,
                                          getAudioBalance(channel + desp_channel), getAudioGain(channel + desp_channel));
            }

            return 0;

        } else if((unsigned) array[1] == 22) { // change gain  of channel

            audio_changain[channel + desp_channel] = array[2] * 2000 / 127 - 1000;

            if(fluid_control && !fluid_control->disable_mainmenu) {

                emit changeVolBalanceGain((channel + desp_channel)| (3<<6), getSynthChanVolume(channel + desp_channel)* 100 / 127,
                                          getAudioBalance(channel + desp_channel), getAudioGain(channel + desp_channel));
            }

            return 0;

        } else if((unsigned) array[1] == 23) { // distortion gain  of channel

            int gain = (unsigned) array[2] * 300 / 127;

            fluid_output->setAudioDistortionFilter( (gain > 0), channel + desp_channel, gain);

            if(fluid_control && !fluid_control->disable_mainmenu) {

                emit changeFilterValue(0, channel + desp_channel);
            }

            return 0;

        } else if((unsigned) array[1] == 24) { // Low Cut gain  of channel

            int gain = (unsigned) array[2] * 200 / 127;

            //gain = get_param_filter(PROC_FILTER_LOW_PASS, channel + desp_channel, GET_FILTER_GAIN);
            int freq = get_param_filter(PROC_FILTER_LOW_PASS, channel + desp_channel, GET_FILTER_FREQ);
            int res = get_param_filter(PROC_FILTER_LOW_PASS, channel + desp_channel, GET_FILTER_RES);

            setAudioLowPassFilter((gain > 0), channel + desp_channel, freq, gain, res);

            if(fluid_control && !fluid_control->disable_mainmenu) {

                emit changeFilterValue(1, channel + desp_channel);
            }

            return 0;

        } else if((unsigned) array[1] == 25) { // Low Cut freq  of channel

            int freq = 50 + ((unsigned) array[2] * 2450 / 127);

            int gain = get_param_filter(PROC_FILTER_LOW_PASS, channel + desp_channel, GET_FILTER_GAIN);
            //int freq = get_param_filter(PROC_FILTER_LOW_PASS, channel + desp_channel, GET_FILTER_FREQ);
            int res = get_param_filter(PROC_FILTER_LOW_PASS, channel + desp_channel, GET_FILTER_RES);

            setAudioLowPassFilter(1, channel + desp_channel, freq, gain, res);

            if(fluid_control && !fluid_control->disable_mainmenu) {

                emit changeFilterValue(1, channel + desp_channel);
            }

            return 0;

        } else if((unsigned) array[1] == 26) { // High Cut gain  of channel

            int gain = (unsigned) array[2] * 200 / 127;

            //gain = get_param_filter(PROC_FILTER_HIGH_PASS, channel + desp_channel, GET_FILTER_GAIN);
            int freq = get_param_filter(PROC_FILTER_HIGH_PASS, channel + desp_channel, GET_FILTER_FREQ);
            int res = get_param_filter(PROC_FILTER_HIGH_PASS, channel + desp_channel, GET_FILTER_RES);

            setAudioHighPassFilter((gain > 0), channel + desp_channel, freq, gain, res);

            if(fluid_control && !fluid_control->disable_mainmenu) {

                emit changeFilterValue(2, channel + desp_channel);
            }

            return 0;

        } else if((unsigned) array[1] == 27) { // High Cut freq  of channel

            int freq = 500 + ((unsigned) array[2] )* 4500 / 127;

            int gain = get_param_filter(PROC_FILTER_HIGH_PASS, channel + desp_channel, GET_FILTER_GAIN);
            //int freq = get_param_filter(PROC_FILTER_HIGH_PASS, channel + desp_channel, GET_FILTER_FREQ);
            int res = get_param_filter(PROC_FILTER_HIGH_PASS, channel + desp_channel, GET_FILTER_RES);

            setAudioHighPassFilter(1, channel + desp_channel, freq, gain, res);

            if(fluid_control && !fluid_control->disable_mainmenu) {

                emit changeFilterValue(2, channel + desp_channel);
            }

            return 0;

        } else if((unsigned) array[1] == 28) { // Low Cut Resonance of channel

            int res = (unsigned) array[2] * 250 / 127;

            int gain = get_param_filter(PROC_FILTER_LOW_PASS, channel + desp_channel, GET_FILTER_GAIN);
            int freq = get_param_filter(PROC_FILTER_LOW_PASS, channel + desp_channel, GET_FILTER_FREQ);
            //int res = get_param_filter(PROC_FILTER_LOW_PASS, channel + desp_channel, GET_FILTER_RES);

            setAudioLowPassFilter(1, channel + desp_channel, freq, gain, res);

            if(fluid_control && !fluid_control->disable_mainmenu) {

                emit changeFilterValue(1, channel + desp_channel);
            }

            return 0;

        } else if((unsigned) array[1] == 29) { // High Cut Resonance of channel

            int res = (unsigned) array[2] * 250 / 127;

            int gain = get_param_filter(PROC_FILTER_HIGH_PASS, channel + desp_channel, GET_FILTER_GAIN);
            int freq = get_param_filter(PROC_FILTER_HIGH_PASS, channel + desp_channel, GET_FILTER_FREQ);
            //int res = get_param_filter(PROC_FILTER_HIGH_PASS, channel + desp_channel, GET_FILTER_RES);

            setAudioHighPassFilter(1, channel + desp_channel, freq, gain, res);

            if(fluid_control && !fluid_control->disable_mainmenu) {

                emit changeFilterValue(2, channel + desp_channel);
            }

            return 0;

        } else if((unsigned) array[1] == 30) { // Main Volume

            synth_gain = ((float) ((unsigned) array[2] * 2000 / 127)) / 1000.0;

            float _gain = synth_gain;

            if(_gain > 1.0f)
                _gain = (_gain * _gain);

            fluid_synth_set_gain(synth, (float) _gain);

            if(fluid_control && !fluid_control->disable_mainmenu) {

                emit changeMainVolume(getSynthGain());
            }

            return 0;

        } else if((unsigned) array[1] == 31) { // switch effect events up/down

// obsolete
/*
            MidiInControl::set_events_to_down((unsigned) array[2] >= (unsigned) 64);

            emit MidiIn_set_events_to_down((unsigned) array[2] >= (unsigned) 64);
*/
            return 0;

        } else if((unsigned) array[1] >= 85 && (unsigned) array[1] <= 90) {
// Leslie CC
            int n = channel + desp_channelVST;

            switch((unsigned) array[1]) {
                case 85: {
                    VST_proc::leslieON[n] = ((unsigned) array[2]) >= 64;
                    break;
                }
                case 86: {
                    VST_proc::leslie[n].depthBass = ((float) ((unsigned) array[2]))/254.0f;
                    break;
                }
                case 87: {
                    VST_proc::leslie[n].rotationSpeedBass = 0.1f + 19.9f * (((float) ((unsigned) array[2]))/127.0f);
                    break;
                }
                case 88: {
                    VST_proc::leslie[n].frequency = 20.0f + 4980.0f * (((float) ((unsigned) array[2]))/127.0f);
                    break;
                }
                case 89: {
                    VST_proc::leslie[n].depthTreble = ((float) ((unsigned) array[2]))/254.0f;
                    break;
                }
                case 90: {
                    VST_proc::leslie[n].rotationSpeedTreble = 0.1f + 19.9f * (((float) ((unsigned) array[2]))/127.0f);
                    break;
                }
            }

            if(fluid_control && !fluid_control->disable_mainmenu) {

                emit changeFilterValue(-1, n);
            }

            return 0;
        }

        return fluid_synth_cc(synth, channel + desp_channel, array[1], array[2]);

    case 0xC0: // program

        ret = fluid_synth_program_change(synth, channel + desp_channel, array[1]);
        break;

    case 0xE0: // pitch bend

        if(VST_proc::VST_isMIDI(channel + desp_channelVST) && mixer) {

            VST_proc::VST_MIDIcmd(channel + desp_channelVST,
                                      ((time_frame.msecsSinceStartOfDay() - frames) *  _sample_rate) / 1000
                                      , array);
            ret = 0;
            break;
        }

        ret = fluid_synth_pitch_bend(synth, channel + desp_channel, (array[1] & 0x7f) | (array[2]<<7));
        break;

    case 0xF0: // systemEX
    {

        if(channel != 0)
            return 0;
        char id2[4] = {0x0, 0x66, 0x66, 'R'}; // global mixer

        if(fluid_control) { // anti-crash!
            fluid_control->disable_mainmenu = true;
                fluid_control->dis();
                fluid_control->deleteLater();
                fluid_control = NULL;
            }

            // sysEX uses VLQ as length! (https://en.wikipedia.org/wiki/Variable-length_quantity)

            unsigned int length = 0;

            int ind = 1;
            while(array[ind] & 128) {
                length = (length + (array[ind] & 127)) * 128;
                ind++;
            }
            length += array[ind];

            // new sysEx2 old compatibility
            if((((array[1+ind] == (char) 0 || array[1+ind] == (char) 1
                 || array[1+ind] == (char) 2) && (array[4+ind] & 0xf0) == 0x70)
                || array[4+ind] == 'R') && array[2+ind] == id2[1]
                    && array[3+ind] == id2[2] && ((array[4+ind] & 0xf0) == 0x70 || array[4+ind] == 'R')) {

                int entries = 13;
                char id[4];
                int BOOL;

                QDataStream qd(&array,
                               QIODevice::ReadOnly);
                qd.startTransaction();

                // new sysEx old compatibility
                qd.readRawData((char *) id, 1); // 0xf0

                do { // VLQ skip
                    qd.readRawData((char *) id, 1);
                } while(id[0] & 128);

                qd.readRawData((char *) id, 4);

                if(id[1] != id2[1] || id[2] != id2[2]) {
                    return 0;
                }

                if(!(((id[0] == (char) 0 || id[0] == (char) 1 || id[0] == (char) 2)
                      && (id[3] & 0xF0) == 0x70) || id[3] == 'R')) {
                    return 0;
                }

                int flag = 1;
                int n = id[3] & 15;
                int bucle = 1;
                int index = 0;

                if(id[3] == 'R') {
                    flag = 2;
                    if(id[0] >= (char) 16) {
                        flag = 3;
                        bucle = 16;
                    }
                    else n = id[0] & 15;
                }

                if(flag == 1) { // 0x7X
                    index = (id[0] % 3) * 16;
                    n+= index; // chan for track index
                }


                if(decode_sys_format(qd, (void *) &entries) < 0) {

                    return 0;
                }

                if(flag == 3 && entries != 15 * SYNTH_CHANS + 1 && entries != 21 * SYNTH_CHANS + 1)
                    bucle = SYNTH_CHANS;

                if((flag == 1 && (entries != 13 && entries != 15 && entries != 21)) ||
                        (flag == 2 && entries != 13 + (n == 0)) ||
                        (flag == 3 && entries != 15 * 16 + 1 &&
                         entries != 15 * SYNTH_CHANS + 1 &&
                         entries != 21 * SYNTH_CHANS + 1)) {
                    return 0;
                }

                for(int m = 0; m < bucle; m++) {

                    if(flag == 3) n = m;

                    if((flag == 1) || ((flag == 2 || flag == 3) && n == 0)) {
                        decode_sys_format(qd, (void *) &fluid_output->synth_gain);
                    }

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

                    if(flag == 3 || (flag == 1 && (entries == 15 || entries == 21))) {
                        decode_sys_format(qd, (void *) &fluid_output->level_WaveModulator[n]);
                        decode_sys_format(qd, (void *) &fluid_output->freq_WaveModulator[n]);
                        if(entries == 21 || entries == 21 * SYNTH_CHANS + 1) {

                            decode_sys_format(qd, (void *) &VST_proc::leslieON[n]);
                            decode_sys_format(qd, (void *) &VST_proc::VST_proc::leslie[n].depthBass);
                            decode_sys_format(qd, (void *) &VST_proc::VST_proc::leslie[n].depthTreble);
                            decode_sys_format(qd, (void *) &VST_proc::VST_proc::leslie[n].frequency);
                            decode_sys_format(qd, (void *) &VST_proc::VST_proc::leslie[n].rotationSpeedBass);
                            decode_sys_format(qd, (void *) &VST_proc::VST_proc::leslie[n].rotationSpeedTreble);
                        }

                    } else {
                        fluid_output->level_WaveModulator[n] = 0.0f;
                        fluid_output->freq_WaveModulator[n] = 0.0f;
                    }

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
                }

                if(fluid_control && !fluid_control->disable_mainmenu) {

                    emit changeMainVolume(getSynthGain());

                }

            } else if(/*array[2] == id2[0] &&*/ array[2+ind] == id2[1]
                 && array[3+ind] == id2[2] && (array[4+ind] == 'V' || array[4+ind] == 'W')) {

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

        if(MidiOutput::file)
            forceDrum(MidiOutput::file->DrumUseCh9);

        wait_signal = 0;

        int *p = &iswaiting_signal;
        *p = 0;
        set_error(0, NULL);


        qWarning("other fluid_Thread");
        status_audio_err = 1;
        mixer = new fluid_Thread((fluidsynth_proc *) fluidsynth_proc::first);

        if(!mixer)  {
            status_fluid_err = 4;
            set_error(666, "FATAL ERROR: error creating fluid_Thread()"); return;
        } /*else
            status_audio_err = 0;*/

        mixer->start(QThread::TimeCriticalPriority);

        int count = 0;

        while(*p != 555) {
            msDelay(5);
            count++;
            if(count > 400) {
                ERROR_CRITICAL2("new_sound_thread():mixer iswaiting_signal fail");
                break;
            }
        }
        *p = 0;

    }

    msDelay(150);

}
void fluidsynth_proc::stop_audio(bool stop) {

    if(stop) qWarning("stop audio on");
    else qWarning("stop audio off");

    if(stop) { // stops the audio loop

        block_midi_events = 1;
        msDelay(10);

        if(mixer) {

            int *p = &iswaiting_signal;

            wait_signal = 2;   
            *p = 0;
            audio_waits.wakeOne();
            msDelay(10);

            int count = 0;
            while(*p != 2) {// waits to sound thread receives the signal
                msDelay(10);
                count++;
                if(count > 1000) {
                    ERROR_CRITICAL2("stop_audio() iswaiting_signal fail");
                    break;
                }
            }

            lock_audio.lock();

            count = 0;
            while(!mixer->isFinished()) {
                msDelay(10);
                count++;
                if(count > 1000) {
                    ERROR_CRITICAL2("stop_audio() mixer->isFinished fail");
                    break;
                }
            }
            msDelay(10);

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

    for(int n = 0; n < SYNTH_CHANS; n++) PROC[n] = NULL;
}

fluid_Thread::~fluid_Thread() {

    if(fbuf && !sharedAudioBuffer) delete[] fbuf;
    if(dry) delete[] dry;
    if(fx) delete[] fx; 
    delete[] buffer;
    delete[] mix_buffer;

    for(int n = 0; n < SYNTH_CHANS; n++) delete PROC[n];

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
    MSG_OUT("audio channels: %i\n", n_aud_chan);
    int n_fx_chan = fluid_synth_count_effects_channels(_proc->synth);
    int n_fx_groups = fluid_synth_count_effects_groups(_proc->synth);
    MSG_OUT("effects channels: %i\n", n_fx_chan);
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

     // setup buffers to mix stereo audio per MIDI Channel (48 channels)

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

     for(int n = 0; n < SYNTH_CHANS; n++) {

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

     int _err_sound_flag = 0;

     int _count_samples = 0;

     MAudioOutput *out_sound_t = NULL;
     QIODevice *out_sound_io_t = NULL;

#ifdef IS_QT5
     QString realdevname = _proc->current_sound->device.deviceName();
#else
     QString realdevname = _proc->current_sound->device.description();
#endif

     _proc->status_audio_err = 1;
     out_sound_t = MAudio::MAudio_CreateDev(_proc->current_sound);

     if(out_sound_t) {

         _proc->output_float = _proc->current_sound->is_float ? 1 : 0;
         output_float = _proc->output_float;
         out_sound_io_t = out_sound_t->start();

         if(!out_sound_io_t) {
            delete out_sound_t;
            out_sound_t = NULL;
            _proc->status_audio_err = 2;
         } else
            _proc->status_audio_err = 0;
     }

     while(1) {

         if(_proc->_player_status == PLAYER_STATUS_WAV_END) {
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

         if(_proc->_player_status == PLAYER_STATUS_WAV_WAIT) {
             _proc->audio_waits.wait(&mutex);
         }

         QThread::usleep(1);

         // test if out buffer changed
         if((sharedAudioBuffer && sharedAudioBuffer->data() != fbuf) || (fluid_out_samples != _proc->fluid_out_samples)) {

             fluid_out_samples = _proc->fluid_out_samples;

#ifndef IS_QT5

             if(out_sound_t) {
                out_sound_t->stop();
                out_sound_t->setBufferSize((out_sound_t->format().bytesPerSample() * 2));
                out_sound_io_t = out_sound_t->start();
             }
#endif

             if(sharedAudioBuffer && sharedAudioBuffer->data() != fbuf) {
                 free(fbuf);
                 fbuf = (float *) sharedAudioBuffer->data();
             }

             _count_samples = 0;
             memset((void *) fbuf, 0, sizeof(float) * szbuf);
             memset(buffer, 0, sizeof(short) * FLUID_OUT_SAMPLES * 2);
             memset(mix_buffer, 0, sizeof(float) * FLUID_OUT_SAMPLES * 2);

             // setup buffers to mix stereo audio per MIDI Channel (48 channels)

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

         bool defaultchanged = false;

#ifndef IS_QT5

         // polling method to change the default device (only if it meets the sample rate)

         if(!(_proc && _proc->synth && _proc->_player_wav) &&_proc->current_sound->is_default_device) {

#ifdef IS_QT5
             if(realdevname != QAudioDeviceInfo::defaultOutputDevice().deviceName()) {
#else
             if(realdevname != QMediaDevices::defaultAudioOutput().description()) {
#endif
                 MAudioDev param;
                 param.device_name = QString("Default Output Device");
                 param.sample_rate = _proc->_sample_rate;
                 param.is_float = _proc->output_float ? true : false;

                 MAudioDev *temp_sound;

                 temp_sound = MAudio::MAudio_findDev(param);
                 if(temp_sound) {

                     if(!temp_sound->err && temp_sound->sample_rate == _proc->_sample_rate) {
#ifdef IS_QT5
                        realdevname = QAudioDeviceInfo::defaultOutputDevice().deviceName();
#else
                        realdevname = QMediaDevices::defaultAudioOutput().description();
#endif
                        defaultchanged = true;
                     }

                     delete temp_sound;
                 }
             }
         }
#endif

         if(!(_proc && _proc->synth && _proc->_player_wav) &&
                 (defaultchanged || (output_float != _proc->output_float || _err_sound_flag))) {

             int outf =_proc->output_float;

             defaultchanged = false;
             _count_samples = 0;

             MAudioDev param;
             param.device_name = _proc->current_sound->device_name;
             param.sample_rate = _proc->_sample_rate;
             param.is_float = outf ? true : false;

             MAudioDev *temp_sound;

             temp_sound = MAudio::MAudio_findDev(param);
             if(!temp_sound) {
                 MSG_ERR("ERROR: Creating Audio Device\n");
                 QMessageBox::critical(NULL, "Fluidsynth_proc Error", "Error creating Audio Device\n.");
                 return;
             }

             if(temp_sound->err) {
                 delete temp_sound;
                 MSG_ERR("ERROR: Audio Device not found\n");
                 QMessageBox::critical(NULL, "Fluidsynth_proc Error", "Audio Device not found\n.");
                 return;
             }

             // test if new device have the same sample rate
             if(temp_sound->sample_rate == _proc->_sample_rate) {

                 if(out_sound_io_t)
                     out_sound_io_t->close();

                 if(out_sound_t) {
                     out_sound_t->stop();
                     out_sound_t->disconnect(this);

                     delete out_sound_t;
                 }

                out_sound_io_t = NULL;

                 _proc->current_sound = temp_sound;
                 _proc->_sample_rate = temp_sound->sample_rate;

                 if(temp_sound->is_float)
                     outf = 1;
                 else
                     outf = 0;

                //qWarning("wow float %i", outf);

                _proc->output_float = output_float = outf;

                _proc->status_audio_err = 1;
                out_sound_t = MAudio::MAudio_CreateDev(temp_sound);

                if(out_sound_t) {

                    out_sound_io_t = out_sound_t->start();

                    if(out_sound_io_t) {

                        _proc->output_float = output_float = outf;
                        _err_sound_flag = 0;

                        _proc->fluid_settings->setValue("Out use float", outf);
                        MSG_OUT("fluid_Thread: audio output:\n    sample_rate: %i use float %i\n", _proc->_sample_rate, output_float);
                        _proc->status_audio_err = 0;
                    } else {

                        delete out_sound_t;
                        out_sound_t = NULL;
                        _proc->output_float = output_float;
                        _proc->status_audio_err = 2;
                    }

                } else
                    _proc->output_float = output_float;
             }

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
                 || *sem == 1 || out_sound_io_t == NULL || out_sound_t == NULL) {

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
             if(_proc->synth)
                MSG_ERR("Error! fluid_Thread: fluid_synth_process()");

             if(_proc && _proc->synth && _proc->_player_wav) { // alternative for WAV files

                 _proc->_player_status = PLAYER_STATUS_WAV_ERROR_SYNTH;
                 QString serr = QString::number(err);
                 QMessageBox::information(NULL, "Error! fluid_Thread: fluid_synth_process()", serr);


             }

             _proc->lock_audio.unlock();

             continue;
         }

         // reset audio buffer
         memset(mix_buffer, 0, fluid_out_samples * 2 * sizeof(float));
         int n, m;

         // VST_mix
         if(vst_fluid_lock && vst_fluid_lock->tryLock()) {

             VST_proc::VST_mix(dry, /*n_aud_chan*/PRE_CHAN, (_proc->_player_wav) ? _proc->_wave_sample_rate : _proc->_sample_rate, fluid_out_samples, 1);

             if(sharedAudioBuffer)
                VST_proc::VST_external_mix((_proc->_player_wav) ? _proc->_wave_sample_rate : _proc->_sample_rate, fluid_out_samples);

             VST_proc::VST_mix(dry, /*n_aud_chan*/PRE_CHAN, (_proc->_player_wav) ? _proc->_wave_sample_rate : _proc->_sample_rate, fluid_out_samples, 2);

             // Leslie effect
             for(int m = 0; m < (n_aud_chan * 2); m += 2) {

                 if(VST_proc::leslieON[m>>1]) {

                     VST_proc::VST_LeslieEffect(dry[m], dry[m + 1], (_proc->_player_wav) ? _proc->_wave_sample_rate : _proc->_sample_rate, fluid_out_samples, &VST_proc::leslie[m>>1]);
                 }

                 VST_proc::VST_Wah_Wah(m>>1, dry[m], dry[m + 1], (_proc->_player_wav) ? _proc->_wave_sample_rate : _proc->_sample_rate, fluid_out_samples);
             }

             vst_fluid_lock->unlock();
         }

         _proc->frames = _proc->time_frame.msecsSinceStartOfDay();

         _count_samples%= (_proc->_player_wav) ? _proc->_wave_sample_rate : _proc->_sample_rate;

         if(1) {

             bool WAVE_MOD = true;


             /** WAVE MODULATOR (1) IN **/

             float step_wm[SYNTH_CHANS];

             if(WAVE_MOD) { // test if WaveModulator is unused
                 WAVE_MOD = false;
                 for(m = 0; m < SYNTH_CHANS; m++) {
                     if(((float) _proc->level_WaveModulator[m]) > 0.0f) WAVE_MOD = true;
                 }
             }

             if(WAVE_MOD) {
                 for(m = 0; m < SYNTH_CHANS; m++) {
                     step_wm[m] = (6.2831f * ((float) _proc->freq_WaveModulator[m]) / ((double) ((_proc->_player_wav) ? _proc->_wave_sample_rate : _proc->_sample_rate)));
                 }
             }

             /** WAVE MODULATOR (1) OUT **/

             // mix audio channels
             for(m = 0; m < (n_aud_chan * 2/*OUT_CHANS */); m += 2) {

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

                 /** WAVE MODULATOR (2) IN **/

                 float f = 0.0f, lev1 = 0.0f, lev2;

                 if(WAVE_MOD && f < 32)
                     f = step_wm[m>>1] * ((double) _count_samples);

                 lev1 = _proc->level_WaveModulator[m>>1];
                 lev2 = 1.0f - lev1;

                 /** WAVE MODULATOR (2) OUT **/

#if defined(__SSE2__)
                 if(using_SSE2) {
                    if(filter) filter->PROC_samples_block(leftz, rightz, fluid_out_samples);
                 }
#endif
                 for(n = 0; n <  fluid_out_samples; n++) {
                     float left = *(leftz++), right = *(rightz++);
                     float *out = &mix_buffer[(n << 1)];

                     if(!using_SSE2) {
                         if(filter) filter->PROC_samples(&left, &right);
                     }

                     /** WAVE MODULATOR (3) IN **/

                     if(WAVE_MOD && (m < 96) && _proc->freq_WaveModulator[m>>1] != 0 && _proc->level_WaveModulator[m>>1] != 0) {
                         float c = (sin(f) + 1.0f)/2.0f;

                        c = c*lev1 + lev2; // sin wave

                        // c = (c>0.5f) ? c*lev1 + lev2 : lev2; // square wave

                        left *= c;
                        right *= c;
                        f+= step_wm[m>>1];
                     }

                     /** WAVE MODULATOR (3) OUT **/

                     left  *= gainl;
                     right *= gainr;
                     out[0] += left;
                     out[1] += right;
                 }
             }
         }

         _count_samples += fluid_out_samples;
// SSE2
         // convert and clip samples to output buffer

         float addl = 0.0, addr = 0.0;

         convert_Clipping_1(mix_buffer, buffer, fluid_out_samples, addl, addr);

         addl = sqrtf((addl) / (float) (fluid_out_samples));
         addr = sqrtf((addr) /  (float) (fluid_out_samples));

         _proc->cleft = 40.0 * log(addl * 200.0);
         _proc->cright = 40.0 * log(addr * 200.0);

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

             if(_proc && _proc->_player_wav) { // alternative for WAV files

                 if(_proc->_player_status >= PLAYER_STATUS_WAV_END && _proc->_player_status <= PLAYER_STATUS_WAV_NOTWRITE)
                     break;

                 if(_proc->_player_status == PLAYER_STATUS_WAV_BREAK || _proc->_player_status <= PLAYER_STATUS_WAV_ERROR)

                    len = -1;

                 else {

                     if(_proc->wavDIS) {
                         len = 0;
                         break;
                     } else {
                         len = _proc->_player_wav->write(((const char  *) buff + pos), total);
                         _proc->_player_wav->flush();
                     }
                 }

                 if(len < 0) {

                     if(_proc->_player_status >= PLAYER_STATUS_WAV_INIT)
                         _proc->_player_status = PLAYER_STATUS_WAV_BREAK;

                 } else {
                     _proc->total_wav_write += len;
                 }

             } else {// write audio stream

                 len = out_sound_io_t->write(((const char  *) buff2 + pos), total);

             }

             int stat = out_sound_t->state();

             if(((len < 0) || out_sound_t->error() || stat == QAudio::SuspendedState ||
#ifdef IS_QT5d
                  stat == QAudio::InterruptedState ||
#endif
                  stat == QAudio::StoppedState) && MidiPlayer::isPlaying()) {

                 emit _proc->pause_player();
                 QThread::usleep(2);
                 _err_sound_flag = 1;
             }

             if(len == 0) {

                 QThread::usleep(2);

                        // timeout at 2 seconds
                 if(((int) (QDateTime::currentMSecsSinceEpoch() - _system_time)) >= 2000) {
                     if (MidiPlayer::isPlaying())
                         emit _proc->pause_player();
                     QThread::usleep(2);
                     _err_sound_flag = 1;
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

     if(out_sound_io_t)
         out_sound_io_t->close();

     if(out_sound_t) {
         out_sound_t->stop();
         out_sound_t->disconnect(this);

         delete out_sound_t;
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


    for(int chan = 0; chan < SYNTH_CHANS; chan++) {

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

        if(!using_SSE2) {
            for(int n = 0; n <= (int) MAX_DIST_DEF; n++) {
                _distortion_tab[n] = (1.0 - exp(-(_gain3 * (float) n) / ((float) MAX_DIST_DEF))) * (1.0f / ((1.0 - exp(-_gain3))));

                if(_distortion_tab[n] < -.8f) _distortion_tab[n] = -.8f;
                else if(_distortion_tab[n] > .8f) _distortion_tab[n] = .8f;
            }
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
    _resdB2 = 0;
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

#if defined(__SSE2__)

#include <emmintrin.h>  // SSE2

inline float clampf(float x, float lo, float hi) {
  return (x < lo) ? lo : (x > hi) ? hi : x;
}

inline __m128 fast_tanh_sse(__m128 x) {
  // Aprox. de tanh(x): x * (27 + x^2) / (27 + 9x^2)
  const __m128 x2 = _mm_mul_ps(x, x);
  const __m128 num = _mm_mul_ps(x, _mm_add_ps(_mm_set1_ps(27.0f), x2));
  const __m128 denom = _mm_add_ps(_mm_set1_ps(27.0f), _mm_mul_ps(_mm_set1_ps(9.0f), x2));
  return _mm_div_ps(num, denom);
}

void PROC_filter::PROC_samples_block(float *left, float *right, int nsamples) {
  __m128 gain1 = _mm_set1_ps(_gain);
  __m128 gain2 = _mm_set1_ps(_gain2);
  __m128 one   = _mm_set1_ps(1.0f);
  __m128 minus_one = _mm_set1_ps(-1.0f);
  __m128 gain3 = _mm_set1_ps(_gain3);

  for (int i = 0; i < nsamples; i += 4) {

    __m128 l = _mm_loadu_ps(&left[i]);
    __m128 r = _mm_loadu_ps(&right[i]);

    // distorsion
    if(_type & PROC_FILTER_DISTORTION) {
        l = _mm_mul_ps(l, gain3); // apply drive
        r = _mm_mul_ps(r, gain3);

        l = fast_tanh_sse(l);    // non-linear waveshaping
        r = fast_tanh_sse(r);

        // optional: normalize again
        const __m128 inv_gain = _mm_set1_ps(1.0f / (_gain3 / 2.0f));
        l = _mm_mul_ps(l, inv_gain);
        r = _mm_mul_ps(r, inv_gain);

        _mm_store_ps(&left[i], l);
        _mm_store_ps(&right[i], r);
    }

    // Clipping [-1.0, 1.0]
    l = _mm_max_ps(minus_one, _mm_min_ps(one, l));
    r = _mm_max_ps(minus_one, _mm_min_ps(one, r));

    // Copias para filtros paso alto
    __m128 l_orig = l;
    __m128 r_orig = r;

    __m128 accul = _mm_setzero_ps();
    __m128 accur = _mm_setzero_ps();

             // Filtro paso bajo
    if (_type & PROC_FILTER_LOW_PASS) {
        // Vectorizamos el filtro solo si se aplica el mismo a todos los canales
        accul = _mm_add_ps(accul, _mm_mul_ps(_mm_set1_ps(_Icoefs[0]), l));
        accul = _mm_add_ps(accul, _mm_mul_ps(_mm_set1_ps(_Icoefs[1]), _mm_set1_ps(_Isleft[1])));
        accul = _mm_add_ps(accul, _mm_mul_ps(_mm_set1_ps(_Icoefs[2]), _mm_set1_ps(_Isleft[2])));
        accul = _mm_sub_ps(accul, _mm_mul_ps(_mm_set1_ps(_Ocoefs[0]), _mm_set1_ps(_Osleft[0])));
        accul = _mm_sub_ps(accul, _mm_mul_ps(_mm_set1_ps(_Ocoefs[1]), _mm_set1_ps(_Osleft[1])));

        accur = _mm_add_ps(accur, _mm_mul_ps(_mm_set1_ps(_Icoefs[0]), r));
        accur = _mm_add_ps(accur, _mm_mul_ps(_mm_set1_ps(_Icoefs[1]), _mm_set1_ps(_Isright[1])));
        accur = _mm_add_ps(accur, _mm_mul_ps(_mm_set1_ps(_Icoefs[2]), _mm_set1_ps(_Isright[2])));
        accur = _mm_sub_ps(accur, _mm_mul_ps(_mm_set1_ps(_Ocoefs[0]), _mm_set1_ps(_Osright[0])));
        accur = _mm_sub_ps(accur, _mm_mul_ps(_mm_set1_ps(_Ocoefs[1]), _mm_set1_ps(_Osright[1])));

        _Isleft[2] = _Isleft[1];
        _Isleft[1] = _Isleft[0];
        _Isleft[0] = _mm_cvtss_f32(l);

        _Isright[2] = _Isright[1];
        _Isright[1] = _Isright[0];
        _Isright[0] = _mm_cvtss_f32(r);

        l = _mm_mul_ps(accul, gain1);
        r = _mm_mul_ps(accur, gain1);

        _Osleft[1] = _Osleft[0];
        _Osleft[0] = _mm_cvtss_f32(accul);

        _Osright[1] = _Osright[0];
        _Osright[0] = _mm_cvtss_f32(accur);
    } else {
        l = r = _mm_setzero_ps();
    }

             // Filtro paso alto
    if (_type & PROC_FILTER_HIGH_PASS) {
        __m128 accul2 = _mm_setzero_ps();
        __m128 accur2 = _mm_setzero_ps();

        accul2 = _mm_add_ps(accul2, _mm_mul_ps(_mm_set1_ps(_I2coefs[0]), l_orig));
        accul2 = _mm_add_ps(accul2, _mm_mul_ps(_mm_set1_ps(_I2coefs[1]), _mm_set1_ps(_I2sleft[1])));
        accul2 = _mm_add_ps(accul2, _mm_mul_ps(_mm_set1_ps(_I2coefs[2]), _mm_set1_ps(_I2sleft[2])));
        accul2 = _mm_sub_ps(accul2, _mm_mul_ps(_mm_set1_ps(_O2coefs[0]), _mm_set1_ps(_O2sleft[0])));
        accul2 = _mm_sub_ps(accul2, _mm_mul_ps(_mm_set1_ps(_O2coefs[1]), _mm_set1_ps(_O2sleft[1])));

        accur2 = _mm_add_ps(accur2, _mm_mul_ps(_mm_set1_ps(_I2coefs[0]), r_orig));
        accur2 = _mm_add_ps(accur2, _mm_mul_ps(_mm_set1_ps(_I2coefs[1]), _mm_set1_ps(_I2sright[1])));
        accur2 = _mm_add_ps(accur2, _mm_mul_ps(_mm_set1_ps(_I2coefs[2]), _mm_set1_ps(_I2sright[2])));
        accur2 = _mm_sub_ps(accur2, _mm_mul_ps(_mm_set1_ps(_O2coefs[0]), _mm_set1_ps(_O2sright[0])));
        accur2 = _mm_sub_ps(accur2, _mm_mul_ps(_mm_set1_ps(_O2coefs[1]), _mm_set1_ps(_O2sright[1])));

        _I2sleft[2] = _I2sleft[1];
        _I2sleft[1] = _I2sleft[0];
        _I2sleft[0] = _mm_cvtss_f32(l_orig);

        _I2sright[2] = _I2sright[1];
        _I2sright[1] = _I2sright[0];
        _I2sright[0] = _mm_cvtss_f32(r_orig);

        l_orig = _mm_add_ps(l, _mm_mul_ps(accul2, gain2));
        r_orig = _mm_add_ps(r, _mm_mul_ps(accur2, gain2));

        _O2sleft[1] = _O2sleft[0];
        _O2sleft[0] = _mm_cvtss_f32(accul2);

        _O2sright[1] = _O2sright[0];
        _O2sright[0] = _mm_cvtss_f32(accur2);
    } else {

        if((_type & 7) != PROC_FILTER_DISTORTION) {
            l_orig = r_orig = _mm_setzero_ps();
        }
    }

    // Mezcla

    l = _mm_add_ps(l, l_orig);
    r = _mm_add_ps(r, r_orig);

    // Clipping final [-1, 1]
    l = _mm_max_ps(minus_one, _mm_min_ps(one, l));
    r = _mm_max_ps(minus_one, _mm_min_ps(one, r));

    _mm_storeu_ps(&left[i], l);
    _mm_storeu_ps(&right[i], r);
  }
}
#endif

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
            _left = -_distortion_tab[(int) (-_left * ((float) MAX_DIST_DEF)) & MAX_DIST_DEF]/* + fmodf(_left, 255.0)*/;
        else
            _left = _distortion_tab[(int) (_left * ((float) MAX_DIST_DEF)) & MAX_DIST_DEF]/* + fmodf(_left, 255.0)*/;

        if(_right < 0.0)
            _right = -_distortion_tab[(int) (-_right * ((float) MAX_DIST_DEF)) & MAX_DIST_DEF]/* + fmodf(_right, 255.0)*/;
        else
            _right = _distortion_tab[(int) (_right * ((float) MAX_DIST_DEF)) & MAX_DIST_DEF]/* + fmodf(_right, 255.0)*/;

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

    audio_changain[chan % SYNTH_CHANS] = gain;
}

int fluidsynth_proc::getAudioGain(int chan){

    return audio_changain[chan % SYNTH_CHANS];
}

void fluidsynth_proc::setAudioBalance(int chan, int balance){

    audio_chanbalance[chan % SYNTH_CHANS] = balance;
}

int fluidsynth_proc::getAudioBalance(int chan){

    return audio_chanbalance[chan % SYNTH_CHANS];
}

void fluidsynth_proc::setAudioMute(int chan, bool mute){

    audio_chanmute[chan % SYNTH_CHANS] = mute;
}

bool fluidsynth_proc::getAudioMute(int chan){

    return audio_chanmute[chan % SYNTH_CHANS];
}

void fluidsynth_proc::setSynthGain(int gain){

    synth_gain = ((float) gain) / 100.0;

    float _gain = synth_gain;

    if(_gain > 1.0f)
        _gain = (_gain * _gain);

    fluid_synth_set_gain(synth, (float) _gain);
}

int fluidsynth_proc::getSynthGain(){
    return (int) (synth_gain * 100.0);
}

void fluidsynth_proc::setAudioDistortionFilter(int on, int chan, int gain) {

    chan %= SYNTH_CHANS;

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

    chan %= SYNTH_CHANS;

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

    chan %= SYNTH_CHANS;

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

    chan %= SYNTH_CHANS;

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

    if(_player_status > PLAYER_STATUS_WAV_INIT)
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
    if(!_bar) ERROR_CRITICAL_NO_MEMORY();
    _bar->setModal(true);

    stop_audio(true);
    _player_wav = wav;
    _player_status = PLAYER_STATUS_WAV_NOTWRITE; // only process (not audio output)

    qWarning("wav fluid_Thread");
    fluid_Thread_playerWAV *player = new fluid_Thread_playerWAV((fluidsynth_proc *) fluidsynth_proc::first);

    if(!player) {
        _player_wav = NULL;
        wav->close();
        new_sound_thread(_sample_rate);
        stop_audio(false);
        MidiPlayer::panic();
        fluid_output->MidiClean();
        return -1;
    }

    MidiOutput::is_playing = 0;

    new_sound_thread(_wave_sample_rate);

    stop_audio(false);

    if(fluid_output->status_fluid_err) {

        emit message_timeout("Error!", "FluidSynth building error");

        fluid_output->wavDIS = true;

        stop_audio(true);
        _player_wav = NULL;
        new_sound_thread(_sample_rate);
        stop_audio(false);
        MidiPlayer::panic();
        fluid_output->MidiClean();
        wav->close();
        MidiOutput::is_playing = 1;
        return -1;
    }

    for(int n = 0; n < PRE_CHAN; n++) {

        VST_proc::VST_DisableButtons(n, true);
    }

    // cause audio_waits.wait() from fluid_Thread::run()
    lock_audio.lock();
    int *p = &_player_status;
    *p = PLAYER_STATUS_WAV_WAIT;
    lock_audio.unlock();
    msDelay(100);
    //*p = PLAYER_STATUS_WAV_INIT;

    // run()
    player->start(QThread::TimeCriticalPriority);

    MSG_OUT("Converting MID to WAV file:\n");


   int counter = 0;

    while(*p == PLAYER_STATUS_WAV_INIT || *p == PLAYER_STATUS_WAV_WAIT) {
        msDelay(250);
        counter++;
        if(counter > 20) break;
    }

    if(*p <= PLAYER_STATUS_WAV_ERROR) {
        *p = PLAYER_STATUS_WAV_BREAK;
        emit message_timeout("Error!", "Fluidsynth sequencer initialization failed");
        msDelay(250);
        delete _bar;
        _bar = NULL;
        goto finish_wav;
    }

    if(*p == PLAYER_STATUS_WAV_INIT || counter > 20) {
        *p = PLAYER_STATUS_WAV_BREAK;
        emit message_timeout("Error!", "Aborted by timeout!");
        msDelay(250);
        delete _bar;
        _bar = NULL;
        goto finish_wav;
    }

    //audio_waits.wakeOne();

    _bar->exec();

    delete _bar;
    _bar = NULL;

    if(*p == PLAYER_STATUS_WAV_RUN) { // progress bar is canceled

        *p = PLAYER_STATUS_WAV_BREAK;
        //QMessageBox::critical(NULL, "Error!", "Aborted!");
        emit message_timeout("Error!", "Aborted!");
    } else if(*p == PLAYER_STATUS_WAV_BREAK || *p == PLAYER_STATUS_WAV_ERROR_SYNTH) {
        *p = PLAYER_STATUS_WAV_BREAK;
        emit message_timeout("Internal Error!", "Aborted!");
    }

    counter = 0;

    while(*p != PLAYER_STATUS_WAV_INIT) {

        msDelay(50); // waits WAV thread ends
        if(*p <= PLAYER_STATUS_WAV_ERROR || *p == PLAYER_STATUS_WAV_BREAK)
            break;
        if(*p == PLAYER_STATUS_WAV_RUN)
            counter++;
        if(counter > 100)
            break;
    }

finish_wav:
    if(player) {
        
        player->terminate();
        player->wait(1000);
    }

    
    fluid_output->wavDIS = true;
    _player_status = PLAYER_STATUS_WAV_INIT;

    stop_audio(true);
    _player_status = PLAYER_STATUS_WAV_INIT;
    _player_wav = NULL;
    new_sound_thread(_sample_rate);
    stop_audio(false);
    MidiPlayer::panic();
    fluid_output->MidiClean();

    for(int n = 0; n < PRE_CHAN; n++) {

        VST_proc::VST_MIDInotesOff(n);

        VST_proc::VST_DisableButtons(n, false);
    }

    VST_proc::VST_MIDIend();

    MidiOutput::is_playing = 1;
    return 0;
}

ProgressDialog::ProgressDialog(QWidget *parent, QString text)
    : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{

    PBar= this;

    if (PBar->objectName().isEmpty())
        PBar->setObjectName(QString::fromUtf8("ProgressBar"));

    PBar->resize(446, 173);
    PBar->setFixedSize(446, 173);
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

    if(!semaf->available())
        semaf->release(1);

}

#define sequence_command(x) {fluid_event_t *evt = new_fluid_event();\
    fluid_event_set_source(evt, (fluid_seq_id_t) -1);\
    fluid_event_set_dest(evt, (fluid_seq_id_t) synthSeqID);\
    x ;\
    fluid_res = fluid_sequencer_send_at(sequencer, evt, (unsigned int) (event_ticks), (int) 1);\
    if(fluid_res < 0) MSG_ERR("Error in sequence_command ev %d err: %i\n", event_ticks, fluid_res);\
    delete_fluid_event(evt);}


#define sequence_callback(x) {unsigned int cevent_ticks = (x);\
    fluid_event_t *evt = new_fluid_event();\
    fluid_event_set_source(evt, (fluid_seq_id_t) -1);\
    fluid_event_set_dest(evt, (fluid_seq_id_t) mySeqID);\
    fluid_event_timer(evt, NULL);\
    fluid_res = fluid_sequencer_send_at(sequencer, evt, (unsigned int) cevent_ticks, (int) 1);\
    if(fluid_res < 0) MSG_ERR("Error in sequence_callback ev %d err: %i\n", cevent_ticks, fluid_res);\
    delete_fluid_event(evt);}

fluid_Thread_playerWAV::fluid_Thread_playerWAV(fluidsynth_proc *proc) {

    _proc = proc;
    sequencer_tick_pos = 0;
    sequencer_tick_end = 0;
    file_events = NULL;

}

int fluid_Thread_playerWAV::init_sequencer_player(){

    file_events = _proc->_file->playerData();
    if(!file_events)
        return -1;

    lock_audio = true;

    sequencer_tick_end = 0;
    sequencer_note_tick_end = 0;

    int n_events = 0;

    it = file_events->lowerBound(0);
    // get last event tick
    while (it != file_events->end()) {
        int sendPosition = it.key();
        OffEvent* noteoff = dynamic_cast<OffEvent*>(it.value());

        // get last tick note off event
        if(noteoff) {
            if(sendPosition > sequencer_note_tick_end)
                sequencer_note_tick_end = sendPosition;
        }

        // get last tick event
        if(sendPosition > sequencer_tick_end)
            sequencer_tick_end = sendPosition;

        it++; n_events++;
    }

    // new sequencer
    sequencer = new_fluid_sequencer2(0);
    if(!sequencer) {
        MSG_OUT("fail sequencer new\n");
        return (-1);
    }

    fluid_sequencer_set_time_scale(sequencer, 1000);

    if(!_proc->synth) {
        delete_fluid_sequencer(sequencer);
        sequencer = NULL;

        MSG_OUT("no synth!\n");
        return (-1);
    }

    // reset synth
    fluid_synth_system_reset(_proc->synth);

    for(int i = 0; i < SYNTH_CHANS; i++) {
        fluid_synth_all_notes_off(_proc->synth, i);
    }


    // register synth as first destination
    synthSeqID = fluid_sequencer_register_fluidsynth(sequencer, _proc->synth);

    if(synthSeqID == FLUID_FAILED) {
        delete_fluid_sequencer(sequencer);
        sequencer = NULL;
        MSG_OUT("fluid_sequencer_register fail!\n");
        return (-1);
    }

    // register myself as second destination
    mySeqID = fluid_sequencer_register_client(sequencer, "MidiEditorWAV", seq_callback, NULL);

    if(mySeqID == FLUID_FAILED) {
        delete_fluid_sequencer(sequencer);
        sequencer = NULL;
        MSG_OUT("fluid_sequencer_register_client fail!\n");
        return (-1);
    }

    // create callback semaphore

    semaf = new QSemaphore(1);
    if(!semaf) {
        fluid_sequencer_unregister_client(sequencer, mySeqID);
        mySeqID = FLUID_FAILED;
        delete_fluid_sequencer(sequencer);
        sequencer = NULL;

        MSG_OUT("new semaf fail\n");
        return (-1);
    }

    fluid_sequencer_process(sequencer, 0);

    MSG_OUT("init_sequencer_player()\n    n_events: %i\n    last tick: %i\n", n_events, sequencer_tick_end);

    _proc->_player_status = PLAYER_STATUS_WAV_RUN; // very important, here
    lock_audio = false;
    _proc->wakeup_audio();

    if(MidiOutput::file)
        _proc->forceDrum(MidiOutput::file->DrumUseCh9);

    for(int track = 0; track < (file_events->begin().value()->file()->MultitrackMode ? 3 : 1) ; track++) {
        // Reset all Controllers
        for (int i = 0; i < 16; i++) {
            QByteArray array;
            array.append(0xB0 | i);
            array.append(121);
            array.append(char(0));
            MidiOutput::sendCommand(array, track);

            array.clear();
            array.append(0xB0 | i);
            array.append(char(123)); // all notes off
            array.append(char(127));

            MidiOutput::sendCommand(array, track);

            array.clear();
            array.append(0xB0 | i);
            array.append(char(0)); // bank 0
            array.append(char(0x0));
            MidiOutput::sendCommand(array, track);

            array.clear();
            array.append(0xC0 | i);
            array.append(char(0x0)); // program
            MidiOutput::sendCommand(array, track);

            array.clear();
            array.append(0xE0 | i); //pitch bend
            array.append(char(0xff));
            array.append(char(0x3f));
            MidiOutput::sendCommand(array, track);

            array.clear();
            array.append(0xB0 | i);
            array.append(char(1)); // MODULATION
            array.append(char(0x0));
            MidiOutput::sendCommand(array, track);

            array.clear();
            array.append(0xB0 | i);
            array.append(char(11)); // EXPRESION
            array.append(char(127));
            MidiOutput::sendCommand(array, track);

            array.clear();
            array.append(0xB0 | i);
            array.append(char(7)); // VOLUME
            array.append(char(100));
            MidiOutput::sendCommand(array, track);

            array.clear();
            array.append(0xB0 | i);
            array.append(char(10)); // PAN
            array.append(char(64));
            MidiOutput::sendCommand(array, track);

            array.clear();
            array.append(0xB0 | i);
            array.append(char(73)); // ATTACK
            array.append(char(64));
            MidiOutput::sendCommand(array, track);

            array.clear();
            array.append(0xB0 | i);
            array.append(char(72)); // RELEASE
            array.append(char(64));
            MidiOutput::sendCommand(array, track);

            array.clear();
            array.append(0xB0 | i);
            array.append(char(75)); // DECAY
            array.append(char(64));
            MidiOutput::sendCommand(array, track);

            array.clear();
            array.append(0xB0 | i);
            array.append(char(91)); // REVERB
            array.append(char(0));
            MidiOutput::sendCommand(array, track);

            array.clear();
            array.append(0xB0 | i);
            array.append(char(93)); // CHORUS
            array.append(char(0));
            MidiOutput::sendCommand(array, track);

            array.clear();
            array.append(0xB0 | i);
            array.append(char(120)); // all sounds off
            array.append(char(0));
            MidiOutput::sendCommand(array, track);

        }
    }

    MidiOutput::playedNotes.clear();

    sequencer_tick_pos = 0;

    it = file_events->lowerBound(sequencer_tick_pos + 0);


    return 0;
}

#define SEQ_FRAME 45

static bool update_prg[SYNTH_CHANS]; // real time prg change event

int fluid_Thread_playerWAV::sequencer_player(){

    int last_time = sequencer_tick_pos+SEQ_FRAME;
    bool fseqcallbk = false;

    semaf->acquire(1); // waiting the callback...

    //int cur_time = fluid_sequencer_get_tick(sequencer);
    //qWarning("cur tick %i %i", cur_time, sequencer_tick_pos);

// run()

    int last_sended = -1;
    int last_time_loop = -1;

    bool direct_output = false;

    while (it != file_events->end() && it.key() < (sequencer_tick_pos + SEQ_FRAME)) {

        // save events for the given tick
        QList<MidiEvent *> onEv, offEv, ctrlEv, prgEv, sysEv;
        int sendPosition = it.key();

        // it needs waits the callback
        if(direct_output) {
            direct_output = false;
            semaf->acquire(1); // waiting the callback...
            fseqcallbk = false;
        }

        do {

            ControlChangeEvent* ctrl = dynamic_cast<ControlChangeEvent*>(it.value());
            ProgChangeEvent* prg = dynamic_cast<ProgChangeEvent*>(it.value());
            SysExEvent* sys = dynamic_cast<SysExEvent*>(it.value());

            if (sys) {
                sysEv.append(it.value());
            } else if (ctrl) {
                if(ctrl && ctrl->control() == 0) {
                    int track = ctrl->file()->MultitrackMode ? ctrl->track()->fluid_index() : 0;
                    // it needs a real time prg change event
                    update_prg[ctrl->channel() + track * 16] = true;
                }
                ctrlEv.append(it.value());
            } else if (prg) {
                prgEv.append(it.value());
            } else if(it.value()->isOnEvent()) {
                onEv.append(it.value());
            } else {
                offEv.append(it.value());
            }


            it++;
        } while (it != file_events->end() && it.key() == sendPosition);

        // test for direct output events
        if(sysEv.count() > 0 || ctrlEv.count() > 0 || prgEv.count() > 0) {
            direct_output = true;
        }

        // sending previous events if it is required
        if(last_time_loop >= 0 && direct_output) {
            int fluid_res;
            fseqcallbk = true;
            sequence_callback(last_time); // I needs to send previous events
            if(fluid_res < 0) return fluid_res;

            semaf->acquire(1); // waiting the callback...
            fseqcallbk = false;
            direct_output = false;
        }

        // SysEx is direct event
        foreach (MidiEvent* event, sysEv) {

            MidiOutput::sendCommand(event, 0);
        }

        // CtrlEv is direct event
        foreach (MidiEvent* event, ctrlEv) {
            int track = event->file()->MultitrackMode ? event->track()->fluid_index() : 0;
            MidiOutput::sendCommand(event, track);
        }

        // PrgEv is direct event
        foreach (MidiEvent* event, prgEv) {
            int track = event->file()->MultitrackMode ? event->track()->fluid_index() : 0;
            MidiOutput::sendCommand(event, track);
        }

        // events from callback
        foreach (MidiEvent* event, offEv) {
            int track = event->file()->MultitrackMode ? event->track()->fluid_index() : 0;
            sendCommand(event, sendPosition + 1, track);
        }


        // events from callback

/* For some stupid reason fluidsynth has problems when changing banks and programs.
    Sending program change just before first note works */

        foreach (MidiEvent* event, onEv) {

            int chan = event->channel();
            int track = event->file()->MultitrackMode ? event->track()->fluid_index() : 0;
            if(event->line() < 128 && update_prg[chan + track * 16]) {
                update_prg[chan + track * 16] = false;

                QByteArray array;
                array.clear();
                array.append(0xC0 | chan); // program change
                array.append(char(event->file()->Prog_MIDI[chan + 4 * (track!=0) + 16 * track]));
                MidiOutput::sendCommand(array, track);
            }

            sendCommand(event, sendPosition + 1, track);
        }


        last_time = sendPosition + 1;
        last_time_loop = last_time;

        // sending events if it is required
        if(direct_output) { // sync time for direct  events

            last_time_loop = -1; // no previous
            last_sended = last_time;
            int fluid_res;

            fseqcallbk = true;
            sequence_callback(last_time); // I needs to send previous events
            if(fluid_res < 0) return fluid_res;

        }

    }

    int fluid_res;

    sequencer_tick_pos = last_time;

    if(last_sended != last_time) {

        if(!fseqcallbk) {

            fseqcallbk = true;
            sequence_callback(sequencer_tick_pos);
            if(fluid_res < 0) return fluid_res;

        }
    }

    if(it == file_events->end() /*|| sequencer_tick_pos > 60000*/) {
        MSG_OUT("sequencer_player() ends\n    tick: %i", sequencer_tick_pos);

        //sequencer_tick_end = sequencer_tick_pos;

        sequencer_tick_end += SEQ_FRAME; //last_time;

        semaf->acquire(1); // waiting the callback...
        fseqcallbk = false;

        for(int n = 0; n < SYNTH_CHANS; n++) {

            int event_ticks= sequencer_tick_end + 1;
            sequence_command(fluid_event_control_change(evt, (int) n, (short) 123, (int) 127))
            if(fluid_res < 0) return fluid_res;
        }

        sequencer_note_tick_end+= 2000; // 2000 ms of silence

        sequencer_tick_end += SEQ_FRAME;

        if(sequencer_tick_end < sequencer_note_tick_end)
            sequencer_tick_end = sequencer_note_tick_end;

        fseqcallbk = true;
        sequence_callback(sequencer_tick_end);

        if(fluid_res < 0) return fluid_res;

        semaf->acquire(1); // waiting the callback...
        fseqcallbk = false;
        return 1;
    }

    return 0;
}

void fluid_Thread_playerWAV::run()
{

    int last_bar = -1;

    _proc->total_wav_write= 0;

    // real time prg change event off
    for(int n = 0; n < SYNTH_CHANS; n++)
        update_prg[n] = false;

    fluid_output->wavDIS = false;

    // write fake header
    write_header(_proc->_player_wav, 0, 0, 0, 0);

    connect(this, SIGNAL(setBar(int)), _proc->_bar, SLOT(setBar(int)));
    connect(this, SIGNAL(endBar()), _proc->_bar, SLOT(hide()));

    if(init_sequencer_player() < 0)
        _proc->_player_status = PLAYER_STATUS_WAV_ERROR;
    else {

        while(1) {

            if(_proc->_player_status == PLAYER_STATUS_WAV_RUN ||
                    _proc->_player_status == PLAYER_STATUS_WAV_NOTWRITE) { // playing

                int ret = sequencer_player();
                //qWarning("sequencer_player() %i", ret);

                if(_proc->_player_status == PLAYER_STATUS_WAV_BREAK) {
                    emit endBar();
                    msDelay(1000);
                    break; // external error
                }

                if(ret == 1) {
                    _proc->_player_status = PLAYER_STATUS_WAV_END; break; // end of file
                }

                if(ret < 0) {
                    _proc->_player_status = PLAYER_STATUS_WAV_BREAK;
                    emit endBar();
                    msDelay(1000);
                    break; // error!
                }
            }

            if(_proc->_player_status == PLAYER_STATUS_WAV_BREAK || _proc->_player_status == PLAYER_STATUS_WAV_ERROR_SYNTH) {
                emit endBar();
                msDelay(1000);
                break; // external error
            }

            if(_proc->_bar) {
                int a = 100 * sequencer_tick_pos / sequencer_tick_end;
                if(last_bar < a) {
                    last_bar = a;
                    emit setBar(a);
                }

            }

            QThread::usleep(1);
        }
    }

    if(file_events)
        file_events->clear();

    if(_proc->_player_status == PLAYER_STATUS_WAV_END) { // ok
        int *p = &_proc->iswaiting_signal;
        int count = 0;
        while(*p != 1) {
            fluidsynth_proc::msDelay(5);
            count++;
            if(count > 400) {
                ERROR_CRITICAL2("fluid_Thread_playerWAV::run() iswaiting_signal fail");
                break;
            }
        }
        _proc->_player_wav->seek(0);
        fluid_output->wavDIS = true;
        QFile *wav = _proc->_player_wav;
        //_proc->_player_wav = NULL;
        write_header(wav, _proc->total_wav_write, _proc->_wave_sample_rate, (_proc->wav_is_float) ? 32 : 16, 2);
        wav->close();

        _proc->_player_status = PLAYER_STATUS_WAV_INIT;

        if( _proc->_bar) {
            emit endBar();
        }

    } else {
        fluid_output->wavDIS = true;
        QFile *wav = _proc->_player_wav;
        //_proc->_player_wav = NULL;
        wav->remove();
        if(_proc->_player_status >= PLAYER_STATUS_WAV_INIT &&
                _proc->_player_status != PLAYER_STATUS_WAV_BREAK)
            _proc->_player_status = PLAYER_STATUS_WAV_INIT;

        if( _proc->_bar) {
            emit endBar();
        }
    }

    fluid_output->wavDIS = true;

    QThread::msleep(1000);

    for(int n = 0; n < SYNTH_CHANS; n++){
        fluid_synth_all_sounds_off(_proc->synth, n);
        fluid_synth_all_notes_off(_proc->synth, n);
    }

    if(sequencer) {

        delete_fluid_sequencer(sequencer);
    }

    if(semaf)
        delete semaf;

}

#define u32 unsigned int

int fluid_Thread_playerWAV::sendCommand(MidiEvent*event, int ms, int track) {

    QByteArray data = event->save();

    if(MidiOutput::AllTracksToOne)
        track = 0;

    int type = data[0] & 0xf0;
    int channel = data[0] & 0xf;
    int desp_channel = track * 16;
    int desp_channelVST = track * 32;
    int fluid_res;
    int event_ticks = ms;

    if(!lock_audio)
        _proc->wakeup_audio();

    if(type == 0x90) { // note on
#ifdef ONLY9
        if(channel == 9 && event->file()->DrumUseCh9) { // drum for fluidsynth
            desp_channel = 0;
            desp_channelVST = 0;
        }
#endif
        if(VST_proc::VST_isMIDI(channel + desp_channelVST)) {
            VST_proc::VST_MIDIcmd(channel + desp_channelVST,((event_ticks % 1000 * (_proc->fluid_out_samples / _proc->_wave_sample_rate)) * _proc->_wave_sample_rate) / 1000, data);
        }

        sequence_command(fluid_event_noteon(evt, (int) (channel + desp_channel), (short) data[1], (short) data[2]))
        if(fluid_res < 0) return fluid_res;
        return 0;

    } else if(type == 0x80) { // note off
#ifdef ONLY9
        if(channel == 9 && event->file()->DrumUseCh9) { // drum for fluidsynth
            desp_channel = 0;
            desp_channelVST = 0;
        }
#endif
        if(VST_proc::VST_isMIDI(channel + desp_channelVST))
            VST_proc::VST_MIDIcmd(channel + desp_channelVST,((event_ticks % 1000 * (_proc->fluid_out_samples / _proc->_wave_sample_rate)) * _proc->_wave_sample_rate) / 1000 , data);

        sequence_command(fluid_event_noteoff(evt, (int) (channel + desp_channel), (short) data[1]))
        if(fluid_res < 0) return fluid_res;
        return 0;

    } else if(type == 0xE0) { // pitch bend
        if(VST_proc::VST_isMIDI(channel + desp_channelVST)) {

            VST_proc::VST_MIDIcmd(channel + desp_channelVST,((event_ticks % 1000 * (_proc->fluid_out_samples / _proc->_wave_sample_rate)) * _proc->_wave_sample_rate) / 1000 , data);

        }
        sequence_command(fluid_event_pitch_bend(evt, (int) (channel + desp_channel), (int) ((data[1] & 0x7f) | (data[2]<<7))))
        if(fluid_res < 0) return fluid_res;
        return 0;
    }
    else // ignore
        return 0;

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

/////////////////////////////////////////////////////////////////////
// SSE2 functions
/////////////////////////////////////////////////////////////////////

bool using_SSE2 = false;

#if defined(__SSE2__)

#include <emmintrin.h> // SSE2

#include <stdint.h>

#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__GNUC__) || defined(__clang__)
#include <cpuid.h>
#endif

bool supportsSSE2() {
  uint32_t eax, ebx, ecx, edx;

#if defined(_MSC_VER)
  int cpuInfo[4];
  __cpuid(cpuInfo, 1);
  edx = cpuInfo[3];
#elif defined(__GNUC__) || defined(__clang__)
  __cpuid(1, eax, ebx, ecx, edx);
#else
  return false; // no soportado
#endif

  return (edx & (1 << 26)) != 0; // bit 26 = SSE2
}

void _convert_Clipping_1_SSE2(float* mix_buffer, short* buffer, int fluid_out_samples, float& addl, float& addr) {

  int n = 0;
  int total = fluid_out_samples * 2;

  __m128 vMin = _mm_set1_ps(-1.0f);
  __m128 vMax = _mm_set1_ps(1.0f);
  __m128 vScale = _mm_set1_ps(32767.0f);

  for (; n <= total - 8; n += 8) {
      __m128 s1 = _mm_loadu_ps(&mix_buffer[n]);
      __m128 s2 = _mm_loadu_ps(&mix_buffer[n + 4]);

      // Clipping
      s1 = _mm_min_ps(_mm_max_ps(s1, vMin), vMax);
      s2 = _mm_min_ps(_mm_max_ps(s2, vMin), vMax);

      // Acumulación por canal (intercalados L R L R ...)
      float temp[8];
      _mm_storeu_ps(&temp[0], s1);
      _mm_storeu_ps(&temp[4], s2);
      for (int i = 0; i < 8; ++i) {
          if ((n + i) & 1)
            addr += temp[i] * temp[i];
          else
            addl += temp[i] * temp[i];
        }

      // Escalado a 16 bits y almacenamiento
      s1 = _mm_mul_ps(s1, vScale);
      s2 = _mm_mul_ps(s2, vScale);

             // Convertimos a enteros
      __m128i i1 = _mm_cvtps_epi32(s1);
      __m128i i2 = _mm_cvtps_epi32(s2);

      // Empaquetamos en 16 bits
      __m128i packed = _mm_packs_epi32(i1, i2);

      // Guardamos
      _mm_storeu_si128((__m128i*)&buffer[n], packed);
    }

  // resto
  for (; n < total; ++n) {
      float sample = mix_buffer[n];

      if (sample < -1.0f) sample = -1.0f;
      else if (sample > 1.0f) sample = 1.0f;

      if (n & 1)
        addr += sample * sample;
      else
        addl += sample * sample;

      buffer[n] = static_cast<short>(sample * 32767.0f);
    }
}
#else

bool supportsSSE2() {
    return false;
}

#endif


void convert_Clipping_1(float* f, short* buffer, int fluid_out_samples, float& addl, float& addr) {

#if defined(__SSE2__)
  if(using_SSE2) {
      _convert_Clipping_1_SSE2(f, buffer, fluid_out_samples, addl, addr);
      return;
  }
#endif

  for(int n = 0; n < fluid_out_samples * 2; n++) {

      if(*f < -1.0f) *f = -1.0f;
      else if(*f  > 1.0f) *f = 1.0f;

      if(n & 1)
        addr+= *f * *f;
      else
        addl+= *f * *f;

      buffer[n] = 32767.0f * (*(f++));

    }

}

#if defined(__SSE2__)

void _set_fbuff_volume_SSE2(float vol, int nsamples, float *lbuf, float *rbuf, float *lvst, float *rvst) {
  int n = 0;
  int aligned_samples = nsamples & ~3; // alinea a 4
  __m128 vvol = _mm_set1_ps(vol);      // volumen como vector

  // bloques de 4
  for (; n < aligned_samples; n += 4) {
      __m128 lsrc = _mm_loadu_ps(&lvst[n]);  // carga 4 floats de lvst
      __m128 rsrc = _mm_loadu_ps(&rvst[n]);  // carga 4 floats de rvst

      __m128 lres = _mm_mul_ps(lsrc, vvol);  // multiplica
      __m128 rres = _mm_mul_ps(rsrc, vvol);

      _mm_storeu_ps(&lbuf[n], lres);         // guarda resultado
      _mm_storeu_ps(&rbuf[n], rres);
    }

  // Resto
  for (; n < nsamples; ++n) {
      lbuf[n] = lvst[n] * vol;
      rbuf[n] = rvst[n] * vol;
    }
}

#endif

void set_fbuff_volume(float vol, int nsamples, float *lbuf, float *rbuf, float *lvst, float *rvst) {

#if defined(__SSE2__)
  if(using_SSE2) {
      _set_fbuff_volume_SSE2(vol, nsamples, lbuf, rbuf, lvst, rvst);
      return;
    }
#endif

  for(int n = 0; n < nsamples; n++) {
      lbuf[n] = lvst[n] * vol;
      rbuf[n] = rvst[n] * vol;
    }
}

#endif
