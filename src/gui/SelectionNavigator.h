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

#ifndef SELECTIONNAVIGATOR_H_
#define SELECTIONNAVIGATOR_H_

class MainWindow;

class SelectionNavigator {
public:
    SelectionNavigator(MainWindow* mainWindow);
    void up();
    void down();
    void left();
    void right();

protected:
    MainWindow* mainWindow;

    void navigate(qreal searchAngle);
    MidiEvent* getFirstSelectedEvent();
    bool eventIsInVisibleTimeRange(MidiEvent* event);
    bool eventsAreSameType(MidiEvent* event1, MidiEvent* event2);
    qreal getDisplayDistanceWeightedByDirection(MidiEvent* originEvent, MidiEvent* targetEvent, qreal searchAngle);
};

#endif
