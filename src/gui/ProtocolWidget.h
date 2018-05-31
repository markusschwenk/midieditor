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

#ifndef PROTOCOLWIDGET_H_
#define PROTOCOLWIDGET_H_

#include <QListWidget>
#include <QPaintEvent>

class MidiFile;
class ProtocolStep;

class ProtocolWidget : public QListWidget {

    Q_OBJECT

public:
    ProtocolWidget(QWidget* parent = 0);
    void setFile(MidiFile* f);

public slots:
    void protocolChanged();
    void update();
    void stepClicked(QListWidgetItem* item);

private:
    MidiFile* file;
    bool protocolHasChanged, nextChangeFromList;

    //protected:
    //	void paintEvent(QPaintEvent *event);
};

#endif
