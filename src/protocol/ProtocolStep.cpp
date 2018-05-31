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
    delete _itemStack;
}

void ProtocolStep::addItem(ProtocolItem* item)
{
    _itemStack->push(item);
}

ProtocolStep* ProtocolStep::releaseStep()
{

    // create the invere Step
    ProtocolStep* step = new ProtocolStep(_stepDescription, _image);

    // Copy the Single steps and release them
    while (!_itemStack->isEmpty()) {
        ProtocolItem* item = _itemStack->pop();
        step->addItem(item->release());
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

QImage* ProtocolStep::image()
{
    return _image;
}
