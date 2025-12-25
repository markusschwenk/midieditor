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
#include "../midi/MidiTrack.h"
#include "../MidiEvent/KeySignatureEvent.h"
#include "../MidiEvent/TimeSignatureEvent.h"
#include "../MidiEvent/TextEvent.h"
#include "../MidiEvent/SysExEvent.h"
#include "../MidiEvent/ControlChangeEvent.h"
#include "../MidiEvent/ProgChangeEvent.h"
#include "../gui/GuiTools.h"

#include "../tool/FingerPatternDialog.h"
#ifdef __WINDOWS_MM__
#include <windows.h>
#endif

#include "MidiFile.h"
#include "MidiInput.h"
#include "MidiOutput.h"
#include "MidiPlayer.h"
#include "MidiInControl.h"
#include <QMultiMap>
#include <QTime>

#define INTERVAL_TIME 15
#define TIMEOUTS_PER_SIGNAL 1

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
    MultitrackMode = file->MultitrackMode;

    for(int n = 0; n < 8; n++) {
        text_ev[n] = -1;
        midi_text[n] = NULL;
    }

    // real time prg change event off
    for(int n = 0; n < 48; n++)
        update_prg[n] = false;

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

    for(int track = 0; track < (MultitrackMode ? 3 : 1) ; track++) {
        // Reset all Controllers
        for (int i = 0; i < 16; i++) {
            QByteArray array;
            array.append(0xB0 | i);
            array.append(121);
            array.append(char(0));
            MidiOutput::sendCommand(array, track);
        }
    }

    MidiOutput::playedNotes.clear();

    // All Events before position should be sent, progChanges and ControlChanges
    QMultiMap<int, MidiEvent*>::iterator it = events->begin();
    while (it != events->end()) {
        if (it.key() >= position) {
            break;
        }

        int track = MultitrackMode ? it.value()->track()->device_index() : 0;

        ControlChangeEvent* ctrl = dynamic_cast<ControlChangeEvent*>(it.value());
        if(ctrl && ctrl->control() == 0) {
            update_prg[ctrl->channel() + track * 16] = true;
        }

        MidiOutput::sendCommand(it.value(), track);
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
        emit(measureChanged(measure, tickInMeasure));
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

        for(int track = 0; track < (MultitrackMode ? 3 : 1) ; track++) {
            // AllNotesOff // All SoundsOff
            for (int i = 0; i < 16; i++) {
                // value (third number) should be 0, but doesnt work
                QByteArray array;
                array.append(0xB0 | i);
                array.append(char(123));
                array.append(char(127));
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

        for(int n = 0; n < 8; n++) {
            text_ev[n] = -1;
            if(midi_text[n]) delete midi_text[n];
            midi_text[n] = NULL;
        }

        text_tim = 0;
        quit();

    } else {

        int newPos = position + diff_t * MidiPlayer::speedScale();
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
            QList<MidiEvent *> onEv, offEv, ctrlEv, prgEv, sysEv;
            int sendPosition = it.key();

            do {
                TextEvent* textev = dynamic_cast<TextEvent*>(it.value());
                ControlChangeEvent* ctrl = dynamic_cast<ControlChangeEvent*>(it.value());
                ProgChangeEvent* prg = dynamic_cast<ProgChangeEvent*>(it.value());
                SysExEvent* sys = dynamic_cast<SysExEvent*>(it.value());

                // ignore text event
                if(textev/*it.value()->line() == MidiEvent::TEXT_EVENT_LINE*/) ;

                else if (sys) {
                    sysEv.append(it.value());
                } else if (ctrl) {
                    if(ctrl && ctrl->control() == 0) {
                        int track = MultitrackMode ? ctrl->track()->device_index() : 0;
                        // it needs a real time prg change event
                        update_prg[ctrl->channel() + track * 16] = true;
                    }
                    ctrlEv.append(it.value());
                } else if (prg) {
                    prgEv.append(it.value());
                } else if (it.value()->isOnEvent()) {
                    onEv.append(it.value());
                } else {
                    offEv.append(it.value());
                }
                it++;
            } while (it != events->end() && it.key() == sendPosition);

            // SysEx event
            foreach (MidiEvent* event, sysEv) {

                MidiOutput::sendCommand(event, 0);
            }

            // CtrlEv event
            foreach (MidiEvent* event, ctrlEv) {

                int track = MultitrackMode ? event->track()->device_index() : 0;

                MidiOutput::sendCommand(event, track);
            }

            // PrgEv event
            foreach (MidiEvent* event, prgEv) {

                int track = MultitrackMode ? event->track()->device_index() : 0;

                MidiOutput::sendCommand(event, track);
            }

            foreach (MidiEvent* ev, offEv) {
                int track = MultitrackMode ? ev->track()->device_index() : 0;

                MidiOutput::sendCommand(ev, track);
            }

            foreach (MidiEvent* ev, onEv) {
                int track = 0;
                if (ev->line() == MidiEvent::KEY_SIGNATURE_EVENT_LINE) {
                    KeySignatureEvent* keySig = dynamic_cast<KeySignatureEvent*>(ev);
                    if (keySig) {
                        emit tonalityChanged(keySig->tonality());
                    }
                } else if (ev->line() == MidiEvent::TIME_SIGNATURE_EVENT_LINE) {
                    TimeSignatureEvent* timeSig = dynamic_cast<TimeSignatureEvent*>(ev);
                    if (timeSig) {
                        emit meterChanged(timeSig->num(), timeSig->denom());
                    }
                } else {

/* For some stupid reason fluidsynth has problems when changing banks and programs.
   Sending program change just before first note works */

                    int chan = ev->channel();
                    track = MultitrackMode ? ev->track()->device_index() : 0;

                    if(ev->line() < 128 && update_prg[chan + 16 * track]) {
                        update_prg[chan + 16 * track] = false;

                        QByteArray array;
                        array.clear();
                        array.append(0xC0 | chan); // program change
                        array.append(char(MidiOutput::file->Prog_MIDI[chan + 4 * (track!=0) + 16 * track]));
                        MidiOutput::sendCommand(array, track);
                    }
                }

                MidiOutput::sendCommand(ev, track);
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

/////////////////////////////////////////
// PlayerThreadSequencer
////////////////////////////////////////


PlayerThreadSequencer::PlayerThreadSequencer()
    : QThread()
{
    timer = 0;
    time = 0;
}

void PlayerThreadSequencer::run()
{
    realtimecount = QDateTime::currentMSecsSinceEpoch();

    if (!timer) {
        timer = new QTimer();
    }

    connect(timer, SIGNAL(timeout()), this, SLOT(timeout()), Qt::DirectConnection);

    realtimecount = QDateTime::currentMSecsSinceEpoch();
    timer->start(INTERVAL_TIME);

    stopped = false;

    if (exec() == 0) {
        timer->stop();
    }
}

void PlayerThreadSequencer::timeout()
{
    static int counter = 0;

    if (!time) {
        time = new QElapsedTimer();
        time->start();
    }

    disconnect(timer, SIGNAL(timeout()), this, SLOT(timeout()));
    qint64 crealtimecount = QDateTime::currentMSecsSinceEpoch();
    qint64 diff_t = crealtimecount - realtimecount;
    if (stopped) {
        disconnect(timer, SIGNAL(timeout()), this, SLOT(timeout()));


        quit();

    } else {

        for(int n = 0; n < 16; n++) {

            if(MidiPlayer::fileSequencer[n] && MidiPlayer::fileSequencer[n]->enable) {
                if(!FingerPatternDialog::finger_on(n))
                    MidiPlayer::fileSequencer[n]->loop(diff_t);
            }

        }

        if(counter == 0 && MidiInControl::key_live) { // used for no sleep the computer

#ifdef __WINDOWS_MM__
            SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
#endif
            MidiInControl::key_live = 0;
        }

        counter++;
        counter&= 127;


        time->restart();
        realtimecount = crealtimecount;
        connect(timer, SIGNAL(timeout()), this, SLOT(timeout()), Qt::DirectConnection);

    }
}

QMutex PlayerSequencer::mutex_player;

PlayerSequencer::PlayerSequencer(int chan) : QObject()
{
    enable = false;
    _chan = chan;
    is_drum_only = false;
    use_drum = false;
    autorhythm = false;
    _force_flush_mode = 0;

    for(int n = 0; n < 4; n++) {
        volume[n] = 127;
        file[n] = NULL;
        events[n] = NULL;
        _buttons[n] = 0;
    }


    note_count = 0;
    note_random = -1;

    for(int n = 0; n < 128; n++) {
        map_key_status[n] = MAP_NOTE_OFF;
        map_key[n] = 0; // ch 0
        map_key2[0][n] = 0; // ch 9
        map_key2[1][n] = 0; // ch 1
        map_key2[2][n] = 0; // ch 2
        map_key2[3][n] = 0; // ch 3

    }

    // run

    // real time prg change event off
    for(int n = 0; n < 16; n++)
        update_prg[n] = false;

    position = 0;
    current_index = -1;

}

void PlayerSequencer::setScaleTime(int bpm, int index)
{
    if(index > 3)
        return;

    mutex_player.lock();
    if(bpm < 1)
        bpm = 1;
    if(bpm > 635)
        bpm = 635;

    double fscale = ((double) bpm) / 120.0;

    for(int n = 0; n < 4; n++) {
        if(index >= 0 && index != n)
            continue;
        if(file[n])
            file[n]->scale_time_sequencer = (float) (fscale);
    }


    mutex_player.unlock();
}

void PlayerSequencer::setVolume(int volume, int index)
{
    if(index > 3)
        return;

    mutex_player.lock();
    if(volume < 4)
        volume = 4;
    if(volume > 127) volume = 127;


    for(int n = 0; n < 4; n++) {
        if(index >= 0 && index != n)
            continue;
        this->volume[n] = volume;
    }

    mutex_player.unlock();
}

void PlayerSequencer::setButtons(unsigned int buttons, int index)
{
    if(index > 3)
        return;
    if(index < 0)
        return;

    _buttons[index] = buttons;

    if(MidiOutput::sequencer_enabled[_chan] == index) {
        MidiOutput::sequencer_cmd[_chan] =
            (MidiOutput::sequencer_cmd[_chan] & ~SEQ_FLAG_MASK) |
            (buttons & SEQ_FLAG_MASK);
    }
}

unsigned int PlayerSequencer::getButtons(int index)
{
    if(index > 3)
        return 0;
    if(index < 0)
        return 0;

    return _buttons[index];
}

void PlayerSequencer::set_flush(int note)
{
    mutex_player.lock();

    QByteArray a;

    for(int n = 0; n < 128; n++) {
        if(map_key_status[n] == MAP_NOTE_ON && (map_key[n] == note || note < 0)) {
            map_key_status[n] = MAP_NOTE_FLUSH; // mark to flush
        }
    }

    mutex_player.unlock();

}


void PlayerSequencer::flush()
{
    QByteArray a;

    for(int note = 0; note < 128; note++) {
        if(map_key_status[note]) {
            map_key_status[note] = MAP_NOTE_OFF;
            map_key[note] = 0;
            a.clear();
            a.append(0x80 | 0);
            a.append(note);
            a.append((char) 0);
            SendInput(a);
        }

        if(map_key2[0][note]) {
            map_key2[0][note] = 0;
            a.clear();
            a.append(0x80 | 9);
            a.append(note);
            a.append((char) 0);
            SendInput(a);
        }

        if(map_key2[1][note]) {
            map_key2[1][note] = 0;
            a.clear();
            a.append(0x80 | 1);
            a.append(note);
            a.append((char) 0);
            SendInput(a);
        }

        if(map_key2[2][note]) {
            map_key2[2][note] = 0;
            a.clear();
            a.append(0x80 | 2);
            a.append(note);
            a.append((char) 0);
            SendInput(a);
        }

        if(map_key2[3][note]) {
            map_key2[3][note] = 0;
            a.clear();
            a.append(0x80 | 3);
            a.append(note);
            a.append((char) 0);
            SendInput(a);
        }
    }

}

bool PlayerSequencer::isFile(int index)
{
    if(index < 0 || index > (3))
        return false;

    return file[index] != NULL;
}

void PlayerSequencer::setMode(int index, unsigned int buttons)
{
    if(index > (3))
        return;

    mutex_player.lock();

    current_index = -1;
    position = 0;

    bool switch_on = (buttons & SEQ_FLAG_SWITCH_ON) ? true : false;

    buttons &= ~SEQ_FLAG_SWITCH_ON;

    MidiInput::note_roll[_chan].clear();

    if(index < 0 || !file[index]) {
        MidiOutput::sequencer_enabled[_chan] = -1;

        MidiOutput::sequencer_cmd[_chan] = SEQ_FLAG_STOP;

        if(index < 0) {
            if(autorhythm)
                _force_flush_mode = 2;
            else
                _force_flush_mode = 1;
        }

        autorhythm = false;

        mutex_player.unlock();
        return;
    }


    buttons &= ~(SEQ_FLAG_ON | SEQ_FLAG_ON2 | SEQ_FLAG_STOP);

    _buttons[index] = buttons;

    if(buttons & SEQ_FLAG_AUTORHYTHM) {
        autorhythm = true;
        MidiOutput::sequencer_cmd[_chan] = buttons | SEQ_FLAG_ON;
    } else {
        autorhythm = false;
        MidiOutput::sequencer_cmd[_chan] = buttons;
    }

    if(switch_on)
        MidiOutput::sequencer_enabled[_chan] = index;
    else
        MidiOutput::sequencer_enabled[_chan] = -1;

    mutex_player.unlock();
}

void PlayerSequencer::unsetFile(int index)
{

    if(index > (3))
        return;

    mutex_player.lock();

    int enabled = MidiOutput::sequencer_enabled[_chan];

    for(int n = 0; n < 4; n++) {

        if(n != index && index >= 0)
            continue;

        if(file[n])
            delete file[n];

        file[n] = NULL;

    }

    if(enabled == index && index >= 0)
        flush();

    mutex_player.unlock();
}

void PlayerSequencer::setFile(MidiFile* f, int index)
{
    if(index < 0 || index > (3))
        return;

    mutex_player.lock();

    if(file[index])
        delete file[index];

    file[index] = f;

    events[index] = file[index]->playerData();

    position = 0;

    enable = true;

    mutex_player.unlock();
}

void PlayerSequencer::SendInput(QByteArray a)
{
    _device = DEVICE_SEQUENCER_ID + _chan;
    std::vector<unsigned char> v;

    for(int n = 0; n < a.length(); n++)
        v.emplace_back((unsigned char) a[n]);

    MidiInput::receiveMessage_mutex(MidiInput::currentTime(), &v, (void*) &_device);

}


void PlayerSequencer::loop(qint64 diff_t)
{
    bool force_stop_loop = false;
    int newPos = position;
    QMultiMap<int, MidiEvent*>::iterator it;

    if(diff_t == -1) {

        QCoreApplication::processEvents();

    }

    if(_force_flush_mode) { // force flush notes in delayed sequencer stop

        bool save = autorhythm;

        if(_force_flush_mode == 1)
            autorhythm = false;
        else
            autorhythm = true;

        flush();

        autorhythm = save;

        _force_flush_mode = 0;
    }


    if(!enable || (current_index >= 0 && (!file[current_index] || !events[current_index])))
        return;

    MidiInput::mutex_input.lock();
    QByteArray note_roll = MidiInput::note_roll[_chan];
    int enabled = MidiOutput::sequencer_enabled[_chan];
    int cmd = MidiOutput::sequencer_cmd[_chan];

    if(enabled >= 0 && autorhythm && (cmd & SEQ_FLAG_AUTORHYTHM)) {
        cmd &= ~SEQ_FLAG_AUTORHYTHM;
        cmd |= SEQ_FLAG_ON;
        MidiOutput::sequencer_cmd[_chan] = cmd;
        note_roll.clear();
        //note_roll.append((char) 60);
    }

    MidiInput::mutex_input.unlock();

    bool send_flush = false;

    if(cmd & SEQ_FLAG_ON) {
        note_count = 0;
        note_random = -1;
    }

    if(!autorhythm && current_index >= 0 && !(cmd & SEQ_FLAG_INFINITE) && !note_roll.length()) {
        position = -diff_t  * ((double) file[current_index]->scale_time_sequencer);
        flush();
    }

    // test flush note (from note off)
    for(int n = 0; n < 128; n++) {
        QByteArray a;
        if(map_key_status[n] == MAP_NOTE_FLUSH) {
            map_key_status[n] = MAP_NOTE_OFF;
            map_key[n] = 0;
            a.append(0x80 | 0);
            a.append(n);
            a.append((char) 0);
            SendInput(a);

            if(map_key2[0][n]) {
                map_key2[0][n] = 0;
                a.clear();
                a.append(0x80 | 9);
                a.append(n);
                a.append((char) 0);
                SendInput(a);
            }

            if(map_key2[1][n]) {
                map_key2[1][n] = 0;
                a.clear();
                a.append(0x80 | 1);
                a.append(n);
                a.append((char) 0);
                SendInput(a);
            }

            if(map_key2[2][n]) {
                map_key2[2][n] = 0;
                a.clear();
                a.append(0x80 | 2);
                a.append(n);
                a.append((char) 0);
                SendInput(a);
            }

            if(map_key2[3][n]) {
                map_key2[3][n] = 0;
                a.clear();
                a.append(0x80 | 3);
                a.append(n);
                a.append((char) 0);
                SendInput(a);
            }

        }
    }

    if(diff_t == -1) goto next;

    if(enabled >= 0 && enabled != current_index) {

        if(enabled < 0 || enabled >= 4 || !file[enabled] || !events[enabled]) {
            enabled = -1;
            //MidiOutput::sequencer_enabled[_chan] = enabled;
        } else {
            current_index = enabled;
            position = -diff_t  * ((double) file[current_index]->scale_time_sequencer);
            send_flush = true;
        }
    }

    if(current_index < 0)
        return;

    if(note_random >= 0)
        note_count+= INTERVAL_TIME;

    newPos = position + diff_t  * ((double) file[current_index]->scale_time_sequencer);

    it = events[current_index]->lowerBound(position);

    if(cmd && enabled >= 0 && (note_roll.length() || autorhythm))
        // it = events->lowerBound(position);
        while (it != events[current_index]->end() && it.key() < newPos) {

                   // save events for the given tick
            QList<MidiEvent *> onEv, offEv, ctrlEv, prgEv, sysEv;
            int sendPosition = it.key();

            do {
                SysExEvent* sys = dynamic_cast<SysExEvent*>(it.value());
                if (sys) {
                    sysEv.append(it.value());
                } else
                    if(!is_drum_only && it.value()->channel() == 0) {
                        TextEvent* textev = dynamic_cast<TextEvent*>(it.value());
                        ControlChangeEvent* ctrl = dynamic_cast<ControlChangeEvent*>(it.value());
                        ProgChangeEvent* prg = dynamic_cast<ProgChangeEvent*>(it.value());

                               // ignore text event
                        if(textev) ;

                        else if (ctrl) {
                            if(ctrl && ctrl->control() == 0) {
                                // it needs a real time prg change event
                                update_prg[ctrl->channel()] = true;
                            }
                            ctrlEv.append(it.value());
                        } else if (prg) {
                            prgEv.append(it.value());
                        } else if (it.value()->isOnEvent()) {

                            onEv.append(it.value());
                        } else {

                            offEv.append(it.value());
                        }
                } else if(((is_drum_only || use_drum || autorhythm) && it.value()->channel() == 9)
                         || (autorhythm && it.value()->channel() >= 1 && it.value()->channel() <= 3)) {
                            TextEvent* textev = dynamic_cast<TextEvent*>(it.value());
                            ControlChangeEvent* ctrl = dynamic_cast<ControlChangeEvent*>(it.value());
                            ProgChangeEvent* prg = dynamic_cast<ProgChangeEvent*>(it.value());

                                   // ignore text event
                            if(textev) ;

                            else if (ctrl) {
                                if(ctrl && ctrl->control() == 0) {
                                    // it needs a real time prg change event
                                    update_prg[ctrl->channel()] = true;
                                }
                                ctrlEv.append(it.value());
                            } else if (prg) {
                                prgEv.append(it.value());
                            } else if (it.value()->isOnEvent()) {

                                onEv.append(it.value());
                            } else {

                                offEv.append(it.value());
                            }
                        }

                it++;
            } while (it != events[current_index]->end() && it.key() == sendPosition);

            // SysEx event
#if 0
// don't send!
            foreach (MidiEvent* event, sysEv) {

                MidiOutput::sendCommand(event, 0);
            }
#endif
            // CtrlEv event

            foreach (MidiEvent* event, ctrlEv) {

                SendInput(event->save());
                //MidiOutput::sendCommand(event, track);
            }

                   // PrgEv event

            foreach (MidiEvent* event, prgEv) {

                SendInput(event->save());
            }

            if(offEv.count()) {
                if((cmd & 3) == SEQ_FLAG_ON) {
                    position = 0;
                    goto next;
                }
            }

            if(!autorhythm && !note_roll.length()) {
                send_flush = false;
                flush();

            } else foreach (MidiEvent* ev, offEv) {

                QByteArray a = ev->save();
                int note = a[1] & 127;
                if(ev->channel() == 0) {
                    for(int n = 0; (!autorhythm && n < note_roll.length()) || (autorhythm && n < 1); n++) {
                        int note2 = autorhythm ? 60 : (note_roll.at(n) & 127);

                        note2+= note - 60;
                        if(note2 < 0)
                            note2 = 0;
                        else if(note2 > 127)
                            note2 = 127;

                        map_key_status[note2] = MAP_NOTE_OFF;
                        map_key[note2] = 0;
                        a[1] = note2;
                        SendInput(a);

                    }
                } else { // drum

                    if((a[0] & 0xf0) == 0x80)
                        map_key2[ev->channel() == 9 ? 0 : (ev->channel() & 3)][note] = 0;

                    SendInput(a);
                }

            }

            if(send_flush) {
                send_flush = false;
                flush();
            }

            if(!autorhythm && !note_roll.length()) {
                note_count = 0;
                note_random = -1;
            } else {

                foreach (MidiEvent* ev, onEv) {

                if (ev->line() == MidiEvent::KEY_SIGNATURE_EVENT_LINE) {

                } else if (ev->line() == MidiEvent::TIME_SIGNATURE_EVENT_LINE) {

                } else if(ev->line() < 128) {

                    /* For some stupid reason fluidsynth has problems when changing banks and programs.
                       Sending program change just before first note works */

                    int chan = ev->channel();
                    QByteArray a = ev->save();
                    a[2] = (char) (((int) a[2] * volume[current_index]) / 127);

                    if(update_prg[chan]) {
                        update_prg[chan] = false;

                        QByteArray array;
                        array.clear();
                        array.append(0xC0 | chan); // program change
                        array.append(char(ev->file()->Prog_MIDI[chan]));
                        SendInput(array);

                    }

                    if(chan == 0) {

                        int note = a[1] & 127;

                        bool found = false;
                        int first_note =-1;

                        for(int n = 0; (!autorhythm && n < note_roll.length()) || (autorhythm && n < 1); n++) {
                            int note2 = autorhythm ? 60 : note_roll.at(n);
                            if(note2 == note_random) {
                                found = true;
                            }
                            note2&= 127;
                            int the_note = note2;

                            if(n == 0) first_note = note2;

                            note2+= note - 60;
                            if(note2 < 0)
                                note2 = 0;
                            else if(note2 > 127)
                                note2 = 127;

                            if(map_key_status[note2] != MAP_NOTE_ON) {
                                map_key_status[note2] = MAP_NOTE_ON;
                                map_key[note2] = the_note;
                                a[1] = note2;
                                //qWarning("note %i", note2);
                                SendInput(a);
                            } else {
                                map_key[note2] = the_note;
                            }
                        }

                        if(autorhythm) {
                            note_random = -1;
                            note_count = 0;
                        }

                        if( note_count >= 2000) {
                            if(cmd & SEQ_FLAG_INFINITE)
                                force_stop_loop = true;
                            note_random = -1;
                            note_count = 0;
                        } else if(!found) {
                            note_count = 0;
                            note_random = first_note;
                        }


                    } else { // drum
                        if((a[0] & 0xf0) == 0x90)
                            map_key2[chan == 9 ? 0 : (chan & 3)][a[1] & 127] = 1;
                        SendInput(a);
                    }
                }

            }
        }
    }

    if(force_stop_loop) cmd &= ~SEQ_FLAG_INFINITE;
    // end if it was last event, but only if not recording
    if (it == events[current_index]->end()) {

        if(cmd & SEQ_FLAG_LOOP) newPos = 0;
        cmd = (cmd & 0xfe);

    }
    position = newPos;

       //MidiInput::setTime(position);


next:
    int flag = (cmd & 7);

    if(flag == SEQ_FLAG_ON || flag == SEQ_FLAG_STOP || enabled < 0 ) {
        if(flag == SEQ_FLAG_STOP || enabled < 0)
            cmd = cmd & 0xf8;
        else
            cmd = (cmd & 0xfc) | SEQ_FLAG_ON2;

        send_flush = false;
        flush();

        position = 0;

    }

    MidiInput::mutex_input.lock();
    MidiOutput::sequencer_enabled[_chan] = enabled;
    MidiOutput::sequencer_cmd[_chan] = cmd;
    MidiInput::mutex_input.unlock();

    if(send_flush) {

        flush();
    }

}

int PlayerThread::timeMs()
{
    return position;
}
