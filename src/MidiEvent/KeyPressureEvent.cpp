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

#include "KeyPressureEvent.h"

KeyPressureEvent::KeyPressureEvent(int channel, int value, int note, MidiTrack* track)
    : MidiEvent(channel, track)
{
    _value = value;
    _note = note;
}

KeyPressureEvent::KeyPressureEvent(KeyPressureEvent& other)
    : MidiEvent(other)
{
    _value = other._value;
    _note = other._note;
}

int KeyPressureEvent::line()
{
    return KEY_PRESSURE_LINE;
}

QString KeyPressureEvent::toMessage()
{
    return "";
}

QByteArray KeyPressureEvent::save()
{
    QByteArray array = QByteArray();
    array.append(0xA0 | channel());
    array.append(_note);
    array.append(_value);
    return array;
}

ProtocolEntry* KeyPressureEvent::copy()
{
    return new KeyPressureEvent(*this);
}

void KeyPressureEvent::reloadState(ProtocolEntry* entry)
{
    KeyPressureEvent* other = dynamic_cast<KeyPressureEvent*>(entry);
    if (!other) {
        return;
    }
    MidiEvent::reloadState(entry);
    _value = other->_value;
    _note = other->_note;
}

void KeyPressureEvent::setValue(int v)
{
    ProtocolEntry* toCopy = copy();
    _value = v;
    protocol(toCopy, this);
}

void KeyPressureEvent::setNote(int n)
{
    ProtocolEntry* toCopy = copy();
    _note = n;
    protocol(toCopy, this);
}

QString KeyPressureEvent::typeString()
{
    return "Key Pressure Event";
}

int KeyPressureEvent::value()
{
    return _value;
}

int KeyPressureEvent::note()
{
    return _note;
}
