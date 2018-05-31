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

#ifndef MIDIEVENT_H_
#define MIDIEVENT_H_

#include "../gui/EventWidget.h"
#include "../gui/GraphicObject.h"
#include "../protocol/ProtocolEntry.h"
#include <QColor>
#include <QDataStream>
#include <QWidget>

class MidiFile;
class QSpinBox;
class QLabel;
class QWidget;
class EventWidget;
class MidiTrack;

class MidiEvent : public ProtocolEntry, public GraphicObject {

public:
    MidiEvent(int channel, MidiTrack* track);
    MidiEvent(MidiEvent& other);

    static MidiEvent* loadMidiEvent(QDataStream* content,
        bool* ok, bool* endEvent, MidiTrack* track, quint8 startByte = 0,
        quint8 secondByte = 0);

    static EventWidget* eventWidget();
    static void setEventWidget(EventWidget* widget);

    enum {
        TEMPO_CHANGE_EVENT_LINE = 128,
        TIME_SIGNATURE_EVENT_LINE,
        KEY_SIGNATURE_EVENT_LINE,
        PROG_CHANGE_LINE,
        CONTROLLER_LINE,
        KEY_PRESSURE_LINE,
        CHANNEL_PRESSURE_LINE,
        TEXT_EVENT_LINE,
        PITCH_BEND_LINE,
        SYSEX_LINE,
        UNKNOWN_LINE
    };
    void setTrack(MidiTrack* track, bool toProtocol = true);
    MidiTrack* track();
    void setChannel(int channel, bool toProtocol = true);
    int channel();
    virtual void setMidiTime(int t, bool toProtocol = true);
    int midiTime();
    void setFile(MidiFile* f);
    MidiFile* file();
    bool shownInEventWidget();

    virtual int line();
    virtual QString toMessage();
    virtual QByteArray save();
    virtual void draw(QPainter* p, QColor c);

    virtual ProtocolEntry* copy();
    virtual void reloadState(ProtocolEntry* entry);

    virtual QString typeString();

    virtual bool isOnEvent();

    static QMap<int, QString> knownMetaTypes();

    void setTemporaryRecordID(int id);
    int temporaryRecordID();

    virtual void moveToChannel(int channel);

protected:
    int numChannel, timePos;
    MidiFile* midiFile;
    static quint8 _startByte;
    static EventWidget* _eventWidget;
    MidiTrack* _track;
    int _tempID;
};

#endif
