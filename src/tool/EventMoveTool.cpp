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

#include "EventMoveTool.h"

#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/NoteOnEvent.h"
#include "../MidiEvent/OffEvent.h"
#include "../gui/MatrixWidget.h"
#include "../midi/MidiFile.h"
#include "../protocol/Protocol.h"
#include "Selection.h"
#include "StandardTool.h"

EventMoveTool::EventMoveTool(bool upDown, bool leftRight)
    : EventTool()
{
    moveUpDown = upDown;
    moveLeftRight = leftRight;
    inDrag = false;
    startX = 0;
    startY = 0;
    if (moveUpDown && moveLeftRight) {
        setImage(":/run_environment/graphics/tool/move_up_down_left_right.png");
        setToolTipText("Move Events (all directions)");
    } else if (moveUpDown) {
        setImage(":/run_environment/graphics/tool/move_up_down.png");
        setToolTipText("Move Events (up and down)");
    } else {
        setImage(":/run_environment/graphics/tool/move_left_right.png");
        setToolTipText("Move Events (left and right)");
    }
}

EventMoveTool::EventMoveTool(EventMoveTool& other)
    : EventTool(other)
{
    moveUpDown = other.moveUpDown;
    moveLeftRight = other.moveLeftRight;
    inDrag = false;
    startX = 0;
    startY = 0;
}

ProtocolEntry* EventMoveTool::copy()
{
    return new EventMoveTool(*this);
}

void EventMoveTool::reloadState(ProtocolEntry* entry)
{
    EventTool::reloadState(entry);
    EventMoveTool* other = dynamic_cast<EventMoveTool*>(entry);
    if (!other) {
        return;
    }
    moveUpDown = other->moveUpDown;
    moveLeftRight = other->moveLeftRight;

    inDrag = false;
    startX = 0;
    startY = 0;
}

void EventMoveTool::draw(QPainter* painter)
{
    paintSelectedEvents(painter);
    int currentX = computeRaster();

    if (inDrag) {
        int shiftX = startX - currentX;
        if (!moveLeftRight) {
            shiftX = 0;
        }
        int shiftY = startY - mouseY;
        if (!moveUpDown) {
            shiftY = 0;
        }
        double lineHeight = matrixWidget->lineHeight();
        int nLines = qAbs(shiftY) / lineHeight;
        if (shiftY < 0) {
            shiftY = -nLines * lineHeight;
        } else {
            shiftY = nLines * lineHeight;
        }
        foreach (MidiEvent* event, Selection::instance()->selectedEvents()) {
            int customShiftY = shiftY;
            if (event->line() > 127) {
                customShiftY = 0;
            }
            if (event->shown()) {
                painter->setPen(Qt::lightGray);
                painter->setBrush(Qt::darkBlue);
                painter->drawRoundedRect(event->x() - shiftX, event->y() - customShiftY,
                    event->width(), event->height(), 1, 1);
                painter->setPen(Qt::gray);
                painter->drawLine(event->x() - shiftX, 0, event->x() - shiftX,
                    matrixWidget->height());
                painter->drawLine(event->x() + event->width() - shiftX, 0,
                    event->x() + event->width() - shiftX, matrixWidget->height());
                painter->setPen(Qt::black);
            }
        }
    }
}

bool EventMoveTool::press(bool leftClick)
{
    Q_UNUSED(leftClick);
    inDrag = true;
    startX = mouseX;
    startY = mouseY;
    if (Selection::instance()->selectedEvents().count() > 0) {
        if (moveUpDown && moveLeftRight) {
            matrixWidget->setCursor(Qt::SizeAllCursor);
        } else if (moveUpDown) {
            matrixWidget->setCursor(Qt::SizeVerCursor);
        } else {
            matrixWidget->setCursor(Qt::SizeHorCursor);
        }
    }
    return true;
}

bool EventMoveTool::release()
{
    inDrag = false;
    matrixWidget->setCursor(Qt::ArrowCursor);
    int currentX = computeRaster();
    int shiftX = startX - currentX;
    if (!moveLeftRight) {
        shiftX = 0;
    }
    int shiftY = startY - mouseY;
    if (!moveUpDown) {
        shiftY = 0;
    }
    double lineHeight = matrixWidget->lineHeight();
    int numLines = shiftY / lineHeight;

    // return when there shiftX/shiftY is too small or there are no selected
    // events
    if (Selection::instance()->selectedEvents().count() == 0 || (-2 <= shiftX && shiftX <= 2 && -2 <= shiftY && shiftY <= 2)) {
        if (_standardTool) {
            Tool::setCurrentTool(_standardTool);
            _standardTool->move(mouseX, mouseY);
            _standardTool->release();
        }
        return true;
    }

    currentProtocol()->startNewAction("Move events", image());

    // backwards to hold stability
    for (int i = Selection::instance()->selectedEvents().count() - 1; i >= 0; i--) {
        MidiEvent* event = Selection::instance()->selectedEvents().at(i);
        NoteOnEvent* ev = dynamic_cast<NoteOnEvent*>(event);
        OffEvent* off = dynamic_cast<OffEvent*>(event);
        if (ev) {
            int note = ev->note() + numLines;
            if (note < 0) {
                note = 0;
            }
            if (note > 127) {
                note = 127;
            }
            ev->setNote(note);
            changeTick(ev, shiftX);
            if (ev->offEvent()) {
                changeTick(ev->offEvent(), shiftX);
            }
        } else if (!off) {
            changeTick(event, shiftX);
        }
    }

    currentProtocol()->endAction();
    if (_standardTool) {
        Tool::setCurrentTool(_standardTool);
        _standardTool->move(mouseX, mouseY);
        _standardTool->release();
    }
    return true;
}

bool EventMoveTool::move(int mouseX, int mouseY)
{
    EventTool::move(mouseX, mouseY);
    return inDrag;
}

bool EventMoveTool::releaseOnly()
{
    inDrag = false;
    matrixWidget->setCursor(Qt::ArrowCursor);
    startX = 0;
    startY = 0;
    return true;
}

void EventMoveTool::setDirections(bool upDown, bool leftRight)
{
    moveUpDown = upDown;
    moveLeftRight = leftRight;
}

bool EventMoveTool::showsSelection()
{
    return true;
}

int EventMoveTool::computeRaster()
{

    if (!moveLeftRight) {
        return mouseX;
    }

    // get all selected events and search for first event / last event
    int firstTick = -1;
    int lastTick = -1;

    foreach (MidiEvent* event, Selection::instance()->selectedEvents()) {

        if ((firstTick == -1) || (event->midiTime() < firstTick)) {
            firstTick = event->midiTime();
        }

        NoteOnEvent* onEvent = dynamic_cast<NoteOnEvent*>(event);
        if (onEvent) {
            if ((lastTick == -1) || (onEvent->offEvent()->midiTime() > lastTick)) {
                lastTick = onEvent->offEvent()->midiTime();
            }
        }
    }

    // compute x positions and compute raster
    bool useLast = (lastTick >= 0) && lastTick <= matrixWidget->maxVisibleMidiTime() && lastTick >= matrixWidget->minVisibleMidiTime();
    bool useFirst = (firstTick >= 0) && firstTick <= matrixWidget->maxVisibleMidiTime() && firstTick >= matrixWidget->minVisibleMidiTime();

    if (!useFirst && !useLast) {
        return mouseX;
    }

    int firstX, distFirst;
    int lastX, distLast;

    if (useFirst) {
        int firstXReal = matrixWidget->xPosOfMs(file()->msOfTick(firstTick)) + mouseX - startX;
        firstX = rasteredX(firstXReal);
        distFirst = firstX - firstXReal;
        if (distFirst == 0) {
            useFirst = false;
        }
    }
    if (useLast) {
        int lastXReal = matrixWidget->xPosOfMs(file()->msOfTick(lastTick)) + mouseX - startX;
        lastX = rasteredX(lastXReal);
        distLast = lastX - lastXReal;
        if (distLast == 0) {
            useLast = false;
        }
    }

    if (useFirst && useLast) {
        if (qAbs(distFirst) < qAbs(distLast)) {
            useLast = false;
        } else {
            useFirst = false;
        }
    }

    int dist;
    if (useFirst) {
        dist = distFirst;
    } else if (useLast) {
        dist = distLast;
    } else {
        dist = 0;
    }

    return mouseX + dist;
}
