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

#ifndef SIZECHANGETOOL_H_
#define SIZECHANGETOOL_H_

#include "EventTool.h"

class SizeChangeTool : public EventTool {

public:
    SizeChangeTool();
    SizeChangeTool(SizeChangeTool& other);

    ProtocolEntry* copy();
    void reloadState(ProtocolEntry* entry);

    void draw(QPainter* painter);
    bool press(bool leftClick);
    bool release();
    bool move(int mouseX, int mouseY);
    bool releaseOnly();

    bool showsSelection();

private:
    bool inDrag, dragsOnEvent;
    int xPos;
};

#endif
