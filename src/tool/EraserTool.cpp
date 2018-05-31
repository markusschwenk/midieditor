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

#include "EraserTool.h"

#include "../MidiEvent/MidiEvent.h"
#include "../gui/MatrixWidget.h"
#include "../midi/MidiChannel.h"
#include "../midi/MidiFile.h"
#include "../protocol/Protocol.h"
#include "Selection.h"

EraserTool::EraserTool()
    : EventTool()
{
    setImage(":/run_environment/graphics/tool/eraser.png");
    setToolTipText("Eraser (remove Events)");
}

EraserTool::EraserTool(EraserTool& other)
    : EventTool(other)
{
    return;
}

ProtocolEntry* EraserTool::copy()
{
    return new EraserTool(*this);
}

void EraserTool::reloadState(ProtocolEntry* entry)
{
    EraserTool* other = dynamic_cast<EraserTool*>(entry);
    if (!other) {
        return;
    }
    EventTool::reloadState(entry);
}

void EraserTool::draw(QPainter* painter)
{
    foreach (MidiEvent* ev, *(matrixWidget->activeEvents())) {
        if (pointInRect(mouseX, mouseY, ev->x(), ev->y(), ev->x() + ev->width(),
                ev->y() + ev->height())) {
            painter->fillRect(ev->x(), ev->y(), ev->width(), ev->height(), Qt::black);
        }
    }
}

bool EraserTool::move(int mouseX, int mouseY)
{
    EventTool::move(mouseX, mouseY);
    return true;
}

bool EraserTool::release()
{
    currentProtocol()->startNewAction("Remove event", image());
    foreach (MidiEvent* ev, *(matrixWidget->activeEvents())) {
        if (pointInRect(mouseX, mouseY, ev->x(), ev->y(), ev->x() + ev->width(),
                ev->y() + ev->height())) {
            file()->channel(ev->channel())->removeEvent(ev);
            if (Selection::instance()->selectedEvents().contains(ev)) {
                deselectEvent(ev);
            }
        }
    }
    currentProtocol()->endAction();
    return true;
}
