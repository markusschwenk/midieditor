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

#ifndef NOSOUNDDIALOG_H
#define NOSOUNDDIALOG_H

#include <QDialog>
#include <QObject>
#include <QWidget>

class CompleteMidiSetupDialog : public QDialog {

    Q_OBJECT

public:
    CompleteMidiSetupDialog(QWidget* parent, bool alertAboutInput, bool alertAboutOutput);
};

#endif
