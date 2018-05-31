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

#include "KeySignatureEvent.h"

KeySignatureEvent::KeySignatureEvent(int channel, int tonality, bool minor, MidiTrack* track)
    : MidiEvent(channel, track)
{
    _tonality = tonality;
    _minor = minor;
}

KeySignatureEvent::KeySignatureEvent(KeySignatureEvent& other)
    : MidiEvent(other)
{
    _tonality = other._tonality;
    _minor = other._minor;
}

int KeySignatureEvent::line()
{
    return KEY_SIGNATURE_EVENT_LINE;
}

QString KeySignatureEvent::toMessage()
{
    return "";
}

QByteArray KeySignatureEvent::save()
{
    QByteArray array = QByteArray();
    array.append(char(0xFF));
    array.append(0x59 | channel());
    array.append(0x02);
    array.append(tonality());
    if (_minor) {
        array.append(0x01);
    } else {
        array.append(char(0x00));
    }
    return array;
}

ProtocolEntry* KeySignatureEvent::copy()
{
    return new KeySignatureEvent(*this);
}

void KeySignatureEvent::reloadState(ProtocolEntry* entry)
{
    KeySignatureEvent* other = dynamic_cast<KeySignatureEvent*>(entry);
    if (!other) {
        return;
    }
    MidiEvent::reloadState(entry);
    _tonality = other->_tonality;
    _minor = other->_minor;
}

QString KeySignatureEvent::typeString()
{
    return "Key Signature Event";
}

int KeySignatureEvent::tonality()
{
    return _tonality;
}

bool KeySignatureEvent::minor()
{
    return _minor;
}

void KeySignatureEvent::setTonality(int t)
{
    ProtocolEntry* toCopy = copy();
    _tonality = t;
    protocol(toCopy, this);
}

void KeySignatureEvent::setMinor(bool minor)
{
    ProtocolEntry* toCopy = copy();
    _minor = minor;
    protocol(toCopy, this);
}

QString KeySignatureEvent::toString(int tonality, bool minor)
{

    QString text = "";

    if (!minor) {
        switch (tonality) {
        case 0: {
            text = "C";
            break;
        }
        case 1: {
            text = "G";
            break;
        }
        case 2: {
            text = "D";
            break;
        }
        case 3: {
            text = "A";
            break;
        }
        case 4: {
            text = "E";
            break;
        }
        case 5: {
            text = "B";
            break;
        }
        case 6: {
            text = "F sharp";
            break;
        }
        case -1: {
            text = "F";
            break;
        }
        case -2: {
            text = "B flat";
            break;
        }
        case -3: {
            text = "E flat";
            break;
        }
        case -4: {
            text = "A flat";
            break;
        }
        case -5: {
            text = "D flat";
            break;
        }
        case -6: {
            text = "G flat";
            break;
        }
        }
        text += " major";
    } else {
        switch (tonality) {
        case 0: {
            text = "a";
            break;
        }
        case 1: {
            text = "e";
            break;
        }
        case 2: {
            text = "b";
            break;
        }
        case 3: {
            text = "f sharp";
            break;
        }
        case 4: {
            text = "c sharp";
            break;
        }
        case 5: {
            text = "g sharp";
            break;
        }
        case 6: {
            text = "d sharp";
            break;
        }
        case -1: {
            text = "d";
            break;
        }
        case -2: {
            text = "g";
            break;
        }
        case -3: {
            text = "c";
            break;
        }
        case -4: {
            text = "f";
            break;
        }
        case -5: {
            text = "b flat";
            break;
        }
        case -6: {
            text = "e flat";
            break;
        }
        }
        text += " minor";
    }
    return text;
}
