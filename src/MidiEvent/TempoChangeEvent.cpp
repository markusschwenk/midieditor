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

#include "TempoChangeEvent.h"
#include "../midi/MidiFile.h"

TempoChangeEvent::TempoChangeEvent(int channel, int value, MidiTrack* track)
    : MidiEvent(channel, track)
{
    _beats = 60000000 / value;
}

TempoChangeEvent::TempoChangeEvent(TempoChangeEvent& other)
    : MidiEvent(other)
{
    _beats = other._beats;
}

int TempoChangeEvent::beatsPerQuarter()
{
    return _beats;
}

double TempoChangeEvent::msPerTick()
{
    double quarters_per_second = (double)_beats / 60;
    double ticks_per_second = (double)(file()->ticksPerQuarter()) * quarters_per_second;
    return 1000 / (ticks_per_second);
}

ProtocolEntry* TempoChangeEvent::copy()
{
    return new TempoChangeEvent(*this);
}

void TempoChangeEvent::reloadState(ProtocolEntry* entry)
{
    TempoChangeEvent* other = dynamic_cast<TempoChangeEvent*>(entry);
    if (!other) {
        return;
    }
    MidiEvent::reloadState(entry);
    _beats = other->_beats;
}

int TempoChangeEvent::line()
{
    return MidiEvent::TEMPO_CHANGE_EVENT_LINE;
}

QByteArray TempoChangeEvent::save()
{
    QByteArray array = QByteArray();

    array.append(char(0xFF));
    array.append(char(0x51));
    array.append(char(0x03));
    int value = 60000000 / _beats;
    for (int i = 2; i >= 0; i--) {
        array.append((value & (0xFF << 8 * i)) >> 8 * i);
    }

    return array;
}

void TempoChangeEvent::setBeats(int beats)
{
    ProtocolEntry* toCopy = copy();
    _beats = beats;
    file()->calcMaxTime();
    protocol(toCopy, this);
}

QString TempoChangeEvent::typeString()
{
    return "Tempo Change Event";
}
