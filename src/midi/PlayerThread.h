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

class MidiFile;
class MidiEvent;
class QTime;

class PlayerThread : public QThread {

    Q_OBJECT

public:
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
    QTime* time;

    int measure, posInMeasure;
};

#endif
