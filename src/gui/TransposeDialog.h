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

#ifndef TRANSPOSEDIALOG_H_
#define TRANSPOSEDIALOG_H_

#include <QDialog>
#include <QList>
#include <QRadioButton>
#include <QSpinBox>

class NoteOnEvent;
class MidiFile;

class TransposeDialog : public QDialog {

public:
    TransposeDialog(QList<NoteOnEvent*> toTranspose, MidiFile* file, QWidget* parent = 0);

public slots:
    void accept();

private:
    QList<NoteOnEvent*> _toTranspose;
    QSpinBox* _valueBox;
    QRadioButton *_up, *_down;
    MidiFile* _file;
};

#endif
