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

#include "MidiPlayer.h"

#include "../MidiEvent/NoteOnEvent.h"
#include "MidiFile.h"
#include "MidiOutput.h"
#include "PlayerThread.h"
#include "SingleNotePlayer.h"

#include "Metronome.h"

PlayerThread* MidiPlayer::filePlayer = new PlayerThread();
bool MidiPlayer::playing = false;
SingleNotePlayer* MidiPlayer::singleNotePlayer = new SingleNotePlayer();
double MidiPlayer::_speed = 1;

void MidiPlayer::play(MidiFile* file)
{

    if (isPlaying()) {
        stop();
    }

#ifdef __WINDOWS_MM__
    delete filePlayer;
    filePlayer = new PlayerThread();

    connect(MidiPlayer::playerThread(),
        SIGNAL(measureChanged(int, int)), Metronome::instance(), SLOT(measureUpdate(int, int)));
    connect(MidiPlayer::playerThread(),
        SIGNAL(measureUpdate(int, int)), Metronome::instance(), SLOT(measureUpdate(int, int)));
    connect(MidiPlayer::playerThread(),
        SIGNAL(meterChanged(int, int)), Metronome::instance(), SLOT(meterChanged(int, int)));
    connect(MidiPlayer::playerThread(),
        SIGNAL(playerStopped()), Metronome::instance(), SLOT(playbackStopped()));
    connect(MidiPlayer::playerThread(),
        SIGNAL(playerStarted()), Metronome::instance(), SLOT(playbackStarted()));
#endif

    int tickFrom = file->cursorTick();
    if (file->pauseTick() >= 0) {
        tickFrom = file->pauseTick();
    }
    file->preparePlayerData(tickFrom);
    filePlayer->setFile(file);
    filePlayer->start(QThread::TimeCriticalPriority);
    playing = true;
}

void MidiPlayer::play(NoteOnEvent* event)
{
    singleNotePlayer->play(event);
}

void MidiPlayer::stop()
{
    playing = false;
    filePlayer->stop();
}

bool MidiPlayer::isPlaying()
{
    return playing;
}

int MidiPlayer::timeMs()
{
    return filePlayer->timeMs();
}

PlayerThread* MidiPlayer::playerThread()
{
    return filePlayer;
}

void MidiPlayer::panic()
{

    // set all cannels note off / sounds off
    for (int i = 0; i < 16; i++) {
        // value (third number) should be 0, but doesnt work
        QByteArray array;
        array.append(0xB0 | i);
        array.append(char(123));
        array.append(char(127));

        MidiOutput::sendCommand(array);

        array.clear();
        array.append(0xB0 | i);
        array.append(char(120));
        array.append(char(0));
        MidiOutput::sendCommand(array);
    }
    if (MidiOutput::isAlternativePlayer) {
        foreach (int channel, MidiOutput::playedNotes.keys()) {
            foreach (int note, MidiOutput::playedNotes.value(channel)) {
                QByteArray array;
                array.append(0x80 | channel);
                array.append(char(note));
                array.append(char(0));
                MidiOutput::sendCommand(array);
            }
        }
    }
}

double MidiPlayer::speedScale()
{
    return _speed;
}

void MidiPlayer::setSpeedScale(double d)
{
    _speed = d;
}
