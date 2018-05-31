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

#include "ClickButton.h"

ClickButton::ClickButton(QString imageName, QWidget* parent)
    : QPushButton(parent)
{
    setFixedSize(40, 40);
    button_mouseInButton = false;
    button_mouseClicked = false;
    setImageName(imageName);
    connect(this, SIGNAL(clicked()), this, SLOT(buttonClick()));
}

void ClickButton::paintEvent(QPaintEvent* event)
{

    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    if (button_mouseInButton) {
        painter.fillRect(0, 0, width(), height(), Qt::gray);
        if (button_mouseClicked) {
            painter.fillRect(0, 0, width(), height(), Qt::darkGray);
        }
    }

    painter.drawImage(QRectF(3, 3, 35, 35), *(image));
}

void ClickButton::enterEvent(QEvent* event)
{
    Q_UNUSED(event);
    button_mouseInButton = true;
}

void ClickButton::leaveEvent(QEvent* event)
{
    Q_UNUSED(event);
    button_mouseInButton = false;
    button_mouseClicked = false;
}

void ClickButton::buttonClick()
{
    button_mouseClicked = true;
    repaint();
}

void ClickButton::setImageName(QString imageName)
{
    image = new QImage(":/run_environment/graphics/tool/" + imageName);
    repaint();
}
