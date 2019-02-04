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

#include "MidiFile.h"

#include <QDataStream>
#include <QFile>

#include "../MidiEvent/ControlChangeEvent.h"
#include "../MidiEvent/KeySignatureEvent.h"
#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/OffEvent.h"
#include "../MidiEvent/OnEvent.h"
#include "../MidiEvent/ProgChangeEvent.h"
#include "../MidiEvent/TempoChangeEvent.h"
#include "../MidiEvent/TextEvent.h"
#include "../MidiEvent/TimeSignatureEvent.h"
#include "../MidiEvent/UnknownEvent.h"
#include "../protocol/Protocol.h"
#include "MidiChannel.h"
#include "MidiTrack.h"
#include "math.h"

#include <QtCore/qmath.h>

int MidiFile::defaultTimePerQuarter = 192;

MidiFile::MidiFile() {
    _saved = true;
    midiTicks = 0;
    _cursorTick = 0;
    prot = new Protocol(this);
    prot->addEmptyAction("New file");
    _path = "";
    _pauseTick = -1;
    for (int i = 0; i < 19; i++) {
        channels[i] = new MidiChannel(this, i);
    }

    timePerQuarter = MidiFile::defaultTimePerQuarter;
    _midiFormat = 1;

    _tracks = new QList<MidiTrack*>();
    MidiTrack* tempoTrack = new MidiTrack(this);
    tempoTrack->setName(tr("Tempo Track"));
    tempoTrack->setNumber(0);
    _tracks->append(tempoTrack);

    MidiTrack* instrumentTrack = new MidiTrack(this);
    instrumentTrack->setName(tr("New Instrument"));
    instrumentTrack->setNumber(1);
    _tracks->append(instrumentTrack);

    connect(tempoTrack, SIGNAL(trackChanged()), this, SIGNAL(trackChanged()));
    connect(instrumentTrack, SIGNAL(trackChanged()), this, SIGNAL(trackChanged()));

    // add timesig
    TimeSignatureEvent* timeSig = new TimeSignatureEvent(18, 4, 2, 24, 8, tempoTrack);
    timeSig->setFile(this);
    channel(18)->eventMap()->insert(0, timeSig);

    // create tempo change
    TempoChangeEvent* tempoEv = new TempoChangeEvent(17, 500000, tempoTrack);
    tempoEv->setFile(this);
    channel(17)->eventMap()->insert(0, tempoEv);

    playerMap = new QMultiMap<int, MidiEvent*>;

    midiTicks = 7680;
    calcMaxTime();
}

MidiFile::MidiFile(QString path, bool* ok, QStringList* log) {

    if (!log) {
        log = new QStringList();
    }

    _pauseTick = -1;
    _saved = true;
    midiTicks = 0;
    _cursorTick = 0;
    prot = new Protocol(this);
    prot->addEmptyAction(tr("File opened"));
    _path = path;
    _tracks = new QList<MidiTrack*>();
    QFile* f = new QFile(path);

    if (!f->open(QIODevice::ReadOnly)) {
        *ok = false;
        log->append(tr("Error: File could not be opened."));
        printLog(log);
        return;
    }

    for (int i = 0; i < 19; i++) {
        channels[i] = new MidiChannel(this, i);
    }

    QDataStream* stream = new QDataStream(f);
    stream->setByteOrder(QDataStream::BigEndian);
    if (!readMidiFile(stream, log)) {
        *ok = false;
        printLog(log);
        return;
    }

    *ok = true;
    playerMap = new QMultiMap<int, MidiEvent*>;
    calcMaxTime();
    printLog(log);
}

MidiFile::MidiFile(int ticks, Protocol* p) {
    midiTicks = ticks;
    prot = p;
}

bool MidiFile::readMidiFile(QDataStream* content, QStringList* log) {

    OffEvent::clearOnEvents();

    quint8 tempByte;

    QString badHeader = tr("Error: Bad format in file header (Expected MThd).");
    (*content) >> tempByte;
    if (tempByte != 'M') {
        log->append(badHeader);
        return false;
    }
    (*content) >> tempByte;
    if (tempByte != 'T') {
        log->append(badHeader);
        return false;
    }
    (*content) >> tempByte;
    if (tempByte != 'h') {
        log->append(badHeader);
        return false;
    }
    (*content) >> tempByte;
    if (tempByte != 'd') {
        log->append(badHeader);
        return false;
    }

    quint32 MThdTrackLength;
    (*content) >> MThdTrackLength;
    if (MThdTrackLength != 6) {
        log->append(tr("Error: MThdTrackLength wrong (expected 6)."));
        return false;
    }

    quint16 midiFormat;
    (*content) >> midiFormat;
    if (midiFormat > 1) {
        log->append(tr("Error: MidiFormat v. 2 cannot be loaded with this Editor."));
        return false;
    }

    _midiFormat = midiFormat;

    quint16 numTracks;
    (*content) >> numTracks;

    quint16 basisVelocity;
    (*content) >> basisVelocity;
    timePerQuarter = (int)basisVelocity;

    bool ok;
    for (int num = 0; num < numTracks; num++) {
        ok = readTrack(content, num, log);
        if (!ok) {
            log->append(tr("Error in Track ") + QString::number(num));
            return false;
        }
    }

    // find corrupted OnEvents (without OffEvent)
    foreach (OnEvent* onevent, OffEvent::corruptedOnEvents()) {
        log->append(tr("Warning: found OnEvent without OffEvent (line ") + QString::number(onevent->line()) + tr(") - removing..."));
        channel(onevent->channel())->removeEvent(onevent);
    }

    OffEvent::clearOnEvents();

    return true;
}

bool MidiFile::readTrack(QDataStream* content, int num, QStringList* log) {

    quint8 tempByte;

    QString badHeader = tr("Error: Bad format in track header (Track ") + QString::number(num) + tr(", Expected MTrk).");
    (*content) >> tempByte;
    if (tempByte != 'M') {
        log->append(badHeader);
        return false;
    }
    (*content) >> tempByte;
    if (tempByte != 'T') {
        log->append(badHeader);
        return false;
    }
    (*content) >> tempByte;
    if (tempByte != 'r') {
        log->append(badHeader);
        return false;
    }
    (*content) >> tempByte;
    if (tempByte != 'k') {
        log->append(badHeader);
        return false;
    }

    quint32 numBytes;
    (*content) >> numBytes;

    bool ok = true;
    bool endEvent = false;
    int position = 0;

    MidiTrack* track = new MidiTrack(this);
    track->setNumber(num);

    _tracks->append(track);
    connect(track, SIGNAL(trackChanged()), this, SIGNAL(trackChanged()));

    int channelFrequency[16];
    for (int i = 0; i < 16; i++) {
        channelFrequency[i] = 0;
    }

    while (!endEvent) {

        position += deltaTime(content);

        MidiEvent* event = MidiEvent::loadMidiEvent(content, &ok, &endEvent, track);
        if (!ok) {
            return false;
        }

        OffEvent* offEvent = dynamic_cast<OffEvent*>(event);
        if (offEvent && !offEvent->onEvent()) {
            log->append(tr("Warning: detected offEvent without prior onEvent. Skipping!"));
            continue;
        }
        // check whether its the tracks name
        if (event && event->line() == MidiEvent::TEXT_EVENT_LINE) {
            TextEvent* textEvent = dynamic_cast<TextEvent*>(event);
            if (textEvent) {
                if (textEvent->type() == TextEvent::TRACKNAME) {
                    track->setNameEvent(textEvent);
                }
            }
        }

        if (endEvent) {
            if (midiTicks < position) {
                midiTicks = position;
            }
            break;
        }
        if (!event) {
            return false;
        }

        event->setFile(this);

        // also inserts to the Map of the channel
        event->setMidiTime(position, false);

        if (event->channel() < 16) {
            channelFrequency[event->channel()]++;
        }
    }

    // end of track
    (*content) >> tempByte;
    QString errorText = tr("Error: track ") + QString::number(num) + tr("not ended as expected. ");
    if (tempByte != 0x00) {
        log->append(errorText);
        return false;
    }

    // check whether TimeSignature at tick 0 is given. If not, create one.
    // this will be done after reading the first track
    if (!channel(18)->eventMap()->contains(0)) {
        log->append(tr("Warning: no TimeSignatureEvent detected at tick 0. Adding default value."));
        TimeSignatureEvent* timeSig = new TimeSignatureEvent(18, 4, 2, 24, 8, track);
        timeSig->setFile(this);
        timeSig->setTrack(track, false);
        channel(18)->eventMap()->insert(0, timeSig);
    }

    // check whether TempoChangeEvent at tick 0 is given. If not, create one.
    if (!channel(17)->eventMap()->contains(0)) {
        log->append("Warning: no TempoChangeEvent detected at tick 0. Adding default value.");
        TempoChangeEvent* tempoEv = new TempoChangeEvent(17, 500000, track);
        tempoEv->setFile(this);
        tempoEv->setTrack(track, false);
        channel(17)->eventMap()->insert(0, tempoEv);
    }

    // assign channel
    int assignedChannel = 0;
    for (int i = 1; i < 16; i++) {
        if (channelFrequency[i] > channelFrequency[assignedChannel]) {
            assignedChannel = i;
        }
    }
    track->assignChannel(assignedChannel);

    return true;
}

int MidiFile::deltaTime(QDataStream* content) {
    return variableLengthvalue(content);
}

int MidiFile::variableLengthvalue(QDataStream* content) {
    quint32 v = 0;
    quint8 byte = 0;
    do {
        (*content) >> byte;
        v <<= 7;
        v |= (byte & 0x7F);
    } while (byte & (1 << 7));
    return (int)v;
}

QMap<int, MidiEvent*>* MidiFile::timeSignatureEvents() {
    return channels[18]->eventMap();
}

QMap<int, MidiEvent*>* MidiFile::tempoEvents() {
    return channels[17]->eventMap();
}

void MidiFile::calcMaxTime() {
    double time = 0;
    QList<MidiEvent*> events = channels[17]->eventMap()->values();
    for (int i = 0; i < events.length(); i++) {
        TempoChangeEvent* ev = dynamic_cast<TempoChangeEvent*>(events.at(i));
        if (!ev) {
            continue;
        }
        int ticks = 0;
        if (i < events.length() - 1) {
            ticks = events.at(i + 1)->midiTime() - ev->midiTime();
        } else {
            //ticks = 0;
            ticks = midiTicks - ev->midiTime();
        }
        time += ticks * ev->msPerTick();
    }
    maxTimeMS = time;
    emit recalcWidgetSize();
}

int MidiFile::maxTime() {
    return maxTimeMS;
}

int MidiFile::endTick() {
    return midiTicks;
}

int MidiFile::tick(int ms) {
    double time = 0;

    QList<MidiEvent*> events = channels[17]->eventMap()->values();
    TempoChangeEvent* event = 0;

    double timeMsNextEvent = 0;

    for (int i = 0; i < events.length(); i++) {

        TempoChangeEvent* ev = dynamic_cast<TempoChangeEvent*>(events.at(i));
        if (!ev) {
            qWarning("unknown eventtype in the List [3]");
            continue;
        }
        event = ev;
        time = timeMsNextEvent;

        int ticks = 0;
        if (i < events.length() - 1) {
            ticks = events.at(i + 1)->midiTime() - ev->midiTime();
        } else {
            break;
        }

        timeMsNextEvent += ticks * ev->msPerTick();
        if (timeMsNextEvent > ms) {
            break;
        }
    }

    if (!event) {
        return 0;
    }

    int startTick = (ms - time) / event->msPerTick() + event->midiTime();
    return startTick;
}

int MidiFile::msOfTick(int tick, QList<MidiEvent*>* events, int
                       msOfFirstEventInList) {

    if (!events) {
        events = new QList<MidiEvent*>(channels[17]->eventMap()->values());
        if (!events) {
            return 0;
        }
    }

    // timeMs holds the time of the current tick
    double timeMs = 0;

    // event is the previous TempoChangeEvent in the list, ev the current
    TempoChangeEvent* event = 0;
    for (int i = 0; i < events->length(); i++) {
        TempoChangeEvent* ev = dynamic_cast<TempoChangeEvent*>(events->at(i));
        if (!ev) {
            continue;
        }
        if (!event || ev->midiTime() <= tick) {
            if (!event) {
                // first event in the list at time msOfFirstEventInList
                timeMs = msOfFirstEventInList;
            } else {
                // any event before the endTick
                timeMs += event->msPerTick() * (ev->midiTime() - event->midiTime());
            }
            event = ev;
        } else {
            // end: ev is later than the endTick
            break;
        }
    }

    if (!event) {
        return 0;
    }

    timeMs += event->msPerTick() * (tick - event->midiTime());
    return (int)timeMs;
}

int MidiFile::tick(int startms, int endms, QList<MidiEvent*>** eventList,
                   int* endTick, int* msOfFirstEvent) {
    // holds the time of the current event
    double time = 0;

    // delete old eventList, create a new
    if ((*eventList)) {
        delete (*eventList);
    }
    *eventList = new QList<MidiEvent*>;

    // TempoChangeEvents
    QList<MidiEvent*> events = channels[17]->eventMap()->values();

    // event is the previous Event in events, ev the current
    TempoChangeEvent* event = 0;

    // necessary for the end condition
    double timeMsNextEvent = 0;

    // find the startEvent and the firstTick
    int i = 0;
    for (; i < events.length(); i++) {

        TempoChangeEvent* ev = dynamic_cast<TempoChangeEvent*>(events.at(i));
        if (!ev) {
            qWarning("unknown eventtype in the List [1]");
            continue;
        }
        event = ev;
        time = timeMsNextEvent;

        int ticks = 0;
        if (i < events.length() - 1) {
            ticks = events.at(i + 1)->midiTime() - ev->midiTime();
        } else {
            break;
        }

        timeMsNextEvent += ticks * ev->msPerTick();
        if (timeMsNextEvent > startms) {
            break;
        }
    }
    int startTick = (startms - time) / event->msPerTick() + event->midiTime();
    *msOfFirstEvent = time;
    (*eventList)->append(event);

    i++;

    // find the endEvent, save all events between start and end in the list and
    // get the endTick
    for (; i < events.length() && timeMsNextEvent < endms; i++) {

        TempoChangeEvent* ev = dynamic_cast<TempoChangeEvent*>(events.at(i));
        if (!ev) {
            continue;
        }
        event = ev;
        if (!(*eventList)->contains(event)) {
            (*eventList)->append(event);
        }

        time = timeMsNextEvent;

        int ticks = 0;
        if (i < events.length() - 1) {
            ticks = events.at(i + 1)->midiTime() - ev->midiTime();
        } else {
            break;
        }

        timeMsNextEvent += ticks * ev->msPerTick();
        if (timeMsNextEvent > endms) {
            break;
        }
    }

    if (!event) {
        return 0;
    }

    *endTick = (endms - time) / event->msPerTick() + event->midiTime();
    return startTick;
}

int MidiFile::numTracks() {
    return _tracks->size();
}

int MidiFile::measure(int startTick, int endTick,
                      QList<TimeSignatureEvent*>** eventList, int* ticksInmeasure) {

    if ((*eventList)) {
        delete (*eventList);
    }
    *eventList = new QList<TimeSignatureEvent*>;
    QList<MidiEvent*> events = channels[18]->eventMap()->values();
    TimeSignatureEvent* event = 0;
    int i = 0;
    int measure = 1;

    // find the startEvent and the firstTick
    for (; i < events.length(); i++) {

        TimeSignatureEvent* ev = dynamic_cast<TimeSignatureEvent*>(events.at(i));
        if (!ev) {
            qWarning("unknown eventtype in the List [2]");
            continue;
        }
        if (!event) {
            event = ev;
            measure = 1;
            continue;
        } else {
            if (ev->midiTime() <= startTick) {
                int ticks = ev->midiTime() - event->midiTime();
                measure += event->measures(ticks);
                event = ev;
            } else {
                break;
            }
        }
    }
    int ticks = startTick - event->midiTime();
    measure += event->measures(ticks, ticksInmeasure);
    (*eventList)->append(event);

    // find the endEvent, save all events between start and end in the list and
    // get the endTick
    for (; i < events.length(); i++) {

        TimeSignatureEvent* ev = dynamic_cast<TimeSignatureEvent*>(events.at(i));

        if (!ev) {
            qWarning("unknown eventtype in the List [2]");
            continue;
        }
        if (ev->midiTime() <= endTick) {
            (*eventList)->append(ev);
        } else {
            break;
        }
    }
    return measure;
}

int MidiFile::ticksPerQuarter() {
    return timePerQuarter;
}

QMultiMap<int, MidiEvent*>* MidiFile::channelEvents(int channel) {
    return channels[channel]->eventMap();
}

Protocol* MidiFile::protocol() {
    return prot;
}

MidiChannel* MidiFile::channel(int i) {
    return channels[i];
}

QString MidiFile::instrumentName(int prog) {
    switch (prog + 1) {
        case 1: {
            return tr("Acoustic Grand Piano");
        }
        case 2: {
            return tr("Bright Acoustic Piano");
        }
        case 3: {
            return tr("Electric Grand Piano");
        }
        case 4: {
            return tr("Honky-tonk Piano");
        }
        case 5: {
            return tr(" Electric Piano 1");
        }
        case 6: {
            return tr("Electric Piano 2");
        }
        case 7: {
            return tr("Harpsichord");
        }
        case 8: {
            return tr("Clavinet (Clavi)");
        }
        case 9: {
            return tr("Celesta");
        }
        case 10: {
            return tr("Glockenspiel");
        }
        case 11: {
            return tr("Music Box");
        }
        case 12: {
            return tr("Vibraphone");
        }
        case 13: {
            return tr("Marimba");
        }
        case 14: {
            return tr("Xylophone");
        }
        case 15: {
            return tr("Tubular Bells");
        }
        case 16: {
            return tr("Dulcimer");
        }
        case 17: {
            return tr("Drawbar Organ");
        }
        case 18: {
            return tr("Percussive Organ");
        }
        case 19: {
            return tr("Rock Organ");
        }
        case 20: {
            return tr("Church Organ");
        }
        case 21: {
            return tr("Reed Organ");
        }
        case 22: {
            return tr("Accordion");
        }
        case 23: {
            return tr("Harmonica");
        }
        case 24: {
            return tr("Tango Accordion");
        }
        case 25: {
            return tr("Acoustic Guitar (nylon)");
        }
        case 26: {
            return tr("Acoustic Guitar (steel)");
        }
        case 27: {
            return tr("Electric Guitar (jazz)");
        }
        case 28: {
            return tr("Electric Guitar (clean)");
        }
        case 29: {
            return tr("Electric Guitar (muted)");
        }
        case 30: {
            return tr("Overdriven Guitar");
        }
        case 31: {
            return tr("Distortion Guitar");
        }
        case 32: {
            return tr("Guitar harmonics");
        }
        case 33: {
            return tr("Acoustic Bass");
        }
        case 34: {
            return tr("Electric Bass (finger)");
        }
        case 35: {
            return tr("Electric Bass (pick)");
        }
        case 36: {
            return tr("Fretless Bass");
        }
        case 37: {
            return tr("Slap Bass 1");
        }
        case 38: {
            return tr("Slap Bass 2");
        }
        case 39: {
            return tr("Synth Bass 1");
        }
        case 40: {
            return tr("Synth Bass 2");
        }
        case 41: {
            return tr("Violin");
        }
        case 42: {
            return tr("Viola");
        }
        case 43: {
            return tr("Cello");
        }
        case 44: {
            return tr("Contrabass");
        }
        case 45: {
            return tr("Tremolo Strings");
        }
        case 46: {
            return tr("Pizzicato Strings");
        }
        case 47: {
            return tr("Orchestral Harp");
        }
        case 48: {
            return tr("Timpani");
        }
        case 49: {
            return tr("String Ensemble 1");
        }
        case 50: {
            return tr("String Ensemble 2");
        }
        case 51: {
            return tr("Synth Strings 1");
        }
        case 52: {
            return tr("Synth Strings 2");
        }
        case 53: {
            return tr("Choir Aahs");
        }
        case 54: {
            return tr("Voice Oohs");
        }
        case 55: {
            return tr("Synth Choir");
        }
        case 56: {
            return tr("Orchestra Hit");
        }
        case 57: {
            return tr("Trumpet");
        }
        case 58: {
            return tr("Trombone");
        }
        case 59: {
            return tr("Tuba");
        }
        case 60: {
            return tr("Muted Trumpet");
        }
        case 61: {
            return tr("French Horn");
        }
        case 62: {
            return tr("Brass Section");
        }
        case 63: {
            return tr("Synth Brass 1");
        }
        case 64: {
            return tr("Synth Brass 2");
        }
        case 65: {
            return tr("Soprano Sax");
        }
        case 66: {
            return tr("Alto Sax");
        }
        case 67: {
            return tr("Tenor Sax");
        }
        case 68: {
            return tr("Baritone Sax");
        }
        case 69: {
            return tr("Oboe");
        }
        case 70: {
            return tr("English Horn");
        }
        case 71: {
            return tr("Bassoon");
        }
        case 72: {
            return tr("Clarinet");
        }
        case 73: {
            return tr("Piccolo");
        }
        case 74: {
            return tr("Flute");
        }
        case 75: {
            return tr("Recorder");
        }
        case 76: {
            return tr("Pan Flute");
        }
        case 77: {
            return tr("Blown Bottle");
        }
        case 78: {
            return tr("Shakuhachi");
        }
        case 79: {
            return tr("Whistle");
        }
        case 80: {
            return tr("Ocarina");
        }
        case 81: {
            return tr("Lead 1 (square)");
        }
        case 82: {
            return tr("Lead 2 (sawtooth)");
        }
        case 83: {
            return tr("Lead 3 (calliope)");
        }
        case 84: {
            return tr("Lead 4 (chiff)");
        }
        case 85: {
            return tr("Lead 5 (charang)");
        }
        case 86: {
            return tr("Lead 6 (voice)");
        }
        case 87: {
            return tr("Lead 7 (fifths)");
        }
        case 88: {
            return tr("Lead 8 (bass + lead)");
        }
        case 89: {
            return tr("Pad 1 (new age)");
        }
        case 90: {
            return tr("Pad 2 (warm)");
        }
        case 91: {
            return tr("Pad 3 (polysynth)");
        }
        case 92: {
            return tr("Pad 4 (choir)");
        }
        case 93: {
            return tr("Pad 5 (bowed)");
        }
        case 94: {
            return tr("Pad 6 (metallic)");
        }
        case 95: {
            return tr("Pad 7 (halo)");
        }
        case 96: {
            return tr("Pad 8 (sweep)");
        }
        case 97: {
            return tr("FX 1 (rain)");
        }
        case 98: {
            return tr("FX 2 (soundtrack)");
        }
        case 99: {
            return tr("FX 3 (crystal)");
        }
        case 100: {
            return tr("FX 4 (atmosphere)");
        }
        case 101: {
            return tr("FX 5 (brightness)");
        }
        case 102: {
            return tr("FX 6 (goblins)");
        }
        case 103: {
            return tr("FX 7 (echoes)");
        }
        case 104: {
            return tr("FX 8 (sci-fi)");
        }
        case 105: {
            return tr("Sitar");
        }
        case 106: {
            return tr("Banjo");
        }
        case 107: {
            return tr("Shamisen");
        }
        case 108: {
            return tr("Koto");
        }
        case 109: {
            return tr("Kalimba");
        }
        case 110: {
            return tr("Bag pipe");
        }
        case 111: {
            return tr("Fiddle");
        }
        case 112: {
            return tr("Shanai");
        }
        case 113: {
            return tr("Tinkle Bell");
        }
        case 114: {
            return tr("Agogo");
        }
        case 115: {
            return tr("Steel Drums");
        }
        case 116: {
            return tr("Woodblock");
        }
        case 117: {
            return tr("Taiko Drum");
        }
        case 118: {
            return tr("Melodic Tom");
        }
        case 119: {
            return tr("Synth Drum");
        }
        case 120: {
            return tr("Reverse Cymbal");
        }
        case 121: {
            return tr("Guitar Fret Noise");
        }
        case 122: {
            return tr("Breath Noise");
        }
        case 123: {
            return tr("Seashore");
        }
        case 124: {
            return tr("Bird Tweet");
        }
        case 125: {
            return tr("Telephone Ring");
        }
        case 126: {
            return tr("Helicopter");
        }
        case 127: {
            return tr("Applause");
        }
        case 128: {
            return tr("Gunshot");
        }
    }
    return tr("out of range");
}

QString MidiFile::controlChangeName(int control) {

    switch (control) {
        case 0: {
            return tr("Bank Select (MSB)");
        }
        case 1: {
            return tr("Modulation Wheel (MSB)");
        }
        case 2: {
            return tr("Breath Controller (MSB) ");
        }

        case 4: {
            return tr("Foot Controller (MSB)");
        }
        case 5: {
            return tr("Portamento Time (MSB)");
        }
        case 6: {
            return tr("Data Entry (MSB)");
        }
        case 7: {
            return tr("Channel Volume (MSB)");
        }
        case 8: {
            return tr("Balance (MSB) ");
        }

        case 10: {
            return tr("Pan (MSB)");
        }
        case 11: {
            return tr("Expression (MSB)");
        }
        case 12: {
            return tr("Effect Control 1 (MSB)");
        }
        case 13: {
            return tr("Effect Control 2 (MSB) ");
        }

        case 16: {
            return tr("General Purpose Controller 1 (MSB)");
        }
        case 17: {
            return tr("General Purpose Controller 2 (MSB)");
        }
        case 18: {
            return tr("General Purpose Controller 3 (MSB)");
        }
        case 19: {
            return tr("General Purpose Controller 4 (MSB) ");
        }

        case 32: {
            return tr("Bank Select (LSB)");
        }
        case 33: {
            return tr("Modulation Wheel (LSB)");
        }
        case 34: {
            return tr("Breath Controller (LSB) ");
        }

        case 36: {
            return tr("Foot Controller (LSB)");
        }
        case 37: {
            return tr("Portamento Time (LSB)");
        }
        case 38: {
            return tr("Data Entry (LSB) Channel");
        }
        case 39: {
            return tr("Channel Volume (LSB)");
        }
        case 40: {
            return tr("Balance (LSB) ");
        }

        case 42: {
            return tr("Pan (LSB)");
        }
        case 43: {
            return tr("Expression (LSB)");
        }
        case 44: {
            return tr("Effect Control 1 (LSB)");
        }
        case 45: {
            return tr("Effect Control 2 (LSB) ");
        }

        case 48: {
            return tr("General Purpose Controller 1 (LSB)");
        }
        case 49: {
            return tr("General Purpose Controller 2 (LSB)");
        }
        case 50: {
            return tr("General Purpose Controller 3 (LSB)");
        }
        case 51: {
            return tr("General Purpose Controller 4 (LSB) ");
        }

        case 64: {
            return tr("Sustain Pedal");
        }
        case 65: {
            return tr("Portamento On/Off");
        }
        case 66: {
            return tr("Sostenuto");
        }
        case 67: {
            return tr("Soft Pedal");
        }
        case 68: {
            return tr("Legato Footswitch");
        }
        case 69: {
            return tr("Hold 2");
        }
        case 70: {
            return tr("Sound Controller 1");
        }
        case 71: {
            return tr("Sound Controller 2");
        }
        case 72: {
            return tr("Sound Controller 3");
        }
        case 73: {
            return tr("Sound Controller 4");
        }
        case 74: {
            return tr("Sound Controller 5");
        }
        case 75: {
            return tr("Sound Controller 6");
        }
        case 76: {
            return tr("Sound Controller 7");
        }
        case 77: {
            return tr("Sound Controller 8");
        }
        case 78: {
            return tr("Sound Controller 9");
        }
        case 79: {
            return tr("Sound Controller 10 (GM2 default: Undefined)");
        }
        case 80: {
            return tr("General Purpose Controller 5");
        }
        case 81: {
            return tr("General Purpose Controller 6");
        }
        case 82: {
            return tr("General Purpose Controller 7");
        }
        case 83: {
            return tr("General Purpose Controller 8");
        }
        case 84: {
            return tr("Portamento Control ");
        }

        case 91: {
            return tr("Effects 1 Depth (default: Reverb Send)");
        }
        case 92: {
            return tr("Effects 2 Depth (default: Tremolo Depth)");
        }
        case 93: {
            return tr("Effects 3 Depth (default: Chorus Send)");
        }
        case 94: {
            return tr("Effects 4 Depth (default: Celeste [Detune] Depth)");
        }
        case 95: {
            return tr("Effects 5 Depth (default: Phaser Depth)");
        }
        case 96: {
            return tr("Data Increment");
        }
        case 97: {
            return tr("Data Decrement");
        }
        case 98: {
            return tr("Non-Registered Parameter Number (LSB)");
        }
        case 99: {
            return tr("Non-Registered Parameter Number(MSB)");
        }
        case 100: {
            return tr("Registered Parameter Number (LSB)");
        }
        case 101: {
            return tr("Registered Parameter Number(MSB) ");
        }

        case 120: {
            return tr("All Sound Off");
        }
        case 121: {
            return tr("Reset All Controllers");
        }
        case 122: {
            return tr("Local Control On/Off");
        }
        case 123: {
            return tr("All Notes Off");
        }
        case 124: {
            return tr("Omni Mode Off");
        }
        case 125: {
            return tr("Omni Mode On");
        }
        case 126: {
            return tr("Poly Mode Off");
        }
        case 127: {
            return tr("Poly Mode On");
        }
    }

    return tr("undefined");
}

QList<MidiEvent*>* MidiFile::eventsBetween(int start, int end) {
    QList<MidiEvent*>* eventList = new QList<MidiEvent*>;
    for (int i = 0; i < 19; i++) {
        QMultiMap<int, MidiEvent*>* events = channels[i]->eventMap();
        QMultiMap<int, MidiEvent*>::iterator current = events->lowerBound(start);
        QMultiMap<int, MidiEvent*>::iterator upperBound = events->upperBound(end);
        while (current != upperBound) {
            if (!eventList->contains(current.value())) {
                eventList->append(current.value());
            }
            current++;
        }
    }
    return eventList;
}

bool MidiFile::channelMuted(int ch) {

    // all general channels
    if (ch > 15) {
        return false;
    }

    // check solochannel
    for (int i = 0; i < 17; i++) {
        if (channel(i)->solo()) {
            return i != ch;
        }
    }

    return channel(ch)->mute();
}

void MidiFile::preparePlayerData(int tickFrom) {

    playerMap->clear();
    QList<MidiEvent*>* prgList;

    for (int i = 0; i < 19; i++) {

        if (channelMuted(i)) {
            continue;
        }

        // prgList saves all ProgramChangeEvents before cursorPosition. The last
        // will be sent when playing
        prgList = new QList<MidiEvent*>;

        QMultiMap<int, MidiEvent*>* channelEvents = channels[i]->eventMap();
        QMultiMap<int, MidiEvent*>::iterator it = channelEvents->begin();

        while (it != channelEvents->end()) {
            int tick = it.key();
            MidiEvent* event = it.value();
            if (tick >= tickFrom) {
                // all Events after cursorTick are added
                int ms = msOfTick(tick);
                if (!event->track()->muted()) {
                    playerMap->insert(ms, event);
                }
            } else {
                ProgChangeEvent* prg = dynamic_cast<ProgChangeEvent*>(event);
                if (prg) {
                    // save ProgramChenges in the list, the last will be added
                    // to the playerMap later
                    prgList->append(prg);
                }
                ControlChangeEvent* ctrl = dynamic_cast<ControlChangeEvent*>(event);
                if (ctrl) {
                    // insert all ControlChanges on first position
                    // playerMap->insert(msOfTick(cursorTick())-1, ctrl);
                }
            }
            it++;
        }

        if (prgList->count() > 0) {
            // set the program of the channel
            playerMap->insert(msOfTick(tickFrom) - 1, prgList->last());
        }

        delete prgList;
        prgList = 0;
    }
}

QMultiMap<int, MidiEvent*>* MidiFile::playerData() {
    return playerMap;
}

int MidiFile::cursorTick() {
    return _cursorTick;
}

void MidiFile::setCursorTick(int tick) {
    _cursorTick = tick;
    emit cursorPositionChanged();
}

int MidiFile::pauseTick() {
    return _pauseTick;
}

void MidiFile::setPauseTick(int tick) {
    _pauseTick = tick;
}

bool MidiFile::save(QString path) {

    QFile* f = new QFile(path);

    if (!f->open(QIODevice::WriteOnly)) {
        return false;
    }

    QDataStream* stream = new QDataStream(f);
    stream->setByteOrder(QDataStream::BigEndian);

    // All Events are stored in allEvents. This is because the data has to be
    // saved by tracks and not by channels
    QMultiMap<int, MidiEvent*> allEvents = QMultiMap<int, MidiEvent*>();
    for (int i = 0; i < 19; i++) {
        QMultiMap<int, MidiEvent*>::iterator it = channels[i]->eventMap()->begin();
        while (it != channels[i]->eventMap()->end()) {
            allEvents.insert(it.key(), it.value());
            it++;
        }
    }

    QByteArray data = QByteArray();
    data.append('M');
    data.append('T');
    data.append('h');
    data.append('d');

    int trackL = 6;
    for (int i = 3; i >= 0; i--) {
        data.append((qint8)((trackL & (0xFF << 8 * i)) >> 8 * i));
    }

    for (int i = 1; i >= 0; i--) {
        data.append((qint8)((_midiFormat & (0xFF << 8 * i)) >> 8 * i));
    }

    for (int i = 1; i >= 0; i--) {
        data.append((qint8)((numTracks() & (0xFF << 8 * i)) >> 8 * i));
    }

    for (int i = 1; i >= 0; i--) {
        data.append((qint8)((timePerQuarter & (0xFF << 8 * i)) >> 8 * i));
    }

    for (int num = 0; num < numTracks(); num++) {

        data.append('M');
        data.append('T');
        data.append('r');
        data.append('k');

        int trackLengthPos = data.count();

        data.append('\0');
        data.append('\0');
        data.append('\0');
        data.append('\0');

        int numBytes = 0;
        int currentTick = 0;
        QMultiMap<int, MidiEvent*>::iterator it = allEvents.begin();
        while (it != allEvents.end()) {

            MidiEvent* event = it.value();
            int tick = it.key();

            if (_tracks->at(num) == event->track()) {

                // write the deltaTime before the event
                int time = tick - currentTick;
                QByteArray deltaTime = writeDeltaTime(time);
                numBytes += deltaTime.count();
                data.append(deltaTime);

                // write the events data
                QByteArray eventData = event->save();
                numBytes += eventData.count();
                data.append(eventData);

                // save this tick as last time
                currentTick = tick;
            }

            it++;
        }

        // write the endEvent
        int time = endTick() - currentTick;
        QByteArray deltaTime = writeDeltaTime(time);
        numBytes += deltaTime.count();
        data.append(deltaTime);
        data.append(char(0xFF));
        data.append(char(0x2F));
        data.append('\0');
        numBytes += 3;

        // write numBytes
        for (int i = 3; i >= 0; i--) {
            data[trackLengthPos + 3 - i] = ((qint8)((numBytes & (0xFF << 8 * i)) >> 8 * i));
        }
    }

    // write data to the filestream
    for (int i = 0; i < data.count(); i++) {
        (*stream) << (qint8)(data.at(i));
    }

    // close the file
    f->close();

    _saved = true;

    return true;
}

QByteArray MidiFile::writeDeltaTime(int time) {
    return writeVariableLengthValue(time);
}

QByteArray MidiFile::writeVariableLengthValue(int value) {

    QByteArray array = QByteArray();

    bool isFirst = true;
    for (int i = 3; i >= 0; i--) {
        int b = value >> (7 * i);
        qint8 byte = (qint8)b & 127;
        if (!isFirst || byte > 0 || i == 0) {
            isFirst = false;
            if (i > 0) {
                // set 8th bit
                byte |= 128;
            }
            array.append(byte);
        }
    }

    return array;
}

QString MidiFile::path() {
    return _path;
}

void MidiFile::setPath(QString path) {
    _path = path;
}

bool MidiFile::saved() {
    return _saved;
}

void MidiFile::setSaved(bool b) {
    _saved = b;
}

void MidiFile::setMaxLengthMs(int ms) {
    ProtocolEntry* toCopy = copy();
    int oldTicks = midiTicks;
    midiTicks = tick(ms);
    ProtocolEntry::protocol(toCopy, this);
    if (midiTicks < oldTicks) {
        // remove events after maxTick
        QList<MidiEvent*>* ev = eventsBetween(midiTicks, oldTicks);
        foreach (MidiEvent* event, *ev) {
            channel(event->channel())->removeEvent(event);
        }
    }
    calcMaxTime();
}

ProtocolEntry* MidiFile::copy() {
    MidiFile* file = new MidiFile(midiTicks, protocol());
    file->_tracks = new QList<MidiTrack*>(*(_tracks));
    file->pasteTracks = pasteTracks;
    return file;
}

void MidiFile::reloadState(ProtocolEntry* entry) {
    MidiFile* file = dynamic_cast<MidiFile*>(entry);
    if (file) {
        midiTicks = file->midiTicks;
        _tracks = new QList<MidiTrack*>(*(file->_tracks));
        pasteTracks = file->pasteTracks;
    }
    calcMaxTime();
}

MidiFile* MidiFile::file() {
    return this;
}

QList<MidiTrack*>* MidiFile::tracks() {
    return _tracks;
}

void MidiFile::addTrack() {
    ProtocolEntry* toCopy = copy();
    MidiTrack* track = new MidiTrack(this);
    track->setNumber(_tracks->size());
    if (track->number() > 0) {
        track->assignChannel(track->number() - 1);
    }
    _tracks->append(track);
    track->setName("New Track");
    int n = 0;
    foreach (MidiTrack* track, *_tracks) {
        track->setNumber(n++);
    }
    ProtocolEntry::protocol(toCopy, this);
    connect(track, SIGNAL(trackChanged()), this, SIGNAL(trackChanged()));
}

bool MidiFile::removeTrack(MidiTrack* track) {

    if (numTracks() < 2) {
        return false;
    }

    ProtocolEntry* toCopy = copy();

    QMultiMap<int, MidiEvent*> allEvents = QMultiMap<int, MidiEvent*>();
    for (int i = 0; i < 19; i++) {
        QMultiMap<int, MidiEvent*>::iterator it = channels[i]->eventMap()->begin();
        while (it != channels[i]->eventMap()->end()) {
            allEvents.insert(it.key(), it.value());
            it++;
        }
    }

    _tracks->removeAll(track);

    QMultiMap<int, MidiEvent*>::iterator it = allEvents.begin();
    while (it != allEvents.end()) {
        MidiEvent* event = it.value();
        if (event->track() == track) {
            if (!channels[event->channel()]->removeEvent(event)) {
                event->setTrack(_tracks->first());
            }
        }
        it++;
    }

    // remove links from pasted tracks
    foreach (MidiFile* fileFrom, pasteTracks.keys()) {
        QList<MidiTrack*> sourcesToRemove;
        foreach (MidiTrack* source, pasteTracks.value(fileFrom).keys()) {
            if (pasteTracks.value(fileFrom).value(source) == track) {
                sourcesToRemove.append(source);
            }
        }
        QMap<MidiTrack*, MidiTrack*> tracks = pasteTracks.value(fileFrom);
        foreach (MidiTrack* source, sourcesToRemove) {
            tracks.remove(source);
        }
        pasteTracks.insert(fileFrom, tracks);
    }
    int n = 0;
    foreach (MidiTrack* track, *_tracks) {
        track->setNumber(n++);
    }

    ProtocolEntry::protocol(toCopy, this);

    return true;
}

MidiTrack* MidiFile::track(int number) {
    if (_tracks->size() > number) {
        return _tracks->at(number);
    } else {
        return 0;
    }
}

int MidiFile::tonalityAt(int tick) {
    QMultiMap<int, MidiEvent*>* events = channels[16]->eventMap();

    QMultiMap<int, MidiEvent*>::iterator it = events->begin();
    KeySignatureEvent* event = 0;
    while (it != events->end()) {
        KeySignatureEvent* keySig = dynamic_cast<KeySignatureEvent*>(it.value());
        if (keySig && keySig->midiTime() <= tick) {
            event = keySig;
        } else if (keySig) {
            break;
        }
        it++;
    }

    if (!event) {
        return 0;
    }

    else {
        return event->tonality();
    }
}

void MidiFile::meterAt(int tick, int* num, int* denum) {
    QMap<int, MidiEvent*>* meterEvents = timeSignatureEvents();
    QMap<int, MidiEvent*>::iterator it = meterEvents->begin();
    TimeSignatureEvent* event = 0;
    while (it != meterEvents->end()) {
        TimeSignatureEvent* timeSig = dynamic_cast<TimeSignatureEvent*>(it.value());
        if (timeSig && timeSig->midiTime() <= tick) {
            event = timeSig;
        } else if (timeSig) {
            break;
        }
        it++;
    }

    if (!event) {
        *num = 4;
        *denum = 4;
    }

    else {
        *num = event->num();
        *denum = event->denom();
    }
}

void MidiFile::printLog(QStringList* log) {
    foreach (QString str, *log) {
        qWarning(str.toUtf8().constData());
    }
}

void MidiFile::registerCopiedTrack(MidiTrack* source, MidiTrack* destination, MidiFile* fileFrom) {

    //  if(fileFrom == this){
    //      return;
    //  }

    ProtocolEntry* toCopy = copy();

    QMap<MidiTrack*, MidiTrack*> list;
    if (pasteTracks.contains(fileFrom)) {
        list = pasteTracks.value(fileFrom);
    }

    list.insert(source, destination);
    pasteTracks.insert(fileFrom, list);

    ProtocolEntry::protocol(toCopy, this);
}

MidiTrack* MidiFile::getPasteTrack(MidiTrack* source, MidiFile* fileFrom) {

    //  if(fileFrom == this){
    //      return source;
    //  }

    if (!pasteTracks.contains(fileFrom) || !pasteTracks.value(fileFrom).contains(source)) {
        return 0;
    }

    return pasteTracks.value(fileFrom).value(source);
}

QList<int> MidiFile::quantization(int fractionSize) {

    int fractionTicks = (4 * ticksPerQuarter()) / qPow(2, fractionSize);

    QList<int> list;

    QList<MidiEvent*> timeSigs = timeSignatureEvents()->values();
    TimeSignatureEvent* last = 0;
    foreach (MidiEvent* event, timeSigs) {

        TimeSignatureEvent* t = dynamic_cast<TimeSignatureEvent*>(event);

        if (last) {

            int current = last->midiTime();
            while (current < t->midiTime()) {
                list.append(current);
                current += fractionTicks;
            }
        }

        last = t;
    }

    if (last) {
        int current = last->midiTime();
        while (current <= midiTicks) {
            list.append(current);
            current += fractionTicks;
        }
    }
    return list;
}
