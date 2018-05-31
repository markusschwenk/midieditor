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
class SingleNotePlayer;

class MidiPlayer : public QObject {

    Q_OBJECT

public:
    static void play(MidiFile* file);
    static void play(NoteOnEvent* event);
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

private:
    static PlayerThread* filePlayer;
    static bool playing;
    static SingleNotePlayer* singleNotePlayer;
    static double _speed;
};

#endif
