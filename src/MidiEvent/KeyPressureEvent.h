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

#ifndef KEYPRESSUREEVENT_H_
#define KEYPRESSUREEVENT_H_

#include "MidiEvent.h"

#include <QLabel>
#include <QSpinBox>
#include <QWidget>

class KeyPressureEvent : public MidiEvent {

public:
    KeyPressureEvent(int channel, int value, int note, MidiTrack* track);
    KeyPressureEvent(KeyPressureEvent& other);

    virtual int line();

    QString toMessage();
    QByteArray save();

    virtual ProtocolEntry* copy();
    virtual void reloadState(ProtocolEntry* entry);

    QString typeString();

    int value();
    int note();
    void setValue(int v);
    void setNote(int n);

private:
    int _value, _note;
};

#endif
