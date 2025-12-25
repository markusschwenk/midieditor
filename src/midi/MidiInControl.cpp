/*
 * MidiEditor
 * Copyright (C) 2010  Markus Schwenk
 *
 * MidiInControl
 * Copyright (C) 2021 Francisco Munoz / "Estwald"
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.+
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "MidiInControl.h"

#include <QMessageBox>
#include <QThread>
#include <QDir>
#include <QFileInfo>
#include <QFileDialog>

#include "../midi/MidiInput.h"
#include "../midi/MidiOutput.h"
#include "../midi/PlayerThread.h"
#include "../midi/MidiPlayer.h"
#include "../midi/MidiFile.h"
#include "../midi/MidiChannel.h"
#include "../midi/MidiTrack.h"
#include "../MidiEvent/ProgChangeEvent.h"
#include "../MidiEvent/ControlChangeEvent.h"
#include "../gui/InstrumentChooser.h"
#include "../gui/SoundEffectChooser.h"
#include "../tool/FingerPatternDialog.h"
#include "../tool/NewNoteTool.h"

#ifdef USE_FLUIDSYNTH
#include "../VST/VST.h"
#endif

#ifndef CUSTOM_MIDIEDITOR_GUI
// Estwald Color Changes
QBrush background1(QImage(":/run_environment/graphics/custom/background.png"));
#endif

static bool show_effects = false;

QWidget *MidiInControl::_main = NULL;
OSDDialog *MidiInControl::osd_dialog = NULL;
QString MidiInControl::OSD;

QString MidiInControl::ActionGP = "Default";
QString MidiInControl::SequencerGP = "Default";

QSettings *MidiInControl::_settings = NULL;
int MidiInControl::_current_note;
int MidiInControl::_current_ctrl;
int MidiInControl::_current_chan = -1;
int MidiInControl::_current_device = -1;

int MidiInControl::sustainUPval = -1;
int MidiInControl::expressionUPval = -1;
int MidiInControl::sustainDOWNval = -1;
int MidiInControl::expressionDOWNval = -1;

static MidiInControl *MidiIn_ctrl = NULL;
static QWidget* _parent = NULL;

static int skip_keys = 0;

QList<InputActionData> MidiInControl::action_effects[MAX_INPUT_DEVICES];
InputActionListWidget * MidiInControl::InputListAction = NULL;
InputSequencerListWidget * MidiInControl::InputListSequencer = NULL;

static int first = 1;

static int led_up[MAX_INPUT_PAIR] = {0};
static int led_down[MAX_INPUT_PAIR] = {0};

int MidiInControl::cur_pairdev = 0;

static bool _split_enable[MAX_INPUT_PAIR] = {true};
static int _note_cut[MAX_INPUT_PAIR] = {0};
static bool _note_duo[MAX_INPUT_PAIR] = {false};
static bool _note_zero[MAX_INPUT_PAIR];
static int _inchannelUp[MAX_INPUT_PAIR];
static int _inchannelDown[MAX_INPUT_PAIR];
static int _channelUp[MAX_INPUT_PAIR];
static int _channelDown[MAX_INPUT_PAIR];
static bool _fixVelUp[MAX_INPUT_PAIR];
static bool _fixVelDown[MAX_INPUT_PAIR];
static bool _autoChordUp[MAX_INPUT_PAIR] = {false};
static bool _autoChordDown[MAX_INPUT_PAIR] = {false};
static bool _notes_only[MAX_INPUT_PAIR] = {false};

bool MidiInControl::invSustainUP[MAX_INPUT_PAIR] = {false};
bool MidiInControl::invExpressionUP[MAX_INPUT_PAIR] = {false};
bool MidiInControl::scalex2ExpressionUP[MAX_INPUT_PAIR] = {false};
bool MidiInControl::invSustainDOWN[MAX_INPUT_PAIR] = {false};
bool MidiInControl::invExpressionDOWN[MAX_INPUT_PAIR] = {false};
bool MidiInControl::scalex2ExpressionDOWN[MAX_INPUT_PAIR] = {false};

static int _transpose_note_up[MAX_INPUT_PAIR] = {0};
static int _transpose_note_down[MAX_INPUT_PAIR] = {0};

static bool _skip_prgbanks[MAX_INPUT_PAIR];
static bool _skip_bankonly[MAX_INPUT_PAIR];
static bool _record_waits;

static int chordTypeUp[MAX_INPUT_PAIR] = {2};
static int chordTypeDown[MAX_INPUT_PAIR] = {2};
static int chordScaleVelocity3Up[MAX_INPUT_PAIR] = {14};
static int chordScaleVelocity5Up[MAX_INPUT_PAIR] = {15};
static int chordScaleVelocity7Up[MAX_INPUT_PAIR] = {16};
static int chordScaleVelocity3Down[MAX_INPUT_PAIR] = {14};
static int chordScaleVelocity5Down[MAX_INPUT_PAIR] = {15};
static int chordScaleVelocity7Down[MAX_INPUT_PAIR] = {16};

int MidiInControl::key_live = 0;
int MidiInControl::key_flag = 0;

bool MidiInControl::VelocityUP_enable[MAX_INPUT_PAIR] = {true};
bool MidiInControl::VelocityDOWN_enable[MAX_INPUT_PAIR] = {true};
int MidiInControl::VelocityUP_scale[MAX_INPUT_PAIR] = {0};
int MidiInControl::VelocityDOWN_scale[MAX_INPUT_PAIR] = {0};
int MidiInControl::VelocityUP_cut[MAX_INPUT_PAIR] = {100};
int MidiInControl::VelocityDOWN_cut[MAX_INPUT_PAIR] = {100};

/////////////////////////////////////////////////////////

static MidiFile *file_live = NULL;

const char MidiInControl::notes[12][3]= {"C ", "C#", "D ", "D#", "E ", "F ", "F#", "G", "G#", "A ", "A#", "B"};

void MidiInControl::update_win() {

    MIDI_INPUT_SEL->clear();

    for(int n = 0; n < MAX_INPUT_PAIR; n++) {
        if(1) {
            MIDI_INPUT_SEL->addItem("Devices " + QString::number(n * 2) + "/" + QString::number(n * 2 + 1), n);
        }
    }

    for(int n = 0; n < MIDI_INPUT_SEL->count(); n++) {
        if(MIDI_INPUT_SEL->itemData(n).toInt() == cur_pairdev) {
            MIDI_INPUT_SEL->setCurrentIndex(n);
            break;
        }
    }

    labelIN->setText("MIDI Device " + QString::number(cur_pairdev * 2) + " (UP):");
    labelIN2->setText("MIDI Device " + QString::number(cur_pairdev * 2 + 1) + " (DOWN):");

    MIDI_INPUT->clear();
    foreach (QString name, MidiInput::inputPorts(MidiInControl::cur_pairdev * 2)) {
        if(name != MidiInput::inputPort(MidiInControl::cur_pairdev * 2)) continue;
        MIDI_INPUT->addItem(name);
        break;
    }

    MIDI_INPUT2->clear();
    foreach (QString name, MidiInput::inputPorts(MidiInControl::cur_pairdev * 2 + 1)) {
        if(name != MidiInput::inputPort(MidiInControl::cur_pairdev * 2 + 1)) continue;
        MIDI_INPUT2->addItem(name);
        break;
    }


    groupBoxVelocityUP->setChecked(VelocityUP_enable[cur_pairdev]);
    dialScaleVelocityUP->setValue(VelocityUP_scale[cur_pairdev]);
    labelViewScaleVelocityUP->setNum(((double) VelocityUP_scale[cur_pairdev])/10.0f);
    dialVelocityUP->setValue(VelocityUP_cut[cur_pairdev]);
    labelViewVelocityUP->setNum(VelocityUP_cut[cur_pairdev]);

    groupBoxVelocityDOWN->setChecked(VelocityDOWN_enable[cur_pairdev]);
    dialScaleVelocityDOWN->setValue(VelocityDOWN_scale[cur_pairdev]);
    labelViewScaleVelocityDOWN->setNum(((double) VelocityDOWN_scale[cur_pairdev])/10.0f);
    dialVelocityDOWN->setValue(VelocityDOWN_cut[cur_pairdev]);
    labelViewVelocityDOWN->setNum(VelocityDOWN_cut[cur_pairdev]);

    SplitBox->setChecked(_split_enable[cur_pairdev]);
    NoteBoxCut->clear();
    NoteBoxCut->addItem("Get It", -1);

    if(_note_cut[cur_pairdev] >= 0) {
        NoteBoxCut->addItem(QString::asprintf("%s %i", notes[_note_cut[cur_pairdev] % 12], _note_cut[cur_pairdev] / 12 - 1), _note_cut[cur_pairdev]);
        NoteBoxCut->setCurrentIndex(1);
    }

    NoteBoxCut->addItem("DUO", -2);

    if(_note_duo[cur_pairdev]) {
        NoteBoxCut->setCurrentIndex(NoteBoxCut->count() - 1);
    }

    NoteBoxCut->addItem("C -1", 0);

    if(_note_zero[cur_pairdev]) {
        NoteBoxCut->setCurrentIndex(NoteBoxCut->count() - 1);
    }

    if(MidiInput::keyboard2_connected[cur_pairdev * 2 + 1])
        NoteBoxCut->setDisabled(true);
    else
        NoteBoxCut->setDisabled(false);

    channelBoxUp->setCurrentIndex(_channelUp[cur_pairdev] + 1);
    channelBoxDown->setCurrentIndex(_channelDown[cur_pairdev] + 1);
    tspinBoxUp->setValue(_transpose_note_up[cur_pairdev]);
    tspinBoxDown->setValue(_transpose_note_down[cur_pairdev]);
    vcheckBoxUp->setChecked(_fixVelUp[cur_pairdev]);
    vcheckBoxDown->setChecked(_fixVelDown[cur_pairdev]);

    echeckBox->setChecked(_notes_only[cur_pairdev]);
    achordcheckBoxUp->setChecked(_autoChordUp[cur_pairdev]);
    achordcheckBoxDown->setChecked(_autoChordDown[cur_pairdev]);

    inchannelBoxUp->setCurrentIndex(_inchannelUp[cur_pairdev] + 1);
    inchannelBoxDown->setCurrentIndex(_inchannelDown[cur_pairdev] + 1);

    checkBoxPrgBank->setChecked(_skip_prgbanks[cur_pairdev]);
    bankskipcheckBox->setChecked(_skip_bankonly[cur_pairdev]);

    if(MidiInControl::InputListAction) {
        MidiInControl::InputListAction->updateList();
    }

    if(MidiInControl::InputListSequencer) {
        MidiInControl::InputListSequencer->updateList();
    }

    sustainUP->setVal(-1, MidiInControl::invSustainUP[cur_pairdev]);
    expressionUP->setVal(-1, MidiInControl::invExpressionUP[cur_pairdev], MidiInControl::scalex2ExpressionUP[cur_pairdev]);
    sustainDOWN->setVal(-1, MidiInControl::invSustainDOWN[cur_pairdev]);
    expressionDOWN->setVal(-1, MidiInControl::invExpressionDOWN[cur_pairdev], MidiInControl::scalex2ExpressionDOWN[cur_pairdev]);

    seqSel->setCurrentIndex(MidiInput::loadSeq_mode);

    this->update();

}

void MidiInControl::init_MidiInControl(QWidget *main, QSettings *settings) {

    _settings = settings;
    _main = main;

    if(!osd_dialog)
        osd_dialog = new OSDDialog((QDialog *) _main, OSD);

    MidiInput::loadSeq_mode = _settings->value("MIDIin/LoadSeq_mode", 2).toInt();

    ActionGP = _settings->value("MIDIin/ActionGroup", "Default").toString();

    MidiInControl::loadActionSettings();

    for(int pairdev = 0; pairdev < MAX_INPUT_PAIR; pairdev++) {
        led_up[pairdev] = 0;
        led_down[pairdev] = 0;
        _split_enable[pairdev] = _settings->value("MIDIin/MIDIin_split_enable" + (pairdev ? QString::number(pairdev) : QString()), true).toBool();
        _note_cut[pairdev] = _settings->value("MIDIin/MIDIin_note_cut" + (pairdev ? QString::number(pairdev) : QString()), -1).toInt();
        _note_duo[pairdev] = _settings->value("MIDIin/MIDIin_note_duo" + (pairdev ? QString::number(pairdev) : QString()), false).toBool();
        _note_zero[pairdev] = _settings->value("MIDIin/MIDIin_note_zero" + (pairdev ? QString::number(pairdev) : QString()), false).toBool();
        _inchannelUp[pairdev] = _settings->value("MIDIin/MIDIin_inchannelUp" + (pairdev ? QString::number(pairdev) : QString()), 0).toInt();
        _inchannelDown[pairdev] = _settings->value("MIDIin/MIDIin_inchannelDown" + (pairdev ? QString::number(pairdev) : QString()), 0).toInt();
        _channelUp[pairdev] = _settings->value("MIDIin/MIDIin_channelUp" + (pairdev ? QString::number(pairdev) : QString()), !pairdev ? -1 : pairdev * 2).toInt();
        _channelDown[pairdev] = _settings->value("MIDIin/MIDIin_channelDown" + (pairdev ? QString::number(pairdev) : QString()), !pairdev ? -1 : pairdev * 2 + 1).toInt();
        _fixVelUp[pairdev] = _settings->value("MIDIin/MIDIin_fixVelUp" + (pairdev ? QString::number(pairdev) : QString()), false).toBool();
        _fixVelDown[pairdev] = _settings->value("MIDIin/MIDIin_fixVelDown" + (pairdev ? QString::number(pairdev) : QString()), false).toBool();
        _autoChordUp[pairdev] = _settings->value("MIDIin/MIDIin_autoChordUp" + (pairdev ? QString::number(pairdev) : QString()), false).toBool();
        _autoChordDown[pairdev] = _settings->value("MIDIin/MIDIin_autoChordDown" + (pairdev ? QString::number(pairdev) : QString()), false).toBool();
        _notes_only[pairdev] = _settings->value("MIDIin/MIDIin_notes_only" + (pairdev ? QString::number(pairdev) : QString()), true).toBool();
        //_events_to_down = _settings->value("MIDIin/MIDIin_events_to_down", false).toBool();
        _transpose_note_up[pairdev] = _settings->value("MIDIin/MIDIin_transpose_note_up" + (pairdev ? QString::number(pairdev) : QString()), 0).toInt();
        _transpose_note_down[pairdev] = _settings->value("MIDIin/MIDIin_transpose_note_down" + (pairdev ? QString::number(pairdev) : QString()), 0).toInt();

        _skip_prgbanks[pairdev] = _settings->value("MIDIin/MIDIin_skip_prgbanks" + (pairdev ? QString::number(pairdev) : QString()), false).toBool();
        _skip_bankonly[pairdev] = _settings->value("MIDIin/MIDIin_skip_bankonly" + (pairdev ? QString::number(pairdev) : QString()), true).toBool();

        chordTypeUp[pairdev] = _settings->value("MIDIin/chordTypeUp" + (pairdev ? QString::number(pairdev) : QString()), 0).toInt();
        chordScaleVelocity3Up[pairdev] = _settings->value("MIDIin/chordScaleVelocity3Up" + (pairdev ? QString::number(pairdev) : QString()), 14).toInt();
        chordScaleVelocity5Up[pairdev] = _settings->value("MIDIin/chordScaleVelocity5Up" + (pairdev ? QString::number(pairdev) : QString()), 15).toInt();
        chordScaleVelocity7Up[pairdev] = _settings->value("MIDIin/chordScaleVelocity7Up" + (pairdev ? QString::number(pairdev) : QString()), 16).toInt();
        chordTypeDown[pairdev] = _settings->value("MIDIin/chordTypeDown" + (pairdev ? QString::number(pairdev) : QString()), 0).toInt();
        chordScaleVelocity3Down[pairdev] = _settings->value("MIDIin/chordScaleVelocity3Down" + (pairdev ? QString::number(pairdev) : QString()), 14).toInt();
        chordScaleVelocity5Down[pairdev] = _settings->value("MIDIin/chordScaleVelocity5Down" + (pairdev ? QString::number(pairdev) : QString()), 15).toInt();
        chordScaleVelocity7Down[pairdev] = _settings->value("MIDIin/chordScaleVelocity7Down" + (pairdev ? QString::number(pairdev) : QString()), 16).toInt();

        MidiInControl::VelocityUP_enable[pairdev] = _settings->value("MIDIin/MIDIin_VelocityUP_enable" + (pairdev ? QString::number(pairdev) : QString()), true).toBool();
        MidiInControl::VelocityUP_scale[pairdev] = _settings->value("MIDIin/MIDIin_VelocityUP_scale" + (pairdev ? QString::number(pairdev) : QString()), 0).toInt();
        MidiInControl::VelocityUP_cut[pairdev] = _settings->value("MIDIin/MIDIin_VelocityUP_cut" + (pairdev ? QString::number(pairdev) : QString()), 100).toInt();
        MidiInControl::VelocityDOWN_enable[pairdev] = _settings->value("MIDIin/MIDIin_VelocityDOWN_enable" + (pairdev ? QString::number(pairdev) : QString()), true).toBool();
        MidiInControl::VelocityDOWN_scale[pairdev] = _settings->value("MIDIin/MIDIin_VelocityDOWN_scale" + (pairdev ? QString::number(pairdev) : QString()), 0).toInt();
        MidiInControl::VelocityDOWN_cut[pairdev] = _settings->value("MIDIin/MIDIin_VelocityDOWN_cut" + (pairdev ? QString::number(pairdev) : QString()), 100).toInt();

        MidiInControl::invSustainUP[pairdev] = _settings->value("MIDIin/MIDIin_invSustainUP" + (pairdev ? QString::number(pairdev) : QString()), false).toBool();
        MidiInControl::invExpressionUP[pairdev] = _settings->value("MIDIin/MIDIin_invExpressionUP" + (pairdev ? QString::number(pairdev) : QString()), false).toBool();
        MidiInControl::scalex2ExpressionUP[pairdev] = _settings->value("MIDIin/MIDIin_scalex2ExpressionUP" + (pairdev ? QString::number(pairdev) : QString()), false).toBool();
        MidiInControl::invSustainDOWN[pairdev] = _settings->value("MIDIin/MIDIin_invSustainDOWN" + (pairdev ? QString::number(pairdev) : QString()), false).toBool();
        MidiInControl::invExpressionDOWN[pairdev] = _settings->value("MIDIin/MIDIin_invExpressionDOWN" + (pairdev ? QString::number(pairdev) : QString()), false).toBool();
        MidiInControl::scalex2ExpressionDOWN[pairdev] = _settings->value("MIDIin/MIDIin_scalex2ExpressionDOWN" + (pairdev ? QString::number(pairdev) : QString()), false).toBool();

        if(_note_duo[pairdev]) _note_zero[pairdev] = false;

        if(MidiInput::loadSeq_mode == 3) {

            MidiInControl::sequencer_load(pairdev);
        }
    }

    _record_waits = _settings->value("MIDIin/MIDIin_record_waits", true).toBool();

    connect(MidiInControl::_main, SIGNAL(remPlayStop()), MidiInControl::_main, SLOT(playStop()), Qt::QueuedConnection);
    connect(MidiInControl::_main, SIGNAL(remRecordStop(int)), MidiInControl::_main, SLOT(recordStop(int)), Qt::QueuedConnection);
    connect(MidiInControl::_main, SIGNAL(remStop()), MidiInControl::_main, SLOT(stop()), Qt::QueuedConnection);
    connect(MidiInControl::_main, SIGNAL(remForward()), MidiInControl::_main, SLOT(forward()), Qt::QueuedConnection);
    connect(MidiInControl::_main, SIGNAL(remBack()), MidiInControl::_main, SLOT(back()), Qt::QueuedConnection);

}

void MidiInControl::paintEvent(QPaintEvent *) {
    QPainter painter(this);

    QLinearGradient linearGrad(0, 0, width(), height());

    linearGrad.setColorAt(0, QColor(0x00, 0x50, 0x70));
    linearGrad.setColorAt(0.5, QColor(0x00, 0x80, 0xa0));
    linearGrad.setColorAt(1, QColor(0x00, 0x60, 0x80));

    painter.fillRect(0, 0, width(), height(), linearGrad);
}

void MidiInControl::VST_reset() {

    // unused
}


MidiInControl::MidiInControl(QWidget* parent): QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint) {

    MIDIin = this;
    _parent = parent;

    led_up[cur_pairdev] = 0;
    led_down[cur_pairdev] = 0;
    tabMIDIin1 = NULL;

    _thru = MidiInput::thru();
    MidiInput::setThruEnabled(true);

    if(!MidiInput::recording()) {
        MidiPlayer::panic(); // reset
        send_live_events();  // send live event effects
        MidiInput::cleanKeyMaps();
    }

    if (MIDIin->objectName().isEmpty())
        MIDIin->setObjectName(QString::fromUtf8("MIDIin"));

    MIDIin->setFixedSize(970 - 180, 540 + 40 + 106);

    MIDIin->setWindowTitle("MIDI In Control");

    CustomQTabWidget *tabWidget = new CustomQTabWidget(MIDIin);
    tabWidget->setObjectName(QString::fromUtf8("Midi_tabWidget"));
    tabWidget->setGeometry(QRect(-2, 0, /*592*/1040, 540 + 40 + 106));

    tabWidget->setFocusPolicy(Qt::NoFocus);
    tabWidget->setAutoFillBackground(true);
    tabWidget->setStyleSheet(QString::fromUtf8(
    "QTabBar::tab:top:selected { background: #c0faca; color: black;}\n"
     ));

    tabMIDIin1 = new QWidget();
    tabMIDIin1->setObjectName(QString::fromUtf8("tabMIDIin1"));
    tabWidget->addTab(tabMIDIin1, "UP/DOWN (Devices 0-1)");

    QWidget *tabMIDIin2 = new QWidget();
    tabMIDIin2->setObjectName(QString::fromUtf8("tabMIDIin2"));
    tab_Actions(tabMIDIin2);
    tabWidget->addTab(tabMIDIin2, "Input Actions");

    QWidget *tabMIDIin3 = new QWidget();
    tabMIDIin3->setObjectName(QString::fromUtf8("tabMIDIin3"));
    tab_Sequencer(tabMIDIin3);
    tabWidget->addTab(tabMIDIin3, "Sequencer");

    QFont font;
    QFont font1;
    QFont font2;
    font.setPointSize(16);
    font2.setPointSize(12);

    int py = 4 + 4;

    QGroupBox * gz = new QGroupBox(tabMIDIin1);
    gz->setObjectName(QString::fromUtf8("gz_group"));
    gz->setGeometry(QRect(30 - 2, py - 4, 120 + 531 - 80 + 6, 40));
    gz->setStyleSheet(QString::fromUtf8("background: #800040bf; \n"));

    QLabel *labelINsel = new QLabel(tabMIDIin1);
    labelINsel->setObjectName(QString::fromUtf8("labelINsel"));
    labelINsel->setGeometry(QRect(30 + 6, py + 2, 120, 16));
    labelINsel->setStyleSheet(QString::fromUtf8("color: white;\n"));
    //labelIN->setAlignment(Qt::AlignCenter);
    labelINsel->setText("MIDI Device Select:");

    MIDI_INPUT_SEL = new QComboBox(tabMIDIin1);
    MIDI_INPUT_SEL->setObjectName(QString::fromUtf8("MIDI_INPUT_SEL"));
    MIDI_INPUT_SEL->setGeometry(QRect(30 + 120, py, 531 - 80, /*31*/ 31));
    TOOLTIP(MIDI_INPUT_SEL, "Select MIDI Input Pair Device");
    MIDI_INPUT_SEL->setFont(font2);
    MIDI_INPUT_SEL->setStyleSheet(QString::fromUtf8("color: black;\n"));

    MIDI_INPUT_SEL->clear();

    bool is_con[MAX_INPUT_DEVICES/2];

    for(int n = 0; n < MAX_INPUT_DEVICES/2; n++)
        is_con[n] = false;

    for(int n = 0; n < MAX_INPUT_DEVICES; n++) {
        if(MidiInput::isConnected(n)) {
            is_con[n/2] = true;
        }
    }

    for(int n = 0; n < MAX_INPUT_PAIR; n++) {
        if(is_con[n] || 1) {
            MIDI_INPUT_SEL->addItem("Devices " + QString::number(n * 2) + "/" + QString::number(n * 2 + 1), n);
        }
    }

    bool found = false;

    for(int n = 0; n < MIDI_INPUT_SEL->count(); n++) {
        if(MIDI_INPUT_SEL->itemData(n).toInt() == cur_pairdev) {
            MIDI_INPUT_SEL->setCurrentIndex(n);
            found = true;
            break;
        }
    }

    connect(MIDI_INPUT_SEL, QOverload<int>::of(&QComboBox::activated), [=](int v)
    {
        if(v >= 0)
            cur_pairdev = MIDI_INPUT_SEL->itemData(v).toInt();

        labelIN->setText("MIDI Device " + QString::number(cur_pairdev * 2) + " (UP):");
        labelIN2->setText("MIDI Device " + QString::number(cur_pairdev * 2 + 1) + " (DOWN):");

        tabWidget->setTabText(0, "UP/DOWN (Devices " + QString::number(cur_pairdev * 2) + "-"
                                     + QString::number(cur_pairdev * 2 + 1) + ")");
        tabWidget->setTabText(1, "Input Actions (Devices " + QString::number(cur_pairdev * 2) + "-"
                                     + QString::number(cur_pairdev * 2 + 1) + ")");
        tabWidget->setTabText(2, "Sequencer (Devices " + QString::number(cur_pairdev * 2) + "-"
                                     + QString::number(cur_pairdev * 2 + 1) + ")");

        update_win();
        update();

               //MidiInput::setInputPort(MIDI_INPUT->itemText(v), 0);

    });

    if(!found && MIDI_INPUT_SEL->count()) {
        //MIDI_INPUT_SEL->setCurrentIndex(-1);
        MIDI_INPUT_SEL->setCurrentIndex(0);
        cur_pairdev = MIDI_INPUT_SEL->itemData(0).toInt();
        update();

    }

    tabWidget->setTabText(0, "UP/DOWN (Devices " + QString::number(cur_pairdev * 2) + "-"
                                 + QString::number(cur_pairdev * 2 + 1) + ")");
    tabWidget->setTabText(1, "Input Actions (Devices " + QString::number(cur_pairdev * 2) + "-"
                                 + QString::number(cur_pairdev * 2 + 1) + ")");
    tabWidget->setTabText(2, "Sequencer (Devices " + QString::number(cur_pairdev * 2) + "-"
                                 + QString::number(cur_pairdev * 2 + 1) + ")");

    py+= 40;

    labelIN = new QLabel(tabMIDIin1);
    labelIN->setObjectName(QString::fromUtf8("labelIN"));
    labelIN->setGeometry(QRect(30, py + 2, 120, 16));
    labelIN->setStyleSheet(QString::fromUtf8("color: white;\n"));
    labelIN->setText("MIDI Device " + QString::number(cur_pairdev * 2) + " (UP):");

    MIDI_INPUT = new QComboBox(tabMIDIin1);
    MIDI_INPUT->setObjectName(QString::fromUtf8("MIDI_INPUT"));
    MIDI_INPUT->setGeometry(QRect(30 + 120, py, 531 - 80, /*31*/ 21));
    TOOLTIP(MIDI_INPUT, "Selected MIDI Input for UP (reconnect)");
    MIDI_INPUT->setFont(font2);
    MIDI_INPUT->setStyleSheet(QString::fromUtf8("color: black;\n"));

    MIDI_INPUT->clear();
    foreach (QString name, MidiInput::inputPorts(MidiInControl::cur_pairdev * 2)) {
        if(name != MidiInput::inputPort(MidiInControl::cur_pairdev * 2)) continue;
        MIDI_INPUT->addItem(name);
        break;
    }

    connect(MIDI_INPUT, QOverload<int>::of(&QComboBox::activated), [=](int v)
    {

        MidiInput::setInputPort(MIDI_INPUT->itemText(v), cur_pairdev * 2);

    });

    py+= 30;

    labelIN2 = new QLabel(tabMIDIin1);
    labelIN2->setObjectName(QString::fromUtf8("labelIN2"));
    labelIN2->setGeometry(QRect(30, py + 2, 120, 16));
    labelIN2->setStyleSheet(QString::fromUtf8("color: white;\n"));
    labelIN2->setText("MIDI Device " + QString::number(cur_pairdev * 2 + 1) + " (DOWN):");

    MIDI_INPUT2 = new QComboBox(tabMIDIin1);
    MIDI_INPUT2->setObjectName(QString::fromUtf8("MIDI_INPUT2"));
    MIDI_INPUT2->setGeometry(QRect(30 + 120, py, 531 - 80, /*31*/ 21));
    TOOLTIP(MIDI_INPUT2, "Selected MIDI Input for DOWN (reconnect)");
    MIDI_INPUT2->setFont(font2);
    MIDI_INPUT2->setStyleSheet(QString::fromUtf8("color: black;\n"));

    MIDI_INPUT2->clear();
    foreach (QString name, MidiInput::inputPorts(MidiInControl::cur_pairdev * 2 + 1)) {
        if(name != MidiInput::inputPort(MidiInControl::cur_pairdev * 2 + 1)) continue;
        MIDI_INPUT2->addItem(name);
        break;
    }

    connect(MIDI_INPUT2, QOverload<int>::of(&QComboBox::activated), [=](int v)
    {

        MidiInput::setInputPort(MIDI_INPUT2->itemText(v), cur_pairdev * 2 + 1);

    });

    py+= 30 - 20;


    QFrame *MIDIin2 = new QFrame(tabMIDIin1); // for heritage style sheet
    MIDIin2->setGeometry(QRect(0, py, width(), height()));
    MIDIin2->setObjectName(QString::fromUtf8("MIDIin2"));
    MIDIin2->setStyleSheet(QString::fromUtf8("color: white;"));

    QString style1 = QString::fromUtf8(
                         "QGroupBox QComboBox {color: white; background-color: #9090b3;} \n"
                         "QGroupBox QComboBox:disabled {color: darkGray; background-color: #8080a3;} \n"
                         "QGroupBox QComboBox QAbstractItemView {color: white; background-color: #9090b3; selection-background-color: #24c2c3;} \n"
                         "QGroupBox QSpinBox {color: white; background-color: #9090b3;} \n"
                         "QGroupBox QSpinBox:disabled {color: darkGray; background-color: #9090b3;} \n"
                         "QGroupBox QPushButton {color: black; background-color: #c7c9df;} \n"
                         "QGroupBox QToolTip {color: black;} \n"

                         //"QGroupBox::indicator:!checked {background-color: transparent; image: url(:/run_environment/graphics/custom/unchecked.png);}\n"
                         //"QGroupBox::indicator:checked {background-color: transparent; image: url(:/run_environment/graphics/custom/checked.png);}\n"
                         //"QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left;\n"
                         //"margin: 0 3px 0 3px;}\n"

                         "QGroupBox QCheckBox::indicator {image: url(:/run_environment/graphics/custom/unchecked.png);}\n"
                         "QGroupBox QCheckBox::indicator:checked {image: url(:/run_environment/graphics/custom/checked.png);}\n");

    groupBoxNote = new QGroupBox(MIDIin2);
    groupBoxNote->setObjectName(QString::fromUtf8("groupBoxNote"));
    groupBoxNote->setGeometry(QRect(30, 16, 790 - 60, 111));
    groupBoxNote->setTitle("Key/Note Event");
    groupBoxNote->setStyleSheet(style1);

    TOOLTIP(groupBoxNote, "Velocity cut/scale for channels UP/DOWN");

    rstButton = new QPushButton(groupBoxNote);
    rstButton->setObjectName(QString::fromUtf8("rstButton"));
    rstButton->setFont(font2);
    rstButton->setStyleSheet(QString::fromUtf8(
        "QPushButton {color: black; background-color: #c7c9df;} \n"
        "QPushButton::disabled { color: gray;}"
        "QToolTip {color: black;} \n"));
    rstButton->setGeometry(QRect(10, 20, 81, 31));
    rstButton->setText("Reset");
    TOOLTIP(rstButton, "Deletes the Split parameters\n"
                          "and use settings by default");

    PanicButton = new QPushButton(groupBoxNote);
    PanicButton->setObjectName(QString::fromUtf8("PanicButton"));
    PanicButton->setGeometry(QRect(100, 20, 81, 31));
    PanicButton->setStyleSheet(QString::fromUtf8(
        "QPushButton {color: white; background-color: #c7c9df;} \n"
        "QToolTip {color: black;} \n"));
    PanicButton->setIcon(QIcon(":/run_environment/graphics/tool/panic.png"));
    PanicButton->setIconSize(QSize(24, 24));
    PanicButton->setText(QString());
    TOOLTIP(PanicButton, "MIDI Panic extended");

    pushButtonFinger = new QPushButton(groupBoxNote);
    pushButtonFinger->setObjectName(QString::fromUtf8("pushButtonFinger"));
    pushButtonFinger->setGeometry(QRect(10, 80, 81, 23));
    pushButtonFinger->setText("FINGER");
    TOOLTIP(pushButtonFinger, "To Finger Pattern Utility Tool");

    connect(pushButtonFinger, QOverload<bool>::of(&QPushButton::clicked), [=](bool)
    {

        if(FingerPatternWin)
            delete FingerPatternWin;

        FingerPatternWin =  new FingerPatternDialog(_main, _settings, MidiInControl::cur_pairdev);

        FingerPatternWin->setModal(false);
        FingerPatternWin->show();
        FingerPatternWin->raise();
        FingerPatternWin->activateWindow();
        MyVirtualKeyboard::overlap();
        QCoreApplication::processEvents();

    });


    pushButtonMessage = new QPushButton(groupBoxNote);
    pushButtonMessage->setObjectName(QString::fromUtf8("pushButtonMessage"));
    pushButtonMessage->setGeometry(QRect(100, 80, 81, 23));
    pushButtonMessage->setText("DON'T TOUCH");
    TOOLTIP(pushButtonMessage, "DON'T TOUCH OR DIE!");

    connect(pushButtonMessage, QOverload<bool>::of(&QPushButton::clicked), [=](bool)
    {
#ifdef USE_FLUIDSYNTH
        extern int strange;
        strange ^= 1;
#endif
        QMessageBox::information(this, "ooh no!", "ok, you wanted it ... :p");
    });


    checkBoxPrgBank = new QCheckBox(groupBoxNote);
    checkBoxPrgBank->setObjectName(QString::fromUtf8("checkBoxPrgBank"));
    checkBoxPrgBank->setGeometry(QRect(120 + 95, 20, 121, 17));
    checkBoxPrgBank->setStyleSheet(QString::fromUtf8("QToolTip {color: black;} \n"));
    checkBoxPrgBank->setChecked(_skip_prgbanks[cur_pairdev]);
    checkBoxPrgBank->setText("Skip Prg/Bank Events");
    TOOLTIP(checkBoxPrgBank, "Skip Prg/Bank Events from the MIDI keyboard\n"
                                "and instead use the 'Split Keyboard' instruments\n"
                                "or from 'New Events' channel");

    bankskipcheckBox = new QCheckBox(groupBoxNote);
    bankskipcheckBox->setObjectName(QString::fromUtf8("bankskipcheckBox"));
    bankskipcheckBox->setGeometry(QRect(120 + 95, 20 + 20, 101, 17));
    bankskipcheckBox->setStyleSheet(QString::fromUtf8("QToolTip {color: black;} \n"));
    bankskipcheckBox->setChecked(_skip_bankonly[cur_pairdev]);
    bankskipcheckBox->setText("Skip Bank Only");
    TOOLTIP(bankskipcheckBox, "ignore the bank change event coming from the\n"
                                 "MIDI keyboard and use bank 0 instead\n"
                                 "Useful if your MIDI keyboard is compatible with\n"
                                 "the General Midi list");

    groupBoxVelocityUP = new QGroupBox(groupBoxNote);
    groupBoxVelocityUP->setObjectName(QString::fromUtf8("groupBoxVelocityUP"));
    groupBoxVelocityUP->setGeometry(QRect(388 /*190*/, 12, 161, 91));
    groupBoxVelocityUP->setAlignment(Qt::AlignCenter);
    groupBoxVelocityUP->setCheckable(true);
    groupBoxVelocityUP->setChecked(VelocityUP_enable[cur_pairdev]);
    groupBoxVelocityUP->setTitle("Velocity UP Scale/Cut");
    groupBoxVelocityUP->setStyleSheet("QGroupBox {margin-top: 0px; background-color:  #60b386;}\n" + GroupBoxChecked2);
    TOOLTIP(groupBoxVelocityUP, "Velocity cut/scale for channels UP");

    dialScaleVelocityUP = new QDialE(groupBoxVelocityUP, QDIALE_BLACK_NOTCH_BLACKDOT);
    dialScaleVelocityUP->setObjectName(QString::fromUtf8("dialScaleVelocityUP"));
    dialScaleVelocityUP->setGeometry(QRect(10, 13, 61, 61));
    dialScaleVelocityUP->setMinimum(0);
    dialScaleVelocityUP->setMaximum(100);
    dialScaleVelocityUP->setNotchTarget(10.000000000000000);
    dialScaleVelocityUP->setNotchesVisible(true);
    dialScaleVelocityUP->setValue(VelocityUP_scale[cur_pairdev]);
    TOOLTIP(dialScaleVelocityUP, "Scale Notes UP. Composite number works like this:\n"
                                    "x.0 to x.9 : increase Velocity from 100% to 190%\n"
                                    "1.x to 10.x: Multiply 127/10 using this factor and fix it\n"
                                    "as the minimun Velocity\n\n"
                                    "Examples (for input velocity 40):\n"
                                    "    0.0 ->  +0%,   0 as minimun = 40\n"
                                    "    0.5 -> +50%,   0 as minimun = 60\n"
                                    "    3.4 -> +40%,  43 as minimun = 56\n"
                                    "    9.1 -> +10%, 115 as minimun = 115\n");

    labelViewScaleVelocityUP = new QLabel(groupBoxVelocityUP);
    labelViewScaleVelocityUP->setObjectName(QString::fromUtf8("labelViewScaleVelocityUP"));
    labelViewScaleVelocityUP->setGeometry(QRect(25, 70, 31, 16));
    labelViewScaleVelocityUP->setFont(font1);
    labelViewScaleVelocityUP->setAlignment(Qt::AlignCenter);
    labelViewScaleVelocityUP->setNum(((double) VelocityUP_scale[cur_pairdev])/10.0f);
    labelViewScaleVelocityUP->setStyleSheet(QString::fromUtf8("color: white; background-color: #00C070;\n"));

    dialVelocityUP = new QDialE(groupBoxVelocityUP, QDIALE_BLACK_NOTCH_BLACKDOT);
    dialVelocityUP->setObjectName(QString::fromUtf8("dialVelocityUP"));
    dialVelocityUP->setGeometry(QRect(90, 13, 61, 61));
    dialVelocityUP->setMinimum(10);
    dialVelocityUP->setMaximum(127);
    dialVelocityUP->setValue(VelocityUP_cut[cur_pairdev]);
    dialVelocityUP->setNotchTarget(8.000000000000000);
    dialVelocityUP->setNotchesVisible(true);
    TOOLTIP(dialVelocityUP, "Velocity Cut for UP\n"
                               "(maximun velocity for the notes)");


    labelViewVelocityUP = new QLabel(groupBoxVelocityUP);
    labelViewVelocityUP->setObjectName(QString::fromUtf8("labelViewVelocityUP"));
    labelViewVelocityUP->setGeometry(QRect(105, 70, 31, 16));
    QFont font3;
    font3.setPointSize(10);
    labelViewVelocityUP->setFont(font3);
    labelViewVelocityUP->setAlignment(Qt::AlignCenter);
    labelViewVelocityUP->setNum(VelocityUP_cut[cur_pairdev]);
    labelViewVelocityUP->setStyleSheet(QString::fromUtf8("color: white; background-color: #00C070;\n"));

    groupBoxVelocityDOWN = new QGroupBox(groupBoxNote);
    groupBoxVelocityDOWN->setObjectName(QString::fromUtf8("groupBoxVelocityDOWN"));
    groupBoxVelocityDOWN->setGeometry(QRect(559/*360*/, 13, 161, 91));

    groupBoxVelocityDOWN->setAlignment(Qt::AlignCenter);
    groupBoxVelocityDOWN->setCheckable(true);
    groupBoxVelocityDOWN->setChecked(VelocityDOWN_enable[cur_pairdev]);
    groupBoxVelocityDOWN->setTitle("Velocity DOWN Scale/Cut");
    groupBoxVelocityDOWN->setStyleSheet("QGroupBox {margin-top: 0px; background-color:  #60b386;}\n" + GroupBoxChecked2);
    TOOLTIP(groupBoxVelocityDOWN, "Velocity cut/scale for channels DOWN");

    dialScaleVelocityDOWN = new QDialE(groupBoxVelocityDOWN, QDIALE_BLACK_NOTCH_BLACKDOT);
    dialScaleVelocityDOWN->setObjectName(QString::fromUtf8("dialScaleVelocityDOWN"));
    dialScaleVelocityDOWN->setGeometry(QRect(10, 13, 61, 61));
    dialScaleVelocityDOWN->setMinimum(0);
    dialScaleVelocityDOWN->setMaximum(100);
    dialScaleVelocityDOWN->setNotchTarget(10.000000000000000);
    dialScaleVelocityDOWN->setNotchesVisible(true);
    dialScaleVelocityDOWN->setValue(VelocityDOWN_scale[cur_pairdev]);
    TOOLTIP(dialScaleVelocityDOWN, "Scale Notes DOWN. Composite number works like this:\n"
                                    "x.0 to x.9 : increase Velocity from 100% to 190%\n"
                                    "1.x to 10.x: Multiply 127/10 using this factor and fix it\n"
                                    "as the minimun Velocity\n\n"
                                    "Examples (for input velocity 40):\n"
                                    "    0.0 ->  +0%,   0 as minimun = 40\n"
                                    "    0.5 -> +50%,   0 as minimun = 60\n"
                                    "    3.4 -> +40%,  43 as minimun = 56\n"
                                    "    9.1 -> +10%, 115 as minimun = 115\n");

    labelViewScaleVelocityDOWN = new QLabel(groupBoxVelocityDOWN);
    labelViewScaleVelocityDOWN->setObjectName(QString::fromUtf8("labelViewScaleVelocityDOWN"));
    labelViewScaleVelocityDOWN->setGeometry(QRect(25, 70, 31, 16));
    labelViewScaleVelocityDOWN->setFont(font1);
    labelViewScaleVelocityDOWN->setAlignment(Qt::AlignCenter);
    labelViewScaleVelocityDOWN->setNum(((double) VelocityDOWN_scale[cur_pairdev])/10.0f);
    labelViewScaleVelocityDOWN->setStyleSheet(QString::fromUtf8("color: white; background-color: #00C070;\n"));

    dialVelocityDOWN = new QDialE(groupBoxVelocityDOWN, QDIALE_BLACK_NOTCH_BLACKDOT);
    dialVelocityDOWN->setObjectName(QString::fromUtf8("dialVelocityDOWN"));
    dialVelocityDOWN->setGeometry(QRect(90, 13, 61, 61));
    dialVelocityDOWN->setMinimum(10);
    dialVelocityDOWN->setMaximum(127);
    dialVelocityDOWN->setValue(VelocityDOWN_cut[cur_pairdev]);
    dialVelocityDOWN->setNotchTarget(8.000000000000000);
    dialVelocityDOWN->setNotchesVisible(true);
    TOOLTIP(dialVelocityDOWN, "Velocity Cut for DOWN\n"
                               "(maximun velocity for the notes)");

    labelViewVelocityDOWN = new QLabel(groupBoxVelocityDOWN);
    labelViewVelocityDOWN->setObjectName(QString::fromUtf8("labelViewVelocityDOWN"));
    labelViewVelocityDOWN->setGeometry(QRect(105, 70, 31, 16));
    labelViewVelocityDOWN->setFont(font3);
    labelViewVelocityDOWN->setAlignment(Qt::AlignCenter);
    labelViewVelocityDOWN->setNum(VelocityDOWN_cut[cur_pairdev]);
    labelViewVelocityDOWN->setStyleSheet(QString::fromUtf8("color: white; background-color: #00C070;\n"));


    connect(groupBoxVelocityUP, QOverload<bool>::of(&QGroupBox::toggled), [=](bool f)
    {
        MidiInControl::VelocityUP_enable[cur_pairdev] =  f;
        _settings->setValue("MIDIin/MIDIin_VelocityUP_enable" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), f);
    });

    connect(groupBoxVelocityDOWN, QOverload<bool>::of(&QGroupBox::toggled), [=](bool f)
    {
        MidiInControl::VelocityDOWN_enable[cur_pairdev] =  f;
        _settings->setValue("MIDIin/MIDIin_VelocityDOWN_enable" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), f);
    });

    connect(dialScaleVelocityUP, QOverload<int>::of(&QDialE::valueChanged), [=](int v)
    {
        MidiInControl::VelocityUP_scale[cur_pairdev] =  v;
        _settings->setValue("MIDIin/MIDIin_VelocityUP_scale" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), v);
        labelViewScaleVelocityUP->setNum(((double) MidiInControl::VelocityUP_scale[cur_pairdev])/10.0f);
    });

    connect(dialScaleVelocityDOWN, QOverload<int>::of(&QDialE::valueChanged), [=](int v)
    {
        MidiInControl::VelocityDOWN_scale[cur_pairdev] =  v;
        _settings->setValue("MIDIin/MIDIin_VelocityDOWN_scale" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), v);
        labelViewScaleVelocityDOWN->setNum(((double) MidiInControl::VelocityDOWN_scale[cur_pairdev])/10.0f);
    });

    connect(dialVelocityUP, QOverload<int>::of(&QDialE::valueChanged), [=](int v)
    {
        MidiInControl::VelocityUP_cut[cur_pairdev] =  v;
        _settings->setValue("MIDIin/MIDIin_VelocityUP_cut" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), v);
        labelViewVelocityUP->setNum(VelocityUP_cut[cur_pairdev]);
    });

    connect(dialVelocityDOWN, QOverload<int>::of(&QDialE::valueChanged), [=](int v)
    {
        MidiInControl::VelocityDOWN_cut[cur_pairdev] =  v;
        _settings->setValue("MIDIin/MIDIin_VelocityDOWN_cut" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), v);
        labelViewVelocityDOWN->setNum(MidiInControl::VelocityDOWN_cut[cur_pairdev]);
    });

    int yyy = 114;

    buttonBox = new QDialogButtonBox(MIDIin);
    buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
    buttonBox->setStyleSheet(QString::fromUtf8("color: white; background: #8695a3; \n"));

    buttonBox->setGeometry(QRect(/*340*/970 - 198 - 221, 634 + 8, 221, 32));
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Ok);

    SplitBox = new QGroupBox(MIDIin2);
    SplitBox->setObjectName(QString::fromUtf8("SplitBox"));

    SplitBox->setStyleSheet(style1 + GroupBoxChecked1);

    SplitBox->setGeometry(QRect(30, 20 + yyy, 790 - 60, 151));
    SplitBox->setCheckable(true);
    SplitBox->setChecked(_split_enable[cur_pairdev]);
    SplitBox->setTitle("Split Keyboard");
    TOOLTIP(SplitBox, "Split the keyboard into two parts\n"
                         "or combine up to two voices.\n"
                         "Modify notes and others events to do it\n"
                         "Disable it if you want record from the\n"
                         "MIDI keyboard only");

    labelCut = new QLabel(SplitBox);
    labelCut->setObjectName(QString::fromUtf8("labelCut"));
    labelCut->setGeometry(QRect(20, 20, 91, 20));
    labelCut->setAlignment(Qt::AlignCenter);
    labelCut->setText("Note Cut");

    NoteBoxCut = new QComboBox(SplitBox);
    NoteBoxCut->setObjectName(QString::fromUtf8("NoteBoxCut"));
    NoteBoxCut->setGeometry(QRect(20, 40, 91, 31));
    TOOLTIP(NoteBoxCut, "Record the Split note from \n"
                           "the MIDI keyboard clicking\n"
                           "'Get It' and pressing one.\n"
                             "Or combine two voices to duo.\n"
                             "C -1 or 'Get It' state disable 'Down'");

    NoteBoxCut->setFont(font);
    NoteBoxCut->addItem("Get It", -1);

    if(_note_cut[cur_pairdev] >= 0) {
        NoteBoxCut->addItem(QString::asprintf("%s %i", notes[_note_cut[cur_pairdev] % 12], _note_cut[cur_pairdev] / 12 - 1), _note_cut[cur_pairdev]);
        NoteBoxCut->setCurrentIndex(1);
    }

    NoteBoxCut->addItem("DUO", -2);

    if(_note_duo[cur_pairdev]) {
        NoteBoxCut->setCurrentIndex(NoteBoxCut->count() - 1);
    }

    NoteBoxCut->addItem("C -1", 0);

    if(_note_zero[cur_pairdev]) {
        NoteBoxCut->setCurrentIndex(NoteBoxCut->count() - 1); 
    }

    if(MidiInput::keyboard2_connected[cur_pairdev * 2 + 1])
        NoteBoxCut->setDisabled(true);
    else
        NoteBoxCut->setDisabled(false);

    echeckBox = new QCheckBox(SplitBox);
    echeckBox->setObjectName(QString::fromUtf8("echeckBox"));
    echeckBox->setGeometry(QRect(20, 80, 81, 31));
    echeckBox->setText("Notes Only");
    echeckBox->setChecked(_notes_only[cur_pairdev]);
    TOOLTIP(echeckBox, "Skip control events from the keyboard\n"
                          "except the Sustain Pedal event");

    /**********************/

    inlabelUp = new QLabel(SplitBox);
    inlabelUp->setObjectName(QString::fromUtf8("inlabelUp"));
    inlabelUp->setGeometry(QRect(120, 20, 81, 20));
    inlabelUp->setAlignment(Qt::AlignCenter);
    inlabelUp->setText("Channel Up IN");

    inchannelBoxUp = new QComboBox(SplitBox);
    inchannelBoxUp->setObjectName(QString::fromUtf8("inchannelBoxUp"));
    inchannelBoxUp->setStyleSheet(QString::fromUtf8(
        "QComboBox {color: white; background-color: #8695a3;} \n"
        "QComboBox QAbstractItemView {color: white; background-color: #8695a3; selection-background-color: #24c2c3;} \n"
        "QComboBox:disabled {color: darkGray; background-color: #8695a3;} \n"
        "QToolTip {color: black;} \n"));
    TOOLTIP(inchannelBoxUp, "Input Channel from MIDI Keyboard");
    inchannelBoxUp->setGeometry(QRect(120, 40, 81, 31));
    inchannelBoxUp->setFont(font);
    inchannelBoxUp->addItem("ALL", -1);
    for(int n = 0; n < 16; n++) {
        inchannelBoxUp->addItem(QString::asprintf("ch %i", n), n);
    }
    inchannelBoxUp->setCurrentIndex(_inchannelUp[cur_pairdev] + 1);

    inlabelDown = new QLabel(SplitBox);
    inlabelDown->setObjectName(QString::fromUtf8("inlabelDown"));
    inlabelDown->setGeometry(QRect(120, 80, 91, 20));
    inlabelDown->setAlignment(Qt::AlignCenter);
    inlabelDown->setText("Channel Down IN");

    inchannelBoxDown = new QComboBox(SplitBox);
    inchannelBoxDown->setObjectName(QString::fromUtf8("inchannelBoxDown"));
    inchannelBoxDown->setStyleSheet(QString::fromUtf8(
        "QComboBox {color: white; background-color: #8695a3} \n"
        "QComboBox QAbstractItemView {color: white; background-color: #8695a3; selection-background-color: #24c2c3;} \n"
        "QComboBox:disabled {color: darkGray; background-color: #8695a3;} \n"
        "QToolTip {color: black;} \n"));
    TOOLTIP(inchannelBoxDown, "Input Channel from MIDI Keyboard");
    inchannelBoxDown->setGeometry(QRect(120, 100, 81, 31));
    inchannelBoxDown->setFont(font);
    inchannelBoxDown->addItem("ALL", -1);
    for(int n = 0; n < 16; n++) {
        inchannelBoxDown->addItem(QString::asprintf("ch %i", n), n);
    }

    inchannelBoxDown->setCurrentIndex(_inchannelDown[cur_pairdev] + 1);

    int dx1 = 95;

    /**********************/
    LEDBoxUp = new QLedBoxE(SplitBox);
    LEDBoxUp->setObjectName(QString::fromUtf8("LEDBoxUp"));
    LEDBoxUp->setGeometry(QRect(120 + dx1 + 8, 30 + 8, 16, 31));

    TOOLTIP(LEDBoxUp, "Indicates events on channel Up");

    LEDBoxDown = new QLedBoxE(SplitBox);
    LEDBoxDown->setObjectName(QString::fromUtf8("LEDBoxDown"));
    LEDBoxDown->setGeometry(QRect(120 + dx1 + 8, 90 + 8, 16, 31));

    TOOLTIP(LEDBoxDown, "Indicates events on channel Down");

    dx1 = 260;

    labelUp = new QLabel(SplitBox);
    labelUp->setObjectName(QString::fromUtf8("labelUp"));
    labelUp->setGeometry(QRect(dx1, 20, 81, 20));
    labelUp->setAlignment(Qt::AlignCenter);
    labelUp->setText("Channel Up");

    channelBoxUp = new QComboBox(SplitBox);
    channelBoxUp->setObjectName(QString::fromUtf8("channelBoxUp"));
    channelBoxUp->setGeometry(QRect(dx1, 40, 81, 31));
    TOOLTIP(channelBoxUp, "Select the MIDI Channel for 'UP'\n"
                             "'--' uses New Events channel");
    channelBoxUp->setFont(font);
    channelBoxUp->addItem("--", -1);

    for(int n = 0; n < 9; n++) {
        channelBoxUp->addItem(QString::asprintf("ch %i", n), n);
    }
    channelBoxUp->setCurrentIndex(_channelUp[cur_pairdev] + 1);

    labelDown = new QLabel(SplitBox);
    labelDown->setObjectName(QString::fromUtf8("labelDown"));
    labelDown->setGeometry(QRect(dx1, 80, 81, 20));
    labelDown->setAlignment(Qt::AlignCenter);
    labelDown->setText("Channel Down");

    channelBoxDown = new QComboBox(SplitBox);
    channelBoxDown->setObjectName(QString::fromUtf8("channelBoxDown"));
    channelBoxDown->setGeometry(QRect(dx1, 100, 81, 31));
    TOOLTIP(channelBoxDown, "Select the MIDI Channel for 'Down'\n"
                                 "'DIS' disable it using the same\n"
                                 "channel of 'Up'");
    channelBoxDown->setFont(font);
    channelBoxDown->addItem("DIS", -1);

    for(int n = 0; n < 9; n++) {
        channelBoxDown->addItem(QString::asprintf("ch %i", n), n);
    }

    channelBoxDown->setCurrentIndex(_channelDown[cur_pairdev] + 1);

    dx1 = 388;

    tlabelUp = new QLabel(SplitBox);
    tlabelUp->setObjectName(QString::fromUtf8("tlabelUp"));
    tlabelUp->setGeometry(QRect(dx1, 20, 71, 20));
    tlabelUp->setAlignment(Qt::AlignCenter);
    tlabelUp->setText("Transpose");

    tspinBoxUp = new QSpinBox(SplitBox);
    tspinBoxUp->setObjectName(QString::fromUtf8("tspinBoxUp"));
    tspinBoxUp->setGeometry(QRect(dx1, 40, 71, 31));
    tspinBoxUp->setFont(font);
    tspinBoxUp->setMinimum(-24);
    tspinBoxUp->setMaximum(24);
    tspinBoxUp->setValue(_transpose_note_up[cur_pairdev]);
    TOOLTIP(tspinBoxUp, "Transposes the position of the notes\n"
                           "from -24 to 24 (24 = 2 octaves).\n"
                            "Notes that exceed the MIDI range are clipped");

    tlabelDown = new QLabel(SplitBox);
    tlabelDown->setObjectName(QString::fromUtf8("tlabelDown"));
    tlabelDown->setGeometry(QRect(dx1, 80, 71, 20));
    tlabelDown->setAlignment(Qt::AlignCenter);
    tlabelDown->setText("Transpose");

    tspinBoxDown = new QSpinBox(SplitBox);
    tspinBoxDown->setObjectName(QString::fromUtf8("tspinBoxDown"));
    tspinBoxDown->setGeometry(QRect(dx1, 100, 71, 31));
    tspinBoxDown->setFont(font);
    tspinBoxDown->setMinimum(-24);
    tspinBoxDown->setMaximum(24);
    tspinBoxDown->setValue(_transpose_note_down[cur_pairdev]);
    TOOLTIP(tspinBoxDown, "Transposes the position of the notes\n"
                               "from -24 to 24 (24 = 2 octaves).\n"
                                "Notes that exceed the MIDI range are clipped");

    dx1 = 472;

    vcheckBoxUp = new QCheckBox(SplitBox);
    vcheckBoxUp->setObjectName(QString::fromUtf8("vcheckBoxUp"));
    vcheckBoxUp->setGeometry(QRect(dx1, 28, 81, 33));
    vcheckBoxUp->setText("Fix Velocity");
    vcheckBoxUp->setChecked(_fixVelUp[cur_pairdev]);
    TOOLTIP(vcheckBoxUp, "Fix velocity notes to 100 (0-127)");

    vcheckBoxDown = new QCheckBox(SplitBox);
    vcheckBoxDown->setObjectName(QString::fromUtf8("vcheckBoxDown"));
    vcheckBoxDown->setGeometry(QRect(dx1, 88, 81, 33));
    vcheckBoxDown->setText("Fix Velocity");
    vcheckBoxDown->setChecked(_fixVelDown[cur_pairdev]);
    TOOLTIP(vcheckBoxDown, "Fix velocity notes to 100 (0-127)");

    achordcheckBoxUp = new QCheckBox(SplitBox);
    achordcheckBoxUp->setObjectName(QString::fromUtf8("achordcheckBoxUp"));
    achordcheckBoxUp->setGeometry(QRect(dx1, 50, 79, 31));
    achordcheckBoxUp->setText("Auto Chord");
    achordcheckBoxUp->setChecked(_autoChordUp[cur_pairdev]);
    TOOLTIP(achordcheckBoxUp, "Select Auto Chord Mode for Up channel");

    achordcheckBoxDown = new QCheckBox(SplitBox);
    achordcheckBoxDown->setObjectName(QString::fromUtf8("achordcheckBoxDown"));
    achordcheckBoxDown->setGeometry(QRect(dx1, 110, 79, 31));
    achordcheckBoxDown->setText("Auto Chord");
    achordcheckBoxDown->setChecked(_autoChordDown[cur_pairdev]);
    TOOLTIP(achordcheckBoxDown, "Select Auto Chord Mode for Down channel");

    dx1 = 559;

    InstButtonUp = new QPushButton(SplitBox);
    InstButtonUp->setObjectName(QString::fromUtf8("InstButtonUp"));
    InstButtonUp->setGeometry(QRect(dx1/*410 + dx1*/, 40, 31, 31));
    InstButtonUp->setIcon(QIcon(":/run_environment/graphics/channelwidget/instrument.png"));
    InstButtonUp->setIconSize(QSize(24, 24));
    TOOLTIP(InstButtonUp, "Select Live Instrument");
    InstButtonUp->setText(QString());

    InstButtonDown = new QPushButton(SplitBox);
    InstButtonDown->setObjectName(QString::fromUtf8("InstButtonDown"));
    InstButtonDown->setGeometry(QRect(dx1, 100, 31, 31));
    InstButtonDown->setIcon(QIcon(":/run_environment/graphics/channelwidget/instrument.png"));
    InstButtonDown->setIconSize(QSize(24, 24));
    TOOLTIP(InstButtonDown, "Select Live Instrument");
    InstButtonDown->setText(QString());

    effectButtonUp = new QPushButton(SplitBox);
    effectButtonUp->setObjectName(QString::fromUtf8("effectButtonUp"));
    effectButtonUp->setGeometry(QRect(dx1 + 40, 40, 31, 31));
    effectButtonUp->setIcon(QIcon(":/run_environment/graphics/channelwidget/sound_effect.png"));
    effectButtonUp->setIconSize(QSize(24, 24));
    effectButtonUp->setText(QString());
    TOOLTIP(effectButtonUp, "Select Sound Effects events to play in Live\n"
                               "These events will not be recorded");
    effectButtonDown = new QPushButton(SplitBox);
    effectButtonDown->setObjectName(QString::fromUtf8("effectButtonDown"));
    effectButtonDown->setGeometry(QRect(dx1 + 40, 100, 31, 31));
    effectButtonDown->setIcon(QIcon(":/run_environment/graphics/channelwidget/sound_effect.png"));
    effectButtonDown->setIconSize(QSize(24, 24));
    effectButtonDown->setText(QString());
    TOOLTIP(effectButtonDown, "Select Sound Effects events to play in Live\n"
                               "These events will not be recorded");

    chordButtonUp = new QPushButton(SplitBox);
    chordButtonUp->setObjectName(QString::fromUtf8("chordButtonUp"));
    chordButtonUp->setGeometry(QRect(dx1 + 80, 40, 31, 31));
    chordButtonUp->setIcon(QIcon(":/run_environment/graphics/tool/meter.png"));
    chordButtonUp->setIconSize(QSize(24, 24));
    chordButtonUp->setText(QString());
    TOOLTIP(chordButtonUp, "Select the chord for auto chord mode");

    connect(chordButtonUp, SIGNAL(clicked()), this, SLOT(setChordDialogUp()));

    chordButtonDown = new QPushButton(SplitBox);
    chordButtonDown->setObjectName(QString::fromUtf8("chordButtonDown"));
    chordButtonDown->setGeometry(QRect(dx1 + 80, 100, 31, 31));
    chordButtonDown->setIcon(QIcon(":/run_environment/graphics/tool/meter.png"));
    chordButtonDown->setIconSize(QSize(24, 24));
    chordButtonDown->setText(QString());
    TOOLTIP(chordButtonDown, "Select the chord for auto chord mode");

    connect(chordButtonDown, SIGNAL(clicked()), this, SLOT(setChordDialogDown()));

    checkBoxWait = new QCheckBox(MIDIin2);
    checkBoxWait->setObjectName(QString::fromUtf8("checkBoxWait"));
    checkBoxWait->setStyleSheet(QString::fromUtf8("QToolTip {color: black;} \n"));
    checkBoxWait->setGeometry(QRect(30, 634 - py - 10, 241, 17));
    checkBoxWait->setChecked(_record_waits);
    checkBoxWait->setText("Recording starts when one key is pressed");
    TOOLTIP(checkBoxWait, "Waits for a key pressed on the MIDI keyboard\n"
                             "to start the recording");


    //////////////////////////////////////////////////////////////////

    QGroupBox *PedalBox = new QGroupBox(MIDIin2);
    PedalBox->setObjectName(QString::fromUtf8("PedalBox"));

    PedalBox->setStyleSheet(style1);

    PedalBox->setGeometry(QRect(30, 20 + yyy + 160, 790 - 60, 151));
    PedalBox->setCheckable(false);
    PedalBox->setTitle("Pedals");
    TOOLTIP(PedalBox, "Midi events pedal sustain and expression input test");


    int px = 64;

    sustainUP = new QPedalE(PedalBox, "Sustain UP");
    sustainUP->setObjectName(QString::fromUtf8("sustainUP"));
    sustainUP->setGeometry(QRect(px, 9, 112, 135));
    px+= 120;

    expressionUP = new QPedalE(PedalBox, "Expression UP", true);
    expressionUP->setObjectName(QString::fromUtf8("expressionUP"));
    expressionUP->setGeometry(QRect(px, 9, 112, 135));
    px+= 240;

    sustainDOWN = new QPedalE(PedalBox, "Sustain DOWN");
    sustainDOWN->setObjectName(QString::fromUtf8("sustainDOWN"));
    sustainDOWN->setGeometry(QRect(px, 9, 112, 135));
    px+= 120;

    expressionDOWN = new QPedalE(PedalBox, "Expression DOWN", true);
    expressionDOWN->setObjectName(QString::fromUtf8("expressionDOWN"));
    expressionDOWN->setGeometry(QRect(px, 9, 112, 135));
    px+= 120;

    sustainUP->setVal(-1, MidiInControl::invSustainUP[cur_pairdev]);
    expressionUP->setVal(-1, MidiInControl::invExpressionUP[cur_pairdev], MidiInControl::scalex2ExpressionUP[cur_pairdev]);
    sustainDOWN->setVal(-1, MidiInControl::invSustainDOWN[cur_pairdev]);
    expressionDOWN->setVal(-1, MidiInControl::invExpressionDOWN[cur_pairdev], MidiInControl::scalex2ExpressionDOWN[cur_pairdev]);

    connect(sustainUP, &QPedalE::isChecked, this, [=](bool checked)
    {
        MidiInControl::invSustainUP[cur_pairdev] = checked;
        _settings->setValue("MIDIin/MIDIin_invSustainUP" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), MidiInControl::invSustainUP[cur_pairdev]);
    });

    connect(expressionUP, &QPedalE::isChecked, this, [=](bool checked)
    {
        MidiInControl::invExpressionUP[cur_pairdev] = checked;
        _settings->setValue("MIDIin/MIDIin_invExpressionUP" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), MidiInControl::invExpressionUP[cur_pairdev]);
    });

    connect(expressionUP, &QPedalE::scalex2Checked, this, [=](bool checked)
    {
        MidiInControl::scalex2ExpressionUP[cur_pairdev] = checked;
        _settings->setValue("MIDIin/MIDIin_scalex2ExpressionUP" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), MidiInControl::scalex2ExpressionUP[cur_pairdev]);
    });

    connect(sustainDOWN, &QPedalE::isChecked, this, [=](bool checked)
    {
        MidiInControl::invSustainDOWN[cur_pairdev] = checked;
        _settings->setValue("MIDIin/MIDIin_invSustainDOWN" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), MidiInControl::invSustainDOWN[cur_pairdev]);
    });

    connect(expressionDOWN, &QPedalE::isChecked, this, [=](bool checked)
    {
        MidiInControl::invExpressionDOWN[cur_pairdev] = checked;
        _settings->setValue("MIDIin/MIDIin_invExpressionDOWN" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), MidiInControl::invExpressionDOWN[cur_pairdev]);
    });

    connect(expressionDOWN, &QPedalE::scalex2Checked, this, [=](bool checked)
    {
        MidiInControl::scalex2ExpressionDOWN[cur_pairdev] = checked;
        _settings->setValue("MIDIin/MIDIin_scalex2ExpressionDOWN" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), MidiInControl::scalex2ExpressionDOWN[cur_pairdev]);
    });


    QGroupBox *seqBox = new QGroupBox(MIDIin2);
    seqBox->setObjectName(QString::fromUtf8("seqBox"));

    seqBox->setStyleSheet(style1);

    seqBox->setGeometry(QRect(30, 20 + yyy + 320, 790 - 60, 64));
    seqBox->setCheckable(false);
    seqBox->setTitle("Sequencer");
    TOOLTIP(seqBox, "Options to load sequencers when launching Midieditor\n"
                    "or when selecting/connecting Midi devices.\n\n"
                    "You can choose to load manually by pressing\n"
                    "'LOAD SEQUENCERS' button or unload the sequencers and\n"
                    "free memory with 'UNLOAD SEQUENCERS'");

    seqSel = new QComboBox(seqBox);
    seqSel->setObjectName(QString::fromUtf8("seqSel"));
    seqSel->setGeometry(QRect(9, 16, 460, 32));
    TOOLTIP(seqSel, "");
    QFont font4;
    font4.setPointSize(8);
    seqSel->setFont(font4);
    seqSel->addItem("Sequencer is not loaded automatically. Press 'LOAD SEQUENCERS' for connected devices", 0);
    seqSel->addItem("Sequencer is not loaded automatically. Press 'LOAD SEQUENCERS' for load in all devices", 1);
    seqSel->addItem("Sequencer is loaded automatically when you connect a device", 2);
    seqSel->addItem("All sequencers are loaded at the start of Midieditor", 3);

    seqSel->setCurrentIndex(MidiInput::loadSeq_mode);

    connect(seqSel, QOverload<int>::of(&QComboBox::activated), this, [=](int v)
    {
        MidiInput::loadSeq_mode = v;
        _settings->setValue("MIDIin/LoadSeq_mode", v);
    });

    QPushButton *seqLoad = new QPushButton(seqBox);
    seqLoad->setObjectName(QString::fromUtf8("seqLoad"));
    seqLoad->setGeometry(QRect(474, 16, 120, 32));
    seqLoad->setFont(font4);
    seqLoad->setText("LOAD SEQUENCERS");
    TOOLTIP(seqLoad, "Load sequencers manually based on selected option");

    connect(seqLoad, QOverload<bool>::of(&QPushButton::clicked), [=](bool)
    {
        for(int pairdev = 0; pairdev < MAX_INPUT_PAIR; pairdev++) {

            if((MidiInput::loadSeq_mode == 0 || MidiInput::loadSeq_mode == 2) &&
               !MidiInput::isConnected(pairdev * 2) && !MidiInput::isConnected(pairdev * 2 + 1)) {
                   MidiInControl::sequencer_unload(pairdev);
                   continue;
                }


            MidiInControl::sequencer_load(pairdev);

        }

    });

    QPushButton *seqUnLoad = new QPushButton(seqBox);
    seqUnLoad->setObjectName(QString::fromUtf8("seqUnLoad"));
    seqUnLoad->setGeometry(QRect(602, 16, 120, 32));
    seqUnLoad->setFont(font4);
    seqUnLoad->setText("UNLOAD SEQUENCERS");
    TOOLTIP(seqUnLoad, "Unload sequencers manually based on selected option");

    connect(seqUnLoad, QOverload<bool>::of(&QPushButton::clicked), [=](bool)
    {

        for(int pairdev = 0; pairdev < MAX_INPUT_PAIR; pairdev++) {
            MidiInControl::sequencer_unload(pairdev);
        }

    });



    //////////////////////////////////////////////////////////////////
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    connect(SplitBox, SIGNAL(clicked(bool)), this, SLOT(set_split_enable(bool)));
    connect(inchannelBoxUp, SIGNAL(activated(int)), this, SLOT(set_inchannelUp(int)));
    connect(inchannelBoxDown, SIGNAL(activated(int)), this, SLOT(set_inchannelDown(int)));
    connect(channelBoxUp, SIGNAL(activated(int)), this, SLOT(set_channelUp(int)));
    connect(channelBoxDown, SIGNAL(activated(int)), this, SLOT(set_channelDown(int)));
    connect(vcheckBoxUp, SIGNAL(clicked(bool)), this, SLOT(set_fixVelUp(bool)));
    connect(vcheckBoxDown, SIGNAL(clicked(bool)), this, SLOT(set_fixVelDown(bool)));

    connect(achordcheckBoxUp, SIGNAL(clicked(bool)), this, SLOT(set_autoChordUp(bool)));
    connect(achordcheckBoxDown, SIGNAL(clicked(bool)), this, SLOT(set_autoChordDown(bool)));

    connect(echeckBox, SIGNAL(clicked(bool)), this, SLOT(set_notes_only(bool)));
    //connect(echeckBoxDown, SIGNAL(clicked(bool)), this, SLOT(set_events_to_down(bool)));
#ifdef USE_FLUIDSYNTH
    //connect(fluid_output, SIGNAL(MidiIn_set_events_to_down(bool)), echeckBoxDown, SLOT(setChecked(bool)));
#endif

    connect(tspinBoxUp, SIGNAL(valueChanged(int)), this, SLOT(set_transpose_note_up(int)));
    connect(tspinBoxDown, SIGNAL(valueChanged(int)), this, SLOT(set_transpose_note_down(int)));

    connect(rstButton, SIGNAL(clicked()), this, SLOT(split_reset()));
    connect(PanicButton, SIGNAL(clicked()), this, SLOT(panic_button()));
    connect(InstButtonUp, SIGNAL(clicked()), this, SLOT(select_instrumentUp()));
    connect(InstButtonDown, SIGNAL(clicked()), this, SLOT(select_instrumentDown()));
    connect(effectButtonUp, SIGNAL(clicked()), this, SLOT(select_SoundEffectUp()));
    connect(effectButtonDown, SIGNAL(clicked()), this, SLOT(select_SoundEffectDown()));

    connect(checkBoxPrgBank, SIGNAL(clicked(bool)), this, SLOT(set_skip_prgbanks(bool)));
    connect(bankskipcheckBox, SIGNAL(clicked(bool)), this, SLOT(set_skip_bankonly(bool)));
    connect(checkBoxWait, SIGNAL(clicked(bool)), this, SLOT(set_record_waits(bool)));

    connect(NoteBoxCut, QOverload<int>::of(&QComboBox::activated), [=](int v)
    {
       int nitem = NoteBoxCut->itemData(v).toInt();
       if(nitem == -1) {

           _current_chan = MidiInControl::inchannelUp(cur_pairdev);

           _current_device = cur_pairdev * 2;

           set_current_note(-1);
           int note = get_key();
           if(note >= 0) {

               while(NoteBoxCut->count() > 1) {
                   NoteBoxCut->removeItem(1);
               }

               NoteBoxCut->insertItem(1, QString::asprintf("%s %i", notes[note % 12], note / 12 - 1), note);
               NoteBoxCut->setCurrentIndex(1);
               NoteBoxCut->addItem("DUO", -2);
               NoteBoxCut->addItem("C -1", 0);
               set_note_cut(note);

           } else {
               if(NoteBoxCut->itemData(1).toInt() >= 0)  NoteBoxCut->removeItem(1);
               set_note_cut(-1);
           }
           _settings->setValue("MIDIin/MIDIin_note_cut" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), _note_cut[cur_pairdev]);
           _note_duo[cur_pairdev] = false;
           _note_zero[cur_pairdev] = false;
       } else if(nitem == -2) {
           _note_duo[cur_pairdev] = true;
           _note_zero[cur_pairdev] = false;
       }
       else {
           _note_duo[cur_pairdev] = false;
           _note_zero[cur_pairdev] = false;

           if(nitem == 0) _note_zero[cur_pairdev] = true;
           else _note_cut[cur_pairdev] = nitem;
       }

       _settings->setValue("MIDIin/MIDIin_note_duo" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), _note_duo[cur_pairdev]);
       _settings->setValue("MIDIin/MIDIin_note_zero" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), _note_zero[cur_pairdev]);

    });


    time_update= new QTimer(this);
    time_update->setSingleShot(false);

    connect(time_update, SIGNAL(timeout()), this, SLOT(update_checks()), Qt::DirectConnection);
    time_update->setSingleShot(false);
    time_update->start(50); //flip

    QMetaObject::connectSlotsByName(MIDIin);
    first = 0;
}

void MidiInControl::reject() {

    hide();
}

void MidiInControl::accept() {

    hide();
}

MidiInControl::~MidiInControl() {

    if(!osd_dialog) {
        delete osd_dialog;
        osd_dialog = NULL;
    }

    time_update->stop();
    delete time_update;
    qWarning("MidiInControl() destructor");
}

void MidiInControl::MidiIn_toexit(MidiInControl *MidiIn) {
    MidiIn_ctrl = MidiIn;
}

void MidiInControl::my_exit() {
    if(file_live) delete file_live;
    file_live = NULL;
    if(MidiIn_ctrl) delete MidiIn_ctrl;
    MidiIn_ctrl = NULL;
}

void MidiInControl::split_reset() {

    int r = QMessageBox::question(this, "Reset Presets", "This action resets MidiInput to default options for these devices.\n\nAre you sure?                         ");
    if(r != QMessageBox::Yes) return;

    _split_enable[cur_pairdev] = true;
    _note_cut[cur_pairdev] = -1;
    _note_duo[cur_pairdev] = false;
    _note_zero[cur_pairdev] = false;

    _inchannelUp[cur_pairdev] = 0;
    _inchannelDown[cur_pairdev] = 0;
    _channelUp[cur_pairdev] =  (!cur_pairdev) ? -1 : cur_pairdev * 2;
    _channelDown[cur_pairdev] = (!cur_pairdev) ? -1 : cur_pairdev * 2 + 1;

    _fixVelUp[cur_pairdev] = false;
    _fixVelDown[cur_pairdev] = false;
    _autoChordUp[cur_pairdev] = false;
    _autoChordDown[cur_pairdev] = false;
    _notes_only[cur_pairdev] = true;

    _transpose_note_up[cur_pairdev] = 0;
    _transpose_note_down[cur_pairdev] = 0;
    _skip_prgbanks[cur_pairdev] = false;
    _skip_bankonly[cur_pairdev] = true;
    _record_waits = true;

    SplitBox->setChecked(_split_enable[cur_pairdev]);

    while(NoteBoxCut->count() > 1) {
        NoteBoxCut->removeItem(1);
    }

    NoteBoxCut->addItem("DUO", -2);
    NoteBoxCut->addItem("C -1", 0);
    NoteBoxCut->setCurrentIndex(0);

    inchannelBoxUp->setCurrentIndex(_inchannelUp[cur_pairdev] + 1);
    inchannelBoxDown->setCurrentIndex(_inchannelDown[cur_pairdev] + 1);
    channelBoxUp->setCurrentIndex(_channelUp[cur_pairdev] + 1);
    channelBoxDown->setCurrentIndex(_channelDown[cur_pairdev] + 1);
    vcheckBoxUp->setChecked(_fixVelUp[cur_pairdev]);
    vcheckBoxDown->setChecked(_fixVelDown[cur_pairdev]);
    achordcheckBoxUp->setChecked(_autoChordUp[cur_pairdev]);
    achordcheckBoxDown->setChecked(_autoChordDown[cur_pairdev]);
    echeckBox->setChecked(_notes_only[cur_pairdev]);
    //echeckBoxDown->setChecked(_events_to_down);
    tspinBoxUp->setValue(_transpose_note_up[cur_pairdev]);
    tspinBoxDown->setValue(_transpose_note_down[cur_pairdev]);
    checkBoxPrgBank->setChecked(_skip_prgbanks[cur_pairdev]);
    bankskipcheckBox->setChecked(_skip_bankonly[cur_pairdev]);
    checkBoxWait->setChecked(_record_waits);

    MidiInControl::invSustainUP[cur_pairdev] = false;
    MidiInControl::invExpressionUP[cur_pairdev] = false;
    MidiInControl::scalex2ExpressionUP[cur_pairdev] = false;
    MidiInControl::invSustainDOWN[cur_pairdev] = false;
    MidiInControl::invExpressionDOWN[cur_pairdev] = false;
    MidiInControl::scalex2ExpressionDOWN[cur_pairdev] = false;

    sustainUP->setVal(-1, MidiInControl::invSustainUP[cur_pairdev]);
    expressionUP->setVal(-1, MidiInControl::invExpressionUP[cur_pairdev], MidiInControl::scalex2ExpressionUP[cur_pairdev]);
    sustainDOWN->setVal(-1, MidiInControl::invSustainDOWN[cur_pairdev]);
    expressionDOWN->setVal(-1, MidiInControl::invExpressionDOWN[cur_pairdev], MidiInControl::scalex2ExpressionDOWN[cur_pairdev]);

    if(file_live) delete file_live;
    file_live = NULL;

    panic_button();

    led_up[cur_pairdev] = 0;
    led_down[cur_pairdev] = 0;
    update_checks();

    if(!MidiInput::recording())
        MidiInput::cleanKeyMaps();

    update();
}

void MidiInControl::panic_button() {

    int dev1 = cur_pairdev * 2;

    seqOn(dev1, -1, false);
    seqOn(dev1 + 1, -1, false);

    FingerPatternDialog::Finger_Action(dev1, 1, true, 1);
    FingerPatternDialog::Finger_Action(dev1, 2, true, 1);
    FingerPatternDialog::Finger_Action(dev1, 3, true, 1);
    FingerPatternDialog::Finger_Action(dev1, 33, true, 1);
    FingerPatternDialog::Finger_Action(dev1, 34, true, 1);
    FingerPatternDialog::Finger_Action(dev1, 35, true, 1);

    msDelay(200);

    MidiPlayer::panic();
    send_live_events();  // send live event effects

    led_up[cur_pairdev] = 0;
    led_down[cur_pairdev] = 0;
    if(!MidiInput::recording())
        MidiInput::cleanKeyMaps();
    update_checks();
}

void MidiInControl::send_live_events() {

    if(file_live) {
        MidiFile * file = MidiOutput::file;
        int index = file ? (file->MultitrackMode ? file->track(NewNoteTool::editTrack())->device_index() : 0) : 0;

        MidiInput::track_index = index;

        for(int n = 0; n < 16; n++) {
            foreach (MidiEvent* event, file_live->channel(n)->eventMap()->values()) {
                MidiOutput::sendCommand(event, index);
            }
        }

    }
}

void MidiInControl::select_instrumentUp() {


    int ch = ((MidiInControl::channelUp(cur_pairdev) < 0)
              ? MidiOutput::standardChannel()
              : (MidiInControl::channelUp(cur_pairdev) & 15));


    if(!file_live) {
        file_live = new MidiFile();
    }

    MidiFile * file = MidiInput::file;
    if(!file) return;
    InstrumentChooser* d = new InstrumentChooser(NULL, ch, file->track(NewNoteTool::editTrack()), this); // no MidiFile()
    d->setModal(true);
    d->exec();
    delete d;
    send_live_events();
}

void MidiInControl::select_SoundEffectUp()
{
    int ch = ((MidiInControl::channelUp(cur_pairdev) < 0)
              ? MidiOutput::standardChannel()
              : (MidiInControl::channelUp(cur_pairdev) & 15));

    if(!file_live) {
        file_live = new MidiFile();
    }

    SoundEffectChooser* d = new SoundEffectChooser(file_live, ch, this, SOUNDEFFECTCHOOSER_FORMIDIIN);
    d->exec();
    delete d;
    send_live_events();
}

void MidiInControl::select_instrumentDown() {

    int ch = ((MidiInControl::channelDown(cur_pairdev) < 0)
              ? ((MidiInControl::channelUp(cur_pairdev) < 0)
                 ? MidiOutput::standardChannel()
                 : (MidiInControl::channelUp(cur_pairdev) & 15))
              : (MidiInControl::channelDown(cur_pairdev) & 15));


    if(!file_live) {
        file_live = new MidiFile();
    }

    MidiFile * file = MidiInput::file;
    InstrumentChooser* d = new InstrumentChooser(NULL, ch, file->track(NewNoteTool::editTrack()), this); // no MidiFile()

    d->setModal(true);
    d->exec();
    delete d;
    send_live_events();
}

void MidiInControl::select_SoundEffectDown()
{
    int ch = ((MidiInControl::channelDown(cur_pairdev) < 0)
              ? ((MidiInControl::channelUp(cur_pairdev) < 0)
                 ? MidiOutput::standardChannel()
                 : (MidiInControl::channelUp(cur_pairdev) & 15))
              : (MidiInControl::channelDown(cur_pairdev) & 15));


    if(!file_live) {
        file_live = new MidiFile();
    }

    SoundEffectChooser* d = new SoundEffectChooser(file_live, ch, this, SOUNDEFFECTCHOOSER_FORMIDIIN, 0);
    d->exec();
    delete d;
    send_live_events();
}


void MidiInControl::set_prog(int channel, int value) {
    if(channel >= 16)
        return;
    MidiFile * file = MidiOutput::file;
    int index = file->MultitrackMode ? file->track(NewNoteTool::editTrack())->device_index(): 0;
    file->Prog_MIDI[channel + 4 * (index != 0) + 16 * index] = value;
}

void MidiInControl::set_bank(int channel, int value) {
    if(channel >= 16)
        return;
    MidiFile * file = MidiOutput::file;
    int index = file->MultitrackMode ? file->track(NewNoteTool::editTrack())->device_index(): 0;
    file->Bank_MIDI[channel + 4 * (index != 0) + 16 * index] = value;
}

void MidiInControl::set_output_prog_bank_channel(int channel) {

    if(channel >= 16)
        return;
    ProgChangeEvent* pevent;
    ControlChangeEvent* cevent;
    MidiFile * file = MidiOutput::file;
    int index = file->MultitrackMode ? file->track(NewNoteTool::editTrack())->device_index() : 0;
    pevent = new ProgChangeEvent(channel, file->Prog_MIDI[channel + 4 * (index != 0) + 16 * index], 0);
    cevent = new ControlChangeEvent(channel, 0x0, file->Bank_MIDI[channel + 4 * (index != 0) + 16 * index], 0);
    MidiOutput::sendCommandDelete(cevent, index);
    MidiOutput::sendCommandDelete(pevent, index);
}

static QMessageBox *mb2 = NULL;
static QMessageBox *mb3 = NULL;

static int get_key_mode = 0;

int MidiInControl::get_key(int chan, int dev) {
    _current_chan = chan;

    if(dev == -1)
        _current_device = cur_pairdev * 2;
    else
        _current_device = dev;

    return get_key();
}

int MidiInControl::get_key() {

    mb2 = NULL;
    mb3 = NULL;
    get_key_mode = 0;
    set_current_note(-1);

    QMessageBox *mb = new QMessageBox(_parent);
    mb->setWindowTitle("MIDI Input Control");
    mb->setText("Press a note in the keyboard");
    mb->setIcon(QMessageBox::Information);
    mb->setStandardButtons(QMessageBox::Cancel);

    QFont font;
    font.setPixelSize(24);
    mb->setFont(font);
    mb->setIconPixmap(QPixmap(":/run_environment/graphics/channelwidget/instrument.png"));
    //mb->button(QMessageBox::Cancel)->animateClick(10000);
    QTimer::singleShot(10000, mb->button(QMessageBox::Cancel), &QAbstractButton::click);

    //mb->exec();
    mb->setModal(false);
    mb->show();
    mb2 = mb;
    while(1) {
        if(!mb2 || mb->isHidden())
            break;
        QCoreApplication::processEvents();
        QThread::msleep(5);
    }
    mb2 = NULL;
    delete mb;

    for(int track_index = 0; track_index < 3; track_index++) {
        for(int n = 0; n < 16; n++) {
            QByteArray array;
            array.clear();
            array.append(0xB0 | n);
            array.append(char(123)); // all notes off
            array.append(char(127));

            MidiOutput::sendCommand(array, track_index);
        }
    }

    return current_note();
}

int MidiInControl::get_ctrl() {

    mb2 = NULL;
    mb3 = NULL;
    _current_ctrl = -1;

    QMessageBox *mb = new QMessageBox(_parent);
    mb->setWindowTitle("MIDI Input Control");
    mb->setText("Press/use a Control Change in the Midi Device");
    mb->setIcon(QMessageBox::Information);
    mb->setStandardButtons(QMessageBox::Cancel);

    QFont font;
    font.setPixelSize(24);
    mb->setFont(font);
    mb->setIconPixmap(QPixmap(":/run_environment/graphics/channelwidget/instrument.png"));
    //mb->button(QMessageBox::Cancel)->animateClick(10000);
    QTimer::singleShot(10000, mb->button(QMessageBox::Cancel), &QAbstractButton::click);

    //mb->exec();
    mb->setModal(false);
    mb->show();

    mb3 = mb;

    while(1) {
        if(!mb3 || mb->isHidden())
            break;
        QCoreApplication::processEvents();
        QThread::msleep(5);
    }
    mb3 = NULL;
    delete mb;

    return _current_ctrl;
}

static int _ret_wait = 555;

int MidiInControl::wait_record(QWidget *parent) {

    if(!_record_waits) {_ret_wait = 0; return 0; }

    _ret_wait = 555;

    _current_chan = 16;
    _current_device = -1;

    get_key_mode = 1;
    set_current_note(-1);

    QMessageBox *mb = new QMessageBox(parent);
    mb->setWindowTitle("MIDI Input Control");
    mb->setText("Press a note in the keyboard\nto start the recording");
    mb->setIcon(QMessageBox::Information);
    mb->setStandardButtons(QMessageBox::Cancel);

    QFont font;
    font.setPixelSize(24);
    mb->setFont(font);
    mb->setIconPixmap(QPixmap(":/run_environment/graphics/channelwidget/instrument.png"));

    //mb->exec();
    mb->setModal(false);
    mb->show();
    mb2 = mb;
    while(1) {
        if(!mb2 || mb->isHidden())
            break;
        QCoreApplication::processEvents();
        QThread::msleep(5);
    }
    mb2 = NULL;
    delete mb;
    _ret_wait = current_note();
    return _ret_wait;
}

int MidiInControl::wait_record_thread() {
    if(!_record_waits) return 0;

    int *v= &_ret_wait;
    while(*v == 555) {
        QCoreApplication::processEvents();
        QThread::msleep(1);
    }
    return _ret_wait;
}

int MidiInControl::skip_key() {
    return skip_keys != 0;
}

int MidiInControl::set_key(int key, int evt, int device) {
    if(mb2 == NULL) return 0;
/*
    if(get_key_mode == 0 && (evt & 0xf0) == 0x90) return 0;
    if(get_key_mode == 1 && (evt & 0xf0) == 0x80) return 0;
    */
    if((evt & 0xf) != _current_chan && _current_chan != 16  && _current_chan != -1) return 0;

    if(device != _current_device && _current_device != -1) return 0;

    if(_current_chan == 16)
        _current_chan = (evt & 0xf);

    _current_note = key;
    /*
    if(mb2 != NULL)
        mb2->hide();
*/
    mb2 = NULL;
    QThread::msleep(30); // very important sleep()
    if(get_key_mode == 1)
        return 0;
    else
        skip_keys = 40;
    return 1;

}

int MidiInControl::set_ctrl(int key, int evt, int device) {
    if(mb3 == NULL) return 0;

    if((evt & 0xf0) != 0xb0) return 0;
    if((evt & 0xf) != _current_chan && _current_chan != 16) return 0;
    if(device != _current_device && _current_device != -1) return 0;

    if(_current_chan == 16)
        _current_chan = (evt & 0xf);
    _current_ctrl = key;
/*
    if(mb3 != NULL)
        mb3->hide();
*/
    mb3 = NULL;
    QThread::msleep(30); // very important sleep()
    skip_keys = 40;

    return 1;

}

bool MidiInControl::split_enable(int dev) {
    return _split_enable[dev];
}

int MidiInControl::note_cut(int dev) {
    return ((_note_zero[dev] || MidiInput::keyboard2_connected[dev * 2 + 1]) ? 0 : _note_cut[dev]);
}

bool MidiInControl::note_duo(int dev) {
    return _note_duo[dev];
}

int MidiInControl::inchannelUp(int dev) {
    return _inchannelUp[dev];
}

int MidiInControl::inchannelDown(int dev) {
    return _inchannelDown[dev];
}

int MidiInControl::channelUp(int dev) {
    return _channelUp[dev];
}

int MidiInControl::channelDown(int dev) {
    return _channelDown[dev];
}

bool MidiInControl::fixVelUp(int dev) {
    return _fixVelUp[dev];
}

bool MidiInControl::fixVelDown(int dev) {
    return _fixVelDown[dev];
}

bool MidiInControl::autoChordUp(int dev) {
    return _autoChordUp[dev];
}

bool MidiInControl::autoChordDown(int dev) {
    return _autoChordDown[dev];
}

int MidiInControl::current_note() {
    return _current_note;
}

bool MidiInControl::skip_prgbanks(int dev) {
    return _skip_prgbanks[dev];
}

bool MidiInControl::skip_bankonly(int dev) {
    return _skip_bankonly[dev];
}

bool MidiInControl::notes_only(int dev) {
    return _notes_only[dev];
}

int MidiInControl::transpose_note_up(int dev) {
    return _transpose_note_up[dev];
}

int MidiInControl::transpose_note_down(int dev) {
    return _transpose_note_down[dev];
}

////////////////////////////////////////////

void MidiInControl::set_note_cut(int v) {
    _note_cut[cur_pairdev] = v;
}

void MidiInControl::set_current_note(int v){
    _current_note = v;
}

// slots

void MidiInControl::set_split_enable(bool v) {

    _split_enable[cur_pairdev]= v;
    _settings->setValue("MIDIin/MIDIin_split_enable" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), _split_enable[cur_pairdev]);

}

void MidiInControl::set_inchannelUp(int v) {
    _inchannelUp[cur_pairdev] = v - 1;
    _settings->setValue("MIDIin/MIDIin_inchannelUp" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), _inchannelUp[cur_pairdev]);

}

void MidiInControl::set_inchannelDown(int v) {
    _inchannelDown[cur_pairdev] = v - 1;
    _settings->setValue("MIDIin/MIDIin_inchannelDown" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), _inchannelDown[cur_pairdev]);

}

void MidiInControl::set_channelUp(int v) {
    _channelUp[cur_pairdev] = v - 1;
    _settings->setValue("MIDIin/MIDIin_channelUp" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), _channelUp[cur_pairdev]);

}

void MidiInControl::set_channelDown(int v) {
    _channelDown[cur_pairdev] = v - 1;
    _settings->setValue("MIDIin/MIDIin_channelDown" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), _channelDown[cur_pairdev]);

}

void MidiInControl::set_fixVelUp(bool v) {
    _settings->setValue("MIDIin/MIDIin_fixVelUp" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), v);
    _fixVelUp[cur_pairdev] = v;
}

void MidiInControl::set_fixVelDown(bool v) {
    _settings->setValue("MIDIin/MIDIin_fixVelDown" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), v);
    _fixVelDown[cur_pairdev] = v;
}

void MidiInControl::set_autoChordUp(bool v) {

    _settings->setValue("MIDIin/MIDIin_autoChordUp" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), v);
    _autoChordUp[cur_pairdev] = v;
}

void MidiInControl::set_autoChordDown(bool v) {

    _settings->setValue("MIDIin/MIDIin_autoChordDown" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), v);
    _autoChordDown[cur_pairdev] = v;

}

void MidiInControl::set_notes_only(bool v) {
    _settings->setValue("MIDIin/MIDIin_notes_only" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), v);
    _notes_only[cur_pairdev] = v;
}

void MidiInControl::set_transpose_note_up(int v) {
    _settings->setValue("MIDIin/MIDIin_transpose_note_up" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), v);
    _transpose_note_up[cur_pairdev] = v;
}

void MidiInControl::set_transpose_note_down(int v) {
    _settings->setValue("MIDIin/MIDIin_transpose_note_down" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), v);
    _transpose_note_down[cur_pairdev] = v;
}

void MidiInControl::set_skip_prgbanks(bool v) {
    _settings->setValue("MIDIin/MIDIin_skip_prgbanks" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), v);
    _skip_prgbanks[cur_pairdev] = v;
}

void MidiInControl::set_skip_bankonly(bool v) {
    _settings->setValue("MIDIin/MIDIin_skip_bankonly" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), v);
    _skip_bankonly[cur_pairdev] = v;
}

void MidiInControl::set_leds(int dev, bool up, bool down, bool up_err, bool down_err) {
    if(up) led_up[dev] = 7;
    if(down) led_down[dev] = 7;
    if(up_err) led_up[dev] = -7;
    if(down_err) led_down[dev] = -7;
}

// from timer, set LEDs, ecc



void MidiInControl::update_checks() {
    /*
    static int flip = 0;
    flip++;
*/

    if(skip_keys)
        skip_keys--;

    if(MidiInput::keyboard2_connected[cur_pairdev * 2 + 1])
        NoteBoxCut->setDisabled(true);
    else
        NoteBoxCut->setDisabled(false);

    achordcheckBoxUp->setChecked(_autoChordUp[cur_pairdev]);
    achordcheckBoxDown->setChecked(_autoChordDown[cur_pairdev]);

    if(MidiInControl::sustainUPval >= 0) {
        int val = MidiInControl::sustainUPval;
        MidiInControl::sustainUPval = -1;
        sustainUP->setVal(val, MidiInControl::invSustainUP[cur_pairdev]);
    }

    if(MidiInControl::expressionUPval >= 0) {
        int val = MidiInControl::expressionUPval;
        MidiInControl::expressionUPval = -1;
        expressionUP->setVal(val, MidiInControl::invExpressionUP[cur_pairdev], MidiInControl::scalex2ExpressionUP[cur_pairdev]);
    }

    if(MidiInControl::sustainDOWNval >= 0) {
        int val = MidiInControl::sustainDOWNval;
        MidiInControl::sustainDOWNval = -1;
        sustainDOWN->setVal(val, MidiInControl::invSustainDOWN[cur_pairdev]);
    }

    if(MidiInControl::expressionDOWNval >= 0) {
        int val = MidiInControl::expressionDOWNval;
        MidiInControl::expressionDOWNval = -1;
        expressionDOWN->setVal(val, MidiInControl::invExpressionDOWN[cur_pairdev], MidiInControl::scalex2ExpressionDOWN[cur_pairdev]);
    }

    for(int pairdev = 0; pairdev < MAX_INPUT_PAIR; pairdev++) {

        if(MidiInput::isConnected(pairdev * 2) && !MIDIin->isEnabled()) {

            if(!MIDI_INPUT->count()) {

                int n = 0;

                foreach (QString name, MidiInput::inputPorts(pairdev * 2)) {

                    if(name != MidiInput::inputPort(pairdev * 2))
                        continue;

                    MIDI_INPUT->addItem(name);
                    MIDI_INPUT->setCurrentIndex(n);

                    break;

                    n++;
                }

            }

            if(tabMIDIin1)
                tabMIDIin1->setDisabled(false);
        } else if(!MidiInput::isConnected(pairdev * 2) && MIDIin->isEnabled()) {

           /* if(tabMIDIin1)
                tabMIDIin1->setDisabled(true);*/

        }

        if(MidiInput::isConnected(pairdev * 2 + 1) && !MIDIin->isEnabled()) {

            if(!MIDI_INPUT2->count()) {

                int n = 0;

                foreach (QString name, MidiInput::inputPorts(pairdev * 2 + 1)) {

                    if(name != MidiInput::inputPort(pairdev * 2 + 1))
                        continue;

                    MIDI_INPUT2->addItem(name);
                    MIDI_INPUT2->setCurrentIndex(n);

                    break;

                    n++;
                }

            }

            if(tabMIDIin1)
                tabMIDIin1->setDisabled(false);

        } else if(!MidiInput::isConnected(pairdev * 2 + 1) && MIDIin->isEnabled()) {

        /* if(tabMIDIin1)
             tabMIDIin1->setDisabled(true);*/

    }

        if(led_up[pairdev]) {
            if(pairdev == this->cur_pairdev) {
                if(led_up[pairdev] < 0)
                    LEDBoxUp->setLed(QColor(0xff, 0x10, 0x00));
                else
                    LEDBoxUp->setLed(QColor(0x60, 0xff, 0x00));
            }

            if(led_up[pairdev] < 0)
                led_up[pairdev]++;
            else
                led_up[pairdev]--;

        } else if(pairdev == this->cur_pairdev)
                    LEDBoxUp->setLed(QColor(0x00, 0x80, 0x10));

        if(led_down[pairdev]) {

            if(pairdev == this->cur_pairdev) {

                if(led_down[pairdev] < 0)
                    LEDBoxDown->setLed(QColor(0xff, 0x10, 0x00));
                else
                    LEDBoxDown->setLed(QColor(0x60, 0xff, 0x00));
            }

            if(led_down[pairdev] < 0)
                led_down[pairdev]++;
            else
                led_down[pairdev]--;


        } else if(pairdev == this->cur_pairdev)
                    LEDBoxDown->setLed(QColor(0x00, 0x80, 0x10));
    }
   // update();
}

void MidiInControl::set_record_waits(bool v) {
    _settings->setValue("MIDIin/MIDIin_record_waits", v);
    _record_waits = v;
}

////////////////////////////////////////////////////////////////////////////////////
// auto chord section

extern int chord_noteM(int note, int position);
extern int chord_notem(int note, int position);

static int buildCMajorProg(int index, int note) {

    switch(index) {
        case 0:
            note = note/12 * 12 + chord_noteM(note % 12, 1);
        break;
        case 1:
            note = note/12 * 12 + chord_noteM(note % 12, 3);
        break;
        case 2:
            note = note/12 * 12 + chord_noteM(note % 12, 5);
        break;
        case 3:
            note = note/12 * 12 + chord_noteM(note % 12, 7);
        break;
        case AUTOCHORD_MAX:
            return 3;
    }

    return note;
}

static int buildCMinorProg(int index, int note) {

    switch(index) {
        case 0:
            note = note/12 * 12 + chord_notem(note % 12, 1);
        break;
        case 1:
            note = note/12 * 12 + chord_notem(note % 12, 3);
        break;
        case 2:
            note = note/12 * 12 + chord_notem(note % 12, 5);
        break;
        case 3:
            note = note/12 * 12 + chord_notem(note % 12, 7);
        break;
        case AUTOCHORD_MAX:
            return 3;
    }

    return note;
}

static int buildPowerChord(int index, int note) {

    switch(index) {
        case 1:
            note += 7;
        break;
        case AUTOCHORD_MAX:
            return 2;
    }

    return note;
}

static int buildPowerChordExt(int index, int note) {

    switch(index) {
        case 1:
            note += 7;
        break;
        case 2:
            note += 12;
        break;
        case AUTOCHORD_MAX:
            return 3;
    }

    return note;
}

static int buildCMajor(int index, int note) {

    switch(index) {
        case 1:
            note += 4;
        break;
        case 2:
            note += 7;
        break;
        case AUTOCHORD_MAX:
            return 3;
    }

    return note;
}

static int buildMajorPentatonic(int index, int note) {
    switch(index) {
        case 1:
            note += 2;
            break;
        case 2:
            note += 4;
            break;
        case 3:
            note += 7;
            break;
        case 4:
            note += 9;
            break;
        case AUTOCHORD_MAX:
            return 5;
    }

    return note;
}

static int buildMinorPentatonic(int index, int note) {
    switch(index) {
        case 1:
            note += 3;
            break;
        case 2:
            note += 5;
            break;
        case 3:
            note += 7;
            break;
        case 4:
            note += 10;
            break;
        case AUTOCHORD_MAX:
            return 5;
    }

    return note;
}

int MidiInControl::GetNoteChord(int type, int index, int note) {
    if(note >= 0 || index == AUTOCHORD_MAX) {
        switch(type) {
            case 0:
                note = buildPowerChord(index, note);
                break;
            case 1:
                note = buildPowerChordExt(index, note);
                break;
            case 2:
                note = buildCMajor(index, note);
                break;
            case 3:
                note = buildCMajorProg(index, note);
                break;
            case 4:
                note = buildCMinorProg(index, note);
                break;
            case 5:
                note = buildMajorPentatonic(index, note);
                break;
            case 6:
                note = buildMinorPentatonic(index, note);
                break;
        }
    }

    if(note < 0) note = 0;
    if(note > 127) note = 127;

    return note;
}

int MidiInControl::autoChordfunUp(int pairdev, int index, int note, int vel) {

    if(!_autoChordUp[pairdev]) { // no chord

        if(index == AUTOCHORD_MAX) return 1;
        if(note >= 0) return note;
        return vel;
    }

    if(note >= 0 || index == AUTOCHORD_MAX) {

        switch(chordTypeUp[pairdev]) {
            case 0:
                note = buildPowerChord(index, note);
                break;
            case 1:
                note = buildPowerChordExt(index, note);
                break;
            case 2:
                note = buildCMajor(index, note);
                break;
            case 3:
                note = buildCMajorProg(index, note);
                break;
        }

        if(note < 0) note = 0;
        if(note > 127) note = 127;

        return note;
    } else { // velocity
        switch(index) {
            case 1:
                vel = vel * chordScaleVelocity3Up[pairdev] / 20;
                break;
            case 2:
                vel = vel * chordScaleVelocity5Up[pairdev] / 20;
                break;
            case 3:
                vel = vel * chordScaleVelocity7Up[pairdev] / 20;
                break;
        }
        return vel;
    }
    return 0;
}

int MidiInControl::autoChordfunDown(int pairdev, int index, int note, int vel) {

    if(!_autoChordDown[pairdev]) { // no chord

        if(index == AUTOCHORD_MAX) return 1;
        if(note >= 0) return note;
        return vel;
    }

    if(note >= 0 || index == AUTOCHORD_MAX) {

        switch(chordTypeDown[pairdev]) {
            case 0:
                note = buildPowerChord(index, note);
                break;
            case 1:
                note = buildPowerChordExt(index, note);
                break;
            case 2:
                note = buildCMajor(index, note);
                break;
            case 3:
                note = buildCMajorProg(index, note);
                break;
        }

        if(note < 0) note = 0;
        if(note > 127) note = 127;

        return note;
    } else { // velocity
        switch(index) {
            case 1:
                vel = vel * chordScaleVelocity3Down[pairdev] / 20;
                break;
            case 2:
                vel = vel * chordScaleVelocity5Down[pairdev] / 20;
                break;
            case 3:
                vel = vel * chordScaleVelocity7Down[pairdev] / 20;
                break;
        }
        return vel;
    }
        return 0;
}

void MidiInControl::setChordDialogUp() {

    MidiInControl_chord* d = new MidiInControl_chord(this);
    d->chordBox->setCurrentIndex(chordTypeUp[cur_pairdev]);
    d->Slider3->setValue(chordScaleVelocity3Up[cur_pairdev]);
    d->Slider5->setValue(chordScaleVelocity5Up[cur_pairdev]);
    d->Slider7->setValue(chordScaleVelocity7Up[cur_pairdev]);

    d->extChord = &chordTypeUp[cur_pairdev];
    d->extSlider3 = &chordScaleVelocity3Up[cur_pairdev];
    d->extSlider5 = &chordScaleVelocity5Up[cur_pairdev];
    d->extSlider7 = &chordScaleVelocity7Up[cur_pairdev];

    d->exec();

    _settings->setValue("MIDIin/chordTypeUp" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), chordTypeUp[cur_pairdev]);
    _settings->setValue("MIDIin/chordScaleVelocity3Up" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), chordScaleVelocity3Up[cur_pairdev]);
    _settings->setValue("MIDIin/chordScaleVelocity5Up" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), chordScaleVelocity5Up[cur_pairdev]);
    _settings->setValue("MIDIin/chordScaleVelocity7Up" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), chordScaleVelocity7Up[cur_pairdev]);

    delete d;
}

void MidiInControl::setChordDialogDown() {

    MidiInControl_chord* d = new MidiInControl_chord(this);
    d->chordBox->setCurrentIndex(chordTypeDown[cur_pairdev]);
    d->Slider3->setValue(chordScaleVelocity3Down[cur_pairdev]);
    d->Slider5->setValue(chordScaleVelocity5Down[cur_pairdev]);
    d->Slider7->setValue(chordScaleVelocity7Down[cur_pairdev]);

    d->extChord = &chordTypeDown[cur_pairdev];
    d->extSlider3 = &chordScaleVelocity3Down[cur_pairdev];
    d->extSlider5 = &chordScaleVelocity5Down[cur_pairdev];
    d->extSlider7 = &chordScaleVelocity7Down[cur_pairdev];

    d->exec();

    _settings->setValue("MIDIin/chordTypeDown" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), chordTypeDown[cur_pairdev]);
    _settings->setValue("MIDIin/chordScaleVelocity3Down" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), chordScaleVelocity3Down[cur_pairdev]);
    _settings->setValue("MIDIin/chordScaleVelocity5Down" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), chordScaleVelocity5Down[cur_pairdev]);
    _settings->setValue("MIDIin/chordScaleVelocity7Down" + (cur_pairdev ? QString::number(cur_pairdev) : QString()), chordScaleVelocity7Down[cur_pairdev]);

    delete d;
}

MidiInControl_chord::MidiInControl_chord(QWidget* parent): QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint) {

    QDialog *chord = this;

    extChord = NULL;
    extSlider3 = NULL;
    extSlider5 = NULL;
    extSlider7 = NULL;

    if (chord->objectName().isEmpty())
        chord->setObjectName(QString::fromUtf8("chord"));
    chord->resize(373, 275);
    chord->setFixedSize(373, 275);
    chord->setWindowTitle("AutoChord & Velocity (Live)");

    buttonBox = new QDialogButtonBox(chord);
    buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
    buttonBox->setGeometry(QRect(160, 220, 191, 31));
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    label3 = new QLabel(chord);
    label3->setObjectName(QString::fromUtf8("label3"));
    label3->setGeometry(QRect(30, 80, 21, 21));
    label3->setAlignment(Qt::AlignCenter);
    label3->setText("3:");

    Slider3 = new QSlider(chord);
    Slider3->setObjectName(QString::fromUtf8("Slider3"));
    Slider3->setGeometry(QRect(60, 80, 241, 22));
    Slider3->setMaximum(20);
    Slider3->setValue(14);
    Slider3->setOrientation(Qt::Horizontal);
    Slider3->setTickPosition(QSlider::TicksBelow);

    label5 = new QLabel(chord);
    label5->setObjectName(QString::fromUtf8("label5"));
    label5->setGeometry(QRect(30, 120, 21, 21));
    label5->setAlignment(Qt::AlignCenter);
    label5->setText("5:");

    Slider5 = new QSlider(chord);
    Slider5->setObjectName(QString::fromUtf8("Slider5"));
    Slider5->setGeometry(QRect(60, 120, 241, 22));
    Slider5->setMaximum(20);
    Slider5->setValue(15);
    Slider5->setOrientation(Qt::Horizontal);
    Slider5->setTickPosition(QSlider::TicksBelow);
    label7 = new QLabel(chord);
    label7->setObjectName(QString::fromUtf8("label7"));
    label7->setGeometry(QRect(30, 160, 21, 21));
    label7->setAlignment(Qt::AlignCenter);
    label7->setText("7:");

    Slider7 = new QSlider(chord);
    Slider7->setObjectName(QString::fromUtf8("Slider7"));
    Slider7->setGeometry(QRect(60, 160, 241, 22));
    Slider7->setMaximum(20);
    Slider7->setValue(16);
    Slider7->setOrientation(Qt::Horizontal);
    Slider7->setTickPosition(QSlider::TicksBelow);

    rstButton = new QPushButton(chord);
    rstButton->setObjectName(QString::fromUtf8("rstButton"));
    rstButton->setGeometry(QRect(30, 226, 75, 23));
    rstButton->setText("Reset");

    chordBox = new QComboBox(chord);
    chordBox->setObjectName(QString::fromUtf8("chordBox"));
    chordBox->setGeometry(QRect(30, 21, 311, 22));
    chordBox->addItem("Power Chord");
    chordBox->addItem("Power Chord Extended");
    chordBox->addItem("C Major Chord (CM)");
    chordBox->addItem("C Major Chord  Progression (CM)");
    chordBox->setCurrentIndex(0);

    label3_2 = new QLabel(chord);
    label3_2->setObjectName(QString::fromUtf8("label3_2"));
    label3_2->setGeometry(QRect(320, 81, 21, 16));
    label3_2->setFrameShape(QFrame::StyledPanel);
    label3_2->setText("0");
    label5_2 = new QLabel(chord);
    label5_2->setObjectName(QString::fromUtf8("label5_2"));
    label5_2->setGeometry(QRect(320, 123, 21, 16));
    label5_2->setFrameShape(QFrame::StyledPanel);
    label5_2->setText("0");
    label7_2 = new QLabel(chord);
    label7_2->setObjectName(QString::fromUtf8("label7_2"));
    label7_2->setGeometry(QRect(320, 161, 21, 16));
    label7_2->setFrameShape(QFrame::StyledPanel);
    label7_2->setText("0");
    labelvelocity = new QLabel(chord);
    labelvelocity->setObjectName(QString::fromUtf8("labelvelocity"));
    labelvelocity->setGeometry(QRect(110, 52, 151, 20));
    labelvelocity->setAlignment(Qt::AlignCenter);
    labelvelocity->setText("Velocity scale x/20");

    connect(rstButton, QOverload<bool>::of(&QPushButton::clicked), [=](bool)
    {
        Slider3->setValue(14);
        Slider5->setValue(15);
        Slider7->setValue(16);
    });

    Slider3->setValue(-1);
    Slider5->setValue(-1);
    Slider7->setValue(-1);

    connect(buttonBox, SIGNAL(accepted()), chord, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), chord, SLOT(reject()));
    connect(Slider3, SIGNAL(valueChanged(int)), label3_2, SLOT(setNum(int)));
    connect(Slider5, SIGNAL(valueChanged(int)), label5_2, SLOT(setNum(int)));
    connect(Slider7, SIGNAL(valueChanged(int)), label7_2, SLOT(setNum(int)));

    connect(chordBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int v)
    {
        if(extChord) *extChord = v;
    });

    connect(Slider3, QOverload<int>::of(&QSlider::valueChanged), [=](int v)
    {
        if(extSlider3) *extSlider3 = v;
    });

    connect(Slider5, QOverload<int>::of(&QSlider::valueChanged), [=](int v)
    {
        if(extSlider5) *extSlider5 = v;
    });

    connect(Slider7, QOverload<int>::of(&QSlider::valueChanged), [=](int v)
    {
        if(extSlider7) *extSlider7 = v;
    });

    Slider3->setValue(0);
    Slider5->setValue(0);
    Slider7->setValue(0);

    QMetaObject::connectSlotsByName(chord);
}

// finger

int MidiInControl::finger_func(int pairdev, std::vector<unsigned char>* message, bool is_keyboard2, bool only_enable) {

    return FingerPatternDialog::Finger_note(pairdev, message, is_keyboard2, only_enable);

}

void MidiInControl::seqOn(int seq, int index, bool on) {

    // stop sequencer
    if(MidiPlayer::fileSequencer[seq])
        MidiPlayer::fileSequencer[seq]->setMode(-1, 0);

    MidiOutput::sequencer_enabled[seq] = -1;
    if(!on) {
        MidiInput::note_roll[seq].clear();
    }

    if(on && index >= 0) {

        unsigned buttons = 0;

        if(MidiPlayer::fileSequencer[seq])
            buttons = MidiPlayer::fileSequencer[seq]->getButtons(index & 3);


        // disable other autorhythm sequencer(s)
        if(buttons & SEQ_FLAG_AUTORHYTHM) {
            for(int seq1 = 0; seq1 < 16; seq1++) {
                if(seq1 == seq)
                    continue;
                if(MidiPlayer::fileSequencer[seq1] && MidiOutput::sequencer_enabled[seq1] >= 0) {

                    if(MidiPlayer::fileSequencer[seq1]->autorhythm) {
                        MidiPlayer::fileSequencer[seq1]->setMode(-1, 0);

                    }


                }
            }

        }

        // prepare sequencer
        if(MidiPlayer::fileSequencer[seq])
            MidiPlayer::fileSequencer[seq]->setMode(index & 3, buttons | SEQ_FLAG_SWITCH_ON);

        //MidiOutput::sequencer_enabled[seq] = (index & 3);
    }

}

int MidiInControl::new_effects(std::vector<unsigned char>* message, int id) {

    int dev = 0;
    bool is_kb2 = false;
    bool delayed_return1 = false;
    bool skip_sustain = false;
    bool skip_expression = false;
    bool skip_aftertouch = false;
    bool is_pitch_bend = false;

    if(id < DEVICE_ID || id >= (DEVICE_ID + MAX_INPUT_DEVICES))
        return RET_NEWEFFECTS_NONE;

    int idev = (id - DEVICE_ID);
    dev = idev / 2;

    // skip DOWN when keyboard is connected for this reason
    if(MidiInput::keyboard2_connected[dev * 2 + 1] &&
        !(idev & 1)) {
        is_kb2 = true;
    }

    int evt = message->at(0);

    int ch = evt & 0xF;

    evt&= 0xF0;

    if(evt == 0xD0) {// Aftertouch Channel Pressure

        if(idev & 1) {

           int ch_in = MidiInControl::inchannelDown(dev);

           if(ch != ch_in)
               skip_aftertouch = true;
           if(ch < 0)
               ch = 0;
        } else {
            int ch_in = MidiInControl::inchannelUp(dev);
            if(ch != ch_in && ch_in >= 0)
                skip_aftertouch = true;

            if(ch < 0)
                ch = 0;
        }

        // hack to Aftertouch Key Pressure

        message->at(0) = 0xA0 | ch;
        evt = 0xA0;

        if(message->size() == 2) {

            message->emplace_back(message->at(1));
        }

        delayed_return1 = true; // skip without action (very important)

    } else if(evt == 0xA0) {// Aftertouch Key Pressure

        delayed_return1 = true; // skip without action
    }

    // Pitch bend
    if(evt == 0xE0 && message->size() == 3) {

        if(idev & 1) {

            if(ch == MidiInControl::inchannelDown(dev)) {

                is_pitch_bend = true;
            }

        } else {

            if(ch == MidiInControl::inchannelUp(dev)) {

                is_pitch_bend = true;
            }
        }

    }


    // sustain pedal
    if(evt == 0xB0 && message->at(1) == 64 && message->size() == 3) {

        if(idev & 1) {

            if(ch == MidiInControl::inchannelDown(dev)) {

                int val = message->at(2);

                if(MidiInControl::invSustainDOWN[dev])
                    val = 127 - val;
                message->at(2) = val;

                MidiInControl::sustainDOWNval = val;
            }

        } else {

            if(ch == MidiInControl::inchannelUp(dev)) {

                int val = message->at(2);

                if(MidiInControl::invSustainUP[dev])
                    val = 127 - val;
                message->at(2) = val;

                MidiInControl::sustainUPval = val;
            }
        }

    }

    // expression pedal
    if(evt == 0xB0 && message->at(1) == 11 && message->size() == 3) {

        if(idev & 1) {

            if(ch == MidiInControl::inchannelDown(dev)) {

                int val = message->at(2);

                if(MidiInControl::scalex2ExpressionDOWN[dev]) {
                  val*=2;
                  if(val > 127)
                      val = 127;
                }

                if(MidiInControl::invExpressionDOWN[dev])
                    val = 127 - val;
                message->at(2) = val;

                MidiInControl::expressionDOWNval = val;
            }

        } else {

            if(ch == MidiInControl::inchannelUp(dev)) {

                int val = message->at(2);

                if(MidiInControl::scalex2ExpressionUP[dev]) {
                    val*=2;
                    if(val > 127)
                        val = 127;
                }

                if(MidiInControl::invExpressionUP[dev])
                    val = 127 - val;
                message->at(2) = val;

                MidiInControl::expressionUPval = val;
            }
        }
    }


    ////

    if(show_effects) {

        if(evt == 0xb0) {
            qWarning("cc %i %i - %x %i", message->at(1), message->at(2),
                     id, idev);
        }

        if(evt == 0xe0) {
            qWarning("e0 %i %i - %x %i", message->at(1), message->at(2),
                     id, idev);
        }
    }

    int ch_up = ((MidiInControl::channelUp(dev) < 0)
                     ? MidiOutput::standardChannel()
                     : (MidiInControl::channelUp(dev) & 15));

    int ch_down = ((MidiInControl::channelDown(dev) < 0)
                       ? ((MidiInControl::channelUp(dev) < 0)
                              ? MidiOutput::standardChannel()
                              : MidiInControl::channelUp(dev))
                       : MidiInControl::channelDown(dev)) & 0xF;

    bool update_event = false;
    bool expression_switch = false;
    bool aftertouch_switch = false;

    for(int l = 0; l < action_effects[dev].count(); l++) {

        bool is_aftertouch = false;
        bool skip_note = false;

        if(update_event) {

            update_event = false;
            evt = message->at(0);

            ch = evt & 0xF;

            evt&= 0xF0;
        }

        InputActionData action = action_effects[dev].at(l);

        if(action.device < 0)
            continue;

        if((DEVICE_ID + action.device) != id)
            continue;

        if(action.channel == 17) {

            if(!(action.device & 1) && (ch != MidiInControl::inchannelUp(dev) && MidiInControl::inchannelUp(dev) != -1))
                continue;

            if((action.device & 1) && (ch != MidiInControl::inchannelDown(dev) && MidiInControl::inchannelDown(dev) != -1))
                continue;

        } else if(action.channel != 16 && action.channel != ch)
            continue;


        if(action.event == EVENT_SUSTAIN || action.event == EVENT_SUSTAIN_INV) {

            skip_sustain = true;

            if(action.control_note >= 0) {
                if(!(MidiInput::keys_switch[idev] & (1 << (action.control_note & 31))))
                    continue;
            }

            if(evt == 0xB0 && message->at(1) == 64 && message->size() == 3) {
                update_event = true;
                int vel  = message->at(2);

                if(action.event == EVENT_SUSTAIN_INV)
                    vel = 127 - vel;

                if(vel >= 64) {
                    evt = 0x90;
                } else {
                    evt = 0x80;
                }
            } else
                continue;
        }

        if(action.event == EVENT_EXPRESSION || action.event == EVENT_EXPRESSION_INV) {

            skip_expression = true;

            if(action.control_note >= 0) {
                if(!(MidiInput::keys_switch[idev] & (1 << (action.control_note & 31))))
                    continue;
            }

            if(evt == 0xB0 && message->at(1) == 11  && message->size() == 3) {
               update_event = true;
               int vel  = message->at(2);

               if(action.event == EVENT_EXPRESSION_INV)
                   vel = 127 - vel;

               if(vel >= 64) {
                    if(!(MidiInput::keys_dev[idev][ch] & KEY_EXPRESSION_PEDAL))
                        expression_switch = true;

                    MidiInput::keys_dev[idev][ch]|= KEY_EXPRESSION_PEDAL;

               } else {
                   if(MidiInput::keys_dev[idev][ch] & KEY_EXPRESSION_PEDAL)
                       expression_switch = true;

                    MidiInput::keys_dev[idev][ch]&= ~KEY_EXPRESSION_PEDAL;
               }
            } else
                continue;
        }

        if(action.event == EVENT_AFTERTOUCH) {

            skip_aftertouch = true;

            action.bypass = -1; // don?t use bypass!

            if(action.control_note >= 0) {
                if(!(MidiInput::keys_switch[idev] & (1 << (action.control_note & 31))))
                    continue;
            }

            if((evt == 0xA0) && message->size() == 3) {
               update_event = true;
               is_aftertouch = true;
               int vel  = message->at(2);

               if(vel >= 64) {
                    if(!(MidiInput::keys_dev[idev][ch] & KEY_AFTERTOUCH))
                        aftertouch_switch = true;

                    MidiInput::keys_dev[idev][ch]|= KEY_AFTERTOUCH;

               }  else if(vel <= 8){
                   if(MidiInput::keys_dev[idev][ch] & KEY_AFTERTOUCH)
                       aftertouch_switch = true;

                    MidiInput::keys_dev[idev][ch]&= ~KEY_AFTERTOUCH;
               }
            } else
                continue;
        }

        if(action.event == EVENT_PITCH_BEND && is_pitch_bend  && message->size() == 3) {
            delayed_return1 = true;

            if(action.control_note >= 0) {
                if(!(MidiInput::keys_switch[idev] & (1 << (action.control_note & 31))))
                    continue;
            }

            action.event = EVENT_CONTROL;
            evt = 0xB0;
            skip_note = true;
        }

        if(action.event == EVENT_MODULATION_WHEEL &&
                (evt == 0xb0) && message->at(1) == 1 && message->size() == 3) {
            delayed_return1 = true;

            if(action.control_note >= 0) {
                if(!(MidiInput::keys_switch[idev] & (1 << (action.control_note & 31))))
                    continue;
            }

            action.event = EVENT_CONTROL;
            skip_note = true;
        }

        if(update_event || action.event == EVENT_NOTE || action.event == EVENT_CONTROL) { // note event & control
            if((((action.event == EVENT_NOTE || action.event == EVENT_SUSTAIN || action.event == EVENT_SUSTAIN_INV)
                 && (evt == 0x80 || evt == 0x90)) ||
                ((action.event == EVENT_CONTROL ||
                  action.event == EVENT_EXPRESSION || action.event == EVENT_EXPRESSION_INV) && (evt == 0xB0)) ||
                  (action.event == EVENT_AFTERTOUCH && (evt == 0xA0))

                ) && message->size() == 3) {

                int note = message->at(1);
                int vel  = message->at(2);

                if(skip_note) {
                    action.control_note = note;

                    if(is_pitch_bend) {

                        if(vel < 64)
                            vel = (63 - vel) * 2 + 1;
                        else
                            vel = (vel - 64)  * 2 + 1;

                        if(vel == 1)
                            vel = 0;

                    } else {
                        if(vel < 80)
                            vel = 64 * vel/ 80;
                        else
                            vel = 64 + 63 * (vel - 80) / 47;
                    }

                }

                if(update_event || (!update_event && action.control_note == note)) {

                    if(update_event && (action.event == EVENT_SUSTAIN || action.event == EVENT_SUSTAIN_INV)) {
                        action.event = EVENT_NOTE;
                        if(evt == 0x80)
                            vel = 0;
                    }

                    if(update_event && (action.event == EVENT_EXPRESSION || action.event == EVENT_EXPRESSION_INV)) {
                        if(action.function == FUNCTION_VAL_BUTTON) {
                            action.event = EVENT_NOTE;

                            if(MidiInput::keys_dev[idev][ch] & KEY_EXPRESSION_PEDAL) {
                                if(!expression_switch)
                                    return RET_NEWEFFECTS_SKIP; // ignore all
                                vel = 127;
                                evt = 0x90;
                            } else {

                                evt = 0x80;
                                vel = 0;
                            }

                        } else if(action.function == FUNCTION_VAL_SWITCH) {

                            if(!expression_switch)
                                return RET_NEWEFFECTS_SKIP; // ignore all

                            action.event = EVENT_NOTE;

                            if(MidiInput::keys_dev[idev][ch] & KEY_EXPRESSION_PEDAL) {

                                vel = 127;
                                evt = 0x90;
                            } else {

                                evt = 0x80;
                                vel = 0;
                            }


                        } else if(action.function == FUNCTION_VAL || action.function == FUNCTION_VAL_CLIP) {
                            action.event = EVENT_CONTROL;
                            evt = 0xb0;
                        }
                    }

                    if(update_event && action.event == EVENT_AFTERTOUCH) {
                        if(action.function == FUNCTION_VAL_BUTTON) {
                            action.event = EVENT_NOTE;

                            if(MidiInput::keys_dev[idev][ch] & KEY_AFTERTOUCH) {
                                if(!aftertouch_switch)
                                    return RET_NEWEFFECTS_SKIP; // ignore all
                                vel = 127;
                                evt = 0x90;
                            } else {

                                evt = 0x80;
                                vel = 0;
                            }

                        } else if(action.function == FUNCTION_VAL_SWITCH) {

                            if(!aftertouch_switch)
                                return RET_NEWEFFECTS_SKIP; // ignore all

                            action.event = EVENT_NOTE;

                            if(MidiInput::keys_dev[idev][ch] & KEY_AFTERTOUCH) {

                                vel = 127;
                                evt = 0x90;
                            } else {

                                evt = 0x80;
                                vel = 0;
                            }


                        } else if(action.function == FUNCTION_VAL || action.function == FUNCTION_VAL_CLIP) {
                            action.event = EVENT_CONTROL;
                            evt = 0xb0;
                        }
                    }

                    if(action.category == CATEGORY_MIDI_EVENTS) { // Midi Events

                        int dev1 = dev;
                        int adev1 = idev;
                        int ch_up1 = ch_up;
                        int ch_down1 = ch_down;

                        if(action.bypass != -1 && action.bypass != idev) {
                            adev1 = action.bypass;
                            dev1 = action.bypass/2;

                            ch_up1 = ((MidiInControl::channelUp(dev1) < 0)
                                             ? MidiOutput::standardChannel()
                                             : (MidiInControl::channelUp(dev1) & 15));

                            ch_down1 = ((MidiInControl::channelDown(dev1) < 0)
                                               ? ((MidiInControl::channelUp(dev1) < 0)
                                                      ? MidiOutput::standardChannel()
                                                      : MidiInControl::channelUp(dev1))
                                               : MidiInControl::channelDown(dev1)) & 0xF;

                        }

                        QString text = "Dev: " + QString::number(adev1) + " -";
                        QString text2;

                        if(action.function == FUNCTION_VAL_CLIP) {
                            if(vel < action.min)
                                vel = action.min;
                            if(vel > action.max)
                                vel = action.max;

                        } else if(action.function == FUNCTION_VAL_BUTTON) {
                            if(evt == 0x90)
                                vel = action.max;
                            if(evt == 0x80)
                                vel = action.min;

                        } else if(action.function == FUNCTION_VAL_SWITCH) {

                            int sw = -1;

                            if(evt == 0x90 || (evt == 0xB0  && (vel >= 64))) {

                                if(action.action == ACTION_PITCHBEND_UP) {

                                    MidiInput::keys_dev[adev1][ch_up1]^= KEY_PITCHBEND_UP;
                                    if(MidiInput::keys_dev[adev1][ch_up1] & KEY_PITCHBEND_UP)
                                        sw = 1;
                                    else
                                        sw = 0;

                                } else if(action.action == ACTION_MODULATION_WHEEL_UP) {

                                    MidiInput::keys_dev[adev1][ch_up1]^= KEY_MODULATION_WHEEL_UP;
                                    if(MidiInput::keys_dev[adev1][ch_up1] & KEY_MODULATION_WHEEL_UP)
                                        sw = 1;
                                    else
                                        sw = 0;

                                } else if(action.action == ACTION_SUSTAIN_UP) {

                                    MidiInput::keys_dev[adev1][ch_up1]^= KEY_SUSTAIN_UP;
                                    if(MidiInput::keys_dev[adev1][ch_up1] & KEY_SUSTAIN_UP)
                                        sw = 1;
                                    else
                                        sw = 0;

                                } else if(action.action == ACTION_SOSTENUTO_UP) {

                                    MidiInput::keys_dev[adev1][ch_up1]^= KEY_SOSTENUTO_UP;
                                    if(MidiInput::keys_dev[adev1][ch_up1] & KEY_SOSTENUTO_UP)
                                        sw = 1;
                                    else
                                        sw = 0;

                                } else if(action.action == ACTION_REVERB_UP) {

                                    MidiInput::keys_dev[adev1][ch_up1]^= KEY_REVERB_UP;
                                    if(MidiInput::keys_dev[adev1][ch_up1] & KEY_REVERB_UP)
                                        sw = 1;
                                    else
                                        sw = 0;

                                } else if(action.action == ACTION_CHORUS_UP) {

                                    MidiInput::keys_dev[adev1][ch_up1]^= KEY_CHORUS_UP;
                                    if(MidiInput::keys_dev[adev1][ch_up1] & KEY_CHORUS_UP)
                                        sw = 1;
                                    else
                                        sw = 0;

                                } else if(action.action == ACTION_PROGRAM_CHANGE_UP) {

                                    MidiInput::keys_dev[adev1][ch_up1]^= KEY_PROGRAM_CHANGE_UP;
                                    if(MidiInput::keys_dev[adev1][ch_up1] & KEY_PROGRAM_CHANGE_UP)
                                        sw = 1;
                                    else
                                        sw = 0;

                                } else if(action.action == ACTION_CHAN_VOLUME_UP) {

                                    MidiInput::keys_dev[adev1][ch_up1]^= KEY_CHAN_VOLUME_UP;
                                    if(MidiInput::keys_dev[adev1][ch_up1] & KEY_CHAN_VOLUME_UP)
                                        sw = 1;
                                    else
                                        sw = 0;

                                } else if(action.action == ACTION_PAN_UP) {

                                    MidiInput::keys_dev[adev1][ch_up1]^= KEY_PAN_UP;
                                    if(MidiInput::keys_dev[adev1][ch_up1] & KEY_PAN_UP)
                                        sw = 1;
                                    else
                                        sw = 0;

                                } else if(action.action == ACTION_ATTACK_UP) {

                                    MidiInput::keys_dev[adev1][ch_up1]^= KEY_ATTACK_UP;
                                    if(MidiInput::keys_dev[adev1][ch_up1] & KEY_ATTACK_UP)
                                        sw = 1;
                                    else
                                        sw = 0;

                                } else if(action.action == ACTION_RELEASE_UP) {

                                    MidiInput::keys_dev[adev1][ch_up1]^= KEY_RELEASE_UP;
                                    if(MidiInput::keys_dev[adev1][ch_up1] & KEY_RELEASE_UP)
                                        sw = 1;
                                    else
                                        sw = 0;

                                } else if(action.action == ACTION_DECAY_UP) {

                                    MidiInput::keys_dev[adev1][ch_up1]^= KEY_DECAY_UP;
                                    if(MidiInput::keys_dev[adev1][ch_up1] & KEY_DECAY_UP)
                                        sw = 1;
                                    else
                                        sw = 0;

                                } else if(action.action == ACTION_PITCHBEND_DOWN) {

                                    MidiInput::keys_dev[adev1][ch_down1]^= KEY_PITCHBEND_DOWN;
                                    if(MidiInput::keys_dev[adev1][ch_down1] & KEY_PITCHBEND_DOWN)
                                        sw = 1;
                                    else
                                        sw = 0;

                                } else if(action.action == ACTION_MODULATION_WHEEL_DOWN) {

                                    MidiInput::keys_dev[adev1][ch_down1]^= KEY_MODULATION_WHEEL_DOWN;
                                    if(MidiInput::keys_dev[adev1][ch_down1] & KEY_MODULATION_WHEEL_DOWN)
                                        sw = 1;
                                    else
                                        sw = 0;

                                } else if(action.action == ACTION_SUSTAIN_DOWN) {

                                    MidiInput::keys_dev[adev1][ch_down1]^= KEY_SUSTAIN_DOWN;
                                    if(MidiInput::keys_dev[adev1][ch_down1] & KEY_SUSTAIN_DOWN)
                                        sw = 1;
                                    else
                                        sw = 0;

                                } else if(action.action == ACTION_SOSTENUTO_DOWN) {

                                    MidiInput::keys_dev[adev1][ch_down1]^= KEY_SOSTENUTO_DOWN;
                                    if(MidiInput::keys_dev[adev1][ch_down1] & KEY_SOSTENUTO_DOWN)
                                        sw = 1;
                                    else
                                        sw = 0;

                                } else if(action.action == ACTION_REVERB_DOWN) {

                                    MidiInput::keys_dev[adev1][ch_down1]^= KEY_REVERB_DOWN;
                                    if(MidiInput::keys_dev[adev1][ch_down1] & KEY_REVERB_DOWN)
                                        sw = 1;
                                    else
                                        sw = 0;

                                } else if(action.action == ACTION_CHORUS_DOWN) {

                                    MidiInput::keys_dev[adev1][ch_down1]^= KEY_CHORUS_DOWN;
                                    if(MidiInput::keys_dev[adev1][ch_down1] & KEY_CHORUS_DOWN)
                                        sw = 1;
                                    else
                                        sw = 0;

                                } else if(action.action == ACTION_PROGRAM_CHANGE_DOWN) {

                                    MidiInput::keys_dev[adev1][ch_down1]^= KEY_PROGRAM_CHANGE_DOWN;
                                    if(MidiInput::keys_dev[adev1][ch_down1] & KEY_PROGRAM_CHANGE_DOWN)
                                        sw = 1;
                                    else
                                        sw = 0;

                                } else if(action.action == ACTION_CHAN_VOLUME_DOWN) {

                                    MidiInput::keys_dev[adev1][ch_down1]^= KEY_CHAN_VOLUME_DOWN;
                                    if(MidiInput::keys_dev[adev1][ch_down1] & KEY_CHAN_VOLUME_DOWN)
                                        sw = 1;
                                    else
                                        sw = 0;

                                } else if(action.action == ACTION_PAN_DOWN) {

                                    MidiInput::keys_dev[adev1][ch_down1]^= KEY_PAN_DOWN;
                                    if(MidiInput::keys_dev[adev1][ch_down1] & KEY_PAN_DOWN)
                                        sw = 1;
                                    else
                                        sw = 0;

                                } else if(action.action == ACTION_ATTACK_DOWN) {

                                    MidiInput::keys_dev[adev1][ch_down1]^= KEY_ATTACK_DOWN;
                                    if(MidiInput::keys_dev[adev1][ch_down1] & KEY_ATTACK_DOWN)
                                        sw = 1;
                                    else
                                        sw = 0;

                                } else if(action.action == ACTION_RELEASE_DOWN) {

                                    MidiInput::keys_dev[adev1][ch_down1]^= KEY_RELEASE_DOWN;
                                    if(MidiInput::keys_dev[adev1][ch_down1] & KEY_RELEASE_DOWN)
                                        sw = 1;
                                    else
                                        sw = 0;

                                } else if(action.action == ACTION_DECAY_DOWN) {

                                    MidiInput::keys_dev[adev1][ch_down1]^= KEY_DECAY_DOWN;
                                    if(MidiInput::keys_dev[adev1][ch_down1] & KEY_DECAY_DOWN)
                                        sw = 1;
                                    else
                                        sw = 0;

                                }


                                if(action.action == ACTION_SUSTAIN_DOWN ||
                                    action.action == ACTION_SOSTENUTO_DOWN) {

                                    if(sw == 1) {
                                        vel = 127;
                                        text2 = " ON ";
                                    } else if(sw == 0) {
                                        vel = 0;
                                        text2 = " OFF ";
                                    }
                                } else {
                                    if(sw == 1) {
                                        vel = action.max;
                                        text2 = " ON ";
                                    } else if(sw == 0) {
                                        vel = action.min;
                                        text2 = " OFF ";
                                    }
                                }
                            }

                            if(sw == -1)
                                return RET_NEWEFFECTS_SKIP; // ignore

                        }

                        if(action.action == ACTION_PITCHBEND_UP) {

                            text+= " PITCHBEND UP - " + text2 + "v: " + QString::number(vel);
                            OSD = text;

                            message->at(0) = 0xE0 | ch_up1;
                            if(!is_pitch_bend) {
                                message->at(1) = (vel << 6) & 192;
                                message->at(2) = (vel >> 2);
                            }

                            if(action.bypass != -1 && action.bypass != idev) {
                                return RET_NEWEFFECTS_BYPASS + action.bypass;
                            }

                            return RET_NEWEFFECTS_SET;

                        } else if(action.action == ACTION_MODULATION_WHEEL_UP) {

                            text+= " MODULATION WHEEL UP - " + text2 + "v: " + QString::number(vel);
                            OSD = text;

                            message->at(0) = 0xB0 | ch_up1;
                            message->at(1) = 1;
                            message->at(2) = (vel >> 2);

                            if(action.bypass != -1 && action.bypass != idev) {
                                return RET_NEWEFFECTS_BYPASS + action.bypass;
                            }

                            return RET_NEWEFFECTS_SET;

                        } else if(action.action == ACTION_SUSTAIN_UP) {

                            text+= " SUSTAIN UP - " + text2;
                            OSD = text;

                            message->at(0) = 0xB0 | ch_up1;
                            message->at(1) = 64;
                            message->at(2) = (vel);

                            if(action.bypass != -1 && action.bypass != idev) {
                                return RET_NEWEFFECTS_BYPASS + action.bypass;
                            }

                            return RET_NEWEFFECTS_SET;

                        } else if(action.action == ACTION_SOSTENUTO_UP) {

                            text+= " SOSTENUTO UP - " + text2;
                            OSD = text;

                            message->at(0) = 0xB0 | ch_up1;
                            message->at(1) = 66;
                            message->at(2) = (vel);

                            if(action.bypass != -1 && action.bypass != idev) {
                                return RET_NEWEFFECTS_BYPASS + action.bypass;
                            }

                            return RET_NEWEFFECTS_SET;

                        } else if(action.action == ACTION_REVERB_UP) {

                            text+= " REVERB UP - " + text2 + "v: " + QString::number(vel);
                            OSD = text;

                            message->at(0) = 0xB0 | ch_up1;
                            message->at(1) = 91;
                            message->at(2) = (vel);

                            if(action.bypass != -1 && action.bypass != idev) {
                                return RET_NEWEFFECTS_BYPASS + action.bypass;
                            }

                            return RET_NEWEFFECTS_SET;

                        } else if(action.action == ACTION_CHORUS_UP) {

                            text+= " CHORUS UP - " + text2 + "v: " + QString::number(vel);
                            OSD = text;

                            message->at(0) = 0xB0 | ch_up1;
                            message->at(1) = 93;
                            message->at(2) = (vel);

                            if(action.bypass != -1 && action.bypass != idev) {
                                return RET_NEWEFFECTS_BYPASS + action.bypass;
                            }

                            return RET_NEWEFFECTS_SET;

                        } else if(action.action == ACTION_PROGRAM_CHANGE_UP) {

                            text+= " PROGRAM_CHANGE UP - " + text2 + "n: " + QString::number(vel);
                            OSD = text;

                            message->clear();
                            message->emplace_back(0xC0 | ch_up1);
                            message->emplace_back(vel);

                            if(action.bypass != -1 && action.bypass != idev) {
                                return RET_NEWEFFECTS_BYPASS + action.bypass;
                            }

                            return RET_NEWEFFECTS_SET;

                        } else if(action.action == ACTION_CHAN_VOLUME_UP) {

                            text+= " CHAN VOLUME UP - " + text2 + "v: " + QString::number(vel);
                            OSD = text;

                            message->at(0) = 0xB0 | ch_up1;
                            message->at(1) = 7;
                            message->at(2) = (vel);

                            if(action.bypass != -1 && action.bypass != idev) {
                                return RET_NEWEFFECTS_BYPASS + action.bypass;
                            }

                            return RET_NEWEFFECTS_SET;

                        } else if(action.action == ACTION_PAN_UP) {

                            text+= " PAN UP - " + text2 + "v: " + QString::number(vel);
                            OSD = text;

                            message->at(0) = 0xB0 | ch_up1;
                            message->at(1) = 10;
                            message->at(2) = (vel);

                            if(action.bypass != -1 && action.bypass != idev) {
                                return RET_NEWEFFECTS_BYPASS + action.bypass;
                            }

                            return RET_NEWEFFECTS_SET;

                        } else if(action.action == ACTION_ATTACK_UP) {

                            text+= " ATTACK UP - " + text2 + "v: " + QString::number(vel);
                            OSD = text;

                            message->at(0) = 0xB0 | ch_up1;
                            message->at(1) = 73;
                            message->at(2) = (vel);

                            if(action.bypass != -1 && action.bypass != idev) {
                                return RET_NEWEFFECTS_BYPASS + action.bypass;
                            }

                            return RET_NEWEFFECTS_SET;

                        } else if(action.action == ACTION_RELEASE_UP) {

                            text+= " RELEASE UP - " + text2 + "v: " + QString::number(vel);
                            OSD = text;

                            message->at(0) = 0xB0 | ch_up1;
                            message->at(1) = 72;
                            message->at(2) = (vel);

                            if(action.bypass != -1 && action.bypass != idev) {
                                return RET_NEWEFFECTS_BYPASS + action.bypass;
                            }

                            return RET_NEWEFFECTS_SET;

                        } else if(action.action == ACTION_DECAY_UP) {

                            text+= " DECAY UP - " + text2 + "v: " + QString::number(vel);
                            OSD = text;

                            message->at(0) = 0xB0 | ch_up1;
                            message->at(1) = 75;
                            message->at(2) = (vel);

                            if(action.bypass != -1 && action.bypass != idev) {
                                return RET_NEWEFFECTS_BYPASS + action.bypass;
                            }

                            return RET_NEWEFFECTS_SET;

                        } else if(action.action == ACTION_PITCHBEND_DOWN) {

                            if(is_kb2)
                                continue;

                            text+= " PITCHBEND DOWN - " + text2 + "v: " + QString::number(vel);
                            OSD = text;

                            message->at(0) = 0xE0 | ch_down1;
                            if(!is_pitch_bend) {
                                message->at(1) = (vel << 6) & 192;
                                message->at(2) = (vel >> 2);
                            }

                            if(action.bypass != -1 && action.bypass != idev) {
                                return RET_NEWEFFECTS_BYPASS + action.bypass;
                            }

                            return RET_NEWEFFECTS_SET;

                        } else if(action.action == ACTION_MODULATION_WHEEL_DOWN) {

                            if(is_kb2)
                                continue;

                            text+= " MODULATION WHEEL DOWN - " + text2 + "v: " + QString::number(vel);
                            OSD = text;

                            message->at(0) = 0xB0 | ch_down1;
                            message->at(1) = 1;
                            message->at(2) = (vel >> 2);

                            if(action.bypass != -1 && action.bypass != idev) {
                                return RET_NEWEFFECTS_BYPASS + action.bypass;
                            }

                            return RET_NEWEFFECTS_SET;

                        } else if(action.action == ACTION_SUSTAIN_DOWN) {

                            if(is_kb2)
                                continue;

                            text+= " SUSTAIN DOWN - " + text2;
                            OSD = text;

                            message->at(0) = 0xB0 | ch_down1;
                            message->at(1) = 64;
                            message->at(2) = (vel);

                            if(action.bypass != -1 && action.bypass != idev) {
                                return RET_NEWEFFECTS_BYPASS + action.bypass;
                            }

                            return RET_NEWEFFECTS_SET;

                        } else if(action.action == ACTION_SOSTENUTO_DOWN) {

                            if(is_kb2)
                                continue;

                            text+= " SOSTENUTO DOWN - " + text2;
                            OSD = text;

                            message->at(0) = 0xB0 | ch_down1;
                            message->at(1) = 66;
                            message->at(2) = (vel);

                            if(action.bypass != -1 && action.bypass != idev) {
                                return RET_NEWEFFECTS_BYPASS + action.bypass;
                            }

                            return RET_NEWEFFECTS_SET;

                        } else if(action.action == ACTION_REVERB_DOWN) {

                            if(is_kb2)
                                continue;

                            text+= " REVERB DOWN - " + text2 + "v: " + QString::number(vel);
                            OSD = text;

                            message->at(0) = 0xB0 | ch_down1;
                            message->at(1) = 91;
                            message->at(2) = (vel);

                            if(action.bypass != -1 && action.bypass != idev) {
                                return RET_NEWEFFECTS_BYPASS + action.bypass;
                            }

                            return RET_NEWEFFECTS_SET;

                        } else if(action.action == ACTION_CHORUS_DOWN) {

                            if(is_kb2)
                                continue;

                            text+= " CHORUS DOWN - " + text2 + "v: " + QString::number(vel);
                            OSD = text;

                            message->at(0) = 0xB0 | ch_down1;
                            message->at(1) = 93;
                            message->at(2) = (vel);

                            if(action.bypass != -1 && action.bypass != idev) {
                                return RET_NEWEFFECTS_BYPASS + action.bypass;
                            }

                            return RET_NEWEFFECTS_SET;

                        } else if(action.action == ACTION_PROGRAM_CHANGE_DOWN) {

                            if(is_kb2)
                                continue;

                            text+= " PROGRAM CHANGE DOWN - " + text2 + "n: " + QString::number(vel);
                            OSD = text;

                            message->clear();
                            message->emplace_back(0xC0 | ch_down1);
                            message->emplace_back(vel);

                            if(action.bypass != -1 && action.bypass != idev) {
                                return RET_NEWEFFECTS_BYPASS + action.bypass;
                            }

                            return RET_NEWEFFECTS_SET;

                        } else if(action.action == ACTION_CHAN_VOLUME_DOWN) {

                            if(is_kb2)
                                continue;

                            text+= " VOLUME DOWN - " + text2 + "v: " + QString::number(vel);
                            OSD = text;

                            message->at(0) = 0xB0 | ch_down1;
                            message->at(1) = 7;
                            message->at(2) = (vel);

                            if(action.bypass != -1 && action.bypass != idev) {
                                return RET_NEWEFFECTS_BYPASS + action.bypass;
                            }

                            return RET_NEWEFFECTS_SET;

                        } else if(action.action == ACTION_PAN_DOWN) {

                            if(is_kb2)
                                continue;

                            text+= " PAN DOWN - " + text2 + "v: " + QString::number(vel);
                            OSD = text;

                            message->at(0) = 0xB0 | ch_down1;
                            message->at(1) = 10;
                            message->at(2) = (vel);

                            if(action.bypass != -1 && action.bypass != idev) {
                                return RET_NEWEFFECTS_BYPASS + action.bypass;
                            }

                            return RET_NEWEFFECTS_SET;

                        } else if(action.action == ACTION_ATTACK_DOWN) {

                            if(is_kb2)
                                continue;

                            text+= " ATTACK DOWN - " + text2 + "v: " + QString::number(vel);
                            OSD = text;

                            message->at(0) = 0xB0 | ch_down1;
                            message->at(1) = 73;
                            message->at(2) = (vel);

                            if(action.bypass != -1 && action.bypass != idev) {
                                return RET_NEWEFFECTS_BYPASS + action.bypass;
                            }

                            return RET_NEWEFFECTS_SET;

                        } else if(action.action == ACTION_RELEASE_DOWN) {

                            if(is_kb2)
                                continue;

                            text+= " RELEASE DOWN - " + text2 + "v: " + QString::number(vel);
                            OSD = text;

                            message->at(0) = 0xB0 | ch_down1;
                            message->at(1) = 72;
                            message->at(2) = (vel);

                            if(action.bypass != -1 && action.bypass != idev) {
                                return RET_NEWEFFECTS_BYPASS + action.bypass;
                            }

                            return RET_NEWEFFECTS_SET;

                        } else if(action.action == ACTION_DECAY_DOWN) {

                            if(is_kb2)
                                continue;

                            text+= " DECAY DOWN - " + text2 + "v: " + QString::number(vel);
                            OSD = text;

                            message->at(0) = 0xB0 | ch_down1;
                            message->at(1) = 75;
                            message->at(2) = (vel);

                            if(action.bypass != -1 && action.bypass != idev) {
                                return RET_NEWEFFECTS_BYPASS + action.bypass;
                            }

                            return RET_NEWEFFECTS_SET;

                        }

                        return RET_NEWEFFECTS_NONE;

                    } else if(action.category == CATEGORY_AUTOCHORD) { // Autochord

                        int dev1 = dev;
                        int idev1 = idev;

                        if(action.bypass != -1 && action.bypass != idev) {

                            dev1 = action.bypass/2;
                            idev1 = action.bypass;

                        }

                        QString text = "Dev: " + QString::number(idev1) + " - AUTO CHORD -";

                        switch(action.action & 3) {
                            case 0:
                                text+= "Power Chord ";
                                break;
                            case 1:
                                text+= "Power Chord Extended ";
                                break;
                            case 2:
                                text+= "C Major Chord (CM) ";
                                break;
                            case 3:
                                text+= "C Major Chord  Progression (CM) ";
                                break;
                        }

                        if(action.action & 32) {
                            if(is_kb2)
                                continue;


                            chordTypeDown[dev1] = action.action & 3;
                            chordScaleVelocity3Down[dev1] = action.lev0;
                            chordScaleVelocity5Down[dev1] = action.lev1;
                            chordScaleVelocity7Down[dev1] = action.lev2;

                        } else {

                            chordTypeUp[dev1] = action.action & 3;
                            chordScaleVelocity3Up[dev1] = action.lev0;
                            chordScaleVelocity5Up[dev1] = action.lev1;
                            chordScaleVelocity7Up[dev1] = action.lev2;
                        }

                        if(action.function == FUNCTION_VAL_SWITCH || action.event == EVENT_CONTROL) {
                            if(evt == 0x90 || (action.event == EVENT_CONTROL && evt == 0xb0 && (vel >= 64))) {

                                if(action.action & 32)
                                    _autoChordDown[dev1] ^= true;
                                else
                                    _autoChordUp[dev1] ^= true;

                                if(action.action & 32) {
                                    if(_autoChordDown[dev1])
                                        text+= "Down ON";
                                    else
                                        text+= "Down OFF";
                                } else {
                                    if(_autoChordUp[dev1])
                                        text+= "Up ON";
                                    else
                                        text+= "Up OFF";
                                }

                                OSD = text;
                            }

                        } else {

                            if(evt == 0x90) {

                                if(action.action & 32)
                                    _autoChordDown[dev1] = true;
                                else
                                    _autoChordUp[dev1] = true;

                            } else {

                                if(action.action & 32)
                                    _autoChordDown[dev1] = false;
                                else
                                    _autoChordUp[dev1] = false;
                            }

                            if(action.action & 32) {
                                if(_autoChordDown[dev1])
                                    text+= "Down ON";
                                else
                                    text+= "Down OFF";
                            } else {
                                if(_autoChordUp[dev1])
                                    text+= "Up ON";
                                else
                                    text+= "Up OFF";
                            }

                            OSD = text;

                        }

                        //qWarning("auto chord %i %i", _autoChordUp[dev1] != 0, _autoChordDown[dev1] != 0);
                        return RET_NEWEFFECTS_SKIP;
                    } else if(action.category == CATEGORY_FLUIDSYNTH) { // Fluidsynth events

                            int dev1 = dev;
                            int adev1 = idev;
                            int ch_up1 = ch_up;
                            int ch_down1 = ch_down;

                            if(action.bypass != -1 && action.bypass != idev) {
                                adev1 = action.bypass;
                                dev1 = action.bypass/2;

                                ch_up1 = ((MidiInControl::channelUp(dev1) < 0)
                                                 ? MidiOutput::standardChannel()
                                                 : (MidiInControl::channelUp(dev1) & 15));

                                ch_down1 = ((MidiInControl::channelDown(dev1) < 0)
                                                   ? ((MidiInControl::channelUp(dev1) < 0)
                                                          ? MidiOutput::standardChannel()
                                                          : MidiInControl::channelUp(dev1))
                                                   : MidiInControl::channelDown(dev1)) & 0xF;

                            }

                            QString text = "Dev: " + QString::number(adev1) + " - ";
                            QString text2;

                            if(action.function == FUNCTION_VAL_CLIP) {
                                if(vel < action.min)
                                    vel = action.min;
                                if(vel > action.max)
                                    vel = action.max;

                            } else if(action.function == FUNCTION_VAL_BUTTON) {
                                if(evt == 0x90)
                                    vel = action.max;
                                if(evt == 0x80)
                                    vel = action.min;

                            } else if(action.function == FUNCTION_VAL_SWITCH) {

                                int sw = -1;

                                if(evt == 0x90 || (evt == 0xB0  && (vel >= 64))) {

                                    if(action.action >= 20 && action.action <= 37) {

                                        MidiInput::keys_fluid[adev1]^= (1 << (action.action - 20));
                                        if(MidiInput::keys_fluid[adev1] & (1 << (action.action - 20)))
                                            sw = 1;
                                        else
                                            sw = 0;

                                    }

                                    if(sw == 1) {
                                        vel = action.max;
                                        text2 = " ON ";
                                    } else if(sw == 0) {
                                        vel = action.min;
                                        text2 = " OFF ";
                                    }

                                }

                                if(sw == -1)
                                    return RET_NEWEFFECTS_SKIP; // ignore

                            }

                            if(action.action >= 20 && action.action <= 37) {

                                text+= MidiFile::controlChangeName((action.action >= 32 && action.action <= 37)
                                                                    ? (action.action - 32 + 85)
                                                                    : (action.action == 31 ? 9 : action.action)) + " - " + text2 + "v: " + QString::number(vel);
                                OSD = text;

                                message->at(0) = 0xB0 | ((adev1 & 1) ? ch_down1 : ch_up1);
                                message->at(1) = (action.action >= 32 && action.action <= 37) ? (action.action - 32 + 85)
                                                                                              : ((action.action == 31 ? 9 : action.action));
                                message->at(2) = vel;

                                if(action.bypass != -1 && action.bypass != idev) {
                                    return RET_NEWEFFECTS_BYPASS + action.bypass;
                                }

                                return RET_NEWEFFECTS_SET;

                            }

                            return RET_NEWEFFECTS_NONE;

                    } else if(action.category == CATEGORY_VST1) { // VST1
                        int dev1 = dev;
                        int adev1 = idev;
                        int ch_up1 = ch_up;
                        int ch_down1 = ch_down;

                        if(action.bypass != -1 && action.bypass != idev) {
                            adev1 = action.bypass;
                            dev1 = action.bypass/2;

                            ch_up1 = ((MidiInControl::channelUp(dev1) < 0)
                                             ? MidiOutput::standardChannel()
                                             : (MidiInControl::channelUp(dev1) & 15));

                            ch_down1 = ((MidiInControl::channelDown(dev1) < 0)
                                               ? ((MidiInControl::channelUp(dev1) < 0)
                                                      ? MidiOutput::standardChannel()
                                                      : MidiInControl::channelUp(dev1))
                                               : MidiInControl::channelDown(dev1)) & 0xF;

                        }

                        QString text = "Dev: " + QString::number(adev1) + " - VST1 ";

                        if(action.action & 32) {
                           if(!(adev1 & 1))
                               adev1++;

                           text+= "DOWN - SET ";
                        } else
                           text+= "UP - SET ";

                        if(action.function == FUNCTION_VAL_BUTTON) {
                            if(evt == 0x90)
                                vel = action.action;
                            if(evt == 0x80)
                                vel = action.action2;

                        } else if(action.function == FUNCTION_VAL_SWITCH) {

                            if(evt == 0x90 || (evt == 0xB0 && (vel >= 64))) {

                                if(action.action & 32) {

                                    if(MidiInput::keys_vst[adev1][ch_down1] != (action.action & 31))
                                        MidiInput::keys_vst[adev1][ch_down1] = (action.action & 31);
                                    else
                                        MidiInput::keys_vst[adev1][ch_down1] = (action.action2 & 31);

                                    vel = MidiInput::keys_vst[adev1][ch_down1];
                                } else {

                                   if(MidiInput::keys_vst[adev1][ch_up1] != (action.action & 31))
                                       MidiInput::keys_vst[adev1][ch_up1] = (action.action & 31);
                                   else
                                       MidiInput::keys_vst[adev1][ch_up1] = (action.action2 & 31);

                                   vel = MidiInput::keys_vst[adev1][ch_up1];

                                }

                            } else
                                return RET_NEWEFFECTS_SKIP;

                        }

                        vel&= 31;
                        vel--;
                        if(vel < 0)
                            vel = 0x7f;

                        if(vel == 0x7f)
                            text+= "DISABLED";
                        else
                            text+= "#" + QString::number(vel);

                        OSD = text;

                        message->clear();
                        message->emplace_back((char) 0xf0);
                        message->emplace_back((char) 0x6);
                        message->emplace_back((char) (((adev1 & 1) ? ch_down1 : ch_up1) & 0xf)) ;
                        message->emplace_back((char) 0x66);
                        message->emplace_back((char) 0x66);
                        message->emplace_back((char) 'W');

                        message->emplace_back((char) vel);
                        message->emplace_back((char) 0xf7);

                        if(action.bypass != -1 && action.bypass != idev) {
                            return RET_NEWEFFECTS_BYPASS + action.bypass;
                        }

                        return RET_NEWEFFECTS_SET;

                    } else if(action.category == CATEGORY_VST2) { // VST2
                            int dev1 = dev;
                            int adev1 = idev;
                            int ch_up1 = ch_up;
                            int ch_down1 = ch_down;

                            if(action.bypass != -1 && action.bypass != idev) {
                                adev1 = action.bypass;
                                dev1 = action.bypass/2;

                                ch_up1 = ((MidiInControl::channelUp(dev1) < 0)
                                                 ? MidiOutput::standardChannel()
                                                 : (MidiInControl::channelUp(dev1) & 15));

                                ch_down1 = ((MidiInControl::channelDown(dev1) < 0)
                                                   ? ((MidiInControl::channelUp(dev1) < 0)
                                                          ? MidiOutput::standardChannel()
                                                          : MidiInControl::channelUp(dev1))
                                                   : MidiInControl::channelDown(dev1)) & 0xF;

                            }

                            QString text = "Dev: " + QString::number(adev1) + " - VST2 ";

                            if(action.action & 32) {
                               if(!(adev1 & 1))
                                   adev1++;

                               text+= "DOWN - SET ";
                            } else
                               text+= "UP - SET ";

                            if(action.function == FUNCTION_VAL_BUTTON) {
                                if(evt == 0x90)
                                    vel = action.action;
                                if(evt == 0x80)
                                    vel = action.action2;

                            } else if(action.function == FUNCTION_VAL_SWITCH) {

                                if(evt == 0x90 || (evt == 0xB0  && (vel >= 64))) {

                                    if(action.action & 32) {

                                        if(MidiInput::keys_vst[adev1][ch_down1] != action.action)
                                            MidiInput::keys_vst[adev1][ch_down1] = action.action;
                                        else
                                            MidiInput::keys_vst[adev1][ch_down1] = action.action2;

                                        vel = MidiInput::keys_vst[adev1][ch_down1];
                                    } else {

                                       if(MidiInput::keys_vst[adev1][ch_up1] != action.action)
                                           MidiInput::keys_vst[adev1][ch_up1] = action.action;
                                       else
                                           MidiInput::keys_vst[adev1][ch_up1] = action.action2;

                                       vel = MidiInput::keys_vst[adev1][ch_up1];

                                    }

                                } else
                                    return RET_NEWEFFECTS_SKIP;

                            }

                            vel&= 31;
                            vel--;
                            if(vel < 0)
                                vel = 0x7f;

                            if(vel == 0x7f)
                                text+= "DISABLED";
                            else
                                text+= "#" + QString::number(vel);

                            OSD = text;

                            message->clear();
                            message->emplace_back((char) 0xf0);
                            message->emplace_back((char) 0x6);
                            message->emplace_back((char) 0x10 + (((adev1 & 1) ? ch_down1 : ch_up1) & 0xf)) ;
                            message->emplace_back((char) 0x66);
                            message->emplace_back((char) 0x66);
                            message->emplace_back((char) 'W');

                            message->emplace_back((char) vel);
                            message->emplace_back((char) 0xf7);

                            if(action.bypass != -1 && action.bypass != idev) {
                                return RET_NEWEFFECTS_BYPASS + action.bypass;
                            }

                            return RET_NEWEFFECTS_SET;

                    } else if(action.category == CATEGORY_SEQUENCER) { // sequencer

                        int dev1 = action.device;

                        if(action.bypass != -1 && action.bypass != idev) {

                            dev1 = action.bypass;

                        }

                        QString text = "Dev: " + QString::number(dev1) + " - SEQUENCER -";

                        if(action.action & 32) {
                            if(is_kb2)
                                continue;

                            if(action.action >= SEQUENCER_ON_1_DOWN && action.action <= SEQUENCER_ON_4_DOWN) {
                                FingerPatternDialog::Finger_Action(dev1, 33, true, 1);
                                FingerPatternDialog::Finger_Action(dev1, 34, true, 1);
                                FingerPatternDialog::Finger_Action(dev1, 35, true, 1);
                            }

                            if(action.action == SEQUENCER_ON_1_DOWN) {

                                if(evt == 0x90 || evt == 0x80 || (action.event == EVENT_CONTROL && evt == 0xb0 && (vel >= 64))) {

                                    int seq_thd = dev1 + 1 * (!(dev1 & 1));

                                    int swx = 0;

                                    if(action.function == FUNCTION_VAL_BUTTON) {
                                       if(evt == 0x90 && MidiOutput::sequencer_enabled[seq_thd] == 0)
                                           return RET_NEWEFFECTS_SKIP; // ignore it
                                       if(evt == 0x80 && MidiOutput::sequencer_enabled[seq_thd] < 0)
                                           return RET_NEWEFFECTS_SKIP; // ignore it
                                       if(evt == 0x80)
                                           swx = 1;
                                       else if(evt == 0x90)
                                           swx = 2;


                                    } else if(evt == 0x80)
                                        return RET_NEWEFFECTS_SKIP; // ignore it

                                    if(swx == 2 || (swx == 0 && MidiOutput::sequencer_enabled[seq_thd] != 0)) {
                                        swx = 2;
                                        seqOn(seq_thd, 0, true);
                                        text+= " 1 DOWN - ON";

                                    } else {

                                        seqOn(seq_thd, 0, false);
                                        text+= " 1 DOWN - OFF";
                                    }

                                    OSD = text;

                                    if(is_aftertouch && swx == 2) {
                                        // start with previous note
                                        std::vector<unsigned char> message2;
                                        int my_id = DEVICE_ID + idev;
                                        message2 = MidiInput::message[idev];
                                        if(message2.size() == 3)
                                            MidiInput::receiveMessage(0.0, &message2, &my_id);
                                    }

                                    return RET_NEWEFFECTS_SKIP;
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == SEQUENCER_ON_2_DOWN) {

                                if(evt == 0x90 || evt == 0x80 || (action.event == EVENT_CONTROL && evt == 0xb0 && (vel >= 64))) {

                                    int seq_thd = dev1 + 1 * (!(dev1 & 1));

                                    int swx = 0;

                                    if(action.function == FUNCTION_VAL_BUTTON) {
                                       if(evt == 0x90 && MidiOutput::sequencer_enabled[seq_thd] == 1)
                                           return RET_NEWEFFECTS_SKIP; // ignore it
                                       if(evt == 0x80 && MidiOutput::sequencer_enabled[seq_thd] < 0)
                                           return RET_NEWEFFECTS_SKIP; // ignore it
                                       if(evt == 0x80)
                                           swx = 1;
                                       else if(evt == 0x90)
                                           swx = 2;

                                    } else if(evt == 0x80)
                                        return RET_NEWEFFECTS_SKIP; // ignore it

                                    if(swx == 2 || (swx == 0 && MidiOutput::sequencer_enabled[seq_thd] != 1)) {
                                        swx = 2;
                                        seqOn(seq_thd, 1, true);
                                        text+= " 2 DOWN - ON";
                                    } else {
                                        seqOn(seq_thd, 1, false);
                                        text+= " 2 DOWN - OFF";
                                    }

                                    OSD = text;

                                    if(is_aftertouch && swx == 2) {
                                        // start with previous note
                                        std::vector<unsigned char> message2;
                                        int my_id = DEVICE_ID + idev;
                                        message2 = MidiInput::message[idev];
                                        if(message2.size() == 3)
                                            MidiInput::receiveMessage(0.0, &message2, &my_id);
                                    }

                                    return RET_NEWEFFECTS_SKIP;
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == SEQUENCER_ON_3_DOWN) {

                                if(evt == 0x90 || evt == 0x80 || (action.event == EVENT_CONTROL && evt == 0xb0 && (vel >= 64))) {

                                    int seq_thd = dev1 + 1 * (!(dev1 & 1));

                                    int swx = 0;

                                    if(action.function == FUNCTION_VAL_BUTTON) {
                                       if(evt == 0x90 && MidiOutput::sequencer_enabled[seq_thd] == 2)
                                           return RET_NEWEFFECTS_SKIP; // ignore it
                                       if(evt == 0x80 && MidiOutput::sequencer_enabled[seq_thd] < 0)
                                           return RET_NEWEFFECTS_SKIP; // ignore it
                                       if(evt == 0x80)
                                           swx = 1;
                                       else if(evt == 0x90)
                                           swx = 2;


                                    } else if(evt == 0x80)
                                        return RET_NEWEFFECTS_SKIP; // ignore it

                                    if(swx == 2 || (swx == 0 && MidiOutput::sequencer_enabled[seq_thd] != 2)) {
                                        swx = 2;
                                        seqOn(seq_thd, 2, true);
                                        text+= " 3 DOWN - ON";
                                    } else {
                                        seqOn(seq_thd, 2, false);
                                        text+= " 3 DOWN - OFF";
                                    }

                                    OSD = text;

                                    if(is_aftertouch && swx == 2) {
                                        // start with previous note
                                        std::vector<unsigned char> message2;
                                        int my_id = DEVICE_ID + idev;
                                        message2 = MidiInput::message[idev];
                                        if(message2.size() == 3)
                                            MidiInput::receiveMessage(0.0, &message2, &my_id);
                                    }


                                    return RET_NEWEFFECTS_SKIP;
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == SEQUENCER_ON_4_DOWN) {

                                if(evt == 0x90 || evt == 0x80 || (action.event == EVENT_CONTROL && evt == 0xb0 && (vel >= 64))) {

                                    int seq_thd = dev1 + 1 * (!(dev1 & 1));

                                    int swx = 0;

                                    if(action.function == FUNCTION_VAL_BUTTON) {
                                       if(evt == 0x90 && MidiOutput::sequencer_enabled[seq_thd] == 3)
                                           return RET_NEWEFFECTS_SKIP; // ignore it
                                       if(evt == 0x80 && MidiOutput::sequencer_enabled[seq_thd] < 0)
                                           return RET_NEWEFFECTS_SKIP; // ignore it
                                       if(evt == 0x80)
                                           swx = 1;
                                       else if(evt == 0x90)
                                           swx = 2;


                                    } else if(evt == 0x80)
                                        return RET_NEWEFFECTS_SKIP; // ignore it

                                    if(swx == 2 || (swx == 0 && MidiOutput::sequencer_enabled[seq_thd] != 3)) {
                                        swx = 2;
                                        seqOn(seq_thd, 3, true);
                                        text+= " 4 DOWN - ON";
                                    } else {
                                        seqOn(seq_thd, 3, false);
                                        text+= " 4 DOWN - OFF";
                                    }

                                    OSD = text;

                                    if(is_aftertouch && swx == 2) {
                                        // start with previous note
                                        std::vector<unsigned char> message2;
                                        int my_id = DEVICE_ID + idev;
                                        message2 = MidiInput::message[idev];
                                        if(message2.size() == 3)
                                            MidiInput::receiveMessage(0.0, &message2, &my_id);
                                    }


                                    return RET_NEWEFFECTS_SKIP;
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == SEQUENCER_SCALE_1_DOWN) {

                                int seq_thd = dev1 + 1 * (!(dev1 & 1));

                                if(MidiOutput::sequencer_enabled[seq_thd] < 0) {

                                    delayed_return1 = true;
                                    continue;
                                }

                                if(action.function == FUNCTION_VAL_CLIP) {
                                    if(vel < action.min)
                                        vel = action.min;
                                    if(vel > action.max)
                                        vel = action.max;
                                } else if(action.function == FUNCTION_VAL_BUTTON) {
                                    if(evt == 0x90)
                                        vel = action.max;
                                    if(evt == 0x80)
                                        vel = action.min;
                                } else if(action.function == FUNCTION_VAL_SWITCH) {
                                    int sw = -1;

                                    if(evt == 0x90 || (evt == 0xB0 && (vel >= 64))) {

                                        MidiInput::keys_seq[seq_thd]^= KEY_SEQ_S1_DOWN;
                                        if(MidiInput::keys_seq[seq_thd] & KEY_SEQ_S1_DOWN)
                                            sw = 1;
                                        else
                                            sw = 0;

                                        if(sw == 1)
                                            vel = action.max;
                                        else if(sw == 0)
                                            vel = action.min;
                                    }

                                    if(sw == -1)
                                        return RET_NEWEFFECTS_SKIP; // ignore

                                }

                                if(MidiPlayer::fileSequencer[seq_thd]) {
                                    text+= " 1 SCALE TIME DOWN - BPM: " + QString::number(vel * 5);
                                    OSD = text;
                                    MidiPlayer::fileSequencer[seq_thd]->setScaleTime(vel * 5, 0);
                                } else {
                                    text+= " 1 SCALE TIME DOWN - IS OFF";
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == SEQUENCER_SCALE_2_DOWN) {

                                int seq_thd = dev1 + 1 * (!(dev1 & 1));

                                if(MidiOutput::sequencer_enabled[seq_thd] < 0) {

                                    delayed_return1 = true;
                                    continue;
                                }

                                if(action.function == FUNCTION_VAL_CLIP) {
                                    if(vel < action.min)
                                        vel = action.min;
                                    if(vel > action.max)
                                        vel = action.max;
                                } else if(action.function == FUNCTION_VAL_BUTTON) {
                                    if(evt == 0x90)
                                        vel = action.max;
                                    if(evt == 0x80)
                                        vel = action.min;
                                } else if(action.function == FUNCTION_VAL_SWITCH) {
                                    int sw = -1;

                                    if(evt == 0x90 || (evt == 0xB0 && (vel >= 64))) {

                                        MidiInput::keys_seq[seq_thd]^= KEY_SEQ_S2_DOWN;
                                        if(MidiInput::keys_seq[seq_thd] & KEY_SEQ_S2_DOWN)
                                            sw = 1;
                                        else
                                            sw = 0;

                                        if(sw == 1)
                                            vel = action.max;
                                        else if(sw == 0)
                                            vel = action.min;
                                    }


                                    if(sw == -1)
                                        return RET_NEWEFFECTS_SKIP; // ignore

                                }

                                if(MidiPlayer::fileSequencer[seq_thd]) {
                                    text+= " 2 SCALE TIME DOWN - BPM: " + QString::number(vel * 5);
                                    OSD = text;
                                    MidiPlayer::fileSequencer[seq_thd]->setScaleTime(vel * 5, 1);
                                } else {
                                    text+= " 2 SCALE TIME DOWN - IS OFF";
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == SEQUENCER_SCALE_3_DOWN) {

                                int seq_thd = dev1 + 1 * (!(dev1 & 1));

                                if(MidiOutput::sequencer_enabled[seq_thd] < 0) {

                                    delayed_return1 = true;
                                    continue;
                                }

                                if(action.function == FUNCTION_VAL_CLIP) {
                                    if(vel < action.min)
                                        vel = action.min;
                                    if(vel > action.max)
                                        vel = action.max;
                                } else if(action.function == FUNCTION_VAL_BUTTON) {
                                    if(evt == 0x90)
                                        vel = action.max;
                                    if(evt == 0x80)
                                        vel = action.min;
                                } else if(action.function == FUNCTION_VAL_SWITCH) {
                                    int sw = -1;

                                    if(evt == 0x90 || (evt == 0xB0 && (vel >= 64))) {

                                        MidiInput::keys_seq[seq_thd]^= KEY_SEQ_S3_DOWN;
                                        if(MidiInput::keys_seq[seq_thd] & KEY_SEQ_S3_DOWN)
                                            sw = 1;
                                        else
                                            sw = 0;

                                        if(sw == 1)
                                            vel = action.max;
                                        else if(sw == 0)
                                            vel = action.min;
                                    }


                                    if(sw == -1)
                                        return RET_NEWEFFECTS_SKIP; // ignore

                                }

                                if(MidiPlayer::fileSequencer[seq_thd]) {
                                    text+= " 3 SCALE TIME DOWN - BPM: " + QString::number(vel * 5);
                                    OSD = text;
                                    MidiPlayer::fileSequencer[seq_thd]->setScaleTime(vel * 5, 2);
                                } else {
                                    text+= " 3 SCALE TIME DOWN - IS OFF";
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == SEQUENCER_SCALE_4_DOWN) {

                                int seq_thd = dev1 + 1 * (!(dev1 & 1));

                                if(MidiOutput::sequencer_enabled[seq_thd] < 0) {

                                    delayed_return1 = true;
                                    continue;
                                }

                                if(action.function == FUNCTION_VAL_CLIP) {
                                    if(vel < action.min)
                                        vel = action.min;
                                    if(vel > action.max)
                                        vel = action.max;
                                } else if(action.function == FUNCTION_VAL_BUTTON) {
                                    if(evt == 0x90)
                                        vel = action.max;
                                    if(evt == 0x80)
                                        vel = action.min;
                                } else if(action.function == FUNCTION_VAL_SWITCH) {
                                    int sw = -1;

                                    if(evt == 0x90 || (evt == 0xB0 && (vel >= 64))) {

                                        MidiInput::keys_seq[seq_thd]^= KEY_SEQ_S4_DOWN;
                                        if(MidiInput::keys_seq[seq_thd] & KEY_SEQ_S4_DOWN)
                                            sw = 1;
                                        else
                                            sw = 0;

                                        if(sw == 1)
                                            vel = action.max;
                                        else if(sw == 0)
                                            vel = action.min;
                                    }


                                    if(sw == -1)
                                        return RET_NEWEFFECTS_SKIP; // ignore

                                }

                                if(MidiPlayer::fileSequencer[seq_thd]) {
                                    text+= " 4 SCALE TIME DOWN - BPM: " + QString::number(vel * 5);
                                    OSD = text;
                                    MidiPlayer::fileSequencer[seq_thd]->setScaleTime(vel * 5, 3);
                                } else {
                                    text+= " 4 SCALE TIME DOWN - IS OFF";
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == SEQUENCER_SCALE_ALL_DOWN) {

                                int seq_thd = dev1 + 1 * (!(dev1 & 1));

                                if(MidiOutput::sequencer_enabled[seq_thd] < 0) {

                                    delayed_return1 = true;
                                    continue;
                                }

                                if(action.function == FUNCTION_VAL_CLIP) {
                                    if(vel < action.min)
                                        vel = action.min;
                                    if(vel > action.max)
                                        vel = action.max;
                                } else if(action.function == FUNCTION_VAL_BUTTON) {
                                    if(evt == 0x90)
                                        vel = action.max;
                                    if(evt == 0x80)
                                        vel = action.min;
                                } else if(action.function == FUNCTION_VAL_SWITCH) {
                                    int sw = -1;

                                    if(evt == 0x90 || (evt == 0xB0 && (vel >= 64))) {

                                        MidiInput::keys_seq[seq_thd]^= KEY_SEQ_SALL_DOWN;
                                        if(MidiInput::keys_seq[seq_thd] & KEY_SEQ_SALL_DOWN)
                                            sw = 1;
                                        else
                                            sw = 0;

                                        if(sw == 1)
                                            vel = action.max;
                                        else if(sw == 0)
                                            vel = action.min;
                                    }


                                    if(sw == -1)
                                        return RET_NEWEFFECTS_SKIP; // ignore

                                }

                                if(MidiPlayer::fileSequencer[seq_thd]) {
                                    text+= " ALL SCALE TIME DOWN - BPM: " + QString::number(vel * 5);
                                    OSD = text;
                                    MidiPlayer::fileSequencer[seq_thd]->setScaleTime(vel * 5, -1);
                                } else {
                                    text+= " ALL SCALE TIME DOWN - IS OFF";
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == SEQUENCER_VOLUME_1_DOWN) {

                                int seq_thd = dev1 + 1 * (!(dev1 & 1));

                                if(MidiOutput::sequencer_enabled[seq_thd] < 0) {

                                    delayed_return1 = true;
                                    continue;
                                }

                                if(action.function == FUNCTION_VAL_CLIP) {
                                    if(vel < action.min)
                                        vel = action.min;
                                    if(vel > action.max)
                                        vel = action.max;
                                } else if(action.function == FUNCTION_VAL_BUTTON) {
                                    if(evt == 0x90)
                                        vel = action.max;
                                    if(evt == 0x80)
                                        vel = action.min;
                                } else if(action.function == FUNCTION_VAL_SWITCH) {
                                    int sw = -1;

                                    if(evt == 0x90 || (evt == 0xB0 && (vel >= 64))) {

                                        MidiInput::keys_seq[seq_thd]^= KEY_SEQ_V1_DOWN;
                                        if(MidiInput::keys_seq[seq_thd] & KEY_SEQ_V1_DOWN)
                                            sw = 1;
                                        else
                                            sw = 0;

                                        if(sw == 1)
                                            vel = action.max;
                                        else if(sw == 0)
                                            vel = action.min;
                                    }

                                    if(sw == -1)
                                        return RET_NEWEFFECTS_SKIP; // ignore

                                }

                                if(MidiPlayer::fileSequencer[seq_thd]) {
                                    text+= " 1 VOLUME SCALE DOWN - VOL: " + QString::number(vel);
                                    OSD = text;
                                    MidiPlayer::fileSequencer[seq_thd]->setVolume(vel, 0);
                                } else {
                                    text+= " 1 VOLUME SCALE DOWN - IS OFF";
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == SEQUENCER_VOLUME_2_DOWN) {

                                int seq_thd = dev1 + 1 * (!(dev1 & 1));

                                if(MidiOutput::sequencer_enabled[seq_thd] < 0) {

                                    delayed_return1 = true;
                                    continue;
                                }

                                if(action.function == FUNCTION_VAL_CLIP) {
                                    if(vel < action.min)
                                        vel = action.min;
                                    if(vel > action.max)
                                        vel = action.max;
                                } else if(action.function == FUNCTION_VAL_BUTTON) {
                                    if(evt == 0x90)
                                        vel = action.max;
                                    if(evt == 0x80)
                                        vel = action.min;
                                } else if(action.function == FUNCTION_VAL_SWITCH) {
                                    int sw = -1;

                                    if(evt == 0x90 || (evt == 0xB0  && (vel >= 64))) {

                                        MidiInput::keys_seq[seq_thd]^= KEY_SEQ_V2_DOWN;
                                        if(MidiInput::keys_seq[seq_thd] & KEY_SEQ_V2_DOWN)
                                            sw = 1;
                                        else
                                            sw = 0;

                                        if(sw == 1)
                                            vel = action.max;
                                        else if(sw == 0)
                                            vel = action.min;
                                    }


                                    if(sw == -1)
                                        return RET_NEWEFFECTS_SKIP; // ignore

                                }

                                if(MidiPlayer::fileSequencer[seq_thd]) {
                                    text+= " 2 VOLUME SCALE DOWN - VOL: " + QString::number(vel);
                                    OSD = text;
                                    MidiPlayer::fileSequencer[seq_thd]->setVolume(vel, 1);
                                } else {
                                    text+= " 2 VOLUME SCALE DOWN - IS OFF";
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == SEQUENCER_VOLUME_3_DOWN) {

                                int seq_thd = dev1 + 1 * (!(dev1 & 1));

                                if(MidiOutput::sequencer_enabled[seq_thd] < 0) {

                                    delayed_return1 = true;
                                    continue;
                                }

                                if(action.function == FUNCTION_VAL_CLIP) {
                                    if(vel < action.min)
                                        vel = action.min;
                                    if(vel > action.max)
                                        vel = action.max;
                                } else if(action.function == FUNCTION_VAL_BUTTON) {
                                    if(evt == 0x90)
                                        vel = action.max;
                                    if(evt == 0x80)
                                        vel = action.min;
                                } else if(action.function == FUNCTION_VAL_SWITCH) {
                                    int sw = -1;

                                    if(evt == 0x90 || (evt == 0xB0 && (vel >= 64))) {

                                        MidiInput::keys_seq[seq_thd]^= KEY_SEQ_V3_DOWN;
                                        if(MidiInput::keys_seq[seq_thd] & KEY_SEQ_V3_DOWN)
                                            sw = 1;
                                        else
                                            sw = 0;

                                        if(sw == 1)
                                            vel = action.max;
                                        else if(sw == 0)
                                            vel = action.min;
                                    }


                                    if(sw == -1)
                                        return RET_NEWEFFECTS_SKIP; // ignore

                                }

                                if(MidiPlayer::fileSequencer[seq_thd]) {
                                    text+= " 3 VOLUME SCALE DOWN - VOL: " + QString::number(vel);
                                    OSD = text;
                                    MidiPlayer::fileSequencer[seq_thd]->setVolume(vel, 2);
                                } else {
                                    text+= " 3 VOLUME SCALE DOWN - IS OFF";
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == SEQUENCER_VOLUME_4_DOWN) {

                                int seq_thd = dev1 + 1 * (!(dev1 & 1));

                                if(MidiOutput::sequencer_enabled[seq_thd] < 0) {

                                    delayed_return1 = true;
                                    continue;
                                }

                                if(action.function == FUNCTION_VAL_CLIP) {
                                    if(vel < action.min)
                                        vel = action.min;
                                    if(vel > action.max)
                                        vel = action.max;
                                } else if(action.function == FUNCTION_VAL_BUTTON) {
                                    if(evt == 0x90)
                                        vel = action.max;
                                    if(evt == 0x80)
                                        vel = action.min;
                                } else if(action.function == FUNCTION_VAL_SWITCH) {
                                    int sw = -1;

                                    if(evt == 0x90 || (evt == 0xB0 && (vel >= 64))) {

                                        MidiInput::keys_seq[seq_thd]^= KEY_SEQ_V4_DOWN;
                                        if(MidiInput::keys_seq[seq_thd] & KEY_SEQ_V4_DOWN)
                                            sw = 1;
                                        else
                                            sw = 0;

                                        if(sw == 1)
                                            vel = action.max;
                                        else if(sw == 0)
                                            vel = action.min;
                                    }


                                    if(sw == -1)
                                        return RET_NEWEFFECTS_SKIP; // ignore

                                }

                                if(MidiPlayer::fileSequencer[seq_thd]) {
                                    text+= " 4 VOLUME SCALE DOWN - VOL: " + QString::number(vel);
                                    OSD = text;
                                    MidiPlayer::fileSequencer[seq_thd]->setVolume(vel, 3);
                                } else {
                                    text+= " 4 VOLUME SCALE DOWN - IS OFF";
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == SEQUENCER_VOLUME_ALL_DOWN) {

                                int seq_thd = dev1 + 1 * (!(dev1 & 1));

                                if(MidiOutput::sequencer_enabled[seq_thd] < 0) {

                                    delayed_return1 = true;
                                    continue;
                                }

                                if(action.function == FUNCTION_VAL_CLIP) {
                                    if(vel < action.min)
                                        vel = action.min;
                                    if(vel > action.max)
                                        vel = action.max;
                                } else if(action.function == FUNCTION_VAL_BUTTON) {
                                    if(evt == 0x90)
                                        vel = action.max;
                                    if(evt == 0x80)
                                        vel = action.min;
                                } else if(action.function == FUNCTION_VAL_SWITCH) {
                                    int sw = -1;

                                    if(evt == 0x90 || (evt == 0xB0 && (vel >= 64))) {

                                        MidiInput::keys_seq[seq_thd]^= KEY_SEQ_VALL_DOWN;
                                        if(MidiInput::keys_seq[seq_thd] & KEY_SEQ_VALL_DOWN)
                                            sw = 1;
                                        else
                                            sw = 0;

                                        if(sw == 1)
                                            vel = action.max;
                                        else if(sw == 0)
                                            vel = action.min;
                                    }


                                    if(sw == -1)
                                        return RET_NEWEFFECTS_SKIP; // ignore

                                }

                                if(MidiPlayer::fileSequencer[seq_thd]) {
                                    text+= " ALL VOLUME SCALE DOWN - VOL: " + QString::number(vel);
                                    OSD = text;
                                    MidiPlayer::fileSequencer[seq_thd]->setVolume(vel, -1);
                                } else {
                                    text+= " ALL VOLUME SCALE DOWN - IS OFF";
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                        } else {

                            if(action.action >= SEQUENCER_ON_1_UP && action.action <= SEQUENCER_ON_4_UP) {
                                FingerPatternDialog::Finger_Action(dev1, 1, true, 1);
                                FingerPatternDialog::Finger_Action(dev1, 2, true, 1);
                                FingerPatternDialog::Finger_Action(dev1, 3, true, 1);
                            }

                            if(action.action == SEQUENCER_ON_1_UP) {

                                if(evt == 0x90 || evt == 0x80 || (action.event == EVENT_CONTROL && evt == 0xb0 && (vel >= 64))) {

                                    int seq_thd = dev1;

                                    int swx = 0;

                                    if(action.function == FUNCTION_VAL_BUTTON) {
                                       if(evt == 0x90 && MidiOutput::sequencer_enabled[seq_thd] == 0)
                                           return RET_NEWEFFECTS_SKIP; // ignore it
                                       if(evt == 0x80 && MidiOutput::sequencer_enabled[seq_thd] < 0)
                                           return RET_NEWEFFECTS_SKIP; // ignore it
                                       if(evt == 0x80)
                                           swx = 1;
                                       else if(evt == 0x90)
                                           swx = 2;


                                    } else if(evt == 0x80)
                                        return RET_NEWEFFECTS_SKIP; // ignore it

                                    if(swx == 2 || (swx == 0 && MidiOutput::sequencer_enabled[seq_thd] != 0)) {
                                        swx = 2;
                                        seqOn(seq_thd, 0, true);
                                        text+= " 1 UP - ON";
                                    } else {
                                        seqOn(seq_thd, 0, false);
                                        text+= " 1 UP - OFF";
                                    }

                                    OSD = text;

                                    if(is_aftertouch && swx == 2) {
                                        // start with previous note
                                        std::vector<unsigned char> message2;
                                        int my_id = DEVICE_ID + idev;
                                        message2 = MidiInput::message[idev];
                                        if(message2.size() == 3)
                                            MidiInput::receiveMessage(0.0, &message2, &my_id);
                                    }

                                    return RET_NEWEFFECTS_SKIP;
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == SEQUENCER_ON_2_UP) {

                                if(evt == 0x90 || evt == 0x80 || (action.event == EVENT_CONTROL && evt == 0xb0 && (vel >= 64))) {

                                    int seq_thd = dev1;

                                    int swx = 0;

                                    if(action.function == FUNCTION_VAL_BUTTON) {
                                       if(evt == 0x90 && MidiOutput::sequencer_enabled[seq_thd] == 1)
                                           return RET_NEWEFFECTS_SKIP; // ignore it
                                       if(evt == 0x80 && MidiOutput::sequencer_enabled[seq_thd] < 0)
                                           return RET_NEWEFFECTS_SKIP; // ignore it
                                       if(evt == 0x80)
                                           swx = 1;
                                       else if(evt == 0x90)
                                           swx = 2;


                                    } else if(evt == 0x80)
                                        return RET_NEWEFFECTS_SKIP; // ignore it

                                    if(swx == 2 || (swx == 0 && MidiOutput::sequencer_enabled[seq_thd] != 1)) {
                                        swx = 2;
                                        seqOn(seq_thd, 1, true);
                                        text+= " 2 UP - ON";
                                    } else {
                                        seqOn(seq_thd, 1, false);
                                        text+= " 2 UP - OFF";
                                    }

                                    OSD = text;

                                    if(is_aftertouch && swx == 2) {
                                        // start with previous note
                                        std::vector<unsigned char> message2;
                                        int my_id = DEVICE_ID + idev;
                                        message2 = MidiInput::message[idev];
                                        if(message2.size() == 3)
                                            MidiInput::receiveMessage(0.0, &message2, &my_id);
                                    }

                                    return RET_NEWEFFECTS_SKIP;
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == SEQUENCER_ON_3_UP) {

                                if(evt == 0x90 || evt == 0x80 || (action.event == EVENT_CONTROL && evt == 0xb0 && (vel >= 64))) {

                                    int seq_thd = dev1;

                                    int swx = 0;

                                    if(action.function == FUNCTION_VAL_BUTTON) {
                                       if(evt == 0x90 && MidiOutput::sequencer_enabled[seq_thd] == 2)
                                           return RET_NEWEFFECTS_SKIP; // ignore it
                                       if(evt == 0x80 && MidiOutput::sequencer_enabled[seq_thd] < 0)
                                           return RET_NEWEFFECTS_SKIP; // ignore it
                                       if(evt == 0x80)
                                           swx = 1;
                                       else if(evt == 0x90)
                                           swx = 2;


                                    } else if(evt == 0x80)
                                        return RET_NEWEFFECTS_SKIP; // ignore it

                                    if(swx == 2 || (swx == 0 && MidiOutput::sequencer_enabled[seq_thd] != 2)) {
                                        swx = 2;
                                        seqOn(seq_thd, 2, true);
                                        text+= " 3 UP - ON";
                                    } else {
                                        seqOn(seq_thd, 2, false);
                                        text+= " 3 UP - OFF";
                                    }

                                    OSD = text;

                                    if(is_aftertouch && swx == 2) {
                                        // start with previous note
                                        std::vector<unsigned char> message2;
                                        int my_id = DEVICE_ID + idev;
                                        message2 = MidiInput::message[idev];
                                        if(message2.size() == 3)
                                            MidiInput::receiveMessage(0.0, &message2, &my_id);
                                    }


                                    return RET_NEWEFFECTS_SKIP;
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == SEQUENCER_ON_4_UP) {

                                if(evt == 0x90 || evt == 0x80 || (action.event == EVENT_CONTROL && evt == 0xb0 && (vel >= 64))) {

                                    int seq_thd = dev1;

                                    int swx = 0;

                                    if(action.function == FUNCTION_VAL_BUTTON) {
                                       if(evt == 0x90 && MidiOutput::sequencer_enabled[seq_thd] == 3)
                                           return RET_NEWEFFECTS_SKIP; // ignore it
                                       if(evt == 0x80 && MidiOutput::sequencer_enabled[seq_thd] < 0)
                                           return RET_NEWEFFECTS_SKIP; // ignore it
                                       if(evt == 0x80)
                                           swx = 1;
                                       else if(evt == 0x90)
                                           swx = 2;


                                    } else if(evt == 0x80)
                                        return RET_NEWEFFECTS_SKIP; // ignore it

                                    if(swx == 2 || (swx == 0 && MidiOutput::sequencer_enabled[seq_thd] != 3)) {
                                        swx = 2;
                                        seqOn(seq_thd, 3, true);
                                        text+= " 4 UP - ON";
                                    } else {
                                        seqOn(seq_thd, 3, false);
                                        text+= " 4 UP - OFF";
                                    }

                                    OSD = text;

                                    if(is_aftertouch && swx == 2) {
                                        // start with previous note
                                        std::vector<unsigned char> message2;
                                        int my_id = DEVICE_ID + idev;
                                        message2 = MidiInput::message[idev];
                                        if(message2.size() == 3)
                                            MidiInput::receiveMessage(0.0, &message2, &my_id);
                                    }

                                    return RET_NEWEFFECTS_SKIP;
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == SEQUENCER_SCALE_1_UP) {

                                int seq_thd = dev1;

                                if(MidiOutput::sequencer_enabled[seq_thd] < 0) {

                                    delayed_return1 = true;
                                    continue;
                                }

                                if(action.function == FUNCTION_VAL_CLIP) {
                                    if(vel < action.min)
                                        vel = action.min;
                                    if(vel > action.max)
                                        vel = action.max;
                                } else if(action.function == FUNCTION_VAL_BUTTON) {
                                    if(evt == 0x90)
                                        vel = action.max;
                                    if(evt == 0x80)
                                        vel = action.min;
                                } else if(action.function == FUNCTION_VAL_SWITCH) {
                                    int sw = -1;

                                    if(evt == 0x90 || (evt == 0xB0 && (vel >= 64))) {

                                        MidiInput::keys_seq[seq_thd]^= KEY_SEQ_S1_UP;
                                        if(MidiInput::keys_seq[seq_thd] & KEY_SEQ_S1_UP)
                                            sw = 1;
                                        else
                                            sw = 0;

                                        if(sw == 1)
                                            vel = action.max;
                                        else if(sw == 0)
                                            vel = action.min;
                                    }


                                    if(sw == -1)
                                        return RET_NEWEFFECTS_SKIP; // ignore

                                }

                                if(MidiPlayer::fileSequencer[seq_thd]) {
                                    text+= " 1 SCALE TIME UP - BPM: " + QString::number(vel * 5);
                                    OSD = text;
                                    MidiPlayer::fileSequencer[seq_thd]->setScaleTime(vel * 5, 0);
                                } else {
                                    text+= " 1 VOLUME SCALE UP - IS OFF";
                                    OSD = text;
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == SEQUENCER_SCALE_2_UP) {

                                int seq_thd = dev1;

                                if(MidiOutput::sequencer_enabled[seq_thd] < 0) {

                                    delayed_return1 = true;
                                    continue;
                                }

                                if(action.function == FUNCTION_VAL_CLIP) {
                                    if(vel < action.min)
                                        vel = action.min;
                                    if(vel > action.max)
                                        vel = action.max;
                                } else if(action.function == FUNCTION_VAL_BUTTON) {
                                    if(evt == 0x90)
                                        vel = action.max;
                                    if(evt == 0x80)
                                        vel = action.min;
                                } else if(action.function == FUNCTION_VAL_SWITCH) {
                                    int sw = -1;

                                    if(evt == 0x90 || (evt == 0xB0 && (vel >= 64))) {

                                        MidiInput::keys_seq[seq_thd]^= KEY_SEQ_S2_UP;
                                        if(MidiInput::keys_seq[seq_thd] & KEY_SEQ_S2_UP)
                                            sw = 1;
                                        else
                                            sw = 0;

                                        if(sw == 1)
                                            vel = action.max;
                                        else if(sw == 0)
                                            vel = action.min;
                                    }


                                    if(sw == -1)
                                        return RET_NEWEFFECTS_SKIP; // ignore

                                }

                                if(MidiPlayer::fileSequencer[seq_thd]) {
                                    text+= " 2 SCALE TIME UP - BPM: " + QString::number(vel * 5);
                                    OSD = text;
                                    MidiPlayer::fileSequencer[seq_thd]->setScaleTime(vel * 5, 1);
                                } else {
                                    text+= " 2 VOLUME SCALE UP - IS OFF";
                                    OSD = text;
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == SEQUENCER_SCALE_3_UP) {

                                int seq_thd = dev1;

                                if(MidiOutput::sequencer_enabled[seq_thd] < 0) {

                                    delayed_return1 = true;
                                    continue;
                                }

                                if(action.function == FUNCTION_VAL_CLIP) {
                                    if(vel < action.min)
                                        vel = action.min;
                                    if(vel > action.max)
                                        vel = action.max;
                                } else if(action.function == FUNCTION_VAL_BUTTON) {
                                    if(evt == 0x90)
                                        vel = action.max;
                                    if(evt == 0x80)
                                        vel = action.min;
                                } else if(action.function == FUNCTION_VAL_SWITCH) {
                                    int sw = -1;

                                    if(evt == 0x90 || (evt == 0xB0 && (vel >= 64))) {

                                        MidiInput::keys_seq[seq_thd]^= KEY_SEQ_S3_UP;
                                        if(MidiInput::keys_seq[seq_thd] & KEY_SEQ_S3_UP)
                                            sw = 1;
                                        else
                                            sw = 0;

                                        if(sw == 1)
                                            vel = action.max;
                                        else if(sw == 0)
                                            vel = action.min;
                                    }


                                    if(sw == -1)
                                        return RET_NEWEFFECTS_SKIP; // ignore

                                }

                                if(MidiPlayer::fileSequencer[seq_thd]) {
                                    text+= " 3 SCALE TIME UP - BPM: " + QString::number(vel * 5);
                                    OSD = text;
                                    MidiPlayer::fileSequencer[seq_thd]->setScaleTime(vel * 5, 2);
                                } else {
                                    text+= " 3 VOLUME SCALE UP - IS OFF";
                                    OSD = text;
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == SEQUENCER_SCALE_4_UP) {

                                int seq_thd = dev1;

                                if(MidiOutput::sequencer_enabled[seq_thd] < 0) {

                                    delayed_return1 = true;
                                    continue;
                                }

                                if(action.function == FUNCTION_VAL_CLIP) {
                                    if(vel < action.min)
                                        vel = action.min;
                                    if(vel > action.max)
                                        vel = action.max;
                                } else if(action.function == FUNCTION_VAL_BUTTON) {
                                    if(evt == 0x90)
                                        vel = action.max;
                                    if(evt == 0x80)
                                        vel = action.min;
                                } else if(action.function == FUNCTION_VAL_SWITCH) {
                                    int sw = -1;

                                    if(evt == 0x90 || (evt == 0xB0 && (vel >= 64))) {

                                        MidiInput::keys_seq[seq_thd]^= KEY_SEQ_S4_UP;
                                        if(MidiInput::keys_seq[seq_thd] & KEY_SEQ_S4_UP)
                                            sw = 1;
                                        else
                                            sw = 0;

                                        if(sw == 1)
                                            vel = action.max;
                                        else if(sw == 0)
                                            vel = action.min;
                                    }


                                    if(sw == -1)
                                        return RET_NEWEFFECTS_SKIP; // ignore

                                }

                                if(MidiPlayer::fileSequencer[seq_thd])  {
                                    text+= " 4 SCALE TIME UP - BPM: " + QString::number(vel * 5);
                                    OSD = text;
                                    MidiPlayer::fileSequencer[seq_thd]->setScaleTime(vel * 5, 3);
                                } else {
                                    text+= " 4 VOLUME SCALE UP - IS OFF";
                                    OSD = text;
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == SEQUENCER_SCALE_ALL_UP) {

                                int seq_thd = dev1;

                                if(MidiOutput::sequencer_enabled[seq_thd] < 0) {

                                    delayed_return1 = true;
                                    continue;
                                }

                                if(action.function == FUNCTION_VAL_CLIP) {
                                    if(vel < action.min)
                                        vel = action.min;
                                    if(vel > action.max)
                                        vel = action.max;
                                } else if(action.function == FUNCTION_VAL_BUTTON) {
                                    if(evt == 0x90)
                                        vel = action.max;
                                    if(evt == 0x80)
                                        vel = action.min;
                                } else if(action.function == FUNCTION_VAL_SWITCH) {
                                    int sw = -1;

                                    if(evt == 0x90 || (evt == 0xB0 && (vel >= 64))) {

                                        MidiInput::keys_seq[seq_thd]^= KEY_SEQ_SALL_UP;
                                        if(MidiInput::keys_seq[seq_thd] & KEY_SEQ_SALL_UP)
                                            sw = 1;
                                        else
                                            sw = 0;

                                        if(sw == 1)
                                            vel = action.max;
                                        else if(sw == 0)
                                            vel = action.min;
                                    }


                                    if(sw == -1)
                                        return RET_NEWEFFECTS_SKIP; // ignore

                                }

                                if(MidiPlayer::fileSequencer[seq_thd])  {
                                    text+= " ALL SCALE TIME UP - BPM: " + QString::number(vel * 5);
                                    OSD = text;
                                    MidiPlayer::fileSequencer[seq_thd]->setScaleTime(vel * 5, 3);
                                } else {
                                    text+= " ALL VOLUME SCALE UP - IS OFF";
                                    OSD = text;
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == SEQUENCER_VOLUME_1_UP) {

                                int seq_thd = dev1;

                                if(MidiOutput::sequencer_enabled[seq_thd] < 0) {

                                    delayed_return1 = true;
                                    continue;
                                }

                                if(action.function == FUNCTION_VAL_CLIP) {
                                    if(vel < action.min)
                                        vel = action.min;
                                    if(vel > action.max)
                                        vel = action.max;
                                } else if(action.function == FUNCTION_VAL_BUTTON) {
                                    if(evt == 0x90)
                                        vel = action.max;
                                    if(evt == 0x80)
                                        vel = action.min;
                                } else if(action.function == FUNCTION_VAL_SWITCH) {
                                    int sw = -1;

                                    if(evt == 0x90 || (evt == 0xB0 && (vel >= 64))) {

                                        MidiInput::keys_seq[seq_thd]^= KEY_SEQ_V1_UP;
                                        if(MidiInput::keys_seq[seq_thd] & KEY_SEQ_V1_UP)
                                            sw = 1;
                                        else
                                            sw = 0;

                                        if(sw == 1)
                                            vel = action.max;
                                        else if(sw == 0)
                                            vel = action.min;
                                    }

                                    if(sw == -1)
                                        return RET_NEWEFFECTS_SKIP; // ignore

                                }

                                if(MidiPlayer::fileSequencer[seq_thd]) {
                                    text+= " 1 VOLUME SCALE UP - VOL: " + QString::number(vel);
                                    OSD = text;
                                    MidiPlayer::fileSequencer[seq_thd]->setVolume(vel, 0);
                                } else {
                                    text+= " 1 VOLUME SCALE UP - IS OFF";
                                    OSD = text;
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == SEQUENCER_VOLUME_2_DOWN) {

                                int seq_thd = dev1;

                                if(MidiOutput::sequencer_enabled[seq_thd] < 0) {

                                    delayed_return1 = true;
                                    continue;
                                }

                                if(action.function == FUNCTION_VAL_CLIP) {
                                    if(vel < action.min)
                                        vel = action.min;
                                    if(vel > action.max)
                                        vel = action.max;
                                } else if(action.function == FUNCTION_VAL_BUTTON) {
                                    if(evt == 0x90)
                                        vel = action.max;
                                    if(evt == 0x80)
                                        vel = action.min;
                                } else if(action.function == FUNCTION_VAL_SWITCH) {
                                    int sw = -1;

                                    if(evt == 0x90 || (evt == 0xB0 && (vel >= 64))) {

                                        MidiInput::keys_seq[seq_thd]^= KEY_SEQ_V2_UP;
                                        if(MidiInput::keys_seq[seq_thd] & KEY_SEQ_V2_UP)
                                            sw = 1;
                                        else
                                            sw = 0;

                                        if(sw == 1)
                                            vel = action.max;
                                        else if(sw == 0)
                                            vel = action.min;
                                    }


                                    if(sw == -1)
                                        return RET_NEWEFFECTS_SKIP; // ignore

                                }

                                if(MidiPlayer::fileSequencer[seq_thd]) {
                                    text+= " 2 VOLUME SCALE UP - VOL: " + QString::number(vel);
                                    OSD = text;
                                    MidiPlayer::fileSequencer[seq_thd]->setVolume(vel, 1);
                                } else {
                                    text+= " 2 VOLUME SCALE UP - IS OFF";
                                    OSD = text;
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == SEQUENCER_VOLUME_3_DOWN) {

                                int seq_thd = dev1;

                                if(MidiOutput::sequencer_enabled[seq_thd] < 0) {

                                    delayed_return1 = true;
                                    continue;
                                }

                                if(action.function == FUNCTION_VAL_CLIP) {
                                    if(vel < action.min)
                                        vel = action.min;
                                    if(vel > action.max)
                                        vel = action.max;
                                } else if(action.function == FUNCTION_VAL_BUTTON) {
                                    if(evt == 0x90)
                                        vel = action.max;
                                    if(evt == 0x80)
                                        vel = action.min;
                                } else if(action.function == FUNCTION_VAL_SWITCH) {
                                    int sw = -1;

                                    if(evt == 0x90 || (evt == 0xB0 && (vel >= 64))) {

                                        MidiInput::keys_seq[seq_thd]^= KEY_SEQ_V3_UP;
                                        if(MidiInput::keys_seq[seq_thd] & KEY_SEQ_V3_UP)
                                            sw = 1;
                                        else
                                            sw = 0;

                                        if(sw == 1)
                                            vel = action.max;
                                        else if(sw == 0)
                                            vel = action.min;
                                    }


                                    if(sw == -1)
                                        return RET_NEWEFFECTS_SKIP; // ignore

                                }

                                if(MidiPlayer::fileSequencer[seq_thd]) {
                                    text+= " 3 VOLUME SCALE UP - VOL: " + QString::number(vel);
                                    OSD = text;
                                    MidiPlayer::fileSequencer[seq_thd]->setVolume(vel, 2);
                                } else {
                                    text+= " 3 VOLUME SCALE UP - IS OFF";
                                    OSD = text;
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == SEQUENCER_VOLUME_4_UP) {

                                int seq_thd = dev1;

                                if(MidiOutput::sequencer_enabled[seq_thd] < 0) {

                                    delayed_return1 = true;
                                    continue;
                                }

                                if(action.function == FUNCTION_VAL_CLIP) {
                                    if(vel < action.min)
                                        vel = action.min;
                                    if(vel > action.max)
                                        vel = action.max;
                                } else if(action.function == FUNCTION_VAL_BUTTON) {
                                    if(evt == 0x90)
                                        vel = action.max;
                                    if(evt == 0x80)
                                        vel = action.min;
                                } else if(action.function == FUNCTION_VAL_SWITCH) {
                                    int sw = -1;

                                    if(evt == 0x90 || (evt == 0xB0 && (vel >= 64))) {

                                        MidiInput::keys_seq[seq_thd]^= KEY_SEQ_V4_UP;
                                        if(MidiInput::keys_seq[seq_thd] & KEY_SEQ_V4_UP)
                                            sw = 1;
                                        else
                                            sw = 0;

                                        if(sw == 1)
                                            vel = action.max;
                                        else if(sw == 0)
                                            vel = action.min;
                                    }


                                    if(sw == -1)
                                        return RET_NEWEFFECTS_SKIP; // ignore

                                }

                                if(MidiPlayer::fileSequencer[seq_thd]) {
                                    text+= " 4 VOLUME SCALE UP - VOL: " + QString::number(vel);
                                    OSD = text;
                                    MidiPlayer::fileSequencer[seq_thd]->setVolume(vel, 3);
                                } else {
                                    text+= " 4 VOLUME SCALE UP - IS OFF";
                                    OSD = text;
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == SEQUENCER_VOLUME_ALL_UP) {

                                int seq_thd = dev1;

                                if(MidiOutput::sequencer_enabled[seq_thd] < 0) {

                                    delayed_return1 = true;
                                    continue;
                                }

                                if(action.function == FUNCTION_VAL_CLIP) {
                                    if(vel < action.min)
                                        vel = action.min;
                                    if(vel > action.max)
                                        vel = action.max;
                                } else if(action.function == FUNCTION_VAL_BUTTON) {
                                    if(evt == 0x90)
                                        vel = action.max;
                                    if(evt == 0x80)
                                        vel = action.min;
                                } else if(action.function == FUNCTION_VAL_SWITCH) {
                                    int sw = -1;

                                    if(evt == 0x90 || (evt == 0xB0 && (vel >= 64))) {

                                        MidiInput::keys_seq[seq_thd]^= KEY_SEQ_VALL_UP;
                                        if(MidiInput::keys_seq[seq_thd] & KEY_SEQ_VALL_UP)
                                            sw = 1;
                                        else
                                            sw = 0;

                                        if(sw == 1)
                                            vel = action.max;
                                        else if(sw == 0)
                                            vel = action.min;
                                    }


                                    if(sw == -1)
                                        return RET_NEWEFFECTS_SKIP; // ignore

                                }

                                if(MidiPlayer::fileSequencer[seq_thd]) {
                                    text+= " ALL VOLUME SCALE UP - VOL: " + QString::number(vel);
                                    OSD = text;
                                    MidiPlayer::fileSequencer[seq_thd]->setVolume(vel, -1);
                                } else {
                                    text+= " ALL VOLUME SCALE UP - IS OFF";
                                    OSD = text;
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                        }

                        return RET_NEWEFFECTS_SKIP;

                    } else if(action.category == CATEGORY_FINGERPATTERN) { // finger pattern

                        int dev1 = action.device;

                        if(action.bypass != -1 && action.bypass != idev) {

                            dev1 = action.bypass;

                        }

                        if(action.action & 32) {
                            if(is_kb2)
                                continue;

                            if(action.action == FINGERPATTERN_PICK_DOWN) {

                                if(evt == 0x90 || evt == 0x80 || (action.event == EVENT_CONTROL && evt == 0xb0  && (vel >= 64))) {

                                    int seq_thd = dev1 + 1 * (!(dev1 & 1));

                                    seqOn(seq_thd, -1, false);

                                    FingerPatternDialog::Finger_Action(dev1, 32, action.function == FUNCTION_VAL_SWITCH, (evt == 0x80) ? 0 : 127);

                                    if(is_aftertouch && (evt != 0x80)) {
                                        // start with previous note
                                        std::vector<unsigned char> message2;
                                        message2 = MidiInput::message[idev];
                                        if(message2.size() == 3)
                                            MidiInControl::finger_func(dev, &message2, (idev & 1) != 0);
                                    }

                                    return RET_NEWEFFECTS_SKIP;
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == FINGERPATTERN_1_DOWN) {

                                if(evt == 0x90 || evt == 0x80 || (action.event == EVENT_CONTROL && evt == 0xb0 && (vel >= 64))) {

                                    int seq_thd = dev1 + 1 * (!(dev1 & 1));

                                    seqOn(seq_thd, -1, false);

                                    FingerPatternDialog::Finger_Action(dev1, 33, action.function == FUNCTION_VAL_SWITCH, (evt == 0x80) ? 0 : 127);

                                    if(is_aftertouch && (evt != 0x80)) {
                                        // start with previous note
                                        std::vector<unsigned char> message2;
                                        message2 = MidiInput::message[idev];
                                        if(message2.size() == 3)
                                            MidiInControl::finger_func(dev, &message2, (idev & 1) != 0);
                                    }

                                    return RET_NEWEFFECTS_SKIP;
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == FINGERPATTERN_2_DOWN) {

                                if(evt == 0x90 || evt == 0x80 || (action.event == EVENT_CONTROL && evt == 0xb0 && (vel >= 64))) {

                                    int seq_thd = dev1 + 1 * (!(dev1 & 1));

                                    seqOn(seq_thd, -1, false);

                                    FingerPatternDialog::Finger_Action(dev1, 34, action.function == FUNCTION_VAL_SWITCH, (evt == 0x80) ? 0 : 127);

                                    return RET_NEWEFFECTS_SKIP;
                                }

                                if(is_aftertouch && (evt != 0x80)) {
                                    // start with previous note
                                    std::vector<unsigned char> message2;
                                    message2 = MidiInput::message[idev];
                                    if(message2.size() == 3)
                                        MidiInControl::finger_func(dev, &message2, (idev & 1) != 0);
                                }

                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == FINGERPATTERN_3_DOWN) {

                                if(evt == 0x90 || evt == 0x80 || (action.event == EVENT_CONTROL && evt == 0xb0 && (vel >= 64))) {

                                    int seq_thd = dev1 + 1 * (!(dev1 & 1));

                                    seqOn(seq_thd, -1, false);

                                    FingerPatternDialog::Finger_Action(dev1, 35, action.function == FUNCTION_VAL_SWITCH, (evt == 0x80) ? 0 : 127);

                                    if(is_aftertouch && (evt != 0x80)) {
                                        // start with previous note
                                        std::vector<unsigned char> message2;
                                        message2 = MidiInput::message[idev];
                                        if(message2.size() == 3)
                                            MidiInControl::finger_func(dev, &message2, (idev & 1) != 0);
                                    }

                                    return RET_NEWEFFECTS_SKIP;
                                }


                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == FINGERPATTERN_SCALE_DOWN) {

                                int seq_thd = dev1;

                                if(action.function == FUNCTION_VAL_CLIP) {
                                    if(vel < action.min)
                                        vel = action.min;
                                    if(vel > action.max)
                                        vel = action.max;
                                } else if(action.function == FUNCTION_VAL_BUTTON) {
                                    if(evt == 0x90)
                                        vel = action.max;
                                    if(evt == 0x80)
                                        vel = action.min;
                                } else if(action.function == FUNCTION_VAL_SWITCH) {
                                    int sw = -1;

                                    if(evt == 0x90 || (evt == 0xB0 && (vel >= 64))) {

                                        MidiInput::keys_seq[seq_thd]^= KEY_FINGER_DOWN;
                                        if(MidiInput::keys_seq[seq_thd] & KEY_FINGER_DOWN)
                                            sw = 1;
                                        else
                                            sw = 0;

                                        if(sw == 1)
                                            vel = action.max;
                                        else if(sw == 0)
                                            vel = action.min;
                                    }

                                    if(sw == -1) {
                                        delayed_return1 = true;
                                        continue;  // ignore
                                    }

                                }

                                QString text = "Dev: " + QString::number(dev1) + " - FINGER PATTERN - TIME SCALE DOWN - ms: "
                                        + QString::number(1000 * ((vel + 1)/2)/64);
                                OSD = text;

                                FingerPatternDialog::Finger_Action_time(dev1, 32, vel);
                                return RET_NEWEFFECTS_SKIP;
                                delayed_return1 = true;
                                continue;

                            }

                        } else {
                            if(action.action == FINGERPATTERN_PICK_UP) {

                                if(evt == 0x90 || evt == 0x80 || (action.event == EVENT_CONTROL && evt == 0xb0 && (vel >= 64))) {

                                    int seq_thd = dev1;

                                    seqOn(seq_thd, -1, false);

                                    FingerPatternDialog::Finger_Action(dev1, 0, action.function == FUNCTION_VAL_SWITCH, (evt == 0x80) ? 0 : 127);

                                    if(is_aftertouch && (evt != 0x80)) {
                                        // start with previous note
                                        std::vector<unsigned char> message2;
                                        message2 = MidiInput::message[idev];
                                        if(message2.size() == 3)
                                            MidiInControl::finger_func(dev, &message2, (idev & 1) != 0);
                                    }

                                    return RET_NEWEFFECTS_SKIP;
                                }


                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == FINGERPATTERN_1_UP) {

                                if(evt == 0x90 || evt == 0x80 || (action.event == EVENT_CONTROL && evt == 0xb0 && (vel >= 64))) {

                                    int seq_thd = dev1;

                                    seqOn(seq_thd, -1, false);

                                    FingerPatternDialog::Finger_Action(dev1, 1, action.function == FUNCTION_VAL_SWITCH, (evt == 0x80) ? 0 : 127);

                                    if(is_aftertouch && (evt != 0x80)) {
                                        // start with previous note
                                        std::vector<unsigned char> message2;
                                        message2 = MidiInput::message[idev];
                                        if(message2.size() == 3)
                                            MidiInControl::finger_func(dev, &message2, (idev & 1) != 0);
                                    }
                                    return RET_NEWEFFECTS_SKIP;
                                }


                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == FINGERPATTERN_2_UP) {

                                if(evt == 0x90 || evt == 0x80 || (action.event == EVENT_CONTROL && evt == 0xb0 && (vel >= 64))) {

                                    int seq_thd = dev1;

                                    seqOn(seq_thd, -1, false);

                                    FingerPatternDialog::Finger_Action(dev1, 2, action.function == FUNCTION_VAL_SWITCH, (evt == 0x80) ? 0 : 127);

                                    if(is_aftertouch && (evt != 0x80)) {
                                        // start with previous note
                                        std::vector<unsigned char> message2;
                                        message2 = MidiInput::message[idev];
                                        if(message2.size() == 3)
                                            MidiInControl::finger_func(dev, &message2, (idev & 1) != 0);
                                    }

                                    return RET_NEWEFFECTS_SKIP;
                                }


                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == FINGERPATTERN_3_UP) {

                                if(evt == 0x90 || evt == 0x80 || (action.event == EVENT_CONTROL && evt == 0xb0 && (vel >= 64))) {

                                    int seq_thd = dev1;

                                    seqOn(seq_thd, -1, false);

                                    FingerPatternDialog::Finger_Action(dev1, 3, action.function == FUNCTION_VAL_SWITCH, (evt == 0x80) ? 0 : 127);

                                    if(is_aftertouch && (evt != 0x80)) {
                                        // start with previous note
                                        std::vector<unsigned char> message2;
                                        message2 = MidiInput::message[idev];
                                        if(message2.size() == 3)
                                            MidiInControl::finger_func(dev, &message2, (idev & 1) != 0);
                                    }

                                    return RET_NEWEFFECTS_SKIP;
                                }


                                return RET_NEWEFFECTS_SKIP;
                            }

                            if(action.action == FINGERPATTERN_SCALE_UP) {

                                int seq_thd = dev1;

                                if(action.function == FUNCTION_VAL_CLIP) {
                                    if(vel < action.min)
                                        vel = action.min;
                                    if(vel > action.max)
                                        vel = action.max;
                                } else if(action.function == FUNCTION_VAL_BUTTON) {
                                    if(evt == 0x90)
                                        vel = action.max;
                                    if(evt == 0x80)
                                        vel = action.min;
                                } else if(action.function == FUNCTION_VAL_SWITCH) {
                                    int sw = -1;

                                    if(evt == 0x90 || (evt == 0xB0 && (vel >= 64))) {

                                        MidiInput::keys_seq[seq_thd]^= KEY_FINGER_UP;
                                        if(MidiInput::keys_seq[seq_thd] & KEY_FINGER_UP)
                                            sw = 1;
                                        else
                                            sw = 0;

                                        if(sw == 1)
                                            vel = action.max;
                                        else if(sw == 0)
                                            vel = action.min;
                                    }

                                    if(sw == -1) {
                                        delayed_return1 = true;
                                        continue;  // ignore
                                    }

                                }

                                QString text = "Dev: " + QString::number(dev1) + " - FINGER PATTERN - TIME SCALE UP - ms: "
                                        + QString::number(1000 * ((vel + 1)/2)/64);
                                OSD = text;

                                FingerPatternDialog::Finger_Action_time(dev1, 0, vel);
                                delayed_return1 = true;
                                continue;

                            }

                        }

                        return RET_NEWEFFECTS_SKIP;
                    } else if(action.category == CATEGORY_SWITCH) { // switch

                        int idev1 = idev;

                        if(action.bypass != -1 && action.bypass != idev) {

                            idev1 = action.bypass;

                        }

                        QString text = "Dev: " + QString::number(idev1) + " - PEDAL SWITCH " +
                                QString::number(action.action & 31);


                        if(action.function == FUNCTION_VAL_SWITCH || action.event == EVENT_CONTROL) {
                            if(evt == 0x90 || (action.event == EVENT_CONTROL && evt == 0xb0 && (vel >= 64))) {

                                MidiInput::keys_switch[idev1] &= (1 << (action.action & 31));
                                MidiInput::keys_switch[idev1] ^= (1 << (action.action & 31));

                                if(MidiInput::keys_switch[idev1] & (1 << (action.action & 31)))
                                    text+= " - ON";
                                else
                                    text+= " - OFF";

                                OSD = text;
                            }

                        } else {

                            if(evt == 0x90) {

                                MidiInput::keys_switch[idev1] = (1 << (action.action & 31));

                            } else {

                                MidiInput::keys_switch[idev1] = 0;
                            }

                            if(MidiInput::keys_switch[idev1] & (1 << (action.action & 31)))
                                text+= " - ON";
                            else
                                text+= " - OFF";

                            OSD = text;

                        }


                        return RET_NEWEFFECTS_SKIP;

                    } else if(action.category == CATEGORY_GENERAL) { // general

                        int idev1 = idev;

                        QString text = "Dev: " + QString::number(idev1) + " - GENERAL ";

                        if(evt == 0x80 || (action.event == EVENT_CONTROL && evt == 0xb0 && (vel >= 64))) {

                            switch(action.action) {

                                case 0: {
                                    text+= "- Play / Stop";

                                    OSD = text;
                                    emit ((MainWindow *) MidiInControl::_main)->remPlayStop();

                                    break;
                                }

                                case 1: {
                                    text+= "- Record / Stop";

                                    OSD = text;
                                    emit ((MainWindow *) MidiInControl::_main)->remRecordStop(-1);

                                    break;
                                }

                                case 2: {
                                    text+= "- Record from (2 secs) / Stop";

                                    OSD = text;
                                    MidiInput::setTime(2000);
                                    emit ((MainWindow *) MidiInControl::_main)->remRecordStop(2000);

                                    break;
                                }

                                case 3: {
                                    text+= "- Stop";

                                    OSD = text;
                                    emit ((MainWindow *) MidiInControl::_main)->remStop();

                                    break;
                                }

                                case 4: {
                                    text+= "- Forward";

                                    OSD = text;
                                    emit ((MainWindow *) MidiInControl::_main)->remForward();

                                    break;
                                }

                                case 5: {
                                    text+= "- Back";

                                    OSD = text;
                                    emit ((MainWindow *) MidiInControl::_main)->remBack();

                                    break;
                                }

                            }

                        }

                        return RET_NEWEFFECTS_SKIP;

                    } else if(action.category == CATEGORY_NONE) { // none
                        return RET_NEWEFFECTS_SKIP;
                    }

                    return RET_NEWEFFECTS_NONE;
                }
            }
        } // end note event

    }

    // default aftertouch
    if(evt == 0xA0) {

        if(skip_aftertouch)
            return RET_NEWEFFECTS_SKIP;

        if(message->size() != 3)
            return RET_NEWEFFECTS_SKIP; // truncated

        QString text = "Dev: " + QString::number(idev) + " - AFTERTOUCH (default) -";

        if(idev & 1) {
            if(ch == MidiInControl::inchannelDown(dev)) {
                // Pitch Bend
                message->at(0) = 0xE0 | ch_down;
                message->at(1) = 0;
                text+= " PITCHBEND DOWN - v: " + QString::number(message->at(2));
                message->at(2) = 64 + (message->at(2) >> 1);
                OSD = text;

                return RET_NEWEFFECTS_SET;
            }
        } else {
            if(ch == MidiInControl::inchannelUp(dev)) {
                // Pitch Bend
                message->at(0) = 0xE0 | ch_up;
                message->at(1) = 0;
                text+= " PITCHBEND UP - v: " + QString::number(message->at(2));
                message->at(2) = 64 + (message->at(2) >> 1);
                OSD = text;

                return RET_NEWEFFECTS_SET;
            }
        }

        return RET_NEWEFFECTS_SKIP;
    }

    if(delayed_return1)
        return RET_NEWEFFECTS_SKIP;

    // default sustain pedal
    if(evt == 0xB0 && message->at(1) == 64) {

        if(skip_sustain)
            return RET_NEWEFFECTS_SKIP;

        if(message->size() != 3)
            return RET_NEWEFFECTS_SKIP; // truncated

        QString text = "Dev: " + QString::number(idev) + " -";

        if(idev & 1) {
            if(ch == MidiInControl::inchannelDown(dev)) {
                message->at(0) = 0xB0 | ch_down;

                if(message->at(2) >= 64)
                    text+= " SUSTAIN PEDAL (default) DOWN - ON";
                else
                    text+= " SUSTAIN PEDAL (default) DOWN - OFF";

                OSD = text;
                return RET_NEWEFFECTS_SET;
            }
        } else {
            if(ch == MidiInControl::inchannelUp(dev)) {
                message->at(0) = 0xB0 | ch_up;
                if(message->at(2) >= 64)
                    text+= " SUSTAIN PEDAL (default) UP - ON";
                else
                    text+= " SUSTAIN PEDAL (default) UP - OFF";

                OSD = text;
                return RET_NEWEFFECTS_SET;
            }
        }

        return RET_NEWEFFECTS_SKIP;
    }

    // default expression pedal
    if(evt == 0xB0 && message->at(1) == 11) {

        if(skip_expression)
            return RET_NEWEFFECTS_SKIP;

        if(message->size() != 3)
            return RET_NEWEFFECTS_SKIP; // truncated

        QString text = "Dev: " + QString::number(idev) + " - EXPRESSION PEDAL (default) -";

        if(idev & 1) {
            if(ch == MidiInControl::inchannelDown(dev)) {
                // Pitch Bend
                message->at(0) = 0xE0 | ch_down;
                message->at(1) = 0;
                text+= " PITCHBEND DOWN - v: " + QString::number(message->at(2));
                message->at(2) = 64 + (message->at(2) >> 1);
                OSD = text;

                return RET_NEWEFFECTS_SET;
            }
        } else {
            if(ch == MidiInControl::inchannelUp(dev)) {
                // Pitch Bend
                message->at(0) = 0xE0 | ch_up;
                message->at(1) = 0;
                text+= " PITCHBEND UP - v: " + QString::number(message->at(2));
                message->at(2) = 64 + (message->at(2) >> 1);
                OSD = text;

                return RET_NEWEFFECTS_SET;
            }
        }

        return RET_NEWEFFECTS_SKIP;
    }

    return RET_NEWEFFECTS_NONE;
}

void MidiInControl::loadActionSettings() {

    for(int index = 0; index < MAX_INPUT_DEVICES; index++) {
        action_effects[index].clear();
        for(int number = 0; number < MAX_LIST_ACTION; number++) {

            InputActionData inActiondata;
            inActiondata.status = 1;
            inActiondata.device = -1; // none
            inActiondata.channel = 17; // user
            inActiondata.event = 0; // note
            inActiondata.control_note = -1;
            inActiondata.category = CATEGORY_MIDI_EVENTS;
            inActiondata.action = 0;
            inActiondata.action2 = 0;
            inActiondata.function = 0;
            inActiondata.min = 0;
            inActiondata.max = 127;
            inActiondata.lev0 = 14;
            inActiondata.lev1 = 15;
            inActiondata.lev2 = 16;
            inActiondata.lev3 = 15;
            inActiondata.bypass = -1;

            QByteArray d = _settings->value(ActionGP + "/inActiondata" + QString::number(index) + " " + QString::number(number), "").toByteArray();
            if(d.length() == 16) {
                inActiondata.status =       d[0];
                inActiondata.device =       d[1];
                inActiondata.channel =      d[2];
                inActiondata.event =        d[3];
                inActiondata.control_note = d[4];
                inActiondata.category =     d[5];
                inActiondata.action =       d[6];
                inActiondata.action2 =      d[7];
                inActiondata.function =     d[8];
                inActiondata.min =          d[9];
                inActiondata.max =          d[10];
                inActiondata.lev0 =         d[11];
                inActiondata.lev1 =         d[12];
                inActiondata.lev2 =         d[13];
                inActiondata.lev3 =         d[14];
                inActiondata.bypass =      d[15];
                action_effects[index].append(inActiondata);
            }
        }
    }

}

void MidiInControl::saveActionSettings() {

    for (int index = 0; index < MAX_INPUT_DEVICES; index++) {
        for (int number = 0; number < action_effects[index].count(); number++) {
            InputActionData inActiondata = action_effects[index].at(number);
            QByteArray d;
            d.append((const char *) &inActiondata, 16);
            _settings->setValue(ActionGP + "/inActiondata" + QString::number(index) + " " + QString::number(number), d);
        }
    }
}

////////////////////////////////////////////////////////////////////////
// InputActionListWidget
////////////////////////////////////////////////////////////////////////

void MidiInControl::tab_Actions(QWidget *w)
{

    QFont font;
    QFont font1;
    QFont font2;
    font.setPointSize(16);
    font1.setPointSize(8);
    font2.setPointSize(12);

    ActionGP = _settings->value("MIDIin/ActionGroup", "Default").toString();
    MidiInControl::loadActionSettings();

    //w->setStyleSheet(QString::fromUtf8("color: white;\n"));

    InputListAction = new InputActionListWidget(w, this);
    InputListAction->setObjectName(QString::fromUtf8("InputActionListWidget"));
    InputListAction->resize(970 - 180, 562);

    QGroupBox *groupBox = new QGroupBox(w);
    groupBox->setObjectName(QString::fromUtf8("groupBox"));
    groupBox->setGeometry(QRect(9, 562 + 10 , 970 - 198 /*931 - 164*/, 111 - 30));
    groupBox->setTitle("");

    int xx = 0;

    QPushButton *ButtonAdd = new QPushButton(groupBox);
    ButtonAdd->setObjectName(QString::fromUtf8("ButtonAdd"));
    ButtonAdd->setGeometry(QRect(9, 9, 110, 30));
    ButtonAdd->setText("Add Action Before");
    ButtonAdd->setCheckable(false);
    TOOLTIP(ButtonAdd, "Add an action before the selected one to the list");

    xx+= 120;

    QPushButton *ButtonAdd2 = new QPushButton(groupBox);
    ButtonAdd2->setObjectName(QString::fromUtf8("ButtonAdd2"));
    ButtonAdd2->setGeometry(QRect(9 + xx, 9, 110, 30));
    ButtonAdd2->setText("Add Action After");
    ButtonAdd2->setCheckable(false);
    TOOLTIP(ButtonAdd2, "Add an action after the selected one to the list");

    xx+= 120;

    QPushButton *ButtonDel = new QPushButton(groupBox);
    ButtonDel->setObjectName(QString::fromUtf8("ButtonDel"));
    ButtonDel->setGeometry(QRect(9 + xx, 9, 110 , 30));
    ButtonDel->setText("Delete Action");
    ButtonDel->setCheckable(false);
    TOOLTIP(ButtonDel, "Deletes the selected action from the list");

    xx+= 120;

    QPushButton *ButtonMov = new QPushButton(groupBox);
    ButtonMov->setObjectName(QString::fromUtf8("ButtonMov"));
    ButtonMov->setGeometry(QRect(9 + xx, 9, 110, 30));
    ButtonMov->setText("Move Action Before");
    ButtonMov->setCheckable(false);
    TOOLTIP(ButtonMov, "Moves the selected action one position up in the list");

    xx+= 120;

    QPushButton *ButtonMov2 = new QPushButton(groupBox);
    ButtonMov2->setObjectName(QString::fromUtf8("ButtonMov2"));
    ButtonMov2->setGeometry(QRect(9 + xx, 9, 110, 30));
    ButtonMov2->setText("Move Action After");
    ButtonMov2->setCheckable(false);
    TOOLTIP(ButtonMov2, "Moves the selected action one position down in the list");

    xx+= 120;

    QLabel *label_ActionGroup = new QLabel(groupBox);
    label_ActionGroup->setObjectName(QString::fromUtf8("label_ActionGroup"));
    label_ActionGroup->setGeometry(QRect(QRect(9, 40, 110, 12)));
    label_ActionGroup->setAlignment(Qt::AlignLeft);
    label_ActionGroup->setFont(font1);
    label_ActionGroup->setStyleSheet("color: white;");
    label_ActionGroup->setText("Action Group:");

    ActionGroup = new QComboBox(groupBox);
    ActionGroup->setObjectName(QString::fromUtf8("ActionGroup"));
    ActionGroup->setGeometry(QRect(9, 44 + 10, 110, 20));
    TOOLTIP(ActionGroup, "Choose the group list of actions.");

    ActionGroup->addItem("Default", 0);

    for(int n = 0; n < 4; n++) {

        ActionGroup->addItem("List " + QString::number(n + 1), n);
    }

    for(int n = 0; n < ActionGroup->count(); n++) {

        if(ActionGroup->itemText(n) == _settings->value("MIDIin/ActionGroup", "Default").toString()) {
            ActionGP = _settings->value("MIDIin/ActionGroup", "Default").toString();
            ActionGroup->setCurrentIndex(n);
            break;
        }
    }

    QLabel *label_ActionTitle = new QLabel(groupBox);
    label_ActionTitle->setObjectName(QString::fromUtf8("label_ActionTitle"));
    label_ActionTitle->setGeometry(QRect(QRect(9 + 120, 40, 110, 12)));
    label_ActionTitle->setAlignment(Qt::AlignLeft);
    label_ActionTitle->setFont(font1);
    label_ActionTitle->setStyleSheet("color: white;");
    label_ActionTitle->setText("Title/Description:");

    ActionTitle = new QLineEdit(groupBox);
    ActionTitle->setObjectName(QString::fromUtf8("ActionTitle"));
    ActionTitle->setGeometry(QRect(9 + 120, 44 + 10, 240 - 9, 20));
    ActionTitle->setMaxLength(64);
    ActionTitle->setText(_settings->value(ActionGP + "/ActionTitle").toString());
    TOOLTIP(ActionTitle, "Title or description of the selected action group.");

    connect(ActionGroup, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int v)
    {
        ActionGP = ActionGroup->itemText(v);
        _settings->setValue("MIDIin/ActionGroup", ActionGP);
        ActionTitle->setText(_settings->value(ActionGP + "/ActionTitle").toString());
        MidiInControl::loadActionSettings();

        InputListAction->updateList();

        this->update();

    });

    connect(ActionTitle, &QLineEdit::textEdited, this, [=](const QString &text) {
        _settings->setValue(ActionGP + "/ActionTitle", text);
    });

    connect(ButtonAdd, &QPushButton::clicked, this, [=](bool)
    {

        for(int n = 0; n < InputListAction->count(); n++) {
            if(n == InputListAction->selected) {

                int index = cur_pairdev;

                int number = 0;
                for (; number < MAX_LIST_ACTION; number++) {
                    QByteArray d = _settings->value(ActionGP + "/inActiondata" + QString::number(index) + " " + QString::number(number), "").toByteArray();
                    if(d.length() != 16) {
                        break;
                    }
                }

                if(number < MAX_LIST_ACTION) { // add one

                    // displace
                    for(int m = number; m > ((n > 0) ? (n - 1) : n); m--) {
                        _settings->setValue(ActionGP + "/inActiondata" + QString::number(index) + " " + QString::number(m),
                                            _settings->value(ActionGP + "/inActiondata" + QString::number(index) + " " + QString::number(m - 1), ""));
                    }

                    InputActionData inActiondata;
                    inActiondata.status = 1;
                    inActiondata.device = -1; // none
                    inActiondata.channel = 17; // user
                    inActiondata.event = 0; // note
                    inActiondata.control_note = -1;
                    inActiondata.category = CATEGORY_MIDI_EVENTS;
                    inActiondata.action = 0;
                    inActiondata.action2 = 0;
                    inActiondata.function = 0;
                    inActiondata.min = 0;
                    inActiondata.max = 127;
                    inActiondata.lev0 = 14;
                    inActiondata.lev1 = 15;
                    inActiondata.lev2 = 16;
                    inActiondata.lev3 = 15;
                    inActiondata.bypass = -1;

                    QByteArray d;
                    d.append((const char *) &inActiondata, 16);

                    _settings->setValue(ActionGP + "/inActiondata" + QString::number(index) + " " + QString::number(n), d);

                    // load items

                    MidiInControl::loadActionSettings();
                    InputListAction->updateList();
                    this->update();

                    if(n >= InputListAction->count())
                        n = InputListAction->count() - 1;

                    InputListAction->selected = n;
                    InputListAction->verticalScrollBar()->setValue(n);
                    emit InputListAction->itemClicked(InputListAction->item(n));

                }

                break;
            }

        }

    });

    connect(ButtonAdd2, &QPushButton::clicked, this, [=](bool)
    {

        for(int n = 0; n < InputListAction->count(); n++) {
            if(n == InputListAction->selected) {

                int index = cur_pairdev;

                int number = 0;
                for (; number < MAX_LIST_ACTION; number++) {
                    QByteArray d = _settings->value(ActionGP + "/inActiondata" + QString::number(index) + " " + QString::number(number), "").toByteArray();
                    if(d.length() != 16) {
                        break;
                    }
                }

                if(number < MAX_LIST_ACTION) { // add one

                    // displace
                    for(int m = number; m > n; m--) {
                        _settings->setValue(ActionGP + "/inActiondata" + QString::number(index) + " " + QString::number(m),
                                            _settings->value(ActionGP + "/inActiondata" + QString::number(index) + " " + QString::number(m - 1), ""));
                    }

                    InputActionData inActiondata;
                    inActiondata.status = 1;
                    inActiondata.device = -1; // none
                    inActiondata.channel = 17; // user
                    inActiondata.event = 0; // note
                    inActiondata.control_note = -1;
                    inActiondata.category = CATEGORY_MIDI_EVENTS;
                    inActiondata.action = 0;
                    inActiondata.action2 = 0;
                    inActiondata.function = 0;
                    inActiondata.min = 0;
                    inActiondata.max = 127;
                    inActiondata.lev0 = 14;
                    inActiondata.lev1 = 15;
                    inActiondata.lev2 = 16;
                    inActiondata.lev3 = 15;
                    inActiondata.bypass = -1;

                    QByteArray d;
                    d.append((const char *) &inActiondata, 16);
                    _settings->setValue(ActionGP + "/inActiondata" + QString::number(index) + " " + QString::number(n + 1), d);

                    // load items

                    MidiInControl::loadActionSettings();
                    InputListAction->updateList();
                    this->update();

                    if(n >= InputListAction->count())
                        n = InputListAction->count() - 1;

                    InputListAction->selected = n;
                    InputListAction->verticalScrollBar()->setValue(n);
                    emit InputListAction->itemClicked(InputListAction->item(n));

                }

                break;
            }

        }

    });

    connect(ButtonDel, &QPushButton::clicked, this, [=](bool)
    {

        for(int n = 0; n < InputListAction->count(); n++) {
            if(n == InputListAction->selected) {

                int index = cur_pairdev;

                int number = 0;
                for (; number < MAX_LIST_ACTION; number++) {
                    QByteArray d = _settings->value(ActionGP + "/inActiondata" + QString::number(index) + " " + QString::number(number), "").toByteArray();
                    if(d.length() != 16) {
                        break;
                    }
                }

                // delete 1

                if(number <= 4) {
                    InputActionData inActiondata;
                    inActiondata.status = 1;
                    inActiondata.device = -1; // none
                    inActiondata.channel = 17; // user
                    inActiondata.event = 0; // note
                    inActiondata.control_note = -1;
                    inActiondata.category = CATEGORY_MIDI_EVENTS;
                    inActiondata.action = 0;
                    inActiondata.action2 = 0;
                    inActiondata.function = 0;
                    inActiondata.min = 0;
                    inActiondata.max = 127;
                    inActiondata.lev0 = 14;
                    inActiondata.lev1 = 15;
                    inActiondata.lev2 = 16;
                    inActiondata.lev3 = 15;
                    inActiondata.bypass = -1;

                    QByteArray d;
                    d.append((const char *) &inActiondata, 16);
                    _settings->setValue(ActionGP + "/inActiondata" + QString::number(index) + " " + QString::number(n), d);

                } else {

                    number--;
                    // displace
                    int m = n;
                    for(; m < number; m++) {
                        _settings->setValue(ActionGP + "/inActiondata" + QString::number(index) + " " + QString::number(m),
                                            _settings->value(ActionGP + "/inActiondata" + QString::number(index) + " " + QString::number(m + 1), ""));
                    }

                    if(m < MAX_LIST_ACTION)
                        _settings->setValue(ActionGP + "/inActiondata" + QString::number(index) + " " + QString::number(m), "");
                }

                // load items

                MidiInControl::loadActionSettings();
                InputListAction->updateList();
                this->update();

                if(n >= InputListAction->count())
                    n = InputListAction->count() - 1;

                InputListAction->selected = n;
                InputListAction->verticalScrollBar()->setValue(n);
                emit InputListAction->itemClicked(InputListAction->item(n));

                break;
            }

        }

    });

    connect(ButtonMov, &QPushButton::clicked, this, [=](bool)
    {

        for(int n = 0; n < InputListAction->count(); n++) {
            if(n == InputListAction->selected) {

                int index = cur_pairdev;

                int number = n - 1;

                QByteArray d;

                if(number >= 0) {
                    d = _settings->value(ActionGP + "/inActiondata" + QString::number(index) + " " + QString::number(number), "").toByteArray();
                    if(d.length() != 16) number = -1;
                }

                if(number >= 0) {

                    _settings->setValue(ActionGP + "/inActiondata" + QString::number(index) + " " + QString::number(number),
                                        _settings->value(ActionGP + "/inActiondata" + QString::number(index) + " " + QString::number(n), ""));
                    _settings->setValue(ActionGP + "/inActiondata" + QString::number(index) + " " + QString::number(n),
                                        d);
                    // load items

                    MidiInControl::loadActionSettings();
                    InputListAction->updateList();
                    this->update();

                    n = number;

                    if(n >= InputListAction->count())
                        n = InputListAction->count() - 1;

                    InputListAction->selected = n;
                    InputListAction->verticalScrollBar()->setValue(n);
                    emit InputListAction->itemClicked(InputListAction->item(n));

                }

                break;
            }

        }

    });

    connect(ButtonMov2, &QPushButton::clicked, this, [=](bool)
    {

        for(int n = 0; n < InputListAction->count(); n++) {
            if(n == InputListAction->selected) {

                int index = cur_pairdev;

                int number = n + 1;

                QByteArray d;

                if(number < InputListAction->count()) {
                    d = _settings->value(ActionGP + "/inActiondata" + QString::number(index) + " " + QString::number(number), "").toByteArray();
                    if(d.length() != 16) number = InputListAction->count();
                }

                if(number < InputListAction->count()) {

                    _settings->setValue(ActionGP + "/inActiondata" + QString::number(index) + " " + QString::number(number),
                                        _settings->value(ActionGP + "/inActiondata" + QString::number(index) + " " + QString::number(n), ""));
                    _settings->setValue(ActionGP + "/inActiondata" + QString::number(index) + " " + QString::number(n),
                                        d);
                    // load items

                    MidiInControl::loadActionSettings();
                    InputListAction->updateList();

                    n = number;

                    if(n >= InputListAction->count())
                        n = InputListAction->count() - 1;

                    InputListAction->selected = n;
                    InputListAction->verticalScrollBar()->setValue(n);
                    emit InputListAction->itemClicked(InputListAction->item(n));

                }

                break;
            }

        }

    });


}

////////////////////////////////////////////////////////////////////////
// Sequencer Tab
////////////////////////////////////////////////////////////////////////

void MidiInControl::tab_Sequencer(QWidget *w)
{

    QFont font;
    QFont font2;
    font.setPointSize(16);
    font2.setPointSize(12);

           //w->setStyleSheet(QString::fromUtf8("color: white;\n"));

    InputListSequencer = new InputSequencerListWidget(w, this);
    InputListSequencer->setObjectName(QString::fromUtf8("InputSequencerListWidget"));
    InputListSequencer->resize(970 - 180, 562);

}

void MidiInControl::sequencer_load(int dev) {


    QSettings *settings = new QSettings(QDir::homePath() + "/Midieditor/settings/sequencer.ini", QSettings::IniFormat);

    for(int index = dev * 8; index < (dev * 8 + 8); index++) {

        QString path;

        QString entry = SequencerGP + QString("/seq-") + QString::number(index/8) + "-" + QString::number((index & 4) ? 32 + (index & 3) : (index & 3))
                    + "-";

        path = settings->value(entry + "select", "").toString();

        if(path != "" && path != "Get It") {

             if(!MidiPlayer::is_sequencer_loaded(index)) {

                 qWarning("load %s %i", path.toLocal8Bit().constData(), index);

                 int vol = settings->value(entry + "DialVol", 127).toInt();

                 if(vol < 0)
                    vol = 0;
                 if(vol > 127)
                    vol = 127;

                 int beats = settings->value(entry + "DialBeats", 120).toInt();

                 if(beats < 1)
                     beats = 1;
                 if(beats > 508)
                     beats = 508;

                 unsigned int buttons = 0;

                 if(settings->value(entry + "ButtonLoop", false).toBool())
                     buttons |= SEQ_FLAG_LOOP;
                 if(settings->value(entry + "ButtonAuto", false).toBool())
                     buttons |= SEQ_FLAG_INFINITE;
                 if(settings->value(entry + "AutoRhythm", false).toBool())
                     buttons |= SEQ_FLAG_AUTORHYTHM;

                 MidiPlayer::unload_sequencer(index);

                 bool ok = true;
                 MidiFile* seq1  = new MidiFile(path, &ok);

                 if(seq1 && ok) {
                     if(MidiPlayer::play_sequencer(seq1, index, beats, vol) < 0) {
                        delete seq1;
                     }

                     if(seq1->is_multichannel_sequencer) {
                         buttons |= SEQ_FLAG_AUTORHYTHM;
                         settings->setValue(entry + "AutoRhythm", true);
                     }

                     if(MidiPlayer::fileSequencer[index/4])
                        MidiPlayer::fileSequencer[index/4]->setButtons(buttons, index & 3);
                 }

             }
        }
    }
}

void MidiInControl::sequencer_unload(int dev) {

    for(int index = dev * 8; index < (dev * 8 + 8); index++) {

        if(MidiPlayer::is_sequencer_loaded(index)) {
            MidiPlayer::unload_sequencer(index);

        }
    }
}

