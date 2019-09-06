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

#include "SenderThread.h"
#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/NoteOnEvent.h"
#include "../MidiEvent/OffEvent.h"

SenderThread::SenderThread()
{
    _eventQueue = new QQueue<MidiEvent*>;
    _noteQueue = new QQueue<MidiEvent*>;
    _mutex = new QMutex();
    _waitCondition = new QWaitCondition();
}

void SenderThread::run()
{
    _mutex->lock();

    while (true) {
        _waitCondition->wait(_mutex, 500);

        // First, send the misc events, such as control change and program change events.
        while (!_eventQueue->isEmpty()) {
            // send command
            MidiOutput::sendEnqueuedCommand(_eventQueue->head()->save());
            _eventQueue->pop_front();
        }
        // Now send the note events.
        while (!_noteQueue->isEmpty()) {
            // send command
            MidiOutput::sendEnqueuedCommand(_noteQueue->head()->save());
            _noteQueue->pop_front();
        }
    }

    _mutex->unlock();
}

void SenderThread::enqueue(MidiEvent* event)
{
    _mutex->lock();

    // If it is a NoteOnEvent or an OffEvent, we put it in _noteQueue.
    if (dynamic_cast<NoteOnEvent*>(event) || dynamic_cast<OffEvent*>(event))
        _noteQueue->push_back(event);
    // Otherwise, it goes into _eventQueue.
    else
        _eventQueue->push_back(event);

    _waitCondition->wakeOne();
    _mutex->unlock();
}
