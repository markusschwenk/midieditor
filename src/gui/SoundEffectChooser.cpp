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

#include "SoundEffectChooser.h"

#include <QtCore/QVariant>
#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QSlider>
#include <QButtonGroup>
#include <QComboBox>
#include <QGridLayout>
#include <QPushButton>

#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/ControlChangeEvent.h"
#include "../MidiEvent/ProgChangeEvent.h"
#include "../MidiEvent/PitchBendEvent.h"
#include "../MidiEvent/NoteOnEvent.h"
#include "../midi/MidiChannel.h"
#include "../midi/MidiFile.h"
#include "../protocol/Protocol.h"



SoundEffectChooser::SoundEffectChooser(MidiFile* f, int channel, QWidget* parent, int flag, int tick_edit)
    : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
    QDialog *SoundEffectChooser= this;
    _flag = flag;
    _file = f;
    _channel = channel;

    if(tick_edit == -123456)
        _tick_edit = _file->cursorTick();
    else
        _tick_edit = tick_edit;

    result = 0;
    QFrame *line;

    label1=label2=label3=label4=label5=label6=label7=label8=label9=label10=label11=labelx=NULL;
    frame1=frame2=frame3=frame4=frame5=frame6=frame7=frame8=frame9=frame10=frame11=framex=0;

    int x,y,x2;

    if (SoundEffectChooser->objectName().isEmpty())
        SoundEffectChooser->setObjectName(QString::fromUtf8("SoundEffectChooser"));
    if(_flag >= SOUNDEFFECTCHOOSER_ALL)
        SoundEffectChooser->resize(461, 311+132);
    else
        SoundEffectChooser->resize(461, 111+64);


    SoundEffectChooser->setInputMethodHints(Qt::ImhNone);
    buttonBox = new QDialogButtonBox(SoundEffectChooser);
    buttonBox->setObjectName(QString::fromUtf8("buttonBox"));

    if(_flag >= SOUNDEFFECTCHOOSER_ALL) {
        buttonBox->setGeometry(QRect(280, 400, 161, 32));
        buttonBox->setOrientation(Qt::Horizontal);
    }

    else {
        buttonBox->setGeometry(QRect(360, 20, 81, 241));
        buttonBox->setOrientation(Qt::Vertical);
    }


    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);


    x=24; y= 20;
    x2=(_flag < SOUNDEFFECTCHOOSER_ALL) ? x : x+221;

    if(_flag >= SOUNDEFFECTCHOOSER_ALL || _flag == SOUNDEFFECTCHOOSER_MODULATIONWHEEL){
        frame1 = new QFrame(SoundEffectChooser);
        frame1->setObjectName(QString::fromUtf8("frame1"));
        frame1->setGeometry(QRect(x-10, y+5, 211, 50));
        frame1->setFrameShape(QFrame::Box);
        frame1->setFrameShadow(QFrame::Raised);
        label1 = new QLabel(SoundEffectChooser);
        label1->setObjectName(QString::fromUtf8("label1"));
        label1->setGeometry(QRect(x+30, y, 111, 31));
        label1->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        line = new QFrame(SoundEffectChooser);
        line->setObjectName(QString::fromUtf8("line"));
        line->setGeometry(QRect(x+100, y+27, 20, 26));
        line->setFrameShadow(QFrame::Sunken);
        line->setLineWidth(1);
        line->setMidLineWidth(0);
        line->setFrameShape(QFrame::VLine);
        ModulationWheelVal = new QSlider(SoundEffectChooser);
        ModulationWheelVal->setObjectName(QString::fromUtf8("ModulationWheelVal"));
        ModulationWheelVal->setGeometry(QRect(x+30, y+30, 160, 22));
        ModulationWheelVal->setMaximum(127);
        ModulationWheelVal->setOrientation(Qt::Horizontal);
        if(_flag >= SOUNDEFFECTCHOOSER_ALL){
            ModulationWheelCheck = new QCheckBox(SoundEffectChooser);
            ModulationWheelCheck->setObjectName(QString::fromUtf8("ModulationWheelCheck"));
            ModulationWheelCheck->setGeometry(QRect(x, y+30, 16, 17));
        }

        label_Modval = new QLabel(SoundEffectChooser);
        label_Modval->setObjectName(QString::fromUtf8("label_Modval"));
        label_Modval->setGeometry(QRect(x+10+160, y+10, 19, 10));
        label_Modval->setStyleSheet(QString::fromUtf8("background-color: white;"));
        label_Modval->setText(QString::number(ModulationWheelVal->value()));
        QObject::connect(ModulationWheelVal, SIGNAL(valueChanged(int)), label_Modval, SLOT(setNum(int)));
    }

    if(_flag >= SOUNDEFFECTCHOOSER_ALL || _flag == SOUNDEFFECTCHOOSER_ATTACK){
        frame6 = new QFrame(SoundEffectChooser);
        frame6->setObjectName(QString::fromUtf8("frame6"));
        frame6->setGeometry(QRect(x2-10, y+5, 211, 50));
        frame6->setFrameShape(QFrame::Box);
        frame6->setFrameShadow(QFrame::Raised);
        label6 = new QLabel(SoundEffectChooser);
        label6->setObjectName(QString::fromUtf8("label6"));
        label6->setGeometry(QRect(x2+30, y, 111, 31));
        label6->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        line = new QFrame(SoundEffectChooser);
        line->setObjectName(QString::fromUtf8("line"));
        line->setGeometry(QRect(x2+100, y+27, 20, 26));
        line->setFrameShadow(QFrame::Sunken);
        line->setLineWidth(1);
        line->setMidLineWidth(0);
        line->setFrameShape(QFrame::VLine);
        AttackVal = new QSlider(SoundEffectChooser);
        AttackVal->setObjectName(QString::fromUtf8("AttackVal"));
        AttackVal->setGeometry(QRect(x2+30, y+30, 160, 22));
        AttackVal->setMaximum(127);
        AttackVal->setValue(64);
        AttackVal->setOrientation(Qt::Horizontal);
        if(_flag >= SOUNDEFFECTCHOOSER_ALL){
            AttackCheck = new QCheckBox(SoundEffectChooser);
            AttackCheck->setObjectName(QString::fromUtf8("AttackCheck"));
            AttackCheck->setGeometry(QRect(x2, y+30, 16, 17));
        }

        label_Attackval = new QLabel(SoundEffectChooser);
        label_Attackval->setObjectName(QString::fromUtf8("label_Attackval"));
        label_Attackval->setGeometry(QRect(x2+10+160, y+10, 19, 10));
        label_Attackval->setStyleSheet(QString::fromUtf8("background-color: white;"));
        label_Attackval->setText(QString::number(AttackVal->value()));
        QObject::connect(AttackVal, SIGNAL(valueChanged(int)), label_Attackval, SLOT(setNum(int)));
    }

    if(_flag >= SOUNDEFFECTCHOOSER_ALL) y+=60;

    if(_flag >= SOUNDEFFECTCHOOSER_ALL || _flag == SOUNDEFFECTCHOOSER_PAN){
        frame2 = new QFrame(SoundEffectChooser);
        frame2->setObjectName(QString::fromUtf8("frame2"));
        frame2->setGeometry(QRect(x-10, y+5, 211, 50));
        frame2->setFrameShape(QFrame::Box);
        frame2->setFrameShadow(QFrame::Raised);
        label2 = new QLabel(SoundEffectChooser);
        label2->setObjectName(QString::fromUtf8("label2"));
        label2->setGeometry(QRect(x+30, y, 101, 31));
        label2->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        line = new QFrame(SoundEffectChooser);
        line->setObjectName(QString::fromUtf8("line"));
        line->setGeometry(QRect(x+100, y+27, 20, 26));
        line->setFrameShadow(QFrame::Sunken);
        line->setLineWidth(1);
        line->setMidLineWidth(0);
        line->setFrameShape(QFrame::VLine);
        PanVal = new QSlider(SoundEffectChooser);
        PanVal->setObjectName(QString::fromUtf8("PanVal"));
        PanVal->setGeometry(QRect(x+30, y+30, 160, 22));
        PanVal->setMaximum(127);
        PanVal->setValue(64);
        PanVal->setOrientation(Qt::Horizontal);
        if(_flag >= SOUNDEFFECTCHOOSER_ALL){
            PanCheck = new QCheckBox(SoundEffectChooser);
            PanCheck->setObjectName(QString::fromUtf8("PanCheck"));
            PanCheck->setGeometry(QRect(x, y+30, 16, 17));
        }
        label_Panval = new QLabel(SoundEffectChooser);
        label_Panval->setObjectName(QString::fromUtf8("label_Modval"));
        label_Panval->setGeometry(QRect(x+10+160, y+10, 19, 10));
        label_Panval->setStyleSheet(QString::fromUtf8("background-color: white;"));
        label_Panval->setText(QString::number(PanVal->value()));
        QObject::connect(PanVal, SIGNAL(valueChanged(int)), label_Panval, SLOT(setNum(int)));
    }

    if(_flag >= SOUNDEFFECTCHOOSER_ALL || _flag == SOUNDEFFECTCHOOSER_RELEASE){
        frame7 = new QFrame(SoundEffectChooser);
        frame7->setObjectName(QString::fromUtf8("frame7"));
        frame7->setGeometry(QRect(x2-10, y+5, 211, 50));
        frame7->setFrameShape(QFrame::Box);
        frame7->setFrameShadow(QFrame::Raised);
        label7 = new QLabel(SoundEffectChooser);
        label7->setObjectName(QString::fromUtf8("label7"));
        label7->setGeometry(QRect(x2+30, y, 111, 31));
        label7->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        line = new QFrame(SoundEffectChooser);
        line->setObjectName(QString::fromUtf8("line"));
        line->setGeometry(QRect(x2+100, y+27, 20, 26));
        line->setFrameShadow(QFrame::Sunken);
        line->setLineWidth(1);
        line->setMidLineWidth(0);
        line->setFrameShape(QFrame::VLine);
        ReleaseVal = new QSlider(SoundEffectChooser);
        ReleaseVal->setObjectName(QString::fromUtf8("ReleaseVal"));
        ReleaseVal->setGeometry(QRect(x2+30, y+30, 160, 22));
        ReleaseVal->setMaximum(127);
        ReleaseVal->setValue(64);
        ReleaseVal->setOrientation(Qt::Horizontal);
        if(_flag >= SOUNDEFFECTCHOOSER_ALL){
            ReleaseCheck = new QCheckBox(SoundEffectChooser);
            ReleaseCheck->setObjectName(QString::fromUtf8("ReleaseCheck"));
            ReleaseCheck->setGeometry(QRect(x2, y+30, 16, 17));
        }
        label_Relval = new QLabel(SoundEffectChooser);
        label_Relval->setObjectName(QString::fromUtf8("label_Attackval"));
        label_Relval->setGeometry(QRect(x2+10+160, y+10, 19, 10));
        label_Relval->setStyleSheet(QString::fromUtf8("background-color: white;"));
        label_Relval->setText(QString::number(ReleaseVal->value()));
        QObject::connect(ReleaseVal, SIGNAL(valueChanged(int)), label_Relval, SLOT(setNum(int)));
    }

    if(_flag >= SOUNDEFFECTCHOOSER_ALL) y+=60;

    if(_flag >= SOUNDEFFECTCHOOSER_ALL || _flag == SOUNDEFFECTCHOOSER_MAINVOLUME){
        frame3 = new QFrame(SoundEffectChooser);
        frame3->setObjectName(QString::fromUtf8("frame3"));
        frame3->setGeometry(QRect(x-10, y+5, 211, 50));
        frame3->setFrameShape(QFrame::Box);
        frame3->setFrameShadow(QFrame::Raised);

        label3 = new QLabel(SoundEffectChooser);
        label3->setObjectName(QString::fromUtf8("label3"));
        label3->setGeometry(QRect(x+30, y, 101, 31));
        label3->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        line = new QFrame(SoundEffectChooser);
        line->setObjectName(QString::fromUtf8("line"));
        line->setGeometry(QRect(x+100, y+27, 20, 26));
        line->setFrameShadow(QFrame::Sunken);
        line->setLineWidth(1);
        line->setMidLineWidth(0);
        line->setFrameShape(QFrame::VLine);
        MainVolumeVal = new QSlider(SoundEffectChooser);
        MainVolumeVal->setObjectName(QString::fromUtf8("MainVolumeVal"));
        MainVolumeVal->setGeometry(QRect(x+30, y+30, 160, 22));
        MainVolumeVal->setMaximum(127);
        MainVolumeVal->setValue(100);
        MainVolumeVal->setOrientation(Qt::Horizontal);
        if(_flag >= SOUNDEFFECTCHOOSER_ALL){
            MainVolumeCheck = new QCheckBox(SoundEffectChooser);
            MainVolumeCheck->setObjectName(QString::fromUtf8("MainVolumeCheck"));
            MainVolumeCheck->setGeometry(QRect(x, y+30, 16, 17));
        }
        label_MainVval = new QLabel(SoundEffectChooser);
        label_MainVval->setObjectName(QString::fromUtf8("label_Modval"));
        label_MainVval->setGeometry(QRect(x+10+160, y+10, 19, 10));
        label_MainVval->setStyleSheet(QString::fromUtf8("background-color: white;"));
        label_MainVval->setText(QString::number(MainVolumeVal->value()));
        QObject::connect(MainVolumeVal, SIGNAL(valueChanged(int)), label_MainVval, SLOT(setNum(int)));
    }

    if(_flag >= SOUNDEFFECTCHOOSER_ALL || _flag == SOUNDEFFECTCHOOSER_DECAY){
        frame8 = new QFrame(SoundEffectChooser);
        frame8->setObjectName(QString::fromUtf8("frame8"));
        frame8->setGeometry(QRect(x2-10, y+5, 211, 50));
        frame8->setFrameShape(QFrame::Box);
        frame8->setFrameShadow(QFrame::Raised);
        label8 = new QLabel(SoundEffectChooser);
        label8->setObjectName(QString::fromUtf8("label8"));
        label8->setGeometry(QRect(x2+30, y, 111, 31));
        label8->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        line = new QFrame(SoundEffectChooser);
        line->setObjectName(QString::fromUtf8("line"));
        line->setGeometry(QRect(x2+100, y+27, 20, 26));
        line->setFrameShadow(QFrame::Sunken);
        line->setLineWidth(1);
        line->setMidLineWidth(0);
        line->setFrameShape(QFrame::VLine);
        DecayVal = new QSlider(SoundEffectChooser);
        DecayVal->setObjectName(QString::fromUtf8("DecayVal"));
        DecayVal->setGeometry(QRect(x2+30, y+30, 160, 22));
        DecayVal->setMaximum(127);
        DecayVal->setValue(64);
        DecayVal->setOrientation(Qt::Horizontal);
        if(_flag >= SOUNDEFFECTCHOOSER_ALL){
            DecayCheck = new QCheckBox(SoundEffectChooser);
            DecayCheck->setObjectName(QString::fromUtf8("DecayCheck"));
            DecayCheck->setGeometry(QRect(x2, y+30, 16, 17));
        }
        label_Decval = new QLabel(SoundEffectChooser);
        label_Decval->setObjectName(QString::fromUtf8("label_Attackval"));
        label_Decval->setGeometry(QRect(x2+10+160, y+10, 19, 10));
        label_Decval->setStyleSheet(QString::fromUtf8("background-color: white;"));
        label_Decval->setText(QString::number(DecayVal->value()));
        QObject::connect(DecayVal, SIGNAL(valueChanged(int)), label_Decval, SLOT(setNum(int)));
    }

    y+= 60;
    if(_flag < SOUNDEFFECTCHOOSER_ALL) {

        framex = new QFrame(SoundEffectChooser);
        framex->setObjectName(QString::fromUtf8("framex"));
        framex->setGeometry(QRect(x-10, y+5, 211, 50));
        framex->setFrameShape(QFrame::Box);
        framex->setFrameShadow(QFrame::Raised);
        labelx = new QLabel(SoundEffectChooser);
        labelx->setObjectName(QString::fromUtf8("labelx"));
        labelx->setGeometry(QRect(x+30, y+20, 181, 31));
        labelx->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        DeleteCheck = new QCheckBox(SoundEffectChooser);
        DeleteCheck->setObjectName(QString::fromUtf8("DeleteCheck"));
        DeleteCheck->setGeometry(QRect(x, y+28, 16, 17));
    }

   if(_flag >= SOUNDEFFECTCHOOSER_ALL || _flag == SOUNDEFFECTCHOOSER_SUSTAIN){
       if(_flag < SOUNDEFFECTCHOOSER_ALL) y= 20;
       frame4 = new QFrame(SoundEffectChooser);
       frame4->setObjectName(QString::fromUtf8("frame4"));
       frame4->setGeometry(QRect(x-10, y+5, 211, 50));
       frame4->setFrameShape(QFrame::Box);
       frame4->setFrameShadow(QFrame::Raised);
       SustainButton = new QRadioButton(SoundEffectChooser);
       SustainButton->setObjectName(QString::fromUtf8("SustainButton"));
       SustainButton->setGeometry(QRect(x+30, y+30, 61, 17));
       SustainButton->setChecked(true);
       SustainButton_2 = new QRadioButton(SoundEffectChooser);
       SustainButton_2->setObjectName(QString::fromUtf8("SustainButton_2"));
       SustainButton_2->setGeometry(QRect(x+80, y+30, 82, 17));
       SustainButton->setText(QCoreApplication::translate("SoundEffectChooser", "On", nullptr));
       SustainButton_2->setText(QCoreApplication::translate("SoundEffectChooser", "Off", nullptr));
       QButtonGroup* group = new QButtonGroup();
       group->addButton(SustainButton);
       group->addButton(SustainButton_2);
       label4 = new QLabel(SoundEffectChooser);
       label4->setObjectName(QString::fromUtf8("label4"));
       label4->setGeometry(QRect(x+30, y, 101, 31));
       label4->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
       if(_flag >= SOUNDEFFECTCHOOSER_ALL){
           SustainCheck = new QCheckBox(SoundEffectChooser);
           SustainCheck->setObjectName(QString::fromUtf8("SustainCheck"));
           SustainCheck->setGeometry(QRect(x, y+30, 16, 17));
       }
   }

   if(_flag >= SOUNDEFFECTCHOOSER_ALL || _flag == SOUNDEFFECTCHOOSER_REVERB){
       if(_flag < SOUNDEFFECTCHOOSER_ALL) y= 20;
       frame9 = new QFrame(SoundEffectChooser);
       frame9->setObjectName(QString::fromUtf8("frame9"));
       frame9->setGeometry(QRect(x2-10, y+5, 211, 50));
       frame9->setFrameShape(QFrame::Box);
       frame9->setFrameShadow(QFrame::Raised);
       label9 = new QLabel(SoundEffectChooser);
       label9->setObjectName(QString::fromUtf8("label9"));
       label9->setGeometry(QRect(x2+30, y, 111, 31));
       label9->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
       line = new QFrame(SoundEffectChooser);
       line->setObjectName(QString::fromUtf8("line"));
       line->setGeometry(QRect(x2+100, y+27, 20, 26));
       line->setFrameShadow(QFrame::Sunken);
       line->setLineWidth(1);
       line->setMidLineWidth(0);
       line->setFrameShape(QFrame::VLine);
       ReverbVal = new QSlider(SoundEffectChooser);
       ReverbVal->setObjectName(QString::fromUtf8("ReverbVal"));
       ReverbVal->setGeometry(QRect(x2+30, y+30, 160, 22));
       ReverbVal->setMaximum(127);
       ReverbVal->setValue(64);
       ReverbVal->setOrientation(Qt::Horizontal);
       if(_flag >= SOUNDEFFECTCHOOSER_ALL){
           ReverbCheck = new QCheckBox(SoundEffectChooser);
           ReverbCheck->setObjectName(QString::fromUtf8("ReverbCheck"));
           ReverbCheck->setGeometry(QRect(x2, y+30, 16, 17));
       }
       label_Revval = new QLabel(SoundEffectChooser);
       label_Revval->setObjectName(QString::fromUtf8("label_Attackval"));
       label_Revval->setGeometry(QRect(x2+10+160, y+10, 19, 10));
       label_Revval->setStyleSheet(QString::fromUtf8("background-color: white;"));
       label_Revval->setText(QString::number(ReverbVal->value()));
       QObject::connect(ReverbVal, SIGNAL(valueChanged(int)), label_Revval, SLOT(setNum(int)));
   }

   if(_flag >= SOUNDEFFECTCHOOSER_ALL) y+=60;

   if(_flag >= SOUNDEFFECTCHOOSER_ALL || _flag == SOUNDEFFECTCHOOSER_SOSTENUTO){
       if(_flag < SOUNDEFFECTCHOOSER_ALL) y= 20;
       frame5 = new QFrame(SoundEffectChooser);
       frame5->setObjectName(QString::fromUtf8("frame5"));
       frame5->setGeometry(QRect(x-10, y+5, 211, 50));
       frame5->setFrameShape(QFrame::Box);
       frame5->setFrameShadow(QFrame::Raised);
       SostenutoButton = new QRadioButton(SoundEffectChooser);
       SostenutoButton->setObjectName(QString::fromUtf8("SostenutoButton"));
       SostenutoButton->setGeometry(QRect(x+30, y+30, 61, 17));
       SostenutoButton->setChecked(true);
       SostenutoButton_2 = new QRadioButton(SoundEffectChooser);
       SostenutoButton_2->setObjectName(QString::fromUtf8("SostenutoButton_2"));
       SostenutoButton_2->setGeometry(QRect(x+80, y+30, 82, 17));
       SostenutoButton->setText(QCoreApplication::translate("SoundEffectChooser", "On", nullptr));
       SostenutoButton_2->setText(QCoreApplication::translate("SoundEffectChooser", "Off", nullptr));
       QButtonGroup* group = new QButtonGroup();
       group->addButton(SostenutoButton);
       group->addButton(SostenutoButton_2);
       label5 = new QLabel(SoundEffectChooser);
       label5->setObjectName(QString::fromUtf8("label5"));
       label5->setGeometry(QRect(x+30, y, 101, 31));
       label5->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
       if(_flag >= SOUNDEFFECTCHOOSER_ALL){
           SostenutoCheck = new QCheckBox(SoundEffectChooser);
           SostenutoCheck->setObjectName(QString::fromUtf8("SostenutoCheck"));
           SostenutoCheck->setGeometry(QRect(x, y+30, 16, 17));
       }
   }

   if(_flag >= SOUNDEFFECTCHOOSER_ALL || _flag == SOUNDEFFECTCHOOSER_CHORUS){
       if(_flag < SOUNDEFFECTCHOOSER_ALL) y= 20;
       frame10 = new QFrame(SoundEffectChooser);
       frame10->setObjectName(QString::fromUtf8("frame10"));
       frame10->setGeometry(QRect(x2-10, y+5, 211, 50));
       frame10->setFrameShape(QFrame::Box);
       frame10->setFrameShadow(QFrame::Raised);
       label10 = new QLabel(SoundEffectChooser);
       label10->setObjectName(QString::fromUtf8("label10"));
       label10->setGeometry(QRect(x2+30, y, 111, 31));
       label10->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
       line = new QFrame(SoundEffectChooser);
       line->setObjectName(QString::fromUtf8("line"));
       line->setGeometry(QRect(x2+100, y+27, 20, 26));
       line->setFrameShadow(QFrame::Sunken);
       line->setLineWidth(1);
       line->setMidLineWidth(0);
       line->setFrameShape(QFrame::VLine);
       ChorusVal = new QSlider(SoundEffectChooser);
       ChorusVal->setObjectName(QString::fromUtf8("ChorusVal"));
       ChorusVal->setGeometry(QRect(x2+30, y+30, 160, 22));
       ChorusVal->setMaximum(127);
       ChorusVal->setValue(64);
       ChorusVal->setOrientation(Qt::Horizontal);
       if(_flag >= SOUNDEFFECTCHOOSER_ALL){
           ChorusCheck = new QCheckBox(SoundEffectChooser);
           ChorusCheck->setObjectName(QString::fromUtf8("ChorusCheck"));
           ChorusCheck->setGeometry(QRect(x2, y+30, 16, 17));
       }
       label_Choval = new QLabel(SoundEffectChooser);
       label_Choval->setObjectName(QString::fromUtf8("label_Attackval"));
       label_Choval->setGeometry(QRect(x2+10+160, y+10, 19, 10));
       label_Choval->setStyleSheet(QString::fromUtf8("background-color: white;"));
       label_Choval->setText(QString::number(ChorusVal->value()));
       QObject::connect(ChorusVal, SIGNAL(valueChanged(int)), label_Choval, SLOT(setNum(int)));
   }

   if(_flag >= SOUNDEFFECTCHOOSER_ALL) {
       y+= 60;
       x2 = x + (221 / 2);
   }

   if(_flag >= SOUNDEFFECTCHOOSER_ALL || _flag == SOUNDEFFECTCHOOSER_PITCHBEND){
       if(_flag < SOUNDEFFECTCHOOSER_ALL) y= 20;
       frame11 = new QFrame(SoundEffectChooser);
       frame11->setObjectName(QString::fromUtf8("frame11"));
       frame11->setGeometry(QRect(x2-10, y+5, 211, 50));
       frame11->setFrameShape(QFrame::Box);
       frame11->setFrameShadow(QFrame::Raised);
       label11 = new QLabel(SoundEffectChooser);
       label11->setObjectName(QString::fromUtf8("label11"));
       label11->setGeometry(QRect(x2+30, y, 111, 31));
       label11->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
       line = new QFrame(SoundEffectChooser);
       line->setObjectName(QString::fromUtf8("line"));
       line->setGeometry(QRect(x2+100, y+27, 20, 26));
       line->setFrameShadow(QFrame::Sunken);
       line->setLineWidth(1);
       line->setMidLineWidth(0);
       line->setFrameShape(QFrame::VLine);
       PitchBendVal = new QSlider(SoundEffectChooser);
       PitchBendVal->setObjectName(QString::fromUtf8("PitchBendVal"));
       PitchBendVal->setGeometry(QRect(x2+30, y+30, 160, 22));
       PitchBendVal->setMinimum(-99);
       PitchBendVal->setMaximum(99);
       PitchBendVal->setValue(0);
       PitchBendVal->setOrientation(Qt::Horizontal);
       if(_flag >= SOUNDEFFECTCHOOSER_ALL){
           PitchBendCheck = new QCheckBox(SoundEffectChooser);
           PitchBendCheck->setObjectName(QString::fromUtf8("PitchBendCheck"));
           PitchBendCheck->setGeometry(QRect(x2, y+30, 16, 17));
       }
       label_Pitchval = new QLabel(SoundEffectChooser);
       label_Pitchval->setObjectName(QString::fromUtf8("label_Attackval"));
       label_Pitchval->setGeometry(QRect(x2+10+160, y+10, 19, 10));
       label_Pitchval->setStyleSheet(QString::fromUtf8("background-color: white;"));
       label_Pitchval->setText(QString::number(PitchBendVal->value()));
       QObject::connect(PitchBendVal, SIGNAL(valueChanged(int)), label_Pitchval, SLOT(setNum(int)));
   }



    char T[256]="";

    if(_flag > SOUNDEFFECTCHOOSER_ALL)
        sprintf(T, "%s / Channel: %2.2i","Edit Sound Effect", channel);
    else if(_flag == SOUNDEFFECTCHOOSER_ALL)
        sprintf(T, "%s / Channel: %2.2i","Sound Effect", channel);
    else if(_flag == SOUNDEFFECTCHOOSER_MODULATIONWHEEL)
        sprintf(T, "%s / Channel: %2.2i", "Edit Modulation Wheel", channel);
    else if(_flag == SOUNDEFFECTCHOOSER_PAN)
        sprintf(T, "%s / Channel: %2.2i", "Edit Pan", channel);
    else if(_flag == SOUNDEFFECTCHOOSER_MAINVOLUME)
        sprintf(T, "%s / Channel: %2.2i", "Edit Main Volume", channel);
    else if(_flag == SOUNDEFFECTCHOOSER_SUSTAIN)
            sprintf(T, "%s / Channel: %2.2i", "Edit Sustain", channel);
    else if(_flag == SOUNDEFFECTCHOOSER_SOSTENUTO)
        sprintf(T, "%s / Channel: %2.2i", "Edit Sostenuto", channel);
    else if(_flag == SOUNDEFFECTCHOOSER_ATTACK)
        sprintf(T, "%s / Channel: %2.2i", "Edit Attack Time", channel);
    else if(_flag == SOUNDEFFECTCHOOSER_RELEASE)
        sprintf(T, "%s / Channel: %2.2i", "Edit Release Time", channel);
    else if(_flag == SOUNDEFFECTCHOOSER_DECAY)
        sprintf(T, "%s / Channel: %2.2i", "Edit Decay Time", channel);
    else if(_flag == SOUNDEFFECTCHOOSER_REVERB)
        sprintf(T, "%s / Channel: %2.2i", "Edit Reverb Level", channel);
    else if(_flag == SOUNDEFFECTCHOOSER_CHORUS)
        sprintf(T, "%s / Channel: %2.2i", "Edit Chorus Level", channel);
    else sprintf(T, "%s / Channel: %2.2i", "Edit", channel);

    SoundEffectChooser->setWindowTitle(QCoreApplication::translate("SoundEffectChooser", (const char *) T, nullptr));
    if(label1)
        label1->setText("Modulation Wheel");

    if(label2)
        label2->setText("Pan");
    if(labelx)
        labelx->setText("Delete this Control Change");


    if(label3)
        label3->setText("Main Volume");
    if(label4)
        label4->setText("Sustain");
    if(label5)
        label5->setText("Sostenuto");

    if(label6)
        label6->setText("Attack Time");
    if(label7)
        label7->setText("Release Time");
    if(label8)
        label8->setText("Decay Time");
    if(label9)
        label9->setText("Reverb Level");
    if(label10)
        label10->setText("Chorus Level");
    if(label11)
        label11->setText("Pitch Bend");



    if(_flag >= SOUNDEFFECTCHOOSER_ALL) {
        ModulationWheelCheck->setText(QCoreApplication::translate("SoundEffectChooser", "CheckBox", nullptr));
        PanCheck->setText(QCoreApplication::translate("SoundEffectChooser", "CheckBox", nullptr));
        MainVolumeCheck->setText(QCoreApplication::translate("SoundEffectChooser", "CheckBox", nullptr));
        SustainCheck->setText(QCoreApplication::translate("SoundEffectChooser", "CheckBox", nullptr));
        SostenutoCheck->setText(QCoreApplication::translate("SoundEffectChooser", "CheckBox", nullptr));
        AttackCheck->setText(QCoreApplication::translate("SoundEffectChooser", "CheckBox", nullptr));
        ReleaseCheck->setText(QCoreApplication::translate("SoundEffectChooser", "CheckBox", nullptr));
        DecayCheck->setText(QCoreApplication::translate("SoundEffectChooser", "CheckBox", nullptr));
        ReverbCheck->setText(QCoreApplication::translate("SoundEffectChooser", "CheckBox", nullptr));
        PitchBendCheck->setText(QCoreApplication::translate("SoundEffectChooser", "CheckBox", nullptr));

    }

    if(flag > SOUNDEFFECTCHOOSER_ALL) {

        if(flag == SOUNDEFFECTCHOOSER_FORMIDIIN) {

            PitchBendCheck->setDisabled(true);
            PitchBendVal->setDisabled(true);
            SustainCheck->setDisabled(true);
            SustainButton->setDisabled(true);
            SustainButton_2->setDisabled(true);
            SostenutoCheck->setDisabled(true);
            SostenutoButton->setDisabled(true);
            SostenutoButton_2->setDisabled(true);
        }

        int dtick= _file->tick(150); // range

        foreach (MidiEvent* event, _file->channel(_channel)->eventMap()->values()) {
            if(event->midiTime() >= _tick_edit - dtick && event->midiTime() < _tick_edit + dtick) {
                ControlChangeEvent* ctrl = dynamic_cast<ControlChangeEvent*>(event);
                PitchBendEvent* pevent = dynamic_cast<PitchBendEvent*>(event);

                if (ctrl && ctrl->control() == 91 ) { // reverb
                    ReverbVal->setValue(ctrl->value());
                    ReverbCheck->setStyleSheet(QString::fromUtf8("background-color: yellow;"));
                    ReverbCheck->setChecked(true);
                    ReverbVal->update();
                }
                if (ctrl && ctrl->control() == 93 ) { // chorus
                    ChorusVal->setValue(ctrl->value());
                    ChorusCheck->setStyleSheet(QString::fromUtf8("background-color: yellow;"));
                    ChorusCheck->setChecked(true);
                    ChorusVal->update();
                }
                if (ctrl && ctrl->control() == 73 ) { // attack
                    AttackVal->setValue(ctrl->value());
                    AttackCheck->setStyleSheet(QString::fromUtf8("background-color: yellow;"));
                    AttackCheck->setChecked(true);
                    AttackVal->update();
                }
                if (ctrl && ctrl->control() == 72 ) { // release
                    ReleaseVal->setValue(ctrl->value());
                    ReleaseCheck->setStyleSheet(QString::fromUtf8("background-color: yellow;"));
                    ReleaseCheck->setChecked(true);
                    ReleaseVal->update();
                }
                if (ctrl && ctrl->control() == 75 ) { // decay
                    DecayVal->setValue(ctrl->value());
                    DecayCheck->setStyleSheet(QString::fromUtf8("background-color: yellow;"));
                    DecayCheck->setChecked(true);
                    DecayVal->update();
                }
                if (ctrl && ctrl->control() == 1 ) { // modulation
                    ModulationWheelVal->setValue(ctrl->value());
                    ModulationWheelCheck->setStyleSheet(QString::fromUtf8("background-color: yellow;"));
                    ModulationWheelCheck->setChecked(true);
                    ModulationWheelVal->update();
                }
                if (ctrl && ctrl->control() == 10 ) { // pan
                    PanVal->setValue(ctrl->value());
                    PanCheck->setStyleSheet(QString::fromUtf8("background-color: yellow;"));
                    PanCheck->setChecked(true);
                    PanVal->update();
                }
                if (ctrl && ctrl->control() == 7 ) { // volume
                    MainVolumeVal->setValue(ctrl->value());
                    MainVolumeCheck->setStyleSheet(QString::fromUtf8("background-color: yellow;"));
                    MainVolumeCheck->setChecked(true);
                    MainVolumeVal->update();
                }
                if (ctrl && ctrl->control() == 64 ) { // sustain
                    if(ctrl->value() >= 64)
                        SustainButton->setChecked(true);
                    else
                        SustainButton_2->setChecked(true);

                    SustainCheck->setStyleSheet(QString::fromUtf8("background-color: yellow;"));
                    SustainCheck->setChecked(true);
                    SustainButton->update();
                }
                if (ctrl && ctrl->control() == 66 ) { // sostenuto
                    if(ctrl->value() >= 64)
                        SostenutoButton->setChecked(true);
                    else
                        SostenutoButton_2->setChecked(true);

                    SostenutoCheck->setStyleSheet(QString::fromUtf8("background-color: yellow;"));
                    SostenutoCheck->setChecked(true);
                    SostenutoButton->update();
                }
                if (pevent && pevent->channel() == _channel ) { // pitch bend
                    PitchBendVal->setValue(((double) pevent->value())*100.0/8192.0-100.0
                                           + ((pevent->value() > 8192) ? 0.5 : 0.0));

                    PitchBendCheck->setStyleSheet(QString::fromUtf8("background-color: yellow;"));
                    PitchBendCheck->setChecked(true);
                    PitchBendVal->update();
                }
            }

        }
    }

    QObject::connect(buttonBox, SIGNAL(accepted()), SoundEffectChooser, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), SoundEffectChooser, SLOT(reject()));

    QMetaObject::connectSlotsByName(SoundEffectChooser);
}

void SoundEffectChooser::accept() {

    result = 1;
    if(_flag < SOUNDEFFECTCHOOSER_ALL) {
        QDialog::accept();
        return;
    }

    MidiTrack* track = 0;

    // get events
    foreach (MidiEvent* event, _file->channel(_channel)->eventMap()->values()) {
        ControlChangeEvent* ctrl = dynamic_cast<ControlChangeEvent*>(event);
        if (ctrl && ctrl->control()!=0 ) {
            track = ctrl->track(); break;
        }
    }
    if (!track) {
        track = _file->track(0);
    }

    ControlChangeEvent* event = 0;
    PitchBendEvent *Pevent = 0;

    _file->protocol()->startNewAction("Sound Effect for channel");

    if(ModulationWheelCheck->isChecked()) {
        event = new ControlChangeEvent(_channel, 1, ModulationWheelVal->value(), track);
        _file->channel(_channel)->insertEvent(event, _tick_edit);

        // borra eventos repetidos proximos
        foreach (MidiEvent* event2, *(_file->eventsBetween(_tick_edit - 10, _tick_edit + 10))) {
            ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event2);
            if (toRemove && event2->channel()==_channel && toRemove != event && toRemove->control() == 1) {
                _file->channel(_channel)->removeEvent(toRemove);
            }
        }

    } else if(_flag > SOUNDEFFECTCHOOSER_ALL) {
        // borra eventos repetidos proximos
        foreach (MidiEvent* event2, *(_file->eventsBetween(_tick_edit - 10, _tick_edit + 10))) {
            ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event2);
            if (toRemove && event2->channel()==_channel && toRemove != event && toRemove->control() == 1) {
                _file->channel(_channel)->removeEvent(toRemove);
            }
        }
    }

    if(PanCheck->isChecked()) {

        event = new ControlChangeEvent(_channel, 10, PanVal->value(), track);
        _file->channel(_channel)->insertEvent(event, _tick_edit);

        // borra eventos repetidos proximos
        foreach (MidiEvent* event2, *(_file->eventsBetween(_tick_edit - 10, _tick_edit + 10))) {
            ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event2);
            if (toRemove && event2->channel()==_channel && toRemove != event && toRemove->control() == 10) {
                _file->channel(_channel)->removeEvent(toRemove);
            }
        }

    } else if(_flag > SOUNDEFFECTCHOOSER_ALL) {
        // borra eventos repetidos proximos
        foreach (MidiEvent* event2, *(_file->eventsBetween(_tick_edit - 10, _tick_edit + 10))) {
            ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event2);
            if (toRemove && event2->channel()==_channel && toRemove != event && toRemove->control() == 10) {
                _file->channel(_channel)->removeEvent(toRemove);
            }
        }
    }

    if(MainVolumeCheck->isChecked()) {

        event = new ControlChangeEvent(_channel, 7, MainVolumeVal->value(), track);
        _file->channel(_channel)->insertEvent(event, _tick_edit);

        // borra eventos repetidos proximos
        foreach (MidiEvent* event2, *(_file->eventsBetween(_tick_edit - 10, _tick_edit + 10))) {
            ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event2);
            if (toRemove && event2->channel()==_channel && toRemove != event && toRemove->control() == 7) {
                _file->channel(_channel)->removeEvent(toRemove);
            }
        }
    } else if(_flag > SOUNDEFFECTCHOOSER_ALL) {
        // borra eventos repetidos proximos
        foreach (MidiEvent* event2, *(_file->eventsBetween(_tick_edit - 10, _tick_edit + 10))) {
            ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event2);
            if (toRemove && event2->channel()==_channel && toRemove != event && toRemove->control() == 7) {
                _file->channel(_channel)->removeEvent(toRemove);
            }
        }
    }

    if(SustainCheck->isChecked()) {

        event = new ControlChangeEvent(_channel, 64, (SustainButton->isChecked()) ? 127 : 0, track);
        _file->channel(_channel)->insertEvent(event, _tick_edit);

        // borra eventos repetidos proximos
        foreach (MidiEvent* event2, *(_file->eventsBetween(_tick_edit - 10, _tick_edit + 10))) {
            ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event2);
            if (toRemove && event2->channel()==_channel && toRemove != event && toRemove->control() == 64) {
                _file->channel(_channel)->removeEvent(toRemove);
            }
        }
    } else if(_flag > SOUNDEFFECTCHOOSER_ALL) {
        // borra eventos repetidos proximos
        foreach (MidiEvent* event2, *(_file->eventsBetween(_tick_edit - 10, _tick_edit + 10))) {
            ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event2);
            if (toRemove && event2->channel()==_channel && toRemove != event && toRemove->control() == 64) {
                _file->channel(_channel)->removeEvent(toRemove);
            }
        }
    }

    if(SostenutoCheck->isChecked()) {

        event = new ControlChangeEvent(_channel, 66, (SostenutoButton->isChecked()) ? 127 : 0, track);
        _file->channel(_channel)->insertEvent(event, _tick_edit);

        // borra eventos repetidos proximos
        foreach (MidiEvent* event2, *(_file->eventsBetween(_tick_edit - 10, _tick_edit + 10))) {
            ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event2);
            if (toRemove && event2->channel()==_channel && toRemove != event && toRemove->control() == 66) {
                _file->channel(_channel)->removeEvent(toRemove);
            }
        }
    } else if(_flag > SOUNDEFFECTCHOOSER_ALL) {
        // borra eventos repetidos proximos
        foreach (MidiEvent* event2, *(_file->eventsBetween(_tick_edit - 10, _tick_edit + 10))) {
            ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event2);
            if (toRemove && event2->channel()==_channel && toRemove != event && toRemove->control() == 66) {
                _file->channel(_channel)->removeEvent(toRemove);
            }
        }
    }

    if(AttackCheck->isChecked()) {

        event = new ControlChangeEvent(_channel, 73, AttackVal->value(), track);
        _file->channel(_channel)->insertEvent(event, _tick_edit);

        // borra eventos repetidos proximos
        foreach (MidiEvent* event2, *(_file->eventsBetween(_tick_edit - 10, _tick_edit + 10))) {
            ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event2);
            if (toRemove && event2->channel()==_channel && toRemove != event && toRemove->control() == 73) {
                _file->channel(_channel)->removeEvent(toRemove);
            }
        }
    } else if(_flag > SOUNDEFFECTCHOOSER_ALL) {
        // borra eventos repetidos proximos
        foreach (MidiEvent* event2, *(_file->eventsBetween(_tick_edit - 10, _tick_edit + 10))) {
            ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event2);
            if (toRemove && event2->channel()==_channel && toRemove != event && toRemove->control() == 73) {
                _file->channel(_channel)->removeEvent(toRemove);
            }
        }
    }

    if(ReleaseCheck->isChecked()) {

        event = new ControlChangeEvent(_channel, 72, ReleaseVal->value(), track);
        _file->channel(_channel)->insertEvent(event, _tick_edit);

        // borra eventos repetidos proximos
        foreach (MidiEvent* event2, *(_file->eventsBetween(_tick_edit - 10, _tick_edit + 10))) {
            ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event2);
            if (toRemove && event2->channel()==_channel && toRemove != event && toRemove->control() == 72) {
                _file->channel(_channel)->removeEvent(toRemove);
            }
        }
    } else if(_flag > SOUNDEFFECTCHOOSER_ALL) {
        // borra eventos repetidos proximos
        foreach (MidiEvent* event2, *(_file->eventsBetween(_tick_edit - 10, _tick_edit + 10))) {
            ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event2);
            if (toRemove && event2->channel()==_channel && toRemove != event && toRemove->control() == 72) {
                _file->channel(_channel)->removeEvent(toRemove);
            }
        }
    }

    if(DecayCheck->isChecked()) {

        event = new ControlChangeEvent(_channel, 75, DecayVal->value(), track);
        _file->channel(_channel)->insertEvent(event, _tick_edit);

        // borra eventos repetidos proximos
        foreach (MidiEvent* event2, *(_file->eventsBetween(_tick_edit - 10, _tick_edit + 10))) {
            ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event2);
            if (toRemove && event2->channel()==_channel && toRemove != event && toRemove->control() == 75) {
                _file->channel(_channel)->removeEvent(toRemove);
            }
        }
    } else if(_flag > SOUNDEFFECTCHOOSER_ALL) {
        // borra eventos repetidos proximos
        foreach (MidiEvent* event2, *(_file->eventsBetween(_tick_edit - 10, _tick_edit + 10))) {
            ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event2);
            if (toRemove && event2->channel()==_channel && toRemove != event && toRemove->control() == 75) {
                _file->channel(_channel)->removeEvent(toRemove);
            }
        }
    }

    if(ReverbCheck->isChecked()) {

        event = new ControlChangeEvent(_channel, 91, ReverbVal->value(), track);
        _file->channel(_channel)->insertEvent(event, _tick_edit);

        // borra eventos repetidos proximos
        foreach (MidiEvent* event2, *(_file->eventsBetween(_tick_edit - 10, _tick_edit + 10))) {
            ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event2);
            if (toRemove && event2->channel()==_channel && toRemove != event && toRemove->control() == 91) {
                _file->channel(_channel)->removeEvent(toRemove);
            }
        }
    } else if(_flag > SOUNDEFFECTCHOOSER_ALL) {
        // borra eventos repetidos proximos
        foreach (MidiEvent* event2, *(_file->eventsBetween(_tick_edit - 10, _tick_edit + 10))) {
            ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event2);
            if (toRemove && event2->channel()==_channel && toRemove != event && toRemove->control() == 91) {
                _file->channel(_channel)->removeEvent(toRemove);
            }
        }
    }

    if(ChorusCheck->isChecked()) {

        event = new ControlChangeEvent(_channel, 93, ChorusVal->value(), track);
        _file->channel(_channel)->insertEvent(event, _tick_edit);

        // borra eventos repetidos proximos
        foreach (MidiEvent* event2, *(_file->eventsBetween(_tick_edit - 10, _tick_edit + 10))) {
            ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event2);
            if (toRemove && event2->channel()==_channel && toRemove != event && toRemove->control() == 93) {
                _file->channel(_channel)->removeEvent(toRemove);
            }
        }
    } else if(_flag > SOUNDEFFECTCHOOSER_ALL) {
        // borra eventos repetidos proximos
        foreach (MidiEvent* event2, *(_file->eventsBetween(_tick_edit - 10, _tick_edit + 10))) {
            ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event2);
            if (toRemove && event2->channel()==_channel && toRemove != event && toRemove->control() == 93) {
                _file->channel(_channel)->removeEvent(toRemove);
            }
        }
    }

    if(PitchBendCheck->isChecked()) {

        int clicks=-1;

        // encuentra notas afectadas
        if(_flag == SOUNDEFFECTCHOOSER_ALL) {
            foreach (MidiEvent* event2, *(_file->eventsBetween(0, _file->endTick()))) {
                NoteOnEvent *on = dynamic_cast<NoteOnEvent*>(event2);
                if (on && event2->channel()  ==_channel) {

                    MidiEvent* off = (MidiEvent* ) on->offEvent();
                    if(off && _tick_edit >= on->midiTime()
                            && _tick_edit <= off->midiTime()) {

                        if(clicks<off->midiTime()) clicks=off->midiTime();

                    }

                }
            }
        }


        // inserta pitchbend on
        Pevent = new PitchBendEvent(_channel, ((PitchBendVal->value() + 100)*8192/100) & 16383, track);
        _file->channel(_channel)->insertEvent(Pevent, _tick_edit);

        // borra eventos repetidos proximos
        foreach (MidiEvent* event2, *(_file->eventsBetween(_tick_edit - 10, _tick_edit + 10))) {
            PitchBendEvent* toRemove = dynamic_cast<PitchBendEvent*>(event2);
            if (toRemove && event2->channel()==_channel && toRemove != Pevent) {
                _file->channel(_channel)->removeEvent(toRemove);
            }
        }

        if(clicks >= 0) {

            // inserta pitchbend off
            Pevent = new PitchBendEvent(_channel, 8192, track);
            _file->channel(_channel)->insertEvent(Pevent, clicks);

            // borra eventos repetidos proximos
            foreach (MidiEvent* event2, *(_file->eventsBetween(clicks-10, clicks+10))) {
                PitchBendEvent* toRemove = dynamic_cast<PitchBendEvent*>(event2);
                if (toRemove && event2->channel()==_channel && toRemove != Pevent) {
                    _file->channel(_channel)->removeEvent(toRemove);
                }
            }
        }
    } else if(_flag > SOUNDEFFECTCHOOSER_ALL) {
        // borra eventos repetidos proximos
        foreach (MidiEvent* event2, *(_file->eventsBetween(_tick_edit - 10, _tick_edit + 10))) {
            PitchBendEvent* toRemove = dynamic_cast<PitchBendEvent*>(event2);
            if (toRemove && event2->channel()==_channel && toRemove != Pevent) {
                _file->channel(_channel)->removeEvent(toRemove);
            }
        }

    }


    _file->protocol()->endAction();

    QDialog::accept();

}



