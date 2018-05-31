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

#include "Protocol.h"

#include <QImage>

#include "../midi/MidiFile.h"
#include "ProtocolItem.h"
#include "ProtocolStep.h"

Protocol::Protocol(MidiFile* f)
{

    _currentStep = 0;

    _file = f;

    _undoSteps = new QList<ProtocolStep*>;
    _redoSteps = new QList<ProtocolStep*>;
}

void Protocol::enterUndoStep(ProtocolItem* item)
{

    if (_currentStep) {
        _currentStep->addItem(item);
    }

    emit protocolChanged();
}

void Protocol::undo(bool emitChanged)
{

    if (!_undoSteps->empty()) {

        // Take last undoStep from the Stack
        ProtocolStep* step = _undoSteps->last();
        _undoSteps->removeLast();

        // release it and copy it to the redo Stack
        ProtocolStep* redoAction = step->releaseStep();
        if (redoAction) {
            _redoSteps->append(redoAction);
        }

        // delete the old Step
        delete step;

        if (emitChanged) {
            emit protocolChanged();
            emit actionFinished();
        }
    }
}

void Protocol::redo(bool emitChanged)
{

    if (!_redoSteps->empty()) {

        // Take last redoSteo from the Stack
        ProtocolStep* step = _redoSteps->last();
        _redoSteps->removeLast();

        // release it and copy it to the undoStack
        ProtocolStep* undoAction = step->releaseStep();
        if (undoAction) {
            _undoSteps->append(undoAction);
        }

        // delete the old Step
        delete step;

        if (emitChanged) {
            emit protocolChanged();
            emit actionFinished();
        }
    }
}

void Protocol::startNewAction(QString description, QImage* img)
{

    // When there is a new Action started the redoStack has to be cleared
    _redoSteps->clear();

    // Any old Action is ended
    endAction();

    // create a new Step
    _currentStep = new ProtocolStep(description, img);
}

void Protocol::endAction()
{

    // only create the Step when it exists and its size is bigger 0
    if (_currentStep && _currentStep->items() > 0) {
        _undoSteps->append(_currentStep);
    }

    // the action is ended so there is no currentStep
    _currentStep = 0;

    // the file has been changed
    _file->setSaved(false);

    emit protocolChanged();
    emit actionFinished();
}

int Protocol::stepsBack()
{
    return _undoSteps->count();
}

int Protocol::stepsForward()
{
    return _redoSteps->count();
}

ProtocolStep* Protocol::undoStep(int i)
{
    return _undoSteps->at(i);
}

ProtocolStep* Protocol::redoStep(int i)
{
    return _redoSteps->at(i);
}

void Protocol::goTo(ProtocolStep* toGo)
{

    if (_undoSteps->contains(toGo)) {

        // do undo() until toGo is the last Step on the undoStack
        while (_undoSteps->last() != toGo && _undoSteps->size() > 1) {
            undo(false);
        }

    } else if (_redoSteps->contains(toGo)) {

        // do redo() until toGo is the last Step on the undoStep
        while (_redoSteps->contains(toGo)) {
            redo(false);
        }
    }

    emit protocolChanged();
    emit actionFinished();
}

void Protocol::addEmptyAction(QString name)
{
    _undoSteps->append(new ProtocolStep(name));
}
