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

#ifndef TEXTEVENT_H_
#define TEXTEVENT_H_

#include "MidiEvent.h"
#include <QByteArray>

class TextEvent : public MidiEvent {

public:
    TextEvent(int channel, MidiTrack* track);
    TextEvent(TextEvent& other);

    QString text();
    void setText(QString text);

    int type();
    void setType(int type);

    int line();

    QByteArray save();

    virtual ProtocolEntry* copy();
    virtual void reloadState(ProtocolEntry* entry);

    enum {
        TEXT = 0x01,
        COPYRIGHT,
        TRACKNAME,
        INSTRUMENT_NAME,
        LYRIK,
        MARKER,
        COMMENT
    };

    QString typeString();
    static QString textTypeString(int type);

    static int getTypeForNewEvents();
    static void setTypeForNewEvents(int type);

private:
    int _type;
    QString _text;
    static int typeForNewEvents;
};

#endif
