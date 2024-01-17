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

#include "OffEvent.h"
#include "OnEvent.h"

QMultiMap<int, OnEvent*>* OffEvent::onEvents = new QMultiMap<int, OnEvent*>();

OffEvent::OffEvent(int ch, int l, MidiTrack* track)
    : MidiEvent(ch, track)
{
    _line = l;
    _onEvent = 0;

    int midi_time = -1;
    int pos = -1;
    midi_modified = false;

    QList<OnEvent*> eventsToClose = onEvents->values(line());
    for (int i = 0; i < eventsToClose.length(); i++) {
        if (eventsToClose.at(i)->channel() == channel()) {

            // find first onEvent to let

            if(midi_time == -1) {
                pos = i;
                midi_time = eventsToClose.at(i)->midiTime();
            } else if(eventsToClose.at(i)->midiTime() < midi_time){
                pos = i;
                midi_time = eventsToClose.at(i)->midiTime();
            }

        }

    }

    if(pos >= 0) {
        setOnEvent(eventsToClose.at(pos));

        // remove entry
        removeOnEvent(eventsToClose.at(pos));
    }

    return;
}

QList<OnEvent*> OffEvent::corruptedOnEvents()
{
    return onEvents->values();
}

void OffEvent::removeOnEvent(OnEvent* event)
{
    onEvents->remove(event->line(), event);
    /*
	for(int j = 0; j<eventsToClose.length(); j++){
		if(i!=j){
			onEvents->insertMulti(line(), eventsToClose.at(j));
		}
	}
	*/
}
OffEvent::OffEvent(OffEvent& other)
    : MidiEvent(other)
{
    _onEvent = other._onEvent;
    midi_modified = other.midi_modified;
}

void OffEvent::setOnEvent(OnEvent* event)
{
    _onEvent = event;
    midi_modified = true;
    event->setOffEvent(this);
}

OnEvent* OffEvent::onEvent()
{
    return _onEvent;
}

void OffEvent::setMidiTime(int t, bool toProtocol)
{
    midi_modified = true;
    MidiEvent::setMidiTime(t, toProtocol);
}

void OffEvent::enterOnEvent(OnEvent* event)
{
    onEvents->insert(event->line(), event);
}

void OffEvent::clearOnEvents()
{

    onEvents->clear();
}

void OffEvent::draw(QPainter* p, QColor c)
{
    if (onEvent() && !onEvent()->shown()) {
        onEvent()->draw(p, c);
    }
}

ProtocolEntry* OffEvent::copy()
{
    return new OffEvent(*this);
}

void OffEvent::reloadState(ProtocolEntry* entry)
{
    OffEvent* other = dynamic_cast<OffEvent*>(entry);
    if (!other) {
        return;
    }
    MidiEvent::reloadState(entry);
    _onEvent = other->_onEvent;
    midi_modified = other->midi_modified;
}

QByteArray OffEvent::save()
{
    if (onEvent()) {
        return onEvent()->saveOffEvent();
    } else {
        return QByteArray();
    }
}

QString OffEvent::toMessage()
{
    if (onEvent()) {
        return onEvent()->offEventMessage();
    } else {
        return QString();
    }
}

int OffEvent::line()
{
    if (onEvent()) {
        return onEvent()->line();
    }
    return _line;
}

bool OffEvent::isOnEvent()
{
    return false;
}
