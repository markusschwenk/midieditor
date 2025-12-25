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

#include "MidiSettingsWidget.h"

#include "../Terminal.h"
#include "../midi/MidiFile.h"
#include "../midi/MidiInput.h"
#include "../midi/MidiOutput.h"
#include "../midi/Metronome.h"

#include <QGroupBox>
#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTextEdit>

AdditionalMidiSettingsWidget::AdditionalMidiSettingsWidget(QSettings* settings, QWidget* parent)
    : SettingsWidget("Additional Midi Settings", parent)
{

    _settings = settings;

    QGridLayout* layout = new QGridLayout(this);
    setLayout(layout);

    layout->addWidget(new QLabel("Default ticks per quarter note:", this), 0, 0, 1, 2);
    _tpqBox = new QSpinBox(this);
    _tpqBox->setMinimum(1);
    _tpqBox->setMaximum(1024);
    _tpqBox->setValue(MidiFile::defaultTimePerQuarter);
    connect(_tpqBox, SIGNAL(valueChanged(int)), this, SLOT(setDefaultTimePerQuarter(int)));
    layout->addWidget(_tpqBox, 0, 2, 1, 4);

    QWidget* tpqInfo = createInfoBox("Note: There aren't many reasons to change this. MIDI files have a resolution for how many ticks can fit in a quarter note. Higher values = more detail. Lower values may be required for compatibility. Only affects new files.");
    layout->addWidget(tpqInfo, 1, 0, 1, 6);

    layout->addWidget(separator(), 2, 0, 1, 6);

    _alternativePlayerModeBox = new QCheckBox("Manually stop notes", this);
    _alternativePlayerModeBox->setChecked(MidiOutput::isAlternativePlayer);

    connect(_alternativePlayerModeBox, SIGNAL(toggled(bool)), this, SLOT(manualModeToggled(bool)));
    layout->addWidget(_alternativePlayerModeBox, 3, 0, 1, 6);

    QWidget* playerModeInfo = createInfoBox("Note: the above option should not be enabled in general. It is only required if the stop button does not stop playback as expected (e.g. when some notes are not stopped correctly).");
    layout->addWidget(playerModeInfo, 4, 0, 1, 6);

    layout->addWidget(separator(), 5, 0, 1, 6);

    MidiOutput::omitSysExLength = _settings->value("Midi/omitSysExLength", false).toBool();

    _sendSysExWithoutLength = new QCheckBox("Send SysEx without length datas", this);
    _sendSysExWithoutLength->setChecked(MidiOutput::omitSysExLength);

    connect(_sendSysExWithoutLength, SIGNAL(toggled(bool)), this, SLOT(omitSysExtLength(bool)));
    layout->addWidget(_sendSysExWithoutLength, 6, 0, 1, 6);

    QWidget* SysExWithoutLengthInfo = createInfoBox("Omit the SysEx length variable datas");
    layout->addWidget(SysExWithoutLengthInfo, 7, 0, 1, 6);

    layout->addWidget(separator(), 8, 0, 1, 6);

    layout->addWidget(new QLabel("Metronome loudness:", this), 9, 0, 1, 2);
    _metronomeLoudnessBox = new QSpinBox(this);
    _metronomeLoudnessBox->setMinimum(10);
    _metronomeLoudnessBox->setMaximum(100);
    _metronomeLoudnessBox->setValue(Metronome::loudness());
    connect(_metronomeLoudnessBox, SIGNAL(valueChanged(int)), this, SLOT(setMetronomeLoudness(int)));
    layout->addWidget(_metronomeLoudnessBox, 10, 2, 1, 4);

    layout->addWidget(separator(), 11, 0, 1, 6);

    layout->addWidget(new QLabel("Start command:", this), 12, 0, 1, 2);
    startCmd = new QLineEdit(this);
    layout->addWidget(startCmd, 13, 2, 1, 4);

    QWidget* startCmdInfo = createInfoBox("The start command can be used to start additional software components (e.g. Midi synthesizers) each time, MidiEditor is started. You can see the output of the started software / script in the field below.");
    layout->addWidget(startCmdInfo, 14, 0, 1, 6);

    layout->addWidget(Terminal::terminal()->console(), 10, 0, 1, 6);

    startCmd->setText(_settings->value("Midi/start_cmd", "").toString());
    layout->setRowStretch(3, 1);
}

void AdditionalMidiSettingsWidget::omitSysExtLength(bool enable)
{
    MidiOutput::omitSysExLength = enable;
    _settings->setValue("Midi/omitSysExLength", enable);

}

void AdditionalMidiSettingsWidget::manualModeToggled(bool enable)
{
    MidiOutput::isAlternativePlayer = enable;
}

void AdditionalMidiSettingsWidget::setDefaultTimePerQuarter(int value)
{
    MidiFile::defaultTimePerQuarter = value;
}

void AdditionalMidiSettingsWidget::setMetronomeLoudness(int value){
    Metronome::setLoudness(value);
}

bool AdditionalMidiSettingsWidget::accept()
{
    QString text = startCmd->text();
    if (!text.isEmpty()) {
        _settings->setValue("Midi/start_cmd", text);
    }
    return true;
}

MidiSettingsWidget::MidiSettingsWidget(QWidget* parent)
    : SettingsWidget("Midi I/O", parent)
{

    layout = new QGridLayout(this);
    setLayout(layout);

    QWidget* playerModeInfo = createInfoBox("Choose the Midi ports on your machine to which MidiEditor connects in order to play and record Midi data.");
    layout->addWidget(playerModeInfo, 0, 0, 1, 6);

    // row 2

    QFont font;
    //font.setBold(true);
    font.setPointSize(12);

    // output
    QLabel *l1 = new QLabel("Midi output (Track idx): ", this);
    l1->setFont(font);

    layout->addWidget(l1, 1, 0, 1, 1);

    QSpinBox *spinTrack = new QSpinBox(this);
    spinTrack->setObjectName(QString::fromUtf8("spinTrackSetting"));
    spinTrack->setToolTip("Select MidiOutput device used for the Track.\nTrack 0 uses the same of Track 1");
    spinTrack->setGeometry(QRect(0, 0, 45, 40));
    spinTrack->setFixedSize(120, 40);
    spinTrack->setPrefix("Track: ");

    spinTrack->setAlignment(Qt::AlignCenter);
    spinTrack->setMaximum(MAX_OUTPUT_DEVICES);
    spinTrack->setMinimum(1);
    spinTrack->setValue(1);
    spinTrack->setAutoFillBackground(true);
    spinTrack->setStyleSheet(QString::fromUtf8("color: black; background-color: white;"));
    QFont f = this->font();
    f.setBold(true);
    f.setPixelSize(16);
    spinTrack->setFont(f);

    layout->addWidget(spinTrack, 1, 1, 1, 1);

    QPushButton* reloadOutputList = new QPushButton();
    reloadOutputList->setToolTip("Refresh port list");
    reloadOutputList->setFlat(true);
    reloadOutputList->setIcon(QIcon(":/run_environment/graphics/tool/refresh.png"));
    reloadOutputList->setFixedSize(30, 30);
    layout->addWidget(reloadOutputList, 1, 2, 1, 1);

    // input
    QLabel *l2 = new QLabel("Midi input: ", this);
    l2->setFont(font);
    layout->addWidget(l2, 1, 3, 1, 1);

    QSpinBox *spinInput = new QSpinBox(this);
    spinInput->setObjectName(QString::fromUtf8("spinInputSetting"));
    spinInput->setToolTip("Select MidiInput device used");
    spinInput->setGeometry(QRect(0, 0, 45, 40));
    spinInput->setFixedSize(180, 40);

    spinInput->setAlignment(Qt::AlignCenter);
    spinInput->setMaximum(MAX_INPUT_DEVICES - 1);
    spinInput->setMinimum(0);
    spinInput->setValue(0);
    spinInput->setPrefix("Device: ");
    spinInput->setSuffix(" UP  ");
    spinInput->setAutoFillBackground(true);
    spinInput->setStyleSheet(QString::fromUtf8("color: black; background-color: white;"));
    f = this->font();
    f.setBold(true);
    f.setPixelSize(16);
    spinInput->setFont(f);

    layout->addWidget(spinInput, 1, 4, 1, 1);

    // input
    QPushButton* reloadInputList = new QPushButton();
    reloadInputList->setFlat(true);
    reloadInputList->setToolTip("Refresh port list");
    reloadInputList->setIcon(QIcon(":/run_environment/graphics/tool/refresh.png"));
    reloadInputList->setFixedSize(30, 30);
    layout->addWidget(reloadInputList, 1, 5, 1, 1);

    // row 2
    // output
    for(int n = 0; n < MAX_OUTPUT_DEVICES; n++) {
        _outList[n] = new QListWidget(this);

        connect(_outList[n], SIGNAL(itemChanged(QListWidgetItem*)), this,
            SLOT(outputChanged(QListWidgetItem*)));

        _outList[n]->setFixedWidth(400);
        _outList[n]->setVisible(false);

        if(n == 0) {

            _outList[n]->setVisible(true);

            layout->addWidget(_outList[n], 2, 0, 1, 3);
            outListIndex = n;
        } else
            layout->addWidget(_outList[n], 0, 0, 1, 3);

    }

    // input
    for(int n = 0; n < MAX_INPUT_DEVICES; n++) {
        _inList[n] = new QListWidget(this);
        connect(_inList[n], SIGNAL(itemChanged(QListWidgetItem*)), this,
            SLOT(inputChanged(QListWidgetItem*)));

        _inList[n]->setFixedWidth(400);
        _inList[n]->setVisible(false);

        if(n == 0) {

            _inList[n]->setVisible(true);

            layout->addWidget(_inList[n], 2, 3, 1, 3);
            inListIndex = n;
        } else
            layout->addWidget(_inList[n], 0, 0, 1, 3);

    }

    // row 3
    QGroupBox *trackOutputGroup;
    QCheckBox *AllTracksToOne;
    QCheckBox *FluidSynthTracksAuto;
    QCheckBox *ForceFluidSynthDrum9;
    QCheckBox *SaveMidiOutDatas;

    trackOutputGroup = new QGroupBox();
    trackOutputGroup->setObjectName(QString::fromUtf8("trackOutputGroup"));
    trackOutputGroup->setGeometry(QRect(12, 5, 271, 97));
    trackOutputGroup->setFixedSize(271, 97);
    trackOutputGroup->setTitle("Track Output Control");

    AllTracksToOne = new QCheckBox(trackOutputGroup);
    AllTracksToOne->setObjectName(QString::fromUtf8("AllTracksToOne"));
    AllTracksToOne->setGeometry(QRect(11, 18, 223, 17));
    AllTracksToOne->setText("Force Tracks to Track 1 (16 chans only)");
    AllTracksToOne->setChecked(false/*MidiOutput::AllTracksToOne*/);

    FluidSynthTracksAuto = new QCheckBox(trackOutputGroup);
    FluidSynthTracksAuto->setObjectName(QString::fromUtf8("FluidSynthTracksAuto"));
    FluidSynthTracksAuto->setGeometry(QRect(11, 35, 250, 17));
    FluidSynthTracksAuto->setText("Use Fluidsynth for All (48 channels/interleaved)");
    FluidSynthTracksAuto->setChecked(false/*MidiOutput::FluidSynthTracksAuto*/);

    ForceFluidSynthDrum9 = new QCheckBox(trackOutputGroup);
    ForceFluidSynthDrum9->setObjectName(QString::fromUtf8("ForceFluidSynthDrum9"));
    ForceFluidSynthDrum9->setGeometry(QRect(11, 53, 228, 17));
    ForceFluidSynthDrum9->setText("Force Fluidsynth use of Channel 9 as Drum");
    ForceFluidSynthDrum9->setChecked(MidiOutput::file->DrumUseCh9);
    MidiOutput::forceDrum(MidiOutput::file->DrumUseCh9);

    SaveMidiOutDatas = new QCheckBox(trackOutputGroup);
    SaveMidiOutDatas->setObjectName(QString::fromUtf8("SaveMidiOutDatas"));
    SaveMidiOutDatas->setGeometry(QRect(11, 70, 228, 17));
    SaveMidiOutDatas->setText("Save/use MidiOut Device to/from MIDI");
    SaveMidiOutDatas->setChecked(MidiOutput::SaveMidiOutDatas);

    if(MidiOutput::AllTracksToOne) {
        MidiOutput::FluidSynthTracksAuto = false;
        _outList[0]->setDisabled(false);
        spinTrack->setDisabled(true);
        spinTrack->setValue(1);
    }

#ifndef USE_FLUIDSYNTH
    FluidSynthTracksAuto->setDisabled(true);
    FluidSynthTracksAuto->setChecked(false);
#else
    if(MidiOutput::FluidSynthTracksAuto) {
        spinTrack->setDisabled(true);
        spinTrack->setValue(1);
        _outList[0]->setDisabled(true);
    }
#endif

    layout->addWidget(trackOutputGroup, 3, 0, 1, 1);

    // output
    connect(reloadOutputList, SIGNAL(clicked()), this,
            SLOT(reloadOutputPorts()));
    reloadOutputPorts();

    connect(spinTrack, QOverload<int>::of(&QSpinBox::valueChanged), [=](int num)
    {
        if(!_outList[outListIndex]->isEnabled())
            return;

        num--;
        if(num < 0)
            num = 0;

        for(int n = 0; n < MAX_OUTPUT_DEVICES; n++) {
           _outList[n]->setVisible(false);
           if(n == num) {
               _outList[n]->setVisible(true);

               layout->replaceWidget(_outList[outListIndex], _outList[n]);
               outListIndex = n;
           }
        }

        reloadOutputPorts();
    });

    // input
    connect(reloadInputList, SIGNAL(clicked()), this,
        SLOT(reloadInputPorts()));
    reloadInputPorts();

    connect(spinInput, QOverload<int>::of(&QSpinBox::valueChanged), [=](int num)
    {
        if(num < 0)
            num = 0;

        if(num & 1)
            spinInput->setSuffix(" DOWN");
        else
            spinInput->setSuffix(" UP  ");

        for(int n = 0; n < MAX_INPUT_DEVICES; n++) {
           _inList[n]->setVisible(false);
           if(n == num) {
               _inList[n]->setVisible(true);

               layout->replaceWidget(_inList[inListIndex], _inList[n]);
               inListIndex = n;
           }
        }

        reloadInputPorts();
    });


    // output
    if(MidiOutput::AllTracksToOne) {
        FluidSynthTracksAuto->setDisabled(true);
        FluidSynthTracksAuto->setChecked(false);
        spinTrack->setDisabled(true);
        spinTrack->setValue(1);
    }
#ifdef IS_QT5
    connect(AllTracksToOne, static_cast<void (QCheckBox::*)(int)>(&QCheckBox::stateChanged), [=](int state)
#else
    connect(AllTracksToOne, static_cast<void (QCheckBox::*)(Qt::CheckState)>(&QCheckBox::checkStateChanged), [=](Qt::CheckState state)
#endif
    {
        int num = 0;

        _outList[0]->setDisabled(false);
        spinTrack->setDisabled(true);
        spinTrack->setValue(num + 1);
        FluidSynthTracksAuto->setDisabled(true);
        FluidSynthTracksAuto->setChecked(false);

        if(state == Qt::Checked) {
            FluidSynthTracksAuto->setDisabled(true);
            for(int n = 0; n < MAX_OUTPUT_DEVICES; n++) {
               _outList[n]->setVisible(false);
               if(n == num) {
                   _outList[n]->setVisible(true);

                   layout->replaceWidget(_outList[outListIndex], _outList[n]);
                   outListIndex = n;
               }
            }

            reloadOutputPorts();

            MidiOutput::AllTracksToOne = true;
        } else {
            spinTrack->setDisabled(false);
            MidiOutput::AllTracksToOne = false;
#ifdef USE_FLUIDSYNTH
            FluidSynthTracksAuto->setDisabled(false);
#endif
        }

        update();
    });

    AllTracksToOne->setChecked(MidiOutput::AllTracksToOne);

#ifdef USE_FLUIDSYNTH

#ifdef IS_QT5
    connect(FluidSynthTracksAuto, static_cast<void (QCheckBox::*)(int)>(&QCheckBox::stateChanged), [=](int state)
#else
    connect(FluidSynthTracksAuto, static_cast<void (QCheckBox::*)(Qt::CheckState)>(&QCheckBox::checkStateChanged), [=](Qt::CheckState state)
#endif
    {
        if(state == Qt::Checked) {

            int num = 0;
            spinTrack->setValue(num + 1);
            spinTrack->setDisabled(true);
            _outList[0]->setDisabled(true);

            for(int n = 0; n < MAX_OUTPUT_DEVICES; n++) {
                _outList[n]->setVisible(false);
                if(n == num) {
                    _outList[n]->setVisible(true);

                    layout->replaceWidget(_outList[outListIndex], _outList[n]);
                    outListIndex = n;
                }
            }

            reloadOutputPorts();

            //_outList[0]->setDisabled(true);

            MidiOutput::FluidSynthTracksAuto = true;

            for(int n = 0; n < MAX_OUTPUT_DEVICES; n++) {

                MidiOutput::setOutputPort(MidiOutput::GetFluidDevice(n), n);

            }

            reloadOutputPorts();

        } else {
            MidiOutput::FluidSynthTracksAuto = false;
            spinTrack->setDisabled(false);
            _outList[0]->setDisabled(false);
        }

    });

    if(!MidiOutput::AllTracksToOne)
        FluidSynthTracksAuto->setChecked(MidiOutput::FluidSynthTracksAuto);

#endif

#ifdef IS_QT5
    connect(ForceFluidSynthDrum9, static_cast<void (QCheckBox::*)(int)>(&QCheckBox::stateChanged), [=](int state)
#else
    connect(ForceFluidSynthDrum9, static_cast<void (QCheckBox::*)(Qt::CheckState)>(&QCheckBox::checkStateChanged), [=](Qt::CheckState state)
#endif
    {
        MidiOutput::forceDrum((state == Qt::Checked) ? true : false);

    });

#ifdef IS_QT5
    connect(SaveMidiOutDatas, static_cast<void (QCheckBox::*)(int)>(&QCheckBox::stateChanged), [=](int state)
#else
    connect(SaveMidiOutDatas, static_cast<void (QCheckBox::*)(Qt::CheckState)>(&QCheckBox::checkStateChanged), [=](Qt::CheckState state)
#endif

    {
        MidiOutput::SaveMidiOutDatas = (state == Qt::Checked) ? true : false;

        if(MidiOutput::SaveMidiOutDatas) {
            if(MidiOutput::loadTrackDevices(true, this)) {

                reloadOutputPorts();

                AllTracksToOne->setChecked(MidiOutput::AllTracksToOne);

                if(MidiOutput::AllTracksToOne) {
                    FluidSynthTracksAuto->setDisabled(true);
                    FluidSynthTracksAuto->setChecked(false);
                    spinTrack->setDisabled(true);
                    spinTrack->setValue(1);
                } else
                    FluidSynthTracksAuto->setChecked(MidiOutput::FluidSynthTracksAuto);

#ifndef USE_FLUIDSYNTH
                FluidSynthTracksAuto->setDisabled(true);
                FluidSynthTracksAuto->setChecked(false);

#else
                if(MidiOutput::FluidSynthTracksAuto) {
                    spinTrack->setDisabled(true);
                    spinTrack->setValue(1);
                    _outList[0]->setDisabled(true);
                }
#endif




            }
        }
    });

}

void MidiSettingsWidget::reloadInputPorts()
{
    for(int n = 0; n < MAX_INPUT_DEVICES; n++) {

        disconnect(_inList[n], SIGNAL(itemChanged(QListWidgetItem*)), this,
            SLOT(inputChanged(QListWidgetItem*)));

        // clear the list
        _inList[n]->clear();

        foreach (QString name, MidiInput::inputPorts(n)) {

            QListWidgetItem* item = new QListWidgetItem(name, _inList[n],
                QListWidgetItem::UserType);
            item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Unchecked);

            if(name != ""){
                item->setToolTip("Midi Input device #" + QString::number(n));
            }

            if (name != "" && name == MidiInput::inputPort(n)) {
                item->setCheckState(Qt::Checked);
            }

            _inList[n]->addItem(item);
        }
        connect(_inList[n], SIGNAL(itemChanged(QListWidgetItem*)), this,
            SLOT(inputChanged(QListWidgetItem*)));
    }
}

void MidiSettingsWidget::reloadOutputPorts()
{
    for(int n = 0; n < MAX_OUTPUT_DEVICES; n++) {

        disconnect(_outList[n], SIGNAL(itemChanged(QListWidgetItem*)), this,
            SLOT(outputChanged(QListWidgetItem*)));

        // clear the list
        _outList[n]->clear();

        foreach (QString name, MidiOutput::outputPorts(n)) {

            QListWidgetItem* item =
                    new QListWidgetItem(name, _outList[n],
                QListWidgetItem::UserType);
            item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Unchecked);
#ifdef USE_FLUIDSYNTH
            if(MidiOutput::FluidDevice(name) >= 0) {
                item->setBackground(QBrush(QColor(0xd0ffd0)));
                item->setToolTip("Fluidsynth Channel device");
            } else {

                if(name != "" && name == MidiOutput::outputPort(n) && MidiOutput::_midiOutMAP[n] >= 0) {
                    item->setBackground(QBrush(QColor(0xd0ffff)));
                    item->setToolTip("Virtual device (it is connected over Track " +
                                     QString::number(MidiOutput::_midiOutMAP[n] + 1) + QString(")\nRemember that both use the same channels"));
                } else if(name != ""){
                    item->setToolTip("Midi Output device");
                }

            }
#else

            if(name != ""){
                item->setToolTip("Midi Output device");
            }

#endif
            if (name != "" && name == MidiOutput::outputPort(n)) {
                item->setCheckState(Qt::Checked);
            }
            _outList[n]->addItem(item);
        }

        connect(_outList[n], SIGNAL(itemChanged(QListWidgetItem*)), this,
            SLOT(outputChanged(QListWidgetItem*)));
    }

    update();
}

void MidiSettingsWidget::inputChanged(QListWidgetItem* item)
{

    int n = inListIndex;

    if (item->checkState() == Qt::Checked) {

        MidiInput::closeInputPort(n);
        for(int m = 0; m < MAX_INPUT_DEVICES; m++) {
            if(n != m) {
               if(MidiInput::inputPort(m) == item->text()) {
                   MidiInput::closeInputPort(m);
               }
            }
        }

        MidiInput::setInputPort(item->text(), n);

        reloadInputPorts();

    } else {
        MidiInput::closeInputPort(n);
        reloadInputPorts();
    }
}

void MidiSettingsWidget::outputChanged(QListWidgetItem* item)
{

    int n = outListIndex;

    if (item->checkState() == Qt::Checked) {

        MidiOutput::closeOutputPort(n);
        MidiOutput::setOutputPort(item->text(), n);

        reloadOutputPorts();
    } else {
        MidiOutput::closeOutputPort(n);
        reloadOutputPorts();
    }
}
