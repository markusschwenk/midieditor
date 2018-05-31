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

#include "ProtocolEntry.h"
#include "../midi/MidiFile.h"
#include "Protocol.h"
#include "ProtocolItem.h"

void ProtocolEntry::protocol(ProtocolEntry* oldObj, ProtocolEntry* newObj)
{

    if (oldObj->file() && oldObj->file()->protocol()) {
        oldObj->file()->protocol()->enterUndoStep(
            new ProtocolItem(oldObj, newObj));
    }
}

MidiFile* ProtocolEntry::file()
{
    // This has to be implemented in the Subclasses
    return 0;
}

void ProtocolEntry::reloadState(ProtocolEntry* entry)
{

    Q_UNUSED(entry);

    // This has to be implemented in the Subclasses
    return;
}

ProtocolEntry* ProtocolEntry::copy()
{

    // This has to be implemented in the Subclasses
    return 0;
}

ProtocolEntry::~ProtocolEntry()
{
}
