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

#ifndef CLICKBUTTON_H
#define CLICKBUTTON_H

#include <QFont>
#include <QImage>
#include <QPainter>
#include <QPushButton>

class ClickButton : public QPushButton {

    Q_OBJECT

public:
    ClickButton(QString imageName, QWidget* parent = 0);
    void setImageName(QString imageName);

public slots:
    void buttonClick();

protected:
    void paintEvent(QPaintEvent* event);
    void enterEvent(QEvent* event);
    void leaveEvent(QEvent* event);

private:
    bool button_mouseInButton, button_mouseClicked;
    QImage* image;
};
#endif
