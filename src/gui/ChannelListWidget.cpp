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
#include "../MidiEvent/ControlChangeEvent.h"
#include "../protocol/Protocol.h"
#include "../tool/NewNoteTool.h"
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>

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

    connect(colored, SIGNAL(doubleClick()), this, SLOT(doubleClick()));

    QString text = "Channel " + QString::number(channel);

    if (channel == 9) text+=" - Drums";
    else
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
    palette.setColor(QPalette::Window, Qt::white);
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
        bViewVST1->setStyleSheet(QString::fromUtf8("background-color: #2080ff80;"));
        connect(bViewVST1, SIGNAL(clicked()), this, SLOT(viewVST1()));

        x+= 32;

        QPushButton *loadVST = new QPushButton(toolBar);
        loadVST->setToolTip("Load VST plugin 1");
        loadVST->setObjectName(QString::fromUtf8("pushButtonloadVST"));
        loadVST->setGeometry(QRect(x, 3, 38, 17));
        loadVST->setText("VST1");
        loadVST->setStyleSheet(QString::fromUtf8("background-color: #6080ff80;"));

        connect(loadVST, SIGNAL(clicked()), this, SLOT(LoadVST1()));

        x+= 39;

        bViewVST2 = new QPushButton(toolBar);
        bViewVST2->setToolTip("View VST plugin 2 window");
        bViewVST2->setObjectName(QString::fromUtf8("pushButtonviewVST2"));
        bViewVST2->setGeometry(QRect(x, 3, 32, 17));
        bViewVST2->setText("View");
        bViewVST2->setStyleSheet(QString::fromUtf8("background-color: #2040ffff;"));

        connect(bViewVST2, SIGNAL(clicked()), this, SLOT(viewVST2()));


        x+= 32;

        QPushButton *loadVST2 = new QPushButton(toolBar);
        loadVST2->setToolTip("Load VST plugin 2");
        loadVST2->setObjectName(QString::fromUtf8("pushButtonloadVST2"));
        loadVST2->setGeometry(QRect(x, 3, 38, 17));
        loadVST2->setText("VST2");
        loadVST2->setStyleSheet(QString::fromUtf8("background-color: #6040ffff;"));

        connect(loadVST2, SIGNAL(clicked()), this, SLOT(LoadVST2()));

#endif
    }

    layout->addWidget(toolBar, 2, 1, 1, 1);


    if(channel < 16 && channel != 9) {

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

            if(OctaveChan_MIDI[channel] == v) return;

            OctaveChan_MIDI[channel] = v;

            if(channel != 9 && channel < 16) {

                // remove old ctrl 3 Octave datas
                foreach (MidiEvent* event, channelList->midiFile()->channel(channel)->eventMap()->values()) {

                    ControlChangeEvent* ctrl = dynamic_cast<ControlChangeEvent*>(event);

                    if (ctrl && ctrl->control() == 3) {
                        channelList->midiFile()->protocol()->startNewAction("Change Octave of notes displayed in Channel #" + QString::number(channel));
                        if(v)
                            ctrl->setValue(v + 5);
                        else
                            channelList->midiFile()->channel(channel)->removeEvent(ctrl);
                        v = 555;
                        break;
                        //
                    }

                }

                if(v && v != 555) { //insert ctrl 0x3 if octave != 0
                    channelList->midiFile()->protocol()->startNewAction("Change Octave of notes displayed in Channel #" + QString::number(channel));

                    MidiTrack* track = channelList->midiFile()->track(NewNoteTool::editTrack());

                    MidiEvent* cevent = new ControlChangeEvent(channel, 0x3, v + 5, track);

                    channelList->midiFile()->channel(channel)->insertEvent(cevent, 0);

                    v= 555;
                }

                if(v == 555)
                    channelList->midiFile()->protocol()->endAction();

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
    if(visibleAction->isChecked())
    emit selectInstrumentClicked(channel);
}

void ChannelListItem::SoundEffect()
{
    if(visibleAction->isChecked())
    emit selectSoundEffectClicked(channel);
}

#ifdef USE_FLUIDSYNTH

void ChannelListItem::ToggleViewVST1(bool on) {
    if(on)
        bViewVST1->setStyleSheet(QString::fromUtf8("background-color: #6080ff80;"));
    else
        bViewVST1->setStyleSheet(QString::fromUtf8("background-color: #2080ff80;"));
}

void ChannelListItem::LoadVST1()
{
    if(visibleAction->isChecked())
    emit LoadVSTClicked(channel, 0);
}

void ChannelListItem::viewVST1()
{
    if(visibleAction->isChecked())
    emit LoadVSTClicked(channel, 1);
}

void ChannelListItem::ToggleViewVST2(bool on) {
    if(on)
        bViewVST2->setStyleSheet(QString::fromUtf8("background-color: #6040ffff;"));
    else
        bViewVST2->setStyleSheet(QString::fromUtf8("background-color: #2040ffff;"));
}

void ChannelListItem::LoadVST2()
{
    if(visibleAction->isChecked())
    emit LoadVSTClicked(channel + 16, 0);
}

void ChannelListItem::viewVST2()
{
    if(visibleAction->isChecked())
    emit LoadVSTClicked(channel + 16, 1);
}
#endif

void ChannelListItem::onBeforeUpdate()
{

    QString text;

    int bank =0;
    int prog =channelList->midiFile()->channel(channel)->progBankAtTick(channelList->midiFile()->cursorTick(), &bank);
    if(channel >=0 && channel <16 && channel!=9)
        {Bank_MIDI[channel]=bank;Prog_MIDI[channel]=prog;}

    if(channel != 9) text= MidiFile::instrumentName(bank, prog);
    else text= MidiFile::drumName(prog);

    if (channel == 16) {
        text = "Events affecting all channels";
    } else {
        if (channel != 9)
            text +=" / Bank "+ QString::asprintf("%3.3u", Bank_MIDI[channel]);
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

void ChannelListItem::doubleClick()
{

    emit doubleClicked(channel);

}

void ChannelListItem::WidgeUpdate()
{
    int v = OctaveChan_MIDI[channel];
    spinOctave->setValue(v);

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

    this->midiFile()->protocol()->startNewAction("Show Channel " + QString().number(chan, 10));
    for(int channel = 0; channel < 17; channel++) {
        if(channel == chan)
            this->midiFile()->channel(channel)->setVisible(true);
        else
            this->midiFile()->channel(channel)->setVisible(false);
    }
    this->midiFile()->protocol()->endAction();
    emit channelStateChanged();
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
        connect(widget, SIGNAL(selectSoundEffectClicked(int)), this, SIGNAL(selectSoundEffectClicked(int)));
        connect(widget, SIGNAL(doubleClicked(int)), this, SLOT(doubleClicked(int)));

#ifdef USE_FLUIDSYNTH
        connect(widget, SIGNAL(LoadVSTClicked(int, int)), this, SIGNAL(LoadVSTClicked(int, int)));
#endif
    }
   // connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(isDoubleClicked(QListWidgetItem *)));


    file = 0;
}

void ChannelListWidget::setFile(MidiFile* f)
{
    file = f;
    connect(file->protocol(), SIGNAL(actionFinished()), this, SLOT(update()));
    update();
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

        if(0)
        if(i < 16 && i != 9 && OctaveChan_MIDI[i] != v) {


            emit items.at(i)->spinOctave->valueChanged(v);
            //
        }




    }


     emit WidgeUpdate();
    // emit channelStateChanged();

    //items.at(0)->spinOctave->valueChanged(OctaveChan_MIDI[0]);
    // items.at(0)->spinOctave->setValue(OctaveChan_MIDI[0]);

    autolock = 0;
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
