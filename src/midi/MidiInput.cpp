/*
 * MidiEditor
 * Copyright (C) 2010  Markus Schwenk
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

#include "MidiInput.h"
#include "MidiInControl.h"
#include "PlayerThread.h"
#include "MidiPlayer.h"

#include "../MidiEvent/ControlChangeEvent.h"
#include "../MidiEvent/KeyPressureEvent.h"
#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/OffEvent.h"
#include "../MidiEvent/OnEvent.h"


#include "MidiTrack.h"

#include <QByteArray>
#include <QTextStream>

#include <cstdlib>

#include "rtmidi/RtMidi.h"

#include "MidiOutput.h"

static int virtual_device = -1;

RtMidiIn* MidiInput::_midiIn[MAX_INPUT_DEVICES];
QString MidiInput::_inPort[MAX_INPUT_DEVICES];
unsigned int MidiInput::keys_dev[MAX_INPUT_DEVICES][16];
unsigned int MidiInput::keys_fluid[MAX_INPUT_DEVICES];
int MidiInput::keys_vst[MAX_INPUT_DEVICES][16];
unsigned int MidiInput::keys_seq[MAX_INPUT_DEVICES];
unsigned int MidiInput::keys_switch[MAX_INPUT_DEVICES];

bool MidiInput::keyboard2_connected[MAX_INPUT_DEVICES];
std::vector<unsigned char> MidiInput::message[MAX_INPUT_DEVICES];

int MidiInput::loadSeq_mode = 2;

QMutex MidiInput::mutex_input;

QMultiMap<int, std::vector<unsigned char> >* MidiInput::_messages = new QMultiMap<int, std::vector<unsigned char> >;
int MidiInput::_currentTime = 0;
bool MidiInput::_recording = false;
bool MidiInput::_thru = false;
int MidiInput::track_index = 0;
MidiFile *MidiInput::file = NULL;

static int nkeyboard[MAX_INPUT_DEVICES];
static int vkeyboard[MAX_INPUT_DEVICES];
//static int keyboard2 = DEVICE_ID + 1;

void MidiInput::init()
{
    cleanKeyMaps();

    for(int n = 0; n < MAX_INPUT_DEVICES; n++) {
        nkeyboard[n] = DEVICE_ID + n;
        vkeyboard[n] = DEVICE_SEQUENCER_ID + n;
        _midiIn[n] = NULL;
        _inPort[n] = "";
        keyboard2_connected[n] = false;

        message[n].clear();

        keys_fluid[n] = 0;

        for(int m = 0; m < 16; m++) {
            keys_dev[n][m] = 0;
            keys_vst[n][m] = 1;
        }

        keys_seq[n] = 0;
        keys_switch[n] = 0;

        // RtMidiIn constructor
        try {
            QString s = QString("MidiEditor input") + QString::number(n);
            _midiIn[n] = new RtMidiIn(RtMidi::UNSPECIFIED, s.toStdString());
            //_midiIn->setQueueSizeLimit(65535);
            _midiIn[n]->ignoreTypes(false, true, true);
            _midiIn[n]->setCallback(&receiveMessage_mutex, &nkeyboard[n]);
        } catch (RtMidiError& error) {
            error.printMessage();
        }
    }

}

static unsigned char map_key[16][128];
static int pedal_chan = -1;

void MidiInput::cleanKeyMaps() {

    memset(map_key, 0, 16 * 128);
    pedal_chan = -1;
}

#define DELAYED_BREAK 1000

// from FingerPatternDialog

extern int finger_token[2];
extern bool _note_finger_disabled;

QByteArray MidiInput::note_roll[16];
//QByteArray MidiInput::note_roll_out[16];
unsigned char MidiInput::note_roll_velocity[16];

void MidiInput::receiveMessage(double /*deltatime*/, std::vector<unsigned char>* message, void* userData)
{

    if(!file)
        return;

    int is_effect = 0;
    int no_record = 0;

    int note_finger = -1;

    int is_finger_token = 0;
    bool ignore_finger_token = false;

    bool is_keyboard2 = false;
    bool keyboard2_connected = false;

    int id = DEVICE_ID;

    int pairdev = 0;

    if(((void *) finger_token) == userData) {

        is_finger_token = 1;
        pairdev = finger_token[1];

    } else if(userData) {

        id = *((int *) userData);

        if(id >= DEVICE_ID && id < (DEVICE_ID + 15)) {
            pairdev = ((id - DEVICE_ID)/2);

            // test DEVICE DOWN Channel;

            if((id - DEVICE_ID) & 1) {
                int ch_down = MidiInControl::channelDown(pairdev);

                for(int n = 0; n < MAX_INPUT_PAIR; n++) {

                    int ch_up = ((MidiInControl::channelUp(n) < 0)
                                     ? MidiOutput::standardChannel()
                                     : MidiInControl::channelUp(n)) & 15;

                    int ch_down2 = MidiInControl::channelDown(n);

                    if(pairdev != n && !MidiInput::isConnected(n * 2))
                        continue;

                    /*
                    if(pairdev != n && !MidiInput::isConnected(n * 2 + 1))
                        ch_down2 = -1;
                    */

                    if(n == pairdev)
                        ch_down2 = -1;
                    else
                        ch_down2 = (ch_down2 & 15);

                    if(ch_down < 0 || ((ch_down & 15) == ch_up) ||
                        ((ch_down & 15) == ch_down2)) {
                        // invalid DOWN channel;
                        MidiInControl::set_leds(pairdev, false, false, false, true);
                        if(ch_down < 0) {
                            QString text = "Dev: " + QString::number((id - DEVICE_ID)) + " - THE CHANNEL IS DISABLED";
                            MidiInControl::OSD = text;
                        } else {
                            QString text = "Dev: " + QString::number((id - DEVICE_ID)) + " - THE CHANNEL IS ALREADY IN USE BY ANOTHER DEVICE";
                            MidiInControl::OSD = text;
                        }

                        return;
                    }
                }
            } else {

                int ch_up = ((MidiInControl::channelUp(pairdev) < 0)
                                 ? MidiOutput::standardChannel()
                                 : MidiInControl::channelUp(pairdev)) & 15;

                int ch_down = MidiInControl::channelDown(pairdev);

                for(int n = 0; n < MAX_INPUT_PAIR; n++) {

                    if(n == pairdev)
                        continue;

                    if(!MidiInput::isConnected(n * 2))
                        continue;

                    int ch_up2 = ((MidiInControl::channelUp(n) < 0)
                                     ? MidiOutput::standardChannel()
                                     : MidiInControl::channelUp(n)) & 15;

                    int ch_down2 = MidiInControl::channelDown(n);

                    if(ch_down < 0 || ch_down2 < 0)
                        ch_down2 = 0;

                    if(ch_up == ch_up2 || ((ch_down & 15) == (ch_down2 & 15))) {
                        // invalid UP channel;
                        MidiInControl::set_leds(pairdev, false, false, true, false);
                        QString text = "Dev: " + QString::number((id - DEVICE_ID)) + " - THE CHANNEL IS ALREADY IN USE BY ANOTHER DEVICE";
                        MidiInControl::OSD = text;
                        return;
                    }
                }

            }

        } else if(id >= DEVICE_SEQUENCER_ID  && id < (DEVICE_SEQUENCER_ID + 16))
            pairdev = ((id - DEVICE_SEQUENCER_ID)/2);

        keyboard2_connected = MidiInput::keyboard2_connected[pairdev * 2 + 1];

        if(id >= DEVICE_ID && id < (DEVICE_ID + 15)) {
            if((id - DEVICE_ID) & 1) {

                is_keyboard2 = true;
            }

        }


    }


    // read keys, read ctrl and effects control

    if (message->size() > 1) {


        if(message->size() == 3 && !is_finger_token && (id >= DEVICE_ID && id < (DEVICE_ID + 15))) {
            int evt = message->at(0);
            int evtm = (evt & 0xF0);

            // convert to note off
            if(evtm == 0x90 && message->at(2) == 0) {
                evt = 0x80 | (evt & 0xf);
                message->at(0) = evt;
                evtm = (evt & 0xF0);
            }

// get_key / get_ctrl for actions
            if(evtm == 0x80 || evtm == 0x90) { // read key on

                if(MidiInControl::skip_key()) // skip key by timer action
                    return;
                if(MidiInControl::set_key((int) message->at(1), evt, id - DEVICE_ID))
                    return;
            }

            if(evtm == 0xb0) {
                if(MidiInControl::skip_key())  // skip key by timer action
                    return;
                // read control
                if(MidiInControl::set_ctrl((int) message->at(1), evt, id - DEVICE_ID))
                    return;
            }

        }

// process actions effects

        if(MidiInControl::split_enable(pairdev) && id >= DEVICE_ID && id < (DEVICE_ID + 15)) {

            int action = MidiInControl::new_effects(message, is_finger_token ? 0 : id);
            if(action == RET_NEWEFFECTS_SKIP) return;
            if(action == RET_NEWEFFECTS_SET) {
                is_effect = 1;
                _messages->insert(_currentTime, *message);
                if(_thru)
                    send_thru(pairdev, is_effect, message);
                return;

            }

            if(action >= RET_NEWEFFECTS_BYPASS) { // bypass
                action = (action - RET_NEWEFFECTS_BYPASS) / 2;

                is_effect = 1;
                _messages->insert(_currentTime, *message);
                if(_thru)
                    send_thru(action, is_effect, message);
                return;
            }
        }


        int evt = message->at(0);

        int ch = evt & 0xF;

        evt&= 0xF0;

        int is_note = 0;

// anti sleep
        if((id >= DEVICE_ID && id < (DEVICE_ID + 15)) && (evt== 0x80 || evt== 0x90)) {
            is_note = 1;
            MidiInControl::key_live = 1; // used for no sleep the computer
           // qDebug("note 0x%x chan %i %i", evt, ch, is_finger_token);

            if(message->size() < 3) return; // note lenght protection
        }

// pre-process sequencer notes

        int seq_in = -1;

        if(id >= DEVICE_SEQUENCER_ID  && id < (DEVICE_SEQUENCER_ID + 16)) {

            int seq_th  = (id - (DEVICE_SEQUENCER_ID));

            if(evt == 0x90 && MidiOutput::sequencer_enabled[seq_th] < 0 )
                return;

            bool autorhythm = MidiPlayer::fileSequencer[seq_th] && MidiPlayer::fileSequencer[seq_th]->autorhythm;

            if(autorhythm)  {
                // use ch 9 bypass
                if((ch >= 0 && ch < 4)) {
                    seq_in = ch + 12;
                    ch = 9;
                    message->at(0) = evt | ch;

                }

            }

            if(ch != 9) {

                if(seq_th & 1) {

                    ch = MidiInControl::inchannelDown(pairdev);
                    is_keyboard2 = true;
                } else
                    ch = MidiInControl::inchannelUp(pairdev);
            }

            message->at(0) = evt | ch;

        }


        int seq_dev0 = 0;
        int seq_dev1 = 1;

        if(id >= DEVICE_SEQUENCER_ID  && id < (DEVICE_SEQUENCER_ID + 16)) {

            int seq_th  = pairdev * 2;

            seq_dev0 = seq_th;
            seq_dev1 = seq_th + 1;

        }

// real note used in sequencer

        if(MidiInControl::split_enable(pairdev) && ch != 9 && !is_finger_token && is_note && (id >= DEVICE_ID  && id < (DEVICE_ID + 15))) {

            bool ret = false;

            int seq_th = pairdev * 2;

            seq_dev0 = seq_th;
            seq_dev1 = seq_th + 1;

            bool autorhythm0 = MidiPlayer::fileSequencer[seq_dev0] && MidiPlayer::fileSequencer[seq_dev0]->autorhythm;
            bool autorhythm1 = MidiPlayer::fileSequencer[seq_dev1] && MidiPlayer::fileSequencer[seq_dev1]->autorhythm;

            //message->at(0) = evt;
            if(evt == 0x90) { // read key on
                int the_note = message->at(1);
                MidiInput::message[id - DEVICE_ID] = *message;

                if((!autorhythm0 && MidiOutput::sequencer_enabled[seq_dev0] >= 0) &&
                   (!is_keyboard2 &&
                    (MidiInput::keyboard2_connected[pairdev * 2  + 1] ||
                     MidiInControl::note_duo(pairdev) || the_note >= MidiInControl::note_cut(pairdev)))) {

                    ret = true;
                    bool break_loop = true;
                    bool note_found = false;
                    int seq = seq_dev0;
                    for(int n = 0; n < note_roll[seq].length(); n++) {
                        if(!(note_roll[seq].at(n) & 128))
                            break_loop = false;

                        if((note_roll[seq].at(n) & 127) == the_note) {
                            note_roll[seq][n] = the_note;
                            note_found = true;
                        }
                    }

                    if(break_loop){
                        note_found = false;
                        note_roll[seq].clear();
                    }

                    // release if max notes
                    if(note_roll[seq].length() >= 32) {
                        if(MidiPlayer::fileSequencer[seq])
                            MidiPlayer::fileSequencer[seq]->set_flush(note_roll[seq].at(0) & 127);
                        note_roll[seq] = note_roll[seq].right(31);
                    }

                    // delete repeat note
                    for(int n = 0; n < note_roll[seq].length(); n++) {
                       if((note_roll[seq].at(n) & 127) == (char) the_note) {
                            deleteIndexArray(note_roll[seq], n);
                            n= -1;
                            //break;
                       }

                    }

                    //int cmd = MidiOutput::sequencer_cmd[seq_dev0];

                    note_roll_velocity[seq] = message->at(2);
                    if(!note_found)
                        note_roll[seq].append(the_note /*| ((cmd & 32) ? 128 : 0)*/);
                    int index = MidiOutput::sequencer_enabled[seq_dev0];
                    if(MidiPlayer::fileSequencer[seq] && index >= 0)
                        MidiOutput::sequencer_cmd[seq_dev0] = MidiPlayer::fileSequencer[seq]->getButtons(index)
                            | SEQ_FLAG_ON;

                    //MidiOutput::sequencer_cmd[seq_dev0] = /*SEQ_FLAG_INFINITE | SEQ_FLAG_LOOP |*/ SEQ_FLAG_ON;
                                                                                  //32 + 16 + 2;
                    //MidiInControl::set_key((int) message->at(1));
                }


                if((!autorhythm1 && MidiOutput::sequencer_enabled[seq_dev1] >= 0)
                   && ((is_keyboard2 && MidiInput::keyboard2_connected[pairdev * 2 + 1]) ||

                    ((MidiInControl::note_duo(pairdev) || the_note < MidiInControl::note_cut(pairdev))
                     && !is_keyboard2))) {

                    ret = true;

                    bool break_loop = true;
                    bool note_found = false;
                    int seq = seq_dev1;
                    for(int n = 0; n < note_roll[seq].length(); n++) {
                        if(!(note_roll[seq].at(n) & 128))
                            break_loop = false;

                        if((note_roll[seq].at(n) & 127) == the_note) {
                            note_roll[seq][n] = the_note;
                            note_found = true;
                        }
                    }

                    if(break_loop) {
                        note_found = false;
                        note_roll[seq].clear();
                    }

                           // release if max notes
                    if(note_roll[seq].length() >= 32) {
                        if(MidiPlayer::fileSequencer[seq])
                            MidiPlayer::fileSequencer[seq]->set_flush(note_roll[seq].at(0) & 127);
                        note_roll[seq] = note_roll[seq].right(31);
                    }

                           // delete repeat note
                    for(int n = 0; n < note_roll[seq].length(); n++) {
                        if((note_roll[seq].at(n) & 127) == (char) the_note) {
                            deleteIndexArray(note_roll[seq], n);
                            n= -1;
                            //break;
                        }

                    }


                    note_roll_velocity[seq] = message->at(2);
                    if(!note_found)
                        note_roll[seq].append(the_note);
                    int index = MidiOutput::sequencer_enabled[seq_dev1];
                    if(MidiPlayer::fileSequencer[seq] && index >= 0)
                        MidiOutput::sequencer_cmd[seq_dev1] = MidiPlayer::fileSequencer[seq]->getButtons(index) | SEQ_FLAG_ON;
                    //MidiOutput::sequencer_cmd[seq_dev1] = /*SEQ_FLAG_INFINITE | SEQ_FLAG_LOOP |*/ SEQ_FLAG_ON;
                }
            } else if(evt == 0x80) { // read key on

                int the_note = message->at(1);

                MidiInput::message[id - DEVICE_ID].clear();

                if((!autorhythm0 && MidiOutput::sequencer_enabled[seq_dev0] >= 0)  &&
                   (!is_keyboard2 &&
                    (MidiInput::keyboard2_connected[pairdev * 2 + 1] || MidiInControl::note_duo(pairdev)
                     || the_note >= MidiInControl::note_cut(pairdev)))) {

                    ret = true;

                    int seq = seq_dev0;

                    bool finded = false;

                    if(note_roll[seq].length()) {
                       QByteArray temp;
                       for(int n = 0; n < note_roll[seq].length(); n++) {

                           if(MidiOutput::sequencer_cmd[seq_dev0] & SEQ_FLAG_INFINITE) {
                               temp.append(note_roll[seq].at(n) | 128);
                              // note_roll[seq].replace(note_roll[seq].at(n), note_roll[seq].at(n) | 128);
                           } else if((note_roll[seq].at(n) & 127) == (char) the_note) {
                               //deleteIndexArray(note_roll[seq], n);

                               if(!(MidiOutput::sequencer_cmd[seq_dev0] & SEQ_FLAG_INFINITE)) {
                                    if(!finded) {
                                        if(MidiPlayer::fileSequencer[seq])
                                            MidiPlayer::fileSequencer[seq]->set_flush(the_note);
                                    }
                                    finded = true;
                               }
                               //break;
                           } else
                               temp.append(note_roll[seq].at(n));
                       }

                       note_roll[seq] = temp;
                    }

                    if(!finded && !(MidiOutput::sequencer_cmd[seq_dev0] & SEQ_FLAG_INFINITE))
                       for(int m = 0; m < 128; m++) {
                           if(map_key[ch][m] == (128 + the_note)) {
                               map_key[ch][m] = 0;
                               message->at(1) = m;
                               no_record = 2;
                               _messages->insert(_currentTime, *message);
                               if(_thru)
                                   send_thru(pairdev, is_effect,  message);
                           }
                       }

                }

                if((!autorhythm1 && MidiOutput::sequencer_enabled[seq_dev1] >= 0)
                   && ((is_keyboard2 && MidiInput::keyboard2_connected[pairdev * 2  + 1]) ||

                    ((MidiInControl::note_duo(pairdev) || the_note < MidiInControl::note_cut(pairdev))
                     && !is_keyboard2))) {

                    ret = true;

                    int seq = seq_dev1;
                    bool finded = false;

                    if(note_roll[seq].length()) {
                        QByteArray temp;
                        for(int n = 0; n < note_roll[seq].length(); n++) {
                            if(MidiOutput::sequencer_cmd[seq_dev1] & SEQ_FLAG_INFINITE) {
                                temp.append(note_roll[seq].at(n) | 128);
                                //note_roll[seq].replace(note_roll[seq].at(n), note_roll[seq].at(n) | 128);
                            } else if((note_roll[seq].at(n) & 127) == (char) the_note) {
                                //deleteIndexArray(note_roll[seq], n);

                                if(!finded) {
                                    if(MidiPlayer::fileSequencer[seq])
                                        MidiPlayer::fileSequencer[seq]->set_flush(the_note);
                                    finded = true;
                                }
                       
                            } else
                                temp.append(note_roll[seq].at(n));

                        }

                        note_roll[seq] = temp;

                    }

                    if(!finded)
                        for(int m = 0; m < 128; m++) {
                           if(map_key[ch][m] == (128 + the_note)) {
                               map_key[ch][m] = 0;
                               message->at(1) = m;  
                               _messages->insert(_currentTime, *message);
                               if(_thru)
                                   send_thru(pairdev, is_effect,  message);
                           }
                        }

                }

                if(!autorhythm0 && !note_roll[seq_dev0].length())
                    MidiOutput::sequencer_cmd[seq_dev0] = SEQ_FLAG_STOP;
                if(!autorhythm1 && !note_roll[seq_dev1].length())
                    MidiOutput::sequencer_cmd[seq_dev1] = SEQ_FLAG_STOP;

            }

            // test it
            if(!autorhythm0 && !note_roll[seq_dev0].length())
                MidiOutput::sequencer_cmd[seq_dev0] = SEQ_FLAG_STOP;
            if(!autorhythm1 && !note_roll[seq_dev1].length())
                MidiOutput::sequencer_cmd[seq_dev1] = SEQ_FLAG_STOP;

            if(ret)
                return;
        }

        // finger

        if(!MidiInControl::split_enable(pairdev)) {
            ignore_finger_token = true;
            if(is_finger_token == 1)
                return;
        }

        if(!(id >= DEVICE_SEQUENCER_ID  && id < (DEVICE_SEQUENCER_ID + 16))
                && !_note_finger_disabled) {

            // finger token in device 0/1
            if(!ignore_finger_token && !is_finger_token && (id >= DEVICE_ID && id < (DEVICE_ID + 15))) {

                if(evt == 0x80 || evt == 0x90) {

                    if(MidiInControl::finger_func(pairdev, message, is_keyboard2))
                        return;

                    is_finger_token = 2;

                    evt = message->at(0);

                    ch = evt & 0xF;

                    evt&= 0xF0;

                }

            }

            if(is_finger_token) {

                 note_finger = finger_token[0];
            }
        }

        // ignore DRUM channel...

        bool autorhythm = false;

        if((ch != 9  || (ch == 9 && !(id >= DEVICE_SEQUENCER_ID  && id < (DEVICE_SEQUENCER_ID + 16)))) && (!is_note ||  (is_note && ch != 9))
            && MidiInControl::split_enable(pairdev)) {

            if(evt == 0x80 || evt == 0x90) {

                int note = (int) (message->at(1));

                int velocity = (int) (message->at(2));
                int velocity2 = velocity;

                int input_chan = (int) (message->at(0) & 0xF);

                if(is_keyboard2) {

                    // qWarning("split");

                    if(ch != 9 && input_chan != MidiInControl::inchannelDown(pairdev) && is_finger_token != 2) {

                        return;
                    }

                    message->at(0) = (message->at(0) & 0xf0) | input_chan;

                }

                int note_master = note;

                int note_up = -1;

                int ch_up = ((MidiInControl::channelUp(pairdev) < 0)
                             ? MidiOutput::standardChannel()
                             : MidiInControl::channelUp(pairdev)) & 15;
                int ch_down = ((MidiInControl::channelDown(pairdev) < 0)
                               ? ((MidiInControl::channelUp(pairdev) < 0)
                                  ? MidiOutput::standardChannel()
                                  : MidiInControl::channelUp(pairdev))
                               : MidiInControl::channelDown(pairdev)) & 0xF;
                int note_down = -1;

                bool is_sequencer = false;


                if(id >= DEVICE_SEQUENCER_ID  && id < (DEVICE_SEQUENCER_ID + 16)) {
                    //pairdev = ((id - DEVICE_ID)/2);
                    //if(MidiOutput::sequencer_enabled[id - DEVICE_SEQUENCER_ID] < 0) return;
                    is_sequencer = true;
                    is_keyboard2 = false;

                    autorhythm = MidiPlayer::fileSequencer[(id - DEVICE_SEQUENCER_ID)] && MidiPlayer::fileSequencer[(id - DEVICE_SEQUENCER_ID)]->autorhythm;

                    if(autorhythm) {

                        //int ch = message->at(0);
                        ch_up = ch_down = 12;
                        //else ch_up = ch_down = 9;
                    }

                    if((id - DEVICE_SEQUENCER_ID) & 1) {
                        input_chan = ch_down;
                        message->at(0) = (message->at(0) & 0xf0) | ch_down;
                    } else {
                        input_chan = ch_up;
                        message->at(0) = (message->at(0) & 0xf0) | ch_up;
                    }
                }

                /********** down (sub keyboard) **********/

                int play_down = 0;
                int play_up = 0;

                if(note_finger < 0)
                    note_finger = note;
/*
                if(MidiOutput::sequencer_enabled[1] >= 0  && evt == 0x90 && id == DEVICE_SEQUENCER_ID + 1 && !note_roll[1].length()) {
                    return;
                }
*/
                if((is_sequencer || is_finger_token) && (is_keyboard2 || input_chan == ch_down)) {

                    play_down = 1;

                }

                if((is_sequencer || is_finger_token) && input_chan == ch_up && !is_keyboard2) {

                    if(is_sequencer)
                        play_down = 0;
                    play_up = 1;

                }

                if(!is_sequencer) {

                    if(is_keyboard2)
                        play_down = 1;
                    else if(!is_sequencer && MidiInControl::note_duo(pairdev) && !keyboard2_connected)
                        play_down = 1;
                    else if(input_chan == ch_down && ch_up == ch_down)
                        play_down = 0;
                } else {
                    if(input_chan == ch_down && ch_up == ch_down)
                        play_down = 0;
                }


                int finput = ((input_chan == MidiInControl::inchannelDown(pairdev)) || MidiInControl::inchannelDown(pairdev) == -1);

                if(is_sequencer)
                    finput = 0;

                if(!(id >= DEVICE_SEQUENCER_ID  && id < (DEVICE_SEQUENCER_ID + 16)) &&
                        !_note_finger_disabled && is_finger_token) finput = 0;

                if(evt == 0x90)
                    MidiInControl::key_flag = 0;

                if(is_keyboard2 || play_down || (finput && ((ch_up != ch_down) &&
                                                             ((MidiInControl::note_duo(pairdev) && !keyboard2_connected) ||
                                                              (is_keyboard2/* || (id == vkeyboard[pairdev + 1]) */||
                                                               (/*id != vkeyboard[pairdev] && */note < MidiInControl::note_cut(pairdev) &&
                                                                !keyboard2_connected)))))) {

                    bool use_seq_vol = false;

                    if(is_sequencer && MidiOutput::sequencer_enabled[seq_dev1] >= 0) {
                        use_seq_vol = true;
                        velocity = note_roll_velocity[seq_dev1];
                        if(autorhythm)
                            velocity = 127;

                    } //else
                        note_down =  note // transpose
                            + MidiInControl::transpose_note_down(pairdev);

                    if(evt == 0x90)
                        MidiInControl::key_flag = 1;

                    if(note_down < 0) note_down = 0;
                    if(note_down > 127) note_down = 127;

                    message->at(0) = evt | ch_down;

                    /*****/
                    int note_send = note_down, vel_send;

                    if(MidiInControl::fixVelDown(pairdev)) { // fix velocity
                        if(evt == 0x90 && velocity != 0)
                            vel_send = (MidiInControl::VelocityDOWN_enable[pairdev]) ? MidiInControl::VelocityDOWN_cut[pairdev] : 100;
                        else
                            vel_send = 0;

                    } else  {

                        vel_send = velocity;

                        if(MidiInControl::VelocityDOWN_enable[pairdev] && velocity != 0) {
                            vel_send += (vel_send * (MidiInControl::VelocityDOWN_scale[pairdev] % 10)/10);
                            if(vel_send < (127 * MidiInControl::VelocityDOWN_scale[pairdev]/100))
                                vel_send = (127 * MidiInControl::VelocityDOWN_scale[pairdev]/100);
                            if(vel_send > MidiInControl::VelocityDOWN_cut[pairdev])
                                vel_send =  MidiInControl::VelocityDOWN_cut[pairdev];
                        }
                    }

                    if(use_seq_vol) {
                        vel_send = vel_send * velocity2/ 127;
                    }

                    int vel_send2 = vel_send;

                    for(int n = 0; n < MidiInControl::autoChordfunDown(pairdev, AUTOCHORD_MAX, -1, -1); n++) { // auto chord
                    //U
                        MidiInControl::set_leds(pairdev, false, true);

                        note_send = MidiInControl::autoChordfunDown(pairdev, n, note_down, -1);
                        //if(n == 0) note_master = note_send;

                        message->at(1) = note_send;

                        if(evt == 0x90) {
                            vel_send  = MidiInControl::autoChordfunDown(pairdev, n, -1, vel_send2);
                            message->at(2) = vel_send;
                        }

                        //
                        no_record = 0;

                        if(evt == 0x80) {

                            int ch = ch_down;
                            int note = note_send;

                            /* this method to disable notes is used because notes
                               can be transposed or autochord notes on/off or change notes.
                            */

                            if(n != 0) no_record = 2; // skip all
                            else {
                                no_record = 2;
                                n = DELAYED_BREAK; // delayed break

                                if(map_key[ch][note] == 0) { // orphan note off (only play by _thru)
                                        no_record = 1;
                                }

                                for(int m = 0; m < 128; m++) {
                                    if(map_key[ch][m] == (128 + note_master)) {
                                        map_key[ch][m] = 0;
                                        message->at(1) = m;
                                        no_record = 2;
                                        _messages->insert(_currentTime, *message);
                                        if(_thru)
                                            send_thru(pairdev, is_effect,  message);
                                    }
                                }

                            }

                        } else if(evt == 0x90) {
                            int ch = ch_down;
                            int note = note_send;
                            no_record = 0;

                            if(map_key[ch][note] & 128) { // double note on

                                    message->at(0) = 0x80 | ch;
                                    if(map_key[ch][note] & 128)
                                    message->at(1) = note;
                                    message->at(2) = 0;

                                    _messages->insert((_currentTime) ? (_currentTime - 1) : 0, *message);
                                    if(_thru)
                                        send_thru(pairdev, is_effect,  message);
                                    message->at(0) =  0x90 | ch;
                                    message->at(1) = note_send;
                                    message->at(2) = velocity;

                                    map_key[ch][note] = 128 + note_master;


                            } else {
                                map_key[ch][note] = 128 + note_master;

                            }
                        }

                        if(!no_record) _messages->insert(_currentTime, *message);

                        if(_thru && no_record != 2)
                            send_thru(pairdev, is_effect,  message);

                        if(n == DELAYED_BREAK) break;
                    }// U
                }

                /********** up (sub keyboard) **********/

                finput = ((input_chan == MidiInControl::inchannelUp(pairdev)) || MidiInControl::inchannelUp(pairdev) == -1);

                if(!(id >= DEVICE_SEQUENCER_ID  && id < (DEVICE_SEQUENCER_ID + 16)) &&
                        !_note_finger_disabled && is_finger_token) finput = 0;

                // up
                if(!is_keyboard2 && (play_up || (finput && (MidiInControl::note_duo(pairdev) ||
                        ((ch_up == ch_down) && !MidiInControl::note_duo(pairdev))
                        || (keyboard2_connected ||
                            note >= MidiInControl::note_cut(pairdev)))))) {

                    bool use_seq_vol = false;

                    if(is_sequencer && MidiOutput::sequencer_enabled[seq_dev0] >= 0) {
                        use_seq_vol = true;
                        velocity = note_roll_velocity[seq_dev0];
                        if(autorhythm)
                            velocity = 127;

                    } //else

                        note_up = (int) (note) // transpose
                            + MidiInControl::transpose_note_up(pairdev);


                    if(evt == 0x90)
                        MidiInControl::key_flag = 2;

                    if(note_up < 0) note_up = 0;
                    if(note_up > 127) note_up = 127;

                    message->at(0) = evt | ch_up;

                    /*****/
                    int note_send = note_up, vel_send;

                    if(MidiInControl::fixVelUp(pairdev)) { // fix velocity
                        if(evt == 0x90 && velocity != 0)
                            vel_send = (MidiInControl::VelocityUP_enable[pairdev]) ? MidiInControl::VelocityUP_cut[pairdev] : 100;
                        else
                            vel_send = 0;

                    } else   {
                        vel_send = velocity;

                        if(MidiInControl::VelocityUP_enable[pairdev] && velocity != 0) {
                            vel_send += (vel_send * (MidiInControl::VelocityUP_scale[pairdev] % 10)/10);
                            if(vel_send < (127 * MidiInControl::VelocityUP_scale[pairdev]/100))
                                vel_send = (127 * MidiInControl::VelocityUP_scale[pairdev]/100);
                            if(vel_send > MidiInControl::VelocityUP_cut[pairdev])
                                vel_send =  MidiInControl::VelocityUP_cut[pairdev];
                        }
                    }

                    if(use_seq_vol) {
                        vel_send = vel_send * velocity2/ 127;
                    }

                    int vel_send2 = vel_send;

                    for(int n = 0; n < MidiInControl::autoChordfunUp(pairdev, AUTOCHORD_MAX, -1, -1); n++) { // auto chord
                    //U
                        MidiInControl::set_leds(pairdev, true, false);

                        note_send = MidiInControl::autoChordfunUp(pairdev, n, note_up, -1);
                        //if(n == 0) note_master = note_send;

                        message->at(1) = note_send;

                        if(evt == 0x90) {
                            vel_send  = MidiInControl::autoChordfunUp(pairdev, n, -1, vel_send2);
                            message->at(2) = vel_send;
                        }

                        //
                        no_record = 0;

                        if(evt == 0x80) {
                            int ch = ch_up;
                            int note = note_send;

                            /* this method to disable notes is used because notes
                               can be transposed or autochord notes on/off or change notes.
                            */

                            if(n != 0) no_record = 2; // skip all
                            else {
                                no_record = 2;
                                n = DELAYED_BREAK; // delayed break

                                if(map_key[ch][note] == 0) { // orphan note off (only play by _thru)
                                        no_record = 1;
                                }

                                for(int m = 0; m < 128; m++) {
                                    if(map_key[ch][m] == (128 + note_master)) {
                                        map_key[ch][m] = 0;
                                        message->at(1) = m;
                                        no_record = 2;
                                        _messages->insert(_currentTime, *message);
                                        if(_thru)
                                            send_thru(pairdev, is_effect,  message);
                                    }
                                }

                            }

                        } else if(evt == 0x90) {
                            int ch = ch_up;
                            int note = note_send;
                            no_record = 0;

                            if(map_key[ch][note] & 128) { // double note on

                                    message->at(0) = 0x80 | ch;
                                    if(map_key[ch][note] & 128)
                                    message->at(1) = note;
                                    message->at(2) = 0;

                                    _messages->insert((_currentTime) ? (_currentTime - 1) : 0, *message);
                                    if(_thru)
                                        send_thru(pairdev, is_effect,  message);
                                    message->at(0) =  0x90 | ch;
                                    message->at(1) = note_send;
                                    message->at(2) = velocity;

                                    map_key[ch][note] = 128 + note_master;

                            } else {

                                map_key[ch][note] = 128 + note_master;
                            }
                        }

                        if(!no_record) _messages->insert(_currentTime, *message);

                        if(_thru && no_record != 2)
                            send_thru(pairdev, is_effect,  message);

                        if(n == DELAYED_BREAK) break;
                    }// U
                    /*****************/
                }
                return;

            } else if(evt != 0xF0) { // end notes, others events... except 0xF0

                int ch_up = ((MidiInControl::channelUp(pairdev) < 0)
                                 ? MidiOutput::standardChannel()
                                 : (MidiInControl::channelUp(pairdev) & 15));

                int ch_down = ((MidiInControl::channelDown(pairdev) < 0)
                                   ? ((MidiInControl::channelUp(pairdev) < 0)
                                          ? MidiOutput::standardChannel()
                                          : MidiInControl::channelUp(pairdev))
                                   : MidiInControl::channelDown(pairdev)) & 0xF;


                if(evt == 0x00) { // unknown, surely part of data of a SysEx or broken event...

                    _messages->insert(_currentTime, *message);
                    if(_thru)
                        send_thru(pairdev, is_effect,  message);

                    return;

                } else {

                    bool is_kb2 = false;

                    if(MidiInput::keyboard2_connected[pairdev * 2 + 1] && ((id - DEVICE_ID) & 1)) {
                        is_kb2 = true;
                    }

                    if(is_kb2 && ch == MidiInControl::inchannelUp(pairdev))
                        message->at(0) = evt | ch_down;
                    else if(ch == MidiInControl::inchannelUp(pairdev))
                        message->at(0) = evt | ch_up;
                    else if(ch == MidiInControl::inchannelDown(pairdev))
                        message->at(0) = evt | ch_down;
                    else return; // ignore
                }

                if(evt == 0xe0 || (evt == 0xb0 && message->at(1) == 1))
                    is_effect = 1;

                if(MidiInControl::notes_only(pairdev) && !is_effect) return;

                    _messages->insert(_currentTime, *message);
                    if(_thru)
                        send_thru(pairdev, is_effect,  message);
                    return;

            } else if(evt == 0xf0){// skip 0xf0 messages
                if(MidiInControl::notes_only(pairdev) && !is_effect) return;
                _messages->insert(_currentTime, *message);
                if(_thru)
                    send_thru(pairdev, is_effect,  message);
                return;
            }
        } else if(ch == 9 && (id >= DEVICE_SEQUENCER_ID  && id < (DEVICE_SEQUENCER_ID + 16))) {

            if(seq_in >= 0) {
                ch = seq_in;
                message->at(0) = evt | ch;
            }

            if(evt == 0x80 || evt == 0x90) {

                if(evt == 0x90) { // read key on
                    int the_note = message->at(1);
                    no_record = 0;

                    if(map_key[ch][the_note] & 128) { // double note on

                        message->at(0) = 0x80 | ch;
                        if(map_key[ch][the_note] & 128)
                            message->at(1) = the_note;
                        message->at(2) = 0;

                        _messages->insert((_currentTime) ? (_currentTime - 1) : 0, *message);
                        if(_thru)
                            send_thru(pairdev, is_effect,  message);
                        message->at(0) =  0x90 | ch;
                        message->at(1) = the_note;


                        map_key[ch][the_note] = 128 + the_note;

                    } else {

                        map_key[ch][the_note] = 128 + the_note;
                    }
                } else if(evt == 0x80) {
                    int the_note = message->at(1);
                    if(map_key[ch][the_note] == 0) { // orphan note off (only play by _thru)
                        no_record = 1;
                    }

                    for(int m = 0; m < 128; m++) {
                        if(map_key[ch][m] == (128 + the_note)) {
                            map_key[ch][m] = 0;
                            message->at(1) = m;
                            no_record = 2;
                            _messages->insert(_currentTime, *message);
                            if(_thru)
                                send_thru(pairdev, is_effect,  message);
                        }
                    }

                }

                if(!no_record) _messages->insert(_currentTime, *message);

                if(_thru && no_record != 2)
                    send_thru(pairdev, is_effect,  message);

            }


        } else { // no split (direct)

            if(id >= DEVICE_SEQUENCER_ID  && id < (DEVICE_SEQUENCER_ID + 16)) {
                MidiOutput::sequencer_enabled[pairdev * 2] = -1;
                MidiOutput::sequencer_enabled[pairdev * 2 + 1] = -1;
                return;
            }

            int note = message->at(0);
            int chan = note & 15;
            note&= 0xf0;
#if 1


            if(note == 0x90) {
                int ch_up = ((MidiInControl::channelUp(pairdev) < 0)
                             ? MidiOutput::standardChannel()
                             : MidiInControl::channelUp(pairdev)) & 15;
                int ch_down = ((MidiInControl::channelDown(pairdev) < 0)
                               ? ((MidiInControl::channelUp(pairdev) < 0)
                                  ? MidiOutput::standardChannel()
                                  : MidiInControl::channelUp(pairdev))
                               : MidiInControl::channelDown(pairdev)) & 0xF;
                int velocity = message->at(2);

                if(chan == ch_up) {

                    if(MidiInControl::VelocityUP_enable[pairdev] && velocity != 0) {
                        velocity+= (velocity * (MidiInControl::VelocityUP_scale[pairdev] % 10)/10);
                        if(velocity < (127 * MidiInControl::VelocityUP_scale[pairdev]/100))
                            velocity = (127 * MidiInControl::VelocityUP_scale[pairdev]/100);
                    }

                    if(MidiInControl::VelocityUP_enable[pairdev] && velocity > MidiInControl::VelocityUP_cut[pairdev])
                       velocity =  MidiInControl::VelocityUP_cut[pairdev];
                    message->at(2) = velocity;


                } else if(chan == ch_down) {

                    if(MidiInControl::VelocityDOWN_enable[pairdev] && velocity != 0) {
                        velocity+= (velocity * (MidiInControl::VelocityDOWN_scale[pairdev] % 10)/10);
                        if(velocity < (127 * MidiInControl::VelocityDOWN_scale[pairdev]/100))
                            velocity = (127 * MidiInControl::VelocityDOWN_scale[pairdev]/100);
                    }

                    if(MidiInControl::VelocityDOWN_enable[pairdev] && velocity > MidiInControl::VelocityDOWN_cut[pairdev])
                       velocity =  MidiInControl::VelocityDOWN_cut[pairdev];
                    message->at(2) = velocity;

                }
            }
#endif
            _messages->insert(_currentTime, *message);
            if(_thru)
                send_thru(pairdev, is_effect,  message);
            return;
        }
    } // end messages

    // ignore messages truncated
    if(_thru)
        send_thru(pairdev, is_effect,  message);
}

void MidiInput::receiveMessage_mutex(double deltatime, std::vector<unsigned char>* message, void* userData)
{

    if(!file)
        return;

    mutex_input.lock();
    double delta_temp = deltatime;
    std::vector<unsigned char>* message_temp = message;
    void* userData_temp = userData;

    MidiInput::receiveMessage(delta_temp, message_temp, userData_temp);
    mutex_input.unlock();
}

void MidiInput::send_thru(int pairdev, int is_effect, std::vector<unsigned char>* message/*, int device*/)
{

    QByteArray a;
    //int channel = -1;

    std::vector<unsigned char> message_out = *message;


    /* no program and bank event from thru */

    int evt = message_out.at(0) & 0xF0;

    if(MidiInControl::skip_prgbanks(pairdev) // skip prg and bank changes
            && (evt == 0xc0 || (evt == 0xb0 && message_out.at(1) == 0))) return;

    for (int i = 0; i < (int) message_out.size(); i++) {

        // check channel
        if (i == 0) {
            switch (message_out.at(i) & 0xF0) {
                case 0x80: {
                    if(MidiInControl::split_enable(pairdev))
                        a.append(message_out.at(0));
                    else if(MidiInControl::skip_prgbanks(pairdev))
                        a.append(0x80 | MidiOutput::standardChannel());
                    else
                        a.append(message_out.at(0));
                    //channel = a[0] & 15;
                    continue;
                }
                case 0x90: {
                    if(MidiInControl::split_enable(pairdev))
                        a.append(message_out.at(0));
                    else if(MidiInControl::skip_prgbanks(pairdev))
                        a.append(0x90 | MidiOutput::standardChannel());
                    else
                        a.append(message_out.at(0));

                    MidiInControl::set_output_prog_bank_channel(
                                (MidiInControl::split_enable(pairdev))
                                ? (message_out.at(0) & 15)
                                : MidiOutput::standardChannel());

                    //channel = a[0] & 15;
                    continue;
                }
                case 0xD0: {
                    if(MidiInControl::skip_prgbanks(pairdev))
                        a.append(0xD0 | MidiOutput::standardChannel());
                    else
                        a.append(message_out.at(0));

                    //channel = a[0] & 15;
                    continue;
                }
                case 0xC0: {
                    if(MidiInControl::skip_prgbanks(pairdev))
                        a.append(0xC0 | MidiOutput::standardChannel());
                    else {
                        MidiInControl::set_prog(((int) message_out.at(0)) & 15,
                                                message_out.at(1) & 127);

                        a.append(message_out.at(0));
                    }

                    //channel = a[0] & 15;
                    continue;
                }
                case 0xB0: {
                    if(MidiInControl::skip_prgbanks(pairdev) && !is_effect)
                        a.append(0xB0 | MidiOutput::standardChannel());
                    else {

                        if(message_out.at(1) == 0) {// change bank

                            if(MidiInControl::skip_bankonly(pairdev))
                                message_out.at(2) = 0; // force bank to 0

                            MidiInControl::set_bank(((int) message_out.at(0)) & 15,
                                                    message_out.at(2) & 15);
                        }

                        a.append(message_out.at(0));
                    }
                    //channel = a[0] & 15;
                    continue;
                }
                case 0xA0: {
                    if(MidiInControl::skip_prgbanks(pairdev))
                        a.append(0xA0 | MidiOutput::standardChannel());
                    else
                        a.append(message_out.at(0));
                    //channel = a[0] & 15;
                    continue;
                }
                case 0xE0: {
                    if(MidiInControl::skip_prgbanks(pairdev) && !is_effect)
                        a.append(0xE0 | MidiOutput::standardChannel());
                    else
                        a.append(message_out.at(0));
                    //channel = a[0] & 15;
                    continue;
                }
            }
        }

        a.append(message_out.at(i));
    }
/*
    if(MidiInput::file && channel >= 0 && MidiInput::file->channel(channel)) {
        // skip channel muted
        if(MidiInput::file->channel(channel)->mute(track_index)) return;
    }
*/
    MidiOutput::sendCommand(a, track_index);
}

QStringList MidiInput::inputPorts(int index)
{

    QStringList ports;

    if(index < 0 || index >= MAX_INPUT_DEVICES || !_midiIn[index])
        return ports;

    // Check outputs.
    unsigned int nPorts = _midiIn[index]->getPortCount();

    for (unsigned int i = 0; i < nPorts; i++) {

        try {
            ports.append(QString::fromStdString(_midiIn[index]->getPortName(i)));
        } catch (RtMidiError&) {
        }
    }

    return ports;
}

bool MidiInput::setInputPort(QString name, int index)
{

    if(index < 0 || index >= MAX_INPUT_DEVICES || !_midiIn[index])
        return false;

    // try to find the port
    unsigned int nPorts = _midiIn[index]->getPortCount();

    cleanKeyMaps();

    bool ret = false;

    QStringList inputVariants;
    QString lname = name;
    if (lname.contains(':')) {
        lname = lname.section(':', 0, 0);
    }
    inputVariants.append(lname);
    if (lname.contains('(')) {
        lname = lname.section('(', 0, 0);
    }
    inputVariants.append(lname);

    int result = 0;
    int ind = 0;

    for (unsigned int i = 0; i < nPorts; i++) {
        QString dev = "";

        try {
            dev = QString::fromStdString(_midiIn[index]->getPortName(i));
        } catch (RtMidiError&) {
        }

        if(dev == "") continue;

        if (dev == name) {
            result = 1;
            ind = i;
            break;
        }

        foreach (QString portVariant, inputVariants) {

            if (dev.startsWith(portVariant)) {
                result = 2;
                ind = i;
                break;
            }

        }
    }

    try {

        keyboard2_connected[index] = false;

        // if the current port has the given name, select it and close
        // current port
        if (result) {

            _midiIn[index]->closePort();
            _midiIn[index]->openPort(ind);
            _inPort[index] = name;

            if(index & 1)
                keyboard2_connected[index] = true;
            ret = true;
        }

    } catch (RtMidiError&) {
    }

    if(ret) {
        if(MidiInput::loadSeq_mode == 2 || MidiInput::loadSeq_mode == 3) {
            MidiInControl::sequencer_load(index/2);
        }
    }

    return ret;

}

bool MidiInput::closeInputPort(int index)
{
    if(index < 0 || index >= MAX_INPUT_DEVICES)
        return false;

    if(!_midiIn[index])
        return false;

    keyboard2_connected[index] = false;

    try {
        _midiIn[index]->closePort();
    } catch (RtMidiError&) {

    }

    _inPort[index] = "";

    return true;
}

QString MidiInput::inputPort(int index)
{
    if(index < 0 || index >= MAX_INPUT_DEVICES)
        return "";

#ifdef HACK_INPUT
    if(index == 0)
        return "FAKE DEVICE";
#endif

    return _inPort[index];
}

void MidiInput::startInput()
{

    // clear eventlist
    _messages->clear();
    cleanKeyMaps();

    _recording = true;
}

QMultiMap<int, MidiEvent*> MidiInput::endInput(MidiTrack* track)
{

    QMultiMap<int, MidiEvent*> eventList;
    QByteArray array;

    QMultiMap<int, std::vector<unsigned char> >::iterator it = _messages->begin();

    bool ok = true;
    bool endEvent = false;

    _recording = false;

    QMultiMap<int, OffEvent*> emptyOffEvents;

    while (ok && it != _messages->end()) {

        array.clear();

        for (unsigned int i = 0; i < it.value().size(); i++) {
            array.append(it.value().at(i));
        }

        QDataStream tempStream(array);

        MidiEvent* event = MidiEvent::loadMidiEvent(&tempStream, &ok, &endEvent, track);
        OffEvent* off = dynamic_cast<OffEvent*>(event);
        if (off && !off->onEvent()) {
            emptyOffEvents.insert(it.key(), off);
            it++;
            continue;
        }
        if (ok) {
            eventList.insert(it.key(), event);
        }
        // if on event, check whether the off event has been loaded before.
        // this occurs when RTMidi fails to send the correct order
        OnEvent* on = dynamic_cast<OnEvent*>(event);
        if (on && emptyOffEvents.contains(it.key())) {
            QMultiMap<int, OffEvent*>::iterator emptyIt = emptyOffEvents.lowerBound(it.key());
            while (emptyIt != emptyOffEvents.end() && emptyIt.key() == it.key()) {
                if (emptyIt.value()->line() == on->line()) {
                    emptyIt.value()->setOnEvent(on);
                    OffEvent::removeOnEvent(on);
                    emptyOffEvents.remove(emptyIt.key(), emptyIt.value());
                    // add offEvent
                    eventList.insert(it.key() + 100, emptyIt.value());
                    break;
                }
                emptyIt++;
            }
        }
        it++;
    }
    QMultiMap<int, MidiEvent*>::iterator it2 = eventList.begin();
    while (it2 != eventList.end()) {
        OnEvent* on = dynamic_cast<OnEvent*>(it2.value());
        if (on && !on->offEvent()) {
            eventList.remove(it2.key(), it2.value());
        }
        it2++;
    }
    _messages->clear();

    _currentTime = 0;

    // perform consistency check
    QMultiMap<int, MidiEvent*> toRemove;
    QList<int> allTicks = toUnique(eventList.keys());

    foreach (int tick, allTicks) {

        int id = 0;
        QMultiMap<int, MidiEvent*> sortedByChannel;
        foreach (MidiEvent* event, eventList.values(tick)) {
            event->setTemporaryRecordID(id);
            sortedByChannel.insert(event->channel(), event);
            id++;
        }

        foreach (int channel, toUnique(sortedByChannel.keys())) {
            QMultiMap<int, MidiEvent*> sortedByLine;

            foreach (MidiEvent* event, sortedByChannel.values(channel)) {
                if ((event->line() == MidiEvent::CONTROLLER_LINE) || (event->line() == MidiEvent::PITCH_BEND_LINE) || (event->line() == MidiEvent::CHANNEL_PRESSURE_LINE) || (event->line() == MidiEvent::KEY_PRESSURE_LINE)) {
                    sortedByLine.insert(event->line(), event);
                }
            }

            foreach (int line, toUnique(sortedByLine.keys())) {

                // check for dublicates and mark all events which have been replaced
                // as toRemove
                if (sortedByLine.values(line).size() > 1) {

                    if (line == MidiEvent::CONTROLLER_LINE) {
                        QMap<int, MidiEvent*> byController;
                        foreach (MidiEvent* event, sortedByLine.values(line)) {
                            ControlChangeEvent* conv = dynamic_cast<ControlChangeEvent*>(event);
                            if (!conv) {
                                continue;
                            }
                            if (byController.contains(conv->control())) {
                                if (conv->temporaryRecordID() > byController[conv->control()]->temporaryRecordID()) {
                                    toRemove.insert(tick, byController[conv->control()]);
                                    byController[conv->control()] = conv;
                                } else {
                                    toRemove.insert(tick, conv);
                                }
                            } else {
                                byController.insert(conv->control(), conv);
                            }
                        }
                    } else if ((line == MidiEvent::PITCH_BEND_LINE) || (line == MidiEvent::CHANNEL_PRESSURE_LINE)) {

                        // search for maximum
                        MidiEvent* maxIdEvent = 0;

                        foreach (MidiEvent* ev, sortedByLine.values(line)) {
                            toRemove.insert(tick, ev);
                            if (!maxIdEvent || (maxIdEvent->temporaryRecordID() < ev->temporaryRecordID())) {
                                maxIdEvent = ev;
                            }
                        }

                        if (maxIdEvent) {
                            toRemove.remove(tick, maxIdEvent);
                        }

                    } else if (line == MidiEvent::KEY_PRESSURE_LINE) {
                        QMap<int, MidiEvent*> byNote;
                        foreach (MidiEvent* event, sortedByLine.values(line)) {
                            KeyPressureEvent* conv = dynamic_cast<KeyPressureEvent*>(event);
                            if (!conv) {
                                continue;
                            }
                            if (byNote.contains(conv->note())) {
                                if (conv->temporaryRecordID() > byNote[conv->note()]->temporaryRecordID()) {
                                    toRemove.insert(tick, byNote[conv->note()]);
                                    byNote[conv->note()] = conv;
                                } else {
                                    toRemove.insert(tick, conv);
                                }
                            } else {
                                byNote.insert(conv->note(), conv);
                            }
                        }
                    }
                }
            }
        }
    }

    if (toRemove.size() > 0) {
        QMultiMap<int, MidiEvent*>::iterator remIt = toRemove.begin();
        while (remIt != toRemove.end()) {
            eventList.remove(remIt.key(), remIt.value());
            remIt++;
        }
    }

    return eventList;
}

void MidiInput::setTime(int ms)
{
    _currentTime = ms;
}

bool MidiInput::recording()
{
    return _recording;
}

QList<int> MidiInput::toUnique(QList<int> in)
{
    QList<int> out;
    foreach (int i, in) {
        if ((out.size() == 0) || (out.last() != i)) {
            out.append(i);
        }
    }
    return out;
}

void MidiInput::setThruEnabled(bool b)
{
    _thru = b;
}

bool MidiInput::thru()
{
    return _thru;
}

bool MidiInput::isConnected(int index)
{
    for(int ind = 0; ind < MAX_INPUT_DEVICES; ind++) {
        if((ind != index && index >= 0) || !_midiIn[ind])
            continue;

        if(ind == virtual_device)
            return true;

        if(_inPort[ind] != "")
            return true;
    }

    return false;

}

void MidiInput::connectVirtual(int device) {
    if(device < DEVICE_ID || device >= (DEVICE_ID + MAX_INPUT_DEVICES))
        device = 0;
    else
        device -= DEVICE_ID;

    virtual_device = device;

    for(int n = 0; n < MAX_INPUT_DEVICES; n++)
        MidiInput::keyboard2_connected[n] = false;

    if(device & 1)
        MidiInput::keyboard2_connected[device] = true;
    else
        MidiInput::keyboard2_connected[device] = false;
    
    if(MidiInput::loadSeq_mode == 2 || MidiInput::loadSeq_mode == 3) {
        MidiInControl::sequencer_load(device/2);
    }
}
