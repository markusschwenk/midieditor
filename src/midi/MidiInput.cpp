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

RtMidiIn* MidiInput::_midiIn = 0;
QString MidiInput::_inPort = "";
QMultiMap<int, std::vector<unsigned char> >* MidiInput::_messages = new QMultiMap<int, std::vector<unsigned char> >;
int MidiInput::_currentTime = 0;
bool MidiInput::_recording = false;
bool MidiInput::_thru = false;

void MidiInput::init()
{

    // RtMidiIn constructor
    try {
         cleanKeyMaps();
        _midiIn = new RtMidiIn(RtMidi::UNSPECIFIED, QString("MidiEditor input").toStdString());
        //_midiIn->setQueueSizeLimit(65535);
        _midiIn->ignoreTypes(false, true, true);
        _midiIn->setCallback(&receiveMessage);
    } catch (RtMidiError& error) {
        error.printMessage();
    }
}

static unsigned char map_key[16][128];
static int pedal_chan = -1;

void MidiInput::cleanKeyMaps() {

    memset(map_key, 0, 16 * 128);
    pedal_chan = -1;
}

#define DELAYED_BREAK 1000

void MidiInput::receiveMessage(double /*deltatime*/, std::vector<unsigned char>* message, void* /*userData*/)
{

    int is_effect = 0;
    int no_record = 0;

    static int _is_recursive = 0;

    if (message->size() > 1) {
        int evt = message->at(0) & 0xF0;

        if(!_is_recursive) {
            if(evt == 0x90) { // read key on
                MidiInControl::set_key((int) message->at(1));
            }


            if(MidiInControl::key_effect()) { // apply effect event

                if((evt == 0xB0 && message->at(1) == 64)) { // test pedal
                    int chan = message->at(0) & 0xF;
                    if(pedal_chan == -1)
                        pedal_chan = chan;
                    else if(pedal_chan != chan)  // skip events from other chans
                            return;
                }

                is_effect = MidiInControl::set_effect(message);
                if(is_effect) {
                    evt = message->at(0) & 0xF0;
                    if(evt == 0) return; // skip
                }
            } else if(MidiInControl::split_enable()) {

                if((evt == 0xB0 && message->at(1) == 64)) { // test pedal
                    int chan = message->at(0) & 0xF;
                    if(MidiInControl::events_to_down()) {
                        if(chan == MidiInControl::inchannelDown())
                            pedal_chan = chan;
                        else if(MidiInControl::inchannelDown() == -1) {
                            if(pedal_chan == -1)
                                pedal_chan = chan;
                        }
                    } else {
                        if(chan == MidiInControl::inchannelUp())
                            pedal_chan = chan;
                        else if(MidiInControl::inchannelUp() == -1) {
                            if(pedal_chan == -1)
                                pedal_chan = chan;
                        }
                    }

                    if(pedal_chan != chan)  // skip events from other chans
                            return;
                }

            }

            if(MidiInControl::skip_prgbanks() // skip prg and bank changes
                    && (evt == 0xc0 || (evt == 0xb0 && message->at(1) == 0))) return;

            if(MidiInControl::skip_bankonly() && evt == 0xB0 // force bank to 0
                    && message->at(1) == 0) message->at(2) = 0;
        }


        if(MidiInControl::split_enable()) {

            if(evt == 0x80 || evt == 0x90) {

                int note = (int) (message->at(1));
                int velocity = (int) (message->at(2));
                int input_chan = (int) (message->at(0) & 0xF);

                int note_master = note;

                int note_up = -1;
                int ch_up = ((MidiInControl::channelUp() < 0)
                             ? MidiOutput::standardChannel()
                             : MidiInControl::channelUp()) & 15;
                int ch_down = ((MidiInControl::channelDown() < 0)
                               ? ((MidiInControl::channelUp() < 0)
                                  ? MidiOutput::standardChannel()
                                  : MidiInControl::channelUp())
                               : MidiInControl::channelDown()) & 0xF;
                int note_down = -1;

                //qWarning("tecla %x %i ch %i", evt, note, input_chan);

                /********** down (sub keyboard) **********/

                int finput = ((input_chan == MidiInControl::inchannelDown()) || MidiInControl::inchannelDown() == -1);
                if(finput && ((ch_up != ch_down) && (MidiInControl::note_duo() || note < MidiInControl::note_cut()))) {

                    note_down =  note // transpose
                            + MidiInControl::transpose_note_down();

                    if(note_down < 0) note_down = 0;
                    if(note_down > 127) note_down = 127;

                    message->at(0) = evt | ch_down;

                    /*****/
                    int note_send = note_down, vel_send;

                    if(MidiInControl::fixVelDown()) { // fix velocity
                        if(evt == 0x90 && velocity != 0)
                            vel_send = 100;
                        else
                            vel_send = 0;
                    } else  vel_send = velocity;



                    for(int n = 0; n < MidiInControl::autoChordfunDown(AUTOCHORD_MAX, -1, -1); n++) { // auto chord
                    //U
                        MidiInControl::set_leds(false, true);

                        note_send = MidiInControl::autoChordfunDown(n, note_down, -1);
                        //if(n == 0) note_master = note_send;

                        message->at(1) = note_send;

                        if(evt == 0x90) {
                            vel_send  = MidiInControl::autoChordfunDown(n, -1, vel_send);
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
                                            send_thru(is_effect,  message);
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
                                        send_thru(is_effect,  message);
                                    message->at(0) =  0x90 | ch;
                                    message->at(1) = note_send;
                                    message->at(2) = velocity;

                                    map_key[ch][note] = 128 + note_master;


                            } else
                                map_key[ch][note] = 128 + note_master;
                        }

                        if(!no_record) _messages->insert(_currentTime, *message);

                        if(_thru && no_record != 2)
                            send_thru(is_effect,  message);

                        if(n == DELAYED_BREAK) break;
                    }// U
                }

                /********** up (sub keyboard) **********/

                finput = ((input_chan == MidiInControl::inchannelUp()) || MidiInControl::inchannelUp() == -1);
                if(finput && (MidiInControl::note_duo() ||
                        ((ch_up == ch_down) && !MidiInControl::note_duo())
                        || note >= MidiInControl::note_cut())) {// up

                    note_up = (int) (note) // transpose
                            + MidiInControl::transpose_note_up();

                    if(note_up < 0) note_up = 0;
                    if(note_up > 127) note_up = 127;

                    message->at(0) = evt | ch_up;

                    /*****/
                    int note_send = note_up, vel_send;

                    if(MidiInControl::fixVelUp()) { // fix velocity
                        if(evt == 0x90 && velocity != 0)
                            vel_send = 100;
                        else
                            vel_send = 0;
                    } else  vel_send = velocity;

                    for(int n = 0; n < MidiInControl::autoChordfunUp(AUTOCHORD_MAX, -1, -1); n++) { // auto chord
                    //U
                        MidiInControl::set_leds(true, false);

                        note_send = MidiInControl::autoChordfunUp(n, note_up, -1);
                        //if(n == 0) note_master = note_send;

                        message->at(1) = note_send;

                        if(evt == 0x90) {
                            vel_send  = MidiInControl::autoChordfunUp(n, -1, vel_send);
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
                                            send_thru(is_effect,  message);
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
                                        send_thru(is_effect,  message);
                                    message->at(0) =  0x90 | ch;
                                    message->at(1) = note_send;
                                    message->at(2) = velocity;

                                    map_key[ch][note] = 128 + note_master;


                            } else
                                map_key[ch][note] = 128 + note_master;
                        }

                        if(!no_record) _messages->insert(_currentTime, *message);

                        if(_thru && no_record != 2)
                            send_thru(is_effect,  message);

                        if(n == DELAYED_BREAK) break;
                    }// U
                    /*****************/
                }
                return;

            } else if(evt != 0xF0) { // end notes, others events... except 0xF0
                int ch_up = ((MidiInControl::channelUp() < 0)
                             ? MidiOutput::standardChannel()
                             : (MidiInControl::channelUp() & 15));

                int ch_down = ((MidiInControl::channelDown() < 0)
                               ? ((MidiInControl::channelUp() < 0)
                                  ? MidiOutput::standardChannel()
                                  : (MidiInControl::channelUp() & 15))
                               : (MidiInControl::channelDown() & 15));

                if(evt == 0x00) { // unknown, surely part of data of a SysEx or broken event...

                    _messages->insert(_currentTime, *message);
                    if(_thru)
                        send_thru(is_effect,  message);

                    return;

                }

                message->at(0) = evt | ch_up;

                if(MidiInControl::events_to_down()) { // event to channel down

                    if(ch_up != ch_down && MidiInControl::note_duo()) {// and events to channel up

                        _messages->insert(_currentTime, *message);
                        if(_thru)
                            send_thru(is_effect,  message);
                    }

                    if(MidiInControl::notes_only() && !is_effect) return;

                    message->at(0) = evt | ch_down;

                    _messages->insert(_currentTime, *message);

                    if(_thru)
                        send_thru(is_effect,  message);
                    return;

                } else
                    if(MidiInControl::notes_only() && !is_effect) return;

                //qWarning("mess event 0x%x %i", evt, message->at(1));
                    _messages->insert(_currentTime, *message);
                    if(_thru)
                        send_thru(is_effect,  message);
                    return;

            } else if(evt == 0xf0){// skip 0xf0 messages
                if(MidiInControl::notes_only() && !is_effect) return;
                _messages->insert(_currentTime, *message);
                if(_thru)
                    send_thru(is_effect,  message);
                return;
            }
        } else { // no split (direct)

            _messages->insert(_currentTime, *message);
            if(_thru)
                send_thru(is_effect,  message);
            return;
        }
    } // end messages

    // ignore messages truncated
    if(_thru)
        send_thru(is_effect,  message);
}


void MidiInput::send_thru(int is_effect, std::vector<unsigned char>* message)
{

    QByteArray a;
    /* no program and bank event from thru */

    int evt = message->at(0) & 0xF0;

    if(MidiInControl::skip_prgbanks() // skip prg and bank changes
            && (evt == 0xc0 || (evt == 0xb0 && message->at(1) == 0))) return;

    for (int i = 0; i < (int) message->size(); i++) {
        // check channel
        if (i == 0) {
            switch (message->at(i) & 0xF0) {
            case 0x80: {
                if(MidiInControl::split_enable())
                    a.append(message->at(0));
                else if(MidiInControl::skip_prgbanks())
                    a.append(0x80 | MidiOutput::standardChannel());
                else
                    a.append(message->at(0));

                continue;
            }
            case 0x90: {
                if(MidiInControl::split_enable())
                    a.append(message->at(0));
                else if(MidiInControl::skip_prgbanks())
                    a.append(0x90 | MidiOutput::standardChannel());
                else
                    a.append(message->at(0));

                MidiInControl::set_output_prog_bank_channel(
                            (MidiInControl::split_enable())
                            ? (message->at(0) & 15)
                            : MidiOutput::standardChannel());

                continue;
            }
            case 0xD0: {
                if(MidiInControl::skip_prgbanks())
                    a.append(0xD0 | MidiOutput::standardChannel());
                else
                    a.append(message->at(0));

                continue;
            }
            case 0xC0: {
                if(MidiInControl::skip_prgbanks())
                    a.append(0xC0 | MidiOutput::standardChannel());
                else {
                    MidiInControl::set_prog(((int) message->at(0)) & 15,
                                            message->at(1) & 127);

                    a.append(message->at(0));
                }
                continue;
            }
            case 0xB0: {
                if(MidiInControl::skip_prgbanks() && !is_effect)
                    a.append(0xB0 | MidiOutput::standardChannel());
                else {

                    if(message->at(1) == 0) {// change bank
                        if(MidiInControl::skip_bankonly()) message->at(2) = 0; // force bank to 0
                        MidiInControl::set_bank(((int) message->at(0)) & 15,
                                                message->at(2) & 15);
                    }
                    a.append(message->at(0));
                }
                continue;
            }
            case 0xA0: {
                if(MidiInControl::skip_prgbanks())
                    a.append(0xA0 | MidiOutput::standardChannel());
                else
                    a.append(message->at(0));
                continue;
            }
            case 0xE0: {
                if(MidiInControl::skip_prgbanks() && !is_effect)
                    a.append(0xE0 | MidiOutput::standardChannel());
                else
                    a.append(message->at(0));

                continue;
            }
            }
        }
        a.append(message->at(i));
    }

    MidiOutput::sendCommand(a);
}

QStringList MidiInput::inputPorts()
{

    QStringList ports;

    // Check outputs.
    unsigned int nPorts = _midiIn->getPortCount();

    for (unsigned int i = 0; i < nPorts; i++) {

        try {
            ports.append(QString::fromStdString(_midiIn->getPortName(i)));
        } catch (RtMidiError&) {
        }
    }

    return ports;
}

bool MidiInput::setInputPort(QString name)
{

    // try to find the port
    unsigned int nPorts = _midiIn->getPortCount();

    cleanKeyMaps();

    for (unsigned int i = 0; i < nPorts; i++) {

        try {

            // if the current port has the given name, select it and close
            // current port
            if (_midiIn->getPortName(i) == name.toStdString()) {

                _midiIn->closePort();
                _midiIn->openPort(i);
                _inPort = name;
                return true;
            }

        } catch (RtMidiError&) {
        }
    }

    // port not found
    return false;
}

QString MidiInput::inputPort()
{
    return _inPort;
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

bool MidiInput::isConnected()
{
    return _inPort != "";
}
