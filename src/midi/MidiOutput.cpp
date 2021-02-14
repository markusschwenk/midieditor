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

#include "MidiOutput.h"

#include "../MidiEvent/MidiEvent.h"

#include <QByteArray>
#include <QFile>
#include <QTextStream>

#include <vector>

#include "rtmidi/RtMidi.h"

#include "../MidiEvent/NoteOnEvent.h"
#include "../MidiEvent/OffEvent.h"
#include "SenderThread.h"

#ifdef USE_FLUIDSYNTH
#include "../fluid/fluidsynth_proc.h"
#else
extern int Bank_MIDI[17];
extern int Prog_MIDI[17];
#endif

RtMidiOut* MidiOutput::_midiOut = 0;
QString MidiOutput::_outPort = "";
SenderThread* MidiOutput::_sender = new SenderThread();
QMap<int, QList<int> > MidiOutput::playedNotes = QMap<int, QList<int> >();
bool MidiOutput::isAlternativePlayer = false;

int MidiOutput::_stdChannel = 0;

void MidiOutput::init()
{

    // RtMidiOut constructor
    try {
        _midiOut = new RtMidiOut(RtMidi::UNSPECIFIED, QString("MidiEditor output").toStdString());
    } catch (RtMidiError& error) {
        error.printMessage();
    }
    _sender->start(QThread::TimeCriticalPriority);
}

void MidiOutput::sendCommand(QByteArray array)
{
   #ifdef USE_FLUIDSYNTH
    if(fluid_output && !fluid_output->disabled && fluid_output->use_fluidsynt) fluid_output->SendMIDIEvent(array);
    else
   #endif
    sendEnqueuedCommand(array);
}

void MidiOutput::sendCommand2(MidiEvent* e) {
    sendCommand(e->save());
    delete e;
}
void MidiOutput::sendCommand(MidiEvent* e)
{

    if ((e->channel() >= 0 && e->channel() < 16) || e->line() == MidiEvent::SYSEX_LINE) {
        _sender->enqueue(e);

        if (isAlternativePlayer) {
            NoteOnEvent* n = dynamic_cast<NoteOnEvent*>(e);
            if (n && n->velocity() > 0) {
                playedNotes[n->channel()].append(n->note());
            } else if (n && n->velocity() == 0) {
                playedNotes[n->channel()].removeOne(n->note());
            } else {
                OffEvent* o = dynamic_cast<OffEvent*>(e);
                if (o) {
                    n = dynamic_cast<NoteOnEvent*>(o->onEvent());
                    if (n) {
                        playedNotes[n->channel()].removeOne(n->note());
                    }
                }
            }
        }
    }
}

QStringList MidiOutput::outputPorts()
{

    QStringList ports;

    // Check outputs.
    unsigned int nPorts = _midiOut->getPortCount();

    for (unsigned int i = 0; i < nPorts; i++) {

        try {
            ports.append(QString::fromStdString(_midiOut->getPortName(i)));
        } catch (RtMidiError&) {
        }
    }
    #ifdef USE_FLUIDSYNTH
        ports.append(QString::fromStdString(FLUID_SYNTH_NAME));
    #endif

    return ports;
}

bool MidiOutput::setOutputPort(QString name)
{

    // try to find the port
    unsigned int nPorts = _midiOut->getPortCount();

    for (unsigned int i = 0; i < nPorts; i++) {

        try {

            // if the current port has the given name, select it and close
            // current port
            if (_midiOut->getPortName(i) == name.toStdString()) {

                _midiOut->closePort();
                _midiOut->openPort(i);
                _outPort = name;
#ifdef USE_FLUIDSYNTH
                if(fluid_output) fluid_output->use_fluidsynt=false;
#endif
                return true;
            }

        } catch (RtMidiError&) {
        }
    }

#ifdef USE_FLUIDSYNTH
    if (FLUID_SYNTH_NAME == name.toStdString()) {
        if(fluid_output && !fluid_output->disabled) fluid_output->use_fluidsynt=true;
        _midiOut->closePort();
        _outPort = FLUID_SYNTH_NAME;
         return true;
    }
#endif

    // port not found
    return false;
}

QString MidiOutput::outputPort()
{
    return _outPort;
}

void MidiOutput::sendEnqueuedCommand(QByteArray array)
{
    int type = array[0] & 0xf0;
    int chan = array[0] & 0xf;
    if(type == 0xC0) Prog_MIDI[chan] = array[1];
    if(type == 0xB0 && array[1]== (char) 0) Bank_MIDI[chan]= array[2];

#ifdef USE_FLUIDSYNTH

    if(fluid_output && !fluid_output->disabled && fluid_output->use_fluidsynt) {
        fluid_output->SendMIDIEvent(array);
        return;
    }
#endif
    if (_outPort != "") {

        // convert data to std::vector
        std::vector<unsigned char> message;

        foreach (char byte, array) {
            message.push_back(byte);
        }
        try {
            _midiOut->sendMessage(&message);
        } catch (RtMidiError& error) {
            error.printMessage();
        }
    }
}

void MidiOutput::setStandardChannel(int channel)
{
    _stdChannel = channel;
}

int MidiOutput::standardChannel()
{
    return _stdChannel;
}

void MidiOutput::sendProgram(int channel, int prog)
{
    QByteArray array = QByteArray();
    array.append(0xC0 | channel);
    array.append(prog);
    sendCommand(array);
}

bool MidiOutput::isConnected()
{
#ifdef USE_FLUIDSYNTH
    if (_outPort == "") {
        if(fluid_output && !fluid_output->disabled) fluid_output->use_fluidsynt=true;
        _outPort = FLUID_SYNTH_NAME;
         return true;
    }
#endif
    return _outPort != "";
}
