/*
 * MidiEditor
 * Copyright (C) 2010  Markus Schwenk
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.+
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtMath>
#include "MainWindow.h"
#include "MatrixWidget.h"
#include "SelectionNavigator.h"
#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/NoteOnEvent.h"
#include "../MidiEvent/OffEvent.h"
#include "../midi/MidiChannel.h"
#include "../midi/MidiFile.h"
#include "../midi/MidiTrack.h"
#include "../protocol/Protocol.h"
#include "../tool/EventTool.h"
#include "../tool/Selection.h"

/*
 * Moves the selection to the nearest event of the same type in the specified
 * direction on screen.
 */
SelectionNavigator::SelectionNavigator(MainWindow* mainWindow)
{
    this->mainWindow = mainWindow;
}

void SelectionNavigator::up()
{
    navigate(-M_PI_2);
}

void SelectionNavigator::down()
{
    navigate(M_PI_2);
}

void SelectionNavigator::left()
{
    navigate(M_PI);
}

void SelectionNavigator::right()
{
    navigate(0.0);
}

void SelectionNavigator::navigate(qreal searchAngle)
{
    MidiEvent* selectedEvent = getFirstSelectedEvent();
    if (!selectedEvent) return;
    MidiFile* file = selectedEvent->file();
    MidiEvent* newSelectedEvent = NULL;
    qreal newSelectedEventDistance = -1.0;

    for (int channelNumber = 0; channelNumber < 19; channelNumber++) {
        MidiChannel* channel = file->channel(channelNumber);
        if (!channel->visible()) continue;

        foreach (MidiEvent* channelEvent, channel->eventMap()->values()) {
            if (channelEvent == selectedEvent) continue;
            if (channelEvent->track()->hidden()) continue;
            if (!eventIsInVisibleTimeRange(channelEvent)) continue;
            if (!eventsAreSameType(selectedEvent, channelEvent)) continue;
            qreal channelEventDistance = getDisplayDistanceWeightedByDirection(selectedEvent, channelEvent, searchAngle);
            if (channelEventDistance < 0.0) continue;

            if (!newSelectedEvent || (channelEventDistance < newSelectedEventDistance)) {
                newSelectedEvent = channelEvent;
                newSelectedEventDistance = channelEventDistance;
            }
        }
    }

    if (!newSelectedEvent) return;
    Protocol* protocol = file->protocol();
    protocol->startNewAction("Tweak selection");
    EventTool::selectEvent(newSelectedEvent, true);
    protocol->endAction();
    mainWindow->updateAll();
}

MidiEvent* SelectionNavigator::getFirstSelectedEvent()
{
    QList<MidiEvent*> selectedEvents = Selection::instance()->selectedEvents();
    return selectedEvents.isEmpty() ? NULL : selectedEvents.first();
}

bool SelectionNavigator::eventIsInVisibleTimeRange(MidiEvent* event)
{
    MatrixWidget* matrixWidget = mainWindow->matrixWidget();
    int time = event->midiTime();
    return (time >= matrixWidget->minVisibleMidiTime() && time < matrixWidget->maxVisibleMidiTime());
}

bool SelectionNavigator::eventsAreSameType(MidiEvent* event1, MidiEvent* event2)
{
    NoteOnEvent* noteOnEvent1 = dynamic_cast<NoteOnEvent*>(event1);
    NoteOnEvent* noteOnEvent2 = dynamic_cast<NoteOnEvent*>(event2);
    if (noteOnEvent1 || noteOnEvent2) return (noteOnEvent1 && noteOnEvent2);
    OffEvent* offEvent1 = dynamic_cast<OffEvent*>(event1);
    OffEvent* offEvent2 = dynamic_cast<OffEvent*>(event2);
    if (offEvent1 || offEvent2) return (offEvent1 && offEvent2);
    return (event1->line() == event2->line());
}

/*
 * Compute the Euclidean distance between two events as measured in pixels for
 * their display in the MatrixWidget, but weight it by how far off axis the
 * target is from the search angle.  There must be a standard term for this
 * when it's used to compute damage in shooter games.  Returns -1 when the
 * target is more than 90 degrees off axis from the search direction, since
 * that should never be considered a match.
 */
qreal SelectionNavigator::getDisplayDistanceWeightedByDirection(
    MidiEvent* originEvent,
    MidiEvent* targetEvent,
    qreal searchAngle)
{
    MatrixWidget* matrixWidget = mainWindow->matrixWidget();
    int originX = matrixWidget->xPosOfMs(matrixWidget->msOfTick(originEvent->midiTime()));
    int originY = matrixWidget->yPosOfLine(originEvent->line());
    int targetX = matrixWidget->xPosOfMs(matrixWidget->msOfTick(targetEvent->midiTime()));
    int targetY = matrixWidget->yPosOfLine(targetEvent->line());
    qreal distanceX = targetX - originX;
    qreal distanceY = targetY - originY;
    qreal distance = qSqrt((distanceX * distanceX) + (distanceY * distanceY));
    qreal angle = qAtan2(distanceY, distanceX);

    qreal angleDifferenceNaive = qFabs(angle - searchAngle);
    qreal angleDifference = fmin(angleDifferenceNaive, (2 * M_PI) - angleDifferenceNaive);
    if (angleDifference >= M_PI_2) return -1.0;
    qreal offAxisDistance = qSin(angleDifference) * distance;

    return distance + offAxisDistance;
}
