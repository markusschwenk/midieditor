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

#include "PaintWidget.h"

PaintWidget::PaintWidget(QWidget* parent)
    : QWidget(parent)
{
    this->setMouseTracking(true);
    this->mouseOver = false;
    this->mousePressed = false;
    this->mouseReleased = false;
    this->repaintOnMouseMove = false;
    this->repaintOnMousePress = false;
    this->repaintOnMouseRelease = false;
    this->inDrag = false;
    this->mousePinned = false;
    this->mouseX = 0;
    this->mouseY = 0;
    this->mouseLastY = 0;
    this->mouseLastX = 0;
    this->enabled = true;
}

void PaintWidget::mouseMoveEvent(QMouseEvent* event)
{

    this->mouseOver = true;

    if (mousePinned) {
        // do not change mousePosition but lastMousePosition to get the
        // correct move distance
        QCursor::setPos(mapToGlobal(QPoint(mouseX, mouseY)));
        mouseLastX = 2 * mouseX - event->x();
        mouseLastY = 2 * mouseY - event->y();
    } else {
        this->mouseLastX = this->mouseX;
        this->mouseLastY = this->mouseY;
        this->mouseX = event->x();
        this->mouseY = event->y();
    }
    if (mousePressed) {
        inDrag = true;
    }

    if (!enabled) {
        return;
    }

    if (this->repaintOnMouseMove) {
        this->update();
    }
}

void PaintWidget::enterEvent(QEvent* event)
{

    this->mouseOver = true;

    if (!enabled) {
        return;
    }

    update();
}

void PaintWidget::leaveEvent(QEvent* event)
{
    this->mouseOver = false;

    if (!enabled) {
        return;
    }

    update();
}

void PaintWidget::mousePressEvent(QMouseEvent* event)
{

    this->mousePressed = true;
    this->mouseReleased = false;

    if (!enabled) {
        return;
    }

    if (this->repaintOnMousePress) {
        this->update();
    }
}

void PaintWidget::mouseReleaseEvent(QMouseEvent* event)
{

    this->inDrag = false;
    this->mouseReleased = true;
    this->mousePressed = false;

    if (!enabled) {
        return;
    }

    if (this->repaintOnMouseRelease) {
        this->update();
    }
}

bool PaintWidget::mouseInRect(int x, int y, int width, int height)
{
    return mouseBetween(x, y, x + width, y + height);
}

bool PaintWidget::mouseInRect(QRectF rect)
{
    return mouseInRect(rect.x(), rect.y(), rect.width(), rect.height());
}

bool PaintWidget::mouseBetween(int x1, int y1, int x2, int y2)
{
    int temp;
    if (x1 > x2) {
        temp = x1;
        x1 = x2;
        x2 = temp;
    }
    if (y1 > y2) {
        temp = y1;
        y1 = y2;
        y2 = temp;
    }
    return mouseOver && mouseX >= x1 && mouseX <= x2 && mouseY >= y1 && mouseY <= y2;
}

int PaintWidget::draggedX()
{
    if (!inDrag) {
        return 0;
    }
    int i = mouseX - mouseLastX;
    mouseLastX = mouseX;
    return i;
}

int PaintWidget::draggedY()
{
    if (!inDrag) {
        return 0;
    }
    int i = mouseY - mouseLastY;
    mouseLastY = mouseY;
    return i;
}

void PaintWidget::setRepaintOnMouseMove(bool b)
{
    repaintOnMouseMove = b;
}

void PaintWidget::setRepaintOnMousePress(bool b)
{
    repaintOnMousePress = b;
}

void PaintWidget::setRepaintOnMouseRelease(bool b)
{
    repaintOnMouseRelease = b;
}

void PaintWidget::setEnabled(bool b)
{
    enabled = b;
    setMouseTracking(enabled);
    update();
}
