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

#include "SizeChangeTool.h"

#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/OffEvent.h"
#include "../MidiEvent/OnEvent.h"
#include "../gui/MatrixWidget.h"
#include "../midi/MidiChannel.h"
#include "../midi/MidiFile.h"
#include "../protocol/Protocol.h"
#include "StandardTool.h"

#include "Selection.h"

SizeChangeTool::SizeChangeTool()
    : EventTool()
{
    inDrag = false;
    xPos = 0;
    dragsOnEvent = false;
    setImage(":/run_environment/graphics/tool/change_size.png");
    setToolTipText("Change the duration of the selected event");
}

SizeChangeTool::SizeChangeTool(SizeChangeTool& other)
    : EventTool(other)
{
    return;
}

ProtocolEntry* SizeChangeTool::copy()
{
    return new SizeChangeTool(*this);
}

void SizeChangeTool::reloadState(ProtocolEntry* entry)
{
    SizeChangeTool* other = dynamic_cast<SizeChangeTool*>(entry);
    if (!other) {
        return;
    }
    EventTool::reloadState(entry);
}

void SizeChangeTool::draw(QPainter* painter)
{

    int currentX = rasteredX(mouseX);
    bool setArrow = true;

    if (!inDrag) {
        paintSelectedEvents(painter);
        return;
    } else {
        painter->setPen(Qt::gray);
        painter->drawLine(currentX, 0, currentX, matrixWidget->height());
        painter->setPen(Qt::black);
    }
    int endEventShift = 0;
    int startEventShift = 0;
    if (dragsOnEvent) {
        startEventShift = currentX - xPos;
    } else {
        endEventShift = currentX - xPos;
    }
    foreach (MidiEvent* event, Selection::instance()->selectedEvents()) {
        bool show = event->shown();
        OnEvent* on = dynamic_cast<OnEvent*>(event);
        if (!show) {

            if (on) {
                show = on->offEvent() && on->offEvent()->shown();
            }
        }
        if (show) {

            int sw = event->width() / 2 - 2;
            if(sw < 2)
                sw = 2;
            else
                sw = 4;

            painter->fillRect(event->x() + startEventShift, event->y(),
                event->width() - startEventShift + endEventShift,
                event->height(), Qt::black);
            if (on && pointInRect(mouseX, mouseY, event->x() + event->width() - sw + endEventShift, event->y(), event->x() + event->width() + 2 + endEventShift, event->y() + event->height())) {
                matrixWidget->setCursor(Qt::SplitHCursor);
                setArrow = false;
            }
            if (on && pointInRect(mouseX, mouseY, event->x() - 2 + startEventShift, event->y(), event->x() + sw + startEventShift, event->y() + event->height())) {
                matrixWidget->setCursor(Qt::SplitHCursor);
                setArrow = false;
            }
        }
    }

    if(setArrow)
        matrixWidget->setCursor(Qt::ArrowCursor);
}

bool SizeChangeTool::press(bool leftClick)
{

    Q_UNUSED(leftClick);

    inDrag = false;
    xPos = mouseX;

    int minor_X = -1;
    int minor_W = -1;

    foreach (MidiEvent* event, Selection::instance()->selectedEvents()) {

        OnEvent* on = dynamic_cast<OnEvent*>(event);
        int sw = event->width() / 2 - 2;
        if(sw < 2)
            sw = 2;
        else
            sw = 4;

        if (on) {
            if(minor_X == -1) {
                minor_X = event->x();
                minor_W = event->width();
            } else if(event->x() < minor_X){
                minor_X = event->x();
                minor_W = event->width();
            }
        }

        if (on && pointInRect(mouseX, mouseY, event->x() + event->width() - sw, event->y(),
                event->x() + event->width() + 2, event->y() + event->height())) {
            dragsOnEvent = false;
            inDrag = true;
            xPos = event->x() + event->width();
            return true;
        }

        if (on && pointInRect(mouseX, mouseY, event->x() - 2, event->y(), event->x() + sw,
                event->y() + event->height())) {
            dragsOnEvent = true;
            xPos = event->x();
            inDrag = true;
            return true;
        }
    }

    if(minor_X >= 0) { // use first note by default
        xPos = minor_X + minor_W;
        dragsOnEvent = false;
        inDrag = true;
        return true;
    }

    return false;
}

bool SizeChangeTool::release()
{

    int currentX = rasteredX(mouseX);

    inDrag = false;
    int endEventShift = 0;
    int startEventShift = 0;

    if (dragsOnEvent) {
        startEventShift = currentX - xPos;
    } else {
        endEventShift = currentX - xPos;
    }

    int size_x = matrixWidget->divs().at(1).first - matrixWidget->divs().at(0).first - 2;
    if(size_x < 5)
        size_x = 5;

    xPos = 0;

    if (Selection::instance()->selectedEvents().count() > 0) {
        currentProtocol()->startNewAction("Change event duration", image());
        foreach (MidiEvent* event, Selection::instance()->selectedEvents()) {
            OnEvent* on = dynamic_cast<OnEvent*>(event);
            OffEvent* off = dynamic_cast<OffEvent*>(event);
            if (on) {
                off = on->offEvent();
                if(off) {

                    int onTick = file()->tick(file()->msOfTick(on->midiTime()) - matrixWidget->timeMsOfWidth(-startEventShift));
                    int offTick = file()->tick(file()->msOfTick(off->midiTime()) - matrixWidget->timeMsOfWidth(-endEventShift));

                    if (onTick < offTick && (!_magnet || (_magnet && (offTick - onTick) >= size_x))) {

                        if (dragsOnEvent) {
                            changeTick(on, -startEventShift);
                        } else {
                            changeTick(off, -endEventShift);
                        }

                        // check

                        onTick = on->midiTime();
                        offTick = off->midiTime();

                        if(offTick < onTick) {

                            if((onTick - offTick) < 15 ) {
                                onTick += 15;
                            }

                            on->setMidiTime(offTick);

                            off->setMidiTime(onTick);

                        } else {
                            if((offTick - onTick) < 15 ) {
                                off->setMidiTime(offTick + 15);
                            }
                        }
                    }
                }
            } else if (off) {
                // do nothing; endEvents are shifted when touching their OnEvent
                continue;
            } else if (dragsOnEvent) {
                // normal events will be moved as normal (???)
                // changeTick(event, -startEventShift);
            }
        }
        currentProtocol()->endAction();
    }
    matrixWidget->setCursor(Qt::ArrowCursor);
    if (_standardTool) {
        Tool::setCurrentTool(_standardTool);
        _standardTool->move(mouseX, mouseY);
        _standardTool->release();
    }
    return true;
}

bool SizeChangeTool::move(int mouseX, int mouseY)
{
    EventTool::move(mouseX, mouseY);
    foreach (MidiEvent* event, Selection::instance()->selectedEvents()) {

        OnEvent* on = dynamic_cast<OnEvent*>(event);
        int sw = event->width() / 2 - 2;
        if(sw < 2)
            sw = 2;
        else
            sw = 4;

        if(on && pointInRect(mouseX, mouseY, event->x() + event->width() - sw, event->y(),
                event->x() + event->width() + 2, event->y() + event->height())) {
            matrixWidget->setCursor(Qt::SplitHCursor);
            return inDrag;
        }

        if (on && pointInRect(mouseX, mouseY, event->x() - 2, event->y(), event->x() + sw,
                event->y() + event->height())) {
            matrixWidget->setCursor(Qt::SplitHCursor);
            return inDrag;
        }
    }
    matrixWidget->setCursor(Qt::ArrowCursor);
    return inDrag;
}

bool SizeChangeTool::releaseOnly()
{
    inDrag = false;
    xPos = 0;
    return true;
}

bool SizeChangeTool::showsSelection()
{
    return true;
}
