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

#include "TextEvent.h"

#include "../midi/MidiFile.h"
#include "../midi/MidiTrack.h"

TextEvent::TextEvent(int channel, MidiTrack* track)
    : MidiEvent(channel, track)
{
    _type = TEXT;
    _text = "";
}

TextEvent::TextEvent(TextEvent& other)
    : MidiEvent(other)
{
    _type = other._type;
    _text = other._text;
}

QString TextEvent::text()
{
    return _text;
}

void TextEvent::setText(QString text)
{
    ProtocolEntry* toCopy = copy();
    _text = text;
    protocol(toCopy, this);
}

int TextEvent::type()
{
    return _type;
}

void TextEvent::setType(int type)
{
    ProtocolEntry* toCopy = copy();
    _type = type;
    protocol(toCopy, this);
}

int TextEvent::line()
{
    return TEXT_EVENT_LINE;
}

QByteArray TextEvent::save()
{
    QByteArray array = QByteArray();
    QByteArray utf8text = _text.toUtf8();

    array.append(char(0xFF));
    array.append(_type);
    array.append(MidiFile::writeVariableLengthValue(utf8text.size()));
    array.append(utf8text);
    return array;
}

QString TextEvent::typeString()
{
    return "Text Event";
}

ProtocolEntry* TextEvent::copy()
{
    return new TextEvent(*this);
}

void TextEvent::reloadState(ProtocolEntry* entry)
{
    TextEvent* other = dynamic_cast<TextEvent*>(entry);
    if (!other) {
        return;
    }
    MidiEvent::reloadState(entry);
    _text = other->_text;
    _type = other->_type;
}

QString TextEvent::textTypeString(int type)
{
    switch (type) {
    case TEXT:
        return "General text";
    case COPYRIGHT:
        return "Copyright";
    case TRACKNAME:
        return "Trackname";
    case INSTRUMENT_NAME:
        return "Instrument name";
    case LYRIK:
        return "Lyric";
    case MARKER:
        return "Marker";
    case COMMENT:
        return "Comment";
    }
    return QString();
}

int TextEvent::typeForNewEvents = TEXT;

void TextEvent::setTypeForNewEvents(int type)
{
    typeForNewEvents = type;
}

int TextEvent::getTypeForNewEvents()
{
    return typeForNewEvents;
}
