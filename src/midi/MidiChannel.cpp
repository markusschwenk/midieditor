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

/**
 * \file midi/MidiChannel.cpp
 *
 * \brief Implements the class MidiChannel.
 */

#include "MidiChannel.h"

#include <QColor>

#include "../gui/Appearance.h"
#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/NoteOnEvent.h"
#include "../MidiEvent/OffEvent.h"
#include "../MidiEvent/ProgChangeEvent.h"
#include "../MidiEvent/ControlChangeEvent.h"
#include "../MidiEvent/TempoChangeEvent.h"
#include "../MidiEvent/TimeSignatureEvent.h"
#include "../gui/EventWidget.h"
#include "MidiFile.h"
#include "MidiTrack.h"

MidiChannel::MidiChannel(MidiFile* f, int num)
{

    _midiFile = f;
    _num = num;

    for(int n = 0; n < MAX_OUTPUT_DEVICES; n++) {
        _visible[n] = true;
        _mute[n] = false;
        _solo[n] = false;
    }
    midi_modified = false;

    _events = new QMultiMap<int, MidiEvent*>;
}

MidiChannel::MidiChannel(MidiChannel& other)
{
    _midiFile = other._midiFile;

    for(int n = 0; n < MAX_OUTPUT_DEVICES; n++) {
        _visible[n] = other._visible[n];
        _mute[n] = other._mute[n];
        _solo[n] = other._solo[n];
    }

    _events = new QMultiMap<int, MidiEvent*>(*(other._events));
    _num = other._num;
    //midi_modified = false;
    midi_modified = other.midi_modified;
}

ProtocolEntry* MidiChannel::copy()
{
    return new MidiChannel(*this);
}

void MidiChannel::reloadState(ProtocolEntry* entry)
{
    MidiChannel* other = dynamic_cast<MidiChannel*>(entry);
    if (!other) {
        return;
    }
    _midiFile = other->_midiFile;
    for(int n = 0; n < MAX_OUTPUT_DEVICES; n++) {
        _visible[n] = other->_visible[n];
        _mute[n] = other->_mute[n];
        _solo[n] = other->_solo[n];
    }
    _events = other->_events;
    _num = other->_num;
    midi_modified = other->midi_modified;
}

MidiFile* MidiChannel::file()
{
    return _midiFile;
}

bool MidiChannel::visible(int track_index)
{
    if (_num > 16) {
        return _midiFile->channel(16)->visible();
    }

    if (_num == 16)
        return _visible[0];

    if(track_index < 0 || track_index >= MAX_OUTPUT_DEVICES)
        track_index = 0;
    return _visible[track_index];
}

void MidiChannel::setVisible(bool b, int track_index)
{
    ProtocolEntry* toCopy = copy();

    if(track_index >= MAX_OUTPUT_DEVICES)
        track_index = 0;

    if(track_index < 0) {
        for(int n = 0; n < MAX_OUTPUT_DEVICES; n++)
            _visible[n] = b;
    } else
        _visible[track_index] = b;

    protocol(toCopy, this);
}

bool MidiChannel::mute(int track_index)
{
    if(track_index < 0 || track_index >= MAX_OUTPUT_DEVICES)
        track_index = 0;
    return _mute[track_index];
}

void MidiChannel::setMute(bool b, int track_index)
{
    ProtocolEntry* toCopy = copy();

    if(track_index >= MAX_OUTPUT_DEVICES)
        track_index = 0;

    if(track_index < 0) {
        for(int n = 0; n < MAX_OUTPUT_DEVICES; n++)
            _mute[n] = b;
    } else
        _mute[track_index] = b;

    protocol(toCopy, this);
}

bool MidiChannel::solo(int track_index)
{
    if(track_index < 0 || track_index >= MAX_OUTPUT_DEVICES)
        track_index = 0;
    return _solo[track_index];
}

void MidiChannel::setSolo(bool b, int track_index)
{
    ProtocolEntry* toCopy = copy();

    if(track_index >= MAX_OUTPUT_DEVICES)
        track_index = 0;

    if(track_index < 0) {
        for(int n = 0; n < MAX_OUTPUT_DEVICES; n++)
            _solo[n] = b;
    } else
        _solo[track_index] = b;

    protocol(toCopy, this);
}

int MidiChannel::number()
{
    return _num;
}

QMultiMap<int, MidiEvent*>* MidiChannel::eventMap()
{
    return _events;
}

QColor* MidiChannel::color()
{
    return Appearance::channelColor(number());
}

NoteOnEvent* MidiChannel::insertNote(int note, int startTick, int endTick, int velocity, MidiTrack* track)
{
    ProtocolEntry* toCopy = copy();
    NoteOnEvent* onEvent = new NoteOnEvent(note, velocity, number(), track);

    OffEvent* off = new OffEvent(number(), 127 - note, track);

    off->setFile(file());
    off->setMidiTime(endTick, false);
    onEvent->setFile(file());
    onEvent->setMidiTime(startTick, false);

    midi_modified = true;
    protocol(toCopy, this);

    return onEvent;
}

bool MidiChannel::removeEvent(MidiEvent* event)
{

    // if its once TimeSig / TempoChange at 0, dont delete event
    if (number() == 18 || number() == 17) {
        if ((event->midiTime() == 0) && (_events->count(0) == 1)) {
            return false;
        }
    }

    // remove from track if its the trackname
    if (number() == 16 && (MidiEvent*)(event->track()->nameEvent()) == event) {
        event->track()->setNameEvent(0);
    }

    ProtocolEntry* toCopy = copy();
    _events->remove(event->midiTime(), event);
    OnEvent* on = dynamic_cast<OnEvent*>(event);
    if (on && on->offEvent()) {
        _events->remove(on->offEvent()->midiTime(), on->offEvent());
    }

    midi_modified = true;
    protocol(toCopy, this);

    //if(MidiEvent::eventWidget()->events().contains(event)){
    //	MidiEvent::eventWidget()->removeEvent(event);
    //}
    return true;
}

void MidiChannel::insertEvent(MidiEvent* event, int tick)
{
    ProtocolEntry* toCopy = copy();
    event->setFile(file());
    event->setMidiTime(tick, false);
    midi_modified = true;
    protocol(toCopy, this);
}

void MidiChannel::deleteAllEvents()
{
    ProtocolEntry* toCopy = copy();
    _events->clear();
    midi_modified = true;
    protocol(toCopy, this);
}

int MidiChannel::progAtTick(int tick, MidiTrack * track)
{
    if(!track) {
        _midiFile->track(0);
    }

    // search for the last ProgChangeEvent in the channel
    QMultiMap<int, MidiEvent*>::iterator it = _events->upperBound(tick);
    if (it == _events->end()) {
        it--;
    }
    if (_events->size()) {
        while (it != _events->begin()) {
            if(!_midiFile->MultitrackMode || (_midiFile->MultitrackMode && it.value()->track() == track)) {
                ProgChangeEvent* ev = dynamic_cast<ProgChangeEvent*>(it.value());
                if (ev && it.key() <= tick) {
                    return ev->program();
                }
            }
            it--;
        }
    }

    // default: first
    foreach (MidiEvent* event, *_events) {
        if(!_midiFile->MultitrackMode || (_midiFile->MultitrackMode && event->track() == track)) {
            ProgChangeEvent* ev = dynamic_cast<ProgChangeEvent*>(event);
            if (ev) {
                return ev->program();
            }
        }
    }
    return 0;
}

int MidiChannel::progBankAtTick(int tick, int *bank, MidiTrack * track)
{
    int _bank= -1;

    if(!track) {
        _midiFile->track(0);
    }

    // search for the last ProgChangeEvent in the channel
    QMultiMap<int, MidiEvent*>::iterator it = _events->lowerBound(tick + 5);
    if (it == _events->end()) {
      it--;
    }


    ProgChangeEvent* ev2 = NULL;
    int default_prg = 0;

    if (_events->size()) {
        int fl = 0;

        while ((fl == 0 && it != _events->begin())
               || (fl == 1 && it == _events->begin())) {
            ProgChangeEvent* ev = dynamic_cast<ProgChangeEvent*>(it.value());
            if(!_midiFile->MultitrackMode || (_midiFile->MultitrackMode && it.value()->track() == track)) {
                if(ev && !ev2 && it.key() <= tick) ev2 = ev;

                ControlChangeEvent* ctrl = dynamic_cast<ControlChangeEvent*>(it.value());
                if (ctrl && ctrl->control()==0x0 && it.key() <= tick) {
                    _bank= ctrl->value();
                }
                if (ev2 && it.key() <= tick && _bank != -1) {
                    if(bank) *bank = _bank;
                    return ev2->program();
                }
            }
            it--;
            if(it == _events->begin()) fl = 1;
        }
    }

    ev2 = NULL;

    // default: first
    foreach (MidiEvent* event, *_events) {
        if(!_midiFile->MultitrackMode || (_midiFile->MultitrackMode && event->track() == track)) {
            ProgChangeEvent* ev = dynamic_cast<ProgChangeEvent*>(event);
            ControlChangeEvent* ctrl = dynamic_cast<ControlChangeEvent*>(event);
            if (ctrl && ctrl->control()==0x0) {
                _bank= ctrl->value();
            }

            if(ev) {
                if(!ev2) default_prg = ev->program();
                ev2 = ev;
            }

            if (ev2 && _bank != -1) {
                if(bank) *bank = _bank;
                return ev2->program();
            }
        }
    }

    if(_bank < 0) _bank = 0; // Default bank

    if(bank) *bank = _bank;
    return default_prg;
}

