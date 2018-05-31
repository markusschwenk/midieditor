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

#ifndef TRACKLISTWIDGET_H_
#define TRACKLISTWIDGET_H_

#include <QColor>
#include <QList>
#include <QListWidget>
#include <QPaintEvent>
#include <QWidget>

class QAction;
class MidiFile;
class QLabel;
class MidiTrack;

class TrackListWidget;
class ColoredWidget;

class TrackListItem : public QWidget {

    Q_OBJECT

public:
    TrackListItem(MidiTrack* track, TrackListWidget* parent);
    void onBeforeUpdate();

signals:
    void trackRenameClicked(int tracknumber);
    void trackRemoveClicked(int tracknumber);

public slots:
    void toggleVisibility(bool visible);
    void toggleAudibility(bool audible);
    void removeTrack();
    void renameTrack();

private:
    QLabel* trackNameLabel;
    TrackListWidget* trackList;
    MidiTrack* track;
    ColoredWidget* colored;
    QAction *visibleAction, *loudAction;
};

class TrackListWidget : public QListWidget {

    Q_OBJECT

public:
    TrackListWidget(QWidget* parent = 0);
    void setFile(MidiFile* f);
    MidiFile* midiFile();

signals:
    void trackRenameClicked(int tracknumber);
    void trackRemoveClicked(int tracknumber);
    void trackClicked(MidiTrack* track);

public slots:
    void update();
    void chooseTrack(QListWidgetItem* item);

private:
    MidiFile* file;
    QMap<MidiTrack*, TrackListItem*> items;
    QList<MidiTrack*> trackorder;
};

#endif
