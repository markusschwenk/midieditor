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

/**
 * \file midi/MidiChannel.h
 *
 * \brief Headerfile.
 *
 * Contains the class description for ProtocolStep.
 */

#ifndef MIDICHANNEL_H_
#define MIDICHANNEL_H_

#include "../protocol/ProtocolEntry.h"
#include <QMultiMap>

class MidiFile;
class MidiEvent;
class QColor;
class MidiTrack;
class NoteOnEvent;

/**
 * \class MidiChannel
 *
 * \brief Every MidiChannel represents an Instrument.
 *
 * In this program there are 18 MidiChannel: the 16 MidiChannels a MidiFile
 * can describe, the Channel saving general Events (like PitchBend), one channel
 * for TempoChangeEvents and one for TimeSignatureEvents.
 *
 * Every channel can be mutes, visible in the MatrixWidget or playing alone
 * (solomode).
 *
 * Every Channel saves his own Events in a QMultiMap of the format miditick,
 * Event.
 */
class MidiChannel : public ProtocolEntry {

public:
    /**
		 * \brief creates a new MidiChannel with number num.
		 *
		 * Sets the channels file to f.
		 */
    MidiChannel(MidiFile* f, int num);

    /**
		 * \brief creates a copy of other.
		 */
    MidiChannel(MidiChannel& other);

    /**
		 * \brief returns the channels file.
		 */
    MidiFile* file();

    /**
		 * \brief returns the channels number.
		 *
		 * 0-15 are MidiChannels, 16 for general Events, 17 for TempoChanges
		 * and 18 for TimeSignatureEvents.
		 */
    int number();

    /**
		 * \brief returns the channels color.
		 *
		 * The color only depends on the channel number.
		 */
    QColor* color();

    /**
		 * \brief returns the eventMap of the channel.
		 *
		 * This contains all MidiEvents of the channel.
		 */
    QMultiMap<int, MidiEvent*>* eventMap();

    /**
		 * \brief inserts a note to this channel.
		 */
    NoteOnEvent* insertNote(int note, int startTick, int endTick, int velocity, MidiTrack* track);

    /**
		 * \brief inserts event into the channels map.
		 */
    void insertEvent(MidiEvent* event, int tick);

    /**
		 * \brief removes event from the eventMap.
		 */
    bool removeEvent(MidiEvent* event);

    /**
		 * \brief returns the program number of the midi program at tick.
		 */
    int progAtTick(int tick);

    /**
		 * \brief returns whether the channel is visible in the MatrixWidget.
		 */
    bool visible();

    /**
		 * \brief sets the channels visibility to b.
		 */
    void setVisible(bool b);

    /**
		 * \brief returns whether the channel is muted.
		 */
    bool mute();

    /**
		 * \brief sets the channel mute or makes it loud.
		 */
    void setMute(bool b);

    /**
		 * \brief returns whether the channel is playing in solo mode.
		 *
		 * If the channel is in solo mode, all other channels are muted.
		 */
    bool solo();

    /**
		 * \brief sets the solo mode to b.
		 */
    void setSolo(bool b);

    /**
		 * \brief removes all events of the channel.
		 */
    void deleteAllEvents();

    /*
		 * The following methods reimplement methods from the superclass
		 * ProtocolEntry
		 */
    ProtocolEntry* copy();

    void reloadState(ProtocolEntry* entry);

protected:
    /**
		 * \brief the midiFile of this channel.
		 */
    MidiFile* _midiFile;

    /**
		 * \brief the flags solo, mute and visible.
		 */
    bool _visible, _mute, _solo;

    /**
		 * \brief contains all MidiEvents of the channel sorted by their tick.
		 */
    QMultiMap<int, MidiEvent*>* _events;

    /**
		 * \brief the channels number.
		 */
    int _num;
};

#endif
