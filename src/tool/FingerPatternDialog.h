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


#ifndef FINGERPATTERNDIALOG_H
#define FINGERPATTERNDIALOG_H

#include <QObject>
#include <QWidget>
#include <QTimer>
#include <QThread>
#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QCheckBox>
#include <QSettings>
#include "../gui/GuiTools.h"

//////////////
/*

enum finger_pattern {
    FTYPE = 0,
    FSTEPS,
    FTEMPO,
    FREPEAT,
    FNOTE1
};
static int getKey(int pairdev_in, int zone, int n, int m, int type, int note, int note_disp) {
map_key_step_maxUP[pairdev_in][n]|= 128;
void FingerPatternDialog::finger_timer(int ndevice)

 map_key_step_maxUP[pairdev][m] = (finger_patternUP[finger_chord_modeUP[pairdev]][3] << 4)
                              | (finger_patternUP[finger_chord_modeUP[pairdev]][1] & 15);

 FingerPattern/FINGER_pattern_save
 */

class NoteOnEvent;

class FingerEdit: public QDialog
{
    Q_OBJECT
public:
    QWidget* _parent;

    QGroupBox *group1;
    QGroupBox *group2;
    QGroupBox *group3;
    QSlider *horizontalSliderNotes;

    QPushButtonE *pa[16];
    QPushButtonE *pb[16 * 18];
    int data[16];
    int tempo[16];

    FingerEdit(QWidget* parent, int pairdev_in, int zone);


    void FingerUpdate(int mode);

    NoteOnEvent* pianoEvent;
};

class finger_Thread : public QThread  {
    Q_OBJECT
public:
    finger_Thread();
    ~finger_Thread();

    void run() override;

private:

};

class FingerPatternDialog: public QDialog
{
    Q_OBJECT

public:
    QSettings *_settings;

    FingerEdit *FEdit;

    QDialogButtonBox *buttonBox;

    QGroupBox *groupBoxPatternNoteUP;
    QPushButton *pushButtonDeleteUP;
    QLabel *labelStepsHUP;
    QLabel *labelTempoHUP;
    QComboBox *comboBoxChordUP;
    QLabel *labelChordUP;
    QComboBox *comboBoxRepeatUP;
    QLabel *labelRepeatUP;
    QSlider *horizontalSliderNotesUP;
    QPushButton * NoteUPButtons[15];
    QLabel *labelNoteUP[15];
    QSlider *horizontalSliderTempoUP;
    QLabel *labelTempoUP[17];
    QComboBox *comboBoxCustomUP;
    QPushButton *pushButtonStoreUP;

    QGroupBox *groupBoxPatternNoteDOWN;
    QPushButton *pushButtonDeleteDOWN;
    QLabel *labelStepsHDOWN;
    QLabel *labelTempoHDOWN;
    QComboBox *comboBoxChordDOWN;
    QLabel *labelChordDOWN;
    QComboBox *comboBoxRepeatDOWN;
    QLabel *labelRepeatDOWN;
    QSlider *horizontalSliderNotesDOWN;
    QPushButton * NoteDOWNButtons[15];
    QLabel *labelNoteDOWN[15];
    QSlider *horizontalSliderTempoDOWN;
    QLabel *labelTempoDOWN[17];
    QComboBox *comboBoxCustomDOWN;
    QPushButton *pushButtonStoreDOWN;

    QPushButton *pushButtonTestNoteUP;
    QPushButton *pushButtonTestNoteDOWN;
    QComboBox *comboBoxBaseNote;
    QLabel *labelBaseNote;

    QWidget *groupPlay;
    QCheckBox *checkBoxDisableFinger;

    QLabel *labelUP;
    QComboBox *comboBoxTypeUP;

    QLabel *labelDOWN;
    QComboBox *comboBoxTypeDOWN;

    QLabel *labelUP2;
    QComboBox *comboBoxTypeUP2;

    QLabel *labelUP3;
    QComboBox *comboBoxTypeUP3;

    QLabel *labelDOWN2;
    QComboBox *comboBoxTypeDOWN2;

    QLabel *labelDOWN3;
    QComboBox *comboBoxTypeDOWN3;

    QPushButton *pushButtonSavePattern;
    QPushButton *pushButtonLoadPattern;

    FingerPatternDialog(QWidget* parent, QSettings *settings, int pairdev_in);
    ~FingerPatternDialog();

    void setComboNotePattern(int type, int index, QComboBox * cb);
    QString setButtonNotePattern(int type, int val);

    static int Finger_Action_time(int dev_in, int token, int value);
    static void Finger_Action(int dev_in, int token, bool switch_on, int value);
    static int Finger_note(int pairdev_in, std::vector<unsigned char>* message, bool is_keyboard2 = false, bool only_enable = false);

    static void finger_timer(int ndevice = -1);

    static bool finger_on(int device);

    static int pairdev_out;

    static int _track_index;

    QTimer *time_update;

public slots:

    void timer_dialog();

    void play_noteUP();
    void stop_noteUP();
    void play_noteDOWN();
    void stop_noteDOWN();
    void reject();
    void save();
    void load();

private:

    void save_custom(int pairdev_in, bool down);
    void load_custom(int pairdev_in, int custom, bool down);

    finger_Thread *thread_timer;

    QWidget *_parent;

};

extern FingerPatternDialog* FingerPatternWin;

#endif // FINGERPATTERNDIALOG_H
