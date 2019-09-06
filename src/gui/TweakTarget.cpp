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

#include <QElapsedTimer>
#include <QList>
#include <QPair>
#include "MainWindow.h"
#include "MatrixWidget.h"
#include "TweakTarget.h"
#include "../MidiEvent/ChannelPressureEvent.h"
#include "../MidiEvent/ControlChangeEvent.h"
#include "../MidiEvent/KeyPressureEvent.h"
#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/NoteOnEvent.h"
#include "../MidiEvent/OffEvent.h"
#include "../MidiEvent/OnEvent.h"
#include "../MidiEvent/PitchBendEvent.h"
#include "../MidiEvent/TempoChangeEvent.h"
#include "../midi/MidiFile.h"
#include "../protocol/Protocol.h"
#include "../tool/Selection.h"

static int getDivNumberForTime(QList<QPair<int, int> > divs, int time)
{
    for (int divNumber = divs.size() - 1; divNumber >= 0; divNumber--) {
        if (divs.at(divNumber).second <= time) return divNumber;
    }

    return 0;
}

static int getTimeOneDivEarlier(QList<QPair<int, int> > divs, int time)
{
    int divNumber = getDivNumberForTime(divs, time);
    int divStartTime = divs.at(divNumber).second;
    int previousDivStartTime;

    if (divNumber > 0) {
        previousDivStartTime = divs.at(divNumber - 1).second;
    } else {
        int nextDivStartTime = divs.at(divNumber + 1).second;
        previousDivStartTime = divStartTime - (nextDivStartTime - divStartTime);
    }

    return previousDivStartTime + (time - divStartTime);
}

static int getTimeOneDivLater(QList<QPair<int, int> > divs, int time)
{
    int divNumber = getDivNumberForTime(divs, time);
    int divStartTime = divs.at(divNumber).second;
    int nextDivStartTime;

    if (divNumber + 1 < divs.size()) {
        nextDivStartTime = divs.at(divNumber + 1).second;
    } else {
        int previousDivStartTime = divs.at(divNumber - 1).second;
        nextDivStartTime = divStartTime + (divStartTime - previousDivStartTime);
    }

    return nextDivStartTime + (time - divStartTime);
}

TweakTarget::TweakTarget()
{
    firstActionTimer = new QElapsedTimer();
    firstActionTimer->start();
    lastActionTimer = new QElapsedTimer();
    lastActionTimer->start();
}

int TweakTarget::getAccelleratedIncrement()
{
    if (lastActionTimer->elapsed() > 100) firstActionTimer->restart();
    lastActionTimer->restart();
    return (firstActionTimer->elapsed() / 200) + 1;
}

TimeTweakTarget::TimeTweakTarget(MainWindow* mainWindow): TweakTarget()
{
    this->mainWindow = mainWindow;
}

void TimeTweakTarget::smallDecrease()
{
    int accelleratedIncrement = getAccelleratedIncrement();
    MidiFile* file = mainWindow->getFile();

    if (file) {
        QList<MidiEvent*> selectedEvents = Selection::instance()->selectedEvents();

        if (selectedEvents.size() > 0) {
            Protocol* protocol = file->protocol();
            protocol->startNewAction("Tweak");

            foreach (MidiEvent* e, selectedEvents) {
                int newTime = e->midiTime() - accelleratedIncrement;

                if (newTime >= 0) {
                    e->setMidiTime(newTime);

                    OnEvent* onEvent = dynamic_cast<OnEvent*>(e);
                    if (onEvent) {
                        MidiEvent* offEvent = onEvent->offEvent();
                        offEvent->setMidiTime(offEvent->midiTime() - accelleratedIncrement);
                    }
                }
            }

            protocol->endAction();
        }
    }
}

void TimeTweakTarget::smallIncrease()
{
    int accelleratedIncrement = getAccelleratedIncrement();
    MidiFile* file = mainWindow->getFile();

    if (file) {
        QList<MidiEvent*> selectedEvents = Selection::instance()->selectedEvents();

        if (selectedEvents.size() > 0) {
            Protocol* protocol = file->protocol();
            protocol->startNewAction("Tweak");

            foreach (MidiEvent* e, selectedEvents) {
                e->setMidiTime(e->midiTime() + accelleratedIncrement);

                OnEvent* onEvent = dynamic_cast<OnEvent*>(e);
                if (onEvent) {
                    MidiEvent* offEvent = onEvent->offEvent();
                    offEvent->setMidiTime(offEvent->midiTime() + accelleratedIncrement);
                }
            }

            protocol->endAction();
        }
    }
}

void TimeTweakTarget::largeDecrease()
{
    MidiFile* file = mainWindow->getFile();

    if (file) {
        QList<MidiEvent*> selectedEvents = Selection::instance()->selectedEvents();

        if (selectedEvents.size() > 0) {
            Protocol* protocol = file->protocol();
            protocol->startNewAction("Tweak");
            QList<QPair<int, int> > divs = mainWindow->matrixWidget()->divs();

            foreach (MidiEvent* e, selectedEvents) {
                int time = e->midiTime();
                int newTime = getTimeOneDivEarlier(divs, time);
                if (newTime >= 0) {
                    e->setMidiTime(newTime);

                    OnEvent* onEvent = dynamic_cast<OnEvent*>(e);
                    if (onEvent) {
                        MidiEvent* offEvent = onEvent->offEvent();
                        int newOffTime = offEvent->midiTime() + (newTime - time);
                        offEvent->setMidiTime(newOffTime);
                    }
                }
            }

            protocol->endAction();
        }
    }
}

void TimeTweakTarget::largeIncrease()
{
    MidiFile* file = mainWindow->getFile();

    if (file) {
        QList<MidiEvent*> selectedEvents = Selection::instance()->selectedEvents();

        if (selectedEvents.size() > 0) {
            Protocol* protocol = file->protocol();
            protocol->startNewAction("Tweak");
            QList<QPair<int, int> > divs = mainWindow->matrixWidget()->divs();

            foreach (MidiEvent* e, selectedEvents) {
                int time = e->midiTime();
                int newTime = getTimeOneDivLater(divs, time);
                e->setMidiTime(newTime);

                OnEvent* onEvent = dynamic_cast<OnEvent*>(e);
                if (onEvent) {
                    MidiEvent* offEvent = onEvent->offEvent();
                    int newOffTime = offEvent->midiTime() + (newTime - time);
                    offEvent->setMidiTime(newOffTime);
                }
            }

            protocol->endAction();
        }
    }
}

StartTimeTweakTarget::StartTimeTweakTarget(MainWindow* mainWindow): TweakTarget()
{
    this->mainWindow = mainWindow;
}

void StartTimeTweakTarget::smallDecrease()
{
    int accelleratedIncrement = getAccelleratedIncrement();
    MidiFile* file = mainWindow->getFile();

    if (file) {
        QList<MidiEvent*> selectedEvents = Selection::instance()->selectedEvents();

        if (selectedEvents.size() > 0) {
            Protocol* protocol = file->protocol();
            protocol->startNewAction("Tweak");

            foreach (MidiEvent* e, selectedEvents) {
                int newTime = e->midiTime() - accelleratedIncrement;
                if (newTime >= 0) e->setMidiTime(newTime);
            }

            protocol->endAction();
        }
    }
}

void StartTimeTweakTarget::smallIncrease()
{
    int accelleratedIncrement = getAccelleratedIncrement();
    MidiFile* file = mainWindow->getFile();

    if (file) {
        QList<MidiEvent*> selectedEvents = Selection::instance()->selectedEvents();

        if (selectedEvents.size() > 0) {
            Protocol* protocol = file->protocol();
            protocol->startNewAction("Tweak");

            foreach (MidiEvent* e, selectedEvents) {
                int newTime = e->midiTime() + accelleratedIncrement;
                e->setMidiTime(newTime);

                OnEvent* onEvent = dynamic_cast<OnEvent*>(e);
                if (onEvent) {
                    MidiEvent* offEvent = onEvent->offEvent();
                    if (newTime > offEvent->midiTime()) offEvent->setMidiTime(newTime);
                }
            }

            protocol->endAction();
        }
    }
}

void StartTimeTweakTarget::largeDecrease()
{
    MidiFile* file = mainWindow->getFile();

    if (file) {
        QList<MidiEvent*> selectedEvents = Selection::instance()->selectedEvents();

        if (selectedEvents.size() > 0) {
            Protocol* protocol = file->protocol();
            protocol->startNewAction("Tweak");
            QList<QPair<int, int> > divs = mainWindow->matrixWidget()->divs();

            foreach (MidiEvent* e, selectedEvents) {
                int newTime = getTimeOneDivEarlier(divs, e->midiTime());
                if (newTime >= 0) e->setMidiTime(newTime);
            }

            protocol->endAction();
        }
    }
}

void StartTimeTweakTarget::largeIncrease()
{
    MidiFile* file = mainWindow->getFile();

    if (file) {
        QList<MidiEvent*> selectedEvents = Selection::instance()->selectedEvents();

        if (selectedEvents.size() > 0) {
            Protocol* protocol = file->protocol();
            protocol->startNewAction("Tweak");
            QList<QPair<int, int> > divs = mainWindow->matrixWidget()->divs();

            foreach (MidiEvent* e, selectedEvents) {
                int newTime = getTimeOneDivLater(divs, e->midiTime());
                e->setMidiTime(newTime);

                OnEvent* onEvent = dynamic_cast<OnEvent*>(e);
                if (onEvent) {
                    MidiEvent* offEvent = onEvent->offEvent();
                    if (newTime > offEvent->midiTime()) offEvent->setMidiTime(newTime);
                }
            }

            protocol->endAction();
        }
    }
}

EndTimeTweakTarget::EndTimeTweakTarget(MainWindow* mainWindow): TweakTarget()
{
    this->mainWindow = mainWindow;
}

void EndTimeTweakTarget::smallDecrease()
{
    int accelleratedIncrement = getAccelleratedIncrement();
    MidiFile* file = mainWindow->getFile();

    if (file) {
        QList<MidiEvent*> selectedEvents = Selection::instance()->selectedEvents();

        if (selectedEvents.size() > 0) {
            Protocol* protocol = file->protocol();
            protocol->startNewAction("Tweak");

            foreach (MidiEvent* e, selectedEvents) {
                OnEvent* onEvent = dynamic_cast<OnEvent*>(e);
                if (onEvent) {
                    MidiEvent* offEvent = onEvent->offEvent();
                    int newTime = offEvent->midiTime() - accelleratedIncrement;
                    if (newTime >= 0) {
                        offEvent->setMidiTime(newTime);
                        if (newTime < onEvent->midiTime()) onEvent->setMidiTime(newTime);
                    }
                } else {
                    int newTime = e->midiTime() - accelleratedIncrement;
                    if (newTime >= 0) e->setMidiTime(newTime);
                }
            }

            protocol->endAction();
        }
    }
}

void EndTimeTweakTarget::smallIncrease()
{
    int accelleratedIncrement = getAccelleratedIncrement();
    MidiFile* file = mainWindow->getFile();

    if (file) {
        QList<MidiEvent*> selectedEvents = Selection::instance()->selectedEvents();

        if (selectedEvents.size() > 0) {
            Protocol* protocol = file->protocol();
            protocol->startNewAction("Tweak");

            foreach (MidiEvent* e, selectedEvents) {
                OnEvent* onEvent = dynamic_cast<OnEvent*>(e);
                if (onEvent) {
                    MidiEvent* offEvent = onEvent->offEvent();
                    offEvent->setMidiTime(offEvent->midiTime() + accelleratedIncrement);
                } else {
                    e->setMidiTime(e->midiTime() + accelleratedIncrement);
                }
            }

            protocol->endAction();
        }
    }
}

void EndTimeTweakTarget::largeDecrease()
{
    MidiFile* file = mainWindow->getFile();

    if (file) {
        QList<MidiEvent*> selectedEvents = Selection::instance()->selectedEvents();

        if (selectedEvents.size() > 0) {
            Protocol* protocol = file->protocol();
            protocol->startNewAction("Tweak");
            QList<QPair<int, int> > divs = mainWindow->matrixWidget()->divs();

            foreach (MidiEvent* e, selectedEvents) {
                OnEvent* onEvent = dynamic_cast<OnEvent*>(e);
                if (onEvent) {
                    MidiEvent* offEvent = onEvent->offEvent();
                    int newTime = getTimeOneDivEarlier(divs, offEvent->midiTime());
                    if (newTime >= 0) {
                        offEvent->setMidiTime(newTime);
                        if (newTime < onEvent->midiTime()) onEvent->setMidiTime(newTime);
                    }
                } else {
                    int newTime = getTimeOneDivEarlier(divs, e->midiTime());
                    if (newTime >= 0) e->setMidiTime(newTime);
                }
            }

            protocol->endAction();
        }
    }
}

void EndTimeTweakTarget::largeIncrease()
{
    MidiFile* file = mainWindow->getFile();

    if (file) {
        QList<MidiEvent*> selectedEvents = Selection::instance()->selectedEvents();

        if (selectedEvents.size() > 0) {
            Protocol* protocol = file->protocol();
            protocol->startNewAction("Tweak");
            QList<QPair<int, int> > divs = mainWindow->matrixWidget()->divs();

            foreach (MidiEvent* e, selectedEvents) {
                OnEvent* onEvent = dynamic_cast<OnEvent*>(e);
                if (onEvent) {
                    MidiEvent* offEvent = onEvent->offEvent();
                    int newTime = getTimeOneDivLater(divs, offEvent->midiTime());
                    offEvent->setMidiTime(newTime);
                } else {
                    int newTime = getTimeOneDivLater(divs, e->midiTime());
                    e->setMidiTime(newTime);
                }
            }

            protocol->endAction();
        }
    }
}

NoteTweakTarget::NoteTweakTarget(MainWindow* mainWindow): TweakTarget()
{
    this->mainWindow = mainWindow;
}

void NoteTweakTarget::smallDecrease()
{
    MidiFile* file = mainWindow->getFile();

    if (file) {
        QList<MidiEvent*> selectedEvents = Selection::instance()->selectedEvents();

        if (selectedEvents.size() > 0) {
            Protocol* protocol = file->protocol();
            protocol->startNewAction("Tweak");

            foreach (MidiEvent* e, selectedEvents) {
                NoteOnEvent* noteOnEvent = dynamic_cast<NoteOnEvent*>(e);
                if (noteOnEvent) {
                    int newNote = noteOnEvent->note() - 1;
                    if (newNote >= 0) noteOnEvent->setNote(newNote);
                }
            }

            protocol->endAction();
        }
    }
}

void NoteTweakTarget::smallIncrease()
{
    MidiFile* file = mainWindow->getFile();

    if (file) {
        QList<MidiEvent*> selectedEvents = Selection::instance()->selectedEvents();

        if (selectedEvents.size() > 0) {
            Protocol* protocol = file->protocol();
            protocol->startNewAction("Tweak");

            foreach (MidiEvent* e, selectedEvents) {
                NoteOnEvent* noteOnEvent = dynamic_cast<NoteOnEvent*>(e);
                if (noteOnEvent) {
                    int newNote = noteOnEvent->note() + 1;
                    if (newNote < 128) noteOnEvent->setNote(newNote);
                }
            }

            protocol->endAction();
        }
    }
}

void NoteTweakTarget::largeDecrease()
{
    MidiFile* file = mainWindow->getFile();

    if (file) {
        QList<MidiEvent*> selectedEvents = Selection::instance()->selectedEvents();

        if (selectedEvents.size() > 0) {
            Protocol* protocol = file->protocol();
            protocol->startNewAction("Tweak");

            foreach (MidiEvent* e, selectedEvents) {
                NoteOnEvent* noteOnEvent = dynamic_cast<NoteOnEvent*>(e);
                if (noteOnEvent) {
                    int newNote = noteOnEvent->note() - 12;
                    if (newNote >= 0) noteOnEvent->setNote(newNote);
                }
            }

            protocol->endAction();
        }
    }
}

void NoteTweakTarget::largeIncrease()
{
    MidiFile* file = mainWindow->getFile();

    if (file) {
        QList<MidiEvent*> selectedEvents = Selection::instance()->selectedEvents();

        if (selectedEvents.size() > 0) {
            Protocol* protocol = file->protocol();
            protocol->startNewAction("Tweak");

            foreach (MidiEvent* e, selectedEvents) {
                NoteOnEvent* noteOnEvent = dynamic_cast<NoteOnEvent*>(e);
                if (noteOnEvent) {
                    int newNote = noteOnEvent->note() + 12;
                    if (newNote < 128) noteOnEvent->setNote(newNote);
                }
            }

            protocol->endAction();
        }
    }
}

ValueTweakTarget::ValueTweakTarget(MainWindow* mainWindow): TweakTarget()
{
    this->mainWindow = mainWindow;
}

void ValueTweakTarget::smallDecrease()
{
    MidiFile* file = mainWindow->getFile();

    if (file) {
        QList<MidiEvent*> selectedEvents = Selection::instance()->selectedEvents();

        if (selectedEvents.size() > 0) {
            Protocol* protocol = file->protocol();
            protocol->startNewAction("Tweak");

            foreach (MidiEvent* e, selectedEvents) {
                NoteOnEvent* noteOnEvent = dynamic_cast<NoteOnEvent*>(e);
                if (noteOnEvent) {
                    int newVelocity = noteOnEvent->velocity() - 1;
                    if (newVelocity >= 0) noteOnEvent->setVelocity(newVelocity);
                }

                ControlChangeEvent* controlChangeEvent = dynamic_cast<ControlChangeEvent*>(e);
                if (controlChangeEvent) {
                    int newValue = controlChangeEvent->value() - 1;
                    if (newValue >= 0) controlChangeEvent->setValue(newValue);
                }

                PitchBendEvent* pitchBendEvent = dynamic_cast<PitchBendEvent*>(e);
                if (pitchBendEvent) {
                    int newValue = pitchBendEvent->value() - 1;
                    if (newValue >= 0) pitchBendEvent->setValue(newValue);
                }

                KeyPressureEvent* keyPressureEvent = dynamic_cast<KeyPressureEvent*>(e);
                if (keyPressureEvent) {
                    int newValue = keyPressureEvent->value() - 1;
                    if (newValue >= 0) keyPressureEvent->setValue(newValue);
                }

                ChannelPressureEvent* channelPressureEvent = dynamic_cast<ChannelPressureEvent*>(e);
                if (channelPressureEvent) {
                    int newValue = channelPressureEvent->value() - 1;
                    if (newValue >= 0) channelPressureEvent->setValue(newValue);
                }

                TempoChangeEvent* tempoChangeEvent = dynamic_cast<TempoChangeEvent*>(e);
                if (tempoChangeEvent) {
                    int newBeatsPerQuarter = tempoChangeEvent->beatsPerQuarter() - 1;
                    if (newBeatsPerQuarter >= 0) tempoChangeEvent->setBeats(newBeatsPerQuarter);
                }
            }

            protocol->endAction();
        }
    }
}

void ValueTweakTarget::smallIncrease()
{
    MidiFile* file = mainWindow->getFile();

    if (file) {
        QList<MidiEvent*> selectedEvents = Selection::instance()->selectedEvents();

        if (selectedEvents.size() > 0) {
            Protocol* protocol = file->protocol();
            protocol->startNewAction("Tweak");

            foreach (MidiEvent* e, selectedEvents) {
                NoteOnEvent* noteOnEvent = dynamic_cast<NoteOnEvent*>(e);
                if (noteOnEvent) {
                    int newVelocity = noteOnEvent->velocity() + 1;
                    if (newVelocity < 128) noteOnEvent->setVelocity(newVelocity);
                }

                ControlChangeEvent* controlChangeEvent = dynamic_cast<ControlChangeEvent*>(e);
                if (controlChangeEvent) {
                    int newValue = controlChangeEvent->value() + 1;
                    if (newValue < 128) controlChangeEvent->setValue(newValue);
                }

                PitchBendEvent* pitchBendEvent = dynamic_cast<PitchBendEvent*>(e);
                if (pitchBendEvent) {
                    int newValue = pitchBendEvent->value() + 1;
                    if (newValue < (1 << 14)) pitchBendEvent->setValue(newValue);
                }

                KeyPressureEvent* keyPressureEvent = dynamic_cast<KeyPressureEvent*>(e);
                if (keyPressureEvent) {
                    int newValue = keyPressureEvent->value() + 1;
                    if (newValue < 128) keyPressureEvent->setValue(newValue);
                }

                ChannelPressureEvent* channelPressureEvent = dynamic_cast<ChannelPressureEvent*>(e);
                if (channelPressureEvent) {
                    int newValue = channelPressureEvent->value() + 1;
                    if (newValue < 128) channelPressureEvent->setValue(newValue);
                }

                TempoChangeEvent* tempoChangeEvent = dynamic_cast<TempoChangeEvent*>(e);
                if (tempoChangeEvent) {
                    int newBeatsPerQuarter = tempoChangeEvent->beatsPerQuarter() + 1;
                    tempoChangeEvent->setBeats(newBeatsPerQuarter);
                }
            }

            protocol->endAction();
        }
    }
}

void ValueTweakTarget::largeDecrease()
{
    int accelleratedIncrement = getAccelleratedIncrement() * 10;
    MidiFile* file = mainWindow->getFile();

    if (file) {
        QList<MidiEvent*> selectedEvents = Selection::instance()->selectedEvents();

        if (selectedEvents.size() > 0) {
            Protocol* protocol = file->protocol();
            protocol->startNewAction("Tweak");

            foreach (MidiEvent* e, selectedEvents) {
                NoteOnEvent* noteOnEvent = dynamic_cast<NoteOnEvent*>(e);
                if (noteOnEvent) {
                    int newVelocity = noteOnEvent->velocity() - 10;
                    if (newVelocity >= 0) noteOnEvent->setVelocity(newVelocity);
                }

                ControlChangeEvent* controlChangeEvent = dynamic_cast<ControlChangeEvent*>(e);
                if (controlChangeEvent) {
                    int newValue = controlChangeEvent->value() - 10;
                    if (newValue >= 0) controlChangeEvent->setValue(newValue);
                }

                PitchBendEvent* pitchBendEvent = dynamic_cast<PitchBendEvent*>(e);
                if (pitchBendEvent) {
                    int newValue = pitchBendEvent->value() - accelleratedIncrement;
                    if (newValue >= 0) pitchBendEvent->setValue(newValue);
                }

                KeyPressureEvent* keyPressureEvent = dynamic_cast<KeyPressureEvent*>(e);
                if (keyPressureEvent) {
                    int newValue = keyPressureEvent->value() - 10;
                    if (newValue >= 0) keyPressureEvent->setValue(newValue);
                }

                ChannelPressureEvent* channelPressureEvent = dynamic_cast<ChannelPressureEvent*>(e);
                if (channelPressureEvent) {
                    int newValue = channelPressureEvent->value() - 10;
                    if (newValue >= 0) channelPressureEvent->setValue(newValue);
                }

                TempoChangeEvent* tempoChangeEvent = dynamic_cast<TempoChangeEvent*>(e);
                if (tempoChangeEvent) {
                    int newBeatsPerQuarter = tempoChangeEvent->beatsPerQuarter() - accelleratedIncrement;
                    if (newBeatsPerQuarter >= 0) tempoChangeEvent->setBeats(newBeatsPerQuarter);
                }
            }

            protocol->endAction();
        }
    }
}

void ValueTweakTarget::largeIncrease()
{
    int accelleratedIncrement = getAccelleratedIncrement() * 10;
    MidiFile* file = mainWindow->getFile();

    if (file) {
        QList<MidiEvent*> selectedEvents = Selection::instance()->selectedEvents();

        if (selectedEvents.size() > 0) {
            Protocol* protocol = file->protocol();
            protocol->startNewAction("Tweak");

            foreach (MidiEvent* e, selectedEvents) {
                NoteOnEvent* noteOnEvent = dynamic_cast<NoteOnEvent*>(e);
                if (noteOnEvent) {
                    int newVelocity = noteOnEvent->velocity() + 10;
                    if (newVelocity < 128) noteOnEvent->setVelocity(newVelocity);
                }

                ControlChangeEvent* controlChangeEvent = dynamic_cast<ControlChangeEvent*>(e);
                if (controlChangeEvent) {
                    int newValue = controlChangeEvent->value() + 10;
                    if (newValue < 128) controlChangeEvent->setValue(newValue);
                }

                PitchBendEvent* pitchBendEvent = dynamic_cast<PitchBendEvent*>(e);
                if (pitchBendEvent) {
                    int newValue = pitchBendEvent->value() + accelleratedIncrement;
                    if (newValue < (1 << 14)) pitchBendEvent->setValue(newValue);
                }

                KeyPressureEvent* keyPressureEvent = dynamic_cast<KeyPressureEvent*>(e);
                if (keyPressureEvent) {
                    int newValue = keyPressureEvent->value() + 10;
                    if (newValue < 128) keyPressureEvent->setValue(newValue);
                }

                ChannelPressureEvent* channelPressureEvent = dynamic_cast<ChannelPressureEvent*>(e);
                if (channelPressureEvent) {
                    int newValue = channelPressureEvent->value() + 10;
                    if (newValue < 128) channelPressureEvent->setValue(newValue);
                }

                TempoChangeEvent* tempoChangeEvent = dynamic_cast<TempoChangeEvent*>(e);
                if (tempoChangeEvent) {
                    int newBeatsPerQuarter = tempoChangeEvent->beatsPerQuarter() + accelleratedIncrement;
                    tempoChangeEvent->setBeats(newBeatsPerQuarter);
                }
            }

            protocol->endAction();
        }
    }
}
