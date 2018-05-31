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

#ifndef UNKNOWNEVENT_H_
#define UNKNOWNEVENT_H_

#include "MidiEvent.h"
#include <QByteArray>

class UnknownEvent : public MidiEvent {

public:
    UnknownEvent(int channel, int type, QByteArray data, MidiTrack* track);
    UnknownEvent(UnknownEvent& other);
    QByteArray data();
    int line();
    QByteArray save();
    int type();
    void setType(int type);
    void setData(QByteArray d);

    virtual ProtocolEntry* copy();
    virtual void reloadState(ProtocolEntry* entry);

private:
    QByteArray _data;
    int _type;
};

#endif
