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
//#include "../midi/MidiInControl.h"
#include "../midi/MidiTrack.h"
#include "../tool/NewNoteTool.h"
#include "Metronome.h"

PlayerThread* MidiPlayer::filePlayer = new PlayerThread();
bool MidiPlayer::playing = false;
SingleNotePlayer* MidiPlayer::singleNotePlayer = new SingleNotePlayer();
double MidiPlayer::_speed = 1;


PlayerThreadSequencer* MidiPlayer::filePlayerSequencer = NULL;
PlayerSequencer* MidiPlayer::fileSequencer[16];

void MidiPlayer::play(MidiFile* file, int mode)
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
    filePlayer->mode = mode;
    if(!mode) {

        filePlayer->start(QThread::TimeCriticalPriority);
        playing = true;
    }
}
void MidiPlayer::start() {
    filePlayer->start(QThread::TimeCriticalPriority);
    playing = true;
}

void MidiPlayer::play(NoteOnEvent* event, int ms)
{
    int track_index = Tool::currentFile()->track(NewNoteTool::editTrack())->device_index();
    singleNotePlayer->play(event, track_index, ms);
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

    for(int track = 0; track < MAX_OUTPUT_DEVICES; track++) {
        // set all channels note off / sounds off
        for (int i = 0; i < 16; i++) {
            // value (third number) should be 0, but doesnt work
            QByteArray array;

            array.clear();
            array.append(0xB0 | i);
            array.append(char(121)); // RESET
            array.append(char(0));
            MidiOutput::sendCommand(array, track);

            array.clear();
            array.append(0xB0 | i);
            array.append(char(123)); // all notes off
            array.append(char(127));

            MidiOutput::sendCommand(array, track);

            array.clear();
            array.append(0xB0 | i);
            array.append(char(0)); // bank 0
            array.append(char(0x0));
            MidiOutput::sendCommand(array, track);

            array.clear();
            array.append(0xC0 | i);
            array.append(char(0x0)); // program
            MidiOutput::sendCommand(array, track);

            array.clear();
            array.append(0xE0 | i); //pitch bend
            array.append(char(0xff));
            array.append(char(0x3f));
            MidiOutput::sendCommand(array, track);

            array.clear();
            array.append(0xB0 | i);
            array.append(char(1)); // MODULATION
            array.append(char(0x0));
            MidiOutput::sendCommand(array, track);

            array.clear();
            array.append(0xB0 | i);
            array.append(char(11)); // EXPRESION
            array.append(char(127));
            MidiOutput::sendCommand(array, track);

            array.clear();
            array.append(0xB0 | i);
            array.append(char(7)); // VOLUME
            array.append(char(100));
            MidiOutput::sendCommand(array, track);

            array.clear();
            array.append(0xB0 | i);
            array.append(char(10)); // PAN
            array.append(char(64));
            MidiOutput::sendCommand(array, track);

            array.clear();
            array.append(0xB0 | i);
            array.append(char(73)); // ATTACK
            array.append(char(64));
            MidiOutput::sendCommand(array, track);

            array.clear();
            array.append(0xB0 | i);
            array.append(char(72)); // RELEASE
            array.append(char(64));
            MidiOutput::sendCommand(array, track);

            array.clear();
            array.append(0xB0 | i);
            array.append(char(75)); // DECAY
            array.append(char(64));
            MidiOutput::sendCommand(array, track);

            array.clear();
            array.append(0xB0 | i);
            array.append(char(91)); // REVERB
            array.append(char(0));
            MidiOutput::sendCommand(array, track);

            array.clear();
            array.append(0xB0 | i);
            array.append(char(93)); // CHORUS
            array.append(char(0));
            MidiOutput::sendCommand(array, track);

            array.clear();
            array.append(0xB0 | i);
            array.append(char(120)); // all sounds off
            array.append(char(0));
            MidiOutput::sendCommand(array, track);

        }
    }

    if (MidiOutput::isAlternativePlayer) {
        foreach (int channel, MidiOutput::playedNotes.keys()) {
            foreach (int note, MidiOutput::playedNotes.value(channel)) {
                QByteArray array;
                array.append(0x80 | channel);
                array.append(char(note));
                array.append(char(0));
                MidiOutput::sendCommand(array, 0);
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

//////////////////////////////////////////////
//  sequencer
/////////////////////////////////////////////

static int init_seq = 1;

void MidiPlayer::init_sequencer() {
    if(init_seq) {
        init_seq = 0;

        filePlayerSequencer = NULL;

        for(int n = 0; n < 16; n++)
            fileSequencer[n] = NULL;

        filePlayerSequencer = new PlayerThreadSequencer();
        filePlayerSequencer->start(QThread::TimeCriticalPriority);

    }
}

extern void msDelay(int ms);

void MidiPlayer::deinit_sequencer() {


    for(int n = 0; n < 16; n++) {

        if(fileSequencer[n]) {

            fileSequencer[n]->unsetFile(-1);

            PlayerSequencer *del = fileSequencer[n];
            fileSequencer[n] = NULL;
            delete del;

        }

        MidiOutput::sequencer_cmd[n] = SEQ_FLAG_STOP;

        MidiOutput::sequencer_enabled[n] = -1;
    }


    if(filePlayerSequencer) {
        filePlayerSequencer->stopped = true;
        msDelay(50);
        delete filePlayerSequencer;
    }

    filePlayerSequencer = NULL;

    qWarning("deinit_sequencer() destructor");
}

void MidiPlayer::unload_sequencer(int seq)
{
    init_sequencer();

    if(seq >= 64)
        return;

    int channel = seq / 4;

    if(fileSequencer[channel] && seq < 0) {
        PlayerSequencer * pseq = fileSequencer[channel];
        pseq->enable = false;
        fileSequencer[channel] = NULL;
        for(int n = 0; n < 4; n++)
            pseq->unsetFile(n);
        delete pseq;
        return;
    } else if(fileSequencer[channel]) {
        PlayerSequencer * pseq = fileSequencer[channel];
        bool temp = pseq->enable;
        pseq->enable = false;
        fileSequencer[channel] = NULL;
        pseq->unsetFile(seq & 3);
        pseq->enable = temp;
        fileSequencer[channel] = pseq;
    }

}

bool MidiPlayer::is_sequencer_loaded(int seq)
{
    init_sequencer();

    if(seq < 0 || seq >= 64)
        return false;

    int channel = seq / 4;

    if(fileSequencer[channel] != NULL) {
        return fileSequencer[channel]->isFile(seq & 3);
    }

    return false;

}

int MidiPlayer::play_sequencer(MidiFile* file, int seq, int bpm, int volume)
{

    init_sequencer();

    if(seq < 0 || seq >= 64)
        return -1;

    if(!file)
        return -1;

    int channel = seq / 4;

    file->is_sequencer = true;
    file->is_multichannel_sequencer = false;

    file->scale_time_sequencer = 1.0f/1.0f; // divider
    file->preparePlayerDataSequencer(0);//

    if(!fileSequencer[channel]) {
        PlayerSequencer * pseq = new PlayerSequencer(channel);
        if(!pseq) return -1;
        fileSequencer[channel] = pseq;
    }

    fileSequencer[channel]->setFile(file, seq & 3);
    fileSequencer[channel]->setScaleTime(bpm, seq & 3);

    if(volume < 0)
        volume = 0;
    if(volume > 127)
        volume = 127;

    fileSequencer[channel]->setVolume(volume, seq & 3);

    return 0;

}



