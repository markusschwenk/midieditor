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

#include "TrackListWidget.h"
#include "ColoredWidget.h"

#include "../midi/MidiChannel.h"
#include "../midi/MidiFile.h"
#include "../midi/MidiTrack.h"
#include "../tool/NewNoteTool.h"
#include "../protocol/Protocol.h"

#include <QAction>
#include <QGridLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QToolBar>
#include <QWidget>

#define ROW_HEIGHT 85

TrackListItem::TrackListItem(MidiTrack* track, TrackListWidget* parent)
    : QWidget(parent)
{

    trackList = parent;
    this->track = track;

    setContentsMargins(0, 0, 0, 0);
    QGridLayout* layout = new QGridLayout(this);
    setLayout(layout);
    layout->setVerticalSpacing(1);

    colored = new ColoredWidget(*(track->color()), this);
    layout->addWidget(colored, 0, 0, 2, 1);
    QString text = "Track " + QString::number(track->number());
    text_track = new QLabel(text, this);
    text_track->setFixedHeight(15);
    layout->addWidget(text_track, 0, 1, 1, 1);

    trackNameLabel = new QLabel("New Track", this);
    trackNameLabel->setFixedHeight(15);
    layout->addWidget(trackNameLabel, 1, 1, 1, 1);

    QToolBar* toolBar = new QToolBar(this);
    toolBar->setIconSize(QSize(12, 12));
    QPalette palette = toolBar->palette();
#ifdef CUSTOM_MIDIEDITOR_GUI
    // Estwald Color Changes
    #ifdef IS_QT5
        palette.setColor(QPalette::Background, QColor(0xe0e0c0));
    #else
        palette.setColor(QPalette::Window, QColor(0xe0e0c0));
    #endif
#else
    #ifdef IS_QT5
        palette.setColor(QPalette::Background, Qt::white);
    #else
        palette.setColor(QPalette::Window, Qt::white);
    #endif
#endif
    toolBar->setPalette(palette);
    // visibility
    visibleAction = new QAction(QIcon(":/run_environment/graphics/trackwidget/visible.png"), "Track visible", toolBar);
    visibleAction->setCheckable(true);
    visibleAction->setChecked(true);
    toolBar->addAction(visibleAction);
    connect(visibleAction, SIGNAL(toggled(bool)), this, SLOT(toggleVisibility(bool)));

    // audibility
    loudAction = new QAction(QIcon(":/run_environment/graphics/trackwidget/loud.png"), "Track audible", toolBar);
    loudAction->setCheckable(true);
    loudAction->setChecked(true);
    toolBar->addAction(loudAction);
    connect(loudAction, SIGNAL(toggled(bool)), this, SLOT(toggleAudibility(bool)));

    toolBar->addSeparator();

    // name
    QAction* renameAction = new QAction(QIcon(":/run_environment/graphics/trackwidget/rename.png"), "Rename track", toolBar);
    toolBar->addAction(renameAction);
    connect(renameAction, SIGNAL(triggered()), this, SLOT(renameTrack()));

    // remove
    QAction* removeAction = new QAction(QIcon(":/run_environment/graphics/trackwidget/remove.png"), "Remove track", toolBar);
    if(track->number() != 0) {
        toolBar->addAction(removeAction);
        connect(removeAction, SIGNAL(triggered()), this, SLOT(removeTrack()));
    } else {
        removeAction->setVisible(false);
    }


    layout->addWidget(toolBar, 2, 1, 1, 1);

    layout->setRowStretch(2, 1);
    setContentsMargins(5, 1, 5, 0);
    setFixedHeight(ROW_HEIGHT);
}

void TrackListItem::toggleVisibility(bool visible)
{
    QString text = "Hide track";
    if (visible) {
        text = "Show track";
    }

    trackList->midiFile()->protocol()->startNewAction(text);
    track->setHidden(!visible);
    trackList->midiFile()->protocol()->endAction();
}

void TrackListItem::toggleAudibility(bool audible)
{
    QString text = "Mute track";
    if (audible) {
        text = "Track audible";
    }
    trackList->midiFile()->protocol()->startNewAction(text);
    track->setMuted(!audible);
    trackList->midiFile()->protocol()->endAction();
}

void TrackListItem::renameTrack()
{
    emit trackRenameClicked(track->number());
}

void TrackListItem::removeTrack()
{
    if(track->file()->numTracks() > 2)
        emit trackRemoveClicked(track->number());
    else
        QMessageBox::critical(this, "MidiEditor", "You cannot delete this track (minimum 2 tracks)");

}

void TrackListItem::onBeforeUpdate()
{

    trackNameLabel->setText(track->name());

    QString text;

    if(MidiOutput::isFluidsynthConnected(track->device_index())) {
        int chan = MidiOutput::_midiOutFluidMAP[MidiOutput::AllTracksToOne ? 0 : track->device_index()] * 16;
        text = "Track " + QString::number(track->number()) + " / " + QString::asprintf("Fluidsynth chan (%i-%i)", chan, chan + 15);
    } else {
        QString out_name = MidiOutput::outputPort(track->device_index());
        out_name.truncate(30);
        text = "Track " + QString::number(track->number()) + " / " + out_name;
    }

    text_track->setText(text);

    if (visibleAction->isChecked() == track->hidden()) {
        disconnect(visibleAction, SIGNAL(toggled(bool)), this, SLOT(toggleVisibility(bool)));
        visibleAction->setChecked(!track->hidden());
        connect(visibleAction, SIGNAL(toggled(bool)), this, SLOT(toggleVisibility(bool)));
    }

    if (loudAction->isChecked() == track->muted()) {
        disconnect(loudAction, SIGNAL(toggled(bool)), this, SLOT(toggleAudibility(bool)));
        loudAction->setChecked(!track->muted());
        connect(loudAction, SIGNAL(toggled(bool)), this, SLOT(toggleAudibility(bool)));
    }
    colored->setColor(*(track->color()));
}

void TrackListItem::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
#ifdef CUSTOM_MIDIEDITOR_GUI
    // Estwald Color Changes
    QPainter *p = new QPainter(this);
    if(!p) return;

    p->fillRect(0, 0, width(), height() - 2, background1);

    int numTracks = trackList->midiFile()->numTracks();

    MidiTrack * t = NULL;
    int i = NewNoteTool::editTrack();
    if(i < numTracks) t = trackList->midiFile()->track(i);
    if(track == t) {

        QColor c(0x80ffff);
        c.setAlpha(32);

        p->fillRect(0, 0, width(), height() - 2, c);
    }


    p->end();
    delete p;
#endif

}

TrackListWidget::TrackListWidget(QWidget* parent)
    : QListWidget(parent)
{

    setSelectionMode(QAbstractItemView::SingleSelection);
#ifdef CUSTOM_MIDIEDITOR_GUI
    // Estwald Color Changes
    setStyleSheet("QListWidget {background-color: #e0e0c0;} QListWidget::item { border-bottom: 1px solid black;}");
#else
    setStyleSheet("QListWidget::item { border-bottom: 1px solid lightGray; }");
#endif
    file = 0;
    connect(this, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(chooseTrack(QListWidgetItem*)));
}

void TrackListWidget::setFile(MidiFile* f, bool update)
{
    file = f;
    connect(file->protocol(), SIGNAL(actionFinished()), this, SLOT(update()));
    if(update)
        this->update();
}

void TrackListWidget::chooseTrack(QListWidgetItem* item)
{

    int t = item->data(Qt::UserRole).toInt();
    MidiTrack* track = trackorder.at(t);
    emit trackClicked(track);

}

void TrackListWidget::update()
{

    if (!file) {
        clear();
        items.clear();
        trackorder.clear();
        QListWidget::update();
        return;
    }

    bool rebuild = false;
    QList<MidiTrack*> oldTracks = trackorder;
    QList<MidiTrack*> realTracks = *file->tracks();

    if (oldTracks.size() != realTracks.size()) {
        rebuild = true;
    } else {
        for (int i = 0; i < oldTracks.size(); i++) {
            if (oldTracks.at(i) != realTracks.at(i)) {
                rebuild = true;
                break;
            }
        }
    }

    if (rebuild) {
        clear();
        items.clear();
        trackorder.clear();

        foreach (MidiTrack* track, realTracks) {
            TrackListItem* widget = new TrackListItem(track, this);
            QListWidgetItem* item = new QListWidgetItem();
            item->setSizeHint(QSize(0, ROW_HEIGHT));
            item->setData(Qt::UserRole, track->number());
            addItem(item);
            setItemWidget(item, widget);
            items.insert(track, widget);
            trackorder.append(track);
            connect(widget, SIGNAL(trackRenameClicked(int)), this, SIGNAL(trackRenameClicked(int)));
            connect(widget, SIGNAL(trackRemoveClicked(int)), this, SIGNAL(trackRemoveClicked(int)));
        }
    }

    int index = -1;

    foreach (TrackListItem* item, items.values()) {
        item->onBeforeUpdate();
        if(item->getTrack()->number() == NewNoteTool::editTrack())
            index = item->getTrack()->number();
    }

    if(index >= 0)

        this->setCurrentRow(index);

    QListWidget::update();
}

MidiFile* TrackListWidget::midiFile()
{
    return file;
}
