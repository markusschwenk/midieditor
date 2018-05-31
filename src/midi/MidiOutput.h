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

class MidiEvent;
class RtMidiIn;
class RtMidiOut;
class QStringList;
class SenderThread;

class MidiOutput : public QObject {

public:
    static void init();
    static void sendCommand(QByteArray array);
    static void sendCommand(MidiEvent* e);
    static QStringList outputPorts();
    static bool setOutputPort(QString name);
    static QString outputPort();
    static void sendEnqueuedCommand(QByteArray array);
    static bool isAlternativePlayer;
    static QMap<int, QList<int> > playedNotes;
    static void setStandardChannel(int channel);
    static int standardChannel();
    static void sendProgram(int channel, int prog);
    static bool isConnected();

private:
    static QString _outPort;
    static RtMidiOut* _midiOut;
    static SenderThread* _sender;
    static int _stdChannel;
};

#endif
