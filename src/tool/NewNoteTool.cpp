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

#include "NewNoteTool.h"

#include "../MidiEvent/ChannelPressureEvent.h"
#include "../MidiEvent/ControlChangeEvent.h"
#include "../MidiEvent/KeyPressureEvent.h"
#include "../MidiEvent/KeySignatureEvent.h"
#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/NoteOnEvent.h"
#include "../MidiEvent/PitchBendEvent.h"
#include "../MidiEvent/ProgChangeEvent.h"
#include "../MidiEvent/SysExEvent.h"
#include "../MidiEvent/TempoChangeEvent.h"
#include "../MidiEvent/TextEvent.h"
#include "../MidiEvent/TimeSignatureEvent.h"
#include "../MidiEvent/UnknownEvent.h"
#include "../gui/MatrixWidget.h"
#include "../midi/MidiChannel.h"
#include "../midi/MidiFile.h"
#include "../midi/MidiPlayer.h"
#include "../protocol/Protocol.h"
#include "StandardTool.h"

int NewNoteTool::_channel = 0;
int NewNoteTool::_track = 0;

NewNoteTool::NewNoteTool()
    : EventTool()
{
    inDrag = false;
    line = 0;
    xPos = 0;
    _channel = 0;
    _track = 0;
    setImage(":/run_environment/graphics/tool/newnote.png");
    setToolTipText("Create new Events");
}

NewNoteTool::NewNoteTool(NewNoteTool& other)
    : EventTool(other)
{
    return;
}

ProtocolEntry* NewNoteTool::copy()
{
    return new NewNoteTool(*this);
}

void NewNoteTool::reloadState(ProtocolEntry* entry)
{
    NewNoteTool* other = dynamic_cast<NewNoteTool*>(entry);
    if (!other) {
        return;
    }
    EventTool::reloadState(entry);
}

void NewNoteTool::draw(QPainter* painter)
{
    int currentX = rasteredX(mouseX);
    if (inDrag) {
        if (line <= 127) {
            int y = matrixWidget->yPosOfLine(line);
            painter->fillRect(xPos, y, currentX - xPos, matrixWidget->lineHeight(), Qt::black);
            painter->setPen(Qt::gray);
            painter->drawLine(xPos, 0, xPos, matrixWidget->height());
            painter->drawLine(currentX, 0, currentX, matrixWidget->height());
            painter->setPen(Qt::black);
        } else {
            int y = matrixWidget->yPosOfLine(line);
            painter->fillRect(currentX, y, 15, matrixWidget->lineHeight(), Qt::black);
            painter->setPen(Qt::gray);
            painter->drawLine(currentX, 0, currentX, matrixWidget->height());
            painter->drawLine(currentX + 15, 0, currentX + 15, matrixWidget->height());
            painter->setPen(Qt::black);
        }
    }
}

bool NewNoteTool::press(bool leftClick)
{
    Q_UNUSED(leftClick);
    inDrag = true;
    line = matrixWidget->lineAtY(mouseY);
    xPos = rasteredX(mouseX);
    return true;
}

bool NewNoteTool::release()
{
    int startTick, endTick;
    int currentX = rasteredX(mouseX);
    inDrag = false;
    if (currentX < xPos || line > 127) {
        int temp = currentX;
        currentX = xPos;
        xPos = temp;
    }

    // get start/end tick if magnet
    rasteredX(currentX, &endTick);
    rasteredX(xPos, &startTick);

    MidiTrack* track = file()->track(_track);
    if (currentX - xPos > 2 || line > 127) {

        // note
        if (line >= 0 && line <= 127) {
            currentProtocol()->startNewAction("Create note", image());

            NoteOnEvent* on = file()->channel(_channel)->insertNote(127 - line,
                startTick, endTick, 100, track);
            selectEvent(on, true, true);
            currentProtocol()->endAction();

            if (_standardTool) {
                Tool::setCurrentTool(_standardTool);
                _standardTool->move(mouseX, mouseY);
                _standardTool->release();
            }

            return true;
        } else {

            MidiTrack* generalTrack = file()->track(0);

            MidiEvent* event;
            // prog
            if (line == MidiEvent::PROG_CHANGE_LINE) {

                currentProtocol()->startNewAction("Create Program Change Event",
                    image());

                event = new ProgChangeEvent(_channel, 0, track);
                int startMs = matrixWidget->msOfXPos(xPos);
                int startTick = file()->tick(startMs);
                file()->channel(_channel)->insertEvent(event, startTick);
                selectEvent(event, true, true);
                currentProtocol()->endAction();

            } else if (line == MidiEvent::TIME_SIGNATURE_EVENT_LINE) {
                currentProtocol()->startNewAction("Create Time Signature Event",
                    image());

                // 4/4
                event = new TimeSignatureEvent(18, 4, 2, 24, 8, generalTrack);
                int startMs = matrixWidget->msOfXPos(xPos);
                int startTick = file()->tick(startMs);
                file()->channel(18)->insertEvent(event, startTick);
                selectEvent(event, true, true);
                currentProtocol()->endAction();
            } else if (line == MidiEvent::TEMPO_CHANGE_EVENT_LINE) {
                currentProtocol()->startNewAction("Create Tempo Change Event",
                    image());
                // quarter = 120
                event = new TempoChangeEvent(17, 500000, generalTrack);
                int startMs = matrixWidget->msOfXPos(xPos);
                int startTick = file()->tick(startMs);
                file()->channel(17)->insertEvent(event, startTick);
                selectEvent(event, true, true);
                currentProtocol()->endAction();
            } else if (line == MidiEvent::KEY_SIGNATURE_EVENT_LINE) {
                currentProtocol()->startNewAction("Create Key Signature Event",
                    image());
                event = new KeySignatureEvent(16, 0, false, generalTrack);

                int startMs = matrixWidget->msOfXPos(xPos);
                int startTick = file()->tick(startMs);
                file()->channel(16)->insertEvent(event, startTick);
                selectEvent(event, true, true);
                currentProtocol()->endAction();
            } else if (line == MidiEvent::CONTROLLER_LINE) {
                currentProtocol()->startNewAction("Create Control Change Event",
                    image());
                event = new ControlChangeEvent(_channel, 0, 0, track);
                int startMs = matrixWidget->msOfXPos(xPos);
                int startTick = file()->tick(startMs);
                file()->channel(_channel)->insertEvent(event, startTick);
                selectEvent(event, true, true);
                currentProtocol()->endAction();
            } else if (line == MidiEvent::KEY_PRESSURE_LINE) {
                currentProtocol()->startNewAction("Create Key Pressure Event",
                    image());
                event = new KeyPressureEvent(_channel, 127, 100, track);
                int startMs = matrixWidget->msOfXPos(xPos);
                int startTick = file()->tick(startMs);
                file()->channel(_channel)->insertEvent(event, startTick);
                selectEvent(event, true, true);
                currentProtocol()->endAction();
            } else if (line == MidiEvent::CHANNEL_PRESSURE_LINE) {
                currentProtocol()->startNewAction(
                    "Create Channel Pressure Event", image());
                event = new ChannelPressureEvent(_channel, 100, track);
                int startMs = matrixWidget->msOfXPos(xPos);
                int startTick = file()->tick(startMs);
                file()->channel(_channel)->insertEvent(event, startTick);
                selectEvent(event, true, true);
                currentProtocol()->endAction();
            } else if (line == MidiEvent::PITCH_BEND_LINE) {
                currentProtocol()->startNewAction(
                    "Create Pitch Bend Event", image());
                event = new PitchBendEvent(_channel, 8192, track);
                int startMs = matrixWidget->msOfXPos(xPos);
                int startTick = file()->tick(startMs);
                file()->channel(_channel)->insertEvent(event, startTick);
                selectEvent(event, true, true);
                currentProtocol()->endAction();
            } else if (line == MidiEvent::TEXT_EVENT_LINE) {
                currentProtocol()->startNewAction(
                    "Create Text Event", image());
                event = new TextEvent(16, track);
                TextEvent* textEvent = (TextEvent*)event;
                textEvent->setText("New Text Event");
                textEvent->setType(TextEvent::getTypeForNewEvents());
                int startMs = matrixWidget->msOfXPos(xPos);
                int startTick = file()->tick(startMs);
                file()->channel(16)->insertEvent(event, startTick);
                selectEvent(event, true, true);
                currentProtocol()->endAction();
            } else if (line == MidiEvent::UNKNOWN_LINE) {
                currentProtocol()->startNewAction(
                    "Create Unknown Event", image());
                event = new UnknownEvent(16, 0x52, QByteArray(), track);
                int startMs = matrixWidget->msOfXPos(xPos);
                int startTick = file()->tick(startMs);
                file()->channel(16)->insertEvent(event, startTick);
                selectEvent(event, true, true);
                currentProtocol()->endAction();
            } else if (line == MidiEvent::SYSEX_LINE) {
                currentProtocol()->startNewAction(
                    "Create SysEx Event", image());
                event = new SysExEvent(16, QByteArray(), track);
                int startMs = matrixWidget->msOfXPos(xPos);
                int startTick = file()->tick(startMs);
                file()->channel(16)->insertEvent(event, startTick);
                selectEvent(event, true, true);
                currentProtocol()->endAction();
            } else {
                if (_standardTool) {
                    Tool::setCurrentTool(_standardTool);
                    _standardTool->move(mouseX, mouseY);
                    _standardTool->release();
                }
                return true;
            }
        }
    }
    if (_standardTool) {
        Tool::setCurrentTool(_standardTool);
        _standardTool->move(mouseX, mouseY);
        _standardTool->release();
    }
    return true;
}

bool NewNoteTool::move(int mouseX, int mouseY)
{
    EventTool::move(mouseX, mouseY);
    return inDrag;
}

bool NewNoteTool::releaseOnly()
{
    inDrag = false;
    xPos = 0;
    return true;
}

int NewNoteTool::editTrack()
{
    return _track;
}

int NewNoteTool::editChannel()
{
    return _channel;
}

void NewNoteTool::setEditTrack(int i)
{
    _track = i;
}

void NewNoteTool::setEditChannel(int i)
{
    _channel = i;
}
