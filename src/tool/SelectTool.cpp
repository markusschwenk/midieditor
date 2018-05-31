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

#include "SelectTool.h"
#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/NoteOnEvent.h"
#include "../gui/MatrixWidget.h"
#include "../midi/MidiFile.h"
#include "../protocol/Protocol.h"
#include "StandardTool.h"

SelectTool::SelectTool(int type)
    : EventTool()
{
    stool_type = type;
    x_rect = 0;
    y_rect = 0;
    switch (stool_type) {
    case SELECTION_TYPE_BOX: {
        setImage(":/run_environment/graphics/tool/select_box.png");
        setToolTipText("Select Events (Box)");
        break;
    }
    case SELECTION_TYPE_SINGLE: {
        setImage(":/run_environment/graphics/tool/select_single.png");
        setToolTipText("Select single Events");
        break;
    }
    case SELECTION_TYPE_LEFT: {
        setImage(":/run_environment/graphics/tool/select_left.png");
        setToolTipText("Select all Events on the left side");
        break;
    }
    case SELECTION_TYPE_RIGHT: {
        setImage(":/run_environment/graphics/tool/select_right.png");
        setToolTipText("Select all Events on the right side");
        break;
    }
    }
}

SelectTool::SelectTool(SelectTool& other)
    : EventTool(other)
{
    stool_type = other.stool_type;
    x_rect = 0;
    y_rect = 0;
}

void SelectTool::draw(QPainter* painter)
{
    paintSelectedEvents(painter);
    if (SELECTION_TYPE_BOX && (x_rect || y_rect)) {
        painter->setPen(Qt::gray);
        painter->setBrush(QColor(0, 0, 0, 100));
        painter->drawRect(x_rect, y_rect, mouseX - x_rect, mouseY - y_rect);
    } else if (stool_type == SELECTION_TYPE_RIGHT || stool_type == SELECTION_TYPE_LEFT) {
        if (mouseIn) {
            painter->setPen(Qt::black);
            painter->setPen(Qt::gray);
            painter->setBrush(QColor(0, 0, 0, 100));
            if (stool_type == SELECTION_TYPE_LEFT) {
                painter->drawRect(0, 0, mouseX, matrixWidget->height() - 1);
            } else {
                painter->drawRect(mouseX, 0, matrixWidget->width() - 1, matrixWidget->height() - 1);
            }
        }
    }
}

bool SelectTool::press(bool leftClick)
{
    Q_UNUSED(leftClick);
    if (stool_type == SELECTION_TYPE_BOX) {
        y_rect = mouseY;
        x_rect = mouseX;
    }
    return true;
}

bool SelectTool::release()
{

    if (!file()) {
        return false;
    }
    file()->protocol()->startNewAction("Selection changed", image());
    ProtocolEntry* toCopy = copy();

    if (!QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier) && !QApplication::keyboardModifiers().testFlag(Qt::ControlModifier)) {
        clearSelection();
    }

    if (stool_type == SELECTION_TYPE_BOX || stool_type == SELECTION_TYPE_SINGLE) {
        int x_start, y_start, x_end, y_end;
        if (stool_type == SELECTION_TYPE_BOX) {
            x_start = x_rect;
            y_start = y_rect;
            x_end = mouseX;
            y_end = mouseY;
            if (x_start > x_end) {
                int tmp = x_start;
                x_start = x_end;
                x_end = tmp;
            }
            if (y_start > y_end) {
                int tmp = y_start;
                y_start = y_end;
                y_end = tmp;
            }
        } else if (stool_type == SELECTION_TYPE_SINGLE) {
            x_start = mouseX;
            y_start = mouseY;
            x_end = mouseX + 1;
            y_end = mouseY + 1;
        }
        foreach (MidiEvent* event, *(matrixWidget->activeEvents())) {
            if (inRect(event, x_start, y_start, x_end, y_end)) {
                selectEvent(event, false);
            }
        }
    } else if (stool_type == SELECTION_TYPE_RIGHT || stool_type == SELECTION_TYPE_LEFT) {
        int tick = file()->tick(matrixWidget->msOfXPos(mouseX));
        int start, end;
        if (stool_type == SELECTION_TYPE_LEFT) {
            start = 0;
            end = tick;
        } else if (stool_type == SELECTION_TYPE_RIGHT) {
            end = file()->endTick();
            start = tick;
        }
        foreach (MidiEvent* event, *(file()->eventsBetween(start, end))) {
            selectEvent(event, false);
        }
    }

    x_rect = 0;
    y_rect = 0;

    protocol(toCopy, this);
    file()->protocol()->endAction();
    if (_standardTool) {
        Tool::setCurrentTool(_standardTool);
        _standardTool->move(mouseX, mouseY);
        _standardTool->release();
    }
    return true;
}

bool SelectTool::inRect(MidiEvent* event, int x_start, int y_start, int x_end, int y_end)
{
    return pointInRect(event->x(), event->y(), x_start, y_start, x_end, y_end) || pointInRect(event->x(), event->y() + event->height(), x_start, y_start, x_end, y_end) || pointInRect(event->x() + event->width(), event->y(), x_start, y_start, x_end, y_end) || pointInRect(event->x() + event->width(), event->y() + event->height(), x_start, y_start, x_end, y_end) || pointInRect(x_start, y_start, event->x(), event->y(), event->x() + event->width(), event->y() + event->height());
}

bool SelectTool::move(int mouseX, int mouseY)
{
    EditorTool::move(mouseX, mouseY);
    return true;
}

ProtocolEntry* SelectTool::copy()
{
    return new SelectTool(*this);
}

void SelectTool::reloadState(ProtocolEntry* entry)
{
    SelectTool* other = dynamic_cast<SelectTool*>(entry);
    if (!other) {
        return;
    }
    EventTool::reloadState(entry);
    x_rect = 0;
    y_rect = 0;
    stool_type = other->stool_type;
}

bool SelectTool::releaseOnly()
{
    return release();
}

bool SelectTool::showsSelection()
{
    return true;
}
