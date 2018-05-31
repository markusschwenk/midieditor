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

#include "GraphicObject.h"

GraphicObject::GraphicObject()
{
    _x = 0;
    _y = 0;
    _width = 0;
    _height = 0;
}

int GraphicObject::x()
{
    return _x;
}

int GraphicObject::y()
{
    return _y;
}

int GraphicObject::width()
{
    return _width;
}

int GraphicObject::height()
{
    return _height;
}

void GraphicObject::setX(int x)
{
    _x = x;
}

void GraphicObject::setY(int y)
{
    _y = y;
}

void GraphicObject::setWidth(int w)
{
    _width = w;
}

void GraphicObject::setHeight(int h)
{
    _height = h;
}

void GraphicObject::draw(QPainter* p, QColor c)
{
    return;
}

bool GraphicObject::shown()
{
    return shownInWidget;
}

void GraphicObject::setShown(bool b)
{
    shownInWidget = b;
}
