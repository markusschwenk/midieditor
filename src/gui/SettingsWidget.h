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

#ifndef SETTINGSWIDGET_H_
#define SETTINGSWIDGET_H_

#include <QIcon>
#include <QWidget>

class QString;

class SettingsWidget : public QWidget {

public:
    SettingsWidget(QString title, QWidget* parent = 0);
    QString title();
    virtual bool accept();
    QWidget* createInfoBox(QString info);
    QWidget* separator();
    virtual QIcon icon();
private:
    QString _title;
};

#endif
