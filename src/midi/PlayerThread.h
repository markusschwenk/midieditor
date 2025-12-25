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

#ifndef PLAYERTHREAD_H_
#define PLAYERTHREAD_H_

#include <QMultiMap>
#include <QObject>
#include <QThread>
#include <QTimer>
#include <QElapsedTimer>
#include <QMutex>

#define MAP_NOTE_OFF 0
#define MAP_NOTE_ON 1
#define MAP_NOTE_FLUSH 2

#define SEQ_FLAG_SWITCH_ON 65536
#define SEQ_FLAG_AUTORHYTHM 128
#define SEQ_FLAG_INFINITE 32
#define SEQ_FLAG_LOOP 16
#define SEQ_FLAG_STOP 4
#define SEQ_FLAG_ON 2
#define SEQ_FLAG_ON2 1

#define SEQ_FLAG_MASK (SEQ_FLAG_INFINITE | SEQ_FLAG_LOOP)


class MidiFile;
class MidiEvent;
//class QTime;

class PlayerThread : public QThread {

    Q_OBJECT

public:
    int mode;

    PlayerThread();
    void setFile(MidiFile* f);
    void stop();
    void run();
    void setInterval(int i);
    int timeMs();

public slots:
    void timeout();

signals:
    void timeMsChanged(int ms);
    void playerStopped();
    void playerStarted();

    void tonalityChanged(int tonality);
    void measureChanged(int measure, int tickInMeasure);
    void meterChanged(int num, int denum);

    void measureUpdate(int measure, int tickInMeasure);

private:
    MidiFile* file;
    QMultiMap<int, MidiEvent*>* events;
    int interval, position, timeoutSinceLastSignal;
    volatile bool stopped;
    QTimer* timer;
    QElapsedTimer* time;
    qint64 realtimecount;

    int measure, posInMeasure;

    int text_tim;
    bool MultitrackMode;

    bool update_prg[48]; // real time prg change event
};

class PlayerThreadSequencer : public QThread {

    Q_OBJECT

public:

    PlayerThreadSequencer();

    void run();

    volatile bool stopped;

public slots:
    void timeout();

signals:
    void loop(qint64 diff_t);
private:

    QTimer* timer;
    QElapsedTimer* time;
    qint64 realtimecount;
};

class PlayerSequencer : public QObject {

    Q_OBJECT

public:

    PlayerSequencer(int chan);

    void setFile(MidiFile* f, int index);
    void unsetFile(int index);
    bool isFile(int index);

    // 0 - 508 bmp
    void setScaleTime(int bpm, int index);
    void setVolume(int volume, int index);
    void setButtons(unsigned int buttons, int index);
    unsigned int getButtons(int index);

    void setMode(int index, unsigned int buttons);

    // program to send notes off (-1 = all)
    void set_flush(int note);

    bool enable;

    void loop(qint64 diff_t);
    void flush(); // send notes off


    bool is_drum_only;
    bool use_drum;
    bool autorhythm;

private:
    int volume[4];
    MidiFile* file[4];
    QMultiMap<int, MidiEvent*>* events[4];
    int position;
    int current_index;

    bool update_prg[16]; // real time prg change event
    unsigned char map_key_status[128]; // ch 0
    unsigned char map_key[128]; // ch 0
    unsigned char map_key2[4][128]; // ch 1 - 3 and 9

    int note_random;
    int note_count;
    int _chan;
    int _device;

    int _force_flush_mode;

    void SendInput(QByteArray a);

    static QMutex mutex_player;

    unsigned int _buttons[4];
};

#endif
