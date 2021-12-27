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
    QComboBox *ComboBoxNoteUP[15];
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
    QComboBox *ComboBoxNoteDOWN[15];
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
    QCheckBox *checkKeyPressedUP;
    QComboBox *comboBoxNoteUP;
    QLabel *labelPickUP;
    QComboBox *comboBoxNotePickUP;

    QLabel *labelDOWN;
    QComboBox *comboBoxTypeDOWN;
    QCheckBox *checkKeyPressedDOWN;
    QComboBox *comboBoxNoteDOWN;
    QLabel *labelPickDOWN;
    QComboBox *comboBoxNotePickDOWN;

    QLabel *labelUP2;
    QComboBox *comboBoxTypeUP2;
    QCheckBox *checkKeyPressedUP2;
    QComboBox *comboBoxNoteUP2;

    QLabel *labelDOWN2;
    QComboBox *comboBoxTypeDOWN2;
    QCheckBox *checkKeyPressedDOWN2;
    QComboBox *comboBoxNoteDOWN2;

    QPushButton *pushButtonSavePattern;
    QPushButton *pushButtonLoadPattern;

    FingerPatternDialog(QWidget* parent, QSettings *settings = 0);
    ~FingerPatternDialog();

    void setComboNotePattern(int type, int index, QComboBox * cb);

    static int Finger_note(std::vector<unsigned char>* message);

    static void finger_timer();

public slots:

    void play_noteUP();
    void stop_noteUP();
    void play_noteDOWN();
    void stop_noteDOWN();
    void reject();
    void save();
    void load();

private:
    finger_Thread *thread_timer;

    QWidget *_parent;

};

#endif // FINGERPATTERNDIALOG_H
