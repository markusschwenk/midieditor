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
