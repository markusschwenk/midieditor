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
#include "../midi/MidiTrack.h"
#include "../midi/MidiOutput.h"
#include "../MidiEvent/ControlChangeEvent.h"
#include "../protocol/Protocol.h"
#include "../tool/NewNoteTool.h"
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>

#define ROW_HEIGHT 85

ChannelListItem::ChannelListItem(int ch, ChannelListWidget* parent, int track_index)
    : QWidget(parent)
{

    channelList = parent;
    channel = ch;
    this->track_index = track_index;

    setContentsMargins(0, 0, 0, 0);
    QGridLayout* layout = new QGridLayout(this);
    setLayout(layout);
    layout->setVerticalSpacing(1);

    colored = new ColoredWidget(*(Appearance::channelColor(channel)), this);

    colored->setToolTip("Double click to show this channel only");

    layout->addWidget(colored, 0, 0, 2, 1);

    connect(colored, SIGNAL(doubleClick()), this, SLOT(doubleClick()));

    QString text = "Channel " + QString::number(channel);

    if (channel == 9) text+=" - Drums";
    else
    if (channel == 16) {
        text = "General Events";
    }
    chanLabel = new QLabel(text, this);
    chanLabel->setFixedHeight(15);
    layout->addWidget(chanLabel, 0, 1, 1, 1);

    instrumentLabel = new QLabel("none", this);
    instrumentLabel->setFixedHeight(15);
    layout->addWidget(instrumentLabel, 1, 1, 1, 1);

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
    visibleAction = new QAction(QIcon(":/run_environment/graphics/channelwidget/visible.png"), "Channel visible", toolBar);
    visibleAction->setCheckable(true);
    visibleAction->setChecked(true);
    toolBar->addAction(visibleAction);
    connect(visibleAction, SIGNAL(toggled(bool)), this, SLOT(toggleVisibility(bool)));

    spinOctave = NULL;
    loudAction = NULL;
    soloAction = NULL;
#ifdef USE_FLUIDSYNTH
    bViewVST1  = NULL;
    loadVST1   = NULL;
    bViewVST2  = NULL;
    loadVST2   = NULL;
#endif
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

        // SoundEffect
        QAction* SoundEffectAction = new QAction(QIcon(":/run_environment/graphics/channelwidget/sound_effect.png"), "Select SoundEffect", toolBar);
        toolBar->addAction( SoundEffectAction);
        connect( SoundEffectAction, SIGNAL(triggered()), this, SLOT(SoundEffect()));

#ifdef USE_FLUIDSYNTH
        // VST

        int x = toolBar->width() + 2;

        bViewVST1 = new QPushButton(toolBar);
        bViewVST1->setToolTip("View VST plugin 1 window");
        bViewVST1->setObjectName(QString::fromUtf8("pushButtonviewVST"));
        bViewVST1->setGeometry(QRect(x, 3, 32, 17));
        bViewVST1->setText("View");
        bViewVST1->setEnabled(false);
        bViewVST1->setStyleSheet(QString::fromUtf8("background-color: #2080ff80;"));
        connect(bViewVST1, SIGNAL(clicked()), this, SLOT(viewVST1()));

        x+= 32;

        loadVST1 = new QPushButton(toolBar);
        loadVST1->setToolTip("Load VST plugin 1");
        loadVST1->setObjectName(QString::fromUtf8("pushButtonloadVST1"));
        loadVST1->setGeometry(QRect(x, 3, 38, 17));
        loadVST1->setText("VST1");
        loadVST1->setStyleSheet(QString::fromUtf8("background-color: #6080ff80;"));

        connect(loadVST1, SIGNAL(clicked()), this, SLOT(LoadVST1()));

        x+= 39;

        bViewVST2 = new QPushButton(toolBar);
        bViewVST2->setToolTip("View VST plugin 2 window");
        bViewVST2->setObjectName(QString::fromUtf8("pushButtonviewVST2"));
        bViewVST2->setGeometry(QRect(x, 3, 32, 17));
        bViewVST2->setText("View");
        bViewVST2->setEnabled(false);
        bViewVST2->setStyleSheet(QString::fromUtf8("background-color: #2040ffff;"));

        connect(bViewVST2, SIGNAL(clicked()), this, SLOT(viewVST2()));

        x+= 32;

        loadVST2 = new QPushButton(toolBar);
        loadVST2->setToolTip("Load VST plugin 2");
        loadVST2->setObjectName(QString::fromUtf8("pushButtonloadVST2"));
        loadVST2->setGeometry(QRect(x, 3, 38, 17));
        loadVST2->setText("VST2");
        loadVST2->setStyleSheet(QString::fromUtf8("background-color: #6040ffff;"));

        connect(loadVST2, SIGNAL(clicked()), this, SLOT(LoadVST2()));

#endif
    }

    layout->addWidget(toolBar, 2, 1, 1, 1);

    if(channel >= 0 && channel < 16) {

        spinOctave = new QSpinBox(this);
        spinOctave->setObjectName(QString::fromUtf8("spinOctave"));
        spinOctave->setToolTip("Change Displayed Octave Notes for channel");
        spinOctave->setGeometry(QRect(0, 0, 31, 18));
        spinOctave->setAlignment(Qt::AlignCenter);
        spinOctave->setMinimum(-5);
        spinOctave->setMaximum(5);
        spinOctave->setValue(OctaveChan_MIDI[channel]);

        connect(spinOctave, QOverload<int>::of(&QSpinBox::valueChanged), [=](int v){

            if(v == 0)
                spinOctave->setStyleSheet(QString::fromUtf8("background-color: #ffffff;"));
            else
                spinOctave->setStyleSheet(QString::fromUtf8("background-color: #8010f030;"));

            if((channel < 0) || (channel >= 16))
                return;

            if(OctaveChan_MIDI[channel] == v) return;

            OctaveChan_MIDI[channel] = v;

            if(channel < 16) {

                // remove old ctrl 3 Octave datas
                foreach (MidiEvent* event, MidiOutput::file->channel(channel)->eventMap()->values()) {

                    ControlChangeEvent* ctrl = dynamic_cast<ControlChangeEvent*>(event);

                    if (ctrl && ctrl->control() == 3) {
                        MidiOutput::file->protocol()->startNewAction("Change Octave of notes displayed in Channel #" + QString::number(channel));
                        if(v)
                            ctrl->setValue(v + 5);
                        else
                            MidiOutput::file->channel(channel)->removeEvent(ctrl);
                        v = 555;
                        break;
                        //
                    }

                }

                if(v && v != 555) { //insert ctrl 0x3 if octave != 0
                    MidiOutput::file->protocol()->startNewAction("Change Octave of notes displayed in Channel #" + QString::number(channel));

                    MidiTrack* track = MidiOutput::file->track(NewNoteTool::editTrack());

                    MidiEvent* cevent = new ControlChangeEvent(channel, 0x3, v + 5, track);

                    MidiOutput::file->channel(channel)->insertEvent(cevent, 0);

                    v= 555;
                }

                if(v == 555)
                    MidiOutput::file->protocol()->endAction();

                emit channelStateChanged();
            }

        });

        connect(parent, SIGNAL(WidgeUpdate()), this, SLOT(WidgeUpdate()));

        layout->addWidget(spinOctave, 2, 0, 1, 1);
    }

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
    MidiOutput::file->protocol()->startNewAction(text);
    MidiOutput::file->channel(channel)->setVisible(visible, track_index);
    MidiOutput::file->protocol()->endAction();
    emit channelStateChanged();
}

void ChannelListItem::toggleAudibility(bool audible)
{
    QString text = "Mute channel";
    if (audible) {
        text = "Channel audible";
    }
    MidiOutput::file->protocol()->startNewAction(text);
    MidiOutput::file->channel(channel)->setMute(!audible, track_index);
    MidiOutput::file->protocol()->endAction();
    emit channelStateChanged();
}

void ChannelListItem::toggleSolo(bool solo)
{
    QString text = "Enter solo mode";
    if (!solo) {
        text = "Exited solo mode";
    }
    MidiOutput::file->protocol()->startNewAction(text);
    MidiOutput::file->channel(channel)->setSolo(false);
    MidiOutput::file->channel(channel)->setSolo(solo, track_index);
    for(int n = 0; n < 16; n++) {
        if(n == channel)
            continue;
        MidiOutput::file->channel(n)->setSolo(false);
    }
    MidiOutput::file->protocol()->endAction();
    emit channelStateChanged();
}

void ChannelListItem::instrument()
{
    if(!visibleAction)
        return;
    if(visibleAction->isChecked())
        emit selectInstrumentClicked(channel);
}

void ChannelListItem::SoundEffect()
{
    if(!visibleAction)
        return;
    if(visibleAction->isChecked())
        emit selectSoundEffectClicked(channel);
}

#ifdef USE_FLUIDSYNTH

void ChannelListItem::ToggleViewVST1(bool on) {

    if(!bViewVST1)
        return;

    bViewVST1->setEnabled(on);
    if(on)
        bViewVST1->setStyleSheet(QString::fromUtf8("background-color: #6080ff80;"));
    else
        bViewVST1->setStyleSheet(QString::fromUtf8("background-color: #2080ff80;"));
    update();
}

void ChannelListItem::LoadVST1()
{
    if(!visibleAction)
        return;
    if(visibleAction->isChecked())
        emit LoadVSTClicked(channel, 0);
}

void ChannelListItem::viewVST1()
{
    if(!visibleAction)
        return;
    if(visibleAction->isChecked() && visibleAction->isEnabled())
        emit LoadVSTClicked(channel, 1);
}

void ChannelListItem::ToggleViewVST2(bool on) {

    if(!bViewVST2)
        return;

    bViewVST2->setEnabled(on);
    if(on)
        bViewVST2->setStyleSheet(QString::fromUtf8("background-color: #6040ffff;"));
    else
        bViewVST2->setStyleSheet(QString::fromUtf8("background-color: #2040ffff;"));
    update();
}

void ChannelListItem::LoadVST2()
{
    if(!visibleAction)
        return;
    if(visibleAction->isChecked() && visibleAction->isEnabled())
        emit LoadVSTClicked(channel + 16, 0);
}

void ChannelListItem::viewVST2()
{
    if(!visibleAction)
        return;
    if(visibleAction->isChecked())
        emit LoadVSTClicked(channel + 16, 1);
}
#endif

void ChannelListItem::onBeforeUpdate()
{
    static bool reentry = false;
    if(reentry)
        return;

    reentry = true;

    QString text;

    int bank =0;
    MidiTrack * track = NULL;

    if(!MidiOutput::file->MultitrackMode)
        track = MidiOutput::file->track(0);
    else
        track = MidiOutput::file->track(NewNoteTool::editTrack());

    int track_index = MidiOutput::AllTracksToOne ? MidiOutput::_midiOutFluidMAP[0]
                                                 : MidiOutput::_midiOutFluidMAP[track->device_index()];

    bool fluid_visible = track_index >= 0;

    if(track_index < 0) // restore track_index
        track_index = track->device_index();

#ifdef USE_FLUIDSYNTH
    if(bViewVST1)
        bViewVST1->setVisible(fluid_visible);
    if(bViewVST2)
        bViewVST2->setVisible(fluid_visible);
    if(loadVST1)
        loadVST1->setVisible(fluid_visible);
    if(loadVST2)
        loadVST2->setVisible(fluid_visible);
#endif

    int prog = MidiOutput::file->channel(channel)->progBankAtTick(MidiOutput::file->cursorTick(), &bank, track);

    if(channel >= 0 && channel < 16 && (channel != 9 ||
                                         (channel == 9 && track->fluid_index() != 0 && fluid_visible && !MidiOutput::file->DrumUseCh9))) {

        spinOctave->setVisible(true);
        MidiOutput::file->Bank_MIDI[channel + 4 * (track_index != 0) + 16 * track_index]=bank;
        MidiOutput::file->Prog_MIDI[channel + 4 * (track_index != 0) + 16 * track_index]=prog;

    } else if(channel == 9) {
        spinOctave->setVisible(false);
    }

    this->track_index = track_index;

    if(channel == 16) {
        text = "Channel " + QString::number(channel);
    } else if(MidiOutput::isFluidsynthConnected(track->device_index())) {
        int chan = MidiOutput::AllTracksToOne ? MidiOutput::_midiOutFluidMAP[0] * 16
                                              : MidiOutput::_midiOutFluidMAP[track_index] * 16;
        text = "Channel " + QString::number(channel) + " / " + QString::asprintf("Fluidsynth chan (%i)", (channel & 15) | (chan & ~15));
    } else {
        QString out_name = MidiOutput::outputPort(track_index);
        out_name.truncate(30);
        text = "Channel " + QString::number(channel) + " / " + out_name;
    }

    if(channel != 9 || (channel == 9 && track->fluid_index() != 0 && MidiOutput::isFluidsynthConnected(track->device_index()) && !MidiOutput::file->DrumUseCh9)) {
        chanLabel->setText(text);
        text= MidiFile::instrumentName(bank, prog);
        if (channel == 16) {
            text = "Events affecting all channels";
        } else {
            text +=" / Bank "+ QString::asprintf("%3.3u", MidiOutput::file->Bank_MIDI[channel + 4 * (track->device_index()!=0) + 16 * track->device_index()]);
        }
    } else {
        text+=" - Drums";
        chanLabel->setText(text);
        text= MidiFile::drumName(prog);
    }

    if(channel <= 16)
        instrumentLabel->setText(text);

    if (MidiOutput::file->channel(channel)->eventMap()->isEmpty()) {
        colored->setColor(Qt::lightGray);
    } else {
        colored->setColor(*(Appearance::channelColor(channel)));
    }

    if (visibleAction->isChecked() != MidiOutput::file->channel(channel)->visible(track_index)) {
        disconnect(visibleAction, SIGNAL(toggled(bool)), this, SLOT(toggleVisibility(bool)));
        visibleAction->setChecked(MidiOutput::file->channel(channel)->visible(track_index));
        connect(visibleAction, SIGNAL(toggled(bool)), this, SLOT(toggleVisibility(bool)));
        emit channelStateChanged();
    }

    if (channel < 16 && (loudAction->isChecked()) == (MidiOutput::file->channel(channel)->mute(track_index))) {
        disconnect(loudAction, SIGNAL(toggled(bool)), this, SLOT(toggleAudibility(bool)));
        loudAction->setChecked(!MidiOutput::file->channel(channel)->mute(track_index));
        connect(loudAction, SIGNAL(toggled(bool)), this, SLOT(toggleAudibility(bool)));
        emit channelStateChanged();
    }

    if (channel < 16 && (soloAction->isChecked()) != (MidiOutput::file->channel(channel)->solo(track_index))) {
        disconnect(soloAction, SIGNAL(toggled(bool)), this, SLOT(toggleSolo(bool)));
        soloAction->setChecked(MidiOutput::file->channel(channel)->solo(track_index));
        connect(soloAction, SIGNAL(toggled(bool)), this, SLOT(toggleSolo(bool)));
        emit channelStateChanged();
    }

    reentry = false;
}

void ChannelListItem::doubleClick()
{

    emit doubleClicked(channel);

}

void ChannelListItem::WidgeUpdate()
{

    int v = (channel >= 0 && channel < 16) ? OctaveChan_MIDI[channel] : 0;
    if(spinOctave)
        spinOctave->setValue(v);

}

void ChannelListItem::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
#ifdef CUSTOM_MIDIEDITOR_GUI
    // Estwald Color Changes
    QPainter *p = new QPainter(this);
    if(!p) return;

    p->fillRect(0, 0, width(), height() - 2, background1);

    if(this->channel == NewNoteTool::editChannel()) {
        QColor c(0x80ffff);
        c.setAlpha(32);
        //#80ff80
        p->fillRect(0, 0, width(), height() - 2, c);
    } else if(this->channel == 9) {
        QColor c(0x80ff80);
        c.setAlpha(32);

        p->fillRect(0, 0, width(), height() - 2, c);
    }

    p->end();
    delete p;
#endif

}

#ifdef USE_FLUIDSYNTH
void ChannelListWidget::ToggleViewVST(int channel, bool on) {

    if(channel < 16)
        items.at(channel & 15)->ToggleViewVST1(on);
    else
        items.at(channel & 15)->ToggleViewVST2(on);
}
#endif

void ChannelListWidget::doubleClicked(int chan)
{
    int track_index = 0;
    if(midiFile()->MultitrackMode)
        track_index = midiFile()->track(NewNoteTool::editTrack())->device_index();

    this->midiFile()->protocol()->startNewAction("Show Channel " + QString().number(chan, 10));
    for(int channel = 0; channel < 17; channel++) {
        if(channel == chan) {
            this->midiFile()->channel(channel)->setVisible(true, track_index);
        } else
            this->midiFile()->channel(channel)->setVisible(false, track_index);
    }
    this->midiFile()->protocol()->endAction();
    emit channelStateChanged();
}

ChannelListWidget::ChannelListWidget(QWidget* parent)
    : QListWidget(parent)
{

    setSelectionMode(QAbstractItemView::NoSelection);
#ifdef CUSTOM_MIDIEDITOR_GUI
    // Estwald Color Changes
    setStyleSheet("QListWidget {background-color: #e0e0c0;} QListWidget::item { border-bottom: 1px solid black;}");
#else
    setStyleSheet("QListWidget::item { border-bottom: 1px solid lightGray; }");
#endif

    int track_index = 0;
    for (int channel = 0; channel < 17; channel++) {
        ChannelListItem* widget = new ChannelListItem(channel, this, track_index);

        QListWidgetItem* item = new QListWidgetItem();
        item->setSizeHint(QSize(0, ROW_HEIGHT));
        item->setData(Qt::UserRole, channel);
        addItem(item);
        setItemWidget(item, widget);
        items.append(widget);

        connect(widget, SIGNAL(channelStateChanged()), this, SIGNAL(channelStateChanged()));

        if(channel < 16) {
            connect(widget, SIGNAL(selectInstrumentClicked(int)), this, SIGNAL(selectInstrumentClicked(int)));
            connect(widget, SIGNAL(selectSoundEffectClicked(int)), this, SIGNAL(selectSoundEffectClicked(int)));
        }

        connect(widget, SIGNAL(doubleClicked(int)), this, SLOT(doubleClicked(int)));

#ifdef USE_FLUIDSYNTH
        if(channel < 16)
            connect(widget, SIGNAL(LoadVSTClicked(int, int)), this, SIGNAL(LoadVSTClicked(int, int)));
#endif
    }

    connect(this, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(chooseChannel(QListWidgetItem*)));
    file = 0;
}

void ChannelListWidget::chooseChannel(QListWidgetItem* item)
{
    int chan = item->data(Qt::UserRole).toInt();
    if(chan < 16) {
        NewNoteTool::setEditChannel(chan);
        emit channelChanged();
        update();
    }
}
#include <QTimer>
void ChannelListWidget::setFile(MidiFile* f, bool update)
{
    file = f;

    connect(file->protocol(), SIGNAL(actionFinished()), this, SLOT(update()));

    if(update)
        this->update();
}

void ChannelListWidget::OctaveUpdate()
{
    static int autolock = 0;

    if(autolock) return;

    autolock = 1;

    for (int i = 0; i < 16; i++) {

        int octrl = 1;

        int v = 0;

        foreach (MidiEvent* event, file->channel(i)->eventMap()->values()) {
            ControlChangeEvent* ctrl = dynamic_cast<ControlChangeEvent*>(event);

            if (ctrl && ctrl->control()==0x3) { // Octave for chan (visual)
                if (octrl) {
                    int octave = ctrl->value() - 5;
                    if(octave < -5) octave = -5;
                    if(octave > 5) octave = 5;
                    v = octave;
                    octrl = 0;
                } else
                    file->channel(i)->removeEvent(ctrl);
            }
        }

        OctaveChan_MIDI[i] = v;
    }


    emit WidgeUpdate();
    // emit channelStateChanged();

           //items.at(0)->spinOctave->valueChanged(OctaveChan_MIDI[0]);
           // items.at(0)->spinOctave->setValue(OctaveChan_MIDI[0]);

    autolock = 0;
}

void ChannelListWidget::update()
{
    int index = -1;


    this->setCurrentRow(0);

           //foreach (ChannelListItem* item, items) {
    for(int chan = 0; chan < 17; chan++) {
        ChannelListItem* item = items.at(chan);

        item->onBeforeUpdate();
        if(item->getChannel() == NewNoteTool::editChannel()) {
            index = item->getChannel();
        }
    }

    if(index >= 0) {

        this->setCurrentRow(index);
    }

    QListWidget::update();

}

MidiFile* ChannelListWidget::midiFile()
{
    return file;
}
