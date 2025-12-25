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
#include "../MidiEvent/OnEvent.h"
#include "../MidiEvent/OffEvent.h"
#include "../gui/MatrixWidget.h"
#include "../midi/MidiChannel.h"
#include "../midi/MidiFile.h"
#include "../midi/MidiTrack.h"
#include "../tool/NewNoteTool.h"
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

    int ch = NewNoteTool::editChannel();

    QList<MidiEvent*> active;

    for(int n = 0; n < 19; n++) {
        if(n == ch) continue;
        foreach (MidiEvent* ev, *(matrixWidget->activeEvents())) {
            if(ev->channel() == n) active.append(ev);
        }
    }

    foreach (MidiEvent* ev, *(matrixWidget->activeEvents())) {
        if(ev->channel() == ch) active.append(ev);
    }

    foreach (MidiEvent* ev, active/**(matrixWidget->activeEvents())*/) {

        if(!(!ev->file()->MultitrackMode ||
             (ev->file()->MultitrackMode && ev->track()->number() == NewNoteTool::editTrack())))
            continue;

        if (pointInRect(mouseX, mouseY, ev->x(), ev->y(), ev->x() + ev->width(),
                        ev->y() + ev->height())) {

            if(!Tool::selectCurrentChanOnly) {
                painter->setPen(Qt::black);
                painter->setBrush(Qt::black);
                painter->drawRoundedRect(ev->x(), ev->y(), ev->width(), ev->height(), 2, 2);
            } else {

                QColor cC = *file()->channel(ev->channel())->color();

                cC.setAlpha(255);

                OffEvent* offEvent = dynamic_cast<OffEvent*>(ev);
                OnEvent* onEvent = dynamic_cast<OnEvent*>(ev);

                int displaced = 0;

                if (onEvent || offEvent) {

                    int channel = ev->channel();

                    // visual octave correction for notes

                    if(channel >= 0 && channel < 16 && OctaveChan_MIDI[channel]) {
                        if((channel == 9 && ev->track()->fluid_index() == 0) ||
                                ((channel == 9 && ev->track()->fluid_index() != 0
                            && MidiOutput::isFluidsynthConnected(ev->track()->device_index()) && ev->file()->DrumUseCh9))) {
                             displaced = 0;
                        } else {
                             displaced = 1;
                        }
                    }

                }

                if(displaced)
                    ev->draw2(painter, cC, Qt::Dense1Pattern, 1);
                else
                    ev->draw(painter, cC, 1);
            }


        } else if(ev->channel() == ch && Tool::selectCurrentChanOnly) {

            QColor cC = *file()->channel(ev->channel())->color();

            cC.setAlpha(255);

            OffEvent* offEvent = dynamic_cast<OffEvent*>(ev);
            OnEvent* onEvent = dynamic_cast<OnEvent*>(ev);

            int displaced = 0;

            if (onEvent || offEvent) {

                int channel = ev->channel();

                // visual octave correction for notes

                if(channel >= 0 && channel < 16 && OctaveChan_MIDI[channel]) {
                    if((channel == 9 && ev->track()->fluid_index() == 0) ||
                            ((channel == 9 && ev->track()->fluid_index() != 0
                        && MidiOutput::isFluidsynthConnected(ev->track()->device_index()) && ev->file()->DrumUseCh9))) {
                         displaced = 0;
                    } else {
                         displaced = 1;
                    }
                }
            }

            if(displaced)
                ev->draw2(painter, cC, Qt::Dense1Pattern, 1);
            else
                ev->draw(painter, cC, 1);
        }

    }

    painter->setPen(Qt::black);
    painter->setBrush(Qt::black);

    if(Tool::selectCurrentChanOnly) {
        foreach (MidiEvent* ev, active) {

            if(ev->channel() != ch)
                continue;

            if(!(!ev->file()->MultitrackMode ||
                              (ev->file()->MultitrackMode && ev->track()->number() == NewNoteTool::editTrack())))
                    continue;


            if (pointInRect(mouseX, mouseY, ev->x(), ev->y(), ev->x() + ev->width(),
                            ev->y() + ev->height())) {

                painter->drawRoundedRect(ev->x(), ev->y(), ev->width(), ev->height(), 2, 2);

            }

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

    int selected = 0;

    int ch = NewNoteTool::editChannel();

    foreach (MidiEvent* ev, *(matrixWidget->activeEvents())) {

        if(ev->channel() != ch && Tool::selectCurrentChanOnly)
            continue;

        if(!(!ev->file()->MultitrackMode ||
                          (ev->file()->MultitrackMode && ev->track()->number() == NewNoteTool::editTrack())))
                continue;

        if (pointInRect(mouseX, mouseY, ev->x(), ev->y(), ev->x() + ev->width(),
                ev->y() + ev->height())) {

            file()->channel(ev->channel())->removeEvent(ev);
            selected++;

            if (Selection::instance()->selectedEvents().contains(ev)) {
                deselectEvent(ev);
            }
        }
    }


    currentProtocol()->changeDescription("Remove event (" + QString::number(selected) + ")");
    currentProtocol()->endAction();
    return true;
}
