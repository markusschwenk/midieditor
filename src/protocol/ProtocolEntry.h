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

#ifndef PROTOCOLENTRY_H
#define PROTOCOLENTRY_H

class MidiFile;

/**
 * \class ProtocolEntry
 *
 * \brief ProtocolEntry is the superclass for objects to protocol.
 *
 * ProtocolEntry is a Superclass for every kind of object which can be written
 * into the Programs Protocol.
 * To protocol the state of a ProtocolEntry, the object is copied
 * (ProtocolEntry *copy())). When the Protocols Method undo() or redo() is
 * called, the ProtocolEntry has to reload his old state from the copy in the
 * Protocol (void reloadState(ProtocolEntry *entry)). Both Methods have to be
 * implemented in the Subclass.
 * Before a ProtocolEntry is changed it has to copy its old state to oldObj.
 * After changing itself, the Method protocol(ProtocolEntry *oldObj,
 * ProtocolEntry *newObj) has to be called, with "this" as newObj.
 */
class ProtocolEntry {

public:
    virtual ~ProtocolEntry();

    /**
		 * \brief copies the ProtocolEntry.
		 *
		 * copy() should save the ProtocolEntrys state so that it will be able
		 * to reload his old state in reloadState().
		 * All important data has to be saved to the returned object; there is
		 * no need to save Layoutinformation because the program will relayout
		 * after every call of the protocol.
		 */
    virtual ProtocolEntry* copy();

    /**
		 * \brief reloads the state of entry.
		 *
		 * This Method has to reload the Objects old state, written to entry in
		 * copy().
		 */
    virtual void reloadState(ProtocolEntry* entry);

    /**
		 * \brief writes the old object oldObj and the new object newObj to the
		 * protocol.
		 */
    virtual void protocol(ProtocolEntry* oldObj, ProtocolEntry* newObj);

    /**
		 * \brief return the entries file.
		 */
    virtual MidiFile* file();
};

#endif
