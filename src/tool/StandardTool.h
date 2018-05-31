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

#ifndef STANDARDTOOL_H_
#define STANDARDTOOL_H_

#include "EventTool.h"

class EventMoveTool;
class SelectTool;
class SizeChangeTool;
class NewNoteTool;

class StandardTool : public EventTool {

public:
    StandardTool();
    StandardTool(StandardTool& other);

    void draw(QPainter* painter);
    bool press(bool leftClick);
    bool move(int mouseX, int mouseY);
    bool release();

    ProtocolEntry* copy();
    void reloadState(ProtocolEntry* entry);
    bool showsSelection();

private:
    EventMoveTool* moveTool;
    SelectTool* selectTool;
    SizeChangeTool* sizeChangeTool;
    NewNoteTool* newNoteTool;
};
#endif
