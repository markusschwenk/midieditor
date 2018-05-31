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

#ifndef OFFEVENT_H_
#define OFFEVENT_H_

#include "MidiEvent.h"
#include <QList>
#include <QMultiMap>

class OnEvent;

class OffEvent : public MidiEvent {

public:
    OffEvent(int ch, int line, MidiTrack* track);
    OffEvent(OffEvent& other);

    void setOnEvent(OnEvent* event);
    OnEvent* onEvent();

    static void enterOnEvent(OnEvent* event);
    static void clearOnEvents();
    static void removeOnEvent(OnEvent* event);
    static QList<OnEvent*> corruptedOnEvents();
    void draw(QPainter* p, QColor c);
    int line();
    QByteArray save();
    QString toMessage();

    ProtocolEntry* copy();
    void reloadState(ProtocolEntry* entry);

    void setMidiTime(int t, bool toProtocol = true);

    virtual bool isOnEvent();

protected:
    OnEvent* _onEvent;

    // Saves all openes and not closed onEvents. When an offEvent is created,
    // it searches his onEvent in onEvents and removes it from onEvents.
    static QMultiMap<int, OnEvent*>* onEvents;

    // needs to save the line, because offEvents are bound to their onEvents.
    // Setting the line is necessary to find the onEvent in the QMap
    int _line;
};

#endif
