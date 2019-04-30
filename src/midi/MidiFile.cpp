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

MidiFile::MidiFile()
{
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
    tempoTrack->setName("Tempo Track");
    tempoTrack->setNumber(0);
    _tracks->append(tempoTrack);

    MidiTrack* instrumentTrack = new MidiTrack(this);
    instrumentTrack->setName("New Instrument");
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

MidiFile::MidiFile(QString path, bool* ok, QStringList* log)
{

    if (!log) {
        log = new QStringList();
    }

    _pauseTick = -1;
    _saved = true;
    midiTicks = 0;
    _cursorTick = 0;
    prot = new Protocol(this);
    prot->addEmptyAction("File opened");
    _path = path;
    _tracks = new QList<MidiTrack*>();
    QFile* f = new QFile(path);

    if (!f->open(QIODevice::ReadOnly)) {
        *ok = false;
        log->append("Error: File could not be opened.");
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

MidiFile::MidiFile(int ticks, Protocol* p)
{
    midiTicks = ticks;
    prot = p;
}

bool MidiFile::readMidiFile(QDataStream* content, QStringList* log)
{

    OffEvent::clearOnEvents();

    quint8 tempByte;

    QString badHeader = "Error: Bad format in file header (Expected MThd).";
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
        log->append("Error: MThdTrackLength wrong (expected 6).");
        return false;
    }

    quint16 midiFormat;
    (*content) >> midiFormat;
    if (midiFormat > 1) {
        log->append("Error: MidiFormat v. 2 cannot be loaded with this Editor.");
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
            log->append("Error in Track " + QString::number(num));
            return false;
        }
    }

    // find corrupted OnEvents (without OffEvent)
    foreach (OnEvent* onevent, OffEvent::corruptedOnEvents()) {
        log->append("Warning: found OnEvent without OffEvent (line " + QString::number(onevent->line()) + ") - removing...");
        channel(onevent->channel())->removeEvent(onevent);
    }

    OffEvent::clearOnEvents();

    return true;
}

bool MidiFile::readTrack(QDataStream* content, int num, QStringList* log)
{

    quint8 tempByte;

    QString badHeader = "Error: Bad format in track header (Track " + QString::number(num) + ", Expected MTrk).";
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
            log->append("Warning: detected offEvent without prior onEvent. Skipping!");
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
    QString errorText = "Error: track " + QString::number(num) + "not ended as expected. ";
    if (tempByte != 0x00) {
        log->append(errorText);
        return false;
    }

    // check whether TimeSignature at tick 0 is given. If not, create one.
    // this will be done after reading the first track
    if (!channel(18)->eventMap()->contains(0)) {
        log->append("Warning: no TimeSignatureEvent detected at tick 0. Adding default value.");
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

int MidiFile::deltaTime(QDataStream* content)
{
    return variableLengthvalue(content);
}

int MidiFile::variableLengthvalue(QDataStream* content)
{
    quint32 v = 0;
    quint8 byte = 0;
    do {
        (*content) >> byte;
        v <<= 7;
        v |= (byte & 0x7F);
    } while (byte & (1 << 7));
    return (int)v;
}

QMap<int, MidiEvent*>* MidiFile::timeSignatureEvents()
{
    return channels[18]->eventMap();
}

QMap<int, MidiEvent*>* MidiFile::tempoEvents()
{
    return channels[17]->eventMap();
}

void MidiFile::calcMaxTime()
{
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
            ticks = midiTicks - ev->midiTime();
        }
        time += ticks * ev->msPerTick();
    }
    maxTimeMS = time;
    emit recalcWidgetSize();
}

int MidiFile::maxTime()
{
    return maxTimeMS;
}

int MidiFile::endTick()
{
    return midiTicks;
}

int MidiFile::tick(int ms)
{
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
                                                                msOfFirstEventInList)
{

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
    int* endTick, int* msOfFirstEvent)
{
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

int MidiFile::numTracks()
{
    return _tracks->size();
}

int MidiFile::measure(int startTick, int endTick,
    QList<TimeSignatureEvent*>** eventList, int* ticksInmeasure)
{

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

int MidiFile::measure(int startTick, int* startTickOfMeasure ,int* endTickOfMeasure)
{
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
    int ticksInmeasure;
    measure += event->measures(ticks, &ticksInmeasure);
    *(startTickOfMeasure) = startTick - ticksInmeasure;
    *(endTickOfMeasure) = *startTickOfMeasure + event->ticksPerMeasure();
    return measure;
}

int MidiFile::ticksPerQuarter()
{
    return timePerQuarter;
}

QMultiMap<int, MidiEvent*>* MidiFile::channelEvents(int channel)
{
    return channels[channel]->eventMap();
}

Protocol* MidiFile::protocol()
{
    return prot;
}

MidiChannel* MidiFile::channel(int i)
{
    return channels[i];
}

QString MidiFile::instrumentName(int prog)
{
    switch (prog + 1) {
    case 1: {
        return "Acoustic Grand Piano";
    }
    case 2: {
        return "Bright Acoustic Piano";
    }
    case 3: {
        return "Electric Grand Piano";
    }
    case 4: {
        return "Honky-tonk Piano";
    }
    case 5: {
        return " Electric Piano 1";
    }
    case 6: {
        return "Electric Piano 2";
    }
    case 7: {
        return "Harpsichord";
    }
    case 8: {
        return "Clavinet (Clavi)";
    }
    case 9: {
        return "Celesta";
    }
    case 10: {
        return "Glockenspiel";
    }
    case 11: {
        return "Music Box";
    }
    case 12: {
        return "Vibraphone";
    }
    case 13: {
        return "Marimba";
    }
    case 14: {
        return "Xylophone";
    }
    case 15: {
        return "Tubular Bells";
    }
    case 16: {
        return "Dulcimer";
    }
    case 17: {
        return "Drawbar Organ";
    }
    case 18: {
        return "Percussive Organ";
    }
    case 19: {
        return "Rock Organ";
    }
    case 20: {
        return "Church Organ";
    }
    case 21: {
        return "Reed Organ";
    }
    case 22: {
        return "Accordion";
    }
    case 23: {
        return "Harmonica";
    }
    case 24: {
        return "Tango Accordion";
    }
    case 25: {
        return "Acoustic Guitar (nylon)";
    }
    case 26: {
        return "Acoustic Guitar (steel)";
    }
    case 27: {
        return "Electric Guitar (jazz)";
    }
    case 28: {
        return "Electric Guitar (clean)";
    }
    case 29: {
        return "Electric Guitar (muted)";
    }
    case 30: {
        return "Overdriven Guitar";
    }
    case 31: {
        return "Distortion Guitar";
    }
    case 32: {
        return "Guitar harmonics";
    }
    case 33: {
        return "Acoustic Bass";
    }
    case 34: {
        return "Electric Bass (finger)";
    }
    case 35: {
        return "Electric Bass (pick)";
    }
    case 36: {
        return "Fretless Bass";
    }
    case 37: {
        return "Slap Bass 1";
    }
    case 38: {
        return "Slap Bass 2";
    }
    case 39: {
        return "Synth Bass 1";
    }
    case 40: {
        return "Synth Bass 2";
    }
    case 41: {
        return "Violin";
    }
    case 42: {
        return "Viola";
    }
    case 43: {
        return "Cello";
    }
    case 44: {
        return "Contrabass";
    }
    case 45: {
        return "Tremolo Strings";
    }
    case 46: {
        return "Pizzicato Strings";
    }
    case 47: {
        return "Orchestral Harp";
    }
    case 48: {
        return "Timpani";
    }
    case 49: {
        return "String Ensemble 1";
    }
    case 50: {
        return "String Ensemble 2";
    }
    case 51: {
        return "Synth Strings 1";
    }
    case 52: {
        return "Synth Strings 2";
    }
    case 53: {
        return "Choir Aahs";
    }
    case 54: {
        return "Voice Oohs";
    }
    case 55: {
        return "Synth Choir";
    }
    case 56: {
        return "Orchestra Hit";
    }
    case 57: {
        return "Trumpet";
    }
    case 58: {
        return "Trombone";
    }
    case 59: {
        return "Tuba";
    }
    case 60: {
        return "Muted Trumpet";
    }
    case 61: {
        return "French Horn";
    }
    case 62: {
        return "Brass Section";
    }
    case 63: {
        return "Synth Brass 1";
    }
    case 64: {
        return "Synth Brass 2";
    }
    case 65: {
        return "Soprano Sax";
    }
    case 66: {
        return "Alto Sax";
    }
    case 67: {
        return "Tenor Sax";
    }
    case 68: {
        return "Baritone Sax";
    }
    case 69: {
        return "Oboe";
    }
    case 70: {
        return "English Horn";
    }
    case 71: {
        return "Bassoon";
    }
    case 72: {
        return "Clarinet";
    }
    case 73: {
        return "Piccolo";
    }
    case 74: {
        return "Flute";
    }
    case 75: {
        return "Recorder";
    }
    case 76: {
        return "Pan Flute";
    }
    case 77: {
        return "Blown Bottle";
    }
    case 78: {
        return "Shakuhachi";
    }
    case 79: {
        return "Whistle";
    }
    case 80: {
        return "Ocarina";
    }
    case 81: {
        return "Lead 1 (square)";
    }
    case 82: {
        return "Lead 2 (sawtooth)";
    }
    case 83: {
        return "Lead 3 (calliope)";
    }
    case 84: {
        return "Lead 4 (chiff)";
    }
    case 85: {
        return "Lead 5 (charang)";
    }
    case 86: {
        return "Lead 6 (voice)";
    }
    case 87: {
        return "Lead 7 (fifths)";
    }
    case 88: {
        return "Lead 8 (bass + lead)";
    }
    case 89: {
        return "Pad 1 (new age)";
    }
    case 90: {
        return "Pad 2 (warm)";
    }
    case 91: {
        return "Pad 3 (polysynth)";
    }
    case 92: {
        return "Pad 4 (choir)";
    }
    case 93: {
        return "Pad 5 (bowed)";
    }
    case 94: {
        return "Pad 6 (metallic)";
    }
    case 95: {
        return "Pad 7 (halo)";
    }
    case 96: {
        return "Pad 8 (sweep)";
    }
    case 97: {
        return "FX 1 (rain)";
    }
    case 98: {
        return "FX 2 (soundtrack)";
    }
    case 99: {
        return "FX 3 (crystal)";
    }
    case 100: {
        return "FX 4 (atmosphere)";
    }
    case 101: {
        return "FX 5 (brightness)";
    }
    case 102: {
        return "FX 6 (goblins)";
    }
    case 103: {
        return "FX 7 (echoes)";
    }
    case 104: {
        return "FX 8 (sci-fi)";
    }
    case 105: {
        return "Sitar";
    }
    case 106: {
        return "Banjo";
    }
    case 107: {
        return "Shamisen";
    }
    case 108: {
        return "Koto";
    }
    case 109: {
        return "Kalimba";
    }
    case 110: {
        return "Bag pipe";
    }
    case 111: {
        return "Fiddle";
    }
    case 112: {
        return "Shanai";
    }
    case 113: {
        return "Tinkle Bell";
    }
    case 114: {
        return "Agogo";
    }
    case 115: {
        return "Steel Drums";
    }
    case 116: {
        return "Woodblock";
    }
    case 117: {
        return "Taiko Drum";
    }
    case 118: {
        return "Melodic Tom";
    }
    case 119: {
        return "Synth Drum";
    }
    case 120: {
        return "Reverse Cymbal";
    }
    case 121: {
        return "Guitar Fret Noise";
    }
    case 122: {
        return "Breath Noise";
    }
    case 123: {
        return "Seashore";
    }
    case 124: {
        return "Bird Tweet";
    }
    case 125: {
        return "Telephone Ring";
    }
    case 126: {
        return "Helicopter";
    }
    case 127: {
        return "Applause";
    }
    case 128: {
        return "Gunshot";
    }
    }
    return "out of range";
}

QString MidiFile::controlChangeName(int control)
{

    switch (control) {
    case 0: {
        return "Bank Select (MSB)";
    }
    case 1: {
        return "Modulation Wheel (MSB)";
    }
    case 2: {
        return "Breath Controller (MSB) ";
    }

    case 4: {
        return "Foot Controller (MSB)";
    }
    case 5: {
        return "Portamento Time (MSB)";
    }
    case 6: {
        return "Data Entry (MSB)";
    }
    case 7: {
        return "Channel Volume (MSB)";
    }
    case 8: {
        return "Balance (MSB) ";
    }

    case 10: {
        return "Pan (MSB)";
    }
    case 11: {
        return "Expression (MSB)";
    }
    case 12: {
        return "Effect Control 1 (MSB)";
    }
    case 13: {
        return "Effect Control 2 (MSB) ";
    }

    case 16: {
        return "General Purpose Controller 1 (MSB)";
    }
    case 17: {
        return "General Purpose Controller 2 (MSB)";
    }
    case 18: {
        return "General Purpose Controller 3 (MSB)";
    }
    case 19: {
        return "General Purpose Controller 4 (MSB) ";
    }

    case 32: {
        return "Bank Select (LSB)";
    }
    case 33: {
        return "Modulation Wheel (LSB)";
    }
    case 34: {
        return "Breath Controller (LSB) ";
    }

    case 36: {
        return "Foot Controller (LSB)";
    }
    case 37: {
        return "Portamento Time (LSB)";
    }
    case 38: {
        return "Data Entry (LSB) Channel";
    }
    case 39: {
        return "Channel Volume (LSB)";
    }
    case 40: {
        return "Balance (LSB) ";
    }

    case 42: {
        return "Pan (LSB)";
    }
    case 43: {
        return "Expression (LSB)";
    }
    case 44: {
        return "Effect Control 1 (LSB)";
    }
    case 45: {
        return "Effect Control 2 (LSB) ";
    }

    case 48: {
        return "General Purpose Controller 1 (LSB)";
    }
    case 49: {
        return "General Purpose Controller 2 (LSB)";
    }
    case 50: {
        return "General Purpose Controller 3 (LSB)";
    }
    case 51: {
        return "General Purpose Controller 4 (LSB) ";
    }

    case 64: {
        return "Sustain Pedal";
    }
    case 65: {
        return "Portamento On/Off";
    }
    case 66: {
        return "Sostenuto";
    }
    case 67: {
        return "Soft Pedal";
    }
    case 68: {
        return "Legato Footswitch";
    }
    case 69: {
        return "Hold 2";
    }
    case 70: {
        return "Sound Controller 1";
    }
    case 71: {
        return "Sound Controller 2";
    }
    case 72: {
        return "Sound Controller 3";
    }
    case 73: {
        return "Sound Controller 4";
    }
    case 74: {
        return "Sound Controller 5";
    }
    case 75: {
        return "Sound Controller 6";
    }
    case 76: {
        return "Sound Controller 7";
    }
    case 77: {
        return "Sound Controller 8";
    }
    case 78: {
        return "Sound Controller 9";
    }
    case 79: {
        return "Sound Controller 10 (GM2 default: Undefined)";
    }
    case 80: {
        return "General Purpose Controller 5";
    }
    case 81: {
        return "General Purpose Controller 6";
    }
    case 82: {
        return "General Purpose Controller 7";
    }
    case 83: {
        return "General Purpose Controller 8";
    }
    case 84: {
        return "Portamento Control ";
    }

    case 91: {
        return "Effects 1 Depth (default: Reverb Send)";
    }
    case 92: {
        return "Effects 2 Depth (default: Tremolo Depth)";
    }
    case 93: {
        return "Effects 3 Depth (default: Chorus Send)";
    }
    case 94: {
        return "Effects 4 Depth (default: Celeste [Detune] Depth)";
    }
    case 95: {
        return "Effects 5 Depth (default: Phaser Depth)";
    }
    case 96: {
        return "Data Increment";
    }
    case 97: {
        return "Data Decrement";
    }
    case 98: {
        return "Non-Registered Parameter Number (LSB)";
    }
    case 99: {
        return "Non-Registered Parameter Number(MSB)";
    }
    case 100: {
        return "Registered Parameter Number (LSB)";
    }
    case 101: {
        return "Registered Parameter Number(MSB) ";
    }

    case 120: {
        return "All Sound Off";
    }
    case 121: {
        return "Reset All Controllers";
    }
    case 122: {
        return "Local Control On/Off";
    }
    case 123: {
        return "All Notes Off";
    }
    case 124: {
        return "Omni Mode Off";
    }
    case 125: {
        return "Omni Mode On";
    }
    case 126: {
        return "Poly Mode Off";
    }
    case 127: {
        return "Poly Mode On";
    }
    }

    return "undefined";
}

QList<MidiEvent*>* MidiFile::eventsBetween(int start, int end)
{
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

bool MidiFile::channelMuted(int ch)
{

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

void MidiFile::preparePlayerData(int tickFrom)
{

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

QMultiMap<int, MidiEvent*>* MidiFile::playerData()
{
    return playerMap;
}

int MidiFile::cursorTick()
{
    return _cursorTick;
}

void MidiFile::setCursorTick(int tick)
{
    _cursorTick = tick;
    emit cursorPositionChanged();
}

int MidiFile::pauseTick()
{
    return _pauseTick;
}

void MidiFile::setPauseTick(int tick)
{
    _pauseTick = tick;
}

bool MidiFile::save(QString path)
{

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

QByteArray MidiFile::writeDeltaTime(int time)
{
    return writeVariableLengthValue(time);
}

QByteArray MidiFile::writeVariableLengthValue(int value)
{

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

QString MidiFile::path()
{
    return _path;
}

void MidiFile::setPath(QString path)
{
    _path = path;
}

bool MidiFile::saved()
{
    return _saved;
}

void MidiFile::setSaved(bool b)
{
    _saved = b;
}

void MidiFile::setMaxLengthMs(int ms)
{
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

ProtocolEntry* MidiFile::copy()
{
    MidiFile* file = new MidiFile(midiTicks, protocol());
    file->_tracks = new QList<MidiTrack*>(*(_tracks));
    file->pasteTracks = pasteTracks;
    return file;
}

void MidiFile::reloadState(ProtocolEntry* entry)
{
    MidiFile* file = dynamic_cast<MidiFile*>(entry);
    if (file) {
        midiTicks = file->midiTicks;
        _tracks = new QList<MidiTrack*>(*(file->_tracks));
        pasteTracks = file->pasteTracks;
    }
    calcMaxTime();
}

MidiFile* MidiFile::file()
{
    return this;
}

QList<MidiTrack*>* MidiFile::tracks()
{
    return _tracks;
}

void MidiFile::addTrack()
{
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

bool MidiFile::removeTrack(MidiTrack* track)
{

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

MidiTrack* MidiFile::track(int number)
{
    if (_tracks->size() > number) {
        return _tracks->at(number);
    } else {
        return 0;
    }
}

int MidiFile::tonalityAt(int tick)
{
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

void MidiFile::meterAt(int tick, int* num, int* denum, TimeSignatureEvent **lastTimeSigEvent)
{
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
    } else {
        *num = event->num();
        *denum = event->denom();
        if (lastTimeSigEvent) {
            *lastTimeSigEvent = event;
        }
    }
}

void MidiFile::printLog(QStringList* log)
{
    foreach (QString str, *log) {
        qWarning(str.toUtf8().constData());
    }
}

void MidiFile::registerCopiedTrack(MidiTrack* source, MidiTrack* destination, MidiFile* fileFrom)
{

    //	if(fileFrom == this){
    //		return;
    //	}

    ProtocolEntry* toCopy = copy();

    QMap<MidiTrack*, MidiTrack*> list;
    if (pasteTracks.contains(fileFrom)) {
        list = pasteTracks.value(fileFrom);
    }

    list.insert(source, destination);
    pasteTracks.insert(fileFrom, list);

    ProtocolEntry::protocol(toCopy, this);
}

MidiTrack* MidiFile::getPasteTrack(MidiTrack* source, MidiFile* fileFrom)
{

    //	if(fileFrom == this){
    //		return source;
    //	}

    if (!pasteTracks.contains(fileFrom) || !pasteTracks.value(fileFrom).contains(source)) {
        return 0;
    }

    return pasteTracks.value(fileFrom).value(source);
}

QList<int> MidiFile::quantization(int fractionSize)
{

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


int MidiFile::startTickOfMeasure(int measure){
    QMap<int, MidiEvent*> *timeSigs = timeSignatureEvents();
    QMap<int, MidiEvent*>::iterator it = timeSigs->begin();

    // Find the time signature event the measure is in and its start measure
    int currentMeasure = 1;
    TimeSignatureEvent *currentEvent = dynamic_cast<TimeSignatureEvent*>(timeSigs->value(0));
    it++;
    while (it != timeSigs->end()) {
        int endMeasureOfCurrentEvent = currentMeasure + ceil((it.key() - currentEvent->midiTime()) / currentEvent->ticksPerMeasure());
        if (endMeasureOfCurrentEvent > measure) {
            break;
        }
        currentEvent = dynamic_cast<TimeSignatureEvent*>(it.value());
        currentMeasure = endMeasureOfCurrentEvent;
        it++;
    }

    return currentEvent->midiTime() + (measure - currentMeasure) * currentEvent->ticksPerMeasure();
}

void MidiFile::deleteMeasures(int from, int to){
    int tickFrom = startTickOfMeasure(from);
    int tickTo = startTickOfMeasure(to + 1);

    // Find and remember meter (time signture event) at the first undeleted measure
    TimeSignatureEvent *lastTimeSig;
    int num;
    int denom;
    meterAt(tickTo, &num, &denom, &lastTimeSig);
    ProtocolEntry* toCopy = copy();

    // Delete all events. For notes, only delete if starting within the given tick range.
    for (int ch = 0; ch < 19; ch++) {
        QMap<int, MidiEvent*>::Iterator it = channel(ch)->eventMap()->begin();
        QList<MidiEvent*> toRemove;
        while(it != channel(ch)->eventMap()->end()) {
            if (it.key() >= tickFrom && it.key() <= tickTo) {
                OffEvent *offEvent = dynamic_cast<OffEvent*>(it.value());
                if (!offEvent) {
                    // Only remove if no off-event, off-event are handled separately
                    toRemove.append(it.value());

                    OnEvent *onEvent = dynamic_cast<OnEvent*>(it.value());
                    if (onEvent) {
                        OffEvent *offEventOfRemovedNote = onEvent->offEvent();
                        toRemove.append(offEventOfRemovedNote);
                    }
                }
            }
            it++;
        }

        foreach (MidiEvent* event, toRemove) {
            channel(ch)->removeEvent(event);
        }
    }

    // All remaining events after the end tick have to be shifted. Note: off events that still
    // exist inbetween the deleted inerval are not shifted, since this would cause negative
    // duration.
    for (int ch = 0; ch < 19; ch++) {
        QList<MidiEvent*> toUpdate;
        QMap<int, MidiEvent*>::Iterator it = channel(ch)->eventMap()->begin();
        while(it != channel(ch)->eventMap()->end()) {
            if (it.key() > tickTo) {
                toUpdate.append(it.value());
            }
            it++;
        }

        foreach(MidiEvent *event, toUpdate) {
            event->setMidiTime(event->midiTime() - (tickTo - tickFrom));
        }
    }

    // Check meter again. If changed, add event to readjust.
    int numAfter;
    int denomAfter;
    meterAt(tickFrom, &numAfter, &denomAfter);
    int ticksPerMeasure;
    if (denom != denomAfter || num != numAfter) {
        TimeSignatureEvent *newEvent = new TimeSignatureEvent(18, num, denom, 24, 8, track(0));
        channel(18)->insertEvent(newEvent, tickFrom);
        ticksPerMeasure = newEvent->ticksPerMeasure();
    } else {
        ticksPerMeasure = lastTimeSig->ticksPerMeasure();
    }

    midiTicks = midiTicks - (tickTo - tickFrom);
    // make sure, we have at east one measure
    if (ticksPerMeasure > midiTicks) {
        midiTicks = ticksPerMeasure;
    }
    calcMaxTime();
    ProtocolEntry::protocol(toCopy, this);
}

void MidiFile::insertMeasures(int after, int numMeasures){
    if (after == 0) {
        // Cannot insert before first measure.
        return;
    }
    ProtocolEntry* toCopy = copy();
    int tick = startTickOfMeasure(after + 1);

    // Find meter at measure and compute number of inserted ticks.
    int num;
    int denom;
    TimeSignatureEvent *lastTimeSig;
    meterAt(tick-1, &num, &denom, &lastTimeSig);
    int numTicks = lastTimeSig->ticksPerMeasure() * numMeasures;
    midiTicks = midiTicks + numTicks;

    // Shift all ticks.
    for (int ch = 0; ch < 19; ch++) {
        QList<MidiEvent*> toUpdate;
        QMap<int, MidiEvent*>::Iterator it = channel(ch)->eventMap()->begin();
        while(it != channel(ch)->eventMap()->end()) {
            if (it.key() >= tick) {
                toUpdate.append(it.value());
            }
            it++;
        }

        foreach(MidiEvent *event, toUpdate) {
            event->setMidiTime(event->midiTime() + numTicks);
        }
    }

    calcMaxTime();
    ProtocolEntry::protocol(toCopy, this);
}
