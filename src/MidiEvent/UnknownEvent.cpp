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

#include "UnknownEvent.h"

#include "../midi/MidiFile.h"

UnknownEvent::UnknownEvent(int channel, int type, QByteArray data, MidiTrack* track)
    : MidiEvent(channel, track)
{
    _data = data;
    _type = type;
}

UnknownEvent::UnknownEvent(UnknownEvent& other)
    : MidiEvent(other)
{
    _data = other._data;
    _type = other._type;
}

QByteArray UnknownEvent::data()
{
    return _data;
}

int UnknownEvent::line()
{
    return UNKNOWN_LINE;
}

QByteArray UnknownEvent::save()
{
    QByteArray s;
    s.append(char(0xFF));
    s.append(_type);
    s.append(MidiFile::writeVariableLengthValue(_data.length()));
    s.append(_data);
    return s;
}

void UnknownEvent::reloadState(ProtocolEntry* entry)
{
    UnknownEvent* other = dynamic_cast<UnknownEvent*>(entry);
    if (!other) {
        return;
    }
    MidiEvent::reloadState(entry);
    _type = other->_type;
    _data = other->_data;
}

ProtocolEntry* UnknownEvent::copy()
{
    return new UnknownEvent(*this);
}

int UnknownEvent::type()
{
    return _type;
}

void UnknownEvent::setType(int type)
{
    ProtocolEntry* toCopy = copy();
    _type = type;
    protocol(toCopy, this);
}

void UnknownEvent::setData(QByteArray d)
{
    ProtocolEntry* toCopy = copy();
    _data = d;
    protocol(toCopy, this);
}
