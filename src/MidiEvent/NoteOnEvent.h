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

#ifndef NOTEONEVENT_H_
#define NOTEONEVENT_H_

#include "OnEvent.h"

class OffEvent;

class NoteOnEvent : public OnEvent {

public:
    NoteOnEvent(int note, int velocity, int ch, MidiTrack* track);
    NoteOnEvent(NoteOnEvent& other);

    int note();
    int velocity();
    int line();

    void setNote(int n);
    void setVelocity(int v);
    virtual ProtocolEntry* copy();
    virtual void reloadState(ProtocolEntry* entry);
    QString toMessage();
    QString offEventMessage();
    QByteArray save();
    QByteArray saveOffEvent();

    QString typeString();

protected:
    int _note, _velocity;
};

#endif
