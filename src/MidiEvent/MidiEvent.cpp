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

#include "MidiEvent.h"
#include "../gui/EventWidget.h"
#include "../midi/MidiFile.h"
#include "../midi/MidiTrack.h"
#include "ChannelPressureEvent.h"
#include "ControlChangeEvent.h"
#include "KeyPressureEvent.h"
#include "KeySignatureEvent.h"
#include "NoteOnEvent.h"
#include "OffEvent.h"
#include "PitchBendEvent.h"
#include "ProgChangeEvent.h"
#include "SysExEvent.h"
#include "TempoChangeEvent.h"
#include "TextEvent.h"
#include "TimeSignatureEvent.h"
#include "UnknownEvent.h"

#include <QByteArray>

#include "../midi/MidiChannel.h"

quint8 MidiEvent::_startByte = 0;
EventWidget* MidiEvent::_eventWidget = 0;

MidiEvent::MidiEvent(int channel, MidiTrack* track)
    : ProtocolEntry()
    , GraphicObject()
{
    _track = track;
    numChannel = channel;
    timePos = 0;
    midiFile = 0;
}

MidiEvent::MidiEvent(MidiEvent& other)
    : ProtocolEntry(other)
    , GraphicObject()
{
    _track = other._track;
    numChannel = other.numChannel;
    timePos = other.timePos;
    midiFile = other.midiFile;
    midi_modified = other.midi_modified;
}

MidiEvent::~MidiEvent() {

    //qWarning("midi event destroyed");
}

MidiEvent* MidiEvent::loadMidiEvent(QDataStream* content, bool* ok,
    bool* endEvent, MidiTrack* track, quint8 startByte, quint8 secondByte)
{

    // first try to load the event. If this does not work try to use
    // old first byte as new first byte. This is implemented in the end of this
    // method using recursive calls.
    // if startByte (paramater) is not 0, this is the second call so first and
    // second byte must not be loaded from the stream but from the parameters.

    *ok = true;

    quint8 tempByte;

    quint8 prevStartByte = _startByte;

    if (!startByte) {
        (*content) >> tempByte;
    } else {
        tempByte = startByte;
    }
    _startByte = tempByte;

    int channel = tempByte & 0x0F;

    switch (tempByte & 0xF0) {

    case 0x80: {
        // Note Off
        if (!startByte) {
            (*content) >> tempByte;
        } else {
            tempByte = secondByte;
        }
        int note = tempByte;
        if (note < 0 || note > 127) {
            *ok = false;
            return 0;
        }
        // skip byte (velocity)
        (*content) >> tempByte;

        OffEvent* event = new OffEvent(channel, 127 - note, track);
        if(event)
            *ok = true;
        else {
            *ok = false;
            ERROR_CRITICAL_NO_MEMORY();
        }
        return event;
    }

    case 0x90: {
        // Note On
        if (!startByte) {
            (*content) >> tempByte;
        } else {
            tempByte = secondByte;
        }
        int note = tempByte;
        if (note < 0 || note > 127) {
            *ok = false;
            return 0;
        }
        (*content) >> tempByte;
        int velocity = tempByte;
        *ok = true;

        if (velocity > 0) {
            NoteOnEvent* event = new NoteOnEvent(note, velocity, channel, track);
            if(event)
                *ok = true;
            else {
                *ok = false;
                ERROR_CRITICAL_NO_MEMORY();
            }
            return event;
        } else {
            OffEvent* event = new OffEvent(channel, 127 - note, track);
            if(event)
                *ok = true;
            else {
                *ok = false;
                ERROR_CRITICAL_NO_MEMORY();
            }
            return event;
        }
    }

    case 0xA0: {
        // Key Pressure
        if (!startByte) {
            (*content) >> tempByte;
        } else {
            tempByte = secondByte;
        }
        int note = tempByte;
        if (note < 0 || note > 127) {
            *ok = false;
            return 0;
        }
        (*content) >> tempByte;
        int value = tempByte;


        KeyPressureEvent *event = new KeyPressureEvent(channel, value, note, track);
        if(event)
            *ok = true;
        else {
            *ok = false;
            ERROR_CRITICAL_NO_MEMORY();
        }

        return event;
    }

    case 0xB0: {
        // Controller
        if (!startByte) {
            (*content) >> tempByte;
        } else {
            tempByte = secondByte;
        }
        int control = tempByte;
        (*content) >> tempByte;
        int value = tempByte;
        ControlChangeEvent *event = new ControlChangeEvent(channel, control, value, track);

        if(event)
            *ok = true;
        else {
            *ok = false;
            ERROR_CRITICAL_NO_MEMORY();
        }

        return event;
    }

    case 0xC0: {
        // programm change
        if (!startByte) {
            (*content) >> tempByte;
        } else {
            tempByte = secondByte;
        }

        ProgChangeEvent *event =new ProgChangeEvent(channel, tempByte, track);
        if(event)
            *ok = true;
        else {
            *ok = false;
            ERROR_CRITICAL_NO_MEMORY();
        }
        return event;
    }

    case 0xD0: {
        // Key Pressure
        if (!startByte) {
            (*content) >> tempByte;
        } else {
            tempByte = secondByte;
        }
        int value = tempByte;

        ChannelPressureEvent *event = new ChannelPressureEvent(channel, value, track);

        if(event)
            *ok = true;
        else {
            *ok = false;
            ERROR_CRITICAL_NO_MEMORY();
        }
        return event;
    }

    case 0xE0: {

        // Pitch Wheel
        if (!startByte) {
            (*content) >> tempByte;
        } else {
            tempByte = secondByte;
        }
        quint8 first = tempByte;
        (*content) >> tempByte;
        quint8 second = tempByte;

        int value = (second << 7) | first;

        PitchBendEvent *event = new PitchBendEvent(channel, value, track);
        if(event)
            *ok = true;
        else {
            *ok = false;
            ERROR_CRITICAL_NO_MEMORY();
        }
        return event;

    }

    case 0xF0: {
        // System Message
        channel = 16; // 16 is channel without number

        switch (tempByte & 0x0F) {

        case 0x00: {

            // SysEx
            QByteArray array;
            while (tempByte != 0xF7) {
                (*content) >> tempByte;
                if (tempByte != 0xF7) {
                    array.append((char)tempByte);
                }
            }

            // sysEX uses VLQ as length! (https://en.wikipedia.org/wiki/Variable-length_quantity)

            unsigned int length = 0;
            int index = 0;

            for(; index < 4; index++) {
                if(array[index] & 128) {
                   length = (length + (array[index] & 127)) * 128;
                } else {
                    length += array[index];
                    break;
                }
            }

            *ok = true;
            if(array.size() == (int) (length + index)) // sysEx have length...
            {
                SysExEvent *event = new SysExEvent(channel, QByteArray(array.constData() + index + 1, array.size() - index - 1), track);

                if(event)
                    *ok = true;
                else {
                    *ok = false;
                    ERROR_CRITICAL_NO_MEMORY();
                }
                return event;

            }
            // for old compatibility
            SysExEvent *event = new SysExEvent(channel, array, track);

            if(event)
                *ok = true;
            else {
                *ok = false;
                ERROR_CRITICAL_NO_MEMORY();
            }
            return event;
        }

        case 0x0F: {
            // MetaEvent
            if (!startByte) {
                (*content) >> tempByte;
            } else {
                tempByte = secondByte;
            }
            switch (tempByte) {
            case 0x51: {
                // TempoChange
                //(*content)>>tempByte;
                //if(tempByte!=3){
                //	*ok = false;
                //	return 0;
                //}
                quint32 value;
                (*content) >> value;
                // 1te Stelle abziehen,
                value -= 50331648;

                TempoChangeEvent *event = new TempoChangeEvent(17, (int)value, track);

                if(event)
                    *ok = true;
                else {
                    *ok = false;
                    ERROR_CRITICAL_NO_MEMORY();
                }
                return event;
            }
            case 0x58: {
                // TimeSignature
                (*content) >> tempByte;
                if (tempByte != 4) {
                    *ok = false;
                    return 0;
                }

                (*content) >> tempByte;
                int num = (int)tempByte;
                (*content) >> tempByte;
                int denom = (int)tempByte;
                (*content) >> tempByte;
                int metronome = (int)tempByte;
                (*content) >> tempByte;
                int num32 = (int)tempByte;

                TimeSignatureEvent *event = new TimeSignatureEvent(18, num, denom, metronome, num32, track);

                if(event)
                    *ok = true;
                else {
                    *ok = false;
                    ERROR_CRITICAL_NO_MEMORY();
                }
                return event;

            }
            case 0x59: {
                // keysignature
                (*content) >> tempByte;
                if (tempByte != 2) {
                    *ok = false;
                    return 0;
                }
                qint8 t;
                (*content) >> t;
                int tonality = (int)t;
                (*content) >> tempByte;
                bool minor = true;
                if (tempByte == 0) {
                    minor = false;
                }

                KeySignatureEvent *event = new KeySignatureEvent(channel, tonality, minor, track);

                if(event)
                    *ok = true;
                else {
                    *ok = false;
                    ERROR_CRITICAL_NO_MEMORY();
                }
                return event;

            }
            case 0x2F: {
                // end Event
                *endEvent = true;
                *ok = true;
                return 0;
            }
            default: {
                if (tempByte >= 0x01 && tempByte <= 0x07) {

                    // textevent
                    // read type
                    TextEvent* textEvent = new TextEvent(channel, track);

                    if(textEvent)
                        *ok = true;
                    else {
                        *ok = false;
                        ERROR_CRITICAL_NO_MEMORY();
                    }

                    textEvent->setType(tempByte);
                    int length = MidiFile::variableLengthvalue(content);
#if 0
                    // use wchar_t because some files use Unicode.
                    wchar_t str[128] = L"";
                    for (int i = 0; i < length; i++) {
                        (*content) >> tempByte;
                        wchar_t temp[2] = { btowc(tempByte) };
                        wcsncat(str, temp, 1);
                    }
                    textEvent->setText(QString::fromWCharArray(str));
#else
 // Modified by Estwald:

                    QString str;
                    char text[length + 1];
                    int counter =0;
                    int is_utf8 = 0;
                    for (int i = 0; i < length; i++) {
                        (*content) >> tempByte;
                        text[i] = tempByte;
                        if(tempByte & 128) {// UTF8 autodetecting decoding chars
                            if(counter == 0) {
                               if((tempByte & ~7) == 0xF0) counter = 3;
                               else if((tempByte & ~15) == 0xE0) counter = 2;
                               else if((tempByte & ~31) == 0xC0) counter = 1;
                            } else if((tempByte & 0xC0) == 0x80) {
                                counter--;
                                if(!counter) is_utf8 = 1;
                            } else counter = 0;
                        } else counter = 0;
                    }
/*
   NOTE: files can use ASCII (and derivates) or UTF8.
   MIDI Editor asumes UTF8 format and because it, is very easy one
   bad conversion/encoding of the text from olds MIDIs, ecc.

   ASCII section (0 to 127 ch) is the same thing, but 128+ in
   UTF8 characters is codified as 2 to 4 bytes length and you
   can try to detect this encode method to import correctly
   UTF8 characters, assuming Local8Bit (i have some .kar/.mid
   in this format and works fine) as alternative.

*/

                    if(!is_utf8)
                        str = QString::fromLocal8Bit((const char *) text, length);
                    else
                        str = QString::fromUtf8((const char *) text, length);
                    textEvent->setText(str);
#endif
                    *ok = true;
                    return textEvent;

                } else {

                    // tempByte is meta event type
                    int typeByte = ((char)tempByte);

                    // read length
                    int length = MidiFile::variableLengthvalue(content);

                    // content
                    QByteArray array;
                    for (int i = 0; i < length; i++) {
                        (*content) >> tempByte;
                        array.append((char)tempByte);
                    }

                    UnknownEvent *event = new UnknownEvent(channel, typeByte, array, track);

                    if(event)
                        *ok = true;
                    else {
                        *ok = false;
                        ERROR_CRITICAL_NO_MEMORY();
                    }
                    return event;

                }
            }
            }
        }
        }
    }
    }

    // if the event could not be loaded try to use old firstByte before the new
    // data.
    // To do this, pass prefFirstByte and secondByte (the current firstByte)
    // and use it recursive.
    _startByte = prevStartByte;
    return loadMidiEvent(content, ok, endEvent, track, _startByte, tempByte);
}

void MidiEvent::setTrack(MidiTrack* track, bool toProtocol)
{
    ProtocolEntry* toCopy = copy();
    if(!toCopy)
        ERROR_CRITICAL_NO_MEMORY2();

    _track = track;
    if (toProtocol) {
        midi_modified = true;
        protocol(toCopy, this);
    } else {
        delete toCopy;
    }
}

MidiTrack* MidiEvent::track()
{
    return _track;
}

void MidiEvent::setChannel(int ch, bool toProtocol)
{
    int oldChannel = channel();
    ProtocolEntry* toCopy = copy();
    if(!toCopy)
        ERROR_CRITICAL_NO_MEMORY2();
    numChannel = ch;
    if (toProtocol) {
        midi_modified = true;
        protocol(toCopy, this);
        file()->channelEvents(oldChannel)->remove(midiTime(), this);
        // tells the new channel to add this event
        setMidiTime(midiTime(), toProtocol);
    } else {
        delete toCopy;
    }
}

int MidiEvent::channel()
{
    return numChannel;
}

QString MidiEvent::toMessage()
{
    return "";
}

QByteArray MidiEvent::save()
{
    return QByteArray();
}

void MidiEvent::setMidiTime(int t, bool toProtocol)
{

    // if its once TimeSig / TempoChange at 0, dont delete event
    if (toProtocol && (channel() == 18 || channel() == 17)) {
        if (midiTime() == 0 && midiFile->channel(channel())->eventMap()->count(0) == 1) {
            return;
        }
    }

    ProtocolEntry* toCopy = copy();
    if(!toCopy)
        ERROR_CRITICAL_NO_MEMORY2();
    file()->channelEvents(numChannel)->remove(timePos, this);
    timePos = t;
    if (timePos > file()->endTick()) {
        file()->setMaxLengthMs(file()->msOfTick(timePos) + 100);
    }
    midi_modified = true;

    if (toProtocol) {
        protocol(toCopy, this);
    } else {
        delete toCopy;
    }

    if(numChannel >= 17)
        file()->channelEvents(numChannel)->replace(timePos, this);
    else
        file()->channelEvents(numChannel)->insert(timePos, this);
}

int MidiEvent::midiTime()
{
    return timePos;
}

void MidiEvent::setFile(MidiFile* f)
{
    midiFile = f;
}

MidiFile* MidiEvent::file()
{
    return midiFile;
}

int MidiEvent::line()
{
    return 0;
}

void MidiEvent::draw(QPainter* p, QColor c, int mode)
{
    if(mode & 1)
        p->setPen(QPen(Qt::black, 2, Qt::SolidLine));
    else
        p->setPen(Qt::gray);

    p->setBrush(c);
    p->drawRoundedRect(x(), y(), width(), height(), 2, 2);

    if(c.alpha() == 255) {

        QLinearGradient linearGrad(QPointF(x(), y()), QPointF(x() + width(), y() + height()));
        linearGrad.setColorAt(0, QColor(80, 80, 80, 0x40));
        linearGrad.setColorAt(0.5, QColor(0xcf, 0xcf, 0xcf, 0x70));
        linearGrad.setColorAt(1.0, QColor(0xff, 0xff, 0xff, 0x70));

        QBrush d(linearGrad);
        p->setBrush(d);
        p->drawRoundedRect(x(), y(), width(), height(), 2, 2);
    }


}

void MidiEvent::draw2(QPainter* p, QColor c, Qt::BrushStyle bs, int mode)
{
    if(mode & 1)
        p->setPen(QPen(Qt::black, 2, Qt::SolidLine));
    else
        p->setPen(Qt::gray);

    QBrush d(c, bs);

    if(c.alpha() == 255) {
        p->setBrush(QColor(0, 0, 0, c.alpha()));
        p->drawRoundedRect(x(), y(), width(), height(), 2, 2);
    }

    p->setBrush(d);
    p->drawRoundedRect(x(), y(), width(), height(), 2, 2);

    if(c.alpha() == 255) {

        QLinearGradient linearGrad(QPointF(x(), y()), QPointF(x() + width(), y() + height()));
        linearGrad.setColorAt(0, QColor(80, 80, 80, 0x40));
        linearGrad.setColorAt(0.5, QColor(0xcf, 0xcf, 0xcf, 0x70));
        linearGrad.setColorAt(1.0, QColor(0xff, 0xff, 0xff, 0x70));

        QBrush d(linearGrad);
        p->setBrush(d);
        p->drawRoundedRect(x(), y(), width(), height(), 2, 2);
    }

}

ProtocolEntry* MidiEvent::copy()
{
    MidiEvent *event = new MidiEvent(*this);
    if(!event)
        ERROR_CRITICAL_NO_MEMORY3();
    return event;
}

void MidiEvent::reloadState(ProtocolEntry* entry)
{

    MidiEvent* other = dynamic_cast<MidiEvent*>(entry);
    if (!other) {
        return;
    }
    _track = other->_track;
    file()->channelEvents(numChannel)->remove(timePos, this);
    numChannel = other->numChannel;
    file()->channelEvents(numChannel)->remove(timePos, this);
    timePos = other->timePos;
    if(numChannel >= 17)
        file()->channelEvents(numChannel)->replace(timePos, this);
    else
        file()->channelEvents(numChannel)->insert(timePos, this);
    midiFile = other->midiFile;
    midi_modified = other->midi_modified;

}

QString MidiEvent::typeString()
{
    return "Midi Event";
}

void MidiEvent::setEventWidget(EventWidget* widget)
{
    _eventWidget = widget;
}

EventWidget* MidiEvent::eventWidget()
{
    return _eventWidget;
}

bool MidiEvent::shownInEventWidget()
{
    if (!_eventWidget) {
        return false;
    }
    return _eventWidget->events().contains(this);
}

bool MidiEvent::isOnEvent()
{
    return true;
}

QMap<int, QString> MidiEvent::knownMetaTypes()
{
    QMap<int, QString> meta;
    for (int i = 1; i < 8; i++) {
        meta.insert(i, "Text Event");
    }
    meta.insert(0x51, "Tempo Change Event");
    meta.insert(0x58, "Time Signature Event");
    meta.insert(0x59, "Key Signature Event");
    meta.insert(0x2F, "End of Track");
    return meta;
}

void MidiEvent::setTemporaryRecordID(int id)
{
    _tempID = id;
}

int MidiEvent::temporaryRecordID()
{
    return _tempID;
}

void MidiEvent::moveToChannel(int ch)
{

    int oldChannel = channel();

    if (oldChannel > 15) {
        return;
    }

    if (oldChannel == ch) {
        return;
    }

    midiFile->channel(oldChannel)->removeEvent(this);

    ProtocolEntry* toCopy = copy();

    numChannel = ch;

    midi_modified = true;
    protocol(toCopy, this);

    midiFile->channel(ch)->insertEvent(this, midiTime());
}
