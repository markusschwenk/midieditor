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

#include "ProtocolStep.h"

#include <QImage>

#include "ProtocolItem.h"

ProtocolStep::ProtocolStep(QString description, QImage* img)
{
    _stepDescription = description;
    _itemStack = new QStack<ProtocolItem*>;
    _image = img;
}

ProtocolStep::~ProtocolStep()
{

    while (!_itemStack->isEmpty()) {
        ProtocolItem* item = _itemStack->pop();
        if(item)
            delete item;
    }

}

void ProtocolStep::addItem(ProtocolItem* item)
{

    if(item->midi_modified) {
        midi_modified = item->midi_modified;
    }


    _itemStack->push(item);
}

ProtocolStep* ProtocolStep::releaseStep()
{

    // create the invere Step
    ProtocolStep* step = new ProtocolStep(_stepDescription, _image);

    // Copy the Single steps and release them
    while (!_itemStack->isEmpty()) {
        ProtocolItem* item = _itemStack->pop();

        if(item->midi_modified) {
            midi_modified = item->midi_modified;
        }

        ProtocolItem* rel = item->release();

        step->addItem(rel);
    }
    return step;
}

int ProtocolStep::items()
{
    return _itemStack->size();
}

QString ProtocolStep::description()
{
    return _stepDescription;
}

void ProtocolStep::setDescription(QString description) {
    _stepDescription = description;
}

QImage* ProtocolStep::image()
{
    return _image;
}
