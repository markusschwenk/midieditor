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

#ifndef SENDERTHREAD_H_
#define SENDERTHREAD_H_

#include <QMutex>
#include <QQueue>
#include <QThread>
#include <QWaitCondition>

#include "MidiOutput.h"

class SenderThread : public QThread {

public:
    SenderThread();
    void run();
    void enqueue(MidiEvent* event);

private:
    QQueue<MidiEvent*>* _eventQueue;
    QQueue<MidiEvent*>* _noteQueue;
    QMutex* _mutex;
    QWaitCondition* _waitCondition;
};

#endif
