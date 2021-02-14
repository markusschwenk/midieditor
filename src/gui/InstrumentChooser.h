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

#ifndef INSTRUMENTCHOOSER_H_
#define INSTRUMENTCHOOSER_H_

#include <QDialog>

#include "MainWindow.h"

class MidiFile;
class QComboBox;
class QCheckBox;

class InstrumentChooser : public QDialog {
    Q_OBJECT
public:
    InstrumentChooser(MidiFile* f, int channel, QWidget* parent = 0, int mode=0, int ticks=0, int instrument=0, int bank=0);

public slots:
    void accept();
    void setInstrument(int index);
    void setBank(int index);
    void PlayTest();
    void PlayTestOff();

private:
    MidiFile* _file;
    QComboBox* _box;
    QComboBox* _box2;
    QCheckBox* _removeOthers;
    int _channel;

    int _mode;
    int _current_tick;
    int _instrument;
    int _bank;

};

#endif
