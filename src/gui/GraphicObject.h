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

#ifndef GRAPHICOBJECT_H_
#define GRAPHICOBJECT_H_

#include <QColor>
#include <QPainter>

class GraphicObject {

public:
    GraphicObject();

    int x();
    int y();
    int width();
    int height();

    void setX(int x);
    void setY(int y);
    void setWidth(int w);
    void setHeight(int h);

    virtual void draw(QPainter* p, QColor c);

    bool shown();
    void setShown(bool b);

private:
    int _x, _y, _width, _height;
    bool shownInWidget;
};

#endif
