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

#include "EventTool.h"

#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/NoteOnEvent.h"
#include "../MidiEvent/OffEvent.h"
#include "../MidiEvent/OnEvent.h"
#include "../gui/EventWidget.h"
#include "../gui/MainWindow.h"
#include "../gui/MatrixWidget.h"
#include "../midi/MidiChannel.h"
#include "../midi/MidiFile.h"
#include "../midi/MidiPlayer.h"
#include "../midi/MidiTrack.h"
#include "../protocol/Protocol.h"
#include "NewNoteTool.h"
#include "Selection.h"

#include <QtCore/qmath.h>
#include <QMessageBox>


//QList<MidiEvent*>* EventTool::copiedEvents = new QList<MidiEvent*>;

int EventTool::_pasteChannel = -1;
int EventTool::_pasteTrack = -2;

bool EventTool::_magnet = false;
QSharedMemory EventTool::sharedMemory = QSharedMemory("midieditor_copy_paste_memory");

EventTool::EventTool()
    : EditorTool()
{
}

EventTool::EventTool(EventTool& other)
    : EditorTool(other)
{
}

void EventTool::selectEvent(MidiEvent* event, bool single, bool ignoreStr)
{

    if (!event->file()->channel(event->channel())->visible()) {
        return;
    }

    if (event->track()->hidden()) {
        return;
    }

    QList<MidiEvent*> selected = Selection::instance()->selectedEvents();

    OffEvent* offevent = dynamic_cast<OffEvent*>(event);
    if (offevent) {
        return;
    }

    if (single && !QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier) && (!QApplication::keyboardModifiers().testFlag(Qt::ControlModifier) || ignoreStr)) {
        selected.clear();
        NoteOnEvent* on = dynamic_cast<NoteOnEvent*>(event);
        if (on) {
            MidiPlayer::play(on);
        }
    }
    if (!selected.contains(event) && (!QApplication::keyboardModifiers().testFlag(Qt::ControlModifier) || ignoreStr)) {
        selected.append(event);
    } else if (QApplication::keyboardModifiers().testFlag(Qt::ControlModifier) && !ignoreStr) {
        selected.removeAll(event);
    }

    Selection::instance()->setSelection(selected);
    _mainWindow->eventWidget()->reportSelectionChangedByTool();
}

void EventTool::deselectEvent(MidiEvent* event)
{

    QList<MidiEvent*> selected = Selection::instance()->selectedEvents();
    selected.removeAll(event);
    Selection::instance()->setSelection(selected);

    if (_mainWindow->eventWidget()->events().contains(event)) {
        _mainWindow->eventWidget()->removeEvent(event);
    }
}

void EventTool::clearSelection()
{
    Selection::instance()->clearSelection();
    _mainWindow->eventWidget()->reportSelectionChangedByTool();
}

void EventTool::paintSelectedEvents(QPainter* painter)
{
    foreach (MidiEvent* event, Selection::instance()->selectedEvents()) {

        bool show = event->shown();

        if (!show) {
            OnEvent* ev = dynamic_cast<OnEvent*>(event);
            if (ev) {
                show = ev->offEvent() && ev->offEvent()->shown();
            }
        }

        if (event->track()->hidden()) {
            show = false;
        }
        if (!(event->file()->channel(event->channel())->visible())) {
            show = false;
        }

        if (show) {
            painter->setBrush(Qt::darkBlue);
            painter->setPen(Qt::lightGray);
            painter->drawRoundedRect(event->x(), event->y(), event->width(),
                event->height(), 1, 1);
        }
    }
}

void EventTool::changeTick(MidiEvent* event, int shiftX)
{
    // TODO: falls event gezeigt ist, über matrixWidget tick erfragen (effizienter)
    //int newMs = matrixWidget->msOfXPos(event->x()-shiftX);

    int newMs = file()->msOfTick(event->midiTime()) - matrixWidget->timeMsOfWidth(shiftX);
    int tick = file()->tick(newMs);

    if (tick < 0) {
        tick = 0;
    }

    // with magnet: set to div value if pixel refers to this tick
    if (magnetEnabled()) {

        int newX = matrixWidget->xPosOfMs(newMs);
        typedef QPair<int, int> TMPPair;
        foreach (TMPPair p, matrixWidget->divs()) {
            int xt = p.first;
            if (newX == xt) {
                tick = p.second;
                break;
            }
        }
    }
    event->setMidiTime(tick);
}

void EventTool::copyAction()
{
    if (Selection::instance()->selectedEvents().size() > 0) {
        // clear old copied Events
        //copiedEvents->clear();
        MidiFile copyFile = MidiFile();
        copyFile.setTicksPerQuarter(Selection::instance()->selectedEvents().first()->file()->ticksPerQuarter());
        MidiTrack* copyTrack = copyFile.track(1);

        foreach (MidiEvent* event, Selection::instance()->selectedEvents()) {

            // add the current Event
            MidiEvent* ev = dynamic_cast<MidiEvent*>(event->copy());
            if (ev) {
                // do not append off event here
                OffEvent* off = dynamic_cast<OffEvent*>(ev);
                if (!off) {
                    //copiedEvents->append(ev);
                    ev->setTrack(copyTrack, false);
                    ev->setChannel(0, false);
                    ev->setFile(&copyFile);
                    copyFile.channelEvents(0)->insert(ev->midiTime(), ev);
                }
            }

            // if its onEvent, add a copy of the OffEvent
            OnEvent* onEv = dynamic_cast<OnEvent*>(ev);
            if (onEv) {
                OffEvent* offEv = dynamic_cast<OffEvent*>(onEv->offEvent()->copy());
                if (offEv) {
                    offEv->setOnEvent(onEv);
                    //copiedEvents->append(offEv);
                    offEv->setTrack(copyTrack, false);
                    offEv->setChannel(0, false);
                    offEv->setFile(&copyFile);
                    copyFile.channelEvents(0)->insert(offEv->midiTime(), offEv);
                }
            }
        }
        
        QByteArray copyFileBytes = copyFile.toByteArray();
        int MAX_COPY_SIZE = 1073741824; //1GB
        if (copyFileBytes.size() > MAX_COPY_SIZE){
            QMessageBox msgBox;
            msgBox.setText("Copied data too large...");
            msgBox.exec();
            return;
        }

        if (!sharedMemory.isAttached()){
            if (!sharedMemory.create(MAX_COPY_SIZE)){
                QMessageBox msgBox;
                msgBox.setText("Failed to copy items...");
                msgBox.exec();
                return;
            }
        }

        sharedMemory.lock();
        char* to = (char*) sharedMemory.data();
        char* from = copyFileBytes.data();
        memcpy(to, from, qMin(sharedMemory.size(), copyFileBytes.size()));
        sharedMemory.unlock();

        //_mainWindow->copiedEventsChanged();
    }
}

void EventTool::pasteAction()
{
    if (!sharedMemory.isAttached() && !sharedMemory.attach()){
        return; //No data copied
    }
    sharedMemory.lock();
    QByteArray pasted_file_raw = QByteArray();
    pasted_file_raw.setRawData((char*)sharedMemory.constData(), sharedMemory.size());
    
    if(pasted_file_raw.size() == 0) {
        sharedMemory.unlock();
        return;
    };
    bool ok = true;
    MidiFile pasted_file(pasted_file_raw, &ok, nullptr);

    sharedMemory.unlock();

    if(!ok){
        QMessageBox msgBox;
        msgBox.setText("The pasted input cannot be interpreted as midi data...");
        msgBox.exec();
    }

    if (pasted_file.channelEvents(0)->size() == 0) {
        return;
    }

    // TODO what happends to TempoEvents??

    // copy copied events to insert unique events
    QList<MidiEvent*> copiedCopiedEvents;
    foreach (MidiEvent* event, *pasted_file.channelEvents(0)) {

        // add the current Event
        MidiEvent* ev = dynamic_cast<MidiEvent*>(event->copy());
        if (ev) {
            // do not append off event here
            OffEvent* off = dynamic_cast<OffEvent*>(ev);
            if (!off) {
                copiedCopiedEvents.append(ev);
            }
        }

        // if its onEvent, add a copy of the OffEvent
        OnEvent* onEv = dynamic_cast<OnEvent*>(ev);
        if (onEv) {
            OffEvent* offEv = dynamic_cast<OffEvent*>(onEv->offEvent()->copy());
            if (offEv) {
                offEv->setOnEvent(onEv);
                copiedCopiedEvents.append(offEv);
            }
        }
    }

    if (copiedCopiedEvents.count() > 0) {

        MidiFile* copiedEventsFile = pasted_file.channelEvents(0)->first()->file();

        // Begin a new ProtocolAction
        currentFile()->protocol()->startNewAction("Paste " + QString::number(copiedCopiedEvents.count()) + " events");

        double tickscale = 1;
        if (currentFile() != copiedEventsFile) {
            tickscale = ((double)(currentFile()->ticksPerQuarter())) / ((double)copiedEventsFile->ticksPerQuarter());
        }

        // get first Tick of the copied events
        int firstTick = -1;
        foreach (MidiEvent* event, copiedCopiedEvents) {
            if ((int)(tickscale * event->midiTime()) < firstTick || firstTick < 0) {
                firstTick = (int)(tickscale * event->midiTime());
            }
        }

        if (firstTick < 0)
            firstTick = 0;

        // calculate the difference of old/new events in MidiTicks
        int diff = currentFile()->cursorTick() - firstTick;

        // set the Positions and add the Events to the channels
        clearSelection();

        foreach (MidiEvent* event, copiedCopiedEvents) {

            // get channel
            int channel = event->channel();
            if (_pasteChannel == -2) {
                channel = NewNoteTool::editChannel();
            }
            if ((_pasteChannel >= 0) && (channel < 16)) {
                channel = _pasteChannel;
            }

            // get track
            MidiTrack* track = event->track();
            if (pasteTrack() == -2) {
                track = currentFile()->track(NewNoteTool::editTrack());
            } else if ((pasteTrack() >= 0) && (pasteTrack() < currentFile()->tracks()->size())) {
                track = currentFile()->track(pasteTrack());
            } else if (event->file() != currentFile() || !currentFile()->tracks()->contains(track)) {
                track = currentFile()->getPasteTrack(event->track(), event->file());
                if (!track) {
                    track = event->track()->copyToFile(currentFile());
                }
            }

            if ((!track) || (track->file() != currentFile())) {
                track = currentFile()->track(0);
            }

            event->setFile(currentFile());
            event->setChannel(channel, false);
            event->setTrack(track, false);
            currentFile()->channel(channel)->insertEvent(event,
                (int)(tickscale * event->midiTime()) + diff);
            selectEvent(event, false, true);
        }

        currentFile()->protocol()->endAction();
    }
}

bool EventTool::showsSelection()
{
    return false;
}

void EventTool::setPasteTrack(int track)
{
    _pasteTrack = track;
}

int EventTool::pasteTrack()
{
    return _pasteTrack;
}

void EventTool::setPasteChannel(int channel)
{
    _pasteChannel = channel;
}

int EventTool::pasteChannel()
{
    return _pasteChannel;
}

int EventTool::rasteredX(int x, int* tick)
{
    if (!_magnet) {
        if (tick) {
            *tick = _currentFile->tick(matrixWidget->msOfXPos(x));
        }
        return x;
    }
    typedef QPair<int, int> TMPPair;
    foreach (TMPPair p, matrixWidget->divs()) {
        int xt = p.first;
        if (qAbs(xt - x) <= 5) {
            if (tick) {
                *tick = p.second;
            }
            return xt;
        }
    }
    if (tick) {
        *tick = _currentFile->tick(matrixWidget->msOfXPos(x));
    }
    return x;
}

void EventTool::enableMagnet(bool enable)
{
    _magnet = enable;
}

bool EventTool::magnetEnabled()
{
    return _magnet;
}
