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
#include "../MidiEvent/TempoChangeEvent.h"
#include "../MidiEvent/TimeSignatureEvent.h"
#include "../gui/EventWidget.h"
#include "MidiFile.h"
#include "MidiTrack.h"

MidiChannel::MidiChannel(MidiFile* f, int num)
{

    _midiFile = f;
    _num = num;

    _visible = true;
    _mute = false;
    _solo = false;

    _events = new QMultiMap<int, MidiEvent*>;
}

MidiChannel::MidiChannel(MidiChannel& other)
{
    _midiFile = other._midiFile;
    _visible = other._visible;
    _mute = other._mute;
    _solo = other._solo;
    _events = new QMultiMap<int, MidiEvent*>(*(other._events));
    _num = other._num;
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
    _visible = other->_visible;
    _mute = other->_mute;
    _solo = other->_solo;
    _events = other->_events;
    _num = other->_num;
}

MidiFile* MidiChannel::file()
{
    return _midiFile;
}

bool MidiChannel::visible()
{
    if (_num > 16) {
        return _midiFile->channel(16)->visible();
    }
    return _visible;
}

void MidiChannel::setVisible(bool b)
{
    ProtocolEntry* toCopy = copy();
    _visible = b;
    protocol(toCopy, this);
}

bool MidiChannel::mute()
{
    return _mute;
}

void MidiChannel::setMute(bool b)
{
    ProtocolEntry* toCopy = copy();
    _mute = b;
    protocol(toCopy, this);
}

bool MidiChannel::solo()
{
    return _solo;
}

void MidiChannel::setSolo(bool b)
{
    ProtocolEntry* toCopy = copy();
    _solo = b;
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
    protocol(toCopy, this);
}

void MidiChannel::deleteAllEvents()
{
    ProtocolEntry* toCopy = copy();
    _events->clear();
    protocol(toCopy, this);
}

int MidiChannel::progAtTick(int tick)
{

    // search for the last ProgChangeEvent in the channel
    QMultiMap<int, MidiEvent*>::iterator it = _events->upperBound(tick);
    if (it == _events->end()) {
        it--;
    }
    if (_events->size()) {
        while (it != _events->begin()) {
            ProgChangeEvent* ev = dynamic_cast<ProgChangeEvent*>(it.value());
            if (ev && it.key() <= tick) {
                return ev->program();
            }
            it--;
        }
    }

    // default: first
    foreach (MidiEvent* event, *_events) {
        ProgChangeEvent* ev = dynamic_cast<ProgChangeEvent*>(event);
        if (ev) {
            return ev->program();
        }
    }
    return 0;
}
