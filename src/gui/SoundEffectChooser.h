/*
 * MidiEditor
 * Copyright (C) 2010  Markus Schwenk
 *
 * SoundEffectChooser
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

#ifndef SOUNDEFFECTCHOOSER_H
#define SOUNDEFFECTCHOOSER_H

#include <QtCore/QVariant>
#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QRadioButton>
#include <QLabel>
#include <QSlider>

class MidiFile;
#define SOUNDEFFECTCHOOSER_MODULATIONWHEEL 0
#define SOUNDEFFECTCHOOSER_PAN 1
#define SOUNDEFFECTCHOOSER_MAINVOLUME 2
#define SOUNDEFFECTCHOOSER_SUSTAIN 3
#define SOUNDEFFECTCHOOSER_SOSTENUTO 4
#define SOUNDEFFECTCHOOSER_ATTACK 5
#define SOUNDEFFECTCHOOSER_RELEASE 6
#define SOUNDEFFECTCHOOSER_DECAY 7
#define SOUNDEFFECTCHOOSER_REVERB 8
#define SOUNDEFFECTCHOOSER_CHORUS 9
#define SOUNDEFFECTCHOOSER_PITCHBEND 10
#define SOUNDEFFECTCHOOSER_ALL 255
#define SOUNDEFFECTCHOOSER_EDITALL 256
#define SOUNDEFFECTCHOOSER_FORMIDIIN 257

class SoundEffectChooser: public QDialog
{
public:
    QDialogButtonBox *buttonBox;

    QLabel *label1;
    QSlider *ModulationWheelVal;
    QCheckBox *ModulationWheelCheck;
    QLabel *label_Modval;

    QLabel *label2;
    QSlider *PanVal;
    QCheckBox *PanCheck;
    QLabel *label_Panval;

    QLabel *label3;
    QSlider *MainVolumeVal;
    QCheckBox *MainVolumeCheck;
    QLabel *label_MainVval;

    QLabel *labelx;
    QCheckBox *DeleteCheck;

    QLabel *label4;
    QRadioButton *SustainButton;
    QRadioButton *SustainButton_2;
    QCheckBox *SustainCheck;

    QLabel *label5;
    QRadioButton *SostenutoButton;
    QRadioButton *SostenutoButton_2;
    QCheckBox *SostenutoCheck;

    QLabel *label6;
    QSlider *AttackVal;
    QCheckBox *AttackCheck;
    QLabel *label_Attackval;

    QLabel *label7;
    QSlider *ReleaseVal;
    QCheckBox *ReleaseCheck;
    QLabel *label_Relval;

    QLabel *label8;
    QSlider *DecayVal;
    QCheckBox *DecayCheck;
    QLabel *label_Decval;

    QLabel *label9;
    QSlider *ReverbVal;
    QCheckBox *ReverbCheck;
    QLabel *label_Revval;

    QLabel *label10;
    QSlider *ChorusVal;
    QCheckBox *ChorusCheck;
    QLabel *label_Choval;

    QLabel *label11;
    QSlider *PitchBendVal;
    QCheckBox *PitchBendCheck;
    QLabel *label_Pitchval;

    QFrame *framex;
    QFrame *frame1;
    QFrame *frame2;
    QFrame *frame3;
    QFrame *frame4;
    QFrame *frame5;

    QFrame *frame6;
    QFrame *frame7;
    QFrame *frame8;
    QFrame *frame9;
    QFrame *frame10;
    QFrame *frame11;

    int result;

    SoundEffectChooser(MidiFile* f, int channel, QWidget* parent, int flag, int tick_edit = -123456);

public slots:
    void accept();


private:
    MidiFile* _file;
    int _channel;
    int _flag;
    int _tick_edit;

};


#endif
