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

#ifndef MIDIOUTPUT_H_
#define MIDIOUTPUT_H_

#include <QList>
#include <QMap>
#include <QObject>
#include "../midi/MidiFile.h"

class MidiEvent;
class RtMidiIn;
class RtMidiOut;
#ifdef IS_QT5
class QStringList;
#endif
class SenderThread;

#define MAX_OUTPUT_DEVICES 31

class MidiOutput : public QObject {

public:
    static void init();
    static void forceDrum(bool force);
    static void sendCommand(QByteArray array, int track_index);
    static void sendCommand(MidiEvent* e, int track_index);
    static void sendCommandDelete(MidiEvent* e, int track_index);
    static QStringList outputPorts(int index = 0);
    static bool closeOutputPort(int index);
    static bool setOutputPort(QString name, int index = 0);
    static QString outputPort(int index = 0, bool force = false);
    static int FluidDevice(QString name);
    static QString GetFluidDevice(int index);
    static void sendEnqueuedCommand(QByteArray array, int track_index);
    static bool isAlternativePlayer;
    static bool omitSysExLength;
    static QMap<int, QList<int> > playedNotes;
    static void setStandardChannel(int channel);
    static int standardChannel();
    static void sendProgram(int channel, int prog, int track_index = 0);
    static bool isConnected();
    static bool isFluidsynthConnected(int track_index = 0);
    static RtMidiOut* _midiOut[MAX_OUTPUT_DEVICES];
    static int _midiOutMAP[MAX_OUTPUT_DEVICES];
    static int _midiOutFluidMAP[MAX_OUTPUT_DEVICES];
    static int is_playing;
    static MidiFile *file;

    static bool AllTracksToOne;
    static bool FluidSynthTracksAuto;
    static bool SaveMidiOutDatas;

    static int sequencer_cmd[16];
    static int sequencer_enabled[16];

    static void update_config();
    static void saveTrackDevices();
    static bool loadTrackDevices(bool ask = false, QWidget *w = 0);


private:
    static QString _outPort[MAX_OUTPUT_DEVICES];

    static SenderThread* _sender;
    static int _stdChannel;

};

#endif
