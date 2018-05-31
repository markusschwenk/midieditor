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

#include "TransposeDialog.h"

#include <QButtonGroup>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

#include "../MidiEvent/NoteOnEvent.h"
#include "../midi/MidiFile.h"
#include "../protocol/Protocol.h"

TransposeDialog::TransposeDialog(QList<NoteOnEvent*> toTranspose, MidiFile* file, QWidget* parent)
{

    QLabel* text = new QLabel("Number of semitones: ", this);
    _valueBox = new QSpinBox(this);
    _valueBox->setMinimum(0);
    _valueBox->setMaximum(2147483647);
    _valueBox->setValue(0);

    QButtonGroup* group = new QButtonGroup();
    _up = new QRadioButton("up", this);
    _down = new QRadioButton("down", this);
    _up->setChecked(true);
    group->addButton(_up);
    group->addButton(_down);

    QPushButton* breakButton = new QPushButton("Cancel");
    connect(breakButton, SIGNAL(clicked()), this, SLOT(hide()));
    QPushButton* acceptButton = new QPushButton("Accept");
    connect(acceptButton, SIGNAL(clicked()), this, SLOT(accept()));

    QGridLayout* layout = new QGridLayout(this);
    layout->addWidget(text, 0, 0, 1, 1);
    layout->addWidget(_valueBox, 0, 1, 1, 2);
    layout->addWidget(_up, 1, 0, 1, 1);
    layout->addWidget(_down, 1, 2, 1, 1);
    layout->addWidget(breakButton, 2, 0, 1, 1);
    layout->addWidget(acceptButton, 2, 2, 1, 1);
    layout->setColumnStretch(1, 1);

    _valueBox->setFocus();
    _toTranspose = toTranspose;
    _file = file;
}

void TransposeDialog::accept()
{

    _file->protocol()->startNewAction("Transpose selection");

    int num = _valueBox->value();
    if (_down->isChecked()) {
        num *= -1;
    }
    foreach (NoteOnEvent* onEvent, _toTranspose) {
        int oldVal = onEvent->note();

        if (oldVal + num >= 0 && oldVal + num < 128) {
            onEvent->setNote(oldVal + num);
        }
    }
    hide();

    _file->protocol()->endAction();
}
