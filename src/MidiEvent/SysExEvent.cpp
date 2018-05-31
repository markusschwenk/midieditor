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

#include "SysExEvent.h"

SysExEvent::SysExEvent(int channel, QByteArray data, MidiTrack* track)
    : MidiEvent(channel, track)
{
    _data = data;
}

SysExEvent::SysExEvent(SysExEvent& other)
    : MidiEvent(other)
{
    _data = other._data;
}

QByteArray SysExEvent::data()
{
    return _data;
}

int SysExEvent::line()
{
    return SYSEX_LINE;
}

QByteArray SysExEvent::save()
{
    QByteArray s;
    s.append(char(0xF0));
    s.append(_data);
    s.append(char(0xF7));
    return s;
}

QString SysExEvent::typeString()
{
    return "System Exclusive Message (SysEx)";
}

ProtocolEntry* SysExEvent::copy()
{
    return new SysExEvent(*this);
}

void SysExEvent::reloadState(ProtocolEntry* entry)
{
    SysExEvent* other = dynamic_cast<SysExEvent*>(entry);
    if (!other) {
        return;
    }
    MidiEvent::reloadState(entry);
    _data = other->_data;
}

void SysExEvent::setData(QByteArray d)
{
    ProtocolEntry* toCopy = copy();
    _data = d;
    protocol(toCopy, this);
}
