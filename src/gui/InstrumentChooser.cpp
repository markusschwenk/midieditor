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

#include "InstrumentChooser.h"
#include <QMessageBox>

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/ProgChangeEvent.h"
#include "../MidiEvent/ControlChangeEvent.h"
#include "../midi/MidiChannel.h"
#include "../midi/MidiFile.h"
#include "../midi/MidiOutput.h"
#include "../protocol/Protocol.h"
#include "../MidiEvent/NoteOnEvent.h"

#include <QThread>

extern int itHaveInstrumentList;

InstrumentChooser::InstrumentChooser(MidiFile* f, int channel, QWidget* parent, int mode, int ticks, int instrument, int bank)
    : QDialog(parent)
{

    _file = f;
    _channel = channel;

    _mode= mode;
    _bank =bank;
    _instrument = instrument;

    if(_file) {
        if(!mode) _current_tick= _file->cursorTick(); else _current_tick = ticks;
        _current_tick=(_current_tick/10)*10;
    } else _current_tick = 0;

    if(!mode) {
        _bank = Bank_MIDI[_channel];
        _instrument = Prog_MIDI[_channel];

    }

    QLabel* starttext = new QLabel("Choose Instrument for Channel " + QString::number(channel), this);

    QLabel* text = new QLabel("Instrument: ", this);
    _box = new QComboBox(this);
    for (int i = 0; i < 128; i++) {
       if(_channel!=9) _box->addItem(MidiFile::instrumentName( _bank, i));
       else _box->addItem(MidiFile::drumName(i));
    }
    _box->setCurrentIndex(_instrument);

    QLabel* endText;
    if(!mode) endText = new QLabel("<b>Warning:</b> this will edit the event at tick 0 of the file."
                                 "<br>If there is a Program Change Event after this tick,"
                                 "<br>the instrument selected there will be audible!"
                                 "<br>If you want all other Program Change Events to be"
                                 "<br>removed, select the box below.");
    else endText = new QLabel(" ");

    if(!mode) _removeOthers = new QCheckBox("Remove other Program Change Events", this);
    else _removeOthers = new QCheckBox("Remove this Program Change Event", this);

    QLabel* text2 = new QLabel("Bank: ", this);
    _box2 = new QComboBox(this);
    if(_channel!=9) {
        for (int i = 0; i < 128; i++) {
            QString text = "undefined";

            int n = 0;
            for (; n < 128; n++)
                if (text.compare(InstrumentList[i].name[n])) break;
            if(n!=128 || i==0 || !itHaveInstrumentList) // bank 0 is always by default
                _box2->addItem(QIcon(":/run_environment/graphics/channelwidget/instrument.png"),
                               QString::asprintf("%3.3u", i));
            else _box2->addItem(QIcon(":/run_environment/graphics/channelwidget/noinstrument.png"),
                                QString::asprintf("%3.3u", i));
        }
        _box2->setCurrentIndex(/*(!mode) ? Bank_MIDI[_channel] :*/ _bank);
    } else {
        _box2->addItem(QString::asprintf("%3.3u", 0));
        _box2->setCurrentIndex(0);
        _box2->setDisabled(true);
    }

    QPushButton* breakButton = new QPushButton("Cancel");
    connect(breakButton, SIGNAL(clicked()), this, SLOT(hide()));
    QPushButton* acceptButton = new QPushButton("Accept");
    connect(acceptButton, SIGNAL(clicked()), this, SLOT(accept()));
    QPushButton* playButton = new QPushButton("Play");
    connect(playButton, SIGNAL(pressed()), this, SLOT(PlayTest()));
    connect(playButton, SIGNAL(released()), this, SLOT(PlayTestOff()));



    connect(_box, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &InstrumentChooser::setInstrument);
    connect(_box2, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &InstrumentChooser::setBank);

    QGridLayout* layout = new QGridLayout(this);
    layout->addWidget(starttext, 0, 0, 1, 3);
    layout->addWidget(text, 1, 0, 1, 1);
    layout->addWidget(_box, 1, 1, 1, 2);
    layout->addWidget(text2, 2, 0, 1, 1);
    layout->addWidget(_box2, 2, 1, 1, 2);
    layout->addWidget(endText, 3, 1, 1, 2);
    layout->addWidget(_removeOthers, 4, 1, 1, 2);
    layout->addWidget(breakButton, 5, 0, 1, 1);
    layout->addWidget(acceptButton, 5, 2, 1, 1);
    layout->addWidget(playButton, 3, 0, 1, 1);
    layout->setColumnStretch(1, 1);


}


void InstrumentChooser::PlayTestOff()
{
    QByteArray offMessage;

    offMessage.clear();
    MidiOutput::sendCommand(offMessage);

    QByteArray cevent = QByteArray();
    cevent.append(0xB0 | _channel);
    cevent.append((char) 123);
    cevent.append((char) 127);

    MidiOutput::sendCommand(cevent);

}


void InstrumentChooser::PlayTest()
{

    QByteArray offMessage;

    QByteArray pevent = QByteArray();
    pevent.append(0xC0 | _channel);
    pevent.append(_instrument);

    QByteArray cevent = QByteArray();
    cevent.append(0xB0 | _channel);
    cevent.append((char) 0);
    cevent.append(_bank);

    QByteArray NoteOn = QByteArray();
    NoteOn.append(0x90 | _channel);
    NoteOn.append((char) 60);
    NoteOn.append((char) 100);

    offMessage.clear();
    MidiOutput::sendCommand(offMessage);

    MidiOutput::sendCommand(cevent);
    MidiOutput::sendCommand(pevent);
    MidiOutput::sendCommand(NoteOn);
    QThread::msleep(50);

}

void InstrumentChooser::setInstrument(int index)
{
    if(index < 128 && _channel < 16){
        _instrument=index;
    }

}


void InstrumentChooser::setBank(int index)
{
    if(index < 128 && _channel < 16 && _channel!=9){

        (!_mode) ? _bank = index: _bank= index;
        _box->clear();
        for (int i = 0; i < 128; i++) {
           if(_channel!=9) _box->addItem(MidiFile::instrumentName( _bank, i));
           else _box->addItem(MidiFile::drumName(i));
        }
        _box->setCurrentIndex(_instrument);
    }

}

void InstrumentChooser::accept()
{
    if(!_file) {
        Bank_MIDI[_channel] = _bank;
        Prog_MIDI[_channel] = _instrument;
        hide();
        return;
    }

    int program = _box->currentIndex();
    bool removeOthers = _removeOthers->isChecked();
    MidiTrack* track = 0;

    if(!_mode) {
        Bank_MIDI[_channel] = _bank;
        Prog_MIDI[_channel] = _instrument;
    }

    // get events
    QList<ProgChangeEvent*> events;

    QList<ControlChangeEvent*> events2;

    foreach (MidiEvent* event, _file->channel(_channel)->eventMap()->values()) {

        ControlChangeEvent* ctrl = dynamic_cast<ControlChangeEvent*>(event);
        ProgChangeEvent* prg = dynamic_cast<ProgChangeEvent*>(event);

        if (ctrl && ctrl->control()==0) {
            events2.append(ctrl);
        }
        if (prg) {
            events.append(prg);
            track = prg->track();
        }
    }
    if (!track) {
        track = _file->track(0);
    }

    _file->protocol()->startNewAction("Edited instrument for channel");

    if (removeOthers) {
        if(!_mode) {

            foreach (ProgChangeEvent* toRemove, events) {
                if (toRemove) {
                    _file->channel(_channel)->removeEvent(toRemove);
                }
            }
            foreach (ControlChangeEvent* toRemove, events2) {
                if (toRemove) {
                    _file->channel(_channel)->removeEvent(toRemove);
                }
            }
        } else {
            // deletes upcoming repeating events
            foreach (MidiEvent* event3, *(_file->eventsBetween(_current_tick-10, _current_tick+10))) {
                ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event3);
                if (toRemove && event3->channel()==_channel
                        && toRemove->control()==0x0) {
                    _file->channel(_channel)->removeEvent(toRemove);
                }
            }
            // deletes upcoming repeating events
            foreach (MidiEvent* event3, *(_file->eventsBetween(_current_tick-10, _current_tick+10))) {
                ProgChangeEvent* toRemove = dynamic_cast<ProgChangeEvent*>(event3);
                if (toRemove && event3->channel()==_channel) {
                    _file->channel(_channel)->removeEvent(toRemove);
                }
            }
            _file->protocol()->endAction();
            hide();
            return;
        }
    }

    ProgChangeEvent* event = 0;
    ControlChangeEvent* cevent = 0;

    int ticks=0;
    if (removeOthers) {
        event = new ProgChangeEvent(_channel, program, track);
        cevent = new ControlChangeEvent(_channel, 0x0, _bank, track);
        _file->channel(_channel)->insertEvent(cevent, ticks);
        _file->channel(_channel)->insertEvent(event, ticks);

    } else {
        event = new ProgChangeEvent(_channel, program, track);
        cevent = new ControlChangeEvent(_channel, 0x0, _bank, track);

        ticks= _current_tick;

        int bank = -1;

        if (events.size()==0) ticks=0;
        else if (events.size() > 0 && events.first()->midiTime() != 0){
            int ticks2= events.first()->midiTime();
            int ticks3= ticks2;

            ProgChangeEvent* event3 = 0;
            ControlChangeEvent* cevent3 = 0;

            if(events.first()->midiTime()>=ticks) ticks=0; else ticks3=0;

            event3 = new ProgChangeEvent(_channel, events.first()->program(), track);

            foreach (MidiEvent* event3, *(_file->eventsBetween(ticks2, ticks2+1))) {
                ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event3);
                if (toRemove && event3->channel()==_channel
                        && toRemove->control()==0x0) {
                    cevent3 = new ControlChangeEvent(_channel, 0x0, toRemove->value(), track);
                    break;
                }
            }

            // deletes upcoming repeating events
            foreach (MidiEvent* event3, *(_file->eventsBetween(ticks2-10, ticks2+10))) {
                ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event3);
                if (toRemove && event3->channel()==_channel
                        && toRemove->control()==0x0) {
                    _file->channel(_channel)->removeEvent(toRemove);
                }
            }
            // deletes upcoming repeating events
            foreach (MidiEvent* event3, *(_file->eventsBetween(ticks2-10, ticks2+10))) {
                ProgChangeEvent* toRemove = dynamic_cast<ProgChangeEvent*>(event3);
                if (toRemove && event3->channel()==_channel) {
                    _file->channel(_channel)->removeEvent(toRemove);
                }
            }


            if(!cevent3) cevent3 = new ControlChangeEvent(_channel, 0x0, 0, track);

            // insert news bank/prg events

            if(cevent3) _file->channel(_channel)->insertEvent(cevent3, ticks3);
            if(event3) _file->channel(_channel)->insertEvent(event3, ticks3);


        }


        foreach (ControlChangeEvent* toFind, events2) {
            if (toFind && toFind->control()==0x0) {
                if(toFind->midiTime()<=ticks+10) bank= toFind->value();
            }
        }


        int old_bank=-1;

        // deletes upcoming repeating events
        foreach (MidiEvent* event2, *(_file->eventsBetween(ticks-10, ticks+10))) {
            ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event2);
            if (toRemove && event2->channel()==_channel
                    && toRemove->control()==0x0 /*&& toRemove != cevent*/) {
                //if(toRemove->value() != _bank)
                old_bank=toRemove->value();
                _file->channel(_channel)->removeEvent(toRemove);
            }
        }

        // deletes upcoming repeating events
        foreach (MidiEvent* event2, *(_file->eventsBetween(ticks-10, ticks+10))) {
            ProgChangeEvent* toRemove = dynamic_cast<ProgChangeEvent*>(event2);
            if (toRemove && event2->channel()==_channel && toRemove != event) {
                _file->channel(_channel)->removeEvent(toRemove);
            }
        }

        // insert news bank/prg events
        _file->channel(_channel)->insertEvent(cevent, ticks);
        _file->channel(_channel)->insertEvent(event, ticks);

        // find last bank change
        if(old_bank ==-1) {
            foreach (MidiEvent* event2, *(_file->eventsBetween(0, ticks-10))) {
                ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event2);
                if (toRemove && event2->channel()==_channel && toRemove->control()==0x0) {
                    old_bank=toRemove->value();

                }
            }
        }

        // find next prg event change and add bank event
        if(old_bank !=-1) {

            foreach (MidiEvent* event2, *(_file->eventsBetween(ticks+10, _file->endTick()))) {
                ControlChangeEvent* ctrl = dynamic_cast<ControlChangeEvent*>(event2);
                ProgChangeEvent* prg = dynamic_cast<ProgChangeEvent*>(event2);

                if (ctrl && event2->channel()==_channel && ctrl->control()==0) {

                    break; // have bank event
                }

                // deletes upcoming repeating bank events
                if (prg && event2->channel()==_channel) {
                    int ticks2= event2->midiTime();

                    foreach (MidiEvent* event3, *(_file->eventsBetween(ticks2, ticks2+1))) {
                        ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event3);
                        if (toRemove && event3->channel()==_channel
                                && toRemove->control()==0x0) {
                            ticks2=-1;
                            break;
                        }
                    }
                    if(ticks2==-1) break; // have bank event

                    // deletes upcoming repeating bank events
                    foreach (MidiEvent* event3, *(_file->eventsBetween(ticks2-10, ticks2+10))) {
                        ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event3);
                        if (toRemove && event3->channel()==_channel
                                && toRemove->control()==0x0) {
                            _file->channel(_channel)->removeEvent(toRemove);
                        }
                    }

                    // deletes upcoming repeating prg events
                    foreach (MidiEvent* event3, *(_file->eventsBetween(ticks2-10, ticks2+10))) {
                        ProgChangeEvent* toRemove = dynamic_cast<ProgChangeEvent*>(event3);
                        if (toRemove && event3->channel()==_channel && toRemove->program() == program) {
                            _file->channel(_channel)->removeEvent(toRemove);
                        }
                    }

                    // insert news bank/prg events
                    MidiEvent* cevent = new ControlChangeEvent(_channel, 0x0, old_bank, track);
                  //  MidiEvent* event = new ProgChangeEvent(_channel, program, track);

                    _file->channel(_channel)->insertEvent(cevent, ticks2);
                   // _file->channel(_channel)->insertEvent(event, ticks2);
                    break;

                }

            }
        }

        bank = -1;

        foreach (MidiEvent* event, _file->channel(_channel)->eventMap()->values()) {

            ControlChangeEvent* ctrl = dynamic_cast<ControlChangeEvent*>(event);
            if (ctrl && ctrl->control()==0) {
                if(bank == ctrl->value()) _file->channel(_channel)->removeEvent(ctrl);
                else bank = ctrl->value();
            }
        }

    }

    _file->protocol()->endAction();
    hide();
}
