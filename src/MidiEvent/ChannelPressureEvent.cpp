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

#include "ChannelPressureEvent.h"

ChannelPressureEvent::ChannelPressureEvent(int channel, int value, MidiTrack* track)
    : MidiEvent(channel, track)
{
    _value = value;
}

ChannelPressureEvent::ChannelPressureEvent(ChannelPressureEvent& other)
    : MidiEvent(other)
{
    _value = other._value;
}

int ChannelPressureEvent::line()
{
    return CHANNEL_PRESSURE_LINE;
}

QString ChannelPressureEvent::toMessage()
{
    return "";
}

QByteArray ChannelPressureEvent::save()
{
    QByteArray array = QByteArray();
    array.append(0xD0 | channel());
    array.append(_value);
    return array;
}

ProtocolEntry* ChannelPressureEvent::copy()
{
    return new ChannelPressureEvent(*this);
}

void ChannelPressureEvent::reloadState(ProtocolEntry* entry)
{
    ChannelPressureEvent* other = dynamic_cast<ChannelPressureEvent*>(entry);
    if (!other) {
        return;
    }
    MidiEvent::reloadState(entry);
    _value = other->_value;
}

QString ChannelPressureEvent::typeString()
{
    return "Channel Pressure Event";
}

void ChannelPressureEvent::setValue(int v)
{
    ProtocolEntry* toCopy = copy();
    _value = v;
    protocol(toCopy, this);
}

int ChannelPressureEvent::value()
{
    return _value;
}
