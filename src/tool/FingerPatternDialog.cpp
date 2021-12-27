/*
 * MidiEditor
 * Copyright (C) 2010  Markus Schwenk
 *
 * FingerPatternDialog
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

#include "FingerPatternDialog.h"
#include "../midi/MidiInput.h"
#include "../midi/MidiOutput.h"
#include "../midi/MidiInControl.h"
#include <QMessageBox>
#include <QTextEdit>
#include <QMutex>
#include <QTime>
#include <QDir>
#include <QFileDialog>

int finger_token = 0;
bool _note_finger_disabled = false;

static int finger_switch_playUP = 0;
static int finger_switch_playDOWN = 0;

static int _note_finger_update_UP = 0;
static int _note_finger_update_DOWN = 0;
static int _note_finger_old_UP = -1;
static int _note_finger_old_DOWN = -1;

static int _note_finger_UP = -1;
static bool _note_finger_pressed_UP = false;
static int _note_finger_on_UP = 0;
static int _note_finger_pattern_UP = 0;
static int _note_finger_pick_UP= -1;
static bool _note_finger_pick_on_UP = false;
static int _note_finger_UP2 = -1;
static bool _note_finger_pressed_UP2 = false;
static int _note_finger_on_UP2 = 0;
static int _note_finger_pattern_UP2 = 0;

static int _note_finger_DOWN = -1;
static bool _note_finger_pressed_DOWN = false;
static int _note_finger_on_DOWN = 0;
static int _note_finger_pattern_DOWN = 0;
static int _note_finger_pick_DOWN= -1;
static int _note_finger_pick_on_DOWN = 0;
static int _note_finger_DOWN2 = -1;
static bool _note_finger_pressed_DOWN2 = false;
static int _note_finger_on_DOWN2 = 0;
static int _note_finger_pattern_DOWN2 = 0;


#define MAX_FINGER_BASIC 5
#define MAX_FINGER_PATTERN (MAX_FINGER_BASIC + 16)
#define MAX_FINGER_NOTES 15

static QStringList stringTypes = {"Power Chord", "C Major Chord  Progression (CM)", "C Minor Chord  Progression (Cm)",
                                 "Major Pentatonic", "Minor Pentatonic"
                                 };

static const char notes[12][3]= {"C ", "C#", "D ", "D#", "E ", "F ", "F#", "G", "G#", "A ", "A#", "B"};

//  type, steps, tempo, repeat, index note 1, index note2...
static unsigned char finger_patternBASIC[MAX_FINGER_BASIC][MAX_FINGER_NOTES + 4] = {
    {0, 2, 16, 0, 2, 3, 4, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 3, 16, 0, 4, 5, 6, 8, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {2, 3, 16, 0, 4, 5, 6, 8, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {3, 5, 16, 0, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {4, 5, 16, 0, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

//  type, steps, tempo, repeat, index note 1, index note2...
static unsigned char finger_patternUP[MAX_FINGER_PATTERN][MAX_FINGER_NOTES + 4] = {
    {0, 2, 16, 0, 2, 3, 4, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 3, 16, 0, 4, 5, 6, 8, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {2, 3, 16, 0, 4, 5, 6, 8, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {3, 5, 16, 0, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {4, 5, 16, 0, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

    {1, 3, 18, 0, 4, 2, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

//  type, steps, tempo, repeat, index note 1, index note2...
static unsigned char finger_patternDOWN[MAX_FINGER_PATTERN][MAX_FINGER_NOTES + 4] = {
    {0, 2, 16, 0, 2, 3, 4, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 3, 16, 0, 4, 5, 6, 8, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {2, 3, 16, 0, 4, 5, 6, 8, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {3, 5, 16, 0, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {4, 5, 16, 0, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

    {0, 5, 10, 1, 8, 12, 2, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 6, 16, 0, 4, 5, 6, 8, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

static QMutex *fingerMUX = NULL;

static int max_tick_counterUP = 6;
static int max_tick_counterDOWN = 6;

static int finger_channelUP = 0;
static int finger_channelDOWN = 0;

static int finger_key = 48;
static int finger_base_note = 48;

static int finger_repeatUP = 0;
static int finger_chord_modeUP = 1; // current pattern
static int finger_repeatDOWN = 0;
static int finger_chord_modeDOWN = 1; // current pattern

static QByteArray Packfinger() {

    QByteArray a;

    a.append('F');
    a.append('G');
    a.append('R');
    a.append((char) 1); // version 1

    a.append((char) MAX_FINGER_BASIC);
    a.append((char) MAX_FINGER_PATTERN);
    a.append((char) (MAX_FINGER_NOTES + 4));

    for(int n = 0; n < MAX_FINGER_PATTERN; n++) {
        if(n < MAX_FINGER_BASIC)
            a.append((char) n);
        else
            a.append((char) (16 + n - MAX_FINGER_BASIC));

        for(int m = 0; m < (MAX_FINGER_NOTES + 4); m++) {

            a.append((unsigned char) finger_patternUP[n][m]);
            a.append((unsigned char) finger_patternDOWN[n][m]);
        }

    }

    a.append((char) 255); // EOF

    return a;
}

static int Unpackfinger(QByteArray a) {

    int i = 7;

    if(a.length() < (7 +MAX_FINGER_NOTES + 4 + 1)) return -1;

    if(a[0] != 'F' && a[1] != 'G' && a[2] != 'R' && a[3] != (char) 1) return -1;

    if(a[4] != (char) MAX_FINGER_BASIC) return -2;
    if(a[5] != (char) MAX_FINGER_PATTERN) return -2;
    if(a[6] != (char) (MAX_FINGER_NOTES + 4)) return -2;

    while(i < a.length()) {
        int index1;

        if(a[i] == (char) 255) break; // EOF

        if((unsigned) a[i] < 16) {
            index1 = a[i];
            if(index1 >= MAX_FINGER_BASIC) // error!
                return -100;
        } else {
            index1 = MAX_FINGER_BASIC + a[i] - 16;
            if(index1 >= MAX_FINGER_PATTERN) // error!
                return -101;
        }

        i++;

        if(i >= a.length()) return -103;


        for(int m = 0; m < (MAX_FINGER_NOTES + 4); m++) {

            if(index1 < MAX_FINGER_BASIC && m == 1) { // fixed values for basic
                switch(index1) {
                    case 0:
                        a[i] = 2;
                        break;
                    case 1:
                        a[i] = 3;
                        break;
                    case 2:
                        a[i] = 3;
                        break;
                    case 3:
                        a[i] = 5;
                        break;
                    case 4:
                        a[i] = 5;
                        break;
                }
            }

            if(index1 < MAX_FINGER_BASIC && m == 3) // fixed values for basic
                a[i] = 0;

            finger_patternUP[index1][m] = a[i];
            i++;
            if(i >= a.length()) return -104;

            if(index1 < MAX_FINGER_BASIC && m == 1) { // fixed values for basic
                switch(index1) {
                    case 0:
                        a[i] = 2;
                        break;
                    case 1:
                        a[i] = 3;
                        break;
                    case 2:
                        a[i] = 3;
                        break;
                    case 3:
                        a[i] = 5;
                        break;
                    case 4:
                        a[i] = 5;
                        break;
                }
            }

            if(index1 < MAX_FINGER_BASIC && m == 3) // fixed values for basic
                a[i] = 0;

            finger_patternDOWN[index1][m] = a[i];
            i++;
            if(i >= a.length()) return -104;
        }

    }

    return 0;
}

void FingerPatternDialog::save() {

    QString dir = QDir::homePath();

    if(_settings) {

        dir = QString::fromUtf8(_settings->value("FINGER_save_path", dir.toUtf8()).toByteArray());
    }

    QString newPath = QFileDialog::getSaveFileName(this, "Save Finger Pattern file",
        dir, "Finger Pattern files (*.finger)");

    if (newPath == "") {
        return;
    }

    QByteArray a = Packfinger();

    QFile *file = new QFile(newPath);

    if(file) {

        if(file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {

            if(_settings) {
                _settings->setValue("FINGER_save_path", newPath.toUtf8());
            }

            file->write(a);

        }

        delete file;
    }

}

void FingerPatternDialog::load() {

    QString dir = QDir::homePath();

    if(_settings) {

        dir = QString::fromUtf8(_settings->value("FINGER_save_path", dir.toUtf8()).toByteArray());
    }

    QString newPath = QFileDialog::getOpenFileName(this, "Load Finger Pattern file",
        dir, "Finger Pattern files (*.finger)");

    if (newPath == "") {
        return;
    }

    QByteArray a;

    QFile *file = new QFile(newPath);

    if(file) {
        if(file->open(QIODevice::ReadOnly)) {

            if(_settings) {
                _settings->setValue("FINGER_save_path", newPath.toUtf8());
            }

            a = file->read(0x10000);

            Unpackfinger(a);

            comboBoxChordUP->blockSignals(true);

            comboBoxChordUP->clear();
            comboBoxChordUP->addItem(stringTypes[0], -1);
            comboBoxChordUP->addItem(stringTypes[1], -2);
            comboBoxChordUP->addItem(stringTypes[2], -3);
            comboBoxChordUP->addItem(stringTypes[3], -4);
            comboBoxChordUP->addItem(stringTypes[4], -5);

            for(int n = 0; n < 16; n++) {
                if(finger_patternUP[n + MAX_FINGER_BASIC][1]) {

                    comboBoxChordUP->addItem("CUSTOM #" + QString::number(n + 1)
                                             + " " + stringTypes[finger_patternUP[n + MAX_FINGER_BASIC][0]], n);
                }
            }

            comboBoxChordUP->blockSignals(false);

            comboBoxChordDOWN->blockSignals(true);

            comboBoxChordDOWN->clear();
            comboBoxChordDOWN->addItem(stringTypes[0], -1);
            comboBoxChordDOWN->addItem(stringTypes[1], -2);
            comboBoxChordDOWN->addItem(stringTypes[2], -3);
            comboBoxChordDOWN->addItem(stringTypes[3], -4);
            comboBoxChordDOWN->addItem(stringTypes[4], -5);

            for(int n = 0; n < 16; n++) {
                if(finger_patternDOWN[n + MAX_FINGER_BASIC][1]) {

                    comboBoxChordDOWN->addItem("CUSTOM #" + QString::number(n + 1)
                                             + " " + stringTypes[finger_patternDOWN[n + MAX_FINGER_BASIC][0]], n);
                }
            }

            comboBoxChordDOWN->blockSignals(false);

            finger_chord_modeUP = 0;
            finger_chord_modeDOWN = 0;

            comboBoxChordUP->currentIndexChanged(finger_chord_modeUP);
            comboBoxChordDOWN->currentIndexChanged(finger_chord_modeDOWN);
            comboBoxChordUP->setCurrentIndex(finger_chord_modeUP);
            comboBoxChordDOWN->setCurrentIndex(finger_chord_modeDOWN);

        }

        delete file;
    }

}

void FingerPatternDialog::reject() {

    if(_settings) {
        _settings->setValue("FINGER_pattern_save", Packfinger());
    }

    hide();
}

FingerPatternDialog::FingerPatternDialog(QWidget* parent, QSettings *settings) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint) {

    QDialog *fingerPatternDialog = this;
    _parent = parent;

    if(settings)
        _settings = settings;
    if(!_settings) _settings = new QSettings(QString("MidiEditor"), QString("FingerPattern"));

    if(_settings) {
        QByteArray a =_settings->value("FINGER_pattern_save").toByteArray();

        Unpackfinger(a);
    }

    if (fingerPatternDialog->objectName().isEmpty())
        fingerPatternDialog->setObjectName(QString::fromUtf8("fingerPatternDialog"));
    fingerPatternDialog->setFixedSize(802, 634);
    fingerPatternDialog->setWindowTitle("Finger Pattern Utility");
    fingerPatternDialog->setToolTip("Utility that modifies the keystroke\nusing selected note patterns and scales");

    if(_parent) {

        int index;

        QWidget *upper =new QWidget(fingerPatternDialog);
        upper->setFixedSize(802, 470);
        upper->setStyleSheet(QString::fromUtf8("color: white; background-color: #808080;"));

        buttonBox = new QDialogButtonBox(fingerPatternDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setGeometry(QRect(540, 590, 231, 32));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Close /*| QDialogButtonBox::Ok*/);

        QFont font0;
        font0.setPointSize(10);

        //
        groupBoxPatternNoteUP = new QGroupBox(upper);
        groupBoxPatternNoteUP->setObjectName(QString::fromUtf8("groupBoxPatternNoteUP"));
        groupBoxPatternNoteUP->setGeometry(QRect(20, 10, 761, 221));
        groupBoxPatternNoteUP->setFlat(false);
        groupBoxPatternNoteUP->setCheckable(false);
        groupBoxPatternNoteUP->setTitle(" Pattern Note UP Channel");
        groupBoxPatternNoteUP->setAlignment(Qt::AlignCenter);
        groupBoxPatternNoteUP->setFont(font0);
        groupBoxPatternNoteUP->setFlat(true);
        groupBoxPatternNoteUP->setStyleSheet(QString::fromUtf8(
        "QGroupBox {color: white; background-color: #606060;} \n"
        "QGroupBox QLabel {color: white; background-color: #606060;} \n"
        "QGroupBox QSlider {color: white; background-color: #606060;} \n"
        "QGroupBox QSlider::sub-page:Horizontal {background: #c0F0c0; height: 4px; margin-top: 7px; margin-bottom: 7px;}\n"
        "QGroupBox QSlider::handle:Horizontal {background: #c02020; border: 3px solid #303030; width: 4px; height: 10px; padding: 0; margin: -10px 0; border-radius: 3px;} \n"
        "QGroupBox QSlider::handle:Horizontal:hover {background: #f04040; border: 3px solid #303030; width: 4px; height: 10px; padding: 0; margin: -10px 0; border-radius: 3px;} \n"
        "QGroupBox QComboBox {color: white; background-color: #808080;} \n"
        "QGroupBox QComboBox:disabled {color: darkGray; background-color: #808080;} \n"
        "QGroupBox QComboBox QAbstractItemView {color: black; background-color: white; selection-background-color: #24c2c3;} \n"
        "QGroupBox QSpinBox {color: white; background-color: #808080;} \n"
        "QGroupBox QSpinBox:disabled {color: darkGray; background-color: #808080;} \n"
        "QGroupBox QPushButton {color: white; background-color: #808080;} \n"
        "QGroupBox QToolTip {color: black;} \n"
        ));
        groupBoxPatternNoteUP->setToolTip("Group panel where you can select the base chord,\n"
                                        " the number of repetitions, the scale note pattern\n"
                                        " and number of notes to use, and the time (in ms) between notes");


        pushButtonDeleteUP = new QPushButton(groupBoxPatternNoteUP);
        pushButtonDeleteUP->setObjectName(QString::fromUtf8("pushButtonDeleteUP"));
        pushButtonDeleteUP->setGeometry(QRect(54, 20, 45, 23));
        pushButtonDeleteUP->setText("DEL");
        pushButtonDeleteUP->setToolTip("Delete this custom note pattern\n"
                                       "or restore basic chords values");

        connect(pushButtonDeleteUP, QOverload<bool>::of(&QPushButton::clicked), [=](bool)
        {

            int r;

            if(finger_chord_modeUP < MAX_FINGER_BASIC)
                r = QMessageBox::question(_parent, "Restore DEFAULT pattern", "Are you sure?                         ");
            else
                r = QMessageBox::question(_parent, "Delete CUSTOM pattern", "Are you sure?                         ");

            if(r != QMessageBox::Yes) return;


            if(finger_chord_modeUP < MAX_FINGER_BASIC) {
                memcpy(&finger_patternUP[finger_chord_modeUP][0], &finger_patternBASIC[finger_chord_modeUP][0], 19);
                comboBoxChordUP->currentIndexChanged(finger_chord_modeUP);
                return;
            }

            comboBoxChordUP->blockSignals(true);

            memset(&finger_patternUP[finger_chord_modeUP][0], 0, 19);
            finger_patternUP[finger_chord_modeUP][2] = 16;

            finger_chord_modeUP = 0;

            if(finger_chord_modeUP < MAX_FINGER_BASIC) { // fixed values for basic
                int val = 0;
                switch(finger_chord_modeUP) {
                    case 0:
                        val = 2;
                        break;
                    case 1:
                        val = 3;
                        break;
                    case 2:
                        val = 3;
                        break;
                    case 3:
                        val = 5;
                        break;
                    case 4:
                        val = 5;
                        break;
                }

                finger_patternUP[finger_chord_modeUP][1] = val;
                finger_patternUP[finger_chord_modeUP][3] = 0;
            }

            comboBoxChordUP->clear();
            comboBoxChordUP->addItem(stringTypes[0], -1);
            comboBoxChordUP->addItem(stringTypes[1], -2);
            comboBoxChordUP->addItem(stringTypes[2], -3);
            comboBoxChordUP->addItem(stringTypes[3], -4);
            comboBoxChordUP->addItem(stringTypes[4], -5);

            for(int n = 0; n < 16; n++) {
                if(finger_patternUP[n + MAX_FINGER_BASIC][1]) {

                    comboBoxChordUP->addItem("CUSTOM #" + QString::number(n + 1)
                                             + " " + stringTypes[finger_patternUP[n + MAX_FINGER_BASIC][0]], n);
                }
            }

            comboBoxChordUP->blockSignals(false);

            comboBoxChordUP->currentIndexChanged(0);
            comboBoxChordUP->setCurrentIndex(0);

        });

        labelStepsHUP = new QLabel(groupBoxPatternNoteUP);
        labelStepsHUP->setObjectName(QString::fromUtf8("labelStepsHUP"));
        labelStepsHUP->setGeometry(QRect(10, 40, 43, 16));
        labelStepsHUP->setAlignment(Qt::AlignCenter);
        labelStepsHUP->setText("Steps");
        labelTempoHUP = new QLabel(groupBoxPatternNoteUP);
        labelTempoHUP->setObjectName(QString::fromUtf8("labelTempoHUP"));
        labelTempoHUP->setGeometry(QRect(10, 143, 87, 13));
        labelTempoHUP->setAlignment(Qt::AlignCenter);
        labelTempoHUP->setText("Tempo (ms)");

        comboBoxChordUP = new QComboBox(groupBoxPatternNoteUP);
        comboBoxChordUP->setObjectName(QString::fromUtf8("comboBoxChordUP"));
        comboBoxChordUP->setGeometry(QRect(148, 20, 252, 22));
        comboBoxChordUP->addItem(stringTypes[0], -1);
        comboBoxChordUP->addItem(stringTypes[1], -2);
        comboBoxChordUP->addItem(stringTypes[2], -3);
        comboBoxChordUP->addItem(stringTypes[3], -4);
        comboBoxChordUP->addItem(stringTypes[4], -5);
        comboBoxChordUP->setToolTip("Select the base chord here\nor a previously saved custom pattern to edit");

        index = finger_chord_modeUP;

        if(finger_chord_modeUP < MAX_FINGER_BASIC)
            comboBoxChordUP->setCurrentIndex(finger_chord_modeUP);


        for(int n = 0; n < 16; n++) {
            if(finger_patternUP[n + MAX_FINGER_BASIC][1]) {

                if(finger_chord_modeUP == MAX_FINGER_BASIC + n)
                    index = comboBoxChordUP->count();

                comboBoxChordUP->addItem("CUSTOM #" + QString::number(n + 1)
                                         + " " + stringTypes[finger_patternUP[n + MAX_FINGER_BASIC][0]], n);

            }
        }

        comboBoxChordUP->setCurrentIndex(index);

        max_tick_counterUP = finger_patternUP[finger_chord_modeUP][2];
        finger_repeatUP = finger_patternUP[finger_chord_modeUP][3];

        labelChordUP = new QLabel(groupBoxPatternNoteUP);
        labelChordUP->setObjectName(QString::fromUtf8("labelChordUP"));
        labelChordUP->setGeometry(QRect(110, 20, 34, 16));
        labelChordUP->setText("Chord:");

        comboBoxRepeatUP = new QComboBox(groupBoxPatternNoteUP);
        comboBoxRepeatUP->setObjectName(QString::fromUtf8("comboBoxRepeatUP"));
        comboBoxRepeatUP->setGeometry(QRect(460, 20, 69, 22));
        comboBoxRepeatUP->addItem("Infinite");
        comboBoxRepeatUP->setToolTip("Selects the number of repeats of the pattern\n"
                                        "before stopping on the last note");
        for(int n = 1; n <= 7; n++)
            comboBoxRepeatUP->addItem(QString::number(n) + " Time");

        comboBoxRepeatUP->setCurrentIndex(finger_repeatUP);

        labelRepeatUP = new QLabel(groupBoxPatternNoteUP);
        labelRepeatUP->setObjectName(QString::fromUtf8("labelRepeat"));
        labelRepeatUP->setGeometry(QRect(410, 20, 47, 13));
        labelRepeatUP->setAlignment(Qt::AlignCenter);
        labelRepeatUP->setText("Repeat:");

        comboBoxCustomUP = new QComboBox(groupBoxPatternNoteUP);
        comboBoxCustomUP->setObjectName(QString::fromUtf8("comboBoxCustomUP"));
        comboBoxCustomUP->setGeometry(QRect(540, 20, 131, 22));
        for(int n = 1; n <= 16; n++)
            comboBoxCustomUP->addItem("CUSTOM #" + QString::number(n));
        comboBoxCustomUP->setToolTip("Select a custom name pattern to save with STORE");

        pushButtonStoreUP = new QPushButton(groupBoxPatternNoteUP);
        pushButtonStoreUP->setObjectName(QString::fromUtf8("pushButtonStoreUP"));
        pushButtonStoreUP->setGeometry(QRect(680, 20, 75, 23));
        pushButtonStoreUP->setText("STORE");
        pushButtonStoreUP->setToolTip("Save a custom note pattern with the selected name.\n"
                                        "It can be selected later from 'Chord'");

        horizontalSliderNotesUP = new QSlider(groupBoxPatternNoteUP);
        horizontalSliderNotesUP->setObjectName(QString::fromUtf8("horizontalSliderNotesUP"));
        horizontalSliderNotesUP->setGeometry(QRect(20, 60, 711, 22));
        horizontalSliderNotesUP->setMinimum(1);
        horizontalSliderNotesUP->setMaximum(15);
        horizontalSliderNotesUP->setValue(finger_patternUP[finger_chord_modeUP][1]);
        horizontalSliderNotesUP->setOrientation(Qt::Horizontal);
        horizontalSliderNotesUP->setInvertedAppearance(false);
        horizontalSliderNotesUP->setInvertedControls(false);
        horizontalSliderNotesUP->setTickPosition(QSlider::TicksBelow);
        horizontalSliderNotesUP->setTickInterval(1);
        horizontalSliderNotesUP->setStyleSheet(QString::fromUtf8(
        "QSlider::handle:Horizontal {background: #40a0a0; border: 3px solid #303030; width: 4px; height: 10px; padding: 0; margin: -9px 0; border-radius: 3px;} \n"
        "QSlider::handle:Horizontal:hover {background: #40c0c0; border: 3px solid #303030; width: 4px; height: 10px; padding: 0; margin: -9px 0; border-radius: 3px;} \n"));
        horizontalSliderNotesUP->setToolTip("Number of notes to use in the pattern");

        for(int n = 0; n < 15; n++) {

            ComboBoxNoteUP[n] = new QComboBox(groupBoxPatternNoteUP);
            ComboBoxNoteUP[n]->setObjectName(QString::fromUtf8("ComboBoxNoteUP") + QString::number(n));
            ComboBoxNoteUP[n]->setGeometry(QRect(4 + 50 * n, 90, 49, 22));
            ComboBoxNoteUP[n]->setToolTip("Base note to use in the pattern for the chosen 'Chord'.\n"
                                            "'-' represents an Octave -\n"
                                            "'+' represents an Octave +\n"
                                            "'*' represents double tempo\n"
                                            "'#' represents silence\n"
                                            "'1' represents the base note\n"
                                            "'2 to 7' represents the next notes of the scale");
            setComboNotePattern(finger_patternUP[finger_chord_modeUP][0], n, ComboBoxNoteUP[n]);

            //ComboBoxNoteUP[n]->setCurrentIndex(0);
            for(int m = 0; m < ComboBoxNoteUP[n]->count(); m++)
                if(ComboBoxNoteUP[n]->itemData(m).toInt() == finger_patternUP[finger_chord_modeUP][4 + n]) {
                    ComboBoxNoteUP[n]->setCurrentIndex(m);
                    break;
                }



            labelNoteUP[n] = new QLabel(groupBoxPatternNoteUP);
            labelNoteUP[n]->setObjectName(QString::fromUtf8("labelNoteUP") + QString::number(n));
            labelNoteUP[n]->setGeometry(QRect(15 + 50 * n, 120, 21, 16));
            labelNoteUP[n]->setAlignment(Qt::AlignCenter);
            labelNoteUP[n]->setText(QString::number(n + 1));
        }

        horizontalSliderTempoUP = new QSlider(groupBoxPatternNoteUP);
        horizontalSliderTempoUP->setObjectName(QString::fromUtf8("horizontalSliderTempoUP"));
        horizontalSliderTempoUP->setGeometry(QRect(10, 160, 731, 22));
        horizontalSliderTempoUP->setMinimum(0);
        horizontalSliderTempoUP->setMaximum(64);
        horizontalSliderTempoUP->setValue(max_tick_counterUP);
        horizontalSliderTempoUP->setOrientation(Qt::Horizontal);
        horizontalSliderTempoUP->setTickPosition(QSlider::TicksBelow);
        horizontalSliderTempoUP->setTickInterval(2);
        horizontalSliderTempoUP->setToolTip("Selected time (in ms) to play a note");


        for(int n = 0; n <= 16; n++) {

            labelTempoUP[n] = new QLabel(groupBoxPatternNoteUP);
            labelTempoUP[n]->setObjectName(QString::fromUtf8("labelTempoUP") + QString::number(n));
            labelTempoUP[n]->setGeometry(QRect(5 + 720 * n / 16, 190, 21, 21));
            labelTempoUP[n]->setAlignment(Qt::AlignCenter);
            int number = 1000*(n * 4 + 1 *(n == 0)) / 64;
            labelTempoUP[n]->setText(QString::number(number));
        }


        connect(comboBoxChordUP, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int num)
        {

           num = comboBoxChordUP->itemData(num).toInt();

           if(num < 0)
               num = (-num) - 1;
           else
               num += MAX_FINGER_BASIC;

           finger_chord_modeUP = num;

            for(int n = 0; n < 15; n++) {

                if(num < MAX_FINGER_BASIC) {

                    finger_patternUP[num][0] = num;

                    if(num == 0) { // power
                        finger_patternUP[num][1] = 2;
                        finger_patternUP[num][4] = 2;
                        finger_patternUP[num][5] = 3;

                    } else if(num == 1 || num == 2) { // major, minor prog
                        finger_patternUP[num][1] = 3;
                        finger_patternUP[num][4] = 4;
                        finger_patternUP[num][5] = 5;
                        finger_patternUP[num][6] = 6;
                    }  else if(num == 3 || num == 4) { // major, minor pentatonic
                        finger_patternUP[num][1] = 5;
                        finger_patternUP[num][4] = 5;
                        finger_patternUP[num][5] = 6;
                        finger_patternUP[num][6] = 7;
                        finger_patternUP[num][7] = 8;
                        finger_patternUP[num][8] = 9;
                    }
                }

                int v = finger_patternUP[num][4 + n];

                max_tick_counterUP = finger_patternUP[num][2];
                finger_repeatUP = finger_patternUP[num][3];
                horizontalSliderTempoUP->setValue(max_tick_counterUP);
                comboBoxRepeatUP->setCurrentIndex(finger_repeatUP);

                setComboNotePattern(finger_patternUP[num][0], n, ComboBoxNoteUP[n]);


                for(int m = 0; m < ComboBoxNoteUP[n]->count(); m++)
                    if(ComboBoxNoteUP[n]->itemData(m).toInt() == v) {
                        ComboBoxNoteUP[n]->setCurrentIndex(m);
                        break;
                    }

            }

            horizontalSliderNotesUP->setValue(finger_patternUP[finger_chord_modeUP][1]);

        });

        connect(comboBoxRepeatUP, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int num)
        {
            finger_repeatUP = num;
            finger_patternUP[finger_chord_modeUP][3] = num;
        });

        for(int n = 0; n < 15; n++) {

            connect(ComboBoxNoteUP[n], QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int num)
            {
                int num2 = ComboBoxNoteUP[n]->itemData(num).toInt();
                finger_patternUP[finger_chord_modeUP][4 + n] = num2;
            });
        }

        connect(horizontalSliderNotesUP, QOverload<int>::of(&QSlider::valueChanged), [=](int num)
        {
            finger_patternUP[finger_chord_modeUP][1] = num;

        });

        connect(horizontalSliderTempoUP, QOverload<int>::of(&QSlider::valueChanged), [=](int num)
        {
            max_tick_counterUP = num;
            finger_patternUP[finger_chord_modeUP][2] = num;
        });


        connect(pushButtonStoreUP, QOverload<bool>::of(&QPushButton::clicked), [=](bool)
        {
            int custom = comboBoxCustomUP->currentIndex();
            int r = QMessageBox::question(_parent, "Store CUSTOM #" + QString::number(custom + 1), "Are you sure?                         ");
            if(r != QMessageBox::Yes) return;

            memcpy(&finger_patternUP[MAX_FINGER_BASIC + custom][0], &finger_patternUP[finger_chord_modeUP][0], 19);

            if(finger_chord_modeUP < MAX_FINGER_BASIC) { // fixed values for basic
                int val = 0;
                switch(finger_chord_modeUP) {
                    case 0:
                        val = 2;
                        break;
                    case 1:
                        val = 3;
                        break;
                    case 2:
                        val = 3;
                        break;
                    case 3:
                        val = 5;
                        break;
                    case 4:
                        val = 5;
                        break;
                }

                finger_patternUP[finger_chord_modeUP][1] = val;
                finger_patternUP[finger_chord_modeUP][3] = 0;
            }

            finger_chord_modeUP = MAX_FINGER_BASIC + custom;

            comboBoxChordUP->clear();
            comboBoxChordUP->addItem(stringTypes[0], -1);
            comboBoxChordUP->addItem(stringTypes[1], -2);
            comboBoxChordUP->addItem(stringTypes[2], -3);
            comboBoxChordUP->addItem(stringTypes[3], -4);
            comboBoxChordUP->addItem(stringTypes[4], -5);

            int indx = 0;
            for(int n = 0; n < 16; n++) {
                if(finger_patternUP[n + MAX_FINGER_BASIC][1]) {
                    if(n == custom) indx= comboBoxChordUP->count();
                    comboBoxChordUP->addItem("CUSTOM #" + QString::number(n + 1)
                                             + " " + stringTypes[finger_patternUP[n + MAX_FINGER_BASIC][0]], n);
                }
            }

            comboBoxChordUP->setCurrentIndex(indx);

        });

//////

        groupBoxPatternNoteDOWN = new QGroupBox(upper);
        groupBoxPatternNoteDOWN->setObjectName(QString::fromUtf8("groupBoxPatternNoteDOWN"));
        groupBoxPatternNoteDOWN->setGeometry(QRect(20, 240, 761, 221));
        groupBoxPatternNoteDOWN->setFlat(false);
        groupBoxPatternNoteDOWN->setCheckable(false);
        groupBoxPatternNoteDOWN->setTitle(" Pattern Note DOWN Channel");
        groupBoxPatternNoteDOWN->setAlignment(Qt::AlignCenter);
        groupBoxPatternNoteDOWN->setFont(font0);
        groupBoxPatternNoteDOWN->setFlat(true);
        groupBoxPatternNoteDOWN->setStyleSheet(QString::fromUtf8(
        "QGroupBox {color: white; background-color: #606060;} \n"
        "QGroupBox QLabel {color: white; background-color: #606060;} \n"
        "QGroupBox QSlider {color: white; background-color: #606060;} \n"
        "QGroupBox QSlider::sub-page:Horizontal {background: #c0F0c0; height: 4px; margin-top: 7px; margin-bottom: 7px;}\n"
        "QGroupBox QSlider::handle:Horizontal {background: #c02020; border: 3px solid #303030; width: 4px; height: 10px; padding: 0; margin: -10px 0; border-radius: 3px;} \n"
        "QGroupBox QSlider::handle:Horizontal:hover {background: #f04040; border: 3px solid #303030; width: 4px; height: 10px; padding: 0; margin: -10px 0; border-radius: 3px;} \n"
        "QGroupBox QComboBox {color: white; background-color: #808080;} \n"
        "QGroupBox QComboBox:disabled {color: darkGray; background-color: #808080;} \n"
        "QGroupBox QComboBox QAbstractItemView {color: black; background-color: white; selection-background-color: #24c2c3;} \n"
        "QGroupBox QSpinBox {color: white; background-color: #808080;} \n"
        "QGroupBox QSpinBox:disabled {color: darkGray; background-color: #808080;} \n"
        "QGroupBox QPushButton {color: white; background-color: #808080;} \n"
        "QGroupBox QToolTip {color: black;} \n"
        ));
        groupBoxPatternNoteDOWN->setToolTip("Group panel where you can select the base chord,\n"
                                        " the number of repetitions, the scale note pattern\n"
                                        " and number of notes to use, and the time (in ms) between notes");

        pushButtonDeleteDOWN = new QPushButton(groupBoxPatternNoteDOWN);
        pushButtonDeleteDOWN->setObjectName(QString::fromUtf8("pushButtonDeleteDOWN"));
        pushButtonDeleteDOWN->setGeometry(QRect(54, 20, 45, 23));
        pushButtonDeleteDOWN->setText("DEL");
        pushButtonDeleteDOWN->setToolTip("Delete this custom note pattern");

        connect(pushButtonDeleteDOWN, QOverload<bool>::of(&QPushButton::clicked), [=](bool)
        {
            int r;

            if(finger_chord_modeDOWN < MAX_FINGER_BASIC)
                r = QMessageBox::question(_parent, "Restore DEFAULT pattern", "Are you sure?                         ");
            else
                r = QMessageBox::question(_parent, "Delete CUSTOM pattern", "Are you sure?                         ");

            if(r != QMessageBox::Yes) return;


            if(finger_chord_modeDOWN < MAX_FINGER_BASIC) {
                memcpy(&finger_patternDOWN[finger_chord_modeDOWN][0], &finger_patternBASIC[finger_chord_modeDOWN][0], 19);
                comboBoxChordDOWN->currentIndexChanged(finger_chord_modeDOWN);
                return;
            }

            comboBoxChordDOWN->blockSignals(true);

            memset(&finger_patternDOWN[finger_chord_modeDOWN][0], 0, 19);
            finger_patternDOWN[finger_chord_modeDOWN][2] = 16;

            finger_chord_modeDOWN = 0;

            if(finger_chord_modeDOWN < MAX_FINGER_BASIC) { // fixed values for basic
                int val = 0;
                switch(finger_chord_modeDOWN) {
                    case 0:
                        val = 2;
                        break;
                    case 1:
                        val = 3;
                        break;
                    case 2:
                        val = 3;
                        break;
                    case 3:
                        val = 5;
                        break;
                    case 4:
                        val = 5;
                        break;
                }

                finger_patternDOWN[finger_chord_modeDOWN][1] = val;
                finger_patternDOWN[finger_chord_modeDOWN][3] = 0;
            }

            comboBoxChordDOWN->clear();
            comboBoxChordDOWN->addItem(stringTypes[0], -1);
            comboBoxChordDOWN->addItem(stringTypes[1], -2);
            comboBoxChordDOWN->addItem(stringTypes[2], -3);
            comboBoxChordDOWN->addItem(stringTypes[3], -4);
            comboBoxChordDOWN->addItem(stringTypes[4], -5);

            for(int n = 0; n < 16; n++) {
                if(finger_patternDOWN[n + MAX_FINGER_BASIC][1]) {

                    comboBoxChordDOWN->addItem("CUSTOM #" + QString::number(n + 1)
                                             + " " + stringTypes[finger_patternDOWN[n + MAX_FINGER_BASIC][0]], n);
                }
            }

            comboBoxChordDOWN->blockSignals(false);

            comboBoxChordDOWN->currentIndexChanged(0);
            comboBoxChordDOWN->setCurrentIndex(0);

        });

        labelStepsHDOWN = new QLabel(groupBoxPatternNoteDOWN);
        labelStepsHDOWN->setObjectName(QString::fromUtf8("labelStepsHDOWN"));
        labelStepsHDOWN->setGeometry(QRect(10, 40, 43, 16));
        labelStepsHDOWN->setAlignment(Qt::AlignCenter);
        labelStepsHDOWN->setText("Steps");
        labelTempoHDOWN = new QLabel(groupBoxPatternNoteDOWN);
        labelTempoHDOWN->setObjectName(QString::fromUtf8("labelTempoHDOWN"));
        labelTempoHDOWN->setGeometry(QRect(10, 143, 87, 13));
        labelTempoHDOWN->setAlignment(Qt::AlignCenter);
        labelTempoHDOWN->setText("Tempo (ms)");

        comboBoxChordDOWN = new QComboBox(groupBoxPatternNoteDOWN);
        comboBoxChordDOWN->setObjectName(QString::fromUtf8("comboBoxChordDOWN"));
        comboBoxChordDOWN->setGeometry(QRect(148, 20, 252, 22));
        comboBoxChordDOWN->addItem(stringTypes[0], -1);
        comboBoxChordDOWN->addItem(stringTypes[1], -2);
        comboBoxChordDOWN->addItem(stringTypes[2], -3);
        comboBoxChordDOWN->addItem(stringTypes[3], -4);
        comboBoxChordDOWN->addItem(stringTypes[4], -5);
        comboBoxChordDOWN->setToolTip("Select the base chord here\n"
                          "or a previously saved custom pattern to edit");


        index = finger_chord_modeDOWN;

        if(finger_chord_modeDOWN < MAX_FINGER_BASIC)
            comboBoxChordDOWN->setCurrentIndex(finger_chord_modeDOWN);


        for(int n = 0; n < 16; n++) {
            if(finger_patternDOWN[n + MAX_FINGER_BASIC][1]) {

                if(finger_chord_modeDOWN == MAX_FINGER_BASIC + n)
                    index = comboBoxChordDOWN->count();

                comboBoxChordDOWN->addItem("CUSTOM #" + QString::number(n + 1)
                                         + " " + stringTypes[finger_patternDOWN[n + MAX_FINGER_BASIC][0]], n);

            }
        }

        comboBoxChordDOWN->setCurrentIndex(index);


        max_tick_counterDOWN = finger_patternDOWN[finger_chord_modeDOWN][2];
        finger_repeatDOWN = finger_patternDOWN[finger_chord_modeDOWN][3];

        labelChordDOWN = new QLabel(groupBoxPatternNoteDOWN);
        labelChordDOWN->setObjectName(QString::fromUtf8("labelChordDOWN"));
        labelChordDOWN->setGeometry(QRect(110, 20, 34, 16));
        labelChordDOWN->setText("Chord:");

        comboBoxRepeatDOWN = new QComboBox(groupBoxPatternNoteDOWN);
        comboBoxRepeatDOWN->setObjectName(QString::fromUtf8("comboBoxRepeatDOWN"));
        comboBoxRepeatDOWN->setGeometry(QRect(460, 20, 69, 22));
        comboBoxRepeatDOWN->addItem("Infinite");
        comboBoxRepeatDOWN->setToolTip("Selects the number of repeats of the pattern\n"
                                        "before stopping on the last note");
        for(int n = 1; n <= 7; n++)
            comboBoxRepeatDOWN->addItem(QString::number(n) + " Time");

        comboBoxRepeatDOWN->setCurrentIndex(finger_repeatDOWN);

        labelRepeatDOWN = new QLabel(groupBoxPatternNoteDOWN);
        labelRepeatDOWN->setObjectName(QString::fromUtf8("labelRepeat"));
        labelRepeatDOWN->setGeometry(QRect(410, 20, 47, 13));
        labelRepeatDOWN->setAlignment(Qt::AlignCenter);
        labelRepeatDOWN->setText("Repeat:");

        comboBoxCustomDOWN = new QComboBox(groupBoxPatternNoteDOWN);
        comboBoxCustomDOWN->setObjectName(QString::fromUtf8("comboBoxCustomDOWN"));
        comboBoxCustomDOWN->setGeometry(QRect(540, 20, 131, 22));
        for(int n = 1; n <= 16; n++)
            comboBoxCustomDOWN->addItem("CUSTOM #" + QString::number(n));
        comboBoxCustomDOWN->setToolTip("Select a custom name pattern to save with STORE");

        pushButtonStoreDOWN = new QPushButton(groupBoxPatternNoteDOWN);
        pushButtonStoreDOWN->setObjectName(QString::fromUtf8("pushButtonStoreDOWN"));
        pushButtonStoreDOWN->setGeometry(QRect(680, 20, 75, 23));
        pushButtonStoreDOWN->setText("STORE");
        pushButtonStoreDOWN->setToolTip("Save a custom note pattern with the selected name.\n"
                                            "It can be selected later from 'Chord'");

        horizontalSliderNotesDOWN = new QSlider(groupBoxPatternNoteDOWN);
        horizontalSliderNotesDOWN->setObjectName(QString::fromUtf8("horizontalSliderNotesDOWN"));
        horizontalSliderNotesDOWN->setGeometry(QRect(20, 60, 711, 22));
        horizontalSliderNotesDOWN->setMinimum(1);
        horizontalSliderNotesDOWN->setMaximum(15);
        horizontalSliderNotesDOWN->setValue(finger_patternDOWN[finger_chord_modeDOWN][1]);
        horizontalSliderNotesDOWN->setOrientation(Qt::Horizontal);
        horizontalSliderNotesDOWN->setInvertedAppearance(false);
        horizontalSliderNotesDOWN->setInvertedControls(false);
        horizontalSliderNotesDOWN->setTickPosition(QSlider::TicksBelow);
        horizontalSliderNotesDOWN->setTickInterval(1);
        horizontalSliderNotesDOWN->setStyleSheet(QString::fromUtf8(
                "QSlider::handle:Horizontal {background: #40a0a0; border: 3px solid #303030; width: 4px; height: 10px; padding: 0; margin: -9px 0; border-radius: 3px;} \n"
                "QSlider::handle:Horizontal:hover {background: #40c0c0; border: 3px solid #303030; width: 4px; height: 10px; padding: 0; margin: -9px 0; border-radius: 3px;} \n"));

        horizontalSliderNotesDOWN->setToolTip("Number of notes to use in the pattern");

        for(int n = 0; n < 15; n++) {

            ComboBoxNoteDOWN[n] = new QComboBox(groupBoxPatternNoteDOWN);
            ComboBoxNoteDOWN[n]->setObjectName(QString::fromUtf8("ComboBoxNoteDOWN") + QString::number(n));
            ComboBoxNoteDOWN[n]->setGeometry(QRect(4 + 50 * n, 90, 49, 22));
            ComboBoxNoteDOWN[n]->setToolTip("Base note to use in the pattern for the chosen 'Chord'.\n"
                                            "'-' represents an Octave -\n"
                                            "'+' represents an Octave +\n"
                                            "'*' represents double tempo\n"
                                            "'#' represents silence\n"
                                            "'1' represents the base note\n"
                                            "'2 to 7' represents the next notes of the scale");

            setComboNotePattern(finger_patternDOWN[finger_chord_modeDOWN][0], n, ComboBoxNoteDOWN[n]);


            for(int m = 0; m < ComboBoxNoteDOWN[n]->count(); m++)
                if(ComboBoxNoteDOWN[n]->itemData(m).toInt() == finger_patternDOWN[finger_chord_modeDOWN][4 + n]) {
                    ComboBoxNoteDOWN[n]->setCurrentIndex(m);
                    break;
                }

            labelNoteDOWN[n] = new QLabel(groupBoxPatternNoteDOWN);
            labelNoteDOWN[n]->setObjectName(QString::fromUtf8("labelNoteDOWN") + QString::number(n));
            labelNoteDOWN[n]->setGeometry(QRect(15 + 50 * n, 120, 21, 16));
            labelNoteDOWN[n]->setAlignment(Qt::AlignCenter);
            labelNoteDOWN[n]->setText(QString::number(n + 1));
        }

        horizontalSliderTempoDOWN = new QSlider(groupBoxPatternNoteDOWN);
        horizontalSliderTempoDOWN->setObjectName(QString::fromUtf8("horizontalSliderTempoDOWN"));
        horizontalSliderTempoDOWN->setGeometry(QRect(10, 160, 731, 22));
        horizontalSliderTempoDOWN->setMinimum(0);
        horizontalSliderTempoDOWN->setMaximum(64);
        horizontalSliderTempoDOWN->setValue(max_tick_counterDOWN);
        horizontalSliderTempoDOWN->setOrientation(Qt::Horizontal);
        horizontalSliderTempoDOWN->setTickPosition(QSlider::TicksBelow);
        horizontalSliderTempoDOWN->setTickInterval(2);
        horizontalSliderTempoDOWN->setToolTip("Selected time (in ms) to play a note");

        for(int n = 0; n <= 16; n++) {
            labelTempoDOWN[n] = new QLabel(groupBoxPatternNoteDOWN);
            labelTempoDOWN[n]->setObjectName(QString::fromUtf8("labelTempoDOWN") + QString::number(n));
            labelTempoDOWN[n]->setGeometry(QRect(5 + 720 * n / 16, 190, 21, 21));
            labelTempoDOWN[n]->setAlignment(Qt::AlignCenter);
            int number = 1000*(n * 4 + 1 *(n == 0)) / 64;
            labelTempoDOWN[n]->setText(QString::number(number));
        }


        connect(comboBoxChordDOWN, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int num)
        {

           num = comboBoxChordDOWN->itemData(num).toInt();

           if(num < 0)
               num = (-num) - 1;
           else
               num += MAX_FINGER_BASIC;

           finger_chord_modeDOWN = num;

            for(int n = 0; n < 15; n++) {

                if(num < MAX_FINGER_BASIC && n == 0) {

                    finger_patternDOWN[num][0] = num;

                    if(num == 0) { // power
                        finger_patternDOWN[num][1] = 2;
                        finger_patternDOWN[num][4] = 2;
                        finger_patternDOWN[num][5] = 3;

                    } else if(num == 1 || num == 2) { // major, minor prog
                        finger_patternDOWN[num][1] = 3;
                        finger_patternDOWN[num][4] = 4;
                        finger_patternDOWN[num][5] = 5;
                        finger_patternDOWN[num][6] = 6;
                    }  else if(num == 3 || num == 4) { // major, minor pentatonic
                        finger_patternDOWN[num][1] = 5;
                        finger_patternDOWN[num][4] = 5;
                        finger_patternDOWN[num][5] = 6;
                        finger_patternDOWN[num][6] = 7;
                        finger_patternDOWN[num][7] = 8;
                        finger_patternDOWN[num][8] = 9;
                    }
                }

                int v = finger_patternDOWN[num][4 + n];

                max_tick_counterDOWN = finger_patternDOWN[num][2];
                finger_repeatDOWN = finger_patternDOWN[num][3];
                horizontalSliderTempoDOWN->setValue(max_tick_counterDOWN);
                comboBoxRepeatDOWN->setCurrentIndex(finger_repeatDOWN);

                setComboNotePattern(finger_patternDOWN[num][0], n, ComboBoxNoteDOWN[n]);

                for(int m = 0; m < ComboBoxNoteDOWN[n]->count(); m++)
                    if(ComboBoxNoteDOWN[n]->itemData(m).toInt() == v) {
                        ComboBoxNoteDOWN[n]->setCurrentIndex(m);
                        break;
                    }

            }

            horizontalSliderNotesDOWN->setValue(finger_patternDOWN[finger_chord_modeDOWN][1]);

        });

        connect(comboBoxRepeatDOWN, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int num)
        {
            finger_repeatDOWN = num;
            finger_patternDOWN[finger_chord_modeDOWN][3] = num;
        });

        for(int n = 0; n < 15; n++) {

            connect(ComboBoxNoteDOWN[n], QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int num)
            {
                int num2 = ComboBoxNoteDOWN[n]->itemData(num).toInt();
                //qDebug("changed #%i %i %i", n, num, num2);
                finger_patternDOWN[finger_chord_modeDOWN][4 + n] = num2;
            });
        }

        connect(horizontalSliderNotesDOWN, QOverload<int>::of(&QSlider::valueChanged), [=](int num)
        {
            finger_patternDOWN[finger_chord_modeDOWN][1] = num;

        });

        connect(horizontalSliderTempoDOWN, QOverload<int>::of(&QSlider::valueChanged), [=](int num)
        {
            max_tick_counterDOWN = num;
            finger_patternDOWN[finger_chord_modeDOWN][2] = num;
        });


        connect(pushButtonStoreDOWN, QOverload<bool>::of(&QPushButton::clicked), [=](bool)
        {
            int custom = comboBoxCustomDOWN->currentIndex();
            int r = QMessageBox::question(_parent, "Store CUSTOM #" + QString::number(custom + 1), "Are you sure?                         ");
            if(r != QMessageBox::Yes) return;

            memcpy(&finger_patternDOWN[MAX_FINGER_BASIC + custom][0], &finger_patternDOWN[finger_chord_modeDOWN][0], 19);

            if(finger_chord_modeDOWN < MAX_FINGER_BASIC) { // fixed values for basic
                int val = 0;
                switch(finger_chord_modeDOWN) {
                    case 0:
                        val = 2;
                        break;
                    case 1:
                        val = 3;
                        break;
                    case 2:
                        val = 3;
                        break;
                    case 3:
                        val = 5;
                        break;
                    case 4:
                        val = 5;
                        break;
                }

                finger_patternDOWN[finger_chord_modeDOWN][1] = val;
                finger_patternDOWN[finger_chord_modeDOWN][3] = 0;
            }

            finger_chord_modeDOWN = MAX_FINGER_BASIC + custom;

            comboBoxChordDOWN->clear();
            comboBoxChordDOWN->addItem(stringTypes[0], -1);
            comboBoxChordDOWN->addItem(stringTypes[1], -2);
            comboBoxChordDOWN->addItem(stringTypes[2], -3);
            comboBoxChordDOWN->addItem(stringTypes[3], -4);
            comboBoxChordUP->addItem(stringTypes[4], -5);

            int indx = 0;
            for(int n = 0; n < 16; n++) {
                if(finger_patternDOWN[n + MAX_FINGER_BASIC][1]) {
                    if(n == custom) indx= comboBoxChordDOWN->count();
                    comboBoxChordDOWN->addItem("CUSTOM #" + QString::number(n + 1)
                                             + " " + stringTypes[finger_patternDOWN[n + MAX_FINGER_BASIC][0]], n);
                }
            }

            comboBoxChordDOWN->setCurrentIndex(indx);

        });

 //******
        int yy = 10;

        labelBaseNote = new QLabel(fingerPatternDialog);
        labelBaseNote->setObjectName(QString::fromUtf8("labelBaseNote"));
        labelBaseNote->setGeometry(QRect(680, 520 + yy, 101, 20));
        labelBaseNote->setAlignment(Qt::AlignCenter);
        labelBaseNote->setText("Base Note");
        labelBaseNote->setToolTip("Base Note for Pattern Note Test");

        comboBoxBaseNote = new QComboBox(fingerPatternDialog);
        comboBoxBaseNote->setObjectName(QString::fromUtf8("comboBoxBaseNote"));
        comboBoxBaseNote->setGeometry(QRect(680, 550 + yy, 101, 22));
        comboBoxBaseNote->clear();
        comboBoxBaseNote->setToolTip("Base Note for Pattern Note Test");

        for(int n = 0; n < 128; n++) {
            comboBoxBaseNote->addItem(QString(&notes[n % 12][0]) + QString::number(n/12 -1));
        }

        comboBoxBaseNote->setCurrentIndex(finger_base_note);

        connect(comboBoxBaseNote, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int num)
        {
            finger_base_note = num;
        });

        pushButtonTestNoteUP = new QPushButton(fingerPatternDialog);
        pushButtonTestNoteUP->setObjectName(QString::fromUtf8("pushButtonTestNoteUP"));
        pushButtonTestNoteUP->setGeometry(QRect(570, 470 + yy, 101, 41));
        pushButtonTestNoteUP->setText("Test Pattern UP");
        pushButtonTestNoteUP->setToolTip("Test Pattern Note UP");

        pushButtonTestNoteDOWN = new QPushButton(fingerPatternDialog);
        pushButtonTestNoteDOWN->setObjectName(QString::fromUtf8("pushButtonTestNoteDOWN"));
        pushButtonTestNoteDOWN->setGeometry(QRect(680, 470 + yy, 101, 41));
        pushButtonTestNoteDOWN->setText("Test Pattern DOWN");
        pushButtonTestNoteDOWN->setToolTip("Test Pattern Note DOWN");

        pushButtonSavePattern = new QPushButton(fingerPatternDialog);
        pushButtonSavePattern->setObjectName(QString::fromUtf8("pushButtonSavePattern"));
        pushButtonSavePattern->setGeometry(QRect(570, 520 + yy, 101, 27));
        pushButtonSavePattern->setText("Save File");
        pushButtonSavePattern->setToolTip("Save Finger Pattern File");

        connect(pushButtonSavePattern, SIGNAL(clicked()), this, SLOT(save()));

        pushButtonLoadPattern = new QPushButton(fingerPatternDialog);
        pushButtonLoadPattern->setObjectName(QString::fromUtf8("pushButtonLoadPattern"));
        pushButtonLoadPattern->setGeometry(QRect(570, 550 + yy - 2, 101, 27));
        pushButtonLoadPattern->setText("Load File");
        pushButtonLoadPattern->setToolTip("Load Finger Pattern File");

        connect(pushButtonLoadPattern, SIGNAL(clicked()), this, SLOT(load()));


// GROUP PLAY **************************************************************+

        groupPlay = new QWidget(fingerPatternDialog);
        groupPlay->setObjectName(QString::fromUtf8("groupPlay"));
        groupPlay->setEnabled(!_note_finger_disabled);
        groupPlay->setGeometry(QRect(10, 460 + yy, 481, 151));

        labelUP = new QLabel(groupPlay);
        labelUP->setObjectName(QString::fromUtf8("labelUP"));
        labelUP->setGeometry(QRect(20, 0, 81, 20));
        labelUP->setAlignment(Qt::AlignCenter);
        labelUP->setText("Note UP");
        labelUP->setToolTip("Note used to enable Finger Pattern #1\n"
                            "to the channel UP");

        comboBoxTypeUP = new QComboBox(groupPlay);
        comboBoxTypeUP->setObjectName(QString::fromUtf8("comboBoxTypeUP"));
        comboBoxTypeUP->setGeometry(QRect(0, 20, 111, 22));
        comboBoxTypeUP->setToolTip("Selected Finger Pattern for UP");

        checkKeyPressedUP = new QCheckBox(groupPlay);
        checkKeyPressedUP->setObjectName(QString::fromUtf8("checkKeyPressedUP"));
        checkKeyPressedUP->setGeometry(QRect(10, 50, 91, 17));
        checkKeyPressedUP->setText("Key Pressed");
        checkKeyPressedUP->setToolTip("Selected the use of Finger Pattern key for UP\n"
                                      "checked: like a switch\n"
                                      "unchecked: like a push button or step to step");

        comboBoxNoteUP = new QComboBox(groupPlay);
        comboBoxNoteUP->setObjectName(QString::fromUtf8("comboBoxNoteUP"));
        comboBoxNoteUP->setGeometry(QRect(10, 70, 91, 31));
        QFont font;
        font.setPointSize(16);
        comboBoxNoteUP->setFont(font);

        labelPickUP = new QLabel(groupPlay);
        labelPickUP->setObjectName(QString::fromUtf8("labelPickUP"));
        labelPickUP->setGeometry(QRect(10, 100, 91, 20));
        labelPickUP->setAlignment(Qt::AlignCenter);
        labelPickUP->setText("Pick Note");
        labelPickUP->setToolTip("Note used to enable Finger Pattern Picking\n"
                                "to the channel UP");

        comboBoxNotePickUP = new QComboBox(groupPlay);
        comboBoxNotePickUP->setObjectName(QString::fromUtf8("comboBoxNotePickUP"));
        comboBoxNotePickUP->setGeometry(QRect(10, 120, 91, 31));
        comboBoxNotePickUP->setFont(font);

        labelUP2 = new QLabel(groupPlay);
        labelUP2->setObjectName(QString::fromUtf8("labelUP2"));
        labelUP2->setGeometry(QRect(260, 0, 81, 20));
        labelUP2->setAlignment(Qt::AlignCenter);
        labelUP2->setText("Note UP 2");
        labelUP2->setToolTip("Note used to enable Finger Pattern #2\n"
                             "to the channel UP");

        comboBoxTypeUP2 = new QComboBox(groupPlay);
        comboBoxTypeUP2->setObjectName(QString::fromUtf8("comboBoxTypeUP2"));
        comboBoxTypeUP2->setGeometry(QRect(240, 20, 111, 22));
        comboBoxTypeUP2->setToolTip("Selected Finger Pattern for UP 2");

        checkKeyPressedUP2 = new QCheckBox(groupPlay);
        checkKeyPressedUP2->setObjectName(QString::fromUtf8("checkKeyPressedUP2"));
        checkKeyPressedUP2->setGeometry(QRect(250, 50, 91, 17));
        checkKeyPressedUP2->setText("Key Pressed");
        checkKeyPressedUP2->setToolTip("Selected the use of Finger Pattern key for UP 2\n"
                                       "checked: like a switch\n"
                                       "unchecked: like a push button or step to step");

        comboBoxNoteUP2 = new QComboBox(groupPlay);
        comboBoxNoteUP2->setObjectName(QString::fromUtf8("comboBoxNoteUP2"));
        comboBoxNoteUP2->setGeometry(QRect(250, 70, 91, 31));
        comboBoxNoteUP2->setFont(font);

        // 250  580 + yy

        comboBoxTypeUP->clear();
        comboBoxTypeUP->addItem(stringTypes[0]);
        comboBoxTypeUP->addItem(stringTypes[1]);
        comboBoxTypeUP->addItem(stringTypes[2]);
        comboBoxTypeUP->addItem(stringTypes[3]);
        comboBoxTypeUP->addItem(stringTypes[4]);

        int indx = 0;

        if(_note_finger_pattern_UP < MAX_FINGER_BASIC) indx = _note_finger_pattern_UP;

        for(int n = 0; n < 16; n++) {
            if(finger_patternUP[n + MAX_FINGER_BASIC][1]) {
                if(n + MAX_FINGER_BASIC == _note_finger_pattern_UP) indx= _note_finger_pattern_UP;

            }

            comboBoxTypeUP->addItem("CUSTOM #" + QString::number(n + 1));
        }

        comboBoxTypeUP->setCurrentIndex(indx);

        connect(comboBoxTypeUP, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int num)
        {
            _note_finger_pattern_UP = num;
            _settings->setValue("FINGER_pattern_UP", _note_finger_pattern_UP);
        });

        comboBoxNoteUP->setToolTip("Note UP key\n"
                                   "if 'key pressed' is selected the key acts as a switch\n"
                                   "if 'key pressed' is not selected then if the recorded\n"
                                   "note is pressed, the pattern will be activated until\n"
                                   "this key is released. This allows the possibility of\n"
                                   "going step by step at the speed of the key is pressed\n"
                                   "and released.\n\n"
                                   "Record the activation note from \n"
                                   "the MIDI keyboard clicking\n"
                                   "'Get It' and pressing one");

        comboBoxNoteUP->addItem("Get It", -1);
        if(_note_finger_UP >= 0) {
            comboBoxNoteUP->addItem(QString::asprintf("%s %i", notes[_note_finger_UP % 12], _note_finger_UP / 12 - 1), _note_finger_UP);
            comboBoxNoteUP->setCurrentIndex(1);
        }

        connect(comboBoxNoteUP, QOverload<int>::of(&QComboBox::activated), [=](int v)
        {
           if(comboBoxNoteUP->itemData(v).toInt() < 0) {

               MidiInControl::set_current_note(-1);
               int note = MidiInControl::get_key();
               if(note >= 0) {

                   comboBoxNoteUP->removeItem(1);
                   comboBoxNoteUP->insertItem(1, QString::asprintf("%s %i", notes[note % 12], note / 12 - 1), note);
                   comboBoxNoteUP->setCurrentIndex(1);
                   _note_finger_UP = note;


               } else _note_finger_UP = -1;
               _settings->setValue("FINGER_note_UP", _note_finger_UP);
           } else {
               _note_finger_UP = comboBoxNoteUP->itemData(v).toInt();
               _settings->setValue("FINGER_note_UP", _note_finger_UP);
           }
        });

        checkKeyPressedUP->setChecked(_note_finger_pressed_UP);

        connect(checkKeyPressedUP, QOverload<bool>::of(&QCheckBox::toggled), [=](bool f)
        {
            _note_finger_pressed_UP = f;
            _settings->setValue("FINGER_pressed_UP", _note_finger_pressed_UP);
        });

        comboBoxNotePickUP->setToolTip("Note UP Pick key\n"
                                       "Pick the first note of the pattern at rythm\n"
                                       "that you press or release this note\n\n"
                                       "Record the activation note from \n"
                                       "the MIDI keyboard clicking\n"
                                       "'Get It' and pressing one");

        comboBoxNotePickUP->addItem("Get It", -1);
        if(_note_finger_pick_UP >= 0) {
            comboBoxNotePickUP->addItem(QString::asprintf("%s %i", notes[_note_finger_pick_UP % 12], _note_finger_pick_UP / 12 - 1), _note_finger_pick_UP);
            comboBoxNotePickUP->setCurrentIndex(1);
        }

        connect(comboBoxNotePickUP, QOverload<int>::of(&QComboBox::activated), [=](int v)
        {
           if(comboBoxNotePickUP->itemData(v).toInt() < 0) {

               MidiInControl::set_current_note(-1);
               int note = MidiInControl::get_key();
               if(note >= 0) {

                   comboBoxNotePickUP->removeItem(1);
                   comboBoxNotePickUP->insertItem(1, QString::asprintf("%s %i", notes[note % 12], note / 12 - 1), note);
                   comboBoxNotePickUP->setCurrentIndex(1);
                   _note_finger_pick_UP = note;


               } else _note_finger_pick_UP = -1;
               _settings->setValue("FINGER_note_pick_UP", _note_finger_pick_UP);
           } else {
               _note_finger_pick_UP = comboBoxNotePickUP->itemData(v).toInt();
               _settings->setValue("FINGER_note_pick_UP", _note_finger_pick_UP);
           }
        });

        comboBoxTypeUP2->clear();
        comboBoxTypeUP2->addItem(stringTypes[0]);
        comboBoxTypeUP2->addItem(stringTypes[1]);
        comboBoxTypeUP2->addItem(stringTypes[2]);
        comboBoxTypeUP2->addItem(stringTypes[3]);
        comboBoxTypeUP2->addItem(stringTypes[4]);

        indx = 0;

        if(_note_finger_pattern_UP2 < MAX_FINGER_BASIC) indx = _note_finger_pattern_UP2;

        for(int n = 0; n < 16; n++) {
            if(finger_patternUP[n + MAX_FINGER_BASIC][1]) {
                if(n + MAX_FINGER_BASIC == _note_finger_pattern_UP2) indx= _note_finger_pattern_UP2;

            }

            comboBoxTypeUP2->addItem("CUSTOM #" + QString::number(n + 1));
        }

        comboBoxTypeUP2->setCurrentIndex(indx);

        connect(comboBoxTypeUP2, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int num)
        {
            _note_finger_pattern_UP2 = num;
            _settings->setValue("FINGER_pattern_UP2", _note_finger_pattern_UP2);
        });

        comboBoxNoteUP2->setToolTip("Note UP 2 key\n"
                                    "if 'key pressed' is selected the key acts as a switch\n"
                                    "if 'key pressed' is not selected then if the recorded\n"
                                    "note is pressed, the pattern will be activated until\n"
                                    "this key is released. This allows the possibility of\n"
                                    "going step by step at the speed of the key is pressed\n"
                                    "and released.\n\n"
                                    "Record the activation note from \n"
                                    "the MIDI keyboard clicking\n"
                                    "'Get It' and pressing one");

        comboBoxNoteUP2->addItem("Get It", -1);

        if(_note_finger_UP2 >= 0) {
            comboBoxNoteUP2->addItem(QString::asprintf("%s %i", notes[_note_finger_UP2 % 12], _note_finger_UP2 / 12 - 1), _note_finger_UP2);
            comboBoxNoteUP2->setCurrentIndex(1);
        }

        connect(comboBoxNoteUP2, QOverload<int>::of(&QComboBox::activated), [=](int v)
        {
           if(comboBoxNoteUP2->itemData(v).toInt() < 0) {

               MidiInControl::set_current_note(-1);
               int note = MidiInControl::get_key();
               if(note >= 0) {

                   comboBoxNoteUP2->removeItem(1);
                   comboBoxNoteUP2->insertItem(1, QString::asprintf("%s %i", notes[note % 12], note / 12 - 1), note);
                   comboBoxNoteUP2->setCurrentIndex(1);
                   _note_finger_UP2 = note;


               } else _note_finger_UP2 = -1;
               _settings->setValue("FINGER_note_UP2", _note_finger_UP2);
           } else {
               _note_finger_UP2 = comboBoxNoteUP2->itemData(v).toInt();
               _settings->setValue("FINGER_note_UP2", _note_finger_UP2);
           }
        });

        checkKeyPressedUP2->setChecked(_note_finger_pressed_UP2);

        connect(checkKeyPressedUP2, QOverload<bool>::of(&QCheckBox::toggled), [=](bool f)
        {
            _note_finger_pressed_UP2 = f;
            _settings->setValue("FINGER_pressed_UP2", _note_finger_pressed_UP2);
        });

        labelDOWN = new QLabel(groupPlay);
        labelDOWN->setObjectName(QString::fromUtf8("labelDOWN"));
        labelDOWN->setGeometry(QRect(140, 0, 81, 20));
        labelDOWN->setAlignment(Qt::AlignCenter);
        labelDOWN->setText("Note DOWN");
        labelDOWN->setToolTip("Note used to enable Finger Pattern #1\n"
                              "to the channel DOWN");

        comboBoxTypeDOWN = new QComboBox(groupPlay);
        comboBoxTypeDOWN->setObjectName(QString::fromUtf8("comboBoxTypeDOWN"));
        comboBoxTypeDOWN->setGeometry(QRect(120, 20, 111, 22));
        comboBoxTypeDOWN->setToolTip("Selected Finger Pattern for DOWN");

        checkKeyPressedDOWN = new QCheckBox(groupPlay);
        checkKeyPressedDOWN->setObjectName(QString::fromUtf8("checkKeyPressedDOWN"));
        checkKeyPressedDOWN->setGeometry(QRect(130, 50, 91, 17));
        checkKeyPressedDOWN->setText("Key Pressed");
        checkKeyPressedDOWN->setToolTip("Selected the use of Finger Pattern key for DOWN\n"
                                        "checked: like a switch\n"
                                        "unchecked: like a push button or step to step");

        comboBoxNoteDOWN = new QComboBox(groupPlay);
        comboBoxNoteDOWN->setObjectName(QString::fromUtf8("comboBoxNoteDOWN"));
        comboBoxNoteDOWN->setGeometry(QRect(130, 70, 91, 31));
        comboBoxNoteDOWN->setFont(font);

        labelPickDOWN = new QLabel(groupPlay);
        labelPickDOWN->setObjectName(QString::fromUtf8("labelPickDOWN"));
        labelPickDOWN->setGeometry(QRect(130, 100, 91, 20));
        labelPickDOWN->setAlignment(Qt::AlignCenter);
        labelPickDOWN->setText("Pick Note");
        labelPickDOWN->setToolTip("Note used to enable Finger Pattern Picking\n"
                                  "to the channel DOWN");

        comboBoxNotePickDOWN = new QComboBox(groupPlay);
        comboBoxNotePickDOWN->setObjectName(QString::fromUtf8("comboBoxNotePickDOWN"));
        comboBoxNotePickDOWN->setGeometry(QRect(130, 120, 91, 31));
        comboBoxNotePickDOWN->setFont(font);

        labelDOWN2 = new QLabel(groupPlay);
        labelDOWN2->setObjectName(QString::fromUtf8("labelDOWN2"));
        labelDOWN2->setGeometry(QRect(380, 0, 81, 20));
        labelDOWN2->setAlignment(Qt::AlignCenter);
        labelDOWN2->setText("Note DOWN 2");
        labelDOWN2->setToolTip("Note used to enable Finger Pattern #2\n"
                               "to the channel DOWN");

        comboBoxTypeDOWN2 = new QComboBox(groupPlay);
        comboBoxTypeDOWN2->setObjectName(QString::fromUtf8("comboBoxTypeDOWN2"));
        comboBoxTypeDOWN2->setGeometry(QRect(360, 20, 111, 22));
        comboBoxTypeDOWN2->setToolTip("Selected Finger Pattern for DOWN 2");

        checkKeyPressedDOWN2 = new QCheckBox(groupPlay);
        checkKeyPressedDOWN2->setObjectName(QString::fromUtf8("checkKeyPressedDOWN2"));
        checkKeyPressedDOWN2->setGeometry(QRect(370, 50, 91, 17));
        checkKeyPressedDOWN2->setText("Key Pressed");
        checkKeyPressedDOWN2->setToolTip("Selected the use of Finger Pattern key for DOWN 2\n"
                                      "checked: like a switch\n"
                                      "unchecked: like a push button or step to step");

        comboBoxNoteDOWN2 = new QComboBox(groupPlay);
        comboBoxNoteDOWN2->setObjectName(QString::fromUtf8("comboBoxNoteDOWN2"));
        comboBoxNoteDOWN2->setGeometry(QRect(370, 70, 91, 31));
        comboBoxNoteDOWN2->setFont(font);

        // 370  580 + yy

        // 250  580 + yy
        checkBoxDisableFinger = new QCheckBox(fingerPatternDialog);
        checkBoxDisableFinger->setObjectName(QString::fromUtf8("checkBoxDisableFinger"));
        checkBoxDisableFinger->setGeometry(QRect(250, 580 + yy, 231, 21));
        QFont font2;
        font2.setPointSize(12);
        checkBoxDisableFinger->setFont(font2);
        checkBoxDisableFinger->setText("Disable Finger Pattern Input");
        checkBoxDisableFinger->setChecked(_note_finger_disabled);

        connect(checkBoxDisableFinger, QOverload<bool>::of(&QCheckBox::toggled), [=](bool f)
        {
            _note_finger_disabled = f;
            _settings->setValue("FINGER_disabled", _note_finger_disabled);
            groupPlay->setDisabled(f);
        });

        comboBoxTypeDOWN->clear();
        comboBoxTypeDOWN->addItem(stringTypes[0]);
        comboBoxTypeDOWN->addItem(stringTypes[1]);
        comboBoxTypeDOWN->addItem(stringTypes[2]);
        comboBoxTypeDOWN->addItem(stringTypes[3]);
        comboBoxTypeDOWN->addItem(stringTypes[4]);

        indx = 0;

        if(_note_finger_pattern_DOWN < MAX_FINGER_BASIC) indx = _note_finger_pattern_DOWN;

        for(int n = 0; n < 16; n++) {
            if(finger_patternDOWN[n + MAX_FINGER_BASIC][1]) {
                if(n + MAX_FINGER_BASIC == _note_finger_pattern_DOWN) indx= _note_finger_pattern_DOWN;

            }

            comboBoxTypeDOWN->addItem("CUSTOM #" + QString::number(n + 1));
        }

        comboBoxTypeDOWN->setCurrentIndex(indx);

        connect(comboBoxTypeDOWN, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int num)
        {
            _note_finger_pattern_DOWN = num;
            _settings->setValue("FINGER_pattern_DOWN", _note_finger_pattern_DOWN);
        });

        comboBoxNoteDOWN->setToolTip("Note DOWN key\n"
                                     "if 'key pressed' is selected the key acts as a switch\n"
                                     "if 'key pressed' is not selected then if the recorded\n"
                                     "note is pressed, the pattern will be activated until\n"
                                     "this key is released. This allows the possibility of\n"
                                     "going step by step at the speed of the key is pressed\n"
                                     "and released.\n\n"
                                     "Record the activation note from \n"
                                     "the MIDI keyboard clicking\n"
                                     "'Get It' and pressing one");

        comboBoxNoteDOWN->addItem("Get It", -1);
        if(_note_finger_DOWN >= 0) {
            comboBoxNoteDOWN->addItem(QString::asprintf("%s %i", notes[_note_finger_DOWN % 12], _note_finger_DOWN / 12 - 1), _note_finger_DOWN);
            comboBoxNoteDOWN->setCurrentIndex(1);
        }

        connect(comboBoxNoteDOWN, QOverload<int>::of(&QComboBox::activated), [=](int v)
        {
            if(comboBoxNoteDOWN->itemData(v).toInt() < 0) {

                MidiInControl::set_current_note(-1);
                int note = MidiInControl::get_key();
                if(note >= 0) {

                    comboBoxNoteDOWN->removeItem(1);
                    comboBoxNoteDOWN->insertItem(1, QString::asprintf("%s %i", notes[note % 12], note / 12 - 1), note);
                    comboBoxNoteDOWN->setCurrentIndex(1);
                    _note_finger_DOWN = note;


                } else _note_finger_DOWN = -1;
                _settings->setValue("FINGER_note_DOWN", _note_finger_DOWN);
            } else {
                _note_finger_DOWN = comboBoxNoteDOWN->itemData(v).toInt();
                _settings->setValue("FINGER_note_DOWN", _note_finger_DOWN);
            }
        });

        checkKeyPressedDOWN->setChecked(_note_finger_pressed_DOWN);

        connect(checkKeyPressedDOWN, QOverload<bool>::of(&QCheckBox::toggled), [=](bool f)
        {
            _note_finger_pressed_DOWN = f;
            _settings->setValue("FINGER_pressed_DOWN", _note_finger_pressed_DOWN);
        });

        comboBoxNotePickDOWN->setToolTip("Note DOWN Pick key\n"
                                         "Pick the first note of the pattern at rythm\n"
                                         "that you press or release this note\n\n"
                                         "Record the activation note from \n"
                                         "the MIDI keyboard clicking\n"
                                         "'Get It' and pressing one");

        comboBoxNotePickDOWN->addItem("Get It", -1);
        if(_note_finger_pick_DOWN >= 0) {
            comboBoxNotePickDOWN->addItem(QString::asprintf("%s %i", notes[_note_finger_pick_DOWN % 12], _note_finger_pick_DOWN / 12 - 1), _note_finger_pick_DOWN);
            comboBoxNotePickDOWN->setCurrentIndex(1);
        }

        connect(comboBoxNotePickDOWN, QOverload<int>::of(&QComboBox::activated), [=](int v)
        {
            if(comboBoxNotePickDOWN->itemData(v).toInt() < 0) {

                MidiInControl::set_current_note(-1);
                int note = MidiInControl::get_key();
                if(note >= 0) {

                    comboBoxNotePickDOWN->removeItem(1);
                    comboBoxNotePickDOWN->insertItem(1, QString::asprintf("%s %i", notes[note % 12], note / 12 - 1), note);
                    comboBoxNotePickDOWN->setCurrentIndex(1);
                    _note_finger_pick_DOWN = note;


                } else _note_finger_pick_DOWN = -1;
                _settings->setValue("FINGER_note_pick_DOWN", _note_finger_pick_DOWN);
            } else {
                _note_finger_pick_DOWN = comboBoxNotePickDOWN->itemData(v).toInt();
                _settings->setValue("FINGER_note_pick_DOWN", _note_finger_pick_DOWN);
            }
        });

        comboBoxTypeDOWN2->clear();
        comboBoxTypeDOWN2->addItem(stringTypes[0]);
        comboBoxTypeDOWN2->addItem(stringTypes[1]);
        comboBoxTypeDOWN2->addItem(stringTypes[2]);
        comboBoxTypeDOWN2->addItem(stringTypes[3]);
        comboBoxTypeDOWN2->addItem(stringTypes[4]);

        indx = 0;

        if(_note_finger_pattern_DOWN2 < MAX_FINGER_BASIC) indx = _note_finger_pattern_DOWN2;

        for(int n = 0; n < 16; n++) {
            if(finger_patternDOWN[n + MAX_FINGER_BASIC][1]) {
                if(n + MAX_FINGER_BASIC == _note_finger_pattern_DOWN2) indx= _note_finger_pattern_DOWN2;

            }

            comboBoxTypeDOWN2->addItem("CUSTOM #" + QString::number(n + 1));
        }

        comboBoxTypeDOWN2->setCurrentIndex(indx);

        connect(comboBoxTypeDOWN2, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int num)
        {
            _note_finger_pattern_DOWN2 = num;
            _settings->setValue("FINGER_pattern_DOWN2", _note_finger_pattern_DOWN2);
        });

        comboBoxNoteDOWN2->setToolTip("Note DOWN 2 key\n"
                                      "if 'key pressed' is selected the key acts as a switch\n"
                                      "if 'key pressed' is not selected then if the recorded\n"
                                      "note is pressed, the pattern will be activated until\n"
                                      "this key is released. This allows the possibility of\n"
                                      "going step by step at the speed of the key is pressed\n"
                                      "and released.\n\n"
                                      "Record the activation note from \n"
                                      "the MIDI keyboard clicking\n"
                                      "'Get It' and pressing one");

        comboBoxNoteDOWN2->addItem("Get It", -1);
        if(_note_finger_DOWN2 >= 0) {
            comboBoxNoteDOWN2->addItem(QString::asprintf("%s %i", notes[_note_finger_DOWN2 % 12], _note_finger_DOWN2 / 12 - 1), _note_finger_DOWN2);
            comboBoxNoteDOWN2->setCurrentIndex(1);
        }

        connect(comboBoxNoteDOWN2, QOverload<int>::of(&QComboBox::activated), [=](int v)
        {
            if(comboBoxNoteDOWN2->itemData(v).toInt() < 0) {

                MidiInControl::set_current_note(-1);
                int note = MidiInControl::get_key();
                if(note >= 0) {

                    comboBoxNoteDOWN2->removeItem(1);
                    comboBoxNoteDOWN2->insertItem(1, QString::asprintf("%s %i", notes[note % 12], note / 12 - 1), note);
                    comboBoxNoteDOWN2->setCurrentIndex(1);
                    _note_finger_DOWN2 = note;


                } else _note_finger_DOWN2 = -1;
                _settings->setValue("FINGER_note_DOWN2", _note_finger_DOWN2);
            } else {
                _note_finger_DOWN2 = comboBoxNoteDOWN2->itemData(v).toInt();
                _settings->setValue("FINGER_note_DOWN2", _note_finger_DOWN2);
            }
        });

        checkKeyPressedDOWN2->setChecked(_note_finger_pressed_DOWN2);

        connect(checkKeyPressedDOWN2, QOverload<bool>::of(&QCheckBox::toggled), [=](bool f)
        {
            _note_finger_pressed_DOWN2 = f;
            _settings->setValue("FINGER_pressed_DOWN2", _note_finger_pressed_DOWN2);
        });

        //***********

        connect(buttonBox, SIGNAL(accepted()), fingerPatternDialog, SLOT(accept()));
        connect(buttonBox, SIGNAL(rejected()), fingerPatternDialog, SLOT(reject()));
        connect(pushButtonTestNoteUP, SIGNAL(pressed()), fingerPatternDialog, SLOT(play_noteUP()));
        connect(pushButtonTestNoteUP, SIGNAL(released()), fingerPatternDialog, SLOT(stop_noteUP()));
        connect(pushButtonTestNoteDOWN, SIGNAL(pressed()), fingerPatternDialog, SLOT(play_noteDOWN()));
        connect(pushButtonTestNoteDOWN, SIGNAL(released()), fingerPatternDialog, SLOT(stop_noteDOWN()));

        thread_timer = NULL;

    } else {

        fingerPatternDialog->setDisabled(true);

        _note_finger_disabled = _settings->value("FINGER_disabled", false).toBool();

        _note_finger_pattern_UP = _settings->value("FINGER_pattern_UP", 0).toInt();
        _note_finger_UP = _settings->value("FINGER_note_UP", -1).toInt();
        _note_finger_pressed_UP = _settings->value("FINGER_pressed_UP", false).toBool();
        _note_finger_pick_UP = _settings->value("FINGER_note_pick_UP", -1).toInt();
        _note_finger_pattern_UP2 = _settings->value("FINGER_pattern_UP2", 0).toInt();
        _note_finger_UP2 = _settings->value("FINGER_note_UP2", -1).toInt();
        _note_finger_pressed_UP2 = _settings->value("FINGER_pressed_UP2", false).toBool();

        _note_finger_pattern_DOWN = _settings->value("FINGER_pattern_DOWN", 0).toInt();
        _note_finger_DOWN = _settings->value("FINGER_note_DOWN", -1).toInt();
        _note_finger_pressed_DOWN = _settings->value("FINGER_pressed_DOWN", false).toBool();
        _note_finger_pick_DOWN = _settings->value("FINGER_note_pick_DOWN", -1).toInt();
        _note_finger_pattern_DOWN2 = _settings->value("FINGER_pattern_DOWN2", 0).toInt();
        _note_finger_DOWN2 = _settings->value("FINGER_note_DOWN2", -1).toInt();
        _note_finger_pressed_DOWN2 = _settings->value("FINGER_pressed_DOWN2", false).toBool();

        fingerMUX = new QMutex(QMutex::NonRecursive);

        thread_timer = new finger_Thread();
        thread_timer->start(QThread::TimeCriticalPriority);
    }


    QMetaObject::connectSlotsByName(fingerPatternDialog);

}


FingerPatternDialog::~FingerPatternDialog() {

    if(thread_timer) {
        thread_timer->terminate();
        thread_timer->wait(1000);
        delete thread_timer;
        if(fingerMUX)  delete fingerMUX;
        fingerMUX = NULL;
        qDebug("Finger pattern thread exit()");
    }

}

void FingerPatternDialog::setComboNotePattern(int type, int index, QComboBox * cb) {

    if(!cb)
        return;

    cb->blockSignals(true);

    cb->clear();

    int i = 0;
    int v = 0;
//index++;
    if(type == 0) {

        if(index != 0) {
            cb->addItem("-1");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::red), Qt::TextColorRole);
            cb->addItem("-5");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::red), Qt::TextColorRole);
        } else
            v+= 2;

        cb->addItem(" 1");
        cb->setItemData(i, v++);
        cb->setItemData(i++, QBrush(Qt::darkGreen), Qt::TextColorRole);
        cb->addItem(" 5");
        cb->setItemData(i, v++);
        cb->setItemData(i++, QBrush(Qt::darkGreen), Qt::TextColorRole);

        if(index != 0) {
            cb->addItem("+1");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::magenta), Qt::TextColorRole);
            cb->addItem("+5");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::magenta), Qt::TextColorRole);

            cb->addItem("*-1");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::red), Qt::TextColorRole);
            cb->addItem("*-5");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::red), Qt::TextColorRole);
        } else
            v+= 4;

        cb->addItem("* 1");
        cb->setItemData(i, v++);
        cb->setItemData(i++, QBrush(Qt::darkGreen), Qt::TextColorRole);
        cb->addItem("* 5");
        cb->setItemData(i, v++);
        cb->setItemData(i++, QBrush(Qt::darkGreen), Qt::TextColorRole);

        if(index != 0) {
            cb->addItem("*+1");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::magenta), Qt::TextColorRole);
            cb->addItem("*+5");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::magenta), Qt::TextColorRole);
            cb->addItem("###");
            cb->setItemData(i, v++);
            i++;
            cb->addItem("*##");
            cb->setItemData(i, v++);
            i++;
        }

    } else if(type == 1 || type == 2){

        if(index != 0) {
            cb->addItem("-1");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::red), Qt::TextColorRole);
            cb->addItem("-3");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::red), Qt::TextColorRole);
            cb->addItem("-5");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::red), Qt::TextColorRole);
            cb->addItem("-7");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::red), Qt::TextColorRole);
        } else
            v+= 4;

        cb->addItem(" 1");
        cb->setItemData(i, v++);
        cb->setItemData(i++, QBrush(Qt::darkGreen), Qt::TextColorRole);
        cb->addItem(" 3");
        cb->setItemData(i, v++);
        cb->setItemData(i++, QBrush(Qt::darkGreen), Qt::TextColorRole);
        cb->addItem(" 5");
        cb->setItemData(i, v++);
        cb->setItemData(i++, QBrush(Qt::darkGreen), Qt::TextColorRole);
        cb->addItem(" 7");
        cb->setItemData(i, v++);
        cb->setItemData(i++, QBrush(Qt::darkGreen), Qt::TextColorRole);

        if(index != 0) {
            cb->addItem("+1");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::magenta), Qt::TextColorRole);
            cb->addItem("+3");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::magenta), Qt::TextColorRole);
            cb->addItem("+5");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::magenta), Qt::TextColorRole);
            cb->addItem("+7");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::magenta), Qt::TextColorRole);

            cb->addItem("*-1");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::red), Qt::TextColorRole);
            cb->addItem("*-3");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::red), Qt::TextColorRole);
            cb->addItem("*-5");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::red), Qt::TextColorRole);
            cb->addItem("*-7");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::red), Qt::TextColorRole);
        }  else
            v+= 8;

        cb->addItem("* 1");
        cb->setItemData(i, v++);
        cb->setItemData(i++, QBrush(Qt::darkGreen), Qt::TextColorRole);
        cb->addItem("* 3");
        cb->setItemData(i, v++);
        cb->setItemData(i++, QBrush(Qt::darkGreen), Qt::TextColorRole);
        cb->addItem("* 5");
        cb->setItemData(i, v++);
        cb->setItemData(i++, QBrush(Qt::darkGreen), Qt::TextColorRole);
        cb->addItem("* 7");
        cb->setItemData(i, v++);
        cb->setItemData(i++, QBrush(Qt::darkGreen), Qt::TextColorRole);

        if(index != 0) {
            cb->addItem("*+1");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::magenta), Qt::TextColorRole);
            cb->addItem("*+3");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::magenta), Qt::TextColorRole);
            cb->addItem("*+5");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::magenta), Qt::TextColorRole);
            cb->addItem("*+7");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::magenta), Qt::TextColorRole);
            cb->addItem("###");
            cb->setItemData(i, v++);
            i++;
            cb->addItem("*##");
            cb->setItemData(i, v++);
            i++;
        }

    }  else if(type == 3 || type == 4) {

        if(index != 0) {
            cb->addItem("-1");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::red), Qt::TextColorRole);
            cb->addItem("-2");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::red), Qt::TextColorRole);
            cb->addItem("-3");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::red), Qt::TextColorRole);
            cb->addItem("-5");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::red), Qt::TextColorRole);
            cb->addItem("-6");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::red), Qt::TextColorRole);
        }  else
            v+= 5;

        cb->addItem(" 1");
        cb->setItemData(i, v++);
        cb->setItemData(i++, QBrush(Qt::darkGreen), Qt::TextColorRole);
        cb->addItem(" 2");
        cb->setItemData(i, v++);
        cb->setItemData(i++, QBrush(Qt::darkGreen), Qt::TextColorRole);
        cb->addItem(" 3");
        cb->setItemData(i, v++);
        cb->setItemData(i++, QBrush(Qt::darkGreen), Qt::TextColorRole);
        cb->addItem(" 5");
        cb->setItemData(i, v++);
        cb->setItemData(i++, QBrush(Qt::darkGreen), Qt::TextColorRole);
        cb->addItem(" 6");
        cb->setItemData(i, v++);
        cb->setItemData(i++, QBrush(Qt::darkGreen), Qt::TextColorRole);

        if(index != 0) {
            cb->addItem("+1");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::magenta), Qt::TextColorRole);
            cb->addItem("+2");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::magenta), Qt::TextColorRole);
            cb->addItem("+3");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::magenta), Qt::TextColorRole);
            cb->addItem("+5");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::magenta), Qt::TextColorRole);
            cb->addItem("+6");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::magenta), Qt::TextColorRole);

            cb->addItem("*-1");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::red), Qt::TextColorRole);
            cb->addItem("*-2");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::red), Qt::TextColorRole);
            cb->addItem("*-3");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::red), Qt::TextColorRole);
            cb->addItem("*-5");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::red), Qt::TextColorRole);
            cb->addItem("*-6");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::red), Qt::TextColorRole);
        } else
            v+= 10;

        cb->addItem("* 1");
        cb->setItemData(i, v++);
        cb->setItemData(i++, QBrush(Qt::darkGreen), Qt::TextColorRole);
        cb->addItem("* 2");
        cb->setItemData(i, v++);
        cb->setItemData(i++, QBrush(Qt::darkGreen), Qt::TextColorRole);
        cb->addItem("* 3");
        cb->setItemData(i, v++);
        cb->setItemData(i++, QBrush(Qt::darkGreen), Qt::TextColorRole);
        cb->addItem("* 5");
        cb->setItemData(i, v++);
        cb->setItemData(i++, QBrush(Qt::darkGreen), Qt::TextColorRole);
        cb->addItem("* 6");
        cb->setItemData(i, v++);
        cb->setItemData(i++, QBrush(Qt::darkGreen), Qt::TextColorRole);

        if(index != 0) {
            cb->addItem("*+1");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::magenta), Qt::TextColorRole);
            cb->addItem("*+2");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::magenta), Qt::TextColorRole);
            cb->addItem("*+3");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::magenta), Qt::TextColorRole);
            cb->addItem("*+5");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::magenta), Qt::TextColorRole);
            cb->addItem("*+6");
            cb->setItemData(i, v++);
            cb->setItemData(i++, QBrush(Qt::magenta), Qt::TextColorRole);
            cb->addItem("###");
            cb->setItemData(i, v++);
            i++;
            cb->addItem("*##");
            cb->setItemData(i, v++);
            i++;
        }
    }

    cb->blockSignals(false);

}


static unsigned char map_key_stepUP[128];
static unsigned char map_key_step_maxUP[128];
static unsigned char map_key_tempo_stepUP[128];

static unsigned char map_keyUP[128];
static unsigned char map_key_offUP[128];
static unsigned char map_key_onUP[128];
static unsigned char map_key_velocityUP[128];

static unsigned char map_key_stepDOWN[128];
static unsigned char map_key_step_maxDOWN[128];
static unsigned char map_key_tempo_stepDOWN[128];

static unsigned char map_keyDOWN[128];
static unsigned char map_key_offDOWN[128];
static unsigned char map_key_onDOWN[128];
static unsigned char map_key_velocityDOWN[128];

static int init_map_key = 1;

int finger_enable_UP = 0;
int finger_enable_DOWN = 0;

static int getKey(int zone, int n, int m, int type, int note, int note_disp) {

    int key2 = 0;

    switch(type) {

    case 0:

        if(note >= 12) {
            key2 = -1;
            if(note > 12) {
                if(!zone)
                    map_key_step_maxUP[n]|= 128;
                else
                    map_key_step_maxDOWN[n]|= 128;
            }

        } else {
            if(note > 5)  {
                if(!zone)
                    map_key_step_maxUP[n]|= 128;
                else
                    map_key_step_maxDOWN[n]|= 128;
            }

            note %= 6;

            int key_off = MidiInControl::GetNoteChord(0, note_disp % 2, 12) % 12;
            m = (m/12)*12 + (m % 12) - key_off;
           // m = m - key_off;

            key2 = MidiInControl::GetNoteChord(0, note % 2, m + 12*(note/2) - 12);
        }

        break;

    case 1:

        if(note >= 24) {

            key2 = -1;

            if(note > 24) {
                if(!zone)
                    map_key_step_maxUP[n]|= 128;
                else
                    map_key_step_maxDOWN[n]|= 128;
            }

        } else {

            if(note > 11)  {
                if(!zone)
                    map_key_step_maxUP[n]|= 128;
                else
                    map_key_step_maxDOWN[n]|= 128;
            }

            note %= 12;

            int key_off = MidiInControl::GetNoteChord(3, note_disp % 4, 12) % 12;
            m = (m/12)*12 + (m % 12) - key_off;

            key2 = MidiInControl::GetNoteChord(3, note % 4, m + 12*(note/4) - 12);
        }

        break;

    case 2:

        if(note >= 24) {

            key2 = -1;

            if(note > 24) {
                if(!zone)
                    map_key_step_maxUP[n]|= 128;
                else
                    map_key_step_maxDOWN[n]|= 128;
            }

        } else {

            if(note > 11) {
                if(!zone)
                    map_key_step_maxUP[n]|= 128;
                else
                    map_key_step_maxDOWN[n]|= 128;
            }

            note %= 12;

            int key_off = MidiInControl::GetNoteChord(4, note_disp % 4, 12) % 12;
            m = (m/12)*12 + (m % 12) - key_off;

            key2 = MidiInControl::GetNoteChord(4, note % 4, m + 12*(note/4) - 12);
        }

        break;

    case 3:

        if(note >= 30) {

            key2 = -1;

            if(note > 30) {
                if(!zone)
                    map_key_step_maxUP[n]|= 128;
                else
                    map_key_step_maxDOWN[n]|= 128;
            }

        } else {

            if(note > 14)  {
                if(!zone)
                    map_key_step_maxUP[n]|= 128;
                else
                    map_key_step_maxDOWN[n]|= 128;
            }

            note %= 15;

            int key_off = MidiInControl::GetNoteChord(5, note_disp % 5, 12) % 12;
            m = (m/12)*12 + (m % 12) - key_off;

            key2 = MidiInControl::GetNoteChord(5, note % 5, m + 12*(note/5) - 12);
        }
        break;
    case 4:

        if(note >= 30) {

            key2 = -1;

            if(note > 30) {
                if(!zone)
                    map_key_step_maxUP[n]|= 128;
                else
                    map_key_step_maxDOWN[n]|= 128;
            }

        } else {

            if(note > 14) {
                if(!zone)
                    map_key_step_maxUP[n]|= 128;
                else
                    map_key_step_maxDOWN[n]|= 128;
            }

            note %= 15;

            int key_off = MidiInControl::GetNoteChord(6, note_disp % 5, 12) % 12;
            m = (m/12)*12 + (m % 12) - key_off;


            key2 = MidiInControl::GetNoteChord(6, note % 5, m + 12*(note/5) - 12);
        }

        break;
    }

    if(key2 > 127) key2 = 127;

    return key2;
}

void FingerPatternDialog::finger_timer() {

    if(!fingerMUX) {
        qFatal("finger_timer() without fingerMUX defined!");
        return;
    }

    fingerMUX->lock();

    QByteArray messageByte;
    std::vector<unsigned char> message;

    int ch_up = ((MidiInControl::channelUp() < 0)
                 ? MidiOutput::standardChannel()
                 : MidiInControl::channelUp()) & 15;
    int ch_down = ((MidiInControl::channelDown() < 0)
                   ? ((MidiInControl::channelUp() < 0)
                      ? MidiOutput::standardChannel()
                      : MidiInControl::channelUp())
                   : MidiInControl::channelDown()) & 0xF;

// INITIALIZE MAPS KEYS

    if(init_map_key) {

        init_map_key = 0;

        for(int n = 0; n < 128; n++) {

            map_keyUP[n] = 0;
            map_key_stepUP[n] = 0;
            map_key_onUP[n] = 0;
            map_key_offUP[n] = 0;

            map_keyDOWN[n] = 0;
            map_key_stepDOWN[n] = 0;
            map_key_onDOWN[n] = 0;
            map_key_offDOWN[n] = 0;
        }
    }

// UP CONTROL KEYS

    if(!finger_switch_playUP) {

        finger_enable_UP = 0;

        if(_note_finger_pick_on_UP) {

            finger_enable_UP = 1;

            if(_note_finger_on_UP != 2 && _note_finger_on_UP2!= 2)
                max_tick_counterUP = 128;

            if(_note_finger_on_UP == 1)
                _note_finger_on_UP = 0;

            if(_note_finger_on_UP2 == 1)
                _note_finger_on_UP2 = 0;


            for(int m = 0; m < 128; m++) {
                if(_note_finger_update_UP) {
                    map_key_tempo_stepUP[m] = 0;
                    map_key_stepUP[m] &= ~15;
                    map_key_step_maxUP[m] = (finger_patternUP[finger_chord_modeUP][3] << 4) | (finger_patternUP[finger_chord_modeUP][1] & 15);
                }
            }

            _note_finger_update_UP = 0;

        } else if(_note_finger_on_UP) {

            finger_enable_UP = 1;
            _note_finger_on_UP2 = 0;
            max_tick_counterUP = finger_patternUP[_note_finger_pattern_UP][2];
            finger_chord_modeUP = _note_finger_pattern_UP;

            for(int m = 0; m < 128; m++) {
                if(_note_finger_update_UP) {
                    if(_note_finger_update_UP == 2) map_key_stepUP[m] &= ~15;
                    map_key_step_maxUP[m] = (finger_patternUP[finger_chord_modeUP][3] << 4) | (finger_patternUP[finger_chord_modeUP][1] & 15);
                } else if(map_key_step_maxUP[m])
                    map_key_step_maxUP[m] = (map_key_step_maxUP[m] & ~15) | (finger_patternUP[finger_chord_modeUP][1] & 15);
            }

            _note_finger_update_UP = 0;

            finger_enable_UP = 1;

        } else if(_note_finger_on_UP2) {

            finger_enable_UP = 1;
            _note_finger_on_UP = 0;
            max_tick_counterUP = finger_patternUP[_note_finger_pattern_UP2][2];
            finger_chord_modeUP = _note_finger_pattern_UP2;

            for(int m = 0; m < 128; m++) {

                if(_note_finger_update_UP) {
                    if(_note_finger_update_UP == 2) map_key_stepUP[m] &= ~15;
                    map_key_step_maxUP[m] = (finger_patternUP[finger_chord_modeUP][3] << 4) | (finger_patternUP[finger_chord_modeUP][1] & 15);
                } else if(map_key_step_maxUP[m])
                    map_key_step_maxUP[m] = (map_key_step_maxUP[m] & ~15) | (finger_patternUP[finger_chord_modeUP][1] & 15);
            }

            _note_finger_update_UP = 0;

            finger_enable_UP = 1;
        }
    }

// DOWN CONTROL KEYS

    if(!finger_switch_playDOWN) {

        finger_enable_DOWN = 0;

        if(_note_finger_pick_on_DOWN) {

            finger_enable_DOWN = 1;

            if(_note_finger_on_DOWN != 2 && _note_finger_on_DOWN2!= 2)
                max_tick_counterDOWN = 128;

            if(_note_finger_on_DOWN == 1)
                _note_finger_on_DOWN = 0;

            if(_note_finger_on_DOWN2 == 1)
                _note_finger_on_DOWN2 = 0;

            for(int m = 0; m < 128; m++) {
                if(_note_finger_update_DOWN) {
                    map_key_tempo_stepUP[m] = 0;
                    map_key_stepDOWN[m] &= ~15;
                    map_key_step_maxDOWN[m] = (finger_patternDOWN[finger_chord_modeDOWN][3] << 4) | (finger_patternDOWN[finger_chord_modeDOWN][1] & 15);
                }
            }

            _note_finger_update_DOWN = 0;

        } else if(_note_finger_on_DOWN) {

            finger_enable_DOWN = 1;
            _note_finger_on_DOWN2 = 0;
            max_tick_counterDOWN = finger_patternDOWN[_note_finger_pattern_DOWN][2];
            finger_chord_modeDOWN = _note_finger_pattern_DOWN;

            for(int m = 0; m < 128; m++) {

                if(_note_finger_update_DOWN) {
                    if(_note_finger_update_DOWN == 2) map_key_stepDOWN[m] &= ~15;
                    map_key_step_maxDOWN[m] = (finger_patternDOWN[finger_chord_modeDOWN][3] << 4) | (finger_patternDOWN[finger_chord_modeDOWN][1] & 15);
                } else if(map_key_step_maxDOWN[m])
                    map_key_step_maxDOWN[m] = (map_key_step_maxDOWN[m] & ~15) | (finger_patternDOWN[finger_chord_modeDOWN][1] & 15);
            }

            _note_finger_update_DOWN = 0;

            finger_enable_DOWN = 1;

        } else if(_note_finger_on_DOWN2) {

            finger_enable_DOWN = 1;
            _note_finger_on_DOWN = 0;

            max_tick_counterDOWN = finger_patternDOWN[_note_finger_pattern_DOWN2][2];
            finger_chord_modeDOWN = _note_finger_pattern_DOWN2;

            for(int m = 0; m < 128; m++) {

                if(_note_finger_update_DOWN) {
                    if(_note_finger_update_DOWN == 2) map_key_stepDOWN[m] &= ~15;
                    map_key_step_maxDOWN[m] = (finger_patternDOWN[finger_chord_modeDOWN][3] << 4) | (finger_patternDOWN[finger_chord_modeDOWN][1] & 15);
                } else if(map_key_step_maxDOWN[m])
                    map_key_step_maxDOWN[m] = (map_key_step_maxDOWN[m] & ~15) | (finger_patternDOWN[finger_chord_modeDOWN][1] & 15);
            }

            _note_finger_update_DOWN = 0;

            finger_enable_DOWN = 1;
        }

    }


// FINGER NOTE ENGINE

    for(int m = 0; m < 128; m++) {

        int flag_end = 0;

// UP ZONE

        if((finger_switch_playUP || finger_enable_UP) && (map_key_stepUP[m] & 128)) {

            int current_index = map_key_stepUP[m] & 15;

            if(map_key_tempo_stepUP[m] == 0) {

                map_key_step_maxUP[m] &= 127;

                for(int n = 0; n < 128; n++) {

                    int key2 = map_keyUP[n];

                    if((key2 & 128) && (key2 & 127) == m) {
                        map_key_offUP[n]=1;
                    }
                }


                for(int n = 0; n < 128; n++) {

                    int key2 = m;

                    if(!_note_finger_pick_on_UP) {

                        key2 = getKey(0, n, m, finger_patternUP[finger_chord_modeUP][0],
                                finger_patternUP[finger_chord_modeUP][4 + current_index], finger_patternUP[finger_chord_modeUP][4]);
                    }

                    if(key2 >= 0 && (map_key_stepUP[n] & 128)) {

                        map_keyUP[key2] = m | 128;
                        map_key_onUP[key2] = 1;
                    }

                }

                if(finger_enable_UP && map_key_step_maxUP[m]!=0)
                    current_index++;

                if(current_index >= ((map_key_step_maxUP[m] & 15))) {

                    current_index = 0;

                    flag_end = 1;

                    if(flag_end) {

                        int loop = (map_key_step_maxUP[m] >> 4) & 7;

                        if(loop) {

                            loop--;

                            map_key_step_maxUP[m] = (map_key_step_maxUP[m] & 15)
                                    | (loop <<4) | (map_key_step_maxUP[m] & 128);

                            if(!loop) {
                                map_key_step_maxUP[m] = 0;
                            }
                        }
                    }
                }

                map_key_stepUP[m]= current_index  | (map_key_stepUP[m] & ~15);
            }

            int tick_counter;

            if(map_key_step_maxUP[m])
                map_key_tempo_stepUP[m]++;
            else
                map_key_tempo_stepUP[m] = 1;

            tick_counter = map_key_tempo_stepUP[m];

            int maxcounter = (max_tick_counterUP + 1);

            if(maxcounter > 64) maxcounter = 64;

            if(map_key_step_maxUP[m] & 128) {
                maxcounter*= 2;
               // qDebug("double up");
            }

            if(tick_counter >= maxcounter) {
                tick_counter = 0;
            }

            map_key_tempo_stepUP[m] = tick_counter;
        }

        flag_end = 0;

 // DOWN ZONE

        if((finger_switch_playDOWN || finger_enable_DOWN) && (map_key_stepDOWN[m] & 128)) {

            int current_index = map_key_stepDOWN[m] & 15;

            if(map_key_tempo_stepDOWN[m] == 0) {

                map_key_step_maxDOWN[m] &= 127;

                for(int n = 0; n < 128; n++) {

                    int key2 = map_keyDOWN[n];

                    if((key2 & 128) && (key2 & 127) == m) {
                        map_key_offDOWN[n]=1;
                    }
                }

                for(int n = 0; n < 128; n++) {

                    int key2 = m;

                    if(!_note_finger_pick_on_DOWN) {

                        key2 = getKey(1, n, m, finger_patternDOWN[finger_chord_modeDOWN][0],
                                finger_patternDOWN[finger_chord_modeDOWN][4 + current_index], finger_patternDOWN[finger_chord_modeDOWN][4]);
                    }

                    if(key2 >= 0 && (map_key_stepDOWN[n] & 128)) {

                        map_keyDOWN[key2] = m | 128;
                        map_key_onDOWN[key2] = 1;
                    }

                }

                if(finger_enable_DOWN && map_key_step_maxDOWN[m]!=0)
                    current_index++;

                if(current_index >= ((map_key_step_maxDOWN[m] & 15))) {

                    current_index = 0;

                    flag_end = 1;

                    if(flag_end) {

                        int loop = (map_key_step_maxDOWN[m] >> 4) & 7;

                        if(loop) {

                            loop--;

                            map_key_step_maxDOWN[m] = (map_key_step_maxDOWN[m] & 15)
                                    | (loop <<4) | (map_key_step_maxDOWN[m] & 128);

                            if(!loop) {
                                map_key_step_maxDOWN[m] = 0;
                            }
                        }
                    }
                }

                map_key_stepDOWN[m]= current_index  | (map_key_stepDOWN[m] & ~15);
            }

            int tick_counter;

            if(map_key_step_maxDOWN[m])
                map_key_tempo_stepDOWN[m]++;
            else
                map_key_tempo_stepDOWN[m] = 1;

            tick_counter = map_key_tempo_stepDOWN[m];

            int maxcounter = (max_tick_counterDOWN + 1);

            if(maxcounter > 64) maxcounter = 64;

            if(map_key_step_maxDOWN[m] & 128) {
                maxcounter*= 2;
               // qDebug("double up");
            }

            if(tick_counter >= maxcounter) {
                tick_counter = 0;
            }

            map_key_tempo_stepDOWN[m] = tick_counter;
        }

    }

// FORCING RESET KEY TEMPO COUNTER

    for(int n = 0; n < 128; n++) {
        if(!finger_enable_UP)
            map_key_tempo_stepUP[n] = 0;

        if(!finger_enable_DOWN)
            map_key_tempo_stepDOWN[n] = 0;
    }


// FINGER PLAY NOTES ON/OFF

    for(int n = 0; n < 128; n++) {

// UP ZONE NOTE OFF

        if(map_key_offUP[n]) {

            if(!finger_switch_playUP) {

                message.clear();
                message.emplace_back((unsigned char) 0x80 + ch_up);
                message.emplace_back((unsigned char) n);
                message.emplace_back((unsigned char) 0);
            } else {

                messageByte.clear();
                messageByte.append((unsigned char) 0x80 + ch_up);
                messageByte.append((unsigned char) n);
                messageByte.append((unsigned char) 0);
            }

            finger_token = map_keyUP[n] & 127;

            if(!finger_switch_playUP)
                MidiInput::receiveMessage(0.0, &message, &finger_token);
            else
                MidiOutput::sendCommand(messageByte);

            map_key_offUP[n]--;
        }

// DOWN ZONE NOTE OFF

        if(map_key_offDOWN[n]) {

            if(!finger_switch_playDOWN) {

                message.clear();
                message.emplace_back((unsigned char) 0x80 + ch_down);
                message.emplace_back((unsigned char) n);
                message.emplace_back((unsigned char) 0);
            } else {

                messageByte.clear();
                messageByte.append((unsigned char) 0x80 + ch_down);
                messageByte.append((unsigned char) n);
                messageByte.append((unsigned char) 0);
            }

            finger_token = map_keyDOWN[n] & 127;

            if(!finger_switch_playDOWN)
                MidiInput::receiveMessage(0.0, &message, &finger_token);
            else
                MidiOutput::sendCommand(messageByte);

            map_key_offDOWN[n]--;
        }

// UP ZONE NOTE ON

        if(map_key_onUP[n]) {

            if(!finger_switch_playUP) {

                message.clear();
                message.emplace_back((unsigned char) 0x90 + ch_up);
                message.emplace_back((unsigned char) n);
                message.emplace_back((unsigned char) map_key_velocityUP[map_keyUP[n] & 127]);
            } else {

                messageByte.clear();
                messageByte.append((unsigned char) 0x90 + ch_up);
                messageByte.append((unsigned char) n);
                messageByte.append((unsigned char) map_key_velocityUP[map_keyUP[n] & 127]);
            }

            finger_token = map_keyUP[n] & 127;

            if(!finger_switch_playUP)
                MidiInput::receiveMessage(0.0, &message, &finger_token);
            else
                MidiOutput::sendCommand(messageByte);

            map_key_onUP[n] = 0;
        }

// DOWN ZONE NOTE ON

        if(map_key_onDOWN[n]) {

            if(!finger_switch_playDOWN) {

                message.clear();
                message.emplace_back((unsigned char) 0x90 + ch_down);
                message.emplace_back((unsigned char) n);
                message.emplace_back((unsigned char) map_key_velocityDOWN[map_keyDOWN[n] & 127]);
            } else {

                messageByte.clear();
                messageByte.append((unsigned char) 0x90 + ch_down);
                messageByte.append((unsigned char) n);
                messageByte.append((unsigned char) map_key_velocityDOWN[map_keyDOWN[n] & 127]);
            }

            finger_token = map_keyDOWN[n] & 127;

            if(!finger_switch_playDOWN)
                MidiInput::receiveMessage(0.0, &message, &finger_token);
            else
                MidiOutput::sendCommand(messageByte);

            map_key_onDOWN[n] = 0;
        }

    }
    fingerMUX->unlock();
}

// FROM DIALOG

void FingerPatternDialog::play_noteUP() {

    std::vector<unsigned char> message;
    QByteArray messageByte;

    int ch_up = ((MidiInControl::channelUp() < 0)
                 ? MidiOutput::standardChannel()
                 : MidiInControl::channelUp()) & 15;

    if(init_map_key) {

       init_map_key = 0;

       for(int n = 0; n < 128; n++) {
           map_keyUP[n] = 0;
           map_key_stepUP[n] = 0;
           map_key_onUP[n] = 0;
           map_key_offUP[n] = 0;

           map_keyDOWN[n] = 0;
           map_key_stepDOWN[n] = 0;
           map_key_onDOWN[n] = 0;
           map_key_offDOWN[n] = 0;
       }
    }

    finger_key = finger_base_note;
    finger_enable_UP = 1;
    finger_switch_playUP = 1;

    for(int n = 0; n < 128; n++) {

        if(map_keyUP[n] == (128 | finger_key)) {

            map_keyUP[n] = 0;

            if(!_parent) {

                message.clear();
                message.emplace_back((unsigned char) 0x80 + ch_up);
                message.emplace_back((unsigned char) n);
                message.emplace_back((unsigned char) 0);

            } else {

                messageByte.clear();
                messageByte.append((unsigned char) 0x80 + ch_up);
                messageByte.append((unsigned char) n);
                messageByte.append((unsigned char) 0);
            }

            if(!_parent) {

                finger_token = finger_key;
                MidiInput::receiveMessage(0.0, &message, &finger_token);
            } else
                MidiOutput::sendCommand(messageByte);


        }
    }

    int steps = finger_patternUP[finger_chord_modeUP][1];

    //qDebug("on");

    map_key_tempo_stepUP[finger_key] = 0;
    map_key_step_maxUP[finger_key] = steps | (finger_repeatUP << 4);
    map_key_stepUP[finger_key] = 128;

    map_key_velocityUP[finger_key] = 100;

    map_keyUP[finger_key] = finger_key | 128;

}

// FROM DIALOG

void FingerPatternDialog::stop_noteUP() {

    std::vector<unsigned char> message;
    QByteArray messageByte;

    int ch_up = ((MidiInControl::channelUp() < 0)
                 ? MidiOutput::standardChannel()
                 : MidiInControl::channelUp()) & 15;

    finger_switch_playUP = 1;

    finger_key = finger_base_note;

    if(init_map_key) {

       init_map_key = 0;

       for(int n = 0; n < 128; n++) {

           map_keyUP[n] = 0;
           map_key_stepUP[n] = 0;
           map_key_onUP[n] = 0;
           map_key_offUP[n] = 0;

           map_keyDOWN[n] = 0;
           map_key_stepDOWN[n] = 0;
           map_key_onDOWN[n] = 0;
           map_key_offDOWN[n] = 0;
       }
    }

    for(int n = 0; n < 128; n++) {

        if(map_keyUP[n] == (128 | finger_key)) {

            map_keyUP[n] = 0;

            if(!_parent) {

                message.clear();
                message.emplace_back((unsigned char) 0x80 + ch_up);
                message.emplace_back((unsigned char) n);
                message.emplace_back((unsigned char) 0);

            } else {

                messageByte.clear();
                messageByte.append((unsigned char) 0x80 + ch_up);
                messageByte.append((unsigned char) n);
                messageByte.append((unsigned char) 0);
            }

            if(!_parent) {

                finger_token = finger_key;

                MidiInput::receiveMessage(0.0, &message, &finger_token);

            } else
                MidiOutput::sendCommand(messageByte);

        }
    }

    map_key_stepUP[finger_key] = 0;

    finger_enable_UP = 0;

    //qDebug("off");

}

// FROM DIALOG

void FingerPatternDialog::play_noteDOWN() {

    std::vector<unsigned char> message;
    QByteArray messageByte;

    int ch_down = ((MidiInControl::channelDown() < 0)
                   ? ((MidiInControl::channelUp() < 0)
                      ? MidiOutput::standardChannel()
                      : MidiInControl::channelUp())
                   : MidiInControl::channelDown()) & 0xF;

    if(init_map_key) {

       init_map_key = 0;

       for(int n = 0; n < 128; n++) {

           map_keyUP[n] = 0;
           map_key_stepUP[n] = 0;
           map_key_onUP[n] = 0;
           map_key_offUP[n] = 0;

           map_keyDOWN[n] = 0;
           map_key_stepDOWN[n] = 0;
           map_key_onDOWN[n] = 0;
           map_key_offDOWN[n] = 0;
       }
    }

    finger_key = finger_base_note;

    finger_enable_DOWN = 1;
    finger_switch_playDOWN = 1;

    for(int n = 0; n < 128; n++) {

        if(map_keyDOWN[n] == (128 | finger_key)) {

            map_keyDOWN[n] = 0;

            if(!_parent) {

                message.clear();
                message.emplace_back((unsigned char) 0x80 + ch_down);
                message.emplace_back((unsigned char) n);
                message.emplace_back((unsigned char) 0);

            } else {

                messageByte.clear();
                messageByte.append((unsigned char) 0x80 + ch_down);
                messageByte.append((unsigned char) n);
                messageByte.append((unsigned char) 0);
            }

            if(!_parent) {

                finger_token = finger_key;
                MidiInput::receiveMessage(0.0, &message, &finger_token);

            } else
                MidiOutput::sendCommand(messageByte);


        }
    }

    int steps = finger_patternDOWN[finger_chord_modeDOWN][1];

    //qDebug("on");
    map_key_tempo_stepDOWN[finger_key] = 0;
    map_key_step_maxDOWN[finger_key] = steps | (finger_repeatDOWN << 4);
    map_key_stepDOWN[finger_key] = 128;

    map_key_velocityDOWN[finger_key] = 100;

    map_keyDOWN[finger_key] = finger_key | 128;


}

// FROM DIALOG

void FingerPatternDialog::stop_noteDOWN() {

    std::vector<unsigned char> message;
    QByteArray messageByte;

    int ch_down = ((MidiInControl::channelDown() < 0)
                   ? ((MidiInControl::channelUp() < 0)
                      ? MidiOutput::standardChannel()
                      : MidiInControl::channelUp())
                   : MidiInControl::channelDown()) & 0xF;

    finger_switch_playDOWN = 1;

    finger_key = finger_base_note;

    if(init_map_key) {

       init_map_key = 0;

       for(int n = 0; n < 128; n++) {

           map_keyUP[n] = 0;
           map_key_stepUP[n] = 0;
           map_key_onUP[n] = 0;
           map_key_offUP[n] = 0;

           map_keyDOWN[n] = 0;
           map_key_stepDOWN[n] = 0;
           map_key_onDOWN[n] = 0;
           map_key_offDOWN[n] = 0;
       }
    }

    for(int n = 0; n < 128; n++) {

        if(map_keyDOWN[n] == (128 | finger_key)) {

            map_keyDOWN[n] = 0;

            if(!_parent) {

                message.clear();
                message.emplace_back((unsigned char) 0x80 + ch_down);
                message.emplace_back((unsigned char) n);
                message.emplace_back((unsigned char) 0);

            } else {

                messageByte.clear();
                messageByte.append((unsigned char) 0x80 + ch_down);
                messageByte.append((unsigned char) n);
                messageByte.append((unsigned char) 0);
            }

            if(!_parent) {

                finger_token = finger_key;
                MidiInput::receiveMessage(0.0, &message, &finger_token);

            } else
                MidiOutput::sendCommand(messageByte);

        }
    }

    map_key_stepDOWN[finger_key] = 0;

    finger_enable_DOWN = 0;

    //qDebug("off");

}

// FROM MidiInput -> MidiInControl::finger_func()

int FingerPatternDialog::Finger_note(std::vector<unsigned char>* message) {

    if(!fingerMUX) {
        qDebug("Finger_note() without fingerMUX defined!");
        return 0;
    }

    fingerMUX->lock();

    if(!message->at(2)) // velocity 0 == note off
        message->at(0) = 0x80 + (message->at(0) & 0xF);
    int evt = message->at(0);

    int ch = evt & 0xf;

    evt&= 0xF0;

    int ch_up = ((MidiInControl::channelUp() < 0)
                 ? MidiOutput::standardChannel()
                 : MidiInControl::channelUp()) & 15;

    int ch_down = ((MidiInControl::channelDown() < 0)
                   ? ((MidiInControl::channelUp() < 0)
                      ? MidiOutput::standardChannel()
                      : MidiInControl::channelUp())
                   : MidiInControl::channelDown()) & 0xF;

    int input_chan_up = MidiInControl::inchannelUp();
    int input_chan_down = MidiInControl::inchannelDown();

    if(input_chan_up == -1)
        input_chan_up = ch;

    if(input_chan_down == -1)
        input_chan_down = ch;

    finger_channelUP = input_chan_up;
    finger_channelDOWN = input_chan_down;

    finger_switch_playUP = 0;
    finger_switch_playDOWN = 0;

    if(init_map_key) {

        init_map_key = 0;

        for(int n = 0; n < 128; n++) {

            map_keyUP[n] = 0;
            map_key_stepUP[n] = 0;
            map_key_onUP[n] = 0;
            map_key_offUP[n] = 0;

            map_keyDOWN[n] = 0;
            map_key_stepDOWN[n] = 0;
            map_key_onDOWN[n] = 0;
            map_key_offDOWN[n] = 0;
        }
    }

    int finger_key = message->at(1);

    int note_cut = MidiInControl::note_cut();

    if(MidiInControl::note_duo())
        note_cut = 0;

    if(evt == 0x90) { // KEY ON

// FINGER KEYS DEFINED FOR UP

        if(finger_key == _note_finger_UP) {

            int pressed = _note_finger_on_UP;

            if(_note_finger_pressed_UP)

                pressed = pressed ? 0 : 2;
            else
                pressed = 1;

            if(pressed) {

                _note_finger_on_UP2 = 0;
                _note_finger_pick_on_UP = 0;

                if(_note_finger_old_UP != finger_key)
                    _note_finger_update_UP = 2;
                else
                    _note_finger_update_UP = 1;

                _note_finger_old_UP = finger_key;

            }

            _note_finger_on_UP = pressed;

            message->at(0) = 0; // DISABLE NOTE
            fingerMUX->unlock();
            return 1;
        }

        if(finger_key == _note_finger_UP2) {

            int pressed = _note_finger_on_UP2;

            if(_note_finger_pressed_UP2)

                pressed = pressed ? 0 : 2;
            else
                pressed = 1;

            if(pressed) {

                _note_finger_on_UP = 0;
                _note_finger_pick_on_UP = 0;

                if(_note_finger_old_UP != finger_key)
                    _note_finger_update_UP = 2;
                else
                    _note_finger_update_UP = 1;

                _note_finger_old_UP = finger_key;
            }


            _note_finger_on_UP2 = pressed;

            message->at(0) = 0; // DISABLE NOTE
            fingerMUX->unlock();
            return 1;
        }

        if(finger_key == _note_finger_pick_UP) {

            if(_note_finger_old_UP != finger_key)
                _note_finger_update_UP = 2;
            else
                _note_finger_update_UP = 1;

            _note_finger_old_UP = finger_key;
            _note_finger_pick_on_UP = true;

            message->at(0) = 0; // DISABLE NOTE
            fingerMUX->unlock();
            return 1;
        }

// FINGER KEYS DEFINED FOR DOWN

        if(finger_key == _note_finger_DOWN) {

            if(note_cut > 0) {

                int pressed = _note_finger_on_DOWN;

                if(_note_finger_pressed_DOWN)

                    pressed = pressed ? 0 : 2;
                else
                    pressed = 1;

                if(pressed) {

                    _note_finger_on_DOWN2 = 0;
                    _note_finger_pick_on_DOWN = 0;

                    if(_note_finger_old_DOWN != finger_key)
                        _note_finger_update_DOWN = 2;
                    else
                        _note_finger_update_DOWN = 1;

                    _note_finger_old_DOWN = finger_key;
                }

                _note_finger_on_DOWN = pressed;

            }

            message->at(0) = 0; // DISABLE NOTE
            fingerMUX->unlock();
            return 1;
        }

        if(finger_key == _note_finger_DOWN2) {

            if(note_cut > 0) {

                int pressed = _note_finger_on_DOWN2;

                if(_note_finger_pressed_DOWN2)

                    pressed = pressed ? 0 : 2;
                else
                    pressed = true;

                if(pressed) {

                    _note_finger_pick_on_DOWN = 0;
                    _note_finger_on_DOWN = 0;

                    if(_note_finger_old_DOWN != finger_key)
                        _note_finger_update_DOWN = 2;
                    else
                        _note_finger_update_DOWN = 1;

                    _note_finger_old_DOWN = finger_key;

                }

                _note_finger_on_DOWN2 = pressed;
            }

            message->at(0) = 0; // DISABLE NOTE
            fingerMUX->unlock();
            return 1;
        }

        if(finger_key == _note_finger_pick_DOWN) {

            if(note_cut > 0) {

                if(_note_finger_old_DOWN != finger_key)
                    _note_finger_update_DOWN = 2;
                else
                    _note_finger_update_DOWN = 1;

                _note_finger_old_DOWN = finger_key;
                _note_finger_pick_on_DOWN = 1;
            }

            message->at(0) = 0; // DISABLE NOTE
            fingerMUX->unlock();
            return 1;
        }

        if((evt == 0x80 || evt == 0x90) && ch == 9) {

            fingerMUX->unlock();
            return 0; // ignore drums
        }

        if((evt == 0x80 || evt == 0x90)
            && ((ch != input_chan_up && ch != input_chan_down)
                || (input_chan_down != ch && input_chan_down != input_chan_up && finger_key < note_cut)
                || (input_chan_up != ch && input_chan_down != input_chan_up && finger_key >= note_cut))) {

            fingerMUX->unlock();
            return 1; // skip notes
        }

// GENERIC KEY DIRECT NOTE OFF (FOR OLD CHILD'S KEYS)

        for(int n = 0; n < 128; n++) {

            // UP

            if(finger_key >= note_cut && map_keyUP[n] == (128 | finger_key)) {

                map_keyUP[n] = 0;

                std::vector<unsigned char> message;

                message.clear();
                message.emplace_back((unsigned char) 0x80 + ch_up);
                message.emplace_back((unsigned char) n);
                message.emplace_back((unsigned char) 0);

                finger_token = finger_key;
                MidiInput::receiveMessage(0.0, &message, &finger_token);

            }

            // DOWN

            if(finger_key < note_cut && map_keyDOWN[n] == (128 | finger_key)) {

                map_keyUP[n] = 0;

                std::vector<unsigned char> message;

                message.clear();
                message.emplace_back((unsigned char) 0x80 + ch_down);
                message.emplace_back((unsigned char) n);
                message.emplace_back((unsigned char) 0);

                finger_token = finger_key;
                MidiInput::receiveMessage(0.0, &message, &finger_token);

            }
        }

// GENERIC KEY INDIRECT NOTE ON (MASTER NOTE FROM finger_timer())

        int steps;

        if(finger_key >= note_cut) {

            steps = finger_patternUP[finger_chord_modeUP][1];

            map_key_tempo_stepUP[finger_key] = 0;
            map_key_step_maxUP[finger_key] = steps | (finger_patternUP[finger_chord_modeUP][3] << 4);
            map_key_stepUP[finger_key] = 128;

            map_key_velocityUP[finger_key] = message->at(2);

            map_keyUP[finger_key] = finger_key | 128;

        } else {

            steps = finger_patternDOWN[finger_chord_modeDOWN][1];

            map_key_tempo_stepDOWN[finger_key] = 0;
            map_key_step_maxDOWN[finger_key] = steps | (finger_patternDOWN[finger_chord_modeDOWN][3] << 4);
            map_key_stepDOWN[finger_key] = 128;

            map_key_velocityDOWN[finger_key] = message->at(2);

            map_keyDOWN[finger_key] = finger_key | 128;
        }


        //qDebug("key on %i", finger_key);

// GENERIC KEY DIRECT NOTE ON (MASTER NOTE WITHOUT CHILD)

        if(!finger_enable_UP && finger_key >= note_cut) {

            message->at(0) = 0x90 + ch_up;
            fingerMUX->unlock();
            return 0;
        }

        if(!finger_enable_DOWN && finger_key < note_cut) {

            message->at(0) = 0x90 + ch_down;
            fingerMUX->unlock();
            return 0;
        }

    } else if(evt == 0x80) { // NOTE OFF

        int finger_key_off = message->at(1);

// FINGER KEYS DEFINED FOR UP

        if(finger_key_off == _note_finger_UP) {

            if(!_note_finger_pressed_UP) _note_finger_on_UP = 0;

            message->at(0) = 0; // DISABLE NOTE
            fingerMUX->unlock();
            return 1;
        }

        if(finger_key_off == _note_finger_UP2) {

            if(!_note_finger_pressed_UP2) _note_finger_on_UP2 = 0;

            message->at(0) = 0; // DISABLE NOTE
            fingerMUX->unlock();
            return 1;
        }

        if(finger_key_off == _note_finger_pick_UP) {

            _note_finger_pick_on_UP = 0;

            message->at(0) = 0; // DISABLE NOTE
            fingerMUX->unlock();
            return 1;
        }

// FINGER KEYS DEFINED FOR DOWN

        if(finger_key_off == _note_finger_DOWN) {

            if(!_note_finger_pressed_DOWN) _note_finger_on_DOWN = 0;

            message->at(0) = 0; // DISABLE NOTE
            fingerMUX->unlock();
            return 1;
        }

        if(finger_key_off == _note_finger_DOWN2) {

            if(!_note_finger_pressed_DOWN2) _note_finger_on_DOWN2 = 0;

            message->at(0) = 0; // DISABLE NOTE
            fingerMUX->unlock();
            return 1;
        }

        if(finger_key_off == _note_finger_pick_DOWN) {

            _note_finger_pick_on_DOWN = 0;

            message->at(0) = 0; // DISABLE NOTE
            fingerMUX->unlock();
            return 1;
        }

        if((evt == 0x80 || evt == 0x90) && ch == 9) {
            fingerMUX->unlock();
            return 0; // ignore drums
        }

// GENERIC KEY DIRECT NOTE OFF (FOR OLD CHILD'S KEYS)

        for(int n = 0; n < 128; n++) {

            if(finger_key_off >= note_cut && map_keyUP[n] == (128 | finger_key_off)) {

                map_keyUP[n] = 0;

                std::vector<unsigned char> message;

                message.clear();
                message.emplace_back((unsigned char) 0x80 + ch_up);
                message.emplace_back((unsigned char) n);
                message.emplace_back((unsigned char) 0);

                finger_token = finger_key_off;
                MidiInput::receiveMessage(0.0, &message, &finger_token);

            }

            if(finger_key_off < note_cut && map_keyDOWN[n] == (128 | finger_key_off)) {

                map_keyDOWN[n] = 0;

                std::vector<unsigned char> message;

                message.clear();
                message.emplace_back((unsigned char) 0x80 + ch_down);
                message.emplace_back((unsigned char) n);
                message.emplace_back((unsigned char) 0);

                finger_token = finger_key_off;
                MidiInput::receiveMessage(0.0, &message, &finger_token);

            }
        }

 // RESET STEP COUNTERS

        if(finger_key >= note_cut)
            map_key_stepUP[finger_key_off] = 0;
        else
            map_key_stepDOWN[finger_key_off] = 0;

        //qDebug("key off %i", finger_key_off);

// GENERIC KEY DIRECT NOTE OFF (MASTER NOTE WITHOUT CHILD)

        if(!finger_enable_UP && finger_key >= note_cut) {

            message->at(0) = 0x80 + ch_up; // NOTE OFF
            fingerMUX->unlock();
            return 0;
        }

        if(!finger_enable_DOWN && finger_key < note_cut) {

            message->at(0) = 0x80 + ch_down; // NOTE OFF
            fingerMUX->unlock();
            return 0;
        }

    }

    message->at(0) = 0; // DISABLE NOTE
    fingerMUX->unlock();
    return 1;
}


finger_Thread::finger_Thread() {

}

finger_Thread::~finger_Thread() {

}

#ifdef __WINDOWS_MM__
#include <windows.h>
#endif

void finger_Thread::run() {

    QTime time;
    int last_time = time.msecsSinceStartOfDay();
    int current_time;
    int counter = 0;

    while(1) {

        FingerPatternDialog::finger_timer();

        if(counter == 0 && MidiInControl::key_live) { // used for no sleep the computer

#ifdef __WINDOWS_MM__
            SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
#endif
            MidiInControl::key_live = 0;
        }

        counter++;
        counter&= 127;

        current_time = time.msecsSinceStartOfDay();

        int time = current_time - last_time;

        if(time > 15) {
            time = time - 15;
            if(time > 5) time = 5; // top correction
        } else {
            time = 15 - time;
            if(time > 5) time = 0; // top correction
            time = -time;
        }

        last_time = current_time;

        msleep(15 - time); // 1000/64 ~15 ms
    }

}
