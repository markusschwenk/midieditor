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

#include "OnEvent.h"

#include "../midi/MidiFile.h"
#include "../midi/MidiTrack.h"
#include "OffEvent.h"
#include <QBoxLayout>

OnEvent::OnEvent(int ch, MidiTrack* track)
    : MidiEvent(ch, track)
{
    _offEvent = 0;

    return;
}

OnEvent::OnEvent(OnEvent& other)
    : MidiEvent(other)
{
    _offEvent = other._offEvent;
    return;
}

void OnEvent::setOffEvent(OffEvent* event)
{
    _offEvent = event;
}

OffEvent* OnEvent::offEvent()
{
    return _offEvent;
}

ProtocolEntry* OnEvent::copy()
{
    return new OnEvent(*this);
}

void OnEvent::reloadState(ProtocolEntry* entry)
{
    OnEvent* other = dynamic_cast<OnEvent*>(entry);
    if (!other) {
        return;
    }
    MidiEvent::reloadState(entry);
    _offEvent = other->_offEvent;
}

QByteArray OnEvent::saveOffEvent()
{
    return QByteArray();
}

QString OnEvent::offEventMessage()
{
    return "";
}

void OnEvent::moveToChannel(int channel)
{
    MidiEvent::moveToChannel(channel);
    offEvent()->moveToChannel(channel);
}
