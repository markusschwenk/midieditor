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
#include "../Terminal.h"

#include "../midi/MidiChannel.h"
#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/TextEvent.h"

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
#endif

int MidiOutput::is_playing = 1;
RtMidiOut* MidiOutput::_midiOut[MAX_OUTPUT_DEVICES]= {0, 0, 0};
int MidiOutput::_midiOutMAP[MAX_OUTPUT_DEVICES];
int MidiOutput::_midiOutFluidMAP[MAX_OUTPUT_DEVICES];
QString MidiOutput::_outPort[MAX_OUTPUT_DEVICES] = {"", "", ""};
MidiFile *MidiOutput::file = NULL;

SenderThread* MidiOutput::_sender = new SenderThread();
QMap<int, QList<int> > MidiOutput::playedNotes = QMap<int, QList<int> >();
bool MidiOutput::isAlternativePlayer = false;
bool MidiOutput::omitSysExLength = false;

int MidiOutput::_stdChannel = 0;

bool MidiOutput::AllTracksToOne = false;
bool MidiOutput::FluidSynthTracksAuto = false;
bool MidiOutput::SaveMidiOutDatas = false;

int MidiOutput::sequencer_cmd[16];
int MidiOutput::sequencer_enabled[16];

void MidiOutput::init()
{

    for(int n = 0; n < 16; n++) {
        sequencer_cmd[n] = 0;
        sequencer_enabled[n] = -1;
    }
    //sequencer_enabled[0] = 0;


    for(int n = 0; n < MAX_OUTPUT_DEVICES; n++) {
        _outPort[n] = "";
        _midiOutMAP[n] = -1;
        _midiOutFluidMAP[n] = -1;

        // RtMidiOut constructor
        try {
            _midiOut[n] = new RtMidiOut(RtMidi::UNSPECIFIED, QString("MidiEditor output" + QString::number(n)).toStdString());
        } catch (RtMidiError& error) {
            error.printMessage();
        }
    }
    _sender->start(QThread::TimeCriticalPriority);
}

void MidiOutput::update_config()
{
    if(AllTracksToOne) {
        FluidSynthTracksAuto = false;
    }
#ifdef USE_FLUIDSYNTH
    if(MidiOutput::FluidSynthTracksAuto) {

        for(int n = 0; n < MAX_OUTPUT_DEVICES; n++) {

            MidiOutput::setOutputPort(MidiOutput::GetFluidDevice(n), n);

        }
    }
#endif
}


void MidiOutput::forceDrum(bool force) {

#ifdef USE_FLUIDSYNTH
    if(fluid_output) {
        fluid_output->forceDrum(force);
        if(MidiOutput::file) {

            MidiOutput::file->DrumUseCh9 = force;
        }
        else
            fluidsynth_proc::_file->DrumUseCh9 = force;

        MidiPlayer::panic();
    }
#else
    if(MidiOutput::file)
        MidiOutput::file->DrumUseCh9 = force;
#endif
}

void MidiOutput::sendCommand(QByteArray array, int track_index)
{
    if(track_index > MAX_OUTPUT_DEVICES || track_index < 0)
        track_index = 0;

    if(AllTracksToOne)
        track_index = 0;

    if(is_playing && track_index == 0 && array[0] == (char)  0xf0) { //sysEX
        int ind = 1;
        while(array[ind] & 128) { // skip VLQ length

            ind++;
        }

        // fluidsynth sysEx
        if(!AllTracksToOne && array[ind + 2] == (char) 0x66 && array[ind + 3] == (char) 0x66) {
            track_index = -1;
#ifdef USE_FLUIDSYNTH
            for(int n = 0; n < MAX_OUTPUT_DEVICES; n++)

                if(_midiOutFluidMAP[n] >= 0) {

                    fluid_output->SendMIDIEvent(array, 0);
                    return;

                }
#endif
            if(track_index < 0) // skip fluidsynth sysEx
                return;
        }
    }

#ifdef USE_FLUIDSYNTH

    if(fluid_output && !fluid_output->disabled
            && (is_playing == 0 || _midiOutFluidMAP[track_index] >= 0)) {
        if(is_playing && !AllTracksToOne)
            track_index = _midiOutFluidMAP[track_index];
        fluid_output->SendMIDIEvent(array, track_index % 3);
    } else
#endif
        sendEnqueuedCommand(array, track_index);
}

void MidiOutput::sendCommandDelete(MidiEvent* e, int track_index) {
    sendCommand(e->save(), track_index);
    delete e;
}

void MidiOutput::sendCommand(MidiEvent* e, int track_index)
{
    if(track_index > MAX_OUTPUT_DEVICES || track_index < 0)
        track_index = 0;

    if(AllTracksToOne)
        track_index = 0;

    if ((e->channel() >= 0 && e->channel() < 16) || e->line() == MidiEvent::SYSEX_LINE) {
        QByteArray array = e->save();

        if(!AllTracksToOne && is_playing && track_index == 0 && array[0] == (char)  0xf0) { //sysEX

            int ind = 1;

            while(array[ind] & 128) { // skip VLQ length

                ind++;
            }

            // fluidsynth sysEx
            if(array[ind + 2] == (char) 0x66 && array[ind + 3] == (char) 0x66) {
               track_index = -1;
#ifdef USE_FLUIDSYNTH
               for(int n = 0; n < MAX_OUTPUT_DEVICES; n++)
                   if(_midiOutFluidMAP[n] >= 0) {

                       fluid_output->SendMIDIEvent(array, 0);
                       return;

                   }
#endif
               if(track_index < 0) // skip fluidsynth sysEx
                   return;
            }
        }
#ifdef USE_FLUIDSYNTH
        if(fluid_output && !fluid_output->disabled
                && (is_playing == 0 ||
                    _midiOutFluidMAP[track_index] >=0)) {

            if(is_playing && !AllTracksToOne)
                track_index = _midiOutFluidMAP[track_index];

            int type = array[0] & 0xf0;
            int chan = array[0] & 0xf;
            int index = chan + 4 * (track_index!=0) + 16 * track_index;
            if(type == 0xC0) file->Prog_MIDI[index] = array[1];
            if(type == 0xB0 && array[1]== (char) 0) file->Bank_MIDI[index]= array[2];
            fluid_output->SendMIDIEvent(array, track_index % 3);
            return;
        }
#endif
         {

            //sendEnqueuedCommand(e->save(), track_index);
            e->temp_track_index = track_index;
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
}

QStringList MidiOutput::outputPorts(int index)
{

    QStringList ports;

    if(index < 0 || index >= MAX_OUTPUT_DEVICES || !_midiOut[index])
        return ports;

    // Check outputs.
    unsigned int nPorts = _midiOut[index]->getPortCount();

    for (unsigned int i = 0; i < nPorts; i++) {

        try {
            ports.append(QString::fromStdString(_midiOut[index]->getPortName(i)));
        } catch (RtMidiError&) {
        }
    }

#ifdef USE_FLUIDSYNTH
    ports.append(QString::fromStdString(FLUID_SYNTH_NAME) + QString(+" chan (0-15)"));
    ports.append(QString::fromStdString(FLUID_SYNTH_NAME) + QString(+" chan (15-31)"));
    ports.append(QString::fromStdString(FLUID_SYNTH_NAME) + QString(+" chan (31-47)"));
#endif

    return ports;
}

bool MidiOutput::closeOutputPort(int index)
{
    if(index < 0 || index >= MAX_OUTPUT_DEVICES)
        return false;

    if(FluidDevice(_outPort[index]) >= 0) {
        if(!FluidSynthTracksAuto)
            _outPort[index] = "";
        _midiOutMAP[index] = -1;
        _midiOutFluidMAP[index] = -1;
    }

    if(!_midiOut[index])
        return false;

    try {
        _midiOut[index]->closePort();
    } catch (RtMidiError&) {

    }

    if(_midiOutMAP[index] == -666) { // find virtual and open to real device

        int ind = index;

        _midiOutMAP[index] = -1;
        _midiOutFluidMAP[index] = -1;
        for(int n = 0; n < MAX_OUTPUT_DEVICES; n++) {

            if(n == index)
                continue;

            if(_midiOutMAP[n] >= 0 && _midiOut[n] && _outPort[n] != "" && _outPort[n] == _outPort[index]) {

                QString name = _outPort[n];

                if(setOutputPort(name, n))
                    break;
                else { // error! close all virtual devices
                    for(int n = 0; n < MAX_OUTPUT_DEVICES; n++) {
                        if(_midiOutMAP[n] >= 0 && _midiOut[n] && _outPort[n] != "" && _outPort[n] == name) {
                            try {
                                _midiOut[n]->closePort();
                            } catch (RtMidiError&) {

                                _midiOutMAP[n] = -1;
                                if(!FluidSynthTracksAuto)
                                    _outPort[n] = "";
                            }
                        }
                    }

                    break;
                }

            }
        }
        if(!FluidSynthTracksAuto)
            _outPort[ind] = "";

    } else {
        if(!FluidSynthTracksAuto)
            _outPort[index] = "";
        _midiOutMAP[index] = -1;
        _midiOutFluidMAP[index] = -1;
    }

    return true;
}

bool MidiOutput::setOutputPort(QString name, int index)
{

    if(index < 0 || index >= MAX_OUTPUT_DEVICES || !_midiOut[index])
        return false;

#ifdef USE_FLUIDSYNTH
    if(FluidDevice(name) >= 0) {

        try {
            _midiOut[index]->closePort();
        } catch (RtMidiError&) {

        }

        if(!FluidSynthTracksAuto)
            _outPort[index] = name;

        _midiOutMAP[index] = -1;
        _midiOutFluidMAP[index] = FluidDevice(name);

        return true;
    }
#endif
    // try to find the port
    unsigned int nPorts = _midiOut[index]->getPortCount();

    for (unsigned int i = 0; i < nPorts; i++) {

        try {

            // if the current port has the given name, select it and close
            // current port
            if (_midiOut[index]->getPortName(i) == name.toStdString()) {

                _midiOut[index]->closePort();

                _midiOut[index]->openPort(i);

#ifdef USE_FLUIDSYNTH
                if(!FluidSynthTracksAuto)
#endif
                    _outPort[index] = name;

                _midiOutMAP[index] = -666;

                for(int n = 0; n < MAX_OUTPUT_DEVICES; n++) {
                    if(n == index)
                        continue;
                    if(_midiOut[n] && _outPort[n] != "" && _outPort[n] == name) {
                       _midiOutMAP[n] = index;

                    }
                }


                return true;
            }

        } catch (RtMidiError&) {
            for(int n = 0; n < MAX_OUTPUT_DEVICES; n++) {
                if(n == index)
                    continue;
                if(_midiOut[n] && _outPort[n] != "" && _outPort[n] == name) {
                   _midiOut[index]->closePort();
#ifdef USE_FLUIDSYNTH
                    if(!FluidSynthTracksAuto)
#endif
                        _outPort[index] = name;

                   _midiOutMAP[index] = n;

                   return true;
                }
            }
        }
    }

    // port not found
    return false;
}

QString MidiOutput::outputPort(int index, bool force)
{
    if(index < 0 || index >= MAX_OUTPUT_DEVICES)
        return "";
#ifdef USE_FLUIDSYNTH
    if(!force && FluidSynthTracksAuto)
        return GetFluidDevice(index);
#endif
    return _outPort[index];
}

void MidiOutput::sendEnqueuedCommand(QByteArray array, int track_index)
{
    if(track_index > MAX_OUTPUT_DEVICES || track_index < 0)
        track_index = 0;

    if(AllTracksToOne)
        track_index = 0;

    int type = array[0] & 0xf0;
    int chan = array[0] & 0xf;
    int index = chan + 4 * (track_index!=0) + 16 * track_index;
    if(type == 0xC0) file->Prog_MIDI[index] = array[1];
    if(type == 0xB0 && array[1]== (char) 0) file->Bank_MIDI[index]= array[2];

#ifdef USE_FLUIDSYNTH

    if(fluid_output && !fluid_output->disabled
            && (is_playing ==0 || _midiOutFluidMAP[track_index] >= 0)) {

        if(is_playing && !AllTracksToOne)
            track_index = _midiOutFluidMAP[track_index];
        fluid_output->SendMIDIEvent(array, track_index % 3);
        return;
    }
#endif

    // test if virtual device
    if(_midiOutFluidMAP[track_index] < 0 && _midiOutMAP[track_index] >= 0) { // is virtual
       track_index = _midiOutMAP[track_index]; // get real device
    }

    if (_midiOutFluidMAP[track_index] < 0 && _midiOut[track_index] &&
            _midiOutMAP[track_index] == -666) { // use real device

        // convert data to std::vector
        std::vector<unsigned char> message;

        if(array[0] == (char) 0xF0 && MidiOutput::omitSysExLength) { // sysEx without len
            int ind = 1;
            while(array[ind] & 128)
                ind++;
            array.remove(1, ind);
        }

        foreach (char byte, array) {
            message.push_back(byte);
        }
        try {
            _midiOut[track_index]->sendMessage(&message);
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

void MidiOutput::sendProgram(int channel, int prog, int track)
{
    QByteArray array = QByteArray();
    array.append(0xC0 | channel);
    array.append(prog);
    sendCommand(array, track);
}

bool MidiOutput::isConnected()
{

    //return FluidSynthTracksAuto || _outPort[0] != "" || _outPort[1] != "" || _outPort[2] != "";

    if(FluidSynthTracksAuto)
        return true;
    for(int n = 0; n < MAX_OUTPUT_DEVICES; n++) {
        if(_outPort[n] != "")
            return true;
    }

    return false;
}

bool MidiOutput::isFluidsynthConnected(int track_index)
{
    if(track_index < 0 || track_index >= MAX_OUTPUT_DEVICES)
        return false;
#ifdef USE_FLUIDSYNTH
    if(AllTracksToOne)
        track_index = 0;
    return _midiOutFluidMAP[track_index] >= 0;

#endif

    return false;
}

QString MidiOutput::GetFluidDevice(int index) {

#ifdef USE_FLUIDSYNTH

    index = index % 3;


    if(index == 0)
        return QString::fromStdString(FLUID_SYNTH_NAME) + QString(+" chan (0-15)");
    if(index == 1)
        return QString::fromStdString(FLUID_SYNTH_NAME) + QString(+" chan (15-31)");
    if(index == 2)
        return QString::fromStdString(FLUID_SYNTH_NAME) + QString(+" chan (31-47)");
#endif

    return "";
}

int MidiOutput::FluidDevice(QString name) {

#ifdef USE_FLUIDSYNTH
    if(!name.compare(QString::fromStdString(FLUID_SYNTH_NAME) + QString(+" chan (0-15)")))
        return 0;
    if(!name.compare(QString::fromStdString(FLUID_SYNTH_NAME) + QString(+" chan (15-31)")))
        return 1;
    if(!name.compare(QString::fromStdString(FLUID_SYNTH_NAME) + QString(+" chan (31-47)")))
        return 2;

    return -1;
#else
    return -1;
#endif
}

bool MidiOutput::loadTrackDevices(bool ask, QWidget *w)
{
    // Load Track Devices Info

    QString info;

    if(SaveMidiOutDatas) {

        foreach (MidiEvent* event, *(file->channelEvents(16))) {
            TextEvent* textInfo = dynamic_cast<TextEvent*>(event);
            if (textInfo && textInfo->channel() == 16 && textInfo->type() == TextEvent::COMMENT) {

                QString s = textInfo->text();
                s= s.mid(s.indexOf("Track_Devices"));
                if(!s.isEmpty()) {
                    info = textInfo->text();
                    break;
                }
            }
        }

        if(!info.isEmpty()) {

            if(ask && QMessageBox::question(w, "MidiEditor", "Track device information from MIDI present.\nDo you want to use it now?") != QMessageBox::Yes) {
                return false;
            }

            for(int n = 0; n < MAX_OUTPUT_DEVICES; n++) {
                closeOutputPort(n);
            }

            //qWarning("info\n%s", info.toLocal8Bit().constData());

            info = info.mid(info.indexOf("Track_Devices"));
            if(!info.isEmpty()) {
                QString s;
                info = info.mid(13);
                int i = info.indexOf("\n");
                s = info.left(i);
                info = info.mid(i + 1);

                info = info.mid(info.indexOf("AllTracksToOne: "));
                if(!info.isEmpty()) {
                    info = info.mid(16);
                    i = info.indexOf("\n");
                    s = info.left(i);
                    if(!s.compare("false"))
                        AllTracksToOne = false;
                    if(!s.compare("true"))
                        AllTracksToOne = true;

                    info = info.mid(i + 1);
                }

                info = info.mid(info.indexOf("FluidSynthTracksAuto: "));
                if(!info.isEmpty()) {
                    info = info.mid(22);
                    i = info.indexOf("\n");
                    s = info.left(i);
                    if(!s.compare("false"))
                        FluidSynthTracksAuto = false;
                    if(!s.compare("true"))
                        FluidSynthTracksAuto = true;

                    info = info.mid(i + 1);
                }

                info = info.mid(info.indexOf("FluidChan9Mode: "));
                if(!info.isEmpty()) {
                    info = info.mid(16);
                    i = info.indexOf("\n");
                    s = info.left(i);
                    if(!s.compare("Instrument"))
                        file->DrumUseCh9 = false;
                    else
                        file->DrumUseCh9 = true;

                    info = info.mid(i + 1);
                }

                for(int n = 0; n < MAX_OUTPUT_DEVICES; n++) {
                    closeOutputPort(n);
                    info = info.mid(info.indexOf(QString::asprintf("%2.2i: ", n)));
                    if(!info.isEmpty()) {
                        info = info.mid(4);
                        i = info.indexOf("\n");
                        s = info.left(i);
                        info = info.mid(i + 1);

                        Terminal::terminal()->setOutport(n, s);

                    } else
                        break;
                }
            }

        }

        update_config();
    }

    return true;
}

void MidiOutput::saveTrackDevices() {

    QString info;

    info.append("Track_Devices\n");
    if(AllTracksToOne)
        info.append("AllTracksToOne: true\n");
    else
        info.append("AllTracksToOne: false\n");
    if(FluidSynthTracksAuto)
        info.append("FluidSynthTracksAuto: true\n");
    else
        info.append("FluidSynthTracksAuto: false\n");

    if(file->DrumUseCh9)
        info.append("FluidChan9Mode: Drum\n");
    else
        info.append("FluidChan9Mode: Instrument\n");

    for(int n = 0; n < MAX_OUTPUT_DEVICES; n++) {
        info.append(QString::asprintf("%2.2i: ", n) + outputPort(n, true) +"\n");
    }
    // Save Track Devices Info

    if(SaveMidiOutDatas) {

        TextEvent *event = new TextEvent(16, file->track(0));
        event->setType(TextEvent::COMMENT);
        event->setText(info);

        file->channel(16)->insertEvent(event, 0);

        // delete old events
        foreach (MidiEvent* event2, *(file->eventsBetween(0, 10))) {
            TextEvent* toRemove = dynamic_cast<TextEvent*>(event2);
            if (toRemove && toRemove != event && toRemove->type() == TextEvent::COMMENT) {

                QString s = toRemove->text();
                s= s.mid(s.indexOf("Track_Devices"));
                if(!s.isEmpty())
                    file->channel(16)->removeEvent(toRemove);
            }
        }
    }

}

