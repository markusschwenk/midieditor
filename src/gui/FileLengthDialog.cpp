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

#include "FileLengthDialog.h"

#include "../midi/MidiFile.h"
#include "../protocol/Protocol.h"
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>

FileLengthDialog::FileLengthDialog(MidiFile* f, QWidget* parent)
    : QDialog(parent)
{
    _file = f;
    QLabel* text = new QLabel("File duration (ms): ", this);
    _box = new QSpinBox(this);
    _box->setMinimum(1);
    _box->setMaximum(2147483647);
    _box->setValue(_file->maxTime());
    QPushButton* breakButton = new QPushButton("Cancel");
    connect(breakButton, SIGNAL(clicked()), this, SLOT(hide()));
    QPushButton* acceptButton = new QPushButton("Accept");
    connect(acceptButton, SIGNAL(clicked()), this, SLOT(accept()));

    QGridLayout* layout = new QGridLayout(this);
    layout->addWidget(text, 0, 0, 1, 1);
    layout->addWidget(_box, 0, 1, 1, 2);
    layout->addWidget(breakButton, 1, 0, 1, 1);
    layout->addWidget(acceptButton, 1, 2, 1, 1);
    layout->setColumnStretch(1, 1);

    _box->setFocus();
    setWindowTitle("File duration");
}

void FileLengthDialog::accept()
{
    _file->protocol()->startNewAction("Change file duration");
    _file->setMaxLengthMs(_box->value());
    _file->protocol()->endAction();
    hide();
}
