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
#include <QMutex>
#include "../midi/MidiFile.h"

#include <vector>

class MidiEvent;
class RtMidiIn;
class RtMidiOut;
#ifdef IS_QT5
class QStringList;
#endif
class MidiTrack;

//#define HACK_INPUT

#define MAX_INPUT_DEVICES 6
#define MAX_INPUT_PAIR (MAX_INPUT_DEVICES/2)

#define DEVICE_ID 0xcacafe1
#define DEVICE_SEQUENCER_ID 0x5EC001
#define DEVICE_ID_TEST (DEVICE_ID + 0)

class MidiInput : public QObject {

public:
    static void init();

    static QStringList inputPorts(int index);
    static bool setInputPort(QString name, int index);
    static QString inputPort(int index);
    static bool closeInputPort(int index);

    static void startInput();
    static QMultiMap<int, MidiEvent*> endInput(MidiTrack* track);

    static void receiveMessage(double /*deltatime*/,
        std::vector<unsigned char>* message, void* /*userData = 0*/);
    static void receiveMessage_mutex(double /*deltatime*/,
        std::vector<unsigned char>* message, void* /*userData = 0*/);

    static void send_thru(int dev, int is_effect, std::vector<unsigned char>* message);
    static void cleanKeyMaps();
    static void setTime(int ms);
    static int currentTime() {
        return _currentTime;
    }

    static void deleteIndexArray(QByteArray &a, int i) {
        QByteArray b;

        b = a.left(i);
        if((i + 1) < a.length())
            b+= a.mid(i + 1);
/*
        for(int n = 0; n < i; n++)
            b.append(a.at(n));
        for(int n = i + 1; n < a.length(); n++)
            b.append(a.at(n));
        a = b;
*/
    }

    static bool recording();
    static void setThruEnabled(bool b);
    static bool thru();

    static bool isConnected(int index);
    static void connectVirtual(int device);

    static RtMidiIn* _midiIn[MAX_INPUT_DEVICES];

    static int track_index;
    static MidiFile *file;
    static bool keyboard2_connected[MAX_INPUT_DEVICES];
    static std::vector<unsigned char> message[MAX_INPUT_DEVICES];

    static QByteArray note_roll[16];
    static unsigned char note_roll_velocity[16];
    static QByteArray note_roll_out[16];

    static unsigned int keys_dev[MAX_INPUT_DEVICES][16];
    static unsigned int keys_fluid[MAX_INPUT_DEVICES];
    static int keys_vst[MAX_INPUT_DEVICES][16];
    static unsigned int keys_seq[MAX_INPUT_DEVICES];
    static unsigned int keys_switch[MAX_INPUT_DEVICES];
    static int loadSeq_mode;

    static QMutex mutex_input;

    static void ClearMessages() {
        _messages->clear();
    }

private:
    static QString _inPort[MAX_INPUT_DEVICES];

    static QMultiMap<int, std::vector<unsigned char> >* _messages;
    static int _currentTime;
    static bool _recording;
    static QList<int> toUnique(QList<int> in);
    static bool _thru;

};

#endif
