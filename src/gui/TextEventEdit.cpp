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

#include "TextEventEdit.h"

#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/TextEvent.h"
#include "../midi/MidiChannel.h"
#include "../midi/MidiFile.h"
#include "../protocol/Protocol.h"

TextEventEdit::TextEventEdit(MidiFile* f, int channel, QWidget* parent, int flag)
    : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
    QDialog *MIDITextDialog= this;
    _flag=flag;
    _file = f;
    _channel = channel;
    result=0;

    if (MIDITextDialog->objectName().isEmpty())
        MIDITextDialog->setObjectName(QString::fromUtf8("MIDITextDialog"));
    MIDITextDialog->resize(390, 208);
    buttonBox = new QDialogButtonBox(MIDITextDialog);
    buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
    buttonBox->setGeometry(QRect(180, 150, 181, 32));
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    textEdit = new QLineEdit(MIDITextDialog);
    textEdit->setObjectName(QString::fromUtf8("textEdit"));
    textEdit->setGeometry(QRect(30, 70, 328, 23));
    textEdit->setMaxLength(32);
    if(flag == TEXTEVENT_NEW_MARKER) textEdit->setText("Marker");

    label = new QLabel(MIDITextDialog);
    label->setObjectName(QString::fromUtf8("label"));
    label->setGeometry(QRect(160, 30, 47, 21));
    label->setAlignment(Qt::AlignCenter);
    delBox = new QCheckBox(MIDITextDialog);
    delBox->setObjectName(QString::fromUtf8("delBox"));
    delBox->setGeometry(QRect(30, 160, 70, 16));
    if(flag >= TEXTEVENT_NEW_TEXT) delBox->setDisabled(true);

    if((flag & 127) == TEXTEVENT_EDIT_TEXT)
        MIDITextDialog->setWindowTitle("Text Event Editor");
    else if((flag & 127) == TEXTEVENT_EDIT_MARKER)
        MIDITextDialog->setWindowTitle("Marker Event Editor");
    else if((flag & 127) == TEXTEVENT_EDIT_LYRIK)
        MIDITextDialog->setWindowTitle("Lyrik Event Editor");
    else if((flag & 127) == TEXTEVENT_EDIT_TRACK_NAME)
        MIDITextDialog->setWindowTitle("Marker Track Name Editor");
    else MIDITextDialog->setWindowTitle("General Text Event Editor");

    label->setText("Text");
    delBox->setText("Delete");

    QObject::connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QMetaObject::connectSlotsByName(this);

}

void TextEventEdit::accept()
{

 result=1;
 if(_flag!=TEXTEVENT_NEW_TEXT && _flag!=TEXTEVENT_NEW_MARKER  &&
         _flag!=TEXTEVENT_NEW_LYRIK && _flag!=TEXTEVENT_NEW_TRACK_NAME) {QDialog::accept();return;}
    MidiTrack* track = 0;

    int type = TextEvent::TEXT;
    if(_flag == TEXTEVENT_NEW_MARKER) type = TextEvent::MARKER;
    if(_flag == TEXTEVENT_NEW_LYRIK) type = TextEvent::LYRIK;
    if(_flag == TEXTEVENT_NEW_TRACK_NAME) type = TextEvent::TRACKNAME;

    // get events
    foreach (MidiEvent* event, _file->channel(_channel)->eventMap()->values()) {
        TextEvent* text = dynamic_cast<TextEvent*>(event);
        if (text && text->type()== type) {
            track = text->track(); break;
        }
    }
    if (!track) {
        track = _file->track(0);
    }

    _file->protocol()->startNewAction("Text Event editor");

    TextEvent *event = new TextEvent(_channel, track);
    event->setType(type);
    event->setText(textEdit->text());
    _file->channel(_channel)->insertEvent(event, _file->cursorTick());

    // borra eventos repetidos proximos
    foreach (MidiEvent* event2, *(_file->eventsBetween(_file->cursorTick()-10, _file->cursorTick()+10))) {
        TextEvent* toRemove = dynamic_cast<TextEvent*>(event2);
        if (toRemove && toRemove != event && toRemove->type()==type) {
            _file->channel(_channel)->removeEvent(toRemove);
        }
    }

    _file->protocol()->endAction();

    QDialog::accept();

}



