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

#ifndef MIDIFILE_H_
#define MIDIFILE_H_

#include "../protocol/ProtocolEntry.h"

#include <QDataStream>
#include <QList>
#include <QMultiMap>
#include <QObject>
#include <QMessageBox>
#include <QFile>

#undef CUSTOM_MIDIEDITOR_GUI
#define CUSTOM_MIDIEDITOR_GUI "By Estwald"

extern bool MidieditorMaster;

extern int OctaveChan_MIDI[17];

extern QBrush background1;
extern QBrush background2;
extern QBrush background3;

class MidiEvent;
class TimeSignatureEvent;
class TempoChangeEvent;
class Protocol;
class MidiChannel;
class MidiTrack;

#define ERROR_CRITICAL(x) {\
    QMessageBox::critical(new QWidget(), "MidiEditor (irrecoverable)", x);\
    exit(-1);\
}

#define ERROR_CRITICAL2(x) {\
    QMessageBox::critical(new QWidget(), "MidiEditor", x);\
}

#define ERROR_CRITICAL_NO_MEMORY() {\
    QMessageBox::critical(new QWidget(), "MidiEditor (irrecoverable)", "Out of memory\n");\
    exit(-1);\
}

#define ERROR_CRITICAL_NO_MEMORY2() {\
    QMessageBox::critical(new QWidget(), "MidiEditor", "Out of memory\n");\
    return;\
}

#define ERROR_CRITICAL_NO_MEMORY3() {\
    QMessageBox::critical(new QWidget(), "MidiEditor", "Out of memory\n");\
}

class MidiFile : public QObject, public ProtocolEntry {

    Q_OBJECT

    using ProtocolEntry::protocol;

public:
    MidiFile(QString path, bool* ok, QStringList* log = 0);
    MidiFile();
    // needed to protocol fileLength
    MidiFile(int maxTime, Protocol* p);

    ~MidiFile();

    bool loadMidiEvents(QList<MidiEvent*>* events, MidiFile *file, bool *selectMultiTrack);
    bool saveMidiEvents(QFile &f, QList<MidiEvent*>* events, bool selectMultiTrack);
    bool save(QString path);
    bool saveMSEQ(QString path, bool onlych0 = true);
    bool lock_backup(bool locked);
    bool backup(bool save_backup = true);
    QByteArray writeDeltaTime(int time);
    int maxTime();
    int endTick();
    int timeMS(int midiTime);
    int measure(int startTick, int* startTickOfMeasure ,int* endTickOfMeasure);
    QMultiMap<int, MidiEvent*>* tempoEvents();
    QMultiMap<int, MidiEvent*>* timeSignatureEvents();
    void calcMaxTime();
    int tick(int ms);
    int tick(int startms, int endms, QList<MidiEvent*>** events, int* endTick, int* msOfFirstEvent);
    int measure(int startTick, int endTick, QList<TimeSignatureEvent*>** eventList, int* tickInMeasure = 0);
    int msOfTick(int tick, QList<MidiEvent*>* events = 0, int msOfFirstEventInList = 0);

    QList<MidiEvent*>* eventsBetween(int start, int end);
    int ticksPerQuarter();
    QMultiMap<int, MidiEvent*>* channelEvents(int channel);

    Protocol* protocol();
    void cleanProtocol();
    MidiChannel* channel(int i);

    void preparePlayerData(int tickFrom);
    void preparePlayerDataSequencer(int tickFrom);

    QMultiMap<int, MidiEvent*>* playerData();

    static QString instrumentName(int bank, int prog);
    static QString drumName(int prog);
    static QString controlChangeName(int control);
    int cursorTick();
    int pauseTick();
    void setCursorTick(int tick);
    void setPauseTick(int tick);
    QString path();
    bool saved();
    void setSaved(bool b);
    void setPath(QString path);
    bool channelMuted(int ch, int track_index);
    int numTracks();
    QList<MidiTrack*>* tracks();
    void addTrack();
    void setMaxLengthMs(int ms);

    void deleteMeasures(int from, int to);
    void insertMeasures(int after, int numMeasures);

    ProtocolEntry* copy();
    void reloadState(ProtocolEntry* entry);
    MidiFile* file();
    bool removeTrack(MidiTrack* track);
    MidiTrack* track(int number);

    int tonalityAt(int tick);
    void meterAt(int tick, int* num, int* denum, TimeSignatureEvent **lastTimeSigEvent = 0);

    static int variableLengthvalue(QDataStream* content);
    static QByteArray writeVariableLengthValue(int value);
    static int defaultTimePerQuarter;

    void registerCopiedTrack(MidiTrack* source, MidiTrack* destination, MidiFile* fileFrom);
    MidiTrack* getPasteTrack(MidiTrack* source, MidiFile* fileFrom);

    QList<int> quantization(int fractionSize);

    int startTickOfMeasure(int measure);

    bool MultitrackMode; // flag to use 16 channels per track or 16 channels for alls
    bool DrumUseCh9;

    int Bank_MIDI[516];
    int Prog_MIDI[516];

    bool is_sequencer;
    bool is_multichannel_sequencer;
    float scale_time_sequencer;

signals:
    void cursorPositionChanged();
    void recalcWidgetSize();
    void trackChanged();

private:
    bool readMidiFile(QDataStream* content, QStringList* log);
    bool readTrack(QDataStream* content, int num, QStringList* log);
    int deltaTime(QDataStream* content);

    int timePerQuarter;
    MidiChannel* channels[19];

    QString _path;
    int midiTicks, maxTimeMS, _cursorTick, _pauseTick, _midiFormat;
    Protocol* prot;
    QMultiMap<int, MidiEvent*>* playerMap;
    bool _saved;
    QList<MidiTrack*>* _tracks;
    QMap<MidiFile*, QMap<MidiTrack*, MidiTrack*> > pasteTracks;

    void printLog(QStringList* log);
    int _file_tracks;
};

#endif
