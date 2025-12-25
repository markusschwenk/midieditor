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
#include "SelectTool.h"

#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/NoteOnEvent.h"

#include "../MidiEvent/TempoChangeEvent.h"
#include "../MidiEvent/TimeSignatureEvent.h"
#include "../MidiEvent/KeySignatureEvent.h"
#include "../MidiEvent/ProgChangeEvent.h"
#include "../MidiEvent/ControlChangeEvent.h"
#include "../MidiEvent/KeyPressureEvent.h"
#include "../MidiEvent/ChannelPressureEvent.h"
#include "../MidiEvent/TextEvent.h"
#include "../MidiEvent/PitchBendEvent.h"
#include "../MidiEvent/SysExEvent.h"
#include "../MidiEvent/UnknownEvent.h"

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
#include <QDir>

#define build_eventp(a,b,c) a* b = dynamic_cast<a*>(c)

QList<MidiEvent*>* EventTool::copiedEvents = new QList<MidiEvent*>;

int EventTool::_pasteChannel = -1;
int EventTool::_pasteTrack = -1;

bool EventTool::_magnet = false;

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
// Estwald Visible
#ifdef USE_FLUIDSYNTH
        if(event->channel() != 16) return;
        SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

        if(sys) {
            QByteArray c = sys->data();

            if(c[1]== (char) 0x66 && c[2]==(char) 0x66 && c[3]=='W') {
                if(!event->file()->channel(c[0] & 15)->visible((c[0]/32) % 3)) {
                    return;
                }
            } else if(c[1]== (char) 0x66 && c[2]==(char) 0x66 && (c[3] & 0x70) == 0x70) {
                if(!event->file()->channel(c[3] & 15)->visible()) {
                    return;
                }
            }
        }

        return;

#else
        return;
#endif
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
            int ms = 2000;
            if(on->offEvent()) {
                ms = on->file()->msOfTick(on->offEvent()->midiTime() - on->midiTime());
            }
            if(ms > 2000) ms = 2000;

            MidiPlayer::play(on, ms);
        }
    }
    if (!selected.contains(event) && (!QApplication::keyboardModifiers().testFlag(Qt::ControlModifier) || ignoreStr)) {
        selected.append(event);
        build_eventp(TempoChangeEvent, ev1, event);
        build_eventp(TimeSignatureEvent, ev2, event);
        build_eventp(KeySignatureEvent, ev3, event);
        build_eventp(ProgChangeEvent, ev4, event);
        build_eventp(ControlChangeEvent, ev5, event);
        build_eventp(KeyPressureEvent, ev6, event);
        build_eventp(ChannelPressureEvent, ev7, event);
        build_eventp(TextEvent, ev8, event);
        build_eventp(PitchBendEvent, ev9, event);
        build_eventp(SysExEvent, ev10, event);

        if(ev1 || ev2 || ev3 || ev4 || ev5
            || ev6  || ev7 || ev8 || ev9 || ev10) {

            _mainWindow->upperTabWidget->setCurrentIndex(_mainWindow->EventSplitterTabPos);

        }
    } else if (QApplication::keyboardModifiers().testFlag(Qt::ControlModifier) && !ignoreStr) {
        selected.removeAll(event);
    }
    if (!selected.contains(event)){

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
// Estwald Visible
#ifdef USE_FLUIDSYNTH
            if(event->channel() != 16)
                show = false;
            else {
                SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

                if(sys) {
                    QByteArray c = sys->data();

                    if(c[1]== (char) 0x66 && c[2]==(char) 0x66 && c[3]=='W') {
                        if(!event->file()->channel(c[0] & 15)->visible((c[0]/32) % 3)) {
                            show = false;
                        }
                    } else if(c[1]== (char) 0x66 && c[2]==(char) 0x66 && (c[3] & 0x70) == 0x70) {
                        if(!event->file()->channel(c[3] & 15)->visible()) {
                            show = false;
                        }
                    }
                }
            }
#else
            show = false;
#endif
        }

        if (show) {
            int channel = event->channel();
            bool trans = false;
            if(!(!event->file()->MultitrackMode ||
                              (event->file()->MultitrackMode && event->track()->number() == NewNoteTool::editTrack())))
                    trans = true;

            // visual octave correction for notes

            int displ = 0;

            if(channel >= 0 && channel < 16 && OctaveChan_MIDI[channel]) {
                if((channel == 9 && event->track()->fluid_index() == 0) ||
                        ((channel == 9 && event->track()->fluid_index() != 0
                    && MidiOutput::isFluidsynthConnected(event->track()->device_index()) && event->file()->DrumUseCh9))) {

                } else {
                     displ = OctaveChan_MIDI[channel];
                }
            }

            if(channel >= 0 && channel < 16 && displ) {

                OffEvent* offEvent = dynamic_cast<OffEvent*>(event);
                OnEvent* onEvent = dynamic_cast<OnEvent*>(event);

                if (onEvent || offEvent) {
                    QBrush d(Qt::darkBlue, Qt::Dense1Pattern);
                    if(trans)
                        d.setColor(0x14148c20);
                    painter->setBrush(d);

                } else {
                    if(trans)
                        painter->setBrush(QColor(0x14148c20));
                    else
                        painter->setBrush(Qt::darkBlue);
                }
            } else {
                if(trans)
                    painter->setBrush(QColor(0x14148c20));
                else
                    painter->setBrush(Qt::darkBlue);
            }
            painter->setPen(/*Qt::lightGray*/Qt::black);
            painter->drawRoundedRect(event->x(), event->y(), event->width(),
                event->height(), 2, 2);

            if(!trans) {

                QLinearGradient linearGrad(QPointF(event->x(), event->y()),
                                           QPointF(event->x() + event->width(),event->y() + event->height()));

                linearGrad.setColorAt(0, QColor(80, 80, 80, 0x20));
                linearGrad.setColorAt(0.5, QColor(0xcf, 0xcf, 0xcf, 0x70));
                linearGrad.setColorAt(1.0, QColor(0xff, 0xff, 0xff, 0x70));

                QBrush d(linearGrad);
                painter->setBrush(d);
                painter->drawRoundedRect(event->x(), event->y(), event->width(), event->height(), 2, 2);
            }

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
            if (newX == xt)
            {
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
        copiedEvents->clear();

        foreach (MidiEvent* event, Selection::instance()->selectedEvents()) {

            // add the current Event
            MidiEvent* ev = dynamic_cast<MidiEvent*>(event->copy());
            if (ev) {
                // do not append off event here
                OffEvent* off = dynamic_cast<OffEvent*>(ev);
                if (!off) {
                    copiedEvents->append(ev);
                }
            }

            // if its onEvent, add a copy of the OffEvent
            OnEvent* onEv = dynamic_cast<OnEvent*>(ev);
            if (onEv) {
                OffEvent* offEv = dynamic_cast<OffEvent*>(onEv->offEvent()->copy());
                if (offEv) {
                    offEv->setOnEvent(onEv);
                    copiedEvents->append(offEv);
                }
            }
        }

        if(copiedEvents) {
            QFile f(QDir::homePath() + "/Midieditor/file_cache/_copy_events.cpy");

            if(!currentFile()->saveMidiEvents(f, copiedEvents, SelectTool::selectMultiTrack)) {
                f.remove();
                QMessageBox::critical(_mainWindow, "MidiEditor - copyAction", "Error creating copy file containing the events");

            }
        }

        _mainWindow->copiedEventsChanged();
    }
}


void EventTool::pasteAction()
{

    if (copiedEvents->size() == 0) {
        return;
    }

    bool selectMultiTrack = SelectTool::selectMultiTrack;

    int num_copied = 0;

    // copy copied events to insert unique events
    QList<MidiEvent*> copiedCopiedEvents;

    foreach (MidiEvent* event, *copiedEvents) {

        // add the current Event
        MidiEvent* ev = dynamic_cast<MidiEvent*>(event->copy());
        if (ev) {
            // do not append off event here
            OffEvent* off = dynamic_cast<OffEvent*>(ev);
            if (!off) {

                TempoChangeEvent* tcev = dynamic_cast<TempoChangeEvent*>(ev);
                TimeSignatureEvent* tsev = dynamic_cast<TimeSignatureEvent*>(ev);

                if(tcev || tsev) { // non 0 position
                    if(currentFile()->cursorTick() >= currentFile()->tick(150)) {
                        copiedCopiedEvents.append(ev);
                        num_copied++;
                    }
                } else {
                    copiedCopiedEvents.append(ev);
                    num_copied++;
                }
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

    bool proto = false;

    double tickscale = 1;

    if (copiedCopiedEvents.count() > 0) {
        qWarning("super paste");
        // Begin a new ProtocolAction
        currentFile()->protocol()->startNewAction("Paste " + QString::number(num_copied) + " events");
        proto = true;

        if (currentFile() != copiedEvents->first()->file()) {
            tickscale = ((double)(currentFile()->ticksPerQuarter())) / ((double)copiedEvents->first()->file()->ticksPerQuarter());
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
            TempoChangeEvent* tcev = dynamic_cast<TempoChangeEvent*>(event);
            TimeSignatureEvent* tsev = dynamic_cast<TimeSignatureEvent*>(event);
            KeySignatureEvent* ksev = dynamic_cast<KeySignatureEvent*>(event);
            KeyPressureEvent* kpev = dynamic_cast<KeyPressureEvent*>(event);
            ChannelPressureEvent* cpev = dynamic_cast<ChannelPressureEvent*>(event);
            TextEvent* tev = dynamic_cast<TextEvent*>(event);
            PitchBendEvent* pbev = dynamic_cast<PitchBendEvent*>(event);
            SysExEvent* sys = dynamic_cast<SysExEvent*>(event);
            UnknownEvent* uev = dynamic_cast<UnknownEvent*>(event);
            ProgChangeEvent* prg = dynamic_cast<ProgChangeEvent*>(event);
            ControlChangeEvent* ctrl = dynamic_cast<ControlChangeEvent*>(event);

            // get channel
            int channel = event->channel();
            int dtick = currentFile()->tick(150);

            int current_tick = (int)(tickscale * event->midiTime()) + diff;

            if(!selectMultiTrack) {
                if (_pasteChannel == -2) {
                    channel = NewNoteTool::editChannel();
                } else if ((_pasteChannel >= 0) && (channel < 16)) {
                    channel = _pasteChannel;
                }
            }

            if(kpev || cpev || pbev || prg || ctrl) { // channel dep

                foreach (MidiEvent* event2,
                         *(currentFile()->eventsBetween(current_tick-dtick, current_tick+dtick))) {
                    KeyPressureEvent* kpev2 = dynamic_cast<KeyPressureEvent*>(event2);
                    ChannelPressureEvent* cpev2 = dynamic_cast<ChannelPressureEvent*>(event2);
                    PitchBendEvent* pbev2 = dynamic_cast<PitchBendEvent*>(event2);
                    ProgChangeEvent* prg2 = dynamic_cast<ProgChangeEvent*>(event2);
                    ControlChangeEvent* ctrl2 = dynamic_cast<ControlChangeEvent*>(event2);

                    if(kpev && kpev2 && kpev2->channel() == channel ) {
                        currentFile()->channel(channel)->removeEvent(event2);
                    } else if(cpev && cpev2 && cpev2->channel() == channel ) {
                        currentFile()->channel(channel)->removeEvent(event2);
                    } else if(pbev && pbev2 && pbev2->channel() == channel ) {
                        currentFile()->channel(channel)->removeEvent(event2);
                    } else if(prg && prg2 && prg2->channel() == channel ) {
                        currentFile()->channel(channel)->removeEvent(event2);
                    } else if(ctrl && ctrl2 && ctrl2->channel() == channel ) {
                        currentFile()->channel(channel)->removeEvent(event2);
                    }

                }

            } else if(tcev || tsev || ksev || tev
                     || uev) {


                foreach (MidiEvent* event2,
                         *(currentFile()->eventsBetween(current_tick-dtick, current_tick+dtick))) {
                    TempoChangeEvent* tcev2 = dynamic_cast<TempoChangeEvent*>(event2);
                    TimeSignatureEvent* tsev2 = dynamic_cast<TimeSignatureEvent*>(event2);
                    KeySignatureEvent* ksev2 = dynamic_cast<KeySignatureEvent*>(event2);
                    TextEvent* tev2 = dynamic_cast<TextEvent*>(event2);
                    UnknownEvent* uev2 = dynamic_cast<UnknownEvent*>(event2);

                    if((tcev && tcev2) || (tsev && tsev2) || (ksev && ksev2) ||
                            (tev && tev2) || (uev && uev2)) {
                        currentFile()->channel(event2->channel())->removeEvent(event2);
                    }

                }

            } else if(sys) {

                QByteArray c = sys->data();

                if(c[1] == (char) 0x66 && c[2] == (char) 0x66
                        && (c[3] == 'V' || c[3] == 'W' || c[3]=='R' || (c[3] & 0xF0)==0x70)) {

                    foreach (MidiEvent* event2,
                             *(currentFile()->eventsBetween(current_tick-dtick, current_tick+dtick))) {
                        SysExEvent* sys2 = dynamic_cast<SysExEvent*>(event2);

                        if(sys2) {
                            QByteArray d = sys2->data();

                            // sysEx chan 0x70
                            if((c[3] & 0xF0) == 0x70 && (d[3] & 0xF0) == 0x70 && c[0] == d[0] && c[1] == d[1] && c[2] == d[2]) {
                                int chan = c[3] & 0xF;

                                if(!selectMultiTrack) {
                                    if (_pasteChannel == -2) {
                                        chan = NewNoteTool::editChannel();
                                    }
                                    if ((_pasteChannel >= 0) && (chan < 16)) {
                                        chan = _pasteChannel;
                                    }
                                }

                                if((d[3] & 0xF) == chan)
                                    currentFile()->channel(16)->removeEvent(sys2);

                            } else
                                // sysEx chan 'W'
                                if(c[3] == d[3] && c[3] == 'W' && c[0] == d[0] && c[1] == d[1] && c[2] == d[2]) {
                                int chan = c[0] & 0xF;

                                if(!selectMultiTrack) {
                                    if (_pasteChannel == -2) {
                                        chan = NewNoteTool::editChannel();
                                    }
                                    if ((_pasteChannel >= 0) && (chan < 16)) {
                                        chan = _pasteChannel;
                                    }
                                }

                                chan|= (c[0] & 0xf0);

                                if((d[0] & 0xFF) == chan)
                                    currentFile()->channel(16)->removeEvent(sys2);

                            } else if(c[0] == d[0] && c[1] == d[1] && c[2] == d[2] && c[3] == d[3]) {
                                currentFile()->channel(16)->removeEvent(sys2);
                            }
                        }
                    }
                }

            }

        }

        foreach (MidiEvent* event, copiedCopiedEvents) {
            SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

            // get channel
            int channel = event->channel();

            if(!selectMultiTrack) {
                if (_pasteChannel == -2) {
                    channel = NewNoteTool::editChannel();
                }
                if ((_pasteChannel >= 0) && (channel < 16)) {
                    channel = _pasteChannel;
                }
            }

            if(sys) {
                QByteArray c = sys->data();

                // get track
                MidiTrack* track = event->track();

                // sysEx to track 0 always
                track = currentFile()->track(0);
                // sysEx chan 0x70
                if((c[3] & 0xF0) == 0x70 && c[0] == (char) 0x00 && c[1] == (char) 0x66 && c[2] == (char) 0x66) {
                    int chan = c[3] & 0xF;

                    if(!selectMultiTrack) {
                        if (_pasteChannel == -2) {
                            chan = NewNoteTool::editChannel();
                        }
                        if ((_pasteChannel >= 0) && (chan < 16)) {
                            chan = _pasteChannel;
                        }
                    }

                    c[3] = 0x70 | (chan & 0xf);

                    sys->setData(c);
                    sys->save();

                } else if(c[3] == 'W' && c[1] == (char) 0x66 && c[2] == (char) 0x66) {
                    int chan = c[0] & 0xF;

                    if(!selectMultiTrack) {
                        if (_pasteChannel == -2) {
                            chan = NewNoteTool::editChannel();
                        }
                        if ((_pasteChannel >= 0) && (chan < 16)) {
                            chan = _pasteChannel;
                        }
                    }

                    chan|= (c[0] & 0xf0);

                    c[0] = chan;
                    sys->setData(c);
                    sys->save();

                }

                event->setFile(currentFile());
                event->setChannel(16, false);
                event->setTrack(track, false);

                currentFile()->channel(channel)->insertEvent(event,
                    (int)(tickscale * event->midiTime()) + diff);
                selectEvent(event, false, true);

            }
            else {//
                // get track
                MidiTrack* track = event->track();
                TempoChangeEvent* tcev = dynamic_cast<TempoChangeEvent*>(event);
                TimeSignatureEvent* tsev = dynamic_cast<TimeSignatureEvent*>(event);

                if(tcev || tsev)
                    track = currentFile()->track(0);
                else if (pasteTrack() == -2 && !selectMultiTrack) {
                    track = currentFile()->track(NewNoteTool::editTrack());
                } else if ((pasteTrack() >= 0)  && !selectMultiTrack && (pasteTrack() < currentFile()->tracks()->size())) {
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

            } //
        }
    }

    if(proto)
        currentFile()->protocol()->endAction();

    currentFile()->calcMaxTime();
}

void EventTool::pasteAction2()
{
    MidiFile tempfile; // temp file

    static QList<MidiEvent*> savedEvents;

    bool selectMultiTrack = SelectTool::selectMultiTrack;

    currentFile()->loadMidiEvents(&savedEvents, &tempfile, &selectMultiTrack);

    if (savedEvents.size() == 0) {
        return;
    }

    int num_copied = 0;

    // copy copied events to insert unique events
    QList<MidiEvent*> copiedCopiedEvents;

    foreach (MidiEvent* event, savedEvents) {

        // add the current Event
        MidiEvent* ev = dynamic_cast<MidiEvent*>(event->copy());
        if (ev) {
            // do not append off event here
            OffEvent* off = dynamic_cast<OffEvent*>(ev);
            if (!off) {

                TempoChangeEvent* tcev = dynamic_cast<TempoChangeEvent*>(ev);
                TimeSignatureEvent* tsev = dynamic_cast<TimeSignatureEvent*>(ev);

                if(tcev || tsev) { // non 0 position
                    if(currentFile()->cursorTick() >= currentFile()->tick(150)) {
                        copiedCopiedEvents.append(ev);
                        num_copied++;
                    }
                } else {
                    copiedCopiedEvents.append(ev);
                    num_copied++;
                }
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

    bool proto = false;

    double tickscale = 1;

    if (copiedCopiedEvents.count() > 0) {
        qWarning("super paste");
        // Begin a new ProtocolAction
        currentFile()->protocol()->startNewAction("Paste " + QString::number(num_copied) + " events");
        proto = true;

        if (currentFile() != savedEvents.first()->file()) {
            tickscale = ((double)(currentFile()->ticksPerQuarter())) / ((double)savedEvents.first()->file()->ticksPerQuarter());
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
            TempoChangeEvent* tcev = dynamic_cast<TempoChangeEvent*>(event);
            TimeSignatureEvent* tsev = dynamic_cast<TimeSignatureEvent*>(event);
            KeySignatureEvent* ksev = dynamic_cast<KeySignatureEvent*>(event);
            KeyPressureEvent* kpev = dynamic_cast<KeyPressureEvent*>(event);
            ChannelPressureEvent* cpev = dynamic_cast<ChannelPressureEvent*>(event);
            TextEvent* tev = dynamic_cast<TextEvent*>(event);
            PitchBendEvent* pbev = dynamic_cast<PitchBendEvent*>(event);
            SysExEvent* sys = dynamic_cast<SysExEvent*>(event);
            UnknownEvent* uev = dynamic_cast<UnknownEvent*>(event);
            ProgChangeEvent* prg = dynamic_cast<ProgChangeEvent*>(event);
            ControlChangeEvent* ctrl = dynamic_cast<ControlChangeEvent*>(event);

            // get channel
            int channel = event->channel();
            int dtick = currentFile()->tick(150);

            int current_tick = (int)(tickscale * event->midiTime()) + diff;

            if(!selectMultiTrack) {
                if (_pasteChannel == -2) {
                    channel = NewNoteTool::editChannel();
                }
                if ((_pasteChannel >= 0) && (channel < 16)) {
                    channel = _pasteChannel;
                }
            }

            if(kpev || cpev || pbev || prg || ctrl) { // channel dep

                foreach (MidiEvent* event2,
                         *(currentFile()->eventsBetween(current_tick-dtick, current_tick+dtick))) {
                    KeyPressureEvent* kpev2 = dynamic_cast<KeyPressureEvent*>(event2);
                    ChannelPressureEvent* cpev2 = dynamic_cast<ChannelPressureEvent*>(event2);
                    PitchBendEvent* pbev2 = dynamic_cast<PitchBendEvent*>(event2);
                    ProgChangeEvent* prg2 = dynamic_cast<ProgChangeEvent*>(event2);
                    ControlChangeEvent* ctrl2 = dynamic_cast<ControlChangeEvent*>(event2);

                    if(kpev && kpev2 && kpev2->channel() == channel ) {
                        currentFile()->channel(channel)->removeEvent(event2);
                    } else if(cpev && cpev2 && cpev2->channel() == channel ) {
                        currentFile()->channel(channel)->removeEvent(event2);
                    } else if(pbev && pbev2 && pbev2->channel() == channel ) {
                        currentFile()->channel(channel)->removeEvent(event2);
                    } else if(prg && prg2 && prg2->channel() == channel ) {
                        currentFile()->channel(channel)->removeEvent(event2);
                    } else if(ctrl && ctrl2 && ctrl2->channel() == channel ) {
                        currentFile()->channel(channel)->removeEvent(event2);
                    }

                }

            } else if(tcev || tsev || ksev || tev
                     || uev) {


                foreach (MidiEvent* event2,
                         *(currentFile()->eventsBetween(current_tick-dtick, current_tick+dtick))) {
                    TempoChangeEvent* tcev2 = dynamic_cast<TempoChangeEvent*>(event2);
                    TimeSignatureEvent* tsev2 = dynamic_cast<TimeSignatureEvent*>(event2);
                    KeySignatureEvent* ksev2 = dynamic_cast<KeySignatureEvent*>(event2);
                    TextEvent* tev2 = dynamic_cast<TextEvent*>(event2);
                    UnknownEvent* uev2 = dynamic_cast<UnknownEvent*>(event2);

                    if((tcev && tcev2) || (tsev && tsev2) || (ksev && ksev2) ||
                        (tev && tev2) || (uev && uev2)) {
                        currentFile()->channel(event2->channel())->removeEvent(event2);
                    }

                }

            } else if(sys) {

                QByteArray c = sys->data();

                if(c[1] == (char) 0x66 && c[2] == (char) 0x66
                    && (c[3] == 'V' || c[3] == 'W' || c[3]=='R' || (c[3] & 0xF0)==0x70)) {

                    foreach (MidiEvent* event2,
                             *(currentFile()->eventsBetween(current_tick-dtick, current_tick+dtick))) {
                        SysExEvent* sys2 = dynamic_cast<SysExEvent*>(event2);

                        if(sys2) {
                            QByteArray d = sys2->data();

                                   // sysEx chan 0x70
                            if((c[3] & 0xF0) == 0x70 && (d[3] & 0xF0) == 0x70 && c[0] == d[0] && c[1] == d[1] && c[2] == d[2]) {
                                int chan = c[3] & 0xF;

                                if(!selectMultiTrack) {
                                    if (_pasteChannel == -2) {
                                        chan = NewNoteTool::editChannel();
                                    }
                                    if ((_pasteChannel >= 0) && (chan < 16)) {
                                        chan = _pasteChannel;
                                    }
                                }

                                if((d[3] & 0xF) == chan)
                                    currentFile()->channel(16)->removeEvent(sys2);

                            } else
                                // sysEx chan 'W'
                                if(c[3] == d[3] && c[3] == 'W' && c[0] == d[0] && c[1] == d[1] && c[2] == d[2]) {
                                    int chan = c[0] & 0xF;

                                    if(!selectMultiTrack) {
                                        if (_pasteChannel == -2) {
                                            chan = NewNoteTool::editChannel();
                                        }
                                        if ((_pasteChannel >= 0) && (chan < 16)) {
                                            chan = _pasteChannel;
                                        }
                                    }

                                    chan|= (c[0] & 0xf0);

                                    if((d[0] & 0xFF) == chan)
                                        currentFile()->channel(16)->removeEvent(sys2);

                                } else if(c[0] == d[0] && c[1] == d[1] && c[2] == d[2] && c[3] == d[3]) {
                                    currentFile()->channel(16)->removeEvent(sys2);
                                }
                        }
                    }
                }

            }

        }

        foreach (MidiEvent* event, copiedCopiedEvents) {
            SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

                   // get channel
            int channel = event->channel();

            if(!selectMultiTrack) {
                if (_pasteChannel == -2) {
                    channel = NewNoteTool::editChannel();
                }
                if ((_pasteChannel >= 0) && (channel < 16)) {
                    channel = _pasteChannel;
                }
            }

            if(sys) {
                QByteArray c = sys->data();

                // get track
                MidiTrack* track =  currentFile()->track(event->track()->number());

                // sysEx to track 0 always
                track = currentFile()->track(0);
                // sysEx chan 0x70
                if((c[3] & 0xF0) == 0x70 && c[0] == (char) 0x00 && c[1] == (char) 0x66 && c[2] == (char) 0x66) {
                    int chan = c[3] & 0xF;

                    if(!selectMultiTrack) {
                        if (_pasteChannel == -2) {
                            chan = NewNoteTool::editChannel();
                        }
                        if ((_pasteChannel >= 0) && (chan < 16)) {
                            chan = _pasteChannel;
                        }
                    }

                    c[3] = 0x70 | (chan & 0xf);

                    sys->setData(c);
                    sys->save();

                } else if(c[3] == 'W' && c[1] == (char) 0x66 && c[2] == (char) 0x66) {
                    int chan = c[0] & 0xF;

                    if(!selectMultiTrack) {
                        if (_pasteChannel == -2) {
                            chan = NewNoteTool::editChannel();
                        }
                        if ((_pasteChannel >= 0) && (chan < 16)) {
                            chan = _pasteChannel;
                        }
                    }

                    chan|= (c[0] & 0xf0);

                    c[0] = chan;
                    sys->setData(c);
                    sys->save();

                }

                event->setFile(currentFile());
                event->setChannel(16, false);
                event->setTrack(track, false);

                currentFile()->channel(channel)->insertEvent(event,
                                                             (int)(tickscale * event->midiTime()) + diff);
                selectEvent(event, false, true);

            }
            else {//
                // get track
                MidiTrack* track = currentFile()->track(event->track()->number());
                TempoChangeEvent* tcev = dynamic_cast<TempoChangeEvent*>(event);
                TimeSignatureEvent* tsev = dynamic_cast<TimeSignatureEvent*>(event);

                if(tcev || tsev)
                    track = currentFile()->track(0);
                else if (pasteTrack() == -2 && !selectMultiTrack) {
                    track = currentFile()->track(NewNoteTool::editTrack());
                } else if ((pasteTrack() >= 0) && !selectMultiTrack && (pasteTrack() < currentFile()->tracks()->size())) {
                    track = currentFile()->track(pasteTrack());
                } else if (event->file() != currentFile() || !currentFile()->tracks()->contains(track)) {

                    //track = currentFile()->getPasteTrack(event->track(), event->file());
                    //MidiTrack* track = currentFile()->track(event->track()->number());
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

            } //
        }
    }

    if(proto)
        currentFile()->protocol()->endAction();

    currentFile()->calcMaxTime();
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
    int size_x = matrixWidget->divs().at(1).first - matrixWidget->divs().at(0).first - 2;
    if(size_x < 5)
        size_x = 5;
    foreach (TMPPair p, matrixWidget->divs()) {
        int xt = p.first;
        if (qAbs(xt - x) <= size_x) {
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
