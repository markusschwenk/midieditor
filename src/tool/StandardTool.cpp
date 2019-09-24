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

#include "StandardTool.h"
#include "EventMoveTool.h"
#include "NewNoteTool.h"
#include "SelectTool.h"
#include "SizeChangeTool.h"

#include "../MidiEvent/MidiEvent.h"
#include "../gui/MatrixWidget.h"
#include "../midi/MidiFile.h"
#include "../protocol/Protocol.h"
#include "Selection.h"

#define NO_ACTION 0
#define SIZE_CHANGE_ACTION 1
#define MOVE_ACTION 2

StandardTool::StandardTool()
    : EventTool()
{

    setImage(":/run_environment/graphics/tool/select.png");

    moveTool = new EventMoveTool(true, true);
    moveTool->setStandardTool(this);
    sizeChangeTool = new SizeChangeTool();
    sizeChangeTool->setStandardTool(this);
    selectTool = new SelectTool(SELECTION_TYPE_BOX);
    selectTool->setStandardTool(this);
    newNoteTool = new NewNoteTool();
    newNoteTool->setStandardTool(this);

    setToolTipText("Standard Tool");
}

StandardTool::StandardTool(StandardTool& other)
    : EventTool(other)
{
    sizeChangeTool = other.sizeChangeTool;
    moveTool = other.moveTool;
    selectTool = other.selectTool;
}

void StandardTool::draw(QPainter* painter)
{
    paintSelectedEvents(painter);
}

bool StandardTool::press(bool leftClick)
{

    if (leftClick) {
        // find event to handle
        MidiEvent* event = 0;
        bool onSelectedEvent = false;
        int minDiffToMouse = 0;
        int action = NO_ACTION;
        foreach (MidiEvent* ev, *(matrixWidget->activeEvents())) {
            if (pointInRect(mouseX, mouseY, ev->x() - 2, ev->y(), ev->x() + ev->width() + 2,
                    ev->y() + ev->height())) {

                if (Selection::instance()->selectedEvents().contains(ev)) {
                    onSelectedEvent = true;
                }

                int diffToMousePos = 0;
                int currentAction = NO_ACTION;

                // left side means SizeChangeTool
                if (pointInRect(mouseX, mouseY, ev->x() - 2, ev->y(), ev->x() + 2,
                        ev->y() + ev->height())) {
                    diffToMousePos = ev->x() - mouseX;
                    currentAction = SIZE_CHANGE_ACTION;
                }

                // right side means SizeChangeTool
                else if (pointInRect(mouseX, mouseY, ev->x() + ev->width() - 2, ev->y(),
                             ev->x() + ev->width() + 2, ev->y() + ev->height())) {
                    diffToMousePos = ev->x() + ev->width() - mouseX;
                    currentAction = SIZE_CHANGE_ACTION;
                }

                // in the event means EventMoveTool, except when CTRL is pressed
                else {
                    int diffRight = ev->x() + ev->width() - mouseX;
                    int diffLeft = diffToMousePos = ev->x() - mouseX;
                    if (diffLeft < 0) {
                        diffLeft *= -1;
                    }
                    if (diffRight < 0) {
                        diffRight *= -1;
                    }
                    if (diffLeft < diffRight) {
                        diffToMousePos = diffLeft;
                    } else {
                        diffToMousePos = diffRight;
                    }
                    if (!QApplication::keyboardModifiers().testFlag(Qt::ControlModifier)) {
                        currentAction = MOVE_ACTION;
                    } else {
                        currentAction = NO_ACTION; // select tool
                    }
                }

                if (diffToMousePos < 0) {
                    diffToMousePos *= -1;
                }

                if (!event || minDiffToMouse > diffToMousePos) {
                    minDiffToMouse = diffToMousePos;
                    event = ev;
                    action = currentAction;
                }
            }
        }

        if (event) {

            switch (action) {

            case NO_ACTION: {
                // no event means SelectTool
                Tool::setCurrentTool(selectTool);
                selectTool->move(mouseX, mouseY);
                selectTool->press(leftClick);
                return true;
            }

            case SIZE_CHANGE_ACTION: {
                if (!onSelectedEvent) {
                    file()->protocol()->startNewAction("Selection changed", image());
                    ProtocolEntry* toCopy = copy();
                    EventTool::selectEvent(event, !Selection::instance()->selectedEvents().contains(event));
                    protocol(toCopy, this);
                    file()->protocol()->endAction();
                }
                Tool::setCurrentTool(sizeChangeTool);
                sizeChangeTool->move(mouseX, mouseY);
                sizeChangeTool->press(leftClick);
                return false;
            }

            case MOVE_ACTION: {
                if (!onSelectedEvent) {
                    file()->protocol()->startNewAction("Selection changed", image());
                    ProtocolEntry* toCopy = copy();
                    EventTool::selectEvent(event, !Selection::instance()->selectedEvents().contains(event));
                    protocol(toCopy, this);
                    file()->protocol()->endAction();
                }
					if(QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier)){
						moveTool->setDirections(true, false);
					} else if(QApplication::keyboardModifiers().testFlag(Qt::AltModifier)){
						moveTool->setDirections(false, true);
					} else {
						moveTool->setDirections(true, true);
					}
                Tool::setCurrentTool(moveTool);
                moveTool->move(mouseX, mouseY);
                moveTool->press(leftClick);
                return false;
            }
            }
        }
    } else {
        // right: new note tool
        Tool::setCurrentTool(newNoteTool);
        newNoteTool->move(mouseX, mouseY);
        newNoteTool->press(leftClick);
        return false;
    }
    Tool::setCurrentTool(selectTool);
    selectTool->move(mouseX, mouseY);
    selectTool->press(leftClick);
    return true;
}

bool StandardTool::move(int mouseX, int mouseY)
{
    EventTool::move(mouseX, mouseY);
    foreach (MidiEvent* ev, *(matrixWidget->activeEvents())) {
        // left/right side means SizeChangeTool
        if (pointInRect(mouseX, mouseY, ev->x() - 2, ev->y(), ev->x() + 2,
                ev->y() + ev->height())
            || pointInRect(mouseX, mouseY, ev->x() + ev->width() - 2, ev->y(),
                   ev->x() + ev->width() + 2, ev->y() + ev->height())) {
            matrixWidget->setCursor(Qt::SplitHCursor);
            return false;
        }
    }
    matrixWidget->setCursor(Qt::ArrowCursor);
    return false;
}

ProtocolEntry* StandardTool::copy()
{
    return new StandardTool(*this);
}

void StandardTool::reloadState(ProtocolEntry* entry)
{
    StandardTool* other = dynamic_cast<StandardTool*>(entry);
    if (!other) {
        return;
    }
    EventTool::reloadState(entry);
    sizeChangeTool = other->sizeChangeTool;
    moveTool = other->moveTool;
    selectTool = other->selectTool;
}

bool StandardTool::release()
{
    matrixWidget->setCursor(Qt::ArrowCursor);
    return true;
}

bool StandardTool::showsSelection()
{
    return true;
}
