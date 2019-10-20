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

#include "RecordDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>

#include "../MidiEvent/ChannelPressureEvent.h"
#include "../MidiEvent/ControlChangeEvent.h"
#include "../MidiEvent/KeyPressureEvent.h"
#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/NoteOnEvent.h"
#include "../MidiEvent/OffEvent.h"
#include "../MidiEvent/OnEvent.h"
#include "../MidiEvent/ProgChangeEvent.h"
#include "../MidiEvent/TempoChangeEvent.h"
#include "../MidiEvent/TextEvent.h"
#include "../MidiEvent/TimeSignatureEvent.h"
#include "../MidiEvent/SysExEvent.h"
#include "../MidiEvent/UnknownEvent.h"
#include "../midi/MidiChannel.h"
#include "../midi/MidiFile.h"
#include "../midi/MidiFile.h"
#include "../midi/MidiTrack.h"
#include "../protocol/Protocol.h"
#include "../tool/NewNoteTool.h"

RecordDialog::RecordDialog(MidiFile* file, QMultiMap<int, MidiEvent*> data, QSettings* settings,
    QWidget* parent)
    : QDialog(parent)
{
    _data = data;
    _file = file;
    _settings = settings;

    QGridLayout* layout = new QGridLayout(this);
    setLayout(layout);

    setWindowTitle("Add " + QString::number(data.size()) + " recorded Events");
    // track
    QLabel* tracklabel = new QLabel("Add to track: ", this);
    layout->addWidget(tracklabel, 1, 0, 1, 1);
    _trackBox = new QComboBox(this);
    _trackBox->addItem("Same as selected for new events");
    for (int i = 0; i < _file->numTracks(); i++) {
        _trackBox->addItem("Track " + QString::number(i) + ": " + _file->track(i)->name());
    }
    layout->addWidget(_trackBox, 1, 1, 1, 3);
    int oldTrack = _settings->value("record_track_index", 0).toInt();
    if (oldTrack >= file->numTracks() + 1) {
        oldTrack = 0;
    }
    _trackBox->setCurrentIndex(oldTrack);

    // channel
    QLabel* channellabel = new QLabel("Add tochannel: ", this);
    layout->addWidget(channellabel, 2, 0, 1, 1);
    _channelBox = new QComboBox(this);
    _channelBox->addItem("Same as selected for new events");
    _channelBox->addItem("Keep channel");
    for (int i = 0; i < 16; i++) {
        _channelBox->addItem("Channel " + QString::number(i));
    }
    _channelBox->setCurrentIndex(_settings->value("record_channel_index", 0).toInt());

    layout->addWidget(_channelBox, 2, 1, 1, 3);

    // ignore types
    QLabel* ignorelabel = new QLabel("Select events to add:", this);
    layout->addWidget(ignorelabel, 3, 0, 1, 4);

    addTypes = new QListWidget(this);
    addListItem(addTypes, "Note on/off Events", 0, true);
    addListItem(addTypes, "Control Change Events", MidiEvent::CONTROLLER_LINE, true);
    addListItem(addTypes, "Pitch Bend Events", MidiEvent::PITCH_BEND_LINE, true);
    addListItem(addTypes, "Channel Pressure Events", MidiEvent::CHANNEL_PRESSURE_LINE, true);
    addListItem(addTypes, "Key Pressure Events", MidiEvent::KEY_PRESSURE_LINE, true);
    addListItem(addTypes, "Program Change Events", MidiEvent::PROG_CHANGE_LINE, true);
    addListItem(addTypes, "System Exclusive Events", MidiEvent::SYSEX_LINE, false);
    addListItem(addTypes, "Tempo Change Events", MidiEvent::TEMPO_CHANGE_EVENT_LINE, false);
    addListItem(addTypes, "Time Signature Events", MidiEvent::TIME_SIGNATURE_EVENT_LINE, false);
    addListItem(addTypes, "Key Signature Events", MidiEvent::KEY_SIGNATURE_EVENT_LINE, false);
    addListItem(addTypes, "Text Events", MidiEvent::TEXT_EVENT_LINE, false);
    addListItem(addTypes, "Unknown Events", MidiEvent::UNKNOWN_LINE, false);
    layout->addWidget(addTypes, 12, 0, 1, 4);

    // buttons
    QPushButton* cancel = new QPushButton("Cancel", this);
    layout->addWidget(cancel, 13, 0, 1, 2);
    connect(cancel, SIGNAL(clicked()), this, SLOT(cancel()));

    QPushButton* ok = new QPushButton("Ok", this);
    layout->addWidget(ok, 13, 2, 1, 2);
    connect(ok, SIGNAL(clicked()), this, SLOT(enter()));
}

void RecordDialog::enter()
{

    int channel = _channelBox->currentIndex();
    bool ownChannel = false;
    if (channel < 2) {
        if (channel == 0) {
            channel = NewNoteTool::editChannel();
        } else {
            ownChannel = true;
        }
    } else {
        channel = channel - 2;
    }

    MidiTrack* track = 0;
    int trackIndex = _trackBox->currentIndex();
    if (trackIndex == 0) {
        track = _file->track(NewNoteTool::editTrack());
    } else {
        track = _file->track(trackIndex - 1);
    }
    if (!track) {
        track = _file->tracks()->last();
    }
    _settings->setValue("record_channel_index", _channelBox->currentIndex());
    _settings->setValue("record_track_index", _trackBox->currentIndex());

    // ignore events
    QList<int> ignoredLines;
    for (int i = 0; i < addTypes->count(); i++) {
        QListWidgetItem* item = addTypes->item(i);
        if (item->checkState() == Qt::Unchecked) {
            int line = item->data(Qt::UserRole).toInt();
            ignoredLines.append(line);
        }
    }

    if (_data.size() > 0) {
        _file->protocol()->startNewAction("Added recorded events");

        // first enlarge the file ( last event + 1000 ms)
        QMultiMap<int, MidiEvent*>::iterator it = _data.end();
        it--;
        int minLength = it.key() + 1000;
        if (minLength > _file->maxTime()) {
            _file->setMaxLengthMs(minLength);
        }

        it = _data.begin();
        while (it != _data.end()) {

            int currentChannel = it.value()->channel();
            if (!ownChannel) {
                currentChannel = channel;
            }

            // check whether to add event or not
            MidiEvent* toCheck = it.value();

            OffEvent* off = dynamic_cast<OffEvent*>(toCheck);
            if (off) {
                toCheck = off->onEvent();
            }

            // note event
            int l = toCheck->line();
            if (l < 128) {
                l = 0;
            }

            bool ignoreEvent = ignoredLines.contains(l);

            // set channels
            TempoChangeEvent* tempo = dynamic_cast<TempoChangeEvent*>(toCheck);
            if (tempo) {
                currentChannel = 17;
            }

            TimeSignatureEvent* time = dynamic_cast<TimeSignatureEvent*>(toCheck);
            if (time) {
                currentChannel = 18;
            }

            TextEvent* text = dynamic_cast<TextEvent*>(toCheck);
            if (text) {
                currentChannel = 16;
            }

            SysExEvent *sysEx = dynamic_cast<SysExEvent*>(toCheck);
            if (sysEx) {
                currentChannel = 16;
            }

            if (!ignoreEvent) {
                MidiEvent* toAdd = it.value();
                toAdd->setFile(_file);
                toAdd->setChannel(currentChannel, false);
                toAdd->setTrack(track, false);
                _file->channel(toAdd->channel())->insertEvent(toAdd, _file->tick(it.key()));
            }
            it++;
        }
        _file->protocol()->endAction();
    }
    hide();
}

void RecordDialog::cancel()
{

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Cancel?");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setText("Do you really want to cancel? The recorded events will be lost.");
    QPushButton* connectButton = msgBox.addButton(tr("Yes"),
        QMessageBox::ActionRole);
    msgBox.addButton(tr("No"), QMessageBox::ActionRole);

    msgBox.exec();

    if (msgBox.clickedButton() == connectButton) {
        // delete events
        foreach (MidiEvent* event, _data) {
            delete event;
        }
        hide();
    }
}

void RecordDialog::addListItem(QListWidget* w, QString title, int line, bool enabled)
{
    QListWidgetItem* item = new QListWidgetItem(w);
    item->setText(title);
    QVariant v(line);
    item->setData(Qt::UserRole, v);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    if (enabled) {
        item->setCheckState(Qt::Checked);
    } else {
        item->setCheckState(Qt::Unchecked);
    }
    w->addItem(item);
}
