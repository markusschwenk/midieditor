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
        _midiIn = new RtMidiIn(RtMidi::UNSPECIFIED, QString("MidiEditor input").toStdString());
        //_midiIn->setQueueSizeLimit(65535);
        _midiIn->ignoreTypes(false, true, true);
        _midiIn->setCallback(&receiveMessage);
    } catch (RtMidiError& error) {
        error.printMessage();
    }
}

void MidiInput::receiveMessage(double deltatime, std::vector<unsigned char>* message, void* userData)
{
    if (message->size() > 1) {
        _messages->insert(_currentTime, *message);
    }

    if (_thru) {
        QByteArray a;
        for (int i = 0; i < message->size(); i++) {
            // check channel
            if (i == 0) {
                switch (message->at(i) & 0xF0) {
                case 0x80: {
                    a.append(0x80 | MidiOutput::standardChannel());
                    continue;
                }
                case 0x90: {
                    a.append(0x90 | MidiOutput::standardChannel());
                    continue;
                }
                case 0xD0: {
                    a.append(0xD0 | MidiOutput::standardChannel());
                    continue;
                }
                case 0xC0: {
                    a.append(0xC0 | MidiOutput::standardChannel());
                    continue;
                }
                case 0xB0: {
                    a.append(0xB0 | MidiOutput::standardChannel());
                    continue;
                }
                case 0xA0: {
                    a.append(0xA0 | MidiOutput::standardChannel());
                    continue;
                }
                case 0xE0: {
                    a.append(0xE0 | MidiOutput::standardChannel());
                    continue;
                }
                }
            }
            a.append(message->at(i));
        }
        MidiOutput::sendCommand(a);
    }
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
