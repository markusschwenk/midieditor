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

#include "PlayerThread.h"
#include "../MidiEvent/KeySignatureEvent.h"
#include "../MidiEvent/NoteOnEvent.h"
#include "../MidiEvent/OffEvent.h"
#include "../MidiEvent/TimeSignatureEvent.h"
#include "../MidiEvent/TextEvent.h"
#include "MidiFile.h"
#include "MidiInput.h"
#include "MidiOutput.h"
#include "MidiPlayer.h"
#include "MidiInControl.h"
#include <QMultiMap>
#include <QTime>

#define INTERVAL_TIME 15
#define TIMEOUTS_PER_SIGNAL 1

//static MidiEvent *
int text_ev[8];
QString *midi_text[8];

PlayerThread::PlayerThread()
    : QThread()
{
    file = 0;
    timer = 0;
    timeoutSinceLastSignal = 0;
    time = 0;
}

void PlayerThread::setFile(MidiFile* f)
{
    file = f;
}

void PlayerThread::stop()
{
    stopped = true;
}

void PlayerThread::setInterval(int i)
{
    interval = i;
}

void PlayerThread::run()
{
    for(int n = 0; n < 8; n++) {
        text_ev[n] = -1;
        midi_text[n] = NULL;
    }

    realtimecount = QDateTime::currentMSecsSinceEpoch();

    text_tim = 0;

    if (!timer) {
        timer = new QTimer();
    }
    if (time) {
        delete time;
        time = 0;
    }

    events = file->playerData();

    if (file->pauseTick() >= 0) {
        position = file->msOfTick(file->pauseTick());
    } else {
        position = file->msOfTick(file->cursorTick());
    }

    if(!mode)
        emit playerStarted();

    // Reset all Controllers
    for (int i = 0; i < 16; i++) {
        QByteArray array;
        array.append(0xB0 | i);
        array.append(121);
        array.append(char(0));
        MidiOutput::sendCommand(array);
    }
    MidiOutput::playedNotes.clear();

    // All Events before position should be sent, progChanges and ControlChanges
    QMultiMap<int, MidiEvent*>::iterator it = events->begin();
    while (it != events->end()) {
        if (it.key() >= position) {
            break;
        }

        MidiOutput::sendCommand(it.value());
        it++;
    }

    setInterval(INTERVAL_TIME);

    connect(timer, SIGNAL(timeout()), this, SLOT(timeout()), Qt::DirectConnection);
    if(!mode) {
        realtimecount = QDateTime::currentMSecsSinceEpoch();
        timer->start(INTERVAL_TIME);
    }

    stopped = false;

    QList<TimeSignatureEvent*>* list = 0;

    int tickInMeasure = 0;
    measure = file->measure(file->cursorTick(), file->cursorTick(), &list, &tickInMeasure);

    emit(measureChanged(measure, tickInMeasure));

    if(mode) {

        if(MidiInControl::wait_record_thread() < 0) {
            emit playerStopped();
            return;
        }

        realtimecount = QDateTime::currentMSecsSinceEpoch();
        emit playerStarted();
        timer->start(INTERVAL_TIME);
        //emit(measureChanged(measure, tickInMeasure));
    }

    if (exec() == 0) {
        timer->stop();
        emit playerStopped();
    }
}


void PlayerThread::timeout()
{

    if (!time) {
        time = new QElapsedTimer();
        time->start();
    }

    disconnect(timer, SIGNAL(timeout()), this, SLOT(timeout()));
    qint64 crealtimecount = QDateTime::currentMSecsSinceEpoch();
    qint64 diff_t = crealtimecount - realtimecount;

    if (stopped) {
        disconnect(timer, SIGNAL(timeout()), this, SLOT(timeout()));

        // AllNotesOff // All SoundsOff
        for (int i = 0; i < 16; i++) {
            // value (third number) should be 0, but doesnt work
            QByteArray array;
            array.append(0xB0 | i);
            array.append(char(123));
            array.append(char(127));
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

        for(int n = 0; n < 8; n++) {
            text_ev[n] = -1;
            if(midi_text[n]) delete midi_text[n];
            midi_text[n] = NULL;
        }

        text_tim = 0;
        quit();

    } else {

        int newPos = position + diff_t /*time->elapsed()*/ * MidiPlayer::speedScale();
        int tick = file->tick(newPos);
        QList<TimeSignatureEvent*>* list = 0;
        int ickInMeasure = 0;

        int new_measure = file->measure(tick, tick, &list, &ickInMeasure);

        // compute current pos

        if (new_measure > measure) {
            emit measureChanged(new_measure, ickInMeasure);
            measure = new_measure;
        }

        time->restart();

        QMultiMap<int, MidiEvent*>::iterator it = events->lowerBound(position);

        // window for karaoke text events (displaced +500 tick)
        while (it != events->end() && it.key() <= newPos + 500) {

            int sendPosition = it.key();

            // get text events before
            while (it != events->end()){
                TextEvent* textev = dynamic_cast<TextEvent*>(it.value());
                if (textev &&
                        it.value()->isOnEvent() && it.key() >= sendPosition
                        && it.key() <= sendPosition + 500) {


                    if(textev->type() == (char) TextEvent::TEXT) {

                        int n;
                        int flag = 1;
                        for(n = 0; n < 8; n++) { // skip repeated events
                            if(text_ev[n]==textev->midiTime()) {
                                flag =0; break;
                            }
                        }

                        if(flag) {

                            if(midi_text[0]) delete midi_text[0];
                            for(n = 0; n < 7; n++) {
                                text_ev[n] = text_ev[n + 1];
                                midi_text[n] = midi_text[n + 1];
                            }

                            text_ev[7] = textev->midiTime();
                            QString *s = new QString(textev->text());
                            midi_text[7] = s;
                            text_tim= 0;

                            //perror((const char *) midi_text[7]->toLatin1());
                        }

                    } //gk

                    it++;break;
                }

                it++;
            }

        }
        text_tim+= INTERVAL_TIME;

        if(text_tim > 2000) { // deleting text events
            int n;
            if(midi_text[0]) delete midi_text[0];
            for(n = 0; n < 7; n++) {
                text_ev[n] = text_ev[n + 1];
                midi_text[n] = midi_text[n + 1];
            }

            text_ev[7] = 0;
            midi_text[7] = NULL;

        }

        it = events->lowerBound(position);
        while (it != events->end() && it.key() < newPos) {

            // save events for the given tick
            QList<MidiEvent *> onEv, offEv;
            int sendPosition = it.key();

            do {
                TextEvent* textev = dynamic_cast<TextEvent*>(it.value());
                // ignore text event
                if(textev/*it.value()->line() == MidiEvent::TEXT_EVENT_LINE*/) ;
                    else if (it.value()->isOnEvent()) {
                    onEv.append(it.value());
                } else {
                    offEv.append(it.value());
                }
                it++;
            } while (it != events->end() && it.key() == sendPosition);

            foreach (MidiEvent* ev, offEv) {
                MidiOutput::sendCommand(ev);
            }
            foreach (MidiEvent* ev, onEv) {
                if (ev->line() == MidiEvent::KEY_SIGNATURE_EVENT_LINE) {
                    KeySignatureEvent* keySig = dynamic_cast<KeySignatureEvent*>(ev);
                    if (keySig) {
                        emit tonalityChanged(keySig->tonality());
                    }
                }
                if (ev->line() == MidiEvent::TIME_SIGNATURE_EVENT_LINE) {
                    TimeSignatureEvent* timeSig = dynamic_cast<TimeSignatureEvent*>(ev);
                    if (timeSig) {
                        emit meterChanged(timeSig->num(), timeSig->denom());
                    }
                }

                MidiOutput::sendCommand(ev);
            }


        }

        // end if it was last event, but only if not recording
        if (it == events->end() && !MidiInput::recording()) {
            stop();
        }
        position = newPos;
        timeoutSinceLastSignal++;
        MidiInput::setTime(position);
        if (timeoutSinceLastSignal == TIMEOUTS_PER_SIGNAL) {
            emit timeMsChanged(position);
            emit measureUpdate(measure, ickInMeasure);
            timeoutSinceLastSignal = 0;
        }
    }
    realtimecount = crealtimecount;
    connect(timer, SIGNAL(timeout()), this, SLOT(timeout()), Qt::DirectConnection);
}

int PlayerThread::timeMs()
{
    return position;
}
