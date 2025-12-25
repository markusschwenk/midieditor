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

#include "SingleNotePlayer.h"

#include "../MidiEvent/NoteOnEvent.h"
#include "MidiOutput.h"
#include <QTimer>
#include "../midi/MidiFile.h"

SingleNotePlayer::SingleNotePlayer()
{
    playing = false;
    _track_index = 0;
    offMessage.clear();

    timer = new QTimer();
    timer->setInterval(SINGLE_NOTE_LENGTH_MS);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, &SingleNotePlayer::timeout);
}

void SingleNotePlayer::play(NoteOnEvent* event, int track_index, int ms)
{
    MidiOutput::playedNotes.clear(); // alternative player sucks

    _track_index = track_index;

    if (playing) {
        MidiOutput::sendCommand(offMessage, track_index);
        timer->stop();
    }

    QByteArray array, array2;

    array2 = event->save();

    int i= array2[0] & 0xf;

    array.clear();
    array.append(0xB0 | i);
    array.append(char(0)); // bank 0
    array.append(char(MidiOutput::file->Bank_MIDI[i]));
    MidiOutput::sendCommand(array, track_index);

    array.clear();
    array.append(0xC0 | i);
    array.append(char(MidiOutput::file->Prog_MIDI[i])); // program
    MidiOutput::sendCommand(array, track_index);

    array.clear();
    array.append(0xE0 | i); //pitch bend
    array.append(char(0xff));
    array.append(char(0x3f));
    MidiOutput::sendCommand(array, track_index);

    offMessage = event->saveOffEvent();
    MidiOutput::sendCommand(event, track_index);
    playing = true;
    timer->setInterval(ms);
    timer->start();
}

void SingleNotePlayer::timeout()
{
    MidiOutput::sendCommand(offMessage, _track_index);
    timer->stop();
    playing = false;
}
