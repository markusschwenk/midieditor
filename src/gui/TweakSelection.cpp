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
#include "TweakSelection.h"
#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/OffEvent.h"
#include "../midi/MidiChannel.h"
#include "../midi/MidiFile.h"
#include "../midi/MidiTrack.h"
#include "../tool/EventTool.h"
#include "../tool/Selection.h"

/*
 * Moves the selection to the nearest appropriate event in the specified
 * direction on screen.  You can only move between events of the same type
 * (note to note, pitch bend to pitch bend, etc) with this mechanism.  Since
 * event types other than notes are displayed in a single line each, only left
 * and right movements are meaningful for them.
 */

TweakSelection::TweakSelection(MainWindow* mainWindow)
{
    this->mainWindow = mainWindow;
}

void TweakSelection::up()
{
    MidiEvent* selectedEvent = getFirstSelectedEvent();
    if (!selectedEvent) return;
    if (!selectedEvent->isOnEvent()) return;
    MidiFile* file = selectedEvent->file();
    MidiEvent* newSelectedEvent = NULL;

    for (int channelNumber = 0; channelNumber < 19; channelNumber++) {
        MidiChannel* channel = file->channel(channelNumber);
        if (!channel->visible()) continue;

        foreach (MidiEvent* channelEvent, channel->eventMap()->values()) {
            if (channelEvent->track()->hidden()) continue;
            if (!channelEvent->isOnEvent()) continue;
            if (!(channelEvent->line() < selectedEvent->line())) continue;

            if (!newSelectedEvent || getDisplayDistanceBetweenEvents(selectedEvent, channelEvent) <
                    getDisplayDistanceBetweenEvents(selectedEvent, newSelectedEvent)) {
                newSelectedEvent = channelEvent;
            }
        }
    }

    selectEvent(newSelectedEvent);
}

void TweakSelection::down()
{
    MidiEvent* selectedEvent = getFirstSelectedEvent();
    if (!selectedEvent) return;
    if (!selectedEvent->isOnEvent()) return;
    MidiFile* file = selectedEvent->file();
    MidiEvent* newSelectedEvent = NULL;

    for (int channelNumber = 0; channelNumber < 19; channelNumber++) {
        MidiChannel* channel = file->channel(channelNumber);
        if (!channel->visible()) continue;

        foreach (MidiEvent* channelEvent, channel->eventMap()->values()) {
            if (channelEvent->track()->hidden()) continue;
            if (!channelEvent->isOnEvent()) continue;
            if (!(channelEvent->line() > selectedEvent->line())) continue;

            if (!newSelectedEvent || getDisplayDistanceBetweenEvents(selectedEvent, channelEvent) <
                    getDisplayDistanceBetweenEvents(selectedEvent, newSelectedEvent)) {
                newSelectedEvent = channelEvent;
            }
        }
    }

    selectEvent(newSelectedEvent);
}

void TweakSelection::left()
{
    MidiEvent* selectedEvent = getFirstSelectedEvent();
    if (!selectedEvent) return;
    bool selectedIsOnEvent = selectedEvent->isOnEvent();
    MidiFile* file = selectedEvent->file();
    MidiEvent* newSelectedEvent = NULL;

    for (int channelNumber = 0; channelNumber < 19; channelNumber++) {
        MidiChannel* channel = file->channel(channelNumber);
        if (!channel->visible()) continue;

        foreach (MidiEvent* channelEvent, channel->eventMap()->values()) {
            if (channelEvent->track()->hidden()) continue;
            if (!(channelEvent->midiTime() < selectedEvent->midiTime())) continue;
            if (dynamic_cast<OffEvent*>(channelEvent)) continue;

            if (selectedIsOnEvent) {
                if (!channelEvent->isOnEvent()) continue;

                if (!newSelectedEvent || getDisplayDistanceBetweenEvents(selectedEvent, channelEvent) <
                        getDisplayDistanceBetweenEvents(selectedEvent, newSelectedEvent)) {
                    newSelectedEvent = channelEvent;
                }
            } else {
                if (channelEvent->line() != selectedEvent->line()) continue;

                if (!newSelectedEvent || channelEvent->midiTime() > newSelectedEvent->midiTime()) {
                    newSelectedEvent = channelEvent;
                }
            }
        }
    }

    selectEvent(newSelectedEvent);
}

void TweakSelection::right()
{
    MidiEvent* selectedEvent = getFirstSelectedEvent();
    if (!selectedEvent) return;
    bool selectedIsOnEvent = selectedEvent->isOnEvent();
    MidiFile* file = selectedEvent->file();
    MidiEvent* newSelectedEvent = NULL;

    for (int channelNumber = 0; channelNumber < 19; channelNumber++) {
        MidiChannel* channel = file->channel(channelNumber);
        if (!channel->visible()) continue;

        foreach (MidiEvent* channelEvent, channel->eventMap()->values()) {
            if (channelEvent->track()->hidden()) continue;
            if (!(channelEvent->midiTime() > selectedEvent->midiTime())) continue;
            if (dynamic_cast<OffEvent*>(channelEvent)) continue;

            if (selectedIsOnEvent) {
                if (!channelEvent->isOnEvent()) continue;

                if (!newSelectedEvent || getDisplayDistanceBetweenEvents(selectedEvent, channelEvent) <
                        getDisplayDistanceBetweenEvents(selectedEvent, newSelectedEvent)) {
                    newSelectedEvent = channelEvent;
                }
            } else {
                if (channelEvent->line() != selectedEvent->line()) continue;

                if (!newSelectedEvent || channelEvent->midiTime() < newSelectedEvent->midiTime()) {
                    newSelectedEvent = channelEvent;
                }
            }
        }
    }

    selectEvent(newSelectedEvent);
}

MidiEvent* TweakSelection::getFirstSelectedEvent()
{
    QList<MidiEvent*> selectedEvents = Selection::instance()->selectedEvents();
    return selectedEvents.isEmpty() ? NULL : selectedEvents.first();
}

qreal TweakSelection::getDisplayDistanceBetweenEvents(MidiEvent* event1, MidiEvent* event2)
{
    MatrixWidget* matrixWidget = mainWindow->matrixWidget();
    int x1 = matrixWidget->xPosOfMs(matrixWidget->msOfTick(event1->midiTime()));
    int x2 = matrixWidget->xPosOfMs(matrixWidget->msOfTick(event2->midiTime()));
    int y1 = matrixWidget->yPosOfLine(event1->line());
    int y2 = matrixWidget->yPosOfLine(event2->line());
    int deltaX = x2 - x1;
    int deltaY = y2 - y1;
    return qSqrt(((qreal)(deltaX) * (qreal)(deltaX)) + ((qreal)(deltaY) * (qreal)(deltaY)));
}

void TweakSelection::selectEvent(MidiEvent* event) {
    if (!event) return;
    EventTool::selectEvent(event, true);
    mainWindow->updateAll();
}
