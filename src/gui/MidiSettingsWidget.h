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

#ifndef MIDISETTINGSDIALOG_H_
#define MIDISETTINGSDIALOG_H_

#include "SettingsWidget.h"

class QWidget;
class QListWidget;
class QListWidgetItem;
class QLineEdit;
class QCheckBox;
class QSpinBox;
class QSettings;

class AdditionalMidiSettingsWidget : public SettingsWidget {

    Q_OBJECT

public:
    AdditionalMidiSettingsWidget(QSettings* settings, QWidget* parent = 0);
    bool accept();

public slots:
    void manualModeToggled(bool enable);
    void setDefaultTimePerQuarter(int value);
    void setMetronomeLoudness(int value);
private:
    QCheckBox* _alternativePlayerModeBox;
    QSettings* _settings;
    QLineEdit* startCmd;
    QSpinBox* _tpqBox;
    QSpinBox* _metronomeLoudnessBox;
};

class MidiSettingsWidget : public SettingsWidget {

    Q_OBJECT

public:
    MidiSettingsWidget(QWidget* parent = 0);

public slots:

    void reloadInputPorts();
    void reloadOutputPorts();
    void inputChanged(QListWidgetItem* item);
    void outputChanged(QListWidgetItem* item);

private:
    QStringList *_inputPorts, *_outputPorts;
    QListWidget *_inList, *_outList;
};

#endif
