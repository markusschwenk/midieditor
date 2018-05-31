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

#ifndef MIDIINPUT_H_
#define MIDIINPUT_H_

#include <QList>
#include <QMultiMap>
#include <QObject>
#include <QProcess>

#include <vector>

class MidiEvent;
class RtMidiIn;
class RtMidiOut;
class QStringList;
class MidiTrack;

class MidiInput : public QObject {

public:
    static void init();

    static void sendCommand(QByteArray array);
    static void sendCommand(MidiEvent* e);

    static QStringList inputPorts();
    static bool setInputPort(QString name);
    static QString inputPort();

    static void startInput();
    static QMultiMap<int, MidiEvent*> endInput(MidiTrack* track);

    static void receiveMessage(double deltatime,
        std::vector<unsigned char>* message, void* userData = 0);

    static void setTime(int ms);

    static bool recording();
    static void setThruEnabled(bool b);
    static bool thru();

    static bool isConnected();

private:
    static QString _inPort;
    static RtMidiIn* _midiIn;
    static QMultiMap<int, std::vector<unsigned char> >* _messages;
    static int _currentTime;
    static bool _recording;
    static QList<int> toUnique(QList<int> in);
    static bool _thru;
};

#endif
