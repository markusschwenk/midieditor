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

#include "ChannelListWidget.h"
#include <QAction>
#include <QGridLayout>
#include <QLabel>
#include <QPainter>
#include <QToolBar>
#include <QWidget>

#include "Appearance.h"
#include "ColoredWidget.h"
#include "../midi/MidiChannel.h"
#include "../midi/MidiFile.h"
#include "../protocol/Protocol.h"

#define ROW_HEIGHT 85

ChannelListItem::ChannelListItem(int ch, ChannelListWidget* parent)
    : QWidget(parent)
{

    channelList = parent;
    channel = ch;

    setContentsMargins(0, 0, 0, 0);
    QGridLayout* layout = new QGridLayout(this);
    setLayout(layout);
    layout->setVerticalSpacing(1);

    colored = new ColoredWidget(*(Appearance::channelColor(channel)), this);
    layout->addWidget(colored, 0, 0, 2, 1);
    QString text = "Channel " + QString::number(channel);
    if (channel == 16) {
        text = "General Events";
    }
    QLabel* text1 = new QLabel(text, this);
    text1->setFixedHeight(15);
    layout->addWidget(text1, 0, 1, 1, 1);

    instrumentLabel = new QLabel("none", this);
    instrumentLabel->setFixedHeight(15);
    layout->addWidget(instrumentLabel, 1, 1, 1, 1);

    QToolBar* toolBar = new QToolBar(this);
    toolBar->setIconSize(QSize(12, 12));
    QPalette palette = toolBar->palette();
    palette.setColor(QPalette::Background, Qt::white);
    toolBar->setPalette(palette);

    // visibility
    visibleAction = new QAction(QIcon(":/run_environment/graphics/channelwidget/visible.png"), "Channel visible", toolBar);
    visibleAction->setCheckable(true);
    visibleAction->setChecked(true);
    toolBar->addAction(visibleAction);
    connect(visibleAction, SIGNAL(toggled(bool)), this, SLOT(toggleVisibility(bool)));

    // audibility
    if (channel < 16) {
        loudAction = new QAction(QIcon(":/run_environment/graphics/channelwidget/loud.png"), "Channel audible", toolBar);
        loudAction->setCheckable(true);
        loudAction->setChecked(true);
        toolBar->addAction(loudAction);
        connect(loudAction, SIGNAL(toggled(bool)), this, SLOT(toggleAudibility(bool)));

        // solo
        soloAction = new QAction(QIcon(":/run_environment/graphics/channelwidget/solo.png"), "Solo mode", toolBar);
        soloAction->setCheckable(true);
        soloAction->setChecked(false);
        toolBar->addAction(soloAction);
        connect(soloAction, SIGNAL(toggled(bool)), this, SLOT(toggleSolo(bool)));

        toolBar->addSeparator();

        // instrument
        QAction* instrumentAction = new QAction(QIcon(":/run_environment/graphics/channelwidget/instrument.png"), "Select instrument", toolBar);
        toolBar->addAction(instrumentAction);
        connect(instrumentAction, SIGNAL(triggered()), this, SLOT(instrument()));
    }

    layout->addWidget(toolBar, 2, 1, 1, 1);

    layout->setRowStretch(2, 1);
    setContentsMargins(5, 1, 5, 0);
    setFixedHeight(ROW_HEIGHT);
}

void ChannelListItem::toggleVisibility(bool visible)
{
    QString text = "Hide channel";
    if (visible) {
        text = "Show channel";
    }
    channelList->midiFile()->protocol()->startNewAction(text);
    channelList->midiFile()->channel(channel)->setVisible(visible);
    channelList->midiFile()->protocol()->endAction();
    emit channelStateChanged();
}

void ChannelListItem::toggleAudibility(bool audible)
{
    QString text = "Mute channel";
    if (audible) {
        text = "Channel audible";
    }
    channelList->midiFile()->protocol()->startNewAction(text);
    channelList->midiFile()->channel(channel)->setMute(!audible);
    channelList->midiFile()->protocol()->endAction();
    emit channelStateChanged();
}

void ChannelListItem::toggleSolo(bool solo)
{
    QString text = "Enter solo mode";
    if (!solo) {
        text = "Exited solo mode";
    }
    channelList->midiFile()->protocol()->startNewAction(text);
    channelList->midiFile()->channel(channel)->setSolo(solo);
    channelList->midiFile()->protocol()->endAction();
    emit channelStateChanged();
}

void ChannelListItem::instrument()
{
    emit selectInstrumentClicked(channel);
}

void ChannelListItem::onBeforeUpdate()
{

    QString text = MidiFile::instrumentName(channelList->midiFile()->channel(channel)->progAtTick(channelList->midiFile()->cursorTick()));
    if (channel == 16) {
        text = "Events affecting all channels";
    }
    instrumentLabel->setText(text);

    if (channelList->midiFile()->channel(channel)->eventMap()->isEmpty()) {
        colored->setColor(Qt::lightGray);
    } else {
        colored->setColor(*(Appearance::channelColor(channel)));
    }

    if (visibleAction->isChecked() != channelList->midiFile()->channel(channel)->visible()) {
        disconnect(visibleAction, SIGNAL(toggled(bool)), this, SLOT(toggleVisibility(bool)));
        visibleAction->setChecked(channelList->midiFile()->channel(channel)->visible());
        connect(visibleAction, SIGNAL(toggled(bool)), this, SLOT(toggleVisibility(bool)));
        emit channelStateChanged();
    }

    if (channel < 16 && (loudAction->isChecked()) == (channelList->midiFile()->channel(channel)->mute())) {
        disconnect(loudAction, SIGNAL(toggled(bool)), this, SLOT(toggleAudibility(bool)));
        loudAction->setChecked(!channelList->midiFile()->channel(channel)->mute());
        connect(loudAction, SIGNAL(toggled(bool)), this, SLOT(toggleAudibility(bool)));
        emit channelStateChanged();
    }

    if (channel < 16 && (soloAction->isChecked()) != (channelList->midiFile()->channel(channel)->solo())) {
        disconnect(soloAction, SIGNAL(toggled(bool)), this, SLOT(toggleSolo(bool)));
        soloAction->setChecked(channelList->midiFile()->channel(channel)->solo());
        connect(soloAction, SIGNAL(toggled(bool)), this, SLOT(toggleSolo(bool)));
        emit channelStateChanged();
    }
}

ChannelListWidget::ChannelListWidget(QWidget* parent)
    : QListWidget(parent)
{

    setSelectionMode(QAbstractItemView::NoSelection);
    setStyleSheet("QListWidget::item { border-bottom: 1px solid lightGray; }");

    for (int channel = 0; channel < 17; channel++) {
        ChannelListItem* widget = new ChannelListItem(channel, this);
        QListWidgetItem* item = new QListWidgetItem();
        item->setSizeHint(QSize(0, ROW_HEIGHT));
        addItem(item);
        setItemWidget(item, widget);
        items.append(widget);

        connect(widget, SIGNAL(channelStateChanged()), this, SIGNAL(channelStateChanged()));
        connect(widget, SIGNAL(selectInstrumentClicked(int)), this, SIGNAL(selectInstrumentClicked(int)));
    }

    file = 0;
}

void ChannelListWidget::setFile(MidiFile* f)
{
    file = f;
    connect(file->protocol(), SIGNAL(actionFinished()), this, SLOT(update()));
    update();
}

void ChannelListWidget::update()
{

    foreach (ChannelListItem* item, items) {
        item->onBeforeUpdate();
    }

    QListWidget::update();
}

MidiFile* ChannelListWidget::midiFile()
{
    return file;
}
