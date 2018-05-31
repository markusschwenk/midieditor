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

#include "ControlChangeEvent.h"

#include "../midi/MidiFile.h"

ControlChangeEvent::ControlChangeEvent(int channel, int control, int value, MidiTrack* track)
    : MidiEvent(channel, track)
{
    _control = control;
    _value = value;
}

ControlChangeEvent::ControlChangeEvent(ControlChangeEvent& other)
    : MidiEvent(other)
{
    _value = other._value;
    _control = other._control;
}

int ControlChangeEvent::line()
{
    return CONTROLLER_LINE;
}

QString ControlChangeEvent::toMessage()
{
    return "cc " + QString::number(channel()) + " " + QString::number(_control) + " " + QString::number(_value);
}

QByteArray ControlChangeEvent::save()
{
    QByteArray array = QByteArray();
    array.append(0xB0 | channel());
    array.append(_control);
    array.append(_value);
    return array;
}

ProtocolEntry* ControlChangeEvent::copy()
{
    return new ControlChangeEvent(*this);
}

void ControlChangeEvent::reloadState(ProtocolEntry* entry)
{
    ControlChangeEvent* other = dynamic_cast<ControlChangeEvent*>(entry);
    if (!other) {
        return;
    }
    MidiEvent::reloadState(entry);
    _control = other->_control;
    _value = other->_value;
}

QString ControlChangeEvent::typeString()
{
    return "Control Change Event";
}

int ControlChangeEvent::value()
{
    return _value;
}

int ControlChangeEvent::control()
{
    return _control;
}

void ControlChangeEvent::setValue(int v)
{
    ProtocolEntry* toCopy = copy();
    _value = v;
    protocol(toCopy, this);
}

void ControlChangeEvent::setControl(int c)
{
    ProtocolEntry* toCopy = copy();
    _control = c;
    protocol(toCopy, this);
}

bool ControlChangeEvent::isOnEvent()
{
    return false;
}
