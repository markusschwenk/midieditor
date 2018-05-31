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

#include "PitchBendEvent.h"

#include "../midi/MidiFile.h"

PitchBendEvent::PitchBendEvent(int channel, int value, MidiTrack* track)
    : MidiEvent(channel, track)
{
    _value = value;
}

PitchBendEvent::PitchBendEvent(PitchBendEvent& other)
    : MidiEvent(other)
{
    _value = other._value;
}

int PitchBendEvent::line()
{
    return PITCH_BEND_LINE;
}

QString PitchBendEvent::toMessage()
{
    return "cc " + QString::number(channel()) + " " + QString::number(_value);
}

QByteArray PitchBendEvent::save()
{
    QByteArray array = QByteArray();
    array.append(0xE0 | channel());
    array.append(_value & 0x7F);
    array.append((_value >> 7) & 0x7F);
    return array;
}

ProtocolEntry* PitchBendEvent::copy()
{
    return new PitchBendEvent(*this);
}

void PitchBendEvent::reloadState(ProtocolEntry* entry)
{
    PitchBendEvent* other = dynamic_cast<PitchBendEvent*>(entry);
    if (!other) {
        return;
    }
    MidiEvent::reloadState(entry);
    _value = other->_value;
}

QString PitchBendEvent::typeString()
{
    return "Pitch Bend Event";
}

int PitchBendEvent::value()
{
    return _value;
}

void PitchBendEvent::setValue(int v)
{
    ProtocolEntry* toCopy = copy();
    _value = v;
    protocol(toCopy, this);
}

bool PitchBendEvent::isOnEvent()
{
    //	return (_control < 64 && _control> 69) || _value > 64;
    return false;
}
