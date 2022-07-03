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

#ifndef CHANNELLISTWIDGET_H_
#define CHANNELLISTWIDGET_H_

#include <QColor>
#include <QList>
#include <QListWidget>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>

class QAction;
class MidiFile;
class QLabel;

class ChannelListWidget;
class ColoredWidget;

class ChannelListItem : public QWidget {

    Q_OBJECT

public:
    QSpinBox *spinOctave;
    ChannelListItem(int channel, ChannelListWidget* parent);
    void onBeforeUpdate();

signals:
    void selectInstrumentClicked(int channel);
    void selectBankClicked(int channel);
    void selectSoundEffectClicked(int channel);
#ifdef USE_FLUIDSYNTH
    void LoadVSTClicked(int channel, int flag);
#endif
    void channelStateChanged();
    void doubleClicked(int channel);

public slots:
    void toggleVisibility(bool visible);
    void toggleAudibility(bool audible);
    void toggleSolo(bool solo);
    void instrument();
    void SoundEffect();
#ifdef USE_FLUIDSYNTH
    void ToggleViewVST1(bool on);
    void LoadVST1();
    void viewVST1();
    void ToggleViewVST2(bool on);
    void LoadVST2();
    void viewVST2();
#endif
    void doubleClick();
    void WidgeUpdate();

private:
#ifdef USE_FLUIDSYNTH
    QPushButton *bViewVST1;
    QPushButton *bViewVST2;
#endif
    QPushButton *bOctave;
    QLabel* instrumentLabel;
    ChannelListWidget* channelList;
    int channel;
    ColoredWidget* colored;
    QAction *visibleAction, *loudAction, *soloAction;
};

class ChannelListWidget : public QListWidget {

    Q_OBJECT

public:
    ChannelListWidget(QWidget* parent = 0);
    void setFile(MidiFile* f);
    MidiFile* midiFile();

signals:
    void channelStateChanged();
    void selectInstrumentClicked(int channel);
    void selectSoundEffectClicked(int channel);
    void WidgeUpdate();
#ifdef USE_FLUIDSYNTH
    void LoadVSTClicked(int channel, int flag);
#endif

public slots:
    void update();
    void OctaveUpdate();
    void doubleClicked(int channel);
#ifdef USE_FLUIDSYNTH
    void ToggleViewVST(int channel, bool on);
#endif

private:
    MidiFile* file;
    QList<ChannelListItem*> items;
};

#endif
