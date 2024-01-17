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

#ifndef SINGLENOTEPLAYER_H_
#define SINGLENOTEPLAYER_H_

#define SINGLE_NOTE_LENGTH_MS 2000

#include <QObject>

class NoteOnEvent;
class QTimer;

class SingleNotePlayer : public QObject {

    Q_OBJECT

public:
    SingleNotePlayer();
    void play(NoteOnEvent* event, int track_index, int ms = SINGLE_NOTE_LENGTH_MS);

public slots:
    void timeout();

private:
    QTimer* timer;
    QByteArray offMessage;
    bool playing;
    int _track_index;
};

#endif
