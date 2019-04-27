/*
 * MidiEditor
 * Copyright (C) 2010  Markus Schwenk
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.+
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIDITRACK_H_
#define MIDITRACK_H_

#include "../protocol/ProtocolEntry.h"

#include <QObject>
#include <QString>

class TextEvent;
class MidiFile;
class QColor;

class MidiTrack : public QObject, public ProtocolEntry {

    Q_OBJECT

public:
    MidiTrack(MidiFile* file);
    MidiTrack(MidiTrack& other);
    virtual ~MidiTrack();

    QString name();
    void setName(QString name);

    int number();
    void setNumber(int number);

    void setNameEvent(TextEvent* nameEvent);
    TextEvent* nameEvent();

    MidiFile* file();

    void assignChannel(int ch);
    int assignedChannel();

    void setHidden(bool hidden);
    bool hidden();

    void setMuted(bool muted);
    bool muted();

    virtual ProtocolEntry* copy();
    virtual void reloadState(ProtocolEntry* entry);

    QColor* color();

    MidiTrack* copyToFile(MidiFile* file);

signals:
    void trackChanged();

private:
    int _number;
    TextEvent* _nameEvent;
    MidiFile* _file;
    bool _hidden, _muted;

    int _assignedChannel;
};

#endif
