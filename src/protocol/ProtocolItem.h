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

#ifndef PROTOCOLITEM_H
#define PROTOCOLITEM_H

class ProtocolEntry;

/**
 * \class ProtocolItem
 *
 * \brief Single action saving the old object and the new object.
 *
 * ProtocolItem saves a new ProtocolEntry and an old ProtocolEntry.
 * The old saves the state which will be reloaded by the new Object
 * on calling release().
 */
class ProtocolItem {

public:
    /**
		 * \brief Creates a new ProtocolItem.
		 */
    ProtocolItem(ProtocolEntry* oldObj, ProtocolEntry* newObj);

    /**
		 * \brief reloads the state of oldObj on newObj.
		 *
		 * Will call newObj.reloadState(newObj) and return a new ProtocolItem.
		 * This new ProtocolItem will store the reverse order: it contains the
		 * information needed to load the state of the new Object to the old
		 * Object.
		 */
    ProtocolItem* release();

private:
    /**
		 * \brief Both states of the Object.
		 */
    ProtocolEntry *_oldObject, *_newObject;
};
#endif
