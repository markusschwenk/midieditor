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

FingerPatternDialog* FingerPatternWin = NULL;

int FingerPatternDialog::pairdev_out = 0;

int finger_token[2] = {0, 0};

// RESET FINGER
bool _note_finger_disabled[MAX_INPUT_PAIR] = {false, false};

static int finger_switch_playUP[MAX_INPUT_PAIR] = {0, 0};
static int finger_switch_playDOWN[MAX_INPUT_PAIR] = {0, 0};

static int update_tick_counterUP[MAX_INPUT_PAIR] = {0, 0};
static int update_tick_counterDOWN[MAX_INPUT_PAIR] = {0, 0};
static int _note_finger_update_UP[MAX_INPUT_PAIR] = {0, 0};
static int _note_finger_update_DOWN[MAX_INPUT_PAIR] = {0, 0};
static int _note_finger_old_UP[MAX_INPUT_PAIR] = {-1, -1};
static int _note_finger_old_DOWN[MAX_INPUT_PAIR] = {-1, -1};

static bool _note_finger_pressed_UP[MAX_INPUT_PAIR] = {false, false};
static int _note_finger_on_UP[MAX_INPUT_PAIR] = {0, 0};
static int _note_finger_pattern_UP[MAX_INPUT_PAIR] = {0, 0};
static bool _note_finger_pick_on_UP[MAX_INPUT_PAIR] = {false, false};
static bool _note_finger_pressed_UP2[MAX_INPUT_PAIR] = {false, false};
static int _note_finger_on_UP2[MAX_INPUT_PAIR] = {0, 0};
static int _note_finger_pattern_UP2[MAX_INPUT_PAIR] = {0, 0};
static bool _note_finger_pressed_UP3[MAX_INPUT_PAIR] = {false, false};
static int _note_finger_on_UP3[MAX_INPUT_PAIR] = {0, 0};
static int _note_finger_pattern_UP3[MAX_INPUT_PAIR] = {0, 0};

static bool _note_finger_pressed_DOWN[MAX_INPUT_PAIR] = {false, false};
static int _note_finger_on_DOWN[MAX_INPUT_PAIR] = {0, 0};
static int _note_finger_pattern_DOWN[MAX_INPUT_PAIR] = {0, 0};
static int _note_finger_pick_on_DOWN[MAX_INPUT_PAIR] = {0, 0};
static bool _note_finger_pressed_DOWN2[MAX_INPUT_PAIR] = {false, false};
static int _note_finger_on_DOWN2[MAX_INPUT_PAIR] = {0, 0};
static int _note_finger_pattern_DOWN2[MAX_INPUT_PAIR] = {0, 0};
static bool _note_finger_pressed_DOWN3[MAX_INPUT_PAIR] = {false, false};
static int _note_finger_on_DOWN3[MAX_INPUT_PAIR] = {0, 0};
static int _note_finger_pattern_DOWN3[MAX_INPUT_PAIR] = {0, 0};


#define MAX_FINGER_BASIC 5
#define MAX_FINGER_PATTERN (MAX_FINGER_BASIC + 16)
#define MAX_FINGER_NOTES 15

enum finger_pattern {
    FTYPE = 0,
    FSTEPS,
    FTEMPO,
    FREPEAT,
    FNOTE1
};

#define FLAG_TEMPOX15 8192
#define FLAG_TEMPOX20 16384
#define FLAG_TEMPO (FLAG_TEMPOX15 | FLAG_TEMPOX20)

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
static unsigned char finger_patternUP_ini[MAX_FINGER_PATTERN][MAX_FINGER_NOTES + 4] = {
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
static unsigned char finger_patternDOWN_ini[MAX_FINGER_PATTERN][MAX_FINGER_NOTES + 4] = {
    {0, 2, 16, 0, 2, 3, 4, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 3, 16, 0, 4, 5, 6, 8, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {2, 3, 16, 0, 4, 5, 6, 8, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {3, 5, 16, 0, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {4, 5, 16, 0, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

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
    {0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};


static unsigned char finger_patternUP[MAX_INPUT_PAIR][MAX_FINGER_PATTERN][MAX_FINGER_NOTES + 4];
static unsigned char finger_patternDOWN[MAX_INPUT_PAIR][MAX_FINGER_PATTERN][MAX_FINGER_NOTES + 4];

static QMutex *fingerMUX = NULL;

static int max_tick_counterUP[MAX_INPUT_PAIR] = {6, 6};
static int max_tick_counterDOWN[MAX_INPUT_PAIR] = {6, 6};

static int finger_channelUP[MAX_INPUT_PAIR] = {0, 0};
static int finger_channelDOWN[MAX_INPUT_PAIR] = {0, 0};

static int finger_key = 48;
static int finger_base_note = 48;

static int finger_repeatUP[MAX_INPUT_PAIR] = {0, 0};
static int finger_chord_modeUP[MAX_INPUT_PAIR] = {1, 1}; // current pattern
static int finger_repeatDOWN[MAX_INPUT_PAIR] = {0, 0};
static int finger_chord_modeDOWN[MAX_INPUT_PAIR] = {1, 1}; // current pattern

static int finger_chord_modeUPEdit[MAX_INPUT_PAIR] = {1, 1}; // current pattern
static int finger_chord_modeDOWNEdit[MAX_INPUT_PAIR] = {1, 1}; // current pattern


////////////////////////////////////////////

// finger timer variables

static unsigned char map_key_stepUP[MAX_INPUT_PAIR][128];
static unsigned short map_key_step_maxUP[MAX_INPUT_PAIR][128];
static unsigned char map_key_tempo_stepUP[MAX_INPUT_PAIR][128];

static unsigned char map_keyUP[MAX_INPUT_PAIR][128];
static unsigned char map_key_offUP[MAX_INPUT_PAIR][128];
static unsigned char map_key_onUP[MAX_INPUT_PAIR][128];
static unsigned char map_key_velocityUP[MAX_INPUT_PAIR][128];

static unsigned char map_key_stepDOWN[MAX_INPUT_PAIR][128];
static unsigned short map_key_step_maxDOWN[MAX_INPUT_PAIR][128];
static unsigned char map_key_tempo_stepDOWN[MAX_INPUT_PAIR][128];

static unsigned char map_keyDOWN[MAX_INPUT_PAIR][128];
static unsigned char map_key_offDOWN[MAX_INPUT_PAIR][128];
static unsigned char map_key_onDOWN[MAX_INPUT_PAIR][128];
static unsigned char map_key_velocityDOWN[MAX_INPUT_PAIR][128];

static int init_map_key = 1;

int finger_enable_UP[MAX_INPUT_PAIR] = {0, 0};
int finger_enable_DOWN[MAX_INPUT_PAIR] = {0, 0};

/////////////////////////////

static QByteArray Packfinger(int pairdev_in) {

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

            a.append((unsigned char) finger_patternUP[pairdev_in][n][m]);
            a.append((unsigned char) finger_patternDOWN[pairdev_in][n][m]);
        }

    }

    a.append((char) 255); // EOF

    return a;
}

static int Unpackfinger(int pairdev_in, QByteArray a) {

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

            if(index1 < MAX_FINGER_BASIC && m == FSTEPS) { // fixed values for basic
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

            if(index1 < MAX_FINGER_BASIC && m == FREPEAT) // fixed values for basic
                a[i] = 0;

            finger_patternUP[pairdev_in][index1][m] = a[i];
            i++;
            if(i >= a.length()) return -104;

            if(index1 < MAX_FINGER_BASIC && m == FSTEPS) { // fixed values for basic
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

            if(index1 < MAX_FINGER_BASIC && m == FREPEAT) // fixed values for basic
                a[i] = 0;

            finger_patternDOWN[pairdev_in][index1][m] = a[i];
            i++;
            if(i >= a.length()) return -104;
        }

    }

    return 0;
}

int FingerPatternDialog::_track_index = 0;

void FingerPatternDialog::save_custom(int pairdev_in, bool down) {

    QString dir = QDir::homePath() + "/Midieditor/fake.cfinger";

    if(_settings) {

        dir = QString::fromUtf8(_settings->value("FingerPattern/FINGER_save_path", dir.toUtf8()).toByteArray());
        dir = QFileInfo(dir).absoluteDir().path() + "/";
    }

    QString newPath = QFileDialog::getSaveFileName(this, "Save Finger Pattern Custom file",
                                                   dir, "Finger Pattern Custom files (*.cfinger)");

    if (newPath == "") {
        return;
    }

    unsigned char finger_pattern[MAX_FINGER_NOTES + 4];

    if(!down) {

        memcpy(&finger_pattern[0], &finger_patternUP[pairdev_in][finger_chord_modeUPEdit[pairdev_in]][FTYPE], 19);

        if(finger_chord_modeUPEdit[pairdev_in] < MAX_FINGER_BASIC) { // fixed values for basic
            int val = 0;
            switch(finger_chord_modeUPEdit[pairdev_in]) {
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


            finger_pattern[1] = val;
            finger_pattern[3] = 0;
        }

    } else {

        memcpy(&finger_pattern[0], &finger_patternDOWN[pairdev_in][finger_chord_modeDOWNEdit[pairdev_in]][FTYPE], 19);

        if(finger_chord_modeDOWNEdit[pairdev_in] < MAX_FINGER_BASIC) { // fixed values for basic
            int val = 0;
            switch(finger_chord_modeDOWNEdit[pairdev_in]) {
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

            finger_pattern[1] = val;
            finger_pattern[3] = 0;
        }


    }

    QByteArray a;

    a.append('C');
    a.append('F');
    a.append('R');
    a.append((char) 1); // version 1

    a.append((char) (MAX_FINGER_NOTES + 4));

    for(int m = 0; m < (MAX_FINGER_NOTES + 4); m++) {

        if(!down)
            a.append((unsigned char) finger_pattern[m]);
        else
            a.append((unsigned char) finger_pattern[m]);
    }

    a.append((char) 255); // EOF

    QFile *file = new QFile(newPath);

    if(file) {

        if(file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {

            if(_settings) {
                _settings->setValue("FingerPattern/FINGER_save_path", newPath.toUtf8());
            }

            file->write(a);

        }

        delete file;
    }

}


void FingerPatternDialog::load_custom(int pairdev_in, int custom, bool down) {

    QString dir = QDir::homePath() + "/Midieditor/fake.cfinger";

    if(_settings) {

        dir = QString::fromUtf8(_settings->value("FingerPattern/FINGER_save_path", dir.toUtf8()).toByteArray());
        dir = QFileInfo(dir).absoluteDir().path() + "/";
    }

    QString newPath = QFileDialog::getOpenFileName(this, "Load Finger Pattern Custom file",
                                                   dir, "Finger Pattern Custom files (*.cfinger)");

    if (newPath == "") {
        return;
    }

    QByteArray a;

    QFile *file = new QFile(newPath);

    if(file) {
        if(file->open(QIODevice::ReadOnly)) {

            if(_settings) {
                _settings->setValue("FingerPattern/FINGER_save_path", newPath.toUtf8());
            }

            a = file->read(0x10000);

            int i = 5;

            if(a.length() != (5 + MAX_FINGER_NOTES + 4 + 1)) return;

            if(a[0] != 'C' && a[1] != 'F' && a[2] != 'R' && a[3] != (char) 1) return;

            if(a[4] != (char) (MAX_FINGER_NOTES + 4)) return;

            int index1 = MAX_FINGER_BASIC + custom;

            for(int m = 0; m < (MAX_FINGER_NOTES + 4); m++) {

                if(!down)
                    finger_patternUP[pairdev_in][index1][m] = a[i];
                else
                    finger_patternDOWN[pairdev_in][index1][m] = a[i];
                i++;
            }

            if(!down) {

                comboBoxChordUP->blockSignals(true);

                comboBoxChordUP->clear();
                comboBoxChordUP->addItem(stringTypes[0], -1);
                comboBoxChordUP->addItem(stringTypes[1], -2);
                comboBoxChordUP->addItem(stringTypes[2], -3);
                comboBoxChordUP->addItem(stringTypes[3], -4);
                comboBoxChordUP->addItem(stringTypes[4], -5);

                for(int n = 0; n < 16; n++) {
                    if(finger_patternUP[pairdev_in][n + MAX_FINGER_BASIC][FSTEPS]) {

                        comboBoxChordUP->addItem("CUSTOM #" + QString::number(n + 1)
                                                     + " " + stringTypes[finger_patternUP[pairdev_in][n + MAX_FINGER_BASIC][FTYPE]], n);
                    }
                }

                comboBoxChordUP->blockSignals(false);

                finger_chord_modeUP[pairdev_in] = 5 + custom;
                finger_chord_modeUPEdit[pairdev_in] =  5 + custom;

                for(int n = 0; n < comboBoxChordUP->count(); n++) {
                    if(comboBoxChordUP->itemData(n).toInt() == custom) {

                        comboBoxChordUP->currentIndexChanged(n);
                        comboBoxChordUP->setCurrentIndex(n);
                        break;
                    }
                }

            } else {

                comboBoxChordDOWN->blockSignals(true);

                comboBoxChordDOWN->clear();
                comboBoxChordDOWN->addItem(stringTypes[0], -1);
                comboBoxChordDOWN->addItem(stringTypes[1], -2);
                comboBoxChordDOWN->addItem(stringTypes[2], -3);
                comboBoxChordDOWN->addItem(stringTypes[3], -4);
                comboBoxChordDOWN->addItem(stringTypes[4], -5);

                for(int n = 0; n < 16; n++) {

                    if(finger_patternDOWN[pairdev_in][n + MAX_FINGER_BASIC][FSTEPS]) {

                        comboBoxChordDOWN->addItem("CUSTOM #" + QString::number(n + 1)
                                                       + " " + stringTypes[finger_patternDOWN[pairdev_in][n + MAX_FINGER_BASIC][FTYPE]], n);
                    }
                }

                comboBoxChordDOWN->blockSignals(false);

                finger_chord_modeDOWN[pairdev_in] = 5 + custom;
                finger_chord_modeDOWNEdit[pairdev_in] = 5 + custom;

                for(int n = 0; n < comboBoxChordDOWN->count(); n++) {
                    if(comboBoxChordDOWN->itemData(n).toInt() == custom) {

                        comboBoxChordDOWN->currentIndexChanged(n);
                        comboBoxChordDOWN->setCurrentIndex(n);
                        break;
                    }
                }
            }


        }

        delete file;
    }

}


void FingerPatternDialog::save() {

    QString dir = QDir::homePath() + "/Midieditor/fake.finger";

    if(_settings) {

        dir = QString::fromUtf8(_settings->value("FingerPattern/FINGER_save_path", dir.toUtf8()).toByteArray());
        dir = QFileInfo(dir).absoluteDir().path() + "/";
    }

    QString newPath = QFileDialog::getSaveFileName(this, "Save Finger Pattern file",
        dir, "Finger Pattern files (*.finger)");

    if (newPath == "") {
        return;
    }

    QByteArray a = Packfinger(pairdev_out);

    QFile *file = new QFile(newPath);

    if(file) {

        if(file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {

            if(_settings) {
                _settings->setValue("FingerPattern/FINGER_save_path", newPath.toUtf8());
            }

            file->write(a);

        }

        delete file;
    }

}

void FingerPatternDialog::load() {

    QString dir = QDir::homePath() + "/Midieditor";

    if(_settings) {

        dir = QString::fromUtf8(_settings->value("FingerPattern/FINGER_save_path", dir.toUtf8()).toByteArray());
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
                _settings->setValue("FingerPattern/FINGER_save_path", newPath.toUtf8());
            }

            a = file->read(0x10000);

            Unpackfinger(pairdev_out, a);

            comboBoxChordUP->blockSignals(true);

            comboBoxChordUP->clear();
            comboBoxChordUP->addItem(stringTypes[0], -1);
            comboBoxChordUP->addItem(stringTypes[1], -2);
            comboBoxChordUP->addItem(stringTypes[2], -3);
            comboBoxChordUP->addItem(stringTypes[3], -4);
            comboBoxChordUP->addItem(stringTypes[4], -5);

            for(int n = 0; n < 16; n++) {
                if(finger_patternUP[pairdev_out][n + MAX_FINGER_BASIC][FSTEPS]) {

                    comboBoxChordUP->addItem("CUSTOM #" + QString::number(n + 1)
                                             + " " + stringTypes[finger_patternUP[pairdev_out][n + MAX_FINGER_BASIC][FTYPE]], n);
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
                if(finger_patternDOWN[pairdev_out][n + MAX_FINGER_BASIC][FSTEPS]) {

                    comboBoxChordDOWN->addItem("CUSTOM #" + QString::number(n + 1)
                                             + " " + stringTypes[finger_patternDOWN[pairdev_out][n + MAX_FINGER_BASIC][FTYPE]], n);
                }
            }

            comboBoxChordDOWN->blockSignals(false);

            finger_chord_modeUP[pairdev_out] = 0;
            finger_chord_modeDOWN[pairdev_out] = 0;
            finger_chord_modeUPEdit[pairdev_out] = 0;
            finger_chord_modeDOWNEdit[pairdev_out] = 0;

            comboBoxChordUP->currentIndexChanged(finger_chord_modeUPEdit[pairdev_out]);
            comboBoxChordDOWN->currentIndexChanged(finger_chord_modeDOWNEdit[pairdev_out]);
            comboBoxChordUP->setCurrentIndex(finger_chord_modeUPEdit[pairdev_out]);
            comboBoxChordDOWN->setCurrentIndex(finger_chord_modeDOWNEdit[pairdev_out]);

        }

        delete file;
    }

}

void FingerPatternDialog::reject() {

    if(_settings) {
        _settings->setValue("FingerPattern/FINGER_pattern_save" + (pairdev_out ? QString::number(pairdev_out) : ""), Packfinger(pairdev_out));
    }

    if(FEdit)
        delete FEdit;
    FEdit = NULL;

    hide();
}

QString FingerPatternDialog::setButtonNotePattern(int type, int val) {

    int gridH = 15;
    int gridM = 5;
    int val2 = val & 31;

    if(type == 0) {
        gridH = 6;
        gridM = 2;

        if(val2 >= 12) {
            if(val2 > 12) {
               if(val & 32)
                   return "$##";

               return "*##";
            }

            return "###";
        }

    }

    if(type == 1 || type == 2) {
        gridH = 12;
        gridM = 4;

        if(val2 >= 24) {
            if(val2 > 24) {
               if(val & 32)
                   return "$##";

               return "*##";
            }

            return "###";
        }
    } else {

        if(val2 >= 30) {
            if(val2 > 30) {
               if(val & 32)
                   return "$##";

               return "*##";
            }

            return "###";
        }

    }

    QString text;

    if(val2 >= gridH) {
        if(val & 32)
            text += "$";

        text += "*";
    }

    int octave = (val2 % gridH)/gridM;

    if(octave == 0)
        text += "-";
    else if(octave == 2)
        text += "+";

    if(type == 0) {

        switch(val2 % gridM) {
            case 0: {
                text += "1";
                break;
            }
            case 1: {
                text += "5";
                break;
            }
        }

    } else if(type == 1 || type == 2) {

        switch(val2 % gridM) {
            case 0: {
                text += "1";
                break;
            }
            case 1: {
                text += "3";
                break;
            }
            case 2: {
                text += "5";
                break;
            }
            case 3: {
                text += "7";
                break;
            }

        }

    } else {

        switch(val2 % gridM) {
            case 0: {
                text += "1";
                break;
            }
            case 1: {
                text += "2";
                break;
            }
            case 2: {
                text += "3";
                break;
            }
            case 3: {
                text += "5";
                break;
            }
            case 4: {
                text += "6";
                break;
            }

        }

    }

    if(val & 64)
        text+= "#";

    return text;
}


void FingerEdit::FingerUpdate(int mode) {

    int xx = 8;
    int yy = 40;

    int gridH = 15;
    int gridM = 5;

    if(mode == 0) {
        gridH = 6;
        gridM = 2;
    }

    if(mode == 1 || mode == 2) {
        gridH = 12;
        gridM = 4;
    }

    int x = xx;

    for(int n = 0; n < 16; n++) {

        int w = 30, h = 18;

        if(n > 0) {
            if(tempo[n - 1] == 1) {
                w = 45;
                pa[n - 1]->setText("T" + QString::number((n)) + " (1.5)");
            } else if(tempo[n - 1]) {
                w = 60;
                pa[n - 1]->setText("T" + QString::number((n)) + " (2.0)");
            } else
                pa[n - 1]->setText("T" + QString::number((n)));

            pa[n - 1]->setGeometry(QRect(x, yy + (h + 2) * -1, w, h));

        }

        int m = 0;

        for(; m < gridH; m++) {

            int mat = m + n * 15;

            pb[mat]->setGeometry(QRect(x, yy + (h + 2) * m, w, h));

            if(n == 0) {

                if(mode == 0) {
                    QString text;
                    switch(m) {
                        case 0: {
                            text = "+5";
                            break;
                        }
                        case 1: {
                            text = "+1";
                            break;
                        }
                        case 2: {
                            text = "5";
                            break;
                        }
                        case 3: {
                            text = "1";
                            break;
                        }
                        case 4: {
                            text = "-5";
                            break;
                        }
                        case 5: {
                            text = "-1";
                            break;
                        }
                    }

                    pb[mat]->setText(text);
                } else if(mode == 1 || mode == 2) {
                    QString text;
                    switch(m) {
                        case 0: {
                            text = "+7";
                            break;
                        }
                        case 1: {
                            text = "+5";
                            break;
                        }
                        case 2: {
                            text = "+3";
                            break;
                        }
                        case 3: {
                            text = "+1";
                            break;
                        }
                        case 4: {
                            text = "7";
                            break;
                        }
                        case 5: {
                            text = "5";
                            break;
                        }
                        case 6: {
                            text = "3";
                            break;
                        }
                        case 7: {
                            text = "1";
                            break;
                        }
                        case 8: {
                            text = "-7";
                            break;
                        }
                        case 9: {
                            text = "-5";
                            break;
                        }
                        case 10: {
                            text = "-3";
                            break;
                        }
                        case 11: {
                            text = "-1";
                            break;
                        }
                    }

                    pb[mat]->setText(text);

                } else {
                    int dat = 4 - (mat % gridM);
                    if(m < gridM)
                        pb[mat]->setText("+" + QString::number(dat + 1 + (dat >= 3)));
                    else if(m >= gridM * 2)
                        pb[mat]->setText("-" + QString::number(dat + 1 + (dat >= 3)));
                    else
                        pb[mat]->setText(QString::number(dat + 1 + (dat >= 3)));
                }

            } else {
                if(n == 1) {
                    if(m < gridM || m >= (2 * gridM))
                        pb[mat]->setDisabled(true);
                }

                pb[mat]->setCheckable(true);

                if((data[n - 1] & 127) == (mat % 15) + 1) {

                    pb[mat]->setChecked(true);
                }

                if((data[n - 1] & 128) && pb[mat]->isChecked())
                    pb[mat]->setText("#");
                else
                    pb[mat]->setText("");


            }
        }

        x+= w + 2;

        for(; m < 15; m++) {
            int mat = m + n * 15;
            if(pb[mat])
                pb[mat]->setVisible(false);
        }
    }


    update();

}

static QString szero = QString::fromUtf8("QPushButton {color: black; background-color: #80809f;} \n"
               "QPushButton:checked {background-color: #007080; border: 3px solid #00506f; border-style: outset;} \n"
               "QPushButton:!checked {color: black; background-color: #80809f; border: 3px solid #50506f; border-style: outset;} \n"
               "QPushButton:disabled {color: black; background-color: #4080809f; border: 3px solid #4050506f; border-style: outset;} \n");

static QString sone = QString::fromUtf8("QPushButton {color: black; background-color: #c0c0df;} \n"
               "QPushButton:checked {background-color: #00e0f0; border: 3px solid #00a0bf; border-style: outset;} \n"
               "QPushButton:!checked {color: black; background-color: #c0c0df; border: 3px solid #8080cf; border-style: outset;} \n"
               "QPushButton:disabled {color: black; background-color: #40c0c0df; border: 3px solid #4050506f; border-style: outset;} \n");

static QString sgroup = QString::fromUtf8(
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
        "QGroupBox QToolTip {color: black;} \n");

FingerEdit::FingerEdit(QWidget* parent,  int pairdev_in, int zone) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint) {

    QDialog *FingerDialog = this;
    _parent = parent;
    FingerPatternDialog *finG = ((FingerPatternDialog *) _parent);

    pianoEvent = NULL;

    int mode = 0;

    int edit = 0;
    int steps = 15;

    int gridH = 15;
    int gridM = 5;

    if(!zone) {

        edit = finger_chord_modeUPEdit[pairdev_in];
        mode = finger_patternUP[pairdev_in][edit][FTYPE];
        steps = finger_patternUP[pairdev_in][edit][FSTEPS];

        if(mode == 0) {
            gridH = 6;
            gridM = 2;
        }

        if(mode == 1 || mode == 2) {
            gridH = 12;
            gridM = 4;
        }

        for(int n = 0; n < 15; n++) {
            int val = finger_patternUP[pairdev_in][edit][FNOTE1 + n];
            int val2 = val & 31;
            int res = 0;

            if(mode == 0) {

                if(val2 >= 12) {
                    res = 0;
                    tempo[n] = (val2 > 12) ? ((val & 32) ? 1 : 2) : 0;

                } else {
                    res =  (2 - ((val2 % gridH)/gridM)) * gridM + gridM - (val2 % gridM);
                    tempo[n] = (val2 >= gridH) ? ((val & 32) ? 1 : 2) : 0;

                }

            } else if(mode == 1 || mode == 2) {

                if(val2 >= 24) {
                    res = 0;
                    tempo[n] = (val2 > 24) ? ((val & 32) ? 1 : 2) : 0;
                } else {
                    res = (2 - ((val2 % gridH)/gridM)) * gridM + gridM - (val2 % gridM);
                    tempo[n] = (val2 >= gridH) ? ((val & 32) ? 1 : 2) : 0;
                }

            } else if(mode == 3 || mode == 4) {

                if(val2 >= 30) {
                    res = 0;
                    tempo[n] = (val2 > 30) ? ((val & 32) ? 1 : 2) : 0;

                } else {
                    res = (2 - ((val2 % gridH)/gridM)) * gridM + gridM - (val2 % gridM);
                    tempo[n] = (val2 >= gridH) ? ((val & 32) ? 1 : 2) : 0;
                }

            }

            if(val & 64)
                res|= 128;

            data[n] = res;
        }

    } else {

        edit = finger_chord_modeDOWNEdit[pairdev_in];
        mode = finger_patternDOWN[pairdev_in][edit][FTYPE];
        steps = finger_patternDOWN[pairdev_in][edit][FSTEPS];

        if(mode == 0) {
            gridH = 6;
            gridM = 2;
        }

        if(mode == 1 || mode == 2) {
            gridH = 12;
            gridM = 4;
        }

        for(int n = 0; n < 15; n++) {
            int val = finger_patternDOWN[pairdev_in][edit][FNOTE1 + n];
            int val2 = val & 31;
            int res = 0;

            if(mode == 0) {

                if(val2 >= 12) {
                    res = 0;
                    tempo[n] = (val2 > 12) ? ((val & 32) ? 1 : 2) : 0;

                } else {
                    res =  (2 - ((val2 % gridH)/gridM)) * gridM + gridM - (val2 % gridM);
                    tempo[n] = (val2 >= gridH) ? ((val & 32) ? 1 : 2) : 0;

                }

            } else if(mode == 1 || mode == 2) {

                if(val2 >= 24) {
                    res = 0;
                    tempo[n] = (val2 > 24) ? ((val & 32) ? 1 : 2) : 0;
                } else {
                    res = (2 - ((val2 % gridH)/gridM)) * gridM + gridM - (val2 % gridM);
                    tempo[n] = (val2 >= gridH) ? ((val & 32) ? 1 : 2) : 0;
                }

            } else if(mode == 3 || mode == 4) {

                if(val2 >= 30) {
                    res = 0;
                    tempo[n] = (val2 > 30) ? ((val & 32) ? 1 : 2) : 0;

                } else {
                    res = (2 - ((val2 % gridH)/gridM)) * gridM + gridM - (val2 % gridM);
                    tempo[n] = (val2 >= gridH) ? ((val & 32) ? 1 : 2) : 0;
                }

            }

            if(val & 64)
                res|= 128;

            data[n] = res;
        }

    }

    int gheight = gridH * 20 + 64 - 12;

    if (FingerDialog->objectName().isEmpty())
        FingerDialog->setObjectName(QString::fromUtf8("FingerDialog"));
    FingerDialog->setFixedSize(32 + 62 * 15 + 32 + 16, 20 + gheight + 200 + 16);
    FingerDialog->setStyleSheet(QString::fromUtf8("color: white; background-color: #808080; \n"));
    FingerDialog->setWindowTitle("Finger Pattern Edit (Devices " + QString::number(pairdev_in * 2) + "-"
                                         + QString::number(pairdev_in * 2 + 1) + ")");

    QFont font;
    font.setPointSize(8);

    group1 = new QGroupBox(this);
    group1->setObjectName(QString::fromUtf8("group1"));
    group1->setFlat(true);
    group1->setGeometry(QRect(16, 10, 62 * 15 + 32 + 16, gheight));
    //group1->setFlat(false);
    group1->setCheckable(false);

    if(!zone)
        group1->setTitle("Pattern Note UP Channel");
    else
        group1->setTitle("Pattern Note DOWN Channel");

    group1->setAlignment(Qt::AlignCenter);
    group1->setStyleSheet(sgroup);

    group2 = new QGroupBox(this);
    group2->setObjectName(QString::fromUtf8("group2"));
    group2->setFlat(true);
    group2->setGeometry(QRect(16, 20 + gheight, /*62 * 15 + 32 + 16*/ 470, 200));
    group2->setCheckable(false);
    group2->setTitle("Settings");
    group2->setAlignment(Qt::AlignCenter);
    group2->setStyleSheet(sgroup);

    group3 = new QGroupBox(this);
    group3->setObjectName(QString::fromUtf8("group3"));
    group3->setFlat(true);
    group3->setGeometry(QRect(32 + 470, 20 + gheight, 62 * 15 + 32 - 470, 200));
    group3->setCheckable(false);
    group3->setTitle("Play");
    group3->setAlignment(Qt::AlignCenter);
    group3->setStyleSheet(sgroup);

    int xx = 8;
    int yy = 32;
    int w = 30, h = 18;

    for(int n = 0; n < 16; n++) {

        if(n > 0) {
            //tempo[n - 1] = 0;
            pa[n - 1] = new QPushButtonE(group1, 1);
            pa[n - 1]->setObjectName(QString::fromUtf8("pa") + QString::number(n));
            pa[n - 1]->setGeometry(QRect(xx + (w + 2) * (n), yy + (h + 2) * -1, w, h));
            pa[n - 1]->setFont(font);
            pa[n - 1]->setText("T" + QString::number((n)));
            pa[n - 1]->setChecked(false);

            pa[n - 1]->setStyleSheet(QString::fromUtf8(
                "QPushButton {color: black; background-color: #c0c0c0;} \n"));

            TOOLTIP(pa[n - 1], "Tempo column.\nClick to change the note tempo.");

            connect(pa[n - 1], QOverload<bool>::of(&QPushButtonE::clicked), [=](bool)
            {
                pa[n - 1]->setChecked(false);

                tempo[n - 1]++;
                if(tempo[n - 1] > 2)
                    tempo[n - 1] = 0;

                // set finger

                int val = data[n - 1];
                int val2 = (val & 127) - 1;
                int res = 0;

                if(val == 0) {

                    if(mode == 0) {

                        if(val == 0) {
                           res = 12;
                           if(tempo[n - 1] == 1)
                               res+= 33;
                           else if(tempo[n - 1] == 2)
                               res++;

                        }


                    } else if(mode == 1 || mode == 2) {

                            if(val == 0) {
                               res = 24;
                               if(tempo[n - 1] == 1)
                                   res+= 33;
                               else if(tempo[n - 1] == 2)
                                   res++;
                            }


                    } else if(mode == 3 || mode == 4) {

                            if(val == 0) {
                               res = 30;
                               if(tempo[n - 1] == 1)
                                   res+= 33;
                               else if(tempo[n - 1] == 2)
                                   res++;
                            }

                    }
                } else {
                    res =  (2 - ((val2 % gridH)/gridM)) * gridM + gridM - (val2 % gridM) - 1;
                    if(val & 128)
                        res|= 64;
                    if(tempo[n - 1] == 1)
                        res+= 32 + gridH;
                    else if(tempo[n - 1] == 2)
                        res+= gridH;
                }

                if(!zone) {

                    finger_patternUP[pairdev_in][edit][FNOTE1 + n - 1] = res;            

                    finG->NoteUPButtons[n - 1]->setText(finG->setButtonNotePattern(finger_patternUP[pairdev_in][edit][FTYPE],
                                                    finger_patternUP[pairdev_in][edit][FNOTE1 + n - 1]));
                } else  {

                    finger_patternDOWN[pairdev_in][edit][FNOTE1 + n - 1] = res;

                    finG->NoteDOWNButtons[n - 1]->setText(finG->setButtonNotePattern(finger_patternDOWN[pairdev_in][edit][FTYPE],
                                                    finger_patternDOWN[pairdev_in][edit][FNOTE1 + n - 1]));
                }

                FingerUpdate(mode);

            });
        }

        for(int m = 0; m < 15; m++) {

            int mat = m + n * 15;

            pb[mat] = new QPushButtonE(group1, 1);
            pb[mat]->setObjectName(QString::fromUtf8("pb") + QString::number(mat));
            pb[mat]->setGeometry(QRect(xx + (w + 2) * n, yy + (h + 2) * m, w, h));
            pb[mat]->setFont(font);

            if(n == 0)
                TOOLTIP(pb[mat], "This is the arpeggio note.")
            else
                TOOLTIP(pb[mat], "Press left button to switch between silent, note or sharp note.\n\nPress right button to play.\n")

            if(n > 0) {

                pb[mat]->setCheckable(true);

                if((data[n - 1] & 127) == (mat % 15) + 1) {

                    pb[mat]->setChecked(true);
                }

                if((data[n - 1] & 128) && pb[mat]->isChecked())
                    pb[mat]->setText("#");
                else
                    pb[mat]->setText("");


                if(n > steps) pb[mat]->setStyleSheet(szero);

                else pb[mat]->setStyleSheet(sone);

                connect(pb[mat], QOverload<bool>::of(&QPushButtonE::clicked), [=](bool)
                {
                    if(!data[n - 1] || n < 2) {
                        data[n - 1] = (mat % 15) + 1;
                    } else if(data[n - 1] & 128)

                        if((data[n - 1] & 127) == ((mat % 15) + 1)) {
                            data[n - 1] = 0;
                        } else
                            data[n - 1] = (mat % 15) + 1;
                    else {

                        if(data[n - 1] == (mat % 15) + 1) {
                            if(n < 2)
                                data[n - 1] = 0;
                            else
                                data[n - 1] |= 128;
                        } else
                             data[n - 1] = (mat % 15) + 1;
                    }


                    for(int nn = 0; nn < 15; nn++) {
                        int ind = nn + (mat/15) * 15;
                        if(nn == (mat % 15)) {

                            if(data[n - 1] == 0)
                                pb[ind]->setChecked(false);
                            else
                                pb[ind]->setChecked(true);
                        } else
                            pb[ind]->setChecked(false);

                    }


                    // set finger

                    int val = data[n - 1];
                    int val2 = (val & 127) - 1;
                    int res = 0;

                    if(val == 0) {

                        if(mode == 0) {

                            if(val == 0) {
                               res = 12;
                               if(tempo[n - 1] == 1)
                                   res+= 33;
                               else if(tempo[n - 1] == 2)
                                   res++;
                            }


                        } else if(mode == 1 || mode == 2) {

                                if(val == 0) {
                                   res = 24;
                                   if(tempo[n - 1] == 1)
                                       res+= 33;
                                   else if(tempo[n - 1] == 2)
                                       res++;
                                }


                        } else if(mode == 3 || mode == 4) {

                                if(val == 0) {
                                   res = 30;
                                   if(tempo[n - 1] == 1)
                                       res+= 33;
                                   else if(tempo[n - 1] == 2)
                                       res++;
                                }

                        }
                    } else {
                        res =  (2 - ((val2 % gridH)/gridM)) * gridM + gridM - (val2 % gridM) - 1;
                        if(val & 128)
                            res|= 64;
                        if(tempo[n - 1] == 1)
                            res+= 32 + gridH;
                        else if(tempo[n - 1] == 2)
                            res+= gridH;
                    }

                    int note_disp = 0;

                    if(!zone) {

                        finger_patternUP[pairdev_in][edit][FNOTE1 + n - 1] = res;
                        note_disp = finger_patternUP[pairdev_in][edit][FNOTE1];

                        finG->NoteUPButtons[n - 1]->setText(finG->setButtonNotePattern(finger_patternUP[pairdev_in][edit][FTYPE],
                                                        finger_patternUP[pairdev_in][edit][FNOTE1 + n - 1]));
                    } else {

                        finger_patternDOWN[pairdev_in][edit][FNOTE1 + n - 1] = res;
                        note_disp = finger_patternDOWN[pairdev_in][edit][FNOTE1];

                        finG->NoteDOWNButtons[n - 1]->setText(finG->setButtonNotePattern(finger_patternDOWN[pairdev_in][edit][FTYPE],
                                                        finger_patternDOWN[pairdev_in][edit][FNOTE1 + n - 1]));
                    }

                    if(val != 0) {

                        if(n == 1)
                            note_disp = res;

                        int key2 = finG->comboBoxBaseNote->currentIndex();
                        int note = res & 31;


                        if(mode == 0) {

                            int key_off = MidiInControl::GetNoteChord(0, note_disp % 2, 12) % 12;
                            key2 = (key2/12)*12 + (key2 % 12) - key_off;
                            note %= 6;
                            key2 = MidiInControl::GetNoteChord(0, note % 2, key2 + 12*(note/2) - 12);

                        } else if(mode == 1 || mode == 2) {

                            note %= 12;

                            if(mode == 1) {
                                int key_off = MidiInControl::GetNoteChord(3, note_disp % 4, 12) % 12;
                                key2 = (key2/12)*12 + (key2 % 12) - key_off;
                                key2 = MidiInControl::GetNoteChord(3, note % 4, key2 + 12*(note/4) - 12);
                            } else {
                                int key_off = MidiInControl::GetNoteChord(4, note_disp % 4, 12) % 12;
                                key2 = (key2/12)*12 + (key2 % 12) - key_off;
                                key2 = MidiInControl::GetNoteChord(4, note % 4, key2 + 12*(note/4) - 12);
                            }

                        } else if(mode == 3 || mode == 4) {

                            note %= 15;

                            if(mode == 3) {
                                int key_off = MidiInControl::GetNoteChord(5, note_disp % 5, 12) % 12;
                                key2 = (key2/12)*12 + (key2 % 12) - key_off;
                                key2 = MidiInControl::GetNoteChord(5, note % 5, key2 + 12*(note/5) - 12);
                            } else {
                                int key_off = MidiInControl::GetNoteChord(6, note_disp % 5, 12) % 12;
                                key2 = (key2/12)*12 + (key2 % 12) - key_off;
                                key2 = MidiInControl::GetNoteChord(6, note % 5, key2 + 12*(note/5) - 12);
                            }

                        }

                        if(key2 < 127 && (res & 64))
                            key2++;

                        if(pianoEvent)
                            delete pianoEvent;

                        pianoEvent = new NoteOnEvent(0, 100, MidiOutput::standardChannel(), 0);

                        pianoEvent->setNote(key2);
                        pianoEvent->setChannel(NewNoteTool::editChannel(), false);

                        int maxcounter = (!zone) ? (max_tick_counterUP[pairdev_in] + 1)
                                                 : (max_tick_counterDOWN[pairdev_in] + 1);

                        if(maxcounter > 64) maxcounter = 64;

                        if(res & 32) {

                            maxcounter = maxcounter * 15/10;
                        } else if((res & 31) >= gridH) {
                            maxcounter *= 2;
                        }

                        MidiPlayer::play(pianoEvent, maxcounter * 15);
                    }

                    FingerUpdate(mode);

                });

                connect(pb[mat], &QPushButtonE::mouseRightButton, this, [=]()
                {

                    // get finger

                    int key = (mat % 15) + 1;

                    if((data[n - 1] & 127) == (mat % 15) + 1) {

                        key = data[n - 1];
                    }


                    // set finger

                    int val = key;
                    int val2 = (val & 127) - 1;
                    int res = 0;

                    {
                        res =  (2 - ((val2 % gridH)/gridM)) * gridM + gridM - (val2 % gridM) - 1;
                        if(val & 128)
                            res|= 64;
                        if(tempo[n - 1] == 1)
                            res+= 32 + gridH;
                        else if(tempo[n - 1] == 2)
                            res+= gridH;
                    }

                    int note_disp = res;

                    if(!zone) {

                        if(n > 1)
                            note_disp = finger_patternUP[pairdev_in][edit][FNOTE1];

                    } else {

                        if(n > 1)
                            note_disp = finger_patternDOWN[pairdev_in][edit][FNOTE1];

                    }

                    if(val != 0) {

                        int key2 = finG->comboBoxBaseNote->currentIndex();
                        int note = res & 31;

                        if(mode == 0) {

                            int key_off = MidiInControl::GetNoteChord(0, note_disp % 2, 12) % 12;
                            key2 = (key2/12)*12 + (key2 % 12) - key_off;
                            note %= 6;
                            key2 = MidiInControl::GetNoteChord(0, note % 2, key2 + 12*(note/2) - 12);

                        } else if(mode == 1 || mode == 2) {

                            note %= 12;

                            if(mode == 1) {
                                int key_off = MidiInControl::GetNoteChord(3, note_disp % 4, 12) % 12;
                                key2 = (key2/12)*12 + (key2 % 12) - key_off;
                                key2 = MidiInControl::GetNoteChord(3, note % 4, key2 + 12*(note/4) - 12);
                            } else {
                                int key_off = MidiInControl::GetNoteChord(4, note_disp % 4, 12) % 12;
                                key2 = (key2/12)*12 + (key2 % 12) - key_off;
                                key2 = MidiInControl::GetNoteChord(4, note % 4, key2 + 12*(note/4) - 12);
                            }

                        } else if(mode == 3 || mode == 4) {

                            note %= 15;

                            if(mode == 3) {
                                int key_off = MidiInControl::GetNoteChord(5, note_disp % 5, 12) % 12;
                                key2 = (key2/12)*12 + (key2 % 12) - key_off;
                                key2 = MidiInControl::GetNoteChord(5, note % 5, key2 + 12*(note/5) - 12);
                            } else {
                                int key_off = MidiInControl::GetNoteChord(6, note_disp % 5, 12) % 12;
                                key2 = (key2/12)*12 + (key2 % 12) - key_off;
                                key2 = MidiInControl::GetNoteChord(6, note % 5, key2 + 12*(note/5) - 12);
                            }

                        }

                        if(key2 < 127 && (res & 64))
                            key2++;

                        if(pianoEvent)
                            delete pianoEvent;

                        pianoEvent = new NoteOnEvent(0, 100, MidiOutput::standardChannel(), 0);

                        pianoEvent->setNote(key2);
                        pianoEvent->setChannel(NewNoteTool::editChannel(), false);

                        int maxcounter = (!zone) ? (max_tick_counterUP[pairdev_in] + 1)
                                                 : (max_tick_counterDOWN[pairdev_in] + 1);

                        if(maxcounter > 64) maxcounter = 64;

                        if(res & 32) {

                            maxcounter = maxcounter * 15/10;
                        } else if((res & 31) >= gridH) {
                            maxcounter *= 2;
                        }

                        MidiPlayer::play(pianoEvent, maxcounter * 15);
                    }

                });

            } else {
                if(m < gridM) {
                    pb[mat]->setStyleSheet(QString::fromUtf8(
                        "QPushButton {color: white; background-color: #ff00ff;} \n"));
                } else if(m >= gridM * 2) {
                    pb[mat]->setStyleSheet(QString::fromUtf8(
                        "QPushButton {color: white; background-color: #df0000;} \n"));
                } else {
                    pb[mat]->setStyleSheet(QString::fromUtf8(
                        "QPushButton {color: white; background-color: #00df00;} \n"));

                }

            }

        }
    }

    QLabel *labelStepsH = new QLabel(group2);
    labelStepsH->setObjectName(QString::fromUtf8("labelStepsH"));
    labelStepsH->setGeometry(QRect(10 + 20, 46 - 30, 43, 16));
    labelStepsH->setText("Steps");

    horizontalSliderNotes = new QSlider(group2);
    horizontalSliderNotes->setObjectName(QString::fromUtf8("horizontalSliderNotes"));
    horizontalSliderNotes->setGeometry(QRect(20 + 50, 60 - 30, 360 /*711/2*/, 22));
    horizontalSliderNotes->setMinimum(1);
    horizontalSliderNotes->setMaximum(15);

    if(!zone)
        horizontalSliderNotes->setValue(finger_patternUP[pairdev_in][edit][FSTEPS]);
    else
        horizontalSliderNotes->setValue(finger_patternDOWN[pairdev_in][edit][FSTEPS]);

    horizontalSliderNotes->setOrientation(Qt::Horizontal);
    horizontalSliderNotes->setInvertedAppearance(false);
    horizontalSliderNotes->setInvertedControls(false);
    horizontalSliderNotes->setTickPosition(QSlider::TicksBelow);
    horizontalSliderNotes->setTickInterval(1);
    horizontalSliderNotes->setStyleSheet(QString::fromUtf8(
    "QSlider::handle:Horizontal {background: #40a0a0; border: 3px solid #303030; width: 4px; height: 10px; padding: 0; margin: -9px 0; border-radius: 3px;} \n"
    "QSlider::handle:Horizontal:hover {background: #40c0c0; border: 3px solid #303030; width: 4px; height: 10px; padding: 0; margin: -9px 0; border-radius: 3px;} \n"));
    TOOLTIP(horizontalSliderNotes, "Number of notes to use in the pattern");


    connect(horizontalSliderNotes, QOverload<int>::of(&QSlider::valueChanged), [=](int num)
    {


        for(int n = 1; n < 16; n++) {
            for(int m = 0; m < 15; m++) {

                int mat = m + n * 15;

                if(n > num) pb[mat]->setStyleSheet(szero);

                else pb[mat]->setStyleSheet(sone);

            }
        }

        if(!zone)
            finG->horizontalSliderNotesUP->setValue(num);
        else
            finG->horizontalSliderNotesDOWN->setValue(num);

    });

    QLabel *labelNote[15];

    for(int n = 0; n < 15; n++) {
        labelNote[n] = new QLabel(group2);
        labelNote[n]->setObjectName(QString::fromUtf8("labelNote") + QString::number(n));
        labelNote[n]->setGeometry(QRect(15 + 50/2 * n + 50, 90 - 8 - 30, 21, 16));
        labelNote[n]->setAlignment(Qt::AlignCenter);
        labelNote[n]->setText(QString::number(n + 1));
    }

    QPushButton *pushButtonTestNote = new QPushButton(group3);
    pushButtonTestNote->setObjectName(QString::fromUtf8("pushButtonTestNote"));
    pushButtonTestNote->setGeometry(QRect(16, 60 - 30, 101, 41));
    pushButtonTestNote->setText("Test Pattern");
    TOOLTIP(pushButtonTestNote, "Test Pattern Note");

    if(!zone) {
        connect(pushButtonTestNote, SIGNAL(pressed()), _parent, SLOT(play_noteUP()));
        connect(pushButtonTestNote, SIGNAL(released()), _parent, SLOT(stop_noteUP()));
    } else  {
        connect(pushButtonTestNote, SIGNAL(pressed()), _parent, SLOT(play_noteDOWN()));
        connect(pushButtonTestNote, SIGNAL(released()), _parent, SLOT(stop_noteDOWN()));
    }

    QLabel *labelBaseNote = new QLabel(group3);
    labelBaseNote->setObjectName(QString::fromUtf8("labelBaseNote"));
    labelBaseNote->setGeometry(QRect(129, 60 - 40 + 4, 101, 20));
    labelBaseNote->setAlignment(Qt::AlignCenter);
    labelBaseNote->setText("Base Note");
    TOOLTIP(labelBaseNote, "Base Note for Pattern Note Test");


    QComboBox *comboBoxBaseNote = new QComboBox(group3);
    comboBoxBaseNote->setObjectName(QString::fromUtf8("comboBoxBaseNote"));
    comboBoxBaseNote->setGeometry(QRect(129, 60 - 12, 101, 22));
    comboBoxBaseNote->clear();
    TOOLTIP(comboBoxBaseNote, "Base Note for Pattern Note Test");

    for(int n = 0; n < 128; n++) {
        comboBoxBaseNote->addItem(QString(&notes[n % 12][0]) + QString::number(n/12 -1));
    }

    comboBoxBaseNote->setCurrentIndex(finger_base_note);

    connect(comboBoxBaseNote, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int num)
    {
        finger_base_note = num;
        finG->comboBoxBaseNote->setCurrentIndex(finger_base_note);
    });

    connect(finG->comboBoxBaseNote, SIGNAL(currentIndexChanged(int)), comboBoxBaseNote, SLOT(setCurrentIndex(int)));

    QLabel * labelTempoH = new QLabel(group2);
    labelTempoH->setObjectName(QString::fromUtf8("labelTempoH"));
    labelTempoH->setGeometry(QRect(10 + 20, 143 - 60, 87, 13));
    labelTempoH->setText("Tempo (ms)");

    QSlider * horizontalSliderTempo = new QSlider(group2);
    horizontalSliderTempo->setObjectName(QString::fromUtf8("horizontalSliderTempo"));
    horizontalSliderTempo->setGeometry(QRect(20 + 50, 160 - 60, 360 /*731*/, 22));
    horizontalSliderTempo->setMinimum(0);
    horizontalSliderTempo->setMaximum(64);

    if(!zone)
        horizontalSliderTempo->setValue(max_tick_counterUP[pairdev_in]);
    else
        horizontalSliderTempo->setValue(max_tick_counterDOWN[pairdev_in]);

    horizontalSliderTempo->setOrientation(Qt::Horizontal);
    horizontalSliderTempo->setTickPosition(QSlider::TicksBelow);
    horizontalSliderTempo->setTickInterval(2);
    TOOLTIP(horizontalSliderTempo, "Selected time (in ms) to play a note");

    connect(horizontalSliderTempo, QOverload<int>::of(&QSlider::valueChanged), [=](int num)
    {

        if(!zone)
            finG->horizontalSliderTempoUP->setValue(num);
        else
            finG->horizontalSliderTempoDOWN->setValue(num);

    });

    QLabel * labelTempo[17];

    for(int n = 0; n <= 16; n++) {

        labelTempo[n] = new QLabel(group2);
        labelTempo[n]->setObjectName(QString::fromUtf8("labelTempo") + QString::number(n));
        labelTempo[n]->setGeometry(QRect(15 + 355 * n / 16 + 50, 190 - 60 - 8, 21, 21));
        labelTempo[n]->setAlignment(Qt::AlignCenter);
        int number = 1000*(n * 4 + 1 *(n == 0)) / 64;
        labelTempo[n]->setText(QString::number(number));
    }

    if(!zone) {
        connect(finG->horizontalSliderNotesUP, SIGNAL(valueChanged(int)), horizontalSliderNotes, SLOT(setValue(int)));
        connect(finG->horizontalSliderTempoUP, SIGNAL(valueChanged(int)), horizontalSliderTempo, SLOT(setValue(int)));
    } else {
        connect(finG->horizontalSliderNotesDOWN, SIGNAL(valueChanged(int)), horizontalSliderNotes, SLOT(setValue(int)));
        connect(finG->horizontalSliderTempoDOWN, SIGNAL(valueChanged(int)), horizontalSliderTempo, SLOT(setValue(int)));
    }


    FingerUpdate(mode);

}



FingerPatternDialog::FingerPatternDialog(QWidget* parent, QSettings *settings, int pairdev_in) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint) {

    QDialog *fingerPatternDialog = this;
    _parent = parent;
    pairdev_out = pairdev_in;

    FEdit = NULL;


    if(settings)
        _settings = settings;
    if(!_settings) //_settings = new QSettings(QString("MidiEditor"), QString("FingerPattern"));
                   _settings = new QSettings(QDir::homePath() + "/Midieditor/settings/MidiEditorFingerPattern.ini", QSettings::IniFormat);

    if(_settings) {
        QByteArray a =_settings->value("FingerPattern/FINGER_pattern_save" + (pairdev_in ? QString::number(pairdev_in) : "")).toByteArray();

        Unpackfinger(pairdev_out, a);
    }

    if (fingerPatternDialog->objectName().isEmpty())
        fingerPatternDialog->setObjectName(QString::fromUtf8("fingerPatternDialog"));
    fingerPatternDialog->setFixedSize(802 + 100, 634);
    fingerPatternDialog->setWindowTitle("Finger Pattern Utility (Devices " + QString::number(pairdev_out * 2) + "-"
                                         + QString::number(pairdev_out * 2 + 1) + ")");
    TOOLTIP(fingerPatternDialog, "Utility that modifies the keystroke\nusing selected note patterns and scales");

    if(_parent) {

        int index;

        QWidget *upper =new QWidget(fingerPatternDialog);
        upper->setFixedSize(802 + 100, 470);
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
        groupBoxPatternNoteUP->setGeometry(QRect(20, 10, 761 + 100, 221));
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

        TOOLTIP(groupBoxPatternNoteUP, "Group panel where you can select the base chord,\n"
                                        " the number of repetitions, the scale note pattern\n"
                                        " and number of notes to use, and the time (in ms) between notes");

        pushButtonDeleteUP = new QPushButton(groupBoxPatternNoteUP);
        pushButtonDeleteUP->setObjectName(QString::fromUtf8("pushButtonDeleteUP"));
        pushButtonDeleteUP->setGeometry(QRect(54, 20, 45, 23));
        pushButtonDeleteUP->setText("DEL");
        TOOLTIP(pushButtonDeleteUP, "Delete this custom note pattern\n"
                                       "or restore basic chords values");

        connect(pushButtonDeleteUP, QOverload<bool>::of(&QPushButton::clicked), [=](bool)
        {

            int r;

            if(finger_chord_modeUPEdit[pairdev_in] < MAX_FINGER_BASIC)
                r = QMessageBox::question(_parent, "Restore DEFAULT pattern", "Are you sure?                         ");
            else
                r = QMessageBox::question(_parent, "Delete CUSTOM pattern", "Are you sure?                         ");

            if(r != QMessageBox::Yes) return;


            if(finger_chord_modeUPEdit[pairdev_in] < MAX_FINGER_BASIC) {
                memcpy(&finger_patternUP[pairdev_in][finger_chord_modeUPEdit[pairdev_in]][FTYPE], &finger_patternBASIC[finger_chord_modeUPEdit[pairdev_in]][FTYPE], 19);
                comboBoxChordUP->currentIndexChanged(finger_chord_modeUPEdit[pairdev_in]);
                return;
            }

            comboBoxChordUP->blockSignals(true);

            memset(&finger_patternUP[pairdev_in][finger_chord_modeUPEdit[pairdev_in]][FTYPE], 0, 19);
            finger_patternUP[pairdev_in][finger_chord_modeUPEdit[pairdev_in]][FTEMPO] = 16;

            finger_chord_modeUPEdit[pairdev_in] = 0;

            if(finger_chord_modeUPEdit[pairdev_in] < MAX_FINGER_BASIC) { // fixed values for basic
                int val = 0;
                switch(finger_chord_modeUPEdit[pairdev_in]) {
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

                finger_patternUP[pairdev_in][finger_chord_modeUPEdit[pairdev_in]][FSTEPS] = val;
                finger_patternUP[pairdev_in][finger_chord_modeUPEdit[pairdev_in]][FREPEAT] = 0;
            }

            comboBoxChordUP->clear();
            comboBoxChordUP->addItem(stringTypes[0], -1);
            comboBoxChordUP->addItem(stringTypes[1], -2);
            comboBoxChordUP->addItem(stringTypes[2], -3);
            comboBoxChordUP->addItem(stringTypes[3], -4);
            comboBoxChordUP->addItem(stringTypes[4], -5);

            for(int n = 0; n < 16; n++) {
                if(finger_patternUP[pairdev_in][n + MAX_FINGER_BASIC][FSTEPS]) {

                    comboBoxChordUP->addItem("CUSTOM #" + QString::number(n + 1)
                                                 + " " + stringTypes[finger_patternUP[pairdev_in][n + MAX_FINGER_BASIC][FTYPE]], n);
                }
            }

            comboBoxChordUP->blockSignals(false);

            comboBoxChordUP->currentIndexChanged(0);
            comboBoxChordUP->setCurrentIndex(0);
        });

        labelStepsHUP = new QLabel(groupBoxPatternNoteUP);
        labelStepsHUP->setObjectName(QString::fromUtf8("labelStepsHUP"));
        labelStepsHUP->setGeometry(QRect(10 + 20, 46, 43, 16));
        //labelStepsHUP->setAlignment(Qt::AlignCenter);
        labelStepsHUP->setText("Steps");
        labelTempoHUP = new QLabel(groupBoxPatternNoteUP);
        labelTempoHUP->setObjectName(QString::fromUtf8("labelTempoHUP"));
        labelTempoHUP->setGeometry(QRect(10 + 20, 143, 87, 13));
        //labelTempoHUP->setAlignment(Qt::AlignCenter);
        labelTempoHUP->setText("Tempo (ms)");

        comboBoxChordUP = new QComboBox(groupBoxPatternNoteUP);
        comboBoxChordUP->setObjectName(QString::fromUtf8("comboBoxChordUP"));
        comboBoxChordUP->setGeometry(QRect(148, 20, 252, 22));
        comboBoxChordUP->addItem(stringTypes[0], -1);
        comboBoxChordUP->addItem(stringTypes[1], -2);
        comboBoxChordUP->addItem(stringTypes[2], -3);
        comboBoxChordUP->addItem(stringTypes[3], -4);
        comboBoxChordUP->addItem(stringTypes[4], -5);
        TOOLTIP(comboBoxChordUP, "Select the base chord here\nor a previously saved custom pattern to edit");

        index = finger_chord_modeUPEdit[pairdev_in];

        if(finger_chord_modeUPEdit[pairdev_in] < MAX_FINGER_BASIC)
            comboBoxChordUP->setCurrentIndex(finger_chord_modeUPEdit[pairdev_in]);


        for(int n = 0; n < 16; n++) {
            if(finger_patternUP[pairdev_in][n + MAX_FINGER_BASIC][FSTEPS]) {

                if(finger_chord_modeUPEdit[pairdev_in] == MAX_FINGER_BASIC + n)
                    index = comboBoxChordUP->count();

                comboBoxChordUP->addItem("CUSTOM #" + QString::number(n + 1)
                                         + " " + stringTypes[finger_patternUP[pairdev_in][n + MAX_FINGER_BASIC][FTYPE]], n);

            }
        }

        comboBoxChordUP->setCurrentIndex(index);

        max_tick_counterUP[pairdev_in] = finger_patternUP[pairdev_in][finger_chord_modeUPEdit[pairdev_in]][FTEMPO];
        finger_repeatUP[pairdev_in] = finger_patternUP[pairdev_in][finger_chord_modeUPEdit[pairdev_in]][FREPEAT];

        labelChordUP = new QLabel(groupBoxPatternNoteUP);
        labelChordUP->setObjectName(QString::fromUtf8("labelChordUP"));
        labelChordUP->setGeometry(QRect(110, 20, 34, 16));
        labelChordUP->setText("Chord:");

        comboBoxRepeatUP = new QComboBox(groupBoxPatternNoteUP);
        comboBoxRepeatUP->setObjectName(QString::fromUtf8("comboBoxRepeatUP"));
        comboBoxRepeatUP->setGeometry(QRect(460, 20, 69, 22));
        comboBoxRepeatUP->addItem("Infinite");
        TOOLTIP(comboBoxRepeatUP, "Selects the number of repeats of the pattern\n"
                                        "before stopping on the last note");
        for(int n = 1; n <= 7; n++)
            comboBoxRepeatUP->addItem(QString::number(n) + " Time");

        comboBoxRepeatUP->setCurrentIndex(finger_repeatUP[pairdev_in]);

        labelRepeatUP = new QLabel(groupBoxPatternNoteUP);
        labelRepeatUP->setObjectName(QString::fromUtf8("labelRepeat"));
        labelRepeatUP->setGeometry(QRect(410, 20, 47, 13));
        labelRepeatUP->setAlignment(Qt::AlignCenter);
        labelRepeatUP->setText("Repeat:");

        comboBoxCustomUP = new QComboBox(groupBoxPatternNoteUP);
        comboBoxCustomUP->setObjectName(QString::fromUtf8("comboBoxCustomUP"));
        comboBoxCustomUP->setGeometry(QRect(540, 20, 131 - 31, 22));
        for(int n = 1; n <= 16; n++)
            comboBoxCustomUP->addItem("CUSTOM #" + QString::number(n));
        TOOLTIP(comboBoxCustomUP, "Select a custom name pattern to save with STORE");

        int despX = -31;
        pushButtonStoreUP = new QPushButton(groupBoxPatternNoteUP);
        pushButtonStoreUP->setObjectName(QString::fromUtf8("pushButtonStoreUP"));
        pushButtonStoreUP->setGeometry(QRect(680 + despX, 20, 64, 23));
        pushButtonStoreUP->setText("STORE");
        TOOLTIP(pushButtonStoreUP, "Save in the register a custom note pattern with the selected name.\n"
                                        "It can be selected later from 'Chord'");

        QPushButton * pushButtonLoadUP = new QPushButton(groupBoxPatternNoteUP);
        pushButtonLoadUP->setObjectName(QString::fromUtf8("pushButtonLoadUP"));
        pushButtonLoadUP->setGeometry(QRect(680 + despX + 68, 20, 64, 23));
        pushButtonLoadUP->setText("LOAD");
        TOOLTIP(pushButtonLoadUP, "Load from file a custom note pattern with the selected name.\n"
                                      "It can be selected later from 'Chord'");

        QPushButton * pushButtonSaveUP = new QPushButton(groupBoxPatternNoteUP);
        pushButtonSaveUP->setObjectName(QString::fromUtf8("pushButtonSaveUP"));
        pushButtonSaveUP->setGeometry(QRect(680 + despX + 68 * 2, 20, 64, 23));
        pushButtonSaveUP->setText("SAVE");
        TOOLTIP(pushButtonSaveUP, "Save to file the custom note pattern.");


        horizontalSliderNotesUP = new QSlider(groupBoxPatternNoteUP);
        horizontalSliderNotesUP->setObjectName(QString::fromUtf8("horizontalSliderNotesUP"));
        horizontalSliderNotesUP->setGeometry(QRect(20 + 50, 60, 711, 22));
        horizontalSliderNotesUP->setMinimum(1);
        horizontalSliderNotesUP->setMaximum(15);
        horizontalSliderNotesUP->setValue(finger_patternUP[pairdev_in][finger_chord_modeUPEdit[pairdev_in]][FSTEPS]);
        horizontalSliderNotesUP->setOrientation(Qt::Horizontal);
        horizontalSliderNotesUP->setInvertedAppearance(false);
        horizontalSliderNotesUP->setInvertedControls(false);
        horizontalSliderNotesUP->setTickPosition(QSlider::TicksBelow);
        horizontalSliderNotesUP->setTickInterval(1);
        horizontalSliderNotesUP->setStyleSheet(QString::fromUtf8(
        "QSlider::handle:Horizontal {background: #40a0a0; border: 3px solid #303030; width: 4px; height: 10px; padding: 0; margin: -9px 0; border-radius: 3px;} \n"
        "QSlider::handle:Horizontal:hover {background: #40c0c0; border: 3px solid #303030; width: 4px; height: 10px; padding: 0; margin: -9px 0; border-radius: 3px;} \n"));
        TOOLTIP(horizontalSliderNotesUP, "Number of notes to use in the pattern");

        for(int n = 0; n < 15; n++) {

            NoteUPButtons[n] = new QPushButton(groupBoxPatternNoteUP);
            NoteUPButtons[n]->setObjectName(QString::fromUtf8("NoteUPButtons") + QString::number(n));
            NoteUPButtons[n]->setGeometry(QRect(4 + 50 * n + 50, 90, 49, 22));
            TOOLTIP(NoteUPButtons[n], "Base note to use in the pattern for the chosen 'Chord'.\n"
                                  "Click to edit the pattern.\n\n"
                                "'-' represents an Octave -\n"
                                "'+' represents an Octave +\n"
                                "'$' represents 1.5 tempo\n"
                                "'*' represents double tempo\n"
                                "'#' after represents sharp\n"
                                "'##' represents silence\n"
                                "'1' represents the base note\n"
                                "'2 to 7' represents the next notes of the scale");


            NoteUPButtons[n]->setText(setButtonNotePattern(
                                  finger_patternUP[pairdev_in][finger_chord_modeUPEdit[pairdev_in]][FTYPE],
                                  finger_patternUP[pairdev_in][finger_chord_modeUPEdit[pairdev_in]][FNOTE1 + n]));

            connect(NoteUPButtons[n], QOverload<bool>::of(&QPushButton::clicked), [=](bool)
            {
                if(FEdit)
                    delete FEdit;

                FEdit = new FingerEdit(this, pairdev_in, 0);
                if(FEdit)
                    FEdit->show();

            });


            labelNoteUP[n] = new QLabel(groupBoxPatternNoteUP);
            labelNoteUP[n]->setObjectName(QString::fromUtf8("labelNoteUP") + QString::number(n));
            labelNoteUP[n]->setGeometry(QRect(15 + 50 * n + 50, 120, 21, 16));
            labelNoteUP[n]->setAlignment(Qt::AlignCenter);
            labelNoteUP[n]->setText(QString::number(n + 1));
        }

        horizontalSliderTempoUP = new QSlider(groupBoxPatternNoteUP);
        horizontalSliderTempoUP->setObjectName(QString::fromUtf8("horizontalSliderTempoUP"));
        horizontalSliderTempoUP->setGeometry(QRect(10 + 50, 160, 731, 22));
        horizontalSliderTempoUP->setMinimum(0);
        horizontalSliderTempoUP->setMaximum(64);
        horizontalSliderTempoUP->setValue(max_tick_counterUP[pairdev_in]);
        horizontalSliderTempoUP->setOrientation(Qt::Horizontal);
        horizontalSliderTempoUP->setTickPosition(QSlider::TicksBelow);
        horizontalSliderTempoUP->setTickInterval(2);
        TOOLTIP(horizontalSliderTempoUP, "Selected time (in ms) to play a note");

        for(int n = 0; n <= 16; n++) {

            labelTempoUP[n] = new QLabel(groupBoxPatternNoteUP);
            labelTempoUP[n]->setObjectName(QString::fromUtf8("labelTempoUP") + QString::number(n));
            labelTempoUP[n]->setGeometry(QRect(5 + 720 * n / 16 + 50, 190, 21, 21));
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

           finger_chord_modeUPEdit[pairdev_in] = num;

            for(int n = 0; n < 15; n++) {

                if(num < MAX_FINGER_BASIC) {

                    finger_patternUP[pairdev_in][num][FTYPE] = num;

                    if(num == 0) { // power
                        finger_patternUP[pairdev_in][num][FSTEPS] = 2;
                        finger_patternUP[pairdev_in][num][FNOTE1] = 2;
                        finger_patternUP[pairdev_in][num][FNOTE1+1] = 3;

                    } else if(num == 1 || num == 2) { // major, minor prog
                        finger_patternUP[pairdev_in][num][FSTEPS] = 3;
                        finger_patternUP[pairdev_in][num][FNOTE1] = 4;
                        finger_patternUP[pairdev_in][num][FNOTE1+1] = 5;
                        finger_patternUP[pairdev_in][num][FNOTE1+2] = 6;
                    }  else if(num == 3 || num == 4) { // major, minor pentatonic
                        finger_patternUP[pairdev_in][num][FSTEPS] = 5;
                        finger_patternUP[pairdev_in][num][FNOTE1] = 5;
                        finger_patternUP[pairdev_in][num][FNOTE1+1] = 6;
                        finger_patternUP[pairdev_in][num][FNOTE1+2] = 7;
                        finger_patternUP[pairdev_in][num][FNOTE1+3] = 8;
                        finger_patternUP[pairdev_in][num][FNOTE1+4] = 9;
                    }
                }

                max_tick_counterUP[pairdev_in] = finger_patternUP[pairdev_in][num][FTEMPO];
                finger_repeatUP[pairdev_in] = finger_patternUP[pairdev_in][num][FREPEAT];
                horizontalSliderTempoUP->setValue(max_tick_counterUP[pairdev_in]);
                comboBoxRepeatUP->setCurrentIndex(finger_repeatUP[pairdev_in]);


                NoteUPButtons[n]->setText(setButtonNotePattern(
                                      finger_patternUP[pairdev_in][num][FTYPE],
                                      finger_patternUP[pairdev_in][num][FNOTE1 + n]));

            }

            horizontalSliderNotesUP->setValue(finger_patternUP[pairdev_in][finger_chord_modeUPEdit[pairdev_in]][FSTEPS]);

        });

        connect(comboBoxRepeatUP, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int num)
        {
            finger_repeatUP[pairdev_in] = num;
            finger_patternUP[pairdev_in][finger_chord_modeUPEdit[pairdev_in]][FREPEAT] = num;
        });



        for(int n = 0; n < 15; n++) {

                /*
            connect(ComboBoxNoteUP[n], QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int num)
            {


                int num2 = ComboBoxNoteUP[n]->itemData(num).toInt();
                finger_patternUP[pairdev_in][finger_chord_modeUPEdit[pairdev_in]][FNOTE1 + n] = num2;


            });
            */
        }

        connect(horizontalSliderNotesUP, QOverload<int>::of(&QSlider::valueChanged), [=](int num)
        {
            finger_patternUP[pairdev_in][finger_chord_modeUPEdit[pairdev_in]][FSTEPS] = num;

        });

        connect(horizontalSliderTempoUP, QOverload<int>::of(&QSlider::valueChanged), [=](int num)
        {
            if(finger_chord_modeUPEdit[pairdev_in] == finger_chord_modeUP[pairdev_in])
                max_tick_counterUP[pairdev_in] = num;

            finger_patternUP[pairdev_in][finger_chord_modeUPEdit[pairdev_in]][FTEMPO] = num;
        });


        connect(pushButtonStoreUP, QOverload<bool>::of(&QPushButton::clicked), [=](bool)
        {
            int custom = comboBoxCustomUP->currentIndex();
            int r = QMessageBox::question(_parent, "Store CUSTOM #" + QString::number(custom + 1), "Are you sure?                         ");
            if(r != QMessageBox::Yes) return;

            memcpy(&finger_patternUP[pairdev_in][MAX_FINGER_BASIC + custom][FTYPE], &finger_patternUP[pairdev_in][finger_chord_modeUPEdit[pairdev_in]][FTYPE], 19);

            if(finger_chord_modeUPEdit[pairdev_in] < MAX_FINGER_BASIC) { // fixed values for basic
                int val = 0;
                switch(finger_chord_modeUPEdit[pairdev_in]) {
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

                finger_patternUP[pairdev_in][finger_chord_modeUPEdit[pairdev_in]][1] = val;
                finger_patternUP[pairdev_in][finger_chord_modeUPEdit[pairdev_in]][3] = 0;
            }

            finger_chord_modeUPEdit[pairdev_in] = MAX_FINGER_BASIC + custom;

            comboBoxChordUP->clear();
            comboBoxChordUP->addItem(stringTypes[0], -1);
            comboBoxChordUP->addItem(stringTypes[1], -2);
            comboBoxChordUP->addItem(stringTypes[2], -3);
            comboBoxChordUP->addItem(stringTypes[3], -4);
            comboBoxChordUP->addItem(stringTypes[4], -5);

            int indx = 0;
            for(int n = 0; n < 16; n++) {
                if(finger_patternUP[pairdev_in][n + MAX_FINGER_BASIC][FSTEPS]) {
                    if(n == custom) indx= comboBoxChordUP->count();
                    comboBoxChordUP->addItem("CUSTOM #" + QString::number(n + 1)
                                             + " " + stringTypes[finger_patternUP[pairdev_in][n + MAX_FINGER_BASIC][FTYPE]], n);
                }
            }

            comboBoxChordUP->setCurrentIndex(indx);

        });

        connect(pushButtonLoadUP, &QPushButton::clicked, this, [=]() {
            int custom = comboBoxCustomUP->currentIndex();
            int r = QMessageBox::question(_parent, "Load CUSTOM #" + QString::number(custom + 1) + " from file", "Are you sure?                         ");
            if(r != QMessageBox::Yes) return;
            load_custom(pairdev_in, custom, false);
        });

        connect(pushButtonSaveUP, &QPushButton::clicked, this, [=]() {
            int r = QMessageBox::question(_parent, "Save to CUSTOM Finger Pattern file", "Are you sure?                         ");
            if(r != QMessageBox::Yes) return;
            save_custom(pairdev_in, false);
        });


//////

        groupBoxPatternNoteDOWN = new QGroupBox(upper);
        groupBoxPatternNoteDOWN->setObjectName(QString::fromUtf8("groupBoxPatternNoteDOWN"));
        groupBoxPatternNoteDOWN->setGeometry(QRect(20, 240, 761 + 100, 221));
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
        TOOLTIP(groupBoxPatternNoteDOWN, "Group panel where you can select the base chord,\n"
                                        " the number of repetitions, the scale note pattern\n"
                                        " and number of notes to use, and the time (in ms) between notes");

        pushButtonDeleteDOWN = new QPushButton(groupBoxPatternNoteDOWN);
        pushButtonDeleteDOWN->setObjectName(QString::fromUtf8("pushButtonDeleteDOWN"));
        pushButtonDeleteDOWN->setGeometry(QRect(54, 20, 45, 23));
        pushButtonDeleteDOWN->setText("DEL");
        TOOLTIP(pushButtonDeleteDOWN, "Delete this custom note pattern");

        connect(pushButtonDeleteDOWN, QOverload<bool>::of(&QPushButton::clicked), [=](bool)
        {
            int r;

            if(finger_chord_modeDOWNEdit[pairdev_in] < MAX_FINGER_BASIC)
                r = QMessageBox::question(_parent, "Restore DEFAULT pattern", "Are you sure?                         ");
            else
                r = QMessageBox::question(_parent, "Delete CUSTOM pattern", "Are you sure?                         ");

            if(r != QMessageBox::Yes) return;


            if(finger_chord_modeDOWNEdit[pairdev_in] < MAX_FINGER_BASIC) {
                memcpy(&finger_patternDOWN[pairdev_in][finger_chord_modeDOWNEdit[pairdev_in]][FTYPE], &finger_patternBASIC[finger_chord_modeDOWNEdit[pairdev_in]][FTYPE], 19);
                comboBoxChordDOWN->currentIndexChanged(finger_chord_modeDOWNEdit[pairdev_in]);
                return;
            }

            comboBoxChordDOWN->blockSignals(true);

            memset(&finger_patternDOWN[pairdev_in][finger_chord_modeDOWNEdit[pairdev_in]][FTYPE], 0, 19);
            finger_patternDOWN[pairdev_in][finger_chord_modeDOWNEdit[pairdev_in]][FTEMPO] = 16;

            finger_chord_modeDOWNEdit[pairdev_in] = 0;

            if(finger_chord_modeDOWNEdit[pairdev_in] < MAX_FINGER_BASIC) { // fixed values for basic
                int val = 0;
                switch(finger_chord_modeDOWNEdit[pairdev_in]) {
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

                finger_patternDOWN[pairdev_in][finger_chord_modeDOWNEdit[pairdev_in]][FSTEPS] = val;
                finger_patternDOWN[pairdev_in][finger_chord_modeDOWNEdit[pairdev_in]][FREPEAT] = 0;
            }

            comboBoxChordDOWN->clear();
            comboBoxChordDOWN->addItem(stringTypes[0], -1);
            comboBoxChordDOWN->addItem(stringTypes[1], -2);
            comboBoxChordDOWN->addItem(stringTypes[2], -3);
            comboBoxChordDOWN->addItem(stringTypes[3], -4);
            comboBoxChordDOWN->addItem(stringTypes[4], -5);

            for(int n = 0; n < 16; n++) {
                if(finger_patternDOWN[pairdev_in][n + MAX_FINGER_BASIC][FSTEPS]) {

                    comboBoxChordDOWN->addItem("CUSTOM #" + QString::number(n + 1)
                                             + " " + stringTypes[finger_patternDOWN[pairdev_in][n + MAX_FINGER_BASIC][FTYPE]], n);
                }
            }

            comboBoxChordDOWN->blockSignals(false);

            comboBoxChordDOWN->currentIndexChanged(0);
            comboBoxChordDOWN->setCurrentIndex(0);

        });

        labelStepsHDOWN = new QLabel(groupBoxPatternNoteDOWN);
        labelStepsHDOWN->setObjectName(QString::fromUtf8("labelStepsHDOWN"));
        labelStepsHDOWN->setGeometry(QRect(10 + 20, 46, 43, 16));
        //labelStepsHDOWN->setAlignment(Qt::AlignCenter);
        labelStepsHDOWN->setText("Steps");
        labelTempoHDOWN = new QLabel(groupBoxPatternNoteDOWN);
        labelTempoHDOWN->setObjectName(QString::fromUtf8("labelTempoHDOWN"));
        labelTempoHDOWN->setGeometry(QRect(10 + 20, 143, 87, 13));
        //labelTempoHDOWN->setAlignment(Qt::AlignCenter);
        labelTempoHDOWN->setText("Tempo (ms)");

        comboBoxChordDOWN = new QComboBox(groupBoxPatternNoteDOWN);
        comboBoxChordDOWN->setObjectName(QString::fromUtf8("comboBoxChordDOWN"));
        comboBoxChordDOWN->setGeometry(QRect(148, 20, 252, 22));
        comboBoxChordDOWN->addItem(stringTypes[0], -1);
        comboBoxChordDOWN->addItem(stringTypes[1], -2);
        comboBoxChordDOWN->addItem(stringTypes[2], -3);
        comboBoxChordDOWN->addItem(stringTypes[3], -4);
        comboBoxChordDOWN->addItem(stringTypes[4], -5);
        TOOLTIP(comboBoxChordDOWN, "Select the base chord here\n"
                          "or a previously saved custom pattern to edit");


        index = finger_chord_modeDOWNEdit[pairdev_in];

        if(finger_chord_modeDOWNEdit[pairdev_in] < MAX_FINGER_BASIC)
            comboBoxChordDOWN->setCurrentIndex(finger_chord_modeDOWNEdit[pairdev_in]);

        for(int n = 0; n < 16; n++) {
            if(finger_patternDOWN[pairdev_in][n + MAX_FINGER_BASIC][FSTEPS]) {

                if(finger_chord_modeDOWNEdit[pairdev_in] == MAX_FINGER_BASIC + n)
                    index = comboBoxChordDOWN->count();

                comboBoxChordDOWN->addItem("CUSTOM #" + QString::number(n + 1)
                                         + " " + stringTypes[finger_patternDOWN[pairdev_in][n + MAX_FINGER_BASIC][FTYPE]], n);

            }
        }

        comboBoxChordDOWN->setCurrentIndex(index);

        max_tick_counterDOWN[pairdev_in] = finger_patternDOWN[pairdev_in][finger_chord_modeDOWNEdit[pairdev_in]][FTEMPO];
        finger_repeatDOWN[pairdev_in] = finger_patternDOWN[pairdev_in][finger_chord_modeDOWNEdit[pairdev_in]][FREPEAT];

        labelChordDOWN = new QLabel(groupBoxPatternNoteDOWN);
        labelChordDOWN->setObjectName(QString::fromUtf8("labelChordDOWN"));
        labelChordDOWN->setGeometry(QRect(110, 20, 34, 16));
        labelChordDOWN->setText("Chord:");

        comboBoxRepeatDOWN = new QComboBox(groupBoxPatternNoteDOWN);
        comboBoxRepeatDOWN->setObjectName(QString::fromUtf8("comboBoxRepeatDOWN"));
        comboBoxRepeatDOWN->setGeometry(QRect(460, 20, 69, 22));
        comboBoxRepeatDOWN->addItem("Infinite");
        TOOLTIP(comboBoxRepeatDOWN, "Selects the number of repeats of the pattern\n"
                                        "before stopping on the last note");
        for(int n = 1; n <= 7; n++)
            comboBoxRepeatDOWN->addItem(QString::number(n) + " Time");

        comboBoxRepeatDOWN->setCurrentIndex(finger_repeatDOWN[pairdev_in]);

        labelRepeatDOWN = new QLabel(groupBoxPatternNoteDOWN);
        labelRepeatDOWN->setObjectName(QString::fromUtf8("labelRepeat"));
        labelRepeatDOWN->setGeometry(QRect(410, 20, 47, 13));
        labelRepeatDOWN->setAlignment(Qt::AlignCenter);
        labelRepeatDOWN->setText("Repeat:");

        comboBoxCustomDOWN = new QComboBox(groupBoxPatternNoteDOWN);
        comboBoxCustomDOWN->setObjectName(QString::fromUtf8("comboBoxCustomDOWN"));
        comboBoxCustomDOWN->setGeometry(QRect(540, 20, 131 - 31, 22));
        for(int n = 1; n <= 16; n++)
            comboBoxCustomDOWN->addItem("CUSTOM #" + QString::number(n));
        TOOLTIP(comboBoxCustomDOWN, "Select a custom name pattern to save with STORE");

        pushButtonStoreDOWN = new QPushButton(groupBoxPatternNoteDOWN);
        pushButtonStoreDOWN->setObjectName(QString::fromUtf8("pushButtonStoreDOWN"));
        pushButtonStoreDOWN->setGeometry(QRect(680 + despX, 20, 64, 23));
        pushButtonStoreDOWN->setText("STORE");
        TOOLTIP(pushButtonStoreDOWN, "Save in the register a custom note pattern with the selected name.\n"
                                            "It can be selected later from 'Chord'");

        QPushButton * pushButtonLoadDOWN = new QPushButton(groupBoxPatternNoteDOWN);
        pushButtonLoadDOWN->setObjectName(QString::fromUtf8("pushButtonLoadUP"));
        pushButtonLoadDOWN->setGeometry(QRect(680 + despX + 68, 20, 64, 23));
        pushButtonLoadDOWN->setText("LOAD");
        TOOLTIP(pushButtonLoadDOWN, "Load from file a custom note pattern with the selected name.\n"
                                     "It can be selected later from 'Chord'");

        QPushButton * pushButtonSaveDOWN = new QPushButton(groupBoxPatternNoteDOWN);
        pushButtonSaveDOWN->setObjectName(QString::fromUtf8("pushButtonSaveDOWN"));
        pushButtonSaveDOWN->setGeometry(QRect(680 + despX + 68 * 2, 20, 64, 23));
        pushButtonSaveDOWN->setText("SAVE");
        TOOLTIP(pushButtonSaveDOWN, "Save to file the custom note pattern.");

        horizontalSliderNotesDOWN = new QSlider(groupBoxPatternNoteDOWN);
        horizontalSliderNotesDOWN->setObjectName(QString::fromUtf8("horizontalSliderNotesDOWN"));
        horizontalSliderNotesDOWN->setGeometry(QRect(20 + 50, 60, 711, 22));
        horizontalSliderNotesDOWN->setMinimum(1);
        horizontalSliderNotesDOWN->setMaximum(15);
        horizontalSliderNotesDOWN->setValue(finger_patternDOWN[pairdev_in][finger_chord_modeDOWNEdit[pairdev_in]][FSTEPS]);
        horizontalSliderNotesDOWN->setOrientation(Qt::Horizontal);
        horizontalSliderNotesDOWN->setInvertedAppearance(false);
        horizontalSliderNotesDOWN->setInvertedControls(false);
        horizontalSliderNotesDOWN->setTickPosition(QSlider::TicksBelow);
        horizontalSliderNotesDOWN->setTickInterval(1);
        horizontalSliderNotesDOWN->setStyleSheet(QString::fromUtf8(
                "QSlider::handle:Horizontal {background: #40a0a0; border: 3px solid #303030; width: 4px; height: 10px; padding: 0; margin: -9px 0; border-radius: 3px;} \n"
                "QSlider::handle:Horizontal:hover {background: #40c0c0; border: 3px solid #303030; width: 4px; height: 10px; padding: 0; margin: -9px 0; border-radius: 3px;} \n"));

        TOOLTIP(horizontalSliderNotesDOWN, "Number of notes to use in the pattern");

        for(int n = 0; n < 15; n++) {

            NoteDOWNButtons[n] = new QPushButton(groupBoxPatternNoteDOWN);
            NoteDOWNButtons[n]->setObjectName(QString::fromUtf8("NoteDOWNButtons") + QString::number(n));
            NoteDOWNButtons[n]->setGeometry(QRect(4 + 50 * n + 50, 90, 49, 22));

            TOOLTIP(NoteDOWNButtons[n], "Base note to use in the pattern for the chosen 'Chord'.\n"
                                  "Click to edit the pattern.\n\n"
                                "'-' represents an Octave -\n"
                                "'+' represents an Octave +\n"
                                "'$' represents 1.5 tempo\n"
                                "'*' represents double tempo\n"
                                "'#' after represents sharp\n"
                                "'##' represents silence\n"
                                "'1' represents the base note\n"
                                "'2 to 7' represents the next notes of the scale");


            NoteDOWNButtons[n]->setText(setButtonNotePattern(
                                  finger_patternDOWN[pairdev_in][finger_chord_modeDOWNEdit[pairdev_in]][FTYPE],
                                  finger_patternDOWN[pairdev_in][finger_chord_modeDOWNEdit[pairdev_in]][FNOTE1 + n]));

            connect(NoteDOWNButtons[n], QOverload<bool>::of(&QPushButton::clicked), [=](bool)
            {

                if(FEdit)
                    delete FEdit;

                FEdit = new FingerEdit(this, pairdev_in, 1);
                if(FEdit)
                    FEdit->show();

            });


            labelNoteDOWN[n] = new QLabel(groupBoxPatternNoteDOWN);
            labelNoteDOWN[n]->setObjectName(QString::fromUtf8("labelNoteDOWN") + QString::number(n));
            labelNoteDOWN[n]->setGeometry(QRect(15 + 50 * n + 50, 120, 21, 16));
            labelNoteDOWN[n]->setAlignment(Qt::AlignCenter);
            labelNoteDOWN[n]->setText(QString::number(n + 1));
        }


        horizontalSliderTempoDOWN = new QSlider(groupBoxPatternNoteDOWN);
        horizontalSliderTempoDOWN->setObjectName(QString::fromUtf8("horizontalSliderTempoDOWN"));
        horizontalSliderTempoDOWN->setGeometry(QRect(10 + 50, 160, 731, 22));
        horizontalSliderTempoDOWN->setMinimum(0);
        horizontalSliderTempoDOWN->setMaximum(64);
        horizontalSliderTempoDOWN->setValue(max_tick_counterDOWN[pairdev_in]);
        horizontalSliderTempoDOWN->setOrientation(Qt::Horizontal);
        horizontalSliderTempoDOWN->setTickPosition(QSlider::TicksBelow);
        horizontalSliderTempoDOWN->setTickInterval(2);
        TOOLTIP(horizontalSliderTempoDOWN, "Selected time (in ms) to play a note");

        for(int n = 0; n <= 16; n++) {
            labelTempoDOWN[n] = new QLabel(groupBoxPatternNoteDOWN);
            labelTempoDOWN[n]->setObjectName(QString::fromUtf8("labelTempoDOWN") + QString::number(n));
            labelTempoDOWN[n]->setGeometry(QRect(5 + 720 * n / 16 + 50, 190, 21, 21));
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

           finger_chord_modeDOWNEdit[pairdev_in] = num;

            for(int n = 0; n < 15; n++) {

                if(num < MAX_FINGER_BASIC && n == 0) {

                    finger_patternDOWN[pairdev_in][num][FTYPE] = num;

                    if(num == 0) { // power
                        finger_patternDOWN[pairdev_in][num][FSTEPS] = 2;
                        finger_patternDOWN[pairdev_in][num][FNOTE1] = 2;
                        finger_patternDOWN[pairdev_in][num][FNOTE1+1] = 3;

                    } else if(num == 1 || num == 2) { // major, minor prog
                        finger_patternDOWN[pairdev_in][num][FSTEPS] = 3;
                        finger_patternDOWN[pairdev_in][num][FNOTE1] = 4;
                        finger_patternDOWN[pairdev_in][num][FNOTE1+1] = 5;
                        finger_patternDOWN[pairdev_in][num][FNOTE1+2] = 6;
                    }  else if(num == 3 || num == 4) { // major, minor pentatonic
                        finger_patternDOWN[pairdev_in][num][FSTEPS] = 5;
                        finger_patternDOWN[pairdev_in][num][FNOTE1] = 5;
                        finger_patternDOWN[pairdev_in][num][FNOTE1+1] = 6;
                        finger_patternDOWN[pairdev_in][num][FNOTE1+2] = 7;
                        finger_patternDOWN[pairdev_in][num][FNOTE1+3] = 8;
                        finger_patternDOWN[pairdev_in][num][FNOTE1+4] = 9;
                    }
                }

                max_tick_counterDOWN[pairdev_in] = finger_patternDOWN[pairdev_in][num][FTEMPO];
                finger_repeatDOWN[pairdev_in] = finger_patternDOWN[pairdev_in][num][FREPEAT];
                horizontalSliderTempoDOWN->setValue(max_tick_counterDOWN[pairdev_in]);
                comboBoxRepeatDOWN->setCurrentIndex(finger_repeatDOWN[pairdev_in]);

                NoteDOWNButtons[n]->setText(setButtonNotePattern(
                                      finger_patternDOWN[pairdev_in][num][FTYPE],
                                      finger_patternDOWN[pairdev_in][num][FNOTE1 + n]));

            }

            horizontalSliderNotesDOWN->setValue(finger_patternDOWN[pairdev_in][finger_chord_modeDOWNEdit[pairdev_in]][FSTEPS]);

        });

        connect(comboBoxRepeatDOWN, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int num)
        {
            finger_repeatDOWN[pairdev_in] = num;
            finger_patternDOWN[pairdev_in][finger_chord_modeDOWNEdit[pairdev_in]][FREPEAT] = num;
        });

        connect(horizontalSliderNotesDOWN, QOverload<int>::of(&QSlider::valueChanged), [=](int num)
        {
            finger_patternDOWN[pairdev_in][finger_chord_modeDOWNEdit[pairdev_in]][FSTEPS] = num;

        });

        connect(horizontalSliderTempoDOWN, QOverload<int>::of(&QSlider::valueChanged), [=](int num)
        {

            if(finger_chord_modeDOWN[pairdev_in] == finger_chord_modeDOWNEdit[pairdev_in])
                max_tick_counterDOWN[pairdev_in] = num;

            finger_patternDOWN[pairdev_in][finger_chord_modeDOWNEdit[pairdev_in]][FTEMPO] = num;
        });


        connect(pushButtonStoreDOWN, QOverload<bool>::of(&QPushButton::clicked), [=](bool)
        {
            int custom = comboBoxCustomDOWN->currentIndex();
            int r = QMessageBox::question(_parent, "Store CUSTOM #" + QString::number(custom + 1), "Are you sure?                         ");
            if(r != QMessageBox::Yes) return;

            memcpy(&finger_patternDOWN[pairdev_in][MAX_FINGER_BASIC + custom][FTYPE], &finger_patternDOWN[pairdev_in][finger_chord_modeDOWNEdit[pairdev_in]][FTYPE], 19);

            if(finger_chord_modeDOWNEdit[pairdev_in] < MAX_FINGER_BASIC) { // fixed values for basic
                int val = 0;
                switch(finger_chord_modeDOWNEdit[pairdev_in]) {
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

                finger_patternDOWN[pairdev_in][finger_chord_modeDOWNEdit[pairdev_in]][FSTEPS] = val;
                finger_patternDOWN[pairdev_in][finger_chord_modeDOWNEdit[pairdev_in]][FREPEAT] = 0;
            }

            finger_chord_modeDOWNEdit[pairdev_in] = MAX_FINGER_BASIC + custom;

            comboBoxChordDOWN->clear();
            comboBoxChordDOWN->addItem(stringTypes[0], -1);
            comboBoxChordDOWN->addItem(stringTypes[1], -2);
            comboBoxChordDOWN->addItem(stringTypes[2], -3);
            comboBoxChordDOWN->addItem(stringTypes[3], -4);
            comboBoxChordUP->addItem(stringTypes[4], -5);

            int indx = 0;
            for(int n = 0; n < 16; n++) {
                if(finger_patternDOWN[pairdev_in][n + MAX_FINGER_BASIC][FSTEPS]) {
                    if(n == custom) indx= comboBoxChordDOWN->count();
                    comboBoxChordDOWN->addItem("CUSTOM #" + QString::number(n + 1)
                                             + " " + stringTypes[finger_patternDOWN[pairdev_in][n + MAX_FINGER_BASIC][FTYPE]], n);
                }
            }

            comboBoxChordDOWN->setCurrentIndex(indx);

        });

        connect(pushButtonLoadDOWN, &QPushButton::clicked, this, [=]() {
            int custom = comboBoxCustomDOWN->currentIndex();
            int r = QMessageBox::question(_parent, "Load CUSTOM #" + QString::number(custom + 1) + " from file", "Are you sure?                         ");
            if(r != QMessageBox::Yes) return;
            load_custom(pairdev_in, custom, true);
        });

        connect(pushButtonSaveDOWN, &QPushButton::clicked, this, [=]() {
            int r = QMessageBox::question(_parent, "Save to CUSTOM Finger Pattern file", "Are you sure?                         ");
            if(r != QMessageBox::Yes) return;
            save_custom(pairdev_in, true);
        });

 //******
        int yy = 10;

        labelBaseNote = new QLabel(fingerPatternDialog);
        labelBaseNote->setObjectName(QString::fromUtf8("labelBaseNote"));
        labelBaseNote->setGeometry(QRect(690, 520 + yy, 101, 20));
        labelBaseNote->setAlignment(Qt::AlignCenter);
        labelBaseNote->setText("Base Note");
        TOOLTIP(labelBaseNote, "Base Note for Pattern Note Test");

        comboBoxBaseNote = new QComboBox(fingerPatternDialog);
        comboBoxBaseNote->setObjectName(QString::fromUtf8("comboBoxBaseNote"));
        comboBoxBaseNote->setGeometry(QRect(690, 550 + yy, 101, 22));
        comboBoxBaseNote->clear();
        TOOLTIP(comboBoxBaseNote, "Base Note for Pattern Note Test");

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
        pushButtonTestNoteUP->setGeometry(QRect(580, 470 + yy, 101, 41));
        pushButtonTestNoteUP->setText("Test Pattern UP");
        TOOLTIP(pushButtonTestNoteUP, "Test Pattern Note UP");

        pushButtonTestNoteDOWN = new QPushButton(fingerPatternDialog);
        pushButtonTestNoteDOWN->setObjectName(QString::fromUtf8("pushButtonTestNoteDOWN"));
        pushButtonTestNoteDOWN->setGeometry(QRect(690, 470 + yy, 121, 41));
        pushButtonTestNoteDOWN->setText("Test Pattern DOWN");
        TOOLTIP(pushButtonTestNoteDOWN, "Test Pattern Note DOWN");

        pushButtonSavePattern = new QPushButton(fingerPatternDialog);
        pushButtonSavePattern->setObjectName(QString::fromUtf8("pushButtonSavePattern"));
        pushButtonSavePattern->setGeometry(QRect(580, 520 + yy, 101, 27));
        pushButtonSavePattern->setText("Save Pattern File");
        TOOLTIP(pushButtonSavePattern, "Save Finger Pattern File");

        connect(pushButtonSavePattern, SIGNAL(clicked()), this, SLOT(save()));

        pushButtonLoadPattern = new QPushButton(fingerPatternDialog);
        pushButtonLoadPattern->setObjectName(QString::fromUtf8("pushButtonLoadPattern"));
        pushButtonLoadPattern->setGeometry(QRect(580, 550 + yy - 2, 101, 27));
        pushButtonLoadPattern->setText("Load Pattern File");
        TOOLTIP(pushButtonLoadPattern, "Load Finger Pattern File");

        connect(pushButtonLoadPattern, SIGNAL(clicked()), this, SLOT(load()));


// GROUP PLAY **************************************************************+

        groupPlay = new QWidget(fingerPatternDialog);
        groupPlay->setObjectName(QString::fromUtf8("groupPlay"));
        groupPlay->setEnabled(!_note_finger_disabled[pairdev_in]);
        groupPlay->setGeometry(QRect(10, 460 + yy, 570, 151));

        labelUP = new QLabel(groupPlay);
        labelUP->setObjectName(QString::fromUtf8("labelUP"));
        labelUP->setGeometry(QRect(0, 0, 180, 20));
        labelUP->setAlignment(Qt::AlignCenter);
        labelUP->setText("Note UP");
        TOOLTIP(labelUP, "Note used to enable Finger Pattern #1\n"
                            "to the channel UP");

        comboBoxTypeUP = new QComboBox(groupPlay);
        comboBoxTypeUP->setObjectName(QString::fromUtf8("comboBoxTypeUP"));
        comboBoxTypeUP->setGeometry(QRect(0, 20, 180, 22));
        TOOLTIP(comboBoxTypeUP, "Selected Finger Pattern for UP");

        labelUP2 = new QLabel(groupPlay);
        labelUP2->setObjectName(QString::fromUtf8("labelUP2"));
        labelUP2->setGeometry(QRect(190, 0, 180, 20));
        labelUP2->setAlignment(Qt::AlignCenter);
        labelUP2->setText("Note UP 2");
        TOOLTIP(labelUP2, "Note used to enable Finger Pattern #2\n"
                             "to the channel UP");

        comboBoxTypeUP2 = new QComboBox(groupPlay);
        comboBoxTypeUP2->setObjectName(QString::fromUtf8("comboBoxTypeUP2"));
        comboBoxTypeUP2->setGeometry(QRect(190, 20, 180, 22));
        TOOLTIP(comboBoxTypeUP2, "Selected Finger Pattern for UP 2");

        labelUP3 = new QLabel(groupPlay);
        labelUP3->setObjectName(QString::fromUtf8("labelUP3"));
        labelUP3->setGeometry(QRect(380, 0, 180, 20));
        labelUP3->setAlignment(Qt::AlignCenter);
        labelUP3->setText("Note UP 3");
        TOOLTIP(labelUP3, "Note used to enable Finger Pattern #3\n"
                             "to the channel UP");

        comboBoxTypeUP3 = new QComboBox(groupPlay);
        comboBoxTypeUP3->setObjectName(QString::fromUtf8("comboBoxTypeUP3"));
        comboBoxTypeUP3->setGeometry(QRect(380, 20, 180, 22));
        TOOLTIP(comboBoxTypeUP3, "Selected Finger Pattern for UP 3");

        // 250  580 + yy

        comboBoxTypeUP->clear();
        comboBoxTypeUP->addItem(stringTypes[0]);
        comboBoxTypeUP->addItem(stringTypes[1]);
        comboBoxTypeUP->addItem(stringTypes[2]);
        comboBoxTypeUP->addItem(stringTypes[3]);
        comboBoxTypeUP->addItem(stringTypes[4]);

        int indx = 0;

        if(_note_finger_pattern_UP[pairdev_in] < MAX_FINGER_BASIC)
            indx = _note_finger_pattern_UP[pairdev_in];


        for(int n = 0; n < 16; n++) {

            if(finger_patternUP[pairdev_in][n + MAX_FINGER_BASIC][FSTEPS]) {

                if(n + MAX_FINGER_BASIC == _note_finger_pattern_UP[pairdev_in])
                    indx= _note_finger_pattern_UP[pairdev_in];

            }

            comboBoxTypeUP->addItem("CUSTOM #" + QString::number(n + 1));
        }

        comboBoxTypeUP->setCurrentIndex(indx);

        connect(comboBoxTypeUP, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int num)
        {
            _note_finger_pattern_UP[pairdev_in] = num;
            _settings->setValue("FingerPattern/FINGER_pattern_UP" + (pairdev_in ? QString::number(pairdev_in) : ""), _note_finger_pattern_UP[pairdev_in]);
        });

        comboBoxTypeUP2->clear();
        comboBoxTypeUP2->addItem(stringTypes[0]);
        comboBoxTypeUP2->addItem(stringTypes[1]);
        comboBoxTypeUP2->addItem(stringTypes[2]);
        comboBoxTypeUP2->addItem(stringTypes[3]);
        comboBoxTypeUP2->addItem(stringTypes[4]);

        indx = 0;

        if(_note_finger_pattern_UP2[pairdev_in] < MAX_FINGER_BASIC)
            indx = _note_finger_pattern_UP2[pairdev_in];

        for(int n = 0; n < 16; n++) {
            if(finger_patternUP[pairdev_in][n + MAX_FINGER_BASIC][FSTEPS]) {
                if(n + MAX_FINGER_BASIC == _note_finger_pattern_UP2[pairdev_in])
                    indx= _note_finger_pattern_UP2[pairdev_in];

            }

            comboBoxTypeUP2->addItem("CUSTOM #" + QString::number(n + 1));
        }

        comboBoxTypeUP2->setCurrentIndex(indx);

        connect(comboBoxTypeUP2, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int num)
        {
            _note_finger_pattern_UP2[pairdev_in] = num;
            _settings->setValue("FingerPattern/FINGER_pattern_UP2" + (pairdev_in ? QString::number(pairdev_in) : ""), _note_finger_pattern_UP2[pairdev_in]);
        });

        comboBoxTypeUP3->clear();
        comboBoxTypeUP3->addItem(stringTypes[0]);
        comboBoxTypeUP3->addItem(stringTypes[1]);
        comboBoxTypeUP3->addItem(stringTypes[2]);
        comboBoxTypeUP3->addItem(stringTypes[3]);
        comboBoxTypeUP3->addItem(stringTypes[4]);

        indx = 0;

        if(_note_finger_pattern_UP3[pairdev_in] < MAX_FINGER_BASIC)
            indx = _note_finger_pattern_UP3[pairdev_in];

        for(int n = 0; n < 16; n++) {
            if(finger_patternUP[pairdev_in][n + MAX_FINGER_BASIC][FSTEPS]) {
                if(n + MAX_FINGER_BASIC == _note_finger_pattern_UP3[pairdev_in])
                    indx= _note_finger_pattern_UP3[pairdev_in];

            }

            comboBoxTypeUP3->addItem("CUSTOM #" + QString::number(n + 1));
        }

        comboBoxTypeUP3->setCurrentIndex(indx);

        connect(comboBoxTypeUP3, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int num)
        {
            _note_finger_pattern_UP3[pairdev_in] = num;
            _settings->setValue("FingerPattern/FINGER_pattern_UP3" + (pairdev_in ? QString::number(pairdev_in) : ""), _note_finger_pattern_UP3[pairdev_in]);
        });


        labelDOWN = new QLabel(groupPlay);
        labelDOWN->setObjectName(QString::fromUtf8("labelDOWN"));
        labelDOWN->setGeometry(QRect(0, 60, 180, 20));
        labelDOWN->setAlignment(Qt::AlignCenter);
        labelDOWN->setText("Note DOWN");
        TOOLTIP(labelDOWN, "Note used to enable Finger Pattern #1\n"
                              "to the channel DOWN");

        comboBoxTypeDOWN = new QComboBox(groupPlay);
        comboBoxTypeDOWN->setObjectName(QString::fromUtf8("comboBoxTypeDOWN"));
        comboBoxTypeDOWN->setGeometry(QRect(0, 80, 180, 22));
        TOOLTIP(comboBoxTypeDOWN, "Selected Finger Pattern for DOWN");

        labelDOWN2 = new QLabel(groupPlay);
        labelDOWN2->setObjectName(QString::fromUtf8("labelDOWN2"));
        labelDOWN2->setGeometry(QRect(190, 60, 180, 20));
        labelDOWN2->setAlignment(Qt::AlignCenter);
        labelDOWN2->setText("Note DOWN 2");
        TOOLTIP(labelDOWN2, "Note used to enable Finger Pattern #2\n"
                               "to the channel DOWN");

        comboBoxTypeDOWN2 = new QComboBox(groupPlay);
        comboBoxTypeDOWN2->setObjectName(QString::fromUtf8("comboBoxTypeDOWN2"));
        comboBoxTypeDOWN2->setGeometry(QRect(190, 80, 180, 22));
        TOOLTIP(comboBoxTypeDOWN2, "Selected Finger Pattern for DOWN 2");

        labelDOWN3 = new QLabel(groupPlay);
        labelDOWN3->setObjectName(QString::fromUtf8("labelDOWN3"));
        labelDOWN3->setGeometry(QRect(380, 60, 180, 20));
        labelDOWN3->setAlignment(Qt::AlignCenter);
        labelDOWN3->setText("Note DOWN 3");
        TOOLTIP(labelDOWN3, "Note used to enable Finger Pattern #3\n"
                               "to the channel DOWN");

        comboBoxTypeDOWN3 = new QComboBox(groupPlay);
        comboBoxTypeDOWN3->setObjectName(QString::fromUtf8("comboBoxTypeDOWN3"));
        comboBoxTypeDOWN3->setGeometry(QRect(380, 80, 180, 22));
        TOOLTIP(comboBoxTypeDOWN3, "Selected Finger Pattern for DOWN 3");

        // 370  580 + yy

        // 250  580 + yy
        checkBoxDisableFinger = new QCheckBox(fingerPatternDialog);
        checkBoxDisableFinger->setObjectName(QString::fromUtf8("checkBoxDisableFinger"));
        checkBoxDisableFinger->setGeometry(QRect(250, 580 + yy, 231, 21));
        QFont font2;
        font2.setPointSize(12);
        checkBoxDisableFinger->setFont(font2);
        checkBoxDisableFinger->setText("Disable Finger Pattern Input");
        checkBoxDisableFinger->setChecked(_note_finger_disabled[pairdev_in]);

        connect(checkBoxDisableFinger, QOverload<bool>::of(&QCheckBox::toggled), [=](bool f)
        {
            _note_finger_disabled[pairdev_in] = f;
            _settings->setValue("FingerPattern/FINGER_disabled" + (pairdev_in ? QString::number(pairdev_in) : ""), _note_finger_disabled[pairdev_in]);
            groupPlay->setDisabled(f);
        });

        comboBoxTypeDOWN->clear();
        comboBoxTypeDOWN->addItem(stringTypes[0]);
        comboBoxTypeDOWN->addItem(stringTypes[1]);
        comboBoxTypeDOWN->addItem(stringTypes[2]);
        comboBoxTypeDOWN->addItem(stringTypes[3]);
        comboBoxTypeDOWN->addItem(stringTypes[4]);

        indx = 0;

        if(_note_finger_pattern_DOWN[pairdev_in] < MAX_FINGER_BASIC) indx = _note_finger_pattern_DOWN[pairdev_in];

        for(int n = 0; n < 16; n++) {
            if(finger_patternDOWN[pairdev_in][n + MAX_FINGER_BASIC][FSTEPS]) {
                if(n + MAX_FINGER_BASIC == _note_finger_pattern_DOWN[pairdev_in])
                    indx= _note_finger_pattern_DOWN[pairdev_in];

            }

            comboBoxTypeDOWN->addItem("CUSTOM #" + QString::number(n + 1));
        }

        comboBoxTypeDOWN->setCurrentIndex(indx);

        connect(comboBoxTypeDOWN, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int num)
        {
            _note_finger_pattern_DOWN[pairdev_in] = num;
            _settings->setValue("FingerPattern/FINGER_pattern_DOWN" + (pairdev_in ? QString::number(pairdev_in) : ""), _note_finger_pattern_DOWN[pairdev_in]);
        });

        comboBoxTypeDOWN2->clear();
        comboBoxTypeDOWN2->addItem(stringTypes[0]);
        comboBoxTypeDOWN2->addItem(stringTypes[1]);
        comboBoxTypeDOWN2->addItem(stringTypes[2]);
        comboBoxTypeDOWN2->addItem(stringTypes[3]);
        comboBoxTypeDOWN2->addItem(stringTypes[4]);

        indx = 0;

        if(_note_finger_pattern_DOWN2[pairdev_in] < MAX_FINGER_BASIC) indx = _note_finger_pattern_DOWN2[pairdev_in];

        for(int n = 0; n < 16; n++) {
            if(finger_patternDOWN[pairdev_in][n + MAX_FINGER_BASIC][FSTEPS]) {
                if(n + MAX_FINGER_BASIC == _note_finger_pattern_DOWN2[pairdev_in])
                    indx= _note_finger_pattern_DOWN2[pairdev_in];

            }

            comboBoxTypeDOWN2->addItem("CUSTOM #" + QString::number(n + 1));
        }

        comboBoxTypeDOWN2->setCurrentIndex(indx);

        connect(comboBoxTypeDOWN2, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int num)
        {
            _note_finger_pattern_DOWN2[pairdev_in] = num;
            _settings->setValue("FingerPattern/FINGER_pattern_DOWN2" + (pairdev_in ? QString::number(pairdev_in) : ""), _note_finger_pattern_DOWN2[pairdev_in]);
        });

        comboBoxTypeDOWN3->clear();
        comboBoxTypeDOWN3->addItem(stringTypes[0]);
        comboBoxTypeDOWN3->addItem(stringTypes[1]);
        comboBoxTypeDOWN3->addItem(stringTypes[2]);
        comboBoxTypeDOWN3->addItem(stringTypes[3]);
        comboBoxTypeDOWN3->addItem(stringTypes[4]);

        indx = 0;

        if(_note_finger_pattern_DOWN3[pairdev_in] < MAX_FINGER_BASIC) indx = _note_finger_pattern_DOWN3[pairdev_in];

        for(int n = 0; n < 16; n++) {
            if(finger_patternDOWN[pairdev_in][n + MAX_FINGER_BASIC][FSTEPS]) {
                if(n + MAX_FINGER_BASIC == _note_finger_pattern_DOWN3[pairdev_in])
                    indx= _note_finger_pattern_DOWN3[pairdev_in];

            }

            comboBoxTypeDOWN3->addItem("CUSTOM #" + QString::number(n + 1));
        }

        comboBoxTypeDOWN3->setCurrentIndex(indx);

        connect(comboBoxTypeDOWN3, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int num)
        {
            _note_finger_pattern_DOWN3[pairdev_in] = num;
            _settings->setValue("FingerPattern/FINGER_pattern_DOWN3" + (pairdev_in ? QString::number(pairdev_in) : ""), _note_finger_pattern_DOWN3[pairdev_in]);
        });

        //***********

        connect(buttonBox, SIGNAL(accepted()), fingerPatternDialog, SLOT(accept()));
        connect(buttonBox, SIGNAL(rejected()), fingerPatternDialog, SLOT(reject()));
        connect(pushButtonTestNoteUP, SIGNAL(pressed()), fingerPatternDialog, SLOT(play_noteUP()));
        connect(pushButtonTestNoteUP, SIGNAL(released()), fingerPatternDialog, SLOT(stop_noteUP()));
        connect(pushButtonTestNoteDOWN, SIGNAL(pressed()), fingerPatternDialog, SLOT(play_noteDOWN()));
        connect(pushButtonTestNoteDOWN, SIGNAL(released()), fingerPatternDialog, SLOT(stop_noteDOWN()));

        thread_timer = NULL;

        time_update= new QTimer(this);
        time_update->setSingleShot(false);

        connect(time_update, SIGNAL(timeout()), this, SLOT(timer_dialog()), Qt::DirectConnection);

        time_update->start(50);

    } else {
// RESET FINGER
        fingerPatternDialog->setDisabled(true);

        for(int pairdev = 0; pairdev < MAX_INPUT_PAIR; pairdev++) {

            for(int n = 0; n < MAX_FINGER_PATTERN; n++) {
                memcpy(&finger_patternUP[pairdev][n][FTYPE], &finger_patternUP_ini[n][FTYPE], 19);
                memcpy(&finger_patternDOWN[pairdev][n][FTYPE], &finger_patternDOWN_ini[n][FTYPE], 19);

                if(_settings) {
                    QByteArray a =_settings->value("FingerPattern/FINGER_pattern_save" + (pairdev ? QString::number(pairdev) : "")).toByteArray();

                    Unpackfinger(pairdev, a);
                }
            }

            finger_switch_playUP[pairdev] = 0;
            finger_switch_playDOWN[pairdev] = 0;

            update_tick_counterUP[pairdev] = 0;
            update_tick_counterDOWN[pairdev] = 0;

            _note_finger_update_UP[pairdev] = 0;
            _note_finger_update_DOWN[pairdev] = 0;
            _note_finger_old_UP[pairdev] = -1;
            _note_finger_old_DOWN[pairdev] = -1;

            _note_finger_on_UP[pairdev] = 0;
            _note_finger_pick_on_UP[pairdev] = false;
            _note_finger_on_UP2[pairdev] = 0;

            _note_finger_on_DOWN[pairdev] = 0;
            _note_finger_pick_on_DOWN[pairdev] = 0;
            _note_finger_on_DOWN2[pairdev] = 0;

            max_tick_counterUP[pairdev] = 6;
            max_tick_counterDOWN[pairdev] = 6;

            finger_channelUP[pairdev] = 0;
            finger_channelDOWN[pairdev] = 0;

            finger_repeatUP[pairdev] = 0;
            finger_chord_modeUP[pairdev] = 1; // current pattern
            finger_repeatDOWN[pairdev] = 0;
            finger_chord_modeDOWN[pairdev] = 1; // current pattern

            finger_chord_modeUPEdit[pairdev] = 1; // current pattern
            finger_chord_modeDOWNEdit[pairdev] = 1; // current pattern

            finger_enable_UP[pairdev] = 0;
            finger_enable_DOWN[pairdev] = 0;

            _note_finger_disabled[pairdev] = _settings->value("FingerPattern/FINGER_disabled" + (pairdev ? QString::number(pairdev) : ""), false).toBool();

            _note_finger_pattern_UP[pairdev] = _settings->value("FingerPattern/FINGER_pattern_UP" + (pairdev ? QString::number(pairdev) : ""), 0).toInt();
            _note_finger_pressed_UP[pairdev] = false; //_settings->value("FingerPattern/FINGER_pressed_UP" + (pairdev ? QString::number(pairdev) : ""), false).toBool();
            _note_finger_pattern_UP2[pairdev] = _settings->value("FingerPattern/FINGER_pattern_UP2" + (pairdev ? QString::number(pairdev) : ""), 0).toInt();
            _note_finger_pressed_UP2[pairdev] = false; //_settings->value("FingerPattern/FINGER_pressed_UP2" + (pairdev ? QString::number(pairdev) : ""), false).toBool();
            _note_finger_pattern_UP3[pairdev] = _settings->value("FingerPattern/FINGER_pattern_UP3" + (pairdev ? QString::number(pairdev) : ""), 0).toInt();
            _note_finger_pressed_UP3[pairdev] = false; //_settings->value("FingerPattern/FINGER_pressed_UP3" + (pairdev ? QString::number(pairdev) : ""), false).toBool();

            _note_finger_pattern_DOWN[pairdev] = _settings->value("FingerPattern/FINGER_pattern_DOWN" + (pairdev ? QString::number(pairdev) : ""), 0).toInt();
            _note_finger_pressed_DOWN[pairdev] = false; //_settings->value("FingerPattern/FINGER_pressed_DOWN" + (pairdev ? QString::number(pairdev) : ""), false).toBool();
            _note_finger_pattern_DOWN2[pairdev] = _settings->value("FingerPattern/FINGER_pattern_DOWN2" + (pairdev ? QString::number(pairdev) : ""), 0).toInt();
            _note_finger_pressed_DOWN2[pairdev] = false; //_settings->value("FingerPattern/FINGER_pressed_DOWN2" + (pairdev ? QString::number(pairdev) : ""), false).toBool();
            _note_finger_pattern_DOWN3[pairdev] = _settings->value("FingerPattern/FINGER_pattern_DOWN3" + (pairdev ? QString::number(pairdev) : ""), 0).toInt();
            _note_finger_pressed_DOWN3[pairdev] = false; //_settings->value("FingerPattern/FINGER_pressed_DOWN3" + (pairdev ? QString::number(pairdev) : ""), false).toBool();

        }

#ifdef IS_QT5
        fingerMUX = new QMutex(QMutex::NonRecursive);
#else
        fingerMUX = new QMutex();
#endif

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

/////////////////////////////////////////////////////////////////////////
// finger timer

static int getKey(int pairdev_in, int zone, int n, int m, int type, int note, int note_disp) {

    int key2 = 0;

    int time_base = 2;
    bool semitone = false;

    note_disp&= 31;

    if(note & 64) {
        note &= 63;
        semitone = true;
    }

    if(note & 32) {
        time_base = 1;
        note &= 31;
    }

    switch(type) {

        case 0: {

            if(note >= 12) {
                key2 = -1;
                if(note > 12) {
                    if(!zone) {
                        if(time_base == 2)
                            map_key_step_maxUP[pairdev_in][n] = (map_key_step_maxUP[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX20;
                        else
                            map_key_step_maxUP[pairdev_in][n] = (map_key_step_maxUP[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX15;
                    } else {

                        if(time_base == 2)
                            map_key_step_maxDOWN[pairdev_in][n] = (map_key_step_maxDOWN[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX20;
                        else
                            map_key_step_maxDOWN[pairdev_in][n] = (map_key_step_maxDOWN[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX15;
                    }
                }

            } else {
                if(note > 5)  {
                    if(!zone) {
                        if(time_base == 2)
                            map_key_step_maxUP[pairdev_in][n] = (map_key_step_maxUP[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX20;
                        else
                            map_key_step_maxUP[pairdev_in][n] = (map_key_step_maxUP[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX15;
                    } else {

                        if(time_base == 2)
                            map_key_step_maxDOWN[pairdev_in][n] = (map_key_step_maxDOWN[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX20;
                        else
                            map_key_step_maxDOWN[pairdev_in][n] = (map_key_step_maxDOWN[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX15;
                    }
                }

                note %= 6;

                int key_off = MidiInControl::GetNoteChord(0, note_disp % 2, 12) % 12;
                m = (m/12)*12 + (m % 12) - key_off;
               // m = m - key_off;

                key2 = MidiInControl::GetNoteChord(0, note % 2, m + 12*(note/2) - 12);

                if(semitone && key2 < 127)
                    key2++;

            }

            break;
        }

        case 1: {

            if(note >= 24) {

                key2 = -1;

                if(note > 24) {
                    if(!zone) {
                        if(time_base == 2)
                            map_key_step_maxUP[pairdev_in][n] = (map_key_step_maxUP[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX20;
                        else
                            map_key_step_maxUP[pairdev_in][n] = (map_key_step_maxUP[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX15;
                    } else {

                        if(time_base == 2)
                            map_key_step_maxDOWN[pairdev_in][n] = (map_key_step_maxDOWN[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX20;
                        else
                            map_key_step_maxDOWN[pairdev_in][n] = (map_key_step_maxDOWN[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX15;
                    }
                }

            } else {

                if(note > 11)  {
                    if(!zone) {
                        if(time_base == 2)
                            map_key_step_maxUP[pairdev_in][n] = (map_key_step_maxUP[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX20;
                        else
                            map_key_step_maxUP[pairdev_in][n] = (map_key_step_maxUP[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX15;
                    } else {

                        if(time_base == 2)
                            map_key_step_maxDOWN[pairdev_in][n] = (map_key_step_maxDOWN[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX20;
                        else
                            map_key_step_maxDOWN[pairdev_in][n] = (map_key_step_maxDOWN[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX15;
                    }
                }

                note %= 12;

                int key_off = MidiInControl::GetNoteChord(3, note_disp % 4, 12) % 12;
                m = (m/12)*12 + (m % 12) - key_off;

                key2 = MidiInControl::GetNoteChord(3, note % 4, m + 12*(note/4) - 12);

                if(semitone && key2 < 127)
                    key2++;
            }

            break;
        }

        case 2: {

            if(note >= 24) {

                key2 = -1;

                if(note > 24) {
                    if(!zone) {
                        if(time_base == 2)
                            map_key_step_maxUP[pairdev_in][n] = (map_key_step_maxUP[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX20;
                        else
                            map_key_step_maxUP[pairdev_in][n] = (map_key_step_maxUP[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX15;
                    } else {

                        if(time_base == 2)
                            map_key_step_maxDOWN[pairdev_in][n] = (map_key_step_maxDOWN[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX20;
                        else
                            map_key_step_maxDOWN[pairdev_in][n] = (map_key_step_maxDOWN[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX15;
                    }
                }

            } else {

                if(note > 11) {
                    if(!zone) {
                        if(time_base == 2)
                            map_key_step_maxUP[pairdev_in][n] = (map_key_step_maxUP[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX20;
                        else
                            map_key_step_maxUP[pairdev_in][n] = (map_key_step_maxUP[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX15;
                    } else {

                        if(time_base == 2)
                            map_key_step_maxDOWN[pairdev_in][n] = (map_key_step_maxDOWN[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX20;
                        else
                            map_key_step_maxDOWN[pairdev_in][n] = (map_key_step_maxDOWN[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX15;
                    }
                }

                note %= 12;

                int key_off = MidiInControl::GetNoteChord(4, note_disp % 4, 12) % 12;
                m = (m/12)*12 + (m % 12) - key_off;

                key2 = MidiInControl::GetNoteChord(4, note % 4, m + 12*(note/4) - 12);

                if(semitone && key2 < 127)
                    key2++;

            }

            break;
        }

        case 3: {

            if(note >= 30) {

                key2 = -1;

                if(note > 30) {
                    if(!zone) {
                        if(time_base == 2)
                            map_key_step_maxUP[pairdev_in][n] = (map_key_step_maxUP[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX20;
                        else
                            map_key_step_maxUP[pairdev_in][n] = (map_key_step_maxUP[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX15;
                    } else {

                        if(time_base == 2)
                            map_key_step_maxDOWN[pairdev_in][n] = (map_key_step_maxDOWN[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX20;
                        else
                            map_key_step_maxDOWN[pairdev_in][n] = (map_key_step_maxDOWN[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX15;
                    }
                }

            } else {

                if(note > 14)  {
                    if(!zone) {
                        if(time_base == 2)
                            map_key_step_maxUP[pairdev_in][n] = (map_key_step_maxUP[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX20;
                        else
                            map_key_step_maxUP[pairdev_in][n] = (map_key_step_maxUP[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX15;
                    } else {

                        if(time_base == 2)
                            map_key_step_maxDOWN[pairdev_in][n] = (map_key_step_maxDOWN[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX20;
                        else
                            map_key_step_maxDOWN[pairdev_in][n] = (map_key_step_maxDOWN[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX15;
                    }
                }

                note %= 15;

                int key_off = MidiInControl::GetNoteChord(5, note_disp % 5, 12) % 12;
                m = (m/12)*12 + (m % 12) - key_off;

                key2 = MidiInControl::GetNoteChord(5, note % 5, m + 12*(note/5) - 12);

                if(semitone && key2 < 127)
                    key2++;

            }

            break;
        }

        case 4: {

            if(note >= 30) {

                key2 = -1;

                if(note > 30) {
                    if(!zone) {
                        if(time_base == 2)
                            map_key_step_maxUP[pairdev_in][n] = (map_key_step_maxUP[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX20;
                        else
                            map_key_step_maxUP[pairdev_in][n] = (map_key_step_maxUP[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX15;
                    } else {

                        if(time_base == 2)
                            map_key_step_maxDOWN[pairdev_in][n] = (map_key_step_maxDOWN[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX20;
                        else
                            map_key_step_maxDOWN[pairdev_in][n] = (map_key_step_maxDOWN[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX15;
                    }
                }

            } else {

                if(note > 14) {
                    if(!zone) {
                        if(time_base == 2)
                            map_key_step_maxUP[pairdev_in][n] = (map_key_step_maxUP[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX20;
                        else
                            map_key_step_maxUP[pairdev_in][n] = (map_key_step_maxUP[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX15;
                    } else {

                        if(time_base == 2)
                            map_key_step_maxDOWN[pairdev_in][n] = (map_key_step_maxDOWN[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX20;
                        else
                            map_key_step_maxDOWN[pairdev_in][n] = (map_key_step_maxDOWN[pairdev_in][n] & ~FLAG_TEMPO) | FLAG_TEMPOX15;
                    }
                }

                note %= 15;

                int key_off = MidiInControl::GetNoteChord(6, note_disp % 5, 12) % 12;
                m = (m/12)*12 + (m % 12) - key_off;


                key2 = MidiInControl::GetNoteChord(6, note % 5, m + 12*(note/5) - 12);

                if(semitone && key2 < 127)
                    key2++;

            }

            break;
        }
    }

    if(key2 > 127) key2 = 127;

    return key2;
}

void FingerPatternDialog::timer_dialog() {


    for(int pairdev = 0; pairdev < MAX_INPUT_PAIR; pairdev++) {

        unsigned int UP = update_tick_counterUP[pairdev];
        unsigned int DOWN = update_tick_counterDOWN[pairdev];


        if(pairdev == pairdev_out) {

            if((UP & 63) == 1 && finger_chord_modeUPEdit[pairdev] != _note_finger_pattern_UP[pairdev]) {

                for(int n = 0; n < comboBoxChordUP->count(); n++) {
                    int num = comboBoxChordUP->itemData(n).toInt();
                    if(num < 0)
                        num = (-num) - 1;
                    else
                        num += MAX_FINGER_BASIC;
                    if( num == _note_finger_pattern_UP[pairdev]) {
                        comboBoxChordUP->setCurrentIndex(n);
                        break;
                    }
                }
            }

            if((UP & 63) == 2 && finger_chord_modeUPEdit[pairdev] != _note_finger_pattern_UP2[pairdev]) {

                for(int n = 0; n < comboBoxChordUP->count(); n++) {
                    int num = comboBoxChordUP->itemData(n).toInt();
                    if(num < 0)
                        num = (-num) - 1;
                    else
                        num += MAX_FINGER_BASIC;
                    if( num == _note_finger_pattern_UP2[pairdev]) {
                        comboBoxChordUP->setCurrentIndex(n);
                        break;
                    }
                }
            }

            if((UP & 63) == 3 && finger_chord_modeUPEdit[pairdev] != _note_finger_pattern_UP3[pairdev]) {

                for(int n = 0; n < comboBoxChordUP->count(); n++) {
                    int num = comboBoxChordUP->itemData(n).toInt();
                    if(num < 0)
                        num = (-num) - 1;
                    else
                        num += MAX_FINGER_BASIC;
                    if( num == _note_finger_pattern_UP3[pairdev]) {
                        comboBoxChordUP->setCurrentIndex(n);
                        break;
                    }
                }
            }

            update_tick_counterUP[pairdev] &= ~63;

            if((DOWN & 63) == 33 && finger_chord_modeDOWNEdit[pairdev] != _note_finger_pattern_DOWN[pairdev]) {

                for(int n = 0; n < comboBoxChordDOWN->count(); n++) {
                    int num = comboBoxChordDOWN->itemData(n).toInt();
                    if(num < 0)
                        num = (-num) - 1;
                    else
                        num += MAX_FINGER_BASIC;
                    if( num == _note_finger_pattern_DOWN[pairdev]) {
                        comboBoxChordDOWN->setCurrentIndex(n);
                        break;
                    }
                }
            }

            if((DOWN & 63) == 34 && finger_chord_modeDOWNEdit[pairdev] != _note_finger_pattern_DOWN2[pairdev]) {

                for(int n = 0; n < comboBoxChordDOWN->count(); n++) {
                    int num = comboBoxChordDOWN->itemData(n).toInt();
                    if(num < 0)
                        num = (-num) - 1;
                    else
                        num += MAX_FINGER_BASIC;
                    if( num == _note_finger_pattern_DOWN2[pairdev]) {
                        comboBoxChordDOWN->setCurrentIndex(n);
                        break;
                    }
                }
            }

            if((DOWN & 63) == 35 && finger_chord_modeDOWNEdit[pairdev] != _note_finger_pattern_DOWN3[pairdev]) {

                for(int n = 0; n < comboBoxChordDOWN->count(); n++) {
                    int num = comboBoxChordDOWN->itemData(n).toInt();
                    if(num < 0)
                        num = (-num) - 1;
                    else
                        num += MAX_FINGER_BASIC;
                    if( num == _note_finger_pattern_DOWN3[pairdev]) {
                        comboBoxChordDOWN->setCurrentIndex(n);
                        break;
                    }
                }
            }

            update_tick_counterDOWN[pairdev] &= ~63;

            if((UP & 64) && finger_chord_modeUPEdit[pairdev] == finger_chord_modeUP[pairdev]) {

                if(update_tick_counterUP[pairdev] && FingerPatternWin->horizontalSliderTempoUP->value() != max_tick_counterUP[pairdev])
                    FingerPatternWin->horizontalSliderTempoUP->setValue(max_tick_counterUP[pairdev]);
            }

            update_tick_counterUP[pairdev] &= ~64;

            if((DOWN & 64) && finger_chord_modeDOWNEdit[pairdev] == finger_chord_modeDOWN[pairdev]) {

                if(update_tick_counterDOWN[pairdev] && FingerPatternWin->horizontalSliderTempoDOWN->value() != max_tick_counterDOWN[pairdev])
                    FingerPatternWin->horizontalSliderTempoDOWN->setValue(max_tick_counterDOWN[pairdev]);
            }

            update_tick_counterDOWN[pairdev] &= ~64;
        }

    }
}

bool FingerPatternDialog::finger_on(int device) {
    int pairdev = device / 2;

    if(device & 1)
        return ((finger_switch_playDOWN[pairdev] != 0) || (finger_enable_DOWN[pairdev] != 0));
    else
        return ((finger_switch_playUP[pairdev] != 0) || (finger_enable_UP[pairdev] != 0));

}

void FingerPatternDialog::finger_timer(int ndevice) {

    if(!fingerMUX) {
        qFatal("finger_timer() without fingerMUX defined!");
        return;
    }


    fingerMUX->lock();

    QByteArray messageByte;
    std::vector<unsigned char> message;


    // INITIALIZE MAPS KEYS

    if(init_map_key) {

        init_map_key = 0;

        for(int pairdev = 0; pairdev < MAX_INPUT_PAIR; pairdev++) {
            for(int n = 0; n < 128; n++) {

                map_keyUP[pairdev][n] = 0;
                map_key_stepUP[pairdev][n] = 0;
                map_key_onUP[pairdev][n] = 0;
                map_key_offUP[pairdev][n] = 0;

                map_keyDOWN[pairdev][n] = 0;
                map_key_stepDOWN[pairdev][n] = 0;
                map_key_onDOWN[pairdev][n] = 0;
                map_key_offDOWN[pairdev][n] = 0;
            }

        }
    }

    // timer loop
    for(int pairdev = 0; pairdev < MAX_INPUT_PAIR; pairdev++) {

        if(ndevice >= 0 && (ndevice>>1) != pairdev)
            continue;

        int ch_up = ((MidiInControl::channelUp(pairdev) < 0)
                         ? MidiOutput::standardChannel()
                         : MidiInControl::channelUp(pairdev)) & 15;
        int ch_down = ((MidiInControl::channelDown(pairdev) < 0)
                           ? ((MidiInControl::channelUp(pairdev) < 0)
                                  ? MidiOutput::standardChannel()
                                  : MidiInControl::channelUp(pairdev))
                           : MidiInControl::channelDown(pairdev)) & 0xF;

        // UP CONTROL KEYS

        if((ndevice < 0 || !(ndevice & 1)) && !finger_switch_playUP[pairdev]) {

            finger_enable_UP[pairdev] = 0;

            if(_note_finger_pick_on_UP[pairdev]) {

                finger_enable_UP[pairdev] = 1;

                if(_note_finger_on_UP[pairdev] != 2 &&
                    _note_finger_on_UP2[pairdev]!= 2 &&
                    _note_finger_on_UP3[pairdev])
                    max_tick_counterUP[pairdev] = 128;

                if(_note_finger_on_UP[pairdev] == 1)
                    _note_finger_on_UP[pairdev] = 0;

                if(_note_finger_on_UP2[pairdev] == 1)
                    _note_finger_on_UP2[pairdev] = 0;

                if(_note_finger_on_UP3[pairdev] == 1)
                    _note_finger_on_UP3[pairdev] = 0;


                for(int m = 0; m < 128; m++) {
                    if(_note_finger_update_UP[pairdev]) {
                        map_key_tempo_stepUP[pairdev][m] = 0;
                        map_key_stepUP[pairdev][m] &= ~127;
                        map_key_step_maxUP[pairdev][m] = (finger_patternUP[pairdev][finger_chord_modeUP[pairdev]][FREPEAT] << 8)
                                                     | (finger_patternUP[pairdev][finger_chord_modeUP[pairdev]][FSTEPS] & 127);
                    }
                }

                _note_finger_update_UP[pairdev] = 0;

            } else if(_note_finger_on_UP[pairdev]) {

                finger_enable_UP[pairdev] = 1;
                _note_finger_on_UP2[pairdev] = 0;
                _note_finger_on_UP3[pairdev] = 0;

                max_tick_counterUP[pairdev] = finger_patternUP[pairdev][_note_finger_pattern_UP[pairdev]][FTEMPO];
                finger_chord_modeUP[pairdev] = _note_finger_pattern_UP[pairdev];

                for(int m = 0; m < 128; m++) {
                    if(_note_finger_update_UP[pairdev]) {
                        if(_note_finger_update_UP[pairdev] == 2) map_key_stepUP[pairdev][m] &= ~127;
                        map_key_step_maxUP[pairdev][m] = (finger_patternUP[pairdev][finger_chord_modeUP[pairdev]][FREPEAT] << 8) | (finger_patternUP[pairdev][finger_chord_modeUP[pairdev]][FSTEPS] & 127);
                    } else if(map_key_step_maxUP[pairdev][m])
                        map_key_step_maxUP[pairdev][m] = (map_key_step_maxUP[pairdev][m] & ~127) | (finger_patternUP[pairdev][finger_chord_modeUP[pairdev]][FSTEPS] & 127);
                }

                _note_finger_update_UP[pairdev] = 0;

                finger_enable_UP[pairdev] = 1;

            } else if(_note_finger_on_UP2[pairdev]) {

                finger_enable_UP[pairdev] = 1;
                _note_finger_on_UP[pairdev] = 0;
                _note_finger_on_UP3[pairdev] = 0;

                max_tick_counterUP[pairdev] = finger_patternUP[pairdev][_note_finger_pattern_UP2[pairdev]][FTEMPO];
                finger_chord_modeUP[pairdev] = _note_finger_pattern_UP2[pairdev];

                for(int m = 0; m < 128; m++) {

                    if(_note_finger_update_UP[pairdev]) {
                        if(_note_finger_update_UP[pairdev] == 2) map_key_stepUP[pairdev][m] &= ~127;
                        map_key_step_maxUP[pairdev][m] = (finger_patternUP[pairdev][finger_chord_modeUP[pairdev]][FREPEAT] << 8) | (finger_patternUP[pairdev][finger_chord_modeUP[pairdev]][FSTEPS] & 127);
                    } else if(map_key_step_maxUP[pairdev][m])
                        map_key_step_maxUP[pairdev][m] = (map_key_step_maxUP[pairdev][m] & ~127) | (finger_patternUP[pairdev][finger_chord_modeUP[pairdev]][FSTEPS] & 127);
                }

                _note_finger_update_UP[pairdev] = 0;

                finger_enable_UP[pairdev] = 1;
            } else if(_note_finger_on_UP3[pairdev]) {

                    finger_enable_UP[pairdev] = 1;
                    _note_finger_on_UP[pairdev] = 0;
                    _note_finger_on_UP2[pairdev] = 0;

                    max_tick_counterUP[pairdev] = finger_patternUP[pairdev][_note_finger_pattern_UP3[pairdev]][FTEMPO];
                    finger_chord_modeUP[pairdev] = _note_finger_pattern_UP3[pairdev];

                    for(int m = 0; m < 128; m++) {

                        if(_note_finger_update_UP[pairdev]) {
                            if(_note_finger_update_UP[pairdev] == 2) map_key_stepUP[pairdev][m] &= ~127;
                            map_key_step_maxUP[pairdev][m] = (finger_patternUP[pairdev][finger_chord_modeUP[pairdev]][FREPEAT] << 8) | (finger_patternUP[pairdev][finger_chord_modeUP[pairdev]][FSTEPS] & 127);
                        } else if(map_key_step_maxUP[pairdev][m])
                            map_key_step_maxUP[pairdev][m] = (map_key_step_maxUP[pairdev][m] & ~127) | (finger_patternUP[pairdev][finger_chord_modeUP[pairdev]][FSTEPS] & 127);
                    }

                    _note_finger_update_UP[pairdev] = 0;

                    finger_enable_UP[pairdev] = 1;
                }

        }

        // DOWN CONTROL KEYS

        if((ndevice < 0 || (ndevice & 1)) && !finger_switch_playDOWN[pairdev]) {

            finger_enable_DOWN[pairdev] = 0;

            if(_note_finger_pick_on_DOWN[pairdev]) {

                finger_enable_DOWN[pairdev] = 1;

                if(_note_finger_on_DOWN[pairdev] != 2 &&
                    _note_finger_on_DOWN2[pairdev]!= 2 &&
                    _note_finger_on_DOWN3[pairdev]!= 2)
                    max_tick_counterDOWN[pairdev] = 128;

                if(_note_finger_on_DOWN[pairdev] == 1)
                    _note_finger_on_DOWN[pairdev] = 0;

                if(_note_finger_on_DOWN2[pairdev] == 1)
                    _note_finger_on_DOWN2[pairdev] = 0;

                if(_note_finger_on_DOWN3[pairdev] == 1)
                    _note_finger_on_DOWN3[pairdev] = 0;

                for(int m = 0; m < 128; m++) {
                    if(_note_finger_update_DOWN[pairdev]) {
                        map_key_tempo_stepUP[pairdev][m] = 0;
                        map_key_stepDOWN[pairdev][m] &= ~127;
                        map_key_step_maxDOWN[pairdev][m] = (finger_patternDOWN[pairdev][finger_chord_modeDOWN[pairdev]][FREPEAT] << 8) | (finger_patternDOWN[pairdev][finger_chord_modeDOWN[pairdev]][FSTEPS] & 127);
                    }
                }

                _note_finger_update_DOWN[pairdev] = 0;

            } else if(_note_finger_on_DOWN[pairdev]) {

                finger_enable_DOWN[pairdev] = 1;
                _note_finger_on_DOWN2[pairdev] = 0;
                _note_finger_on_DOWN3[pairdev] = 0;
                max_tick_counterDOWN[pairdev] = finger_patternDOWN[pairdev][_note_finger_pattern_DOWN[pairdev]][FTEMPO];

                int i = finger_chord_modeDOWN[pairdev] = _note_finger_pattern_DOWN[pairdev];

                for(int m = 0; m < 128; m++) {

                    if(_note_finger_update_DOWN[pairdev]) {
                        if(_note_finger_update_DOWN[pairdev] == 2) map_key_stepDOWN[pairdev][m] &= ~127;
                        map_key_step_maxDOWN[pairdev][m] = (finger_patternDOWN[pairdev][i][FREPEAT] << 8) | (finger_patternDOWN[pairdev][i][FSTEPS] & 127);
                    } else if(map_key_step_maxDOWN[pairdev][m])
                        map_key_step_maxDOWN[pairdev][m] = (map_key_step_maxDOWN[pairdev][m] & ~127) | (finger_patternDOWN[pairdev][i][FSTEPS] & 127);
                }

                _note_finger_update_DOWN[pairdev] = 0;

                finger_enable_DOWN[pairdev] = 1;

            } else if(_note_finger_on_DOWN2[pairdev]) {

                finger_enable_DOWN[pairdev] = 1;
                _note_finger_on_DOWN[pairdev] = 0;
                _note_finger_on_DOWN3[pairdev] = 0;

                max_tick_counterDOWN[pairdev] = finger_patternDOWN[pairdev][_note_finger_pattern_DOWN2[pairdev]][FTEMPO];
                finger_chord_modeDOWN[pairdev] = _note_finger_pattern_DOWN2[pairdev];

                for(int m = 0; m < 128; m++) {

                    if(_note_finger_update_DOWN[pairdev]) {
                        if(_note_finger_update_DOWN[pairdev] == 2) map_key_stepDOWN[pairdev][m] &= ~127;
                        map_key_step_maxDOWN[pairdev][m] = (finger_patternDOWN[pairdev][finger_chord_modeDOWN[pairdev]][FREPEAT] << 8) | (finger_patternDOWN[pairdev][finger_chord_modeDOWN[pairdev]][FSTEPS] & 127);
                    } else if(map_key_step_maxDOWN[pairdev][m])
                        map_key_step_maxDOWN[pairdev][m] = (map_key_step_maxDOWN[pairdev][m] & ~127) | (finger_patternDOWN[pairdev][finger_chord_modeDOWN[pairdev]][FSTEPS] & 127);
                }

                _note_finger_update_DOWN[pairdev] = 0;

                finger_enable_DOWN[pairdev] = 1;

            } else if(_note_finger_on_DOWN3[pairdev]) {

                finger_enable_DOWN[pairdev] = 1;
                _note_finger_on_DOWN[pairdev] = 0;
                _note_finger_on_DOWN2[pairdev] = 0;

                max_tick_counterDOWN[pairdev] = finger_patternDOWN[pairdev][_note_finger_pattern_DOWN3[pairdev]][FTEMPO];
                finger_chord_modeDOWN[pairdev] = _note_finger_pattern_DOWN3[pairdev];

                for(int m = 0; m < 128; m++) {

                    if(_note_finger_update_DOWN[pairdev]) {
                        if(_note_finger_update_DOWN[pairdev] == 2) map_key_stepDOWN[pairdev][m] &= ~127;
                        map_key_step_maxDOWN[pairdev][m] = (finger_patternDOWN[pairdev][finger_chord_modeDOWN[pairdev]][FREPEAT] << 8) | (finger_patternDOWN[pairdev][finger_chord_modeDOWN[pairdev]][FSTEPS] & 127);
                    } else if(map_key_step_maxDOWN[pairdev][m])
                        map_key_step_maxDOWN[pairdev][m] = (map_key_step_maxDOWN[pairdev][m] & ~127) | (finger_patternDOWN[pairdev][finger_chord_modeDOWN[pairdev]][FSTEPS] & 127);
                }

                _note_finger_update_DOWN[pairdev] = 0;

                finger_enable_DOWN[pairdev] = 1;
            }

        }


        // FINGER NOTE ENGINE

        for(int m = 0; m < 128; m++) {

            int flag_end = 0;

            // UP ZONE

            if((ndevice < 0 || !(ndevice & 1)) && ((finger_switch_playUP[pairdev] || finger_enable_UP[pairdev]) && (map_key_stepUP[pairdev][m] & 128))) {

                int current_index = map_key_stepUP[pairdev][m] & 127;

                if(map_key_tempo_stepUP[pairdev][m] == 0) {

                    map_key_step_maxUP[pairdev][m] &= ~FLAG_TEMPO;

                    for(int n = 0; n < 128; n++) {

                        int key2 = map_keyUP[pairdev][n];

                        if((key2 & 128) && (key2 & 127) == m) {
                            map_key_offUP[pairdev][n]=1;
                        }
                    }


                    for(int n = 0; n < 128; n++) {

                        int key2 = m;

                        if(!_note_finger_pick_on_UP[pairdev]) {

                            key2 = getKey(pairdev, 0, n, m, finger_patternUP[pairdev][finger_chord_modeUP[pairdev]][FTYPE],
                                          finger_patternUP[pairdev][finger_chord_modeUP[pairdev]][FNOTE1 + current_index], finger_patternUP[pairdev][finger_chord_modeUP[pairdev]][FNOTE1]);
                        }

                        if(key2 >= 0 && (map_key_stepUP[pairdev][n] & 128)) {

                            map_keyUP[pairdev][key2] = m | 128;
                            map_key_onUP[pairdev][key2] = 1;
                        }

                    }

                    if(finger_enable_UP[pairdev] && map_key_step_maxUP[pairdev][m] != 0)
                        current_index++;

                    if(current_index >= ((map_key_step_maxUP[pairdev][m] & 127))) {

                        current_index = 0;

                        flag_end = 1;

                        if(flag_end) {

                            int loop = (map_key_step_maxUP[pairdev][m] >> 8) & 7;

                            if(loop) {

                                loop--;

                                map_key_step_maxUP[pairdev][m] = (map_key_step_maxUP[pairdev][m] & 127)
                                                             | (loop << 8) | (map_key_step_maxUP[pairdev][m] & FLAG_TEMPO);

                                if(!loop) {
                                    map_key_step_maxUP[pairdev][m] = 0;
                                }
                            }
                        }
                    }

                    map_key_stepUP[pairdev][m]= current_index  | (map_key_stepUP[pairdev][m] & ~127);
                }

                int tick_counter;

                if(map_key_step_maxUP[pairdev][m])
                    map_key_tempo_stepUP[pairdev][m]++;
                else
                    map_key_tempo_stepUP[pairdev][m] = 1;

                tick_counter = map_key_tempo_stepUP[pairdev][m];

                int maxcounter = (max_tick_counterUP[pairdev] + 1);

                if(maxcounter > 64) maxcounter = 64;

                if(map_key_step_maxUP[pairdev][m] & FLAG_TEMPOX15) {

                    maxcounter= maxcounter * 15/10;

                } else if(map_key_step_maxUP[pairdev][m] & FLAG_TEMPOX20) {
                    maxcounter*= 2;
                    // qDebug("double up");
                }

                if(tick_counter >= maxcounter) {
                    tick_counter = 0;
                }

                map_key_tempo_stepUP[pairdev][m] = tick_counter;
            }

            flag_end = 0;

            // DOWN ZONE

            if((ndevice < 0 || (ndevice & 1)) && (finger_switch_playDOWN[pairdev] || finger_enable_DOWN[pairdev]) && (map_key_stepDOWN[pairdev][m] & 128)) {

                int current_index = map_key_stepDOWN[pairdev][m] & 127;

                if(map_key_tempo_stepDOWN[pairdev][m] == 0) {

                    map_key_step_maxDOWN[pairdev][m] &= ~FLAG_TEMPO;

                    for(int n = 0; n < 128; n++) {

                        int key2 = map_keyDOWN[pairdev][n];

                        if((key2 & 128) && (key2 & 127) == m) {
                            map_key_offDOWN[pairdev][n]=1;
                        }
                    }

                    for(int n = 0; n < 128; n++) {

                        int key2 = m;

                        if(!_note_finger_pick_on_DOWN[pairdev]) {

                            key2 = getKey(pairdev, 1, n, m, finger_patternDOWN[pairdev][finger_chord_modeDOWN[pairdev]][FTYPE],
                                          finger_patternDOWN[pairdev][finger_chord_modeDOWN[pairdev]][FNOTE1 + current_index], finger_patternDOWN[pairdev][finger_chord_modeDOWN[pairdev]][FNOTE1]);
                        }

                        if(key2 >= 0 && (map_key_stepDOWN[pairdev][n] & 128)) {

                            map_keyDOWN[pairdev][key2] = m | 128;
                            map_key_onDOWN[pairdev][key2] = 1;
                        }

                    }

                    if(finger_enable_DOWN[pairdev] && map_key_step_maxDOWN[pairdev][m]!=0)
                        current_index++;

                    if(current_index >= ((map_key_step_maxDOWN[pairdev][m] & 127))) {

                        current_index = 0;

                        flag_end = 1;

                        if(flag_end) {

                            int loop = (map_key_step_maxDOWN[pairdev][m] >> 8) & 7;

                            if(loop) {

                                loop--;

                                map_key_step_maxDOWN[pairdev][m] = (map_key_step_maxDOWN[pairdev][m] & 127)
                                                               | (loop << 8) | (map_key_step_maxDOWN[pairdev][m] & FLAG_TEMPO);

                                if(!loop) {
                                    map_key_step_maxDOWN[pairdev][m] = 0;
                                }
                            }
                        }
                    }

                    map_key_stepDOWN[pairdev][m]= current_index  | (map_key_stepDOWN[pairdev][m] & ~127);
                }

                int tick_counter;

                if(map_key_step_maxDOWN[pairdev][m])
                    map_key_tempo_stepDOWN[pairdev][m]++;
                else
                    map_key_tempo_stepDOWN[pairdev][m] = 1;

                tick_counter = map_key_tempo_stepDOWN[pairdev][m];

                int maxcounter = (max_tick_counterDOWN[pairdev] + 1);

                if(maxcounter > 64) maxcounter = 64;

                if(map_key_step_maxDOWN[pairdev][m] & FLAG_TEMPOX15) {

                    maxcounter = maxcounter * 15/10;

                } else if(map_key_step_maxDOWN[pairdev][m] & FLAG_TEMPOX20) {

                    maxcounter*= 2;
                    // qDebug("double up");
                }


                if(tick_counter >= maxcounter) {
                    tick_counter = 0;
                }

                map_key_tempo_stepDOWN[pairdev][m] = tick_counter;
            }

        }

        // FORCING RESET KEY TEMPO COUNTER

        for(int n = 0; n < 128; n++) {
            if((ndevice < 0 || !(ndevice & 1)) && !finger_enable_UP[pairdev])
                map_key_tempo_stepUP[pairdev][n] = 0;

            if((ndevice < 0 || (ndevice & 1)) && !finger_enable_DOWN[pairdev])
                map_key_tempo_stepDOWN[pairdev][n] = 0;
        }


        // FINGER PLAY NOTES ON/OFF

        for(int n = 0; n < 128; n++) {

            // UP ZONE NOTE OFF

            if((ndevice < 0 || !(ndevice & 1)) && map_key_offUP[pairdev][n]) {

                if(!finger_switch_playUP[pairdev]) {

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

                finger_token[0] = map_keyUP[pairdev][n] & 127;
                finger_token[1] = pairdev;

                if(!finger_switch_playUP[pairdev]) {
                    fingerMUX->unlock();
                    MidiInput::receiveMessage_mutex(0.0, &message, finger_token);
                    fingerMUX->lock();
                } else {
                    _track_index = MidiInput::track_index;
                    MidiOutput::sendCommand(messageByte, _track_index);
                }

                map_key_offUP[pairdev][n]--;
            }

            // DOWN ZONE NOTE OFF

            if((ndevice < 0 || (ndevice & 1)) && map_key_offDOWN[pairdev][n]) {

                if(!finger_switch_playDOWN[pairdev]) {

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

                finger_token[0] = map_keyDOWN[pairdev][n] & 127;
                finger_token[1] = pairdev;

                if(!finger_switch_playDOWN[pairdev]) {
                    fingerMUX->unlock();
                    MidiInput::receiveMessage_mutex(0.0, &message, finger_token);
                    fingerMUX->lock();
                } else {
                    _track_index = MidiInput::track_index;
                    MidiOutput::sendCommand(messageByte, _track_index);
                }

                map_key_offDOWN[pairdev][n]--;
            }

            // UP ZONE NOTE ON

            if((ndevice < 0 || !(ndevice & 1)) && map_key_onUP[pairdev][n]) {

                if(!finger_switch_playUP[pairdev]) {

                    message.clear();
                    message.emplace_back((unsigned char) 0x90 + ch_up);
                    message.emplace_back((unsigned char) n);
                    message.emplace_back((unsigned char) map_key_velocityUP[pairdev][map_keyUP[pairdev][n] & 127]);
                } else {

                    messageByte.clear();
                    messageByte.append((unsigned char) 0x90 + ch_up);
                    messageByte.append((unsigned char) n);
                    messageByte.append((unsigned char) map_key_velocityUP[pairdev][map_keyUP[pairdev][n] & 127]);
                }

                finger_token[0] = map_keyUP[pairdev][n] & 127;
                finger_token[1] = pairdev;

                if(!finger_switch_playUP[pairdev]) {
                    fingerMUX->unlock();
                    MidiInput::receiveMessage_mutex(0.0, &message, finger_token);
                    fingerMUX->lock();
                } else {
                    _track_index = MidiInput::track_index;
                    MidiOutput::sendCommand(messageByte, _track_index);
                }

                map_key_onUP[pairdev][n] = 0;
            }

            // DOWN ZONE NOTE ON

            if((ndevice < 0 || (ndevice & 1)) && map_key_onDOWN[pairdev][n]) {

                if(!finger_switch_playDOWN[pairdev]) {

                    message.clear();
                    message.emplace_back((unsigned char) 0x90 + ch_down);
                    message.emplace_back((unsigned char) n);
                    message.emplace_back((unsigned char) map_key_velocityDOWN[pairdev][map_keyDOWN[pairdev][n] & 127]);
                } else {

                    messageByte.clear();
                    messageByte.append((unsigned char) 0x90 + ch_down);
                    messageByte.append((unsigned char) n);
                    messageByte.append((unsigned char) map_key_velocityDOWN[pairdev][map_keyDOWN[pairdev][n] & 127]);
                }

                finger_token[0] = map_keyDOWN[pairdev][n] & 127;
                finger_token[1] = pairdev;

                if(!finger_switch_playDOWN[pairdev]) {
                    fingerMUX->unlock();
                    MidiInput::receiveMessage_mutex(0.0, &message, finger_token);
                    fingerMUX->lock();
                } else {
                    _track_index = MidiInput::track_index;
                    MidiOutput::sendCommand(messageByte, _track_index);
                }

                map_key_onDOWN[pairdev][n] = 0;
            }

        }
    }
    fingerMUX->unlock();
}

// FROM DIALOG

void FingerPatternDialog::play_noteUP() {

    std::vector<unsigned char> message;
    QByteArray messageByte;

    int ch_up = ((MidiInControl::channelUp(pairdev_out) < 0)
                     ? MidiOutput::standardChannel()
                     : MidiInControl::channelUp(pairdev_out)) & 15;

    if(init_map_key) {

        init_map_key = 0;

        for(int dev = 0; dev < MAX_INPUT_PAIR; dev++) {
            for(int n = 0; n < 128; n++) {
                map_keyUP[dev][n] = 0;
                map_key_stepUP[dev][n] = 0;
                map_key_onUP[dev][n] = 0;
                map_key_offUP[dev][n] = 0;

                map_keyDOWN[dev][n] = 0;
                map_key_stepDOWN[dev][n] = 0;
                map_key_onDOWN[dev][n] = 0;
                map_key_offDOWN[dev][n] = 0;
            }
        }
    }

    _note_finger_on_UP[pairdev_out] = 0;
    _note_finger_on_UP2[pairdev_out] = 0;
    _note_finger_pick_on_UP[pairdev_out] = 0;

    finger_key = finger_base_note;
    finger_enable_UP[pairdev_out] = 1;
    finger_switch_playUP[pairdev_out] = 1;
    finger_chord_modeUP[pairdev_out] = finger_chord_modeUPEdit[pairdev_out];
    max_tick_counterUP[pairdev_out] = finger_patternUP[pairdev_out][finger_chord_modeUPEdit[pairdev_out]][FTEMPO];

    for(int n = 0; n < 128; n++) {

        if(map_keyUP[pairdev_out][n] == (128 | finger_key)) {

            map_keyUP[pairdev_out][n] = 0;

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

                finger_token[0] = finger_key;
                finger_token[1] = pairdev_out;
                MidiInput::receiveMessage(0.0, &message, finger_token);
            } else {
                _track_index = MidiInput::track_index;
                MidiOutput::sendCommand(messageByte, _track_index);
            }


        }
    }

    int steps = finger_patternUP[pairdev_out][finger_chord_modeUP[pairdev_out]][FSTEPS];

           //qDebug("on");

    map_key_tempo_stepUP[pairdev_out][finger_key] = 0;
    map_key_step_maxUP[pairdev_out][finger_key] = steps | (finger_repeatUP[pairdev_out] << 8);
    map_key_stepUP[pairdev_out][finger_key] = 128;

    map_key_velocityUP[pairdev_out][finger_key] = 100;

    map_keyUP[pairdev_out][finger_key] = finger_key | 128;

}

// FROM DIALOG

void FingerPatternDialog::stop_noteUP() {

    std::vector<unsigned char> message;
    QByteArray messageByte;

    int ch_up = ((MidiInControl::channelUp(pairdev_out) < 0)
                     ? MidiOutput::standardChannel()
                     : MidiInControl::channelUp(pairdev_out)) & 15;

    finger_switch_playUP[pairdev_out] = 1;

    finger_key = finger_base_note;

    if(init_map_key) {

        init_map_key = 0;

        for(int dev = 0; dev < MAX_INPUT_PAIR; dev++) {
            for(int n = 0; n < 128; n++) {

                map_keyUP[pairdev_out][n] = 0;
                map_key_stepUP[pairdev_out][n] = 0;
                map_key_onUP[pairdev_out][n] = 0;
                map_key_offUP[pairdev_out][n] = 0;

                map_keyDOWN[pairdev_out][n] = 0;
                map_key_stepDOWN[pairdev_out][n] = 0;
                map_key_onDOWN[pairdev_out][n] = 0;
                map_key_offDOWN[pairdev_out][n] = 0;
            }
        }
    }

    for(int n = 0; n < 128; n++) {

        if(map_keyUP[pairdev_out][n] == (128 | finger_key)) {

            map_keyUP[pairdev_out][n] = 0;

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

                finger_token[0] = finger_key;
                finger_token[1] = pairdev_out;

                MidiInput::receiveMessage(0.0, &message, finger_token);

            } else {
                _track_index = MidiInput::track_index;
                MidiOutput::sendCommand(messageByte, _track_index);
            }

        }
    }

    map_key_stepUP[pairdev_out][finger_key] = 0;

    finger_enable_UP[pairdev_out] = 0;

           //qDebug("off");

}

// FROM DIALOG

void FingerPatternDialog::play_noteDOWN() {

    std::vector<unsigned char> message;
    QByteArray messageByte;

    int ch_down = ((MidiInControl::channelDown(pairdev_out) < 0)
                       ? ((MidiInControl::channelUp(pairdev_out) < 0)
                              ? MidiOutput::standardChannel()
                              : MidiInControl::channelUp(pairdev_out))
                       : MidiInControl::channelDown(pairdev_out)) & 0xF;

    if(init_map_key) {

        init_map_key = 0;

        for(int dev = 0; dev < MAX_INPUT_PAIR; dev++) {
            for(int n = 0; n < 128; n++) {

                map_keyUP[dev][n] = 0;
                map_key_stepUP[dev][n] = 0;
                map_key_onUP[dev][n] = 0;
                map_key_offUP[dev][n] = 0;

                map_keyDOWN[dev][n] = 0;
                map_key_stepDOWN[dev][n] = 0;
                map_key_onDOWN[dev][n] = 0;
                map_key_offDOWN[dev][n] = 0;
            }
        }
    }

    _note_finger_on_DOWN[pairdev_out] = 0;
    _note_finger_on_DOWN2[pairdev_out] = 0;
    _note_finger_pick_on_DOWN[pairdev_out] = 0;

    finger_key = finger_base_note;

    finger_chord_modeDOWN[pairdev_out] = finger_chord_modeDOWNEdit[pairdev_out];
    max_tick_counterDOWN[pairdev_out] = finger_patternDOWN[pairdev_out][finger_chord_modeDOWNEdit[pairdev_out]][FTEMPO];

    finger_enable_DOWN[pairdev_out] = 1;
    finger_switch_playDOWN[pairdev_out] = 1;

    for(int n = 0; n < 128; n++) {

        if(map_keyDOWN[pairdev_out][n] == (128 | finger_key)) {

            map_keyDOWN[pairdev_out][n] = 0;

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

                finger_token[0] = finger_key;
                finger_token[1] = pairdev_out;
                MidiInput::receiveMessage(0.0, &message, finger_token);

            } else {
                _track_index = MidiInput::track_index;
                MidiOutput::sendCommand(messageByte, _track_index);
            }


        }
    }

    int steps = finger_patternDOWN[pairdev_out][finger_chord_modeDOWN[pairdev_out]][FSTEPS];

           //qDebug("on");
    map_key_tempo_stepDOWN[pairdev_out][finger_key] = 0;
    map_key_step_maxDOWN[pairdev_out][finger_key] = steps | (finger_repeatDOWN[pairdev_out] << 8);
    map_key_stepDOWN[pairdev_out][finger_key] = 128;

    map_key_velocityDOWN[pairdev_out][finger_key] = 100;

    map_keyDOWN[pairdev_out][finger_key] = finger_key | 128;


}

// FROM DIALOG

void FingerPatternDialog::stop_noteDOWN() {

    std::vector<unsigned char> message;
    QByteArray messageByte;

    int ch_down = ((MidiInControl::channelDown(pairdev_out) < 0)
                       ? ((MidiInControl::channelUp(pairdev_out) < 0)
                              ? MidiOutput::standardChannel()
                              : MidiInControl::channelUp(pairdev_out))
                       : MidiInControl::channelDown(pairdev_out)) & 0xF;

    finger_switch_playDOWN[pairdev_out] = 1;

    finger_key = finger_base_note;

    if(init_map_key) {

        init_map_key = 0;

        for(int dev = 0; dev < MAX_INPUT_PAIR; dev++) {
            for(int n = 0; n < 128; n++) {

                map_keyUP[dev][n] = 0;
                map_key_stepUP[dev][n] = 0;
                map_key_onUP[dev][n] = 0;
                map_key_offUP[dev][n] = 0;

                map_keyDOWN[dev][n] = 0;
                map_key_stepDOWN[dev][n] = 0;
                map_key_onDOWN[dev][n] = 0;
                map_key_offDOWN[dev][n] = 0;
            }
        }
    }

    for(int n = 0; n < 128; n++) {

        if(map_keyDOWN[pairdev_out][n] == (128 | finger_key)) {

            map_keyDOWN[pairdev_out][n] = 0;

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

                finger_token[0] = finger_key;
                finger_token[1] = pairdev_out;
                MidiInput::receiveMessage(0.0, &message, finger_token);

            } else {
                _track_index = MidiInput::track_index;
                MidiOutput::sendCommand(messageByte, _track_index);
            }

        }
    }

    map_key_stepDOWN[pairdev_out][finger_key] = 0;

    finger_enable_DOWN[pairdev_out] = 0;

           //qDebug("off");

}

int FingerPatternDialog::Finger_Action_time(int dev_in, int token, int value) {

    if(!fingerMUX) return 0;
    fingerMUX->lock();

    int pairdev_in = dev_in / 2;

    if(token < 32) {

        if(value >= 127)
            value = 128;
        if(value < 0)
            value = 0;

        value/=2;


        // 0 y 64
        if(finger_chord_modeUPEdit[pairdev_in] == finger_chord_modeUP[pairdev_in])
            max_tick_counterUP[pairdev_in] = value;
        finger_patternUP[pairdev_in][finger_chord_modeUP[pairdev_in]][FTEMPO] = value;

        update_tick_counterUP[pairdev_in] |= 64;

    } else {

        if(value >= 127)
            value = 128;
        if(value < 0)
            value = 0;

        value/=2;

        // 0 y 64
        if(finger_chord_modeDOWNEdit[pairdev_in] == finger_chord_modeDOWN[pairdev_in])
            max_tick_counterDOWN[pairdev_in] = value;
        finger_patternDOWN[pairdev_in][finger_chord_modeDOWN[pairdev_in]][FTEMPO] = value;

        update_tick_counterDOWN[pairdev_in] |= 64;
    }

    fingerMUX->unlock();
    return 0;
}

void FingerPatternDialog::Finger_Action(int dev_in, int token, bool switch_on, int value) {


    if(!fingerMUX) return;
    fingerMUX->lock();

    int pairdev_in = dev_in / 2;

    QString text = "Dev: " + QString::number(dev_in) + " - FINGER PATTERN -";

    finger_switch_playUP[pairdev_in] = 0;
    finger_switch_playDOWN[pairdev_in] = 0;

    if(value >= 64) {

        if(token == 1) { //_note_finger_UP[pairdev_in]

            _note_finger_pressed_UP[pairdev_in] = switch_on;

            int pressed = _note_finger_on_UP[pairdev_in];

            if(switch_on)
                pressed = pressed ? 0 : 2;
            else
                pressed = 1;

            if(pressed) {

                _note_finger_on_UP3[pairdev_in] = 0;
                _note_finger_on_UP2[pairdev_in] = 0;
                _note_finger_pick_on_UP[pairdev_in] = 0;

                if(_note_finger_old_UP[pairdev_in] != token)
                    _note_finger_update_UP[pairdev_in] = 2;
                else
                    _note_finger_update_UP[pairdev_in] = 1;

                _note_finger_old_UP[pairdev_in] = token;

            }

            if(pressed)
                text+= " #1 UP - ON";
            else
                text+= " #1 UP - OFF";

            MidiInControl::OSD = text;

            _note_finger_on_UP[pairdev_in] = pressed;

            update_tick_counterUP[pairdev_in] = token;

            fingerMUX->unlock();
            return;
        }

        if(token == 2) { //_note_finger_UP2[pairdev_in]

            _note_finger_pressed_UP2[pairdev_in] = switch_on;

            int pressed = _note_finger_on_UP2[pairdev_in];

            if(switch_on)
                pressed = pressed ? 0 : 2;
            else
                pressed = 1;

            if(pressed) {

                _note_finger_on_UP[pairdev_in] = 0;
                _note_finger_on_UP3[pairdev_in] = 0;
                _note_finger_pick_on_UP[pairdev_in] = 0;

                if(_note_finger_old_UP[pairdev_in] != token)
                    _note_finger_update_UP[pairdev_in] = 2;
                else
                    _note_finger_update_UP[pairdev_in] = 1;

                _note_finger_old_UP[pairdev_in] = token;

            }

            if(pressed)
                text+= " #2 UP - ON";
            else
                text+= " #2 UP - OFF";

            MidiInControl::OSD = text;

            _note_finger_on_UP2[pairdev_in] = pressed;

            update_tick_counterUP[pairdev_in] = token;

            fingerMUX->unlock();
            return;
        }

        if(token == 3) { //_note_finger_UP3[pairdev_in]

            _note_finger_pressed_UP3[pairdev_in] = switch_on;

            int pressed = _note_finger_on_UP3[pairdev_in];

            if(switch_on)
                pressed = pressed ? 0 : 2;
            else
                pressed = 1;

            if(pressed) {

                _note_finger_on_UP[pairdev_in] = 0;
                _note_finger_on_UP2[pairdev_in] = 0;
                _note_finger_pick_on_UP[pairdev_in] = 0;

                if(_note_finger_old_UP[pairdev_in] != token)
                    _note_finger_update_UP[pairdev_in] = 2;
                else
                    _note_finger_update_UP[pairdev_in] = 1;

                _note_finger_old_UP[pairdev_in] = token;

            }

            if(pressed)
                text+= " #3 UP - ON";
            else
                text+= " #3 UP - OFF";

            MidiInControl::OSD = text;

            _note_finger_on_UP3[pairdev_in] = pressed;

            update_tick_counterUP[pairdev_in] = token;

            fingerMUX->unlock();
            return;
        }

        if(token == 0) { //_note_finger_pick_UP[pairdev_in]

            if(_note_finger_old_UP[pairdev_in] != token)
                _note_finger_update_UP[pairdev_in] = 2;
            else
                _note_finger_update_UP[pairdev_in] = 1;

            _note_finger_old_UP[pairdev_in] = token;
            _note_finger_pick_on_UP[pairdev_in] = true;

            text+= " PICK UP - ON";
            MidiInControl::OSD = text;

            fingerMUX->unlock();
            return;
        }

        if(token == 33) { //_note_finger_DOWN[pairdev_in]
            _note_finger_pressed_DOWN[pairdev_in] = switch_on;

            int pressed = _note_finger_on_DOWN[pairdev_in];

            if(switch_on)
                pressed = pressed ? 0 : 2;
            else
                pressed = 1;

            if(pressed) {

                _note_finger_on_DOWN3[pairdev_in] = 0;
                _note_finger_on_DOWN2[pairdev_in] = 0;
                _note_finger_pick_on_DOWN[pairdev_in] = 0;

                if(_note_finger_old_DOWN[pairdev_in] != token)
                    _note_finger_update_DOWN[pairdev_in] = 2;
                else
                    _note_finger_update_DOWN[pairdev_in] = 1;

                _note_finger_old_DOWN[pairdev_in] = token;

            }

            if(pressed)
                text+= " #1 DOWN - ON";
            else
                text+= " #1 DOWN - OFF";

            MidiInControl::OSD = text;

            _note_finger_on_DOWN[pairdev_in] = pressed;

            update_tick_counterDOWN[pairdev_in] = token;

            fingerMUX->unlock();
            return;
        }

        if(token == 34) { //_note_finger_DOWN2[pairdev_in]
            _note_finger_pressed_DOWN2[pairdev_in] = switch_on;

            int pressed = _note_finger_on_DOWN2[pairdev_in];

            if(switch_on)
                pressed = pressed ? 0 : 2;
            else
                pressed = 1;

            if(pressed) {

                _note_finger_on_DOWN[pairdev_in] = 0;
                _note_finger_on_DOWN3[pairdev_in] = 0;
                _note_finger_pick_on_DOWN[pairdev_in] = 0;

                if(_note_finger_old_DOWN[pairdev_in] != token)
                    _note_finger_update_DOWN[pairdev_in] = 2;
                else
                    _note_finger_update_DOWN[pairdev_in] = 1;

                _note_finger_old_DOWN[pairdev_in] = token;

            }

            if(pressed)
                text+= " #2 DOWN - ON";
            else
                text+= " #2 DOWN - OFF";

            MidiInControl::OSD = text;

            _note_finger_on_DOWN2[pairdev_in] = pressed;

            update_tick_counterDOWN[pairdev_in] = token;

            fingerMUX->unlock();
            return;
        }

        if(token == 35) { //_note_finger_DOWN3[pairdev_in]

            _note_finger_pressed_DOWN3[pairdev_in] = switch_on;

            int pressed = _note_finger_on_DOWN3[pairdev_in];

            if(switch_on)
                pressed = pressed ? 0 : 2;
            else
                pressed = 1;

            if(pressed) {

                _note_finger_on_DOWN[pairdev_in] = 0;
                _note_finger_on_DOWN2[pairdev_in] = 0;
                _note_finger_pick_on_DOWN[pairdev_in] = 0;

                if(_note_finger_old_DOWN[pairdev_in] != token)
                    _note_finger_update_DOWN[pairdev_in] = 2;
                else
                    _note_finger_update_DOWN[pairdev_in] = 1;

                _note_finger_old_DOWN[pairdev_in] = token;

            }

            if(pressed)
                text+= " #3 DOWN - ON";
            else
                text+= " #3 DOWN - OFF";

            MidiInControl::OSD = text;

            _note_finger_on_DOWN3[pairdev_in] = pressed;

            update_tick_counterDOWN[pairdev_in] = token;

            fingerMUX->unlock();
            return;
        }

        if(token == 32) { //_note_finger_pick_UP[pairdev_in]

            if(_note_finger_old_DOWN[pairdev_in] != token)
                _note_finger_update_DOWN[pairdev_in] = 2;
            else
                _note_finger_update_DOWN[pairdev_in] = 1;

            _note_finger_old_DOWN[pairdev_in] = token;
            _note_finger_pick_on_DOWN[pairdev_in] = true;

            text+= " PICK DOWN - ON";
            MidiInControl::OSD = text;

            fingerMUX->unlock();
            return;
        }


    } else { // off

        if(token == 1) { // _note_finger_UP[pairdev_in]

            if(!_note_finger_pressed_UP[pairdev_in] || value == 1) _note_finger_on_UP[pairdev_in] = 0;

            if(value == 1) {
                std::vector<unsigned char> message;

                int ch_up = ((MidiInControl::channelUp(pairdev_in) < 0)
                                 ? MidiOutput::standardChannel()
                                 : MidiInControl::channelUp(pairdev_in)) & 15;

                for(int n = 0; n < 128; n++) {

                    if(map_keyUP[pairdev_in][n] & 128) {

                        int finger_key = map_keyUP[pairdev_in][n] & 127;
                        map_keyUP[pairdev_in][n] = 0;

                        message.clear();
                        message.emplace_back((unsigned char) 0x80 + ch_up);
                        message.emplace_back((unsigned char) n);
                        message.emplace_back((unsigned char) 0);

                        finger_token[0] = finger_key;
                        finger_token[1] = pairdev_in;
                        MidiInput::receiveMessage(0.0, &message, finger_token);

                    }
                }

            }

            fingerMUX->unlock();
            return;
        }

        if(token == 2) { // _note_finger_UP2[pairdev_in]

            if(!_note_finger_pressed_UP2[pairdev_in] || value == 1) _note_finger_on_UP2[pairdev_in] = 0;

            if(value == 1) {
                std::vector<unsigned char> message;

                int ch_up = ((MidiInControl::channelUp(pairdev_in) < 0)
                                 ? MidiOutput::standardChannel()
                                 : MidiInControl::channelUp(pairdev_in)) & 15;

                for(int n = 0; n < 128; n++) {

                    if(map_keyUP[pairdev_in][n] & 128) {

                        int finger_key = map_keyUP[pairdev_in][n] & 127;
                        map_keyUP[pairdev_in][n] = 0;

                        message.clear();
                        message.emplace_back((unsigned char) 0x80 + ch_up);
                        message.emplace_back((unsigned char) n);
                        message.emplace_back((unsigned char) 0);

                        finger_token[0] = finger_key;
                        finger_token[1] = pairdev_in;
                        MidiInput::receiveMessage(0.0, &message, finger_token);

                    }
                }

            }

            fingerMUX->unlock();
            return;
        }

        if(token == 3) { // _note_finger_UP3[pairdev_in]

            if(!_note_finger_pressed_UP3[pairdev_in] || value == 1)
                _note_finger_on_UP3[pairdev_in] = 0;

            if(value == 1) {
                std::vector<unsigned char> message;

                int ch_up = ((MidiInControl::channelUp(pairdev_in) < 0)
                                 ? MidiOutput::standardChannel()
                                 : MidiInControl::channelUp(pairdev_in)) & 15;

                for(int n = 0; n < 128; n++) {

                    if(map_keyUP[pairdev_in][n] & 128) {

                        int finger_key = map_keyUP[pairdev_in][n] & 127;
                        map_keyUP[pairdev_in][n] = 0;

                        message.clear();
                        message.emplace_back((unsigned char) 0x80 + ch_up);
                        message.emplace_back((unsigned char) n);
                        message.emplace_back((unsigned char) 0);

                        finger_token[0] = finger_key;
                        finger_token[1] = pairdev_in;
                        MidiInput::receiveMessage(0.0, &message, finger_token);

                    }
                }

            }

            fingerMUX->unlock();
            return;
        }

        if(token == 0) { //_note_finger_pick_UP[pairdev_in]

            _note_finger_pick_on_UP[pairdev_in] = 0;

            if(value == 1) {
                std::vector<unsigned char> message;

                int ch_up = ((MidiInControl::channelUp(pairdev_in) < 0)
                                 ? MidiOutput::standardChannel()
                                 : MidiInControl::channelUp(pairdev_in)) & 15;

                for(int n = 0; n < 128; n++) {

                    if(map_keyUP[pairdev_in][n] & 128) {

                        int finger_key = map_keyUP[pairdev_in][n] & 127;
                        map_keyUP[pairdev_in][n] = 0;

                        message.clear();
                        message.emplace_back((unsigned char) 0x80 + ch_up);
                        message.emplace_back((unsigned char) n);
                        message.emplace_back((unsigned char) 0);

                        finger_token[0] = finger_key;
                        finger_token[1] = pairdev_in;
                        MidiInput::receiveMessage(0.0, &message, finger_token);

                    }
                }

            }

            fingerMUX->unlock();
            return;

        }

        if(token == 33) { // _note_finger_DOWN[pairdev_in]

            if(!_note_finger_pressed_DOWN[pairdev_in] || value == 1) _note_finger_on_DOWN[pairdev_in] = 0;

            if(value == 1) {
                std::vector<unsigned char> message;

                int ch_down = ((MidiInControl::channelDown(pairdev_in) < 0)
                                   ? ((MidiInControl::channelUp(pairdev_in) < 0)
                                          ? MidiOutput::standardChannel()
                                          : MidiInControl::channelUp(pairdev_in))
                                   : MidiInControl::channelDown(pairdev_in)) & 0xF;

                for(int n = 0; n < 128; n++) {

                    if(map_keyDOWN[pairdev_in][n] & 128) {

                        int finger_key = map_keyDOWN[pairdev_in][n] & 127;
                        map_keyDOWN[pairdev_in][n] = 0;

                        message.clear();
                        message.emplace_back((unsigned char) 0x80 + ch_down);
                        message.emplace_back((unsigned char) n);
                        message.emplace_back((unsigned char) 0);

                        finger_token[0] = finger_key;
                        finger_token[1] = pairdev_in;
                        MidiInput::receiveMessage(0.0, &message, finger_token);

                    }
                }

            }

            fingerMUX->unlock();
            return;
        }

        if(token == 34) { // _note_finger_DOWN2[pairdev_in]

            if(!_note_finger_pressed_DOWN2[pairdev_in] || value == 1) _note_finger_on_DOWN2[pairdev_in] = 0;

            if(value == 1) {
                std::vector<unsigned char> message;

                int ch_down = ((MidiInControl::channelDown(pairdev_out) < 0)
                                   ? ((MidiInControl::channelUp(pairdev_out) < 0)
                                          ? MidiOutput::standardChannel()
                                          : MidiInControl::channelUp(pairdev_out))
                                   : MidiInControl::channelDown(pairdev_out)) & 0xF;

                for(int n = 0; n < 128; n++) {

                    if(map_keyDOWN[pairdev_in][n] & 128) {

                        int finger_key = map_keyDOWN[pairdev_in][n] & 127;
                        map_keyDOWN[pairdev_in][n] = 0;

                        message.clear();
                        message.emplace_back((unsigned char) 0x80 + ch_down);
                        message.emplace_back((unsigned char) n);
                        message.emplace_back((unsigned char) 0);

                        finger_token[0] = finger_key;
                        finger_token[1] = pairdev_in;
                        MidiInput::receiveMessage(0.0, &message, finger_token);

                    }
                }

            }

            fingerMUX->unlock();
            return;
        }

        if(token == 35) { // _note_finger_DOWN3[pairdev_in]

            if(!_note_finger_pressed_DOWN3[pairdev_in] || value == 1)
                _note_finger_on_DOWN3[pairdev_in] = 0;

            if(value == 1) {
                std::vector<unsigned char> message;

                int ch_down = ((MidiInControl::channelDown(pairdev_out) < 0)
                                   ? ((MidiInControl::channelUp(pairdev_out) < 0)
                                          ? MidiOutput::standardChannel()
                                          : MidiInControl::channelUp(pairdev_out))
                                   : MidiInControl::channelDown(pairdev_out)) & 0xF;

                for(int n = 0; n < 128; n++) {

                    if(map_keyDOWN[pairdev_in][n] & 128) {

                        int finger_key = map_keyDOWN[pairdev_in][n] & 127;
                        map_keyDOWN[pairdev_in][n] = 0;

                        message.clear();
                        message.emplace_back((unsigned char) 0x80 + ch_down);
                        message.emplace_back((unsigned char) n);
                        message.emplace_back((unsigned char) 0);

                        finger_token[0] = finger_key;
                        finger_token[1] = pairdev_in;
                        MidiInput::receiveMessage(0.0, &message, finger_token);

                    }
                }

            }

            fingerMUX->unlock();
            return;
        }


        if(token == 32) { //_note_finger_pick_DOWN[pairdev_in]

            _note_finger_pick_on_DOWN[pairdev_in] = 0;

            if(value == 1) {
                std::vector<unsigned char> message;

                int ch_down = ((MidiInControl::channelDown(pairdev_out) < 0)
                                   ? ((MidiInControl::channelUp(pairdev_out) < 0)
                                          ? MidiOutput::standardChannel()
                                          : MidiInControl::channelUp(pairdev_out))
                                   : MidiInControl::channelDown(pairdev_out)) & 0xF;

                for(int n = 0; n < 128; n++) {

                    if(map_keyDOWN[pairdev_in][n] & 128) {

                        int finger_key = map_keyDOWN[pairdev_in][n] & 127;
                        map_keyDOWN[pairdev_in][n] = 0;

                        message.clear();
                        message.emplace_back((unsigned char) 0x80 + ch_down);
                        message.emplace_back((unsigned char) n);
                        message.emplace_back((unsigned char) 0);

                        finger_token[0] = finger_key;
                        finger_token[1] = pairdev_in;
                        MidiInput::receiveMessage(0.0, &message, finger_token);

                    }
                }

            }

            fingerMUX->unlock();
            return;

        }

    }

    fingerMUX->unlock();

}
// FROM MidiInput -> MidiInControl::finger_func()

int FingerPatternDialog::Finger_note(int pairdev_in, std::vector<unsigned char>* message, bool is_keyboard2, bool only_enable) {

    if(!fingerMUX) {
        qDebug("Finger_note() without fingerMUX defined!");
        return 0;
    }

    fingerMUX->lock();

    only_enable = false;

    if(!message->at(2)) // velocity 0 == note off
        message->at(0) = 0x80 + (message->at(0) & 0xF);
    int evt = message->at(0);

    int ch = evt & 0xf;
    int ch_orig = ch;

    evt&= 0xF0;

    int ch_up = ((MidiInControl::channelUp(pairdev_in) < 0)
                     ? MidiOutput::standardChannel()
                     : MidiInControl::channelUp(pairdev_in)) & 15;

    int ch_down = ((MidiInControl::channelDown(pairdev_in) < 0)
                       ? ((MidiInControl::channelUp(pairdev_in) < 0)
                              ? MidiOutput::standardChannel()
                              : MidiInControl::channelUp(pairdev_in))
                       : MidiInControl::channelDown(pairdev_in)) & 0xF;

    int input_chan_up = MidiInControl::inchannelUp(pairdev_in);
    int input_chan_down = MidiInControl::inchannelDown(pairdev_in);

    if(input_chan_up == -1)
        input_chan_up = ch;

    if(input_chan_down == -1)
        input_chan_down = ch;

    finger_channelUP[pairdev_in] = input_chan_up;
    finger_channelDOWN[pairdev_in] = input_chan_down;

    finger_switch_playUP[pairdev_in] = 0;
    finger_switch_playDOWN[pairdev_in] = 0;

    if(init_map_key) {

        init_map_key = 0;

        for(int dev = 0; dev < MAX_INPUT_PAIR; dev++) {
            for(int n = 0; n < 128; n++) {

                map_keyUP[dev][n] = 0;
                map_key_stepUP[dev][n] = 0;
                map_key_onUP[dev][n] = 0;
                map_key_offUP[dev][n] = 0;

                map_keyDOWN[dev][n] = 0;
                map_key_stepDOWN[dev][n] = 0;
                map_key_onDOWN[dev][n] = 0;
                map_key_offDOWN[dev][n] = 0;
            }
        }
    }

    int finger_key = message->at(1);

    int note_cut = MidiInControl::note_cut(pairdev_in);

    if(MidiInput::keyboard2_connected[pairdev_in * 2 + 1] || MidiInControl::note_duo(pairdev_in))
        note_cut = 0;

    if(evt == 0x90) { // KEY ON

        // FINGER KEYS DEFINED FOR UP

        if((evt == 0x80 || evt == 0x90) && ch == 9) {

            fingerMUX->unlock();
            return 0; // ignore drums
        }

        if((evt == 0x80 || evt == 0x90)
            && ((ch != input_chan_up && ch != input_chan_down)
                || (input_chan_down != ch && input_chan_down != input_chan_up &&
                    (finger_key < note_cut &&
                     !MidiInput::keyboard2_connected[pairdev_in * 2 + 1]))
                || (input_chan_up != ch && input_chan_down != input_chan_up &&
                    (finger_key >= note_cut && !MidiInput::keyboard2_connected[pairdev_in * 2 + 1])))) {

            fingerMUX->unlock();
            return 1; // skip notes
        }

        // GENERIC KEY DIRECT NOTE OFF (FOR OLD CHILD'S KEYS)

        for(int n = 0; n < 128; n++) {

            // UP

            if((finger_key >= note_cut || MidiInput::keyboard2_connected[pairdev_in * 2 + 1]) && !is_keyboard2 && map_keyUP[pairdev_in][n] == (128 | finger_key)) {

                map_keyUP[pairdev_in][n] = 0;

                std::vector<unsigned char> message;

                message.clear();
                message.emplace_back((unsigned char) 0x80 + ch_up);
                message.emplace_back((unsigned char) n);
                message.emplace_back((unsigned char) 0);

                finger_token[0] = finger_key;
                finger_token[1] = pairdev_in;
                MidiInput::receiveMessage(0.0, &message, finger_token);

            }

            // DOWN

            if((finger_key < note_cut || is_keyboard2) && map_keyDOWN[pairdev_in][n] == (128 | finger_key)) {

                map_keyDOWN[pairdev_in][n] = 0;

                std::vector<unsigned char> message;

                message.clear();
                message.emplace_back((unsigned char) 0x80 + ch_down);
                message.emplace_back((unsigned char) n);
                message.emplace_back((unsigned char) 0);

                finger_token[0] = finger_key;
                finger_token[1] = pairdev_in;
                MidiInput::receiveMessage(0.0, &message, finger_token);

            }
        }

        // GENERIC KEY INDIRECT NOTE ON (MASTER NOTE FROM finger_timer())

        int steps;


        if((finger_key >= note_cut || MidiInput::keyboard2_connected[pairdev_in * 2 + 1]) && !is_keyboard2) {

            bool step1 = true;

            if(finger_enable_UP[pairdev_in]
                && (_note_finger_pick_on_UP[pairdev_in] ||
                    _note_finger_on_UP[pairdev_in] ||
                    _note_finger_on_UP2[pairdev_in] ||
                    _note_finger_on_UP3[pairdev_in]))
                step1 = false;

            steps = finger_patternUP[pairdev_in][finger_chord_modeUP[pairdev_in]][FSTEPS];

            map_key_tempo_stepUP[pairdev_in][finger_key] = 0;
            map_key_step_maxUP[pairdev_in][finger_key] = steps | (finger_patternUP[pairdev_in][finger_chord_modeUP[pairdev_in]][FREPEAT] << 8);
            map_key_stepUP[pairdev_in][finger_key] = 128 + ((step1) ? 1 : 0);

            map_key_velocityUP[pairdev_in][finger_key] = message->at(2);

            map_keyUP[pairdev_in][finger_key] = finger_key | 128;

        } else if((is_keyboard2 && MidiInput::keyboard2_connected[pairdev_in * 2 + 1]) || !MidiInput::keyboard2_connected[pairdev_in * 2 + 1]){

            bool step1 = true;

            if(finger_enable_DOWN[pairdev_in]
                && (_note_finger_pick_on_DOWN[pairdev_in] ||
                    _note_finger_on_DOWN[pairdev_in] ||
                    _note_finger_on_DOWN2[pairdev_in] ||
                    _note_finger_on_DOWN3[pairdev_in]))
                step1 = false;

            steps = finger_patternDOWN[pairdev_in][finger_chord_modeDOWN[pairdev_in]][FSTEPS];

            map_key_tempo_stepDOWN[pairdev_in][finger_key] = 0;
            map_key_step_maxDOWN[pairdev_in][finger_key] = steps | (finger_patternDOWN[pairdev_in][finger_chord_modeDOWN[pairdev_in]][FREPEAT] << 8);
            map_key_stepDOWN[pairdev_in][finger_key] = 128 + ((step1) ? 1 : 0);

            map_key_velocityDOWN[pairdev_in][finger_key] = message->at(2);

            map_keyDOWN[pairdev_in][finger_key] = finger_key | 128;

        }


        // GENERIC KEY DIRECT NOTE ON (MASTER NOTE WITHOUT CHILD)

        if(!finger_enable_UP[pairdev_in] && (finger_key >= note_cut || MidiInput::keyboard2_connected[pairdev_in * 2 + 1]) && !is_keyboard2) {

            message->at(0) = 0x90 + ch_up;
            fingerMUX->unlock();
            return 0;
        }

        if(!finger_enable_DOWN[pairdev_in] && (finger_key < note_cut || is_keyboard2)) {

            message->at(0) = 0x90 + ch_down;
            fingerMUX->unlock();
            return 0;
        }



    } else if(evt == 0x80) { // NOTE OFF

        int finger_key_off = message->at(1);

        // FINGER KEYS DEFINED FOR UP

        if(only_enable) {
            message->at(0) = message->at(0) | ch_orig;
            fingerMUX->unlock();
            return 0;
        }

        if((evt == 0x80 || evt == 0x90) && ch == 9) {
            fingerMUX->unlock();
            return 0; // ignore drums
        }

        // GENERIC KEY DIRECT NOTE OFF (FOR OLD CHILD'S KEYS)

        for(int n = 0; n < 128; n++) {

            if((finger_key_off >= note_cut || MidiInput::keyboard2_connected[pairdev_in * 2 + 1]) && !is_keyboard2 && map_keyUP[pairdev_in][n] == (128 | finger_key_off)) {

                map_keyUP[pairdev_in][n] = 0;

                std::vector<unsigned char> message;

                message.clear();
                message.emplace_back((unsigned char) 0x80 + ch_up);
                message.emplace_back((unsigned char) n);
                message.emplace_back((unsigned char) 0);

                finger_token[0] = finger_key_off;
                finger_token[1] = pairdev_in;
                MidiInput::receiveMessage(0.0, &message, finger_token);

            }

            if((finger_key_off < note_cut || is_keyboard2) && map_keyDOWN[pairdev_in][n] == (128 | finger_key_off)) {

                map_keyDOWN[pairdev_in][n] = 0;

                std::vector<unsigned char> message;

                message.clear();
                message.emplace_back((unsigned char) 0x80 + ch_down);
                message.emplace_back((unsigned char) n);
                message.emplace_back((unsigned char) 0);

                finger_token[0] = finger_key_off;
                finger_token[1] = pairdev_in;
                MidiInput::receiveMessage(0.0, &message, finger_token);

            }
        }

               // RESET STEP COUNTERS

        if(finger_key >= note_cut && !is_keyboard2)
            map_key_stepUP[pairdev_in][finger_key_off] = 0;
        else
            map_key_stepDOWN[pairdev_in][finger_key_off] = 0;

               //qDebug("key off %i", finger_key_off);

        // GENERIC KEY DIRECT NOTE OFF (MASTER NOTE WITHOUT CHILD)

        if(!finger_enable_UP[pairdev_in] && (finger_key >= note_cut || MidiInput::keyboard2_connected[pairdev_in * 2 + 1])
            && !is_keyboard2) {

            message->at(0) = 0x80 + ch_up; // NOTE OFF
            fingerMUX->unlock();
            return 0;
        }

        if(!finger_enable_DOWN[pairdev_in] && (finger_key < note_cut || is_keyboard2)) {

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

