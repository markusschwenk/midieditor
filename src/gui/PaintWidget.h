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

#ifndef PAINTWIDGET_H
#define PAINTWIDGET_H

#include <QCursor>
#include <QEvent>
#include <QMouseEvent>
#include <QPoint>
#include <QWidget>

class PaintWidget : public QWidget {

public:
    PaintWidget(QWidget* parent = 0);
    void setRepaintOnMouseMove(bool b);
    void setRepaintOnMousePress(bool b);
    void setRepaintOnMouseRelease(bool b);
    void setEnabled(bool b);

protected:
    void mouseMoveEvent(QMouseEvent* event);
    void enterEvent(QEvent* event);
    void leaveEvent(QEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);

    int movedX() { return mouseX - mouseLastX; }
    int movedY() { return mouseY - mouseLastY; }
    int draggedX();
    int draggedY();
    bool mouseInRect(int x, int y, int width, int height);
    bool mouseInRect(QRectF rect);
    bool mouseBetween(int x1, int y1, int x2, int y2);

    void setMousePinned(bool b) { mousePinned = b; }

    bool mouseOver, mousePressed, mouseReleased, repaintOnMouseMove,
        repaintOnMousePress, repaintOnMouseRelease, inDrag, mousePinned,
        enabled;
    int mouseX, mouseY, mouseLastX, mouseLastY;
};
#endif
