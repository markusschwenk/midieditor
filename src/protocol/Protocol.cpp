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
#include "../tool/EventTool.h"
#include "../tool/NewNoteTool.h"
#include "ProtocolItem.h"
#include "ProtocolStep.h"

static bool _limit_undo = true;

Protocol::Protocol(MidiFile* f)
{

    _currentStep = 0;
    number = 1;
    number_saved = 0;
    currentTrackDeleted = false;

    _file = f;

    _undoSteps = new QList<ProtocolStep*>;
    _redoSteps = new QList<ProtocolStep*>;
}


void Protocol::clean(bool forced) {

    _currentStep = 0;
    currentTrackDeleted = false;

    for(int n = 0; n < _undoSteps->count(); n++)  {
        if(_undoSteps->at(n))
            delete _undoSteps->at(n);
    }

    for(int n = 0; n < _redoSteps->count(); n++)  {
        if(_redoSteps->at(n))
            delete _redoSteps->at(n);
    }

    _undoSteps->clear();
    _redoSteps->clear();

    if(forced)
        return;

    addEmptyAction("Reset Undo/Redo list");

    emit protocolChanged();
    emit actionFinished();

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
    int ntrack = NewNoteTool::editTrack();
    NewNoteTool::setEditTrack(0); // prevent deleted track

    bool file_modified = false;
    bool magnet = EventTool::magnetEnabled();
    EventTool::enableMagnet(false);

    if (!_undoSteps->empty()) {

        // Take last undoStep from the Stack
        ProtocolStep* step = _undoSteps->last();
        _undoSteps->removeLast();

        if(step) {

            file_modified = step->midi_modified;

            // release it and copy it to the redo Stack
            ProtocolStep* redoAction = step->releaseStep();
            if (redoAction) {

                redoAction->midi_modified = file_modified;
                number = step->number;

                _redoSteps->append(redoAction);

                if(file_modified)
                    save_description = "(-) " + redoAction->description();
                if(file_modified)
                    _file->setSaved(false);
                else
                    _file->setSaved(true);
            }

            // delete the old Step
            delete step;
        }

        if(ntrack >= _file->numTracks()) {
            currentTrackDeleted = true;
            if(_file->numTracks() > 1)
                NewNoteTool::setEditTrack(1);
            else
                NewNoteTool::setEditTrack(0);
        } else
            NewNoteTool::setEditTrack(ntrack);

        EventTool::enableMagnet(magnet);

        if (emitChanged) {
            emit protocolChanged();
            emit actionFinished();
        }
    }


}

void Protocol::redo(bool emitChanged)
{

    bool file_modified = false;

    bool magnet = EventTool::magnetEnabled(); // save magnet flag
    EventTool::enableMagnet(false);

    if (!_redoSteps->empty()) {

        // Take last redoSteo from the Stack
        ProtocolStep* step = _redoSteps->last();
        _redoSteps->removeLast();

        if(step) {

            file_modified = step->midi_modified;

            // release it and copy it to the undoStack
            ProtocolStep* undoAction = step->releaseStep();
            if (undoAction) {

                undoAction->midi_modified = file_modified;

                _undoSteps->append(undoAction);

                if(file_modified)
                    save_description = "(+) " + undoAction->description();
                if(file_modified)
                    _file->setSaved(false);
                else
                    _file->setSaved(true);

                if(_limit_undo && _undoSteps->count() > 64) {

                    if(!_undoSteps->isEmpty()) _undoSteps->pop_front();

                }
            }

            // delete the old Step
            delete step;
        }

        EventTool::enableMagnet(magnet); // restore magnet flag

        if(NewNoteTool::editTrack() >= _file->numTracks()) {
            currentTrackDeleted = true;
            if(_file->numTracks() > 1)
                NewNoteTool::setEditTrack(1);
            else
                NewNoteTool::setEditTrack(0);
        }

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
    _currentStep->number = number;

}

void Protocol::changeDescription(QString description) {
    if (_currentStep)
        _currentStep->setDescription(description);
}


void Protocol::limitUndoAction(bool limit) {
    _limit_undo = limit;
}

void Protocol::endAction()
{

    bool step_added = false;
    bool file_modified = false;
    // only create the Step when it exists and its size is bigger 0
    if (_currentStep && _currentStep->items() > 0) {

        file_modified = _currentStep->midi_modified;
        if(file_modified) {
            save_description = "(+) " + _currentStep->description();
        }

        number++;
        _currentStep->number = number;

        _undoSteps->append(_currentStep);

        step_added = true;
    }

    if(!_currentStep) return;

    // the action is ended so there is no currentStep
    _currentStep = 0;

    // the file has been changed
    if(file_modified) {

        _file->setSaved(false);
    } else
        _file->setSaved(true);

    if(_limit_undo && step_added && _undoSteps->count() > 64) {

        if(!_undoSteps->isEmpty()) _undoSteps->pop_front();

    }

    emit protocolChanged();
    emit actionFinished();

}

bool Protocol::testFileModified() {

    bool ret = false;

    if(number < number_saved)
        return true;

    for(int n = 0; n < _undoSteps->count(); n++) {

        if(_undoSteps->at(n)->number > number_saved && _undoSteps->at(n)->midi_modified) {
            ret = true;
            break;
        }
    }


    return ret;
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
    if(!_undoSteps || i < 0 || i >= _undoSteps->count())
        return NULL;

    return _undoSteps->at(i);
}

ProtocolStep* Protocol::redoStep(int i)
{
    if(!_redoSteps || i < 0 || i >= _redoSteps->count())
        return NULL;

    return _redoSteps->at(i);
}

void Protocol::goTo(ProtocolStep* toGo)
{

    QString description;
    bool file_modified = false;

    if (_undoSteps->contains(toGo)) {

        // do undo() until toGo is the last Step on the undoStack
        while (_undoSteps->last() != toGo && _undoSteps->size() > 1) {
            undo(false);
            if(!_file->saved()) {
                description = "Goto " + save_description;
                file_modified = true;
            }
        }

    } else if (_redoSteps->contains(toGo)) {

        // do redo() until toGo is the last Step on the undoStep
        while (_redoSteps->contains(toGo)) {
            redo(false);
            if(!_file->saved()) {
                description = "Goto " + save_description;
                file_modified = true;
            }
        }
    }

    save_description = description;
    if(file_modified)
        _file->setSaved(false);
    else
        _file->setSaved(true);

    emit protocolChanged();
    emit actionFinished();
}

void Protocol::addEmptyAction(QString name)
{
    _undoSteps->append(new ProtocolStep(name));
    if(_limit_undo && _undoSteps->count() > 64) {

        if(!_undoSteps->isEmpty()) _undoSteps->pop_front();

    }
}
