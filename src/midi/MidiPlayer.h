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

#ifndef MIDIPLAYER_H_
#define MIDIPLAYER_H_

#include <QObject>

class MidiFile;
class NoteOnEvent;
class PlayerThread;
class PlayerThreadSequencer;
class PlayerSequencer;
class SingleNotePlayer;

class MidiPlayer : public QObject {

    Q_OBJECT

public:
    static void play(MidiFile* file, int mode = 0);
    static void play(NoteOnEvent* event, int ms = 2000);

    static void init_sequencer();
    static void deinit_sequencer();
    static int play_sequencer(MidiFile* file, int seq, int bmp = 120, int volume = 127);
    static void unload_sequencer(int seq);
    static bool is_sequencer_loaded(int seq);

    static void start();
    static void stop();
    static bool isPlaying();
    static int timeMs();
    static PlayerThread* playerThread();
    static double speedScale();
    static void setSpeedScale(double d);

    /**
		 * Send all Notes off to every channel.
		 */
    static void panic();

    static PlayerThreadSequencer* filePlayerSequencer;

    static PlayerSequencer* fileSequencer[16];

private:
    static PlayerThread* filePlayer;
    static bool playing;
    static SingleNotePlayer* singleNotePlayer;
    static double _speed;

};

#endif
