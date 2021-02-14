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

#define SINGLE_NOTE_LENGTH_MS 2000

#include "../MidiEvent/NoteOnEvent.h"
#include "MidiOutput.h"
#include <QTimer>
#include "../midi/MidiFile.h"

SingleNotePlayer::SingleNotePlayer()
{
    playing = false;
    offMessage.clear();
    timer = new QTimer();
    timer->setInterval(SINGLE_NOTE_LENGTH_MS);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, &SingleNotePlayer::timeout);
}

void SingleNotePlayer::play(NoteOnEvent* event)
{
    if (playing) {
        MidiOutput::sendCommand(offMessage);
        timer->stop();
    }

    QByteArray array, array2;

    array2 = event->save();

    int i= array2[0] & 0xf;

    array.clear();
    array.append(0xB0 | i);
    array.append(char(0)); // bank 0
    array.append(char(Bank_MIDI[i]));
    MidiOutput::sendCommand(array);

    array.clear();
    array.append(0xC0 | i);
    array.append(char(Prog_MIDI[i])); // program
    MidiOutput::sendCommand(array);

    array.clear();
    array.append(0xE0 | i); //pitch bend
    array.append(char(0xff));
    array.append(char(0x3f));
    MidiOutput::sendCommand(array);

    offMessage = event->saveOffEvent();
    MidiOutput::sendCommand(event);
    playing = true;
    timer->start();
}

void SingleNotePlayer::timeout()
{
    MidiOutput::sendCommand(offMessage);
    timer->stop();
    playing = false;
}
