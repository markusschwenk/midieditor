/*
 * MidiEditor
 * Copyright (C) 2010  Markus Schwenk
 *
 * Notes_util
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


#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSlider>

#include "../gui/MainWindow.h"
#include "../gui/ChannelListWidget.h"

#include "../tool/EventTool.h"
#include "../tool/Selection.h"
#include "../tool/FingerPatternDialog.h"

#include "../protocol/Protocol.h"

#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/NoteOnEvent.h"
#include "../MidiEvent/OffEvent.h"
#include "../MidiEvent/TextEvent.h"
#include "../MidiEvent/PitchBendEvent.h"
#include "../MidiEvent/SysExEvent.h"

#include "../midi/MidiChannel.h"
#include "../midi/MidiFile.h"
#include "../midi/MidiOutput.h"

#include "../gui/MidiEditorInstrument.h"
#include "../gui/TextEventEdit.h"

#ifdef USE_FLUIDSYNTH
#include "../fluid/fluidsynth_proc.h"
#include "../fluid/FluidDialog.h"
#endif

extern int _piano_insert_mode;

static QWidget *MW = NULL;

QDialog *velocityd;
int velocityd_result =0;

extern instrument_list InstrumentList[129];
extern int itHaveInstrumentList;
extern int itHaveDrumList;


extern int chord_noteM(int note, int position);
extern int chord_notem(int note, int position);


class chordVelDialog: public QDialog
{

public:
    QDialogButtonBox *buttonBox;
    QLabel *label3;
    QSlider *Slider3;
    QLabel *label5;
    QSlider *Slider5;
    QLabel *label7;
    QSlider *Slider7;

    chordVelDialog(QWidget* parent);

private:
    QPushButton *rstButton;

};

static int pnote3 = 14;
static int pnote5 = 15;
static int pnote7 = 16;

chordVelDialog::chordVelDialog(QWidget* parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint) {

    QDialog *chord = this;

    if (chord->objectName().isEmpty())
        chord->setObjectName(QString::fromUtf8("chord"));

    chord->resize(293, 256);
    chord->setFixedSize(293, 256);

    buttonBox = new QDialogButtonBox(chord);
    buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
    buttonBox->setGeometry(QRect(30, 190, 191, 31));
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    label3 = new QLabel(chord);
    label3->setObjectName(QString::fromUtf8("label3"));
    label3->setGeometry(QRect(30, 50, 21, 21));
    label3->setAlignment(Qt::AlignCenter);
    Slider3 = new QSlider(chord);
    Slider3->setObjectName(QString::fromUtf8("Slider3"));
    Slider3->setGeometry(QRect(60, 50, 171, 22));
    Slider3->setMaximum(20);
    Slider3->setValue(pnote3);
    Slider3->setOrientation(Qt::Horizontal);
    Slider3->setTickPosition(QSlider::TicksBelow);
    label5 = new QLabel(chord);
    label5->setObjectName(QString::fromUtf8("label5"));
    label5->setGeometry(QRect(30, 90, 21, 21));
    label5->setAlignment(Qt::AlignCenter);
    Slider5 = new QSlider(chord);
    Slider5->setObjectName(QString::fromUtf8("Slider5"));
    Slider5->setGeometry(QRect(60, 90, 171, 22));
    Slider5->setMaximum(20);
    Slider5->setValue(pnote5);
    Slider5->setOrientation(Qt::Horizontal);
    Slider5->setTickPosition(QSlider::TicksBelow);
    label7 = new QLabel(chord);
    label7->setObjectName(QString::fromUtf8("label7"));
    label7->setGeometry(QRect(30, 130, 21, 21));
    label7->setAlignment(Qt::AlignCenter);
    Slider7 = new QSlider(chord);
    Slider7->setObjectName(QString::fromUtf8("Slider7"));
    Slider7->setGeometry(QRect(60, 130, 171, 22));
    Slider7->setMaximum(20);
    Slider7->setValue(pnote7);
    Slider7->setOrientation(Qt::Horizontal);
    Slider7->setTickPosition(QSlider::TicksBelow);

    chord->setWindowTitle(QCoreApplication::translate("chord", "Chord Proportional Velocity", nullptr));
    label3->setText("3:");
    label5->setText("5:");
    label7->setText("7:");

    rstButton = new QPushButton(chord);
    rstButton->setObjectName(QString::fromUtf8("rstButton"));
    rstButton->setGeometry(QRect(110, 10, 75, 23));
    rstButton->setText("Reset");

    connect(rstButton, QOverload<bool>::of(&QPushButton::clicked), [=](bool)
    {
        pnote3 = 14;
        pnote5 = 15;
        pnote7 = 16;

        Slider3->setValue(pnote3);
        Slider5->setValue(pnote5);
        Slider7->setValue(pnote7);
    });

    QObject::connect(buttonBox, SIGNAL(accepted()), chord, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), chord, SLOT(reject()));

    QMetaObject::connectSlotsByName(chord);
}

void MainWindow::Notes_util(QWidget * _MW) {

    MW = _MW;

    pnote3 = _settings->value("pnote3v", 14).toInt();
    pnote5 = _settings->value("pnote5v", 15).toInt();
    pnote7 = _settings->value("pnote7v", 16).toInt();

}

void MainWindow::setChordVelocityProp() {


    chordVelDialog* d = new chordVelDialog(this);

    if(d->exec() == QDialog::Accepted) {

        pnote3 = d->Slider3->value();
        pnote5 = d->Slider5->value();
        pnote7 = d->Slider7->value();

        _settings->setValue("pnote3v", pnote3);
        _settings->setValue("pnote5v", pnote5);
        _settings->setValue("pnote7v", pnote7);
    }

    delete d;
}

void MainWindow::velocity_accept()
{
    velocityd_result = 1;
    velocityd->hide();
}

void MainWindow::velocityScale() {
    if (!(Selection::instance()->selectedEvents().size() > 0 && file)) return;

    velocityd= new QDialog(this, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

    QFrame *frame1, *line;
    QDialogButtonBox *buttonBox;
    QLabel *label1;
    QSlider *velocityVal;
    velocityd_result = 0;

    int x, y;

    if (velocityd->objectName().isEmpty())
        velocityd->setObjectName(QString::fromUtf8("Velocity Correction"));

    velocityd->resize(461, 175);

    velocityd->setInputMethodHints(Qt::ImhNone);
    buttonBox = new QDialogButtonBox(velocityd);
    buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
    buttonBox->setGeometry(QRect(360, 20, 81, 241));
    buttonBox->setOrientation(Qt::Vertical);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);


    x = 40; y= 20;

    frame1 = new QFrame(velocityd);
    frame1->setObjectName(QString::fromUtf8("frame1"));
    frame1->setGeometry(QRect(x - 10, y + 5, 211, 50));
    frame1->setFrameShape(QFrame::Box);
    frame1->setFrameShadow(QFrame::Raised);
    label1 = new QLabel(velocityd);
    label1->setObjectName(QString::fromUtf8("label1"));
    label1->setGeometry(QRect(x + 30, y, 111, 31));
    label1->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
    line = new QFrame(velocityd);
    line->setObjectName(QString::fromUtf8("line"));
    line->setGeometry(QRect(x + 100, y + 27, 20, 26));
    line->setFrameShadow(QFrame::Sunken);
    line->setLineWidth(1);
    line->setMidLineWidth(0);
    line->setFrameShape(QFrame::VLine);
    velocityVal = new QSlider(velocityd);
    velocityVal->setObjectName(QString::fromUtf8("velocityVal"));
    velocityVal->setGeometry(QRect(x + 30, y + 30, 160, 22));
    velocityVal->setMaximum(200);
    velocityVal->setValue(100);
    velocityVal->setOrientation(Qt::Horizontal);

    velocityd->setWindowTitle("Velocity Scale");
    if(label1)
        label1->setText("Velocity Percent");

   QObject::connect(buttonBox, SIGNAL(accepted()), this, SLOT(velocity_accept()));
   QObject::connect(buttonBox, SIGNAL(rejected()), velocityd, SLOT(reject()));

    QMetaObject::connectSlotsByName(velocityd);

    velocityd->setModal(true);

    if(!velocityd->exec() && velocityd_result) {
        int val=velocityVal->value();

        file->protocol()->startNewAction("Velocity Scale", 0);

        foreach (MidiEvent* e, Selection::instance()->selectedEvents()) {
            NoteOnEvent *on = dynamic_cast<NoteOnEvent*>(e);
            if (on) {
                int vel = on->velocity() * val / 100;
                if(vel < 0) vel = 0;
                if(vel > 127) vel = 127;
                on->setVelocity(vel);
            }
        }
        file->protocol()->endAction();

    }

    delete velocityd;
}



#define REM_LONGNOTESCORRECTION        0x10
#define REM_OVERLAPPEDNOTESCORRECTION  0x11
#define REM_STRETCHNOTES               0x12
#define REM_BUILDPOWERCHORD            0x20
#define REM_BUILDPOWERCHORDINV         0x21
#define REM_BUILDPOWERPOWERCHORD       0x22

#define REM_BUILDCMAJORPROG            0x30
#define REM_BUILDCMINORPROG            0x31
#define REM_BUILDCMAJORINV1PROG        0x32
#define REM_BUILDCMINORINV1PROG        0x33
#define REM_BUILDCMAJORINV2PROG        0x34
#define REM_BUILDCMINORINV2PROG        0x35

#define REM_BUILDMAJOR                 0x40
#define REM_BUILDMINOR                 0x41
#define REM_BUILDAUG                   0x42
#define REM_BUILDIS                    0x43
#define REM_BUILDSEVENTH               0x44
#define REM_BUILDMAJORSEVENTH          0x45
#define REM_BUILDMINORSEVENTH          0x46
#define REM_BUILDMINORSEVENTHMAJOR     0x47
#define REM_PITCHBEND_EFFECT1          0x60
#define REM_VOLUMEOFF_EFFECT           0x61
#define REM_CHOPPY_AUDIO_EFFECT        0x62
#define REM_MUTE_AUDIO_EFFECT          0x63
#define REM_CONV_PATTERN_NOTE          0x64

#define TEST_IFMULTICHAN  0x1
#define TEST_IFMAXNOTES   0x2
#define TEST_IFNOTDRUM    0x4


#define TEST_MAX_NOTES    1000


#define M_MainThreadProgressDialog(a, b, c) {\
    MainThreadProgressDialog bar(this, QString(a), b, c);\
    bar.exec();\
}

void MainWindow::longNotesCorrection() {

    M_MainThreadProgressDialog(QString("Long Notes Correction Progress..."),
                           REM_LONGNOTESCORRECTION, 0);

    //NotesCorrection(2);
}

void MainWindow::overlappedNotesCorrection() {

    M_MainThreadProgressDialog(QString("Overlapped Notes Correction Progress..."),
                             REM_OVERLAPPEDNOTESCORRECTION, 0);

    //NotesCorrection(1);
}

void MainWindow::stretchNotes() {

    M_MainThreadProgressDialog(QString("Stretch Notes Progress..."),
                             REM_STRETCHNOTES, TEST_IFMULTICHAN | TEST_IFNOTDRUM);

    //NotesCorrection(3);
}

#define CNOTE  0
#define CNOTES 1
#define DNOTE  2
#define DNOTES 3
#define ENOTE  4
#define FNOTE  5
#define FNOTES 6
#define GNOTE  7
#define GNOTES 8
#define ANOTE  9
#define ANOTES 10
#define BNOTE 11

int note_base = ANOTE;

#define CM 0
#define Cm 1
#define Cd 2
//                  C   C#  D   D#  E   F   F#  G   G#  A   A#  B
int map_scaleM[12]={CM, CM, Cm, CM, Cm, CM, CM, CM, CM, Cm, CM, Cd};
int map_scalem[12]={Cm, Cm, Cd, Cm, CM, Cm, Cm, Cm, Cm, CM, Cm, CM};


int chord_noteM(int note, int position) {

    int chord= map_scaleM[note % 12];

    if(position <= 1) return note; // fundamental

    if(position == 3) {
        if(chord == 0) return note + 4; else return note + 3;
    }

    if(position == 5) {
        if(chord == 2) return note + 6; else return note + 7;
    }

    if(position == 7) {
        if(chord == 2) return note + 10; else return note + 11;
    }

    return -1;

}

int chord_notem(int note, int position) {

    int chord = map_scalem[note % 12];

    if(chord == 0) note--;

    if(position <= 1) return note; // fundamental

    if(position == 3) {
        if(chord == 0) return note + 4; else return note + 3;
    }

    if(position == 5) {
        if(chord == 2) return note + 6; else return note + 7;
    }

    if(position == 7) {
        if(chord == 2) return note + 10; else return note + 11;
    }

    return -1;
}

void MainWindow::buildCMajor() {
    M_MainThreadProgressDialog(QString("Build C Major Chord Progress..."),
                           REM_BUILDCMAJORPROG, TEST_IFMULTICHAN | TEST_IFNOTDRUM);
    // buildCMajorProg(0, 0, 666);
}

void MainWindow::buildCMinor() {
    M_MainThreadProgressDialog(QString("Build C Minor Chord Progress..."),
                           REM_BUILDCMINORPROG, TEST_IFMULTICHAN | TEST_IFNOTDRUM);
    // buildCMinorProg(0, 0, 666);
}

void MainWindow::buildCMajorInv1() {
    M_MainThreadProgressDialog(QString("Build C Major Inv 1 Chord Progress..."),
                           REM_BUILDCMAJORINV1PROG, TEST_IFMULTICHAN | TEST_IFNOTDRUM);
    // buildCMajorProg(-12, -12, 666);
}

void MainWindow::buildCMinorInv1() {
    M_MainThreadProgressDialog(QString("Build C Minor Inv 1 Chord Progress..."),
                           REM_BUILDCMINORINV1PROG, TEST_IFMULTICHAN | TEST_IFNOTDRUM);
    // buildCMinorProg(-12, -12, 666);
}

void MainWindow::buildCMajorInv2() {
    M_MainThreadProgressDialog(QString("Build C Major Inv 2 Chord Progress..."),
                           REM_BUILDCMAJORINV2PROG, TEST_IFMULTICHAN | TEST_IFNOTDRUM);
    // buildCMajorProg(0, -12, 666);
}

void MainWindow::buildCMinorInv2() {
    M_MainThreadProgressDialog(QString("Build C Minor Inv 2 Chord Progress..."),
                           REM_BUILDCMINORINV2PROG, TEST_IFMULTICHAN | TEST_IFNOTDRUM);
    // buildCMinorProg(0, -12, 666);
}


void MainWindow::buildpowerchord() {
    M_MainThreadProgressDialog(QString("Build Power Chord Progress..."),
                             REM_BUILDPOWERCHORD, TEST_IFMULTICHAN | TEST_IFNOTDRUM);

    //builddichord(666, 7, 666);
}

void MainWindow::buildpowerchordInv() {
    M_MainThreadProgressDialog(QString("Build Power Chord Inv Progress..."),
                             REM_BUILDPOWERCHORDINV, TEST_IFMULTICHAN | TEST_IFNOTDRUM);

    //builddichord(666, -5, 666);
}

void MainWindow::buildpowerpowerchord() {
    M_MainThreadProgressDialog(QString("Build Power Chord Extended Progress..."),
                             REM_BUILDPOWERPOWERCHORD, TEST_IFMULTICHAN | TEST_IFNOTDRUM);

    //builddichord(7, 12, 666);
}

void MainWindow::buildMajor() {
    M_MainThreadProgressDialog(QString("Build Major Chord Progress..."),
                           REM_BUILDMAJOR, TEST_IFMULTICHAN | TEST_IFNOTDRUM);
    // builddichord(4, 7, 666);
}

void MainWindow::buildMinor() {
    M_MainThreadProgressDialog(QString("Build Minor Chord Progress..."),
                           REM_BUILDMINOR, TEST_IFMULTICHAN | TEST_IFNOTDRUM);
    // builddichord(3, 7, 666);
}

void MainWindow::buildAug() {
    M_MainThreadProgressDialog(QString("Build Augmented Chord Progress..."),
                           REM_BUILDAUG, TEST_IFMULTICHAN | TEST_IFNOTDRUM);
    // builddichord(4, 8, 666);
}

void MainWindow::buildDis() {
    M_MainThreadProgressDialog(QString("Build Diminished Chord Progress..."),
                           REM_BUILDIS, TEST_IFMULTICHAN | TEST_IFNOTDRUM);
    // builddichord(3, 6, 666);
}

void MainWindow::buildSeventh() {
    M_MainThreadProgressDialog(QString("Build Seventh Chord Progress..."),
                           REM_BUILDSEVENTH, TEST_IFMULTICHAN | TEST_IFNOTDRUM);
    // builddichord(4, 7, 10);
}

void MainWindow::buildMajorSeventh() {
    M_MainThreadProgressDialog(QString("Build Major Seventh Chord Progress..."),
                           REM_BUILDMAJORSEVENTH, TEST_IFMULTICHAN | TEST_IFNOTDRUM);
    // builddichord(4, 7, 11);
}

void MainWindow::buildMinorSeventh() {
    M_MainThreadProgressDialog(QString("Build Minor Seventh Chord Progress..."),
                           REM_BUILDMINORSEVENTH, TEST_IFMULTICHAN | TEST_IFNOTDRUM);
    // builddichord(3, 7, 10);
}

void MainWindow::buildMinorSeventhMajor() {
    M_MainThreadProgressDialog(QString("Build Minor with 7 Major Chord Progress..."),
                           REM_BUILDMINORSEVENTHMAJOR, TEST_IFMULTICHAN | TEST_IFNOTDRUM);
    // builddichord(3, 7, 11);
}

void MainWindow::pitchbend_effect1() {
    M_MainThreadProgressDialog(QString("Pitch Bend Effect1 Progress..."),
                           REM_PITCHBEND_EFFECT1, TEST_IFMULTICHAN | TEST_IFMAXNOTES | TEST_IFNOTDRUM);
}

void MainWindow::volumeoff_effect() {
    M_MainThreadProgressDialog(QString("Volume Off effect Progress..."),
                           REM_VOLUMEOFF_EFFECT, TEST_IFMULTICHAN | TEST_IFMAXNOTES);

}

void MainWindow::choppy_audio_effect() {
    M_MainThreadProgressDialog(QString("Choppy Audio Effect Progress..."),
                           REM_CHOPPY_AUDIO_EFFECT, TEST_IFMULTICHAN | TEST_IFMAXNOTES | TEST_IFNOTDRUM);

}

void MainWindow::mute_audio_effect() {
    M_MainThreadProgressDialog(QString("Mute Audio Effect Progress..."),
                           REM_MUTE_AUDIO_EFFECT, TEST_IFMULTICHAN | TEST_IFMAXNOTES | TEST_IFNOTDRUM);

}

void MainWindow::conv_pattern_note() {
    M_MainThreadProgressDialog(QString("Pattern Note Effect Progress..."),
                           REM_CONV_PATTERN_NOTE, TEST_IFMULTICHAN | TEST_IFMAXNOTES | TEST_IFNOTDRUM);

}

/*************************************************************************/
/* MainThreadProgressDialog section start                                */
/*************************************************************************/


MainThreadProgressDialog::MainThreadProgressDialog(MainWindow *parent, QString text, int index, int test_type)
    : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint /*| Qt::WindowCloseButtonHint*/)
{


    PBar = this;
    mt = NULL;

    if (PBar->objectName().isEmpty())
        PBar->setObjectName(QString::fromUtf8("ProgressBarMidiEditor"));

    PBar->resize(446, 173);
    PBar->setFixedSize(446, 173);
    PBar->setWindowTitle(QCoreApplication::translate("ProgressBarMidiEditor", "MidiEditor Progress Bar", nullptr));

    progressBar = new QProgressBar(PBar);
    progressBar->setObjectName(QString::fromUtf8("progressBarME"));
    progressBar->setGeometry(QRect(100, 100, 251, 23));
    progressBar->setValue(0);

    label = new QLabel(PBar);
    label->setObjectName(QString::fromUtf8("label"));
    label->setGeometry(QRect(100, 50, 221, 21));
    label->setAlignment(Qt::AlignCenter);
    label->setText(text);

    QMetaObject::connectSlotsByName(PBar);

    // selection test

    MidiFile* file = parent->getFile();

    if(Selection::instance()->selectedEvents().size() > 0 && file) {

        int cur_chan = -1;
        int notes = 0;

        foreach (MidiEvent* e, Selection::instance()->selectedEvents()) {
            NoteOnEvent *on = dynamic_cast<NoteOnEvent*>(e);
            if(on) {
                int chan = e->channel();
                if(chan >= 0 && chan < 16) { // only notes chans

                    if(test_type & TEST_IFMULTICHAN) {
                        if(cur_chan == -1) cur_chan = chan;
                        if(cur_chan != chan && cur_chan != -2) {
                            int r = QMessageBox::question( this, "MidiEditor", "You are using notes from different channels in your selection\n"
                                                                 "Are you sure to proceed?                                             ");
                            if(r != QMessageBox::Yes) {

                                index = 0x666;
                                break;

                            } else
                                cur_chan = -2;
                        }
                    }

                    if((test_type & TEST_IFNOTDRUM) && chan == 9) { // skip drum
                        continue;
                    }

                    notes++;

                    if((test_type & TEST_IFMAXNOTES) && notes >= TEST_MAX_NOTES) {
                        QMessageBox::critical(this, "MidiEditor", "Too much notes selected (max " + QString::number(TEST_MAX_NOTES) + " notes)\n"+
                                                    "Aborted procedure!!              ");

                        index = 0x666;
                        break;

                    }
                }
            }
        }

        //qWarning("nnotes %i", notes);
    }
    else
        index = 0x666;

    mt = new Main_Thread(parent);

    if(!mt)  {

       return;
    }

    mt->index = index;

    connect(mt, SIGNAL(setBar(int)), this, SLOT(setBar(int)));
    connect(mt, SIGNAL(endBar()), this, SLOT(hide()));

    mt->start(QThread::TimeCriticalPriority);

}


MainThreadProgressDialog::~MainThreadProgressDialog() {
    if(mt) {

        disconnect(mt, SIGNAL(setBar(int)), this, SLOT(setBar(int)));
        disconnect(mt, SIGNAL(endBar()), this, SLOT(hide()));

        while(!mt->isFinished()) QThread::msleep(5);

        delete mt;
        mt = NULL;
    }

}

void MainThreadProgressDialog::setBar(int num) {

   if(progressBar) {
       progressBar->setValue(num);
   }
}

void MainThreadProgressDialog::reject() {

    if(mt) {

        disconnect(mt, SIGNAL(setBar(int)), this, SLOT(setBar(int)));
        disconnect(mt, SIGNAL(endBar()), this, SLOT(hide()));

        while(!mt->isFinished()) QThread::msleep(5);

        delete mt;
        mt = NULL;
    }

    hide();
}

/*************************************************************************/
/* Main_Thread section start                                             */
/*************************************************************************/

Main_Thread::Main_Thread(MainWindow *m) : QThread(m){
    mw = m;
    file = m->getFile();
}

Main_Thread::~Main_Thread() {

}

void Main_Thread::run()
{
    loop = 1;

    switch(index) {
        case -2:

            break;
        case REM_LONGNOTESCORRECTION:
            NotesCorrection(2);
            break;
        case REM_OVERLAPPEDNOTESCORRECTION:
            NotesCorrection(1);
            break;
        case REM_STRETCHNOTES:
            NotesCorrection(3);
            break;

        case REM_BUILDPOWERCHORD:
            builddichord(666, 7, 666);
            break;

        case REM_BUILDPOWERCHORDINV:
            builddichord(666, -5, 666);
            break;

        case REM_BUILDPOWERPOWERCHORD:
            builddichord(7, 12, 666);
            break;

        case REM_BUILDCMAJORPROG:
            buildCMajorProg(0, 0, 666);
            break;

        case REM_BUILDCMINORPROG:
            buildCMinorProg(0, 0, 666);
            break;

        case REM_BUILDCMAJORINV1PROG:
            buildCMajorProg(-12, -12, 666);
            break;

        case REM_BUILDCMINORINV1PROG:
            buildCMinorProg(-12, -12, 666);
            break;

        case REM_BUILDCMAJORINV2PROG:
            buildCMajorProg(0, -12, 666);
            break;

        case REM_BUILDCMINORINV2PROG:
            buildCMinorProg(0, -12, 666);
            break;

        case REM_BUILDMAJOR:
            builddichord(4, 7, 666);
            break;

        case REM_BUILDMINOR:
            builddichord(3, 7, 666);
            break;

        case REM_BUILDAUG:
            builddichord(4, 8, 666);
            break;

        case REM_BUILDIS:
            builddichord(3, 6, 666);
            break;

        case REM_BUILDSEVENTH:
            builddichord(4, 7, 10);
            break;

        case REM_BUILDMAJORSEVENTH:
            builddichord(4, 7, 11);
            break;

        case REM_BUILDMINORSEVENTH:
            builddichord(3, 7, 10);
            break;

        case REM_BUILDMINORSEVENTHMAJOR:
            builddichord(3, 7, 11);
            break;

        case REM_PITCHBEND_EFFECT1:
            pitchbend_effect1();
            break;
        case REM_VOLUMEOFF_EFFECT:
            volumeoff_effect();
            break;

        case REM_CHOPPY_AUDIO_EFFECT:
            choppy_audio_effect();
            break;

        case REM_MUTE_AUDIO_EFFECT:
            mute_audio_effect();
            break;

        case REM_CONV_PATTERN_NOTE:
            conv_pattern_note();
            break;

        case 0x666: // do nothing
            emit endBar();
            break;

        default: // do nothing
            emit endBar();
            break;

    }

}

void Main_Thread::pitchbend_effect1() {

    if (!(Selection::instance()->selectedEvents().size() > 0 && file)) {
        emit endBar();
        return;
    }

    if (Selection::instance()->selectedEvents().size() > 0 && file) {

        QList<MidiEvent*> events;

        foreach (MidiEvent* e, Selection::instance()->selectedEvents()) {
            NoteOnEvent *on = dynamic_cast<NoteOnEvent*>(e);
            if(on) {
                int chan = e->channel();
                if(chan >= 0 && chan < 16 && chan != 9) {
                    events.append(e);
                }
            }
        }

        int max_counter = events.count();

        if(!max_counter) {
            emit endBar();
            return;
        }

        int modcount = max_counter / 100;
        if(modcount < 1) modcount = 1;


        file->protocol()->startNewAction("Pitch Bend Effect1", 0);

        int counter = 0;
        foreach (MidiEvent* e, events) {
            NoteOnEvent *on = dynamic_cast<NoteOnEvent*>(e);

            if (on) {
                int note = on->note();
                MidiEvent* off =(MidiEvent* ) on->offEvent();
                on->setNote(note);

                if((counter % modcount) == 0) {
                    emit setBar(100 * counter /  max_counter);
                }

                counter++;

                if(off){

                    // borra eventos repetidos proximos
                    foreach (MidiEvent* event2, *(file->eventsBetween(on->midiTime()-10, off->midiTime()+10))) {
                        PitchBendEvent* toRemove = dynamic_cast<PitchBendEvent*>(event2);
                        if (toRemove && event2->channel() == e->channel()) {
                            file->channel(e->channel())->removeEvent(toRemove);
                        }
                    }
                    int clicks, s = 1;

                    for(clicks = on->midiTime(); clicks < off->midiTime(); clicks+= 25) {
                        PitchBendEvent* Pevent = new PitchBendEvent(e->channel(), (48+s*4)*16383/100,
                                                                    e->track());
                        file->channel(e->channel())->insertEvent(Pevent, clicks);
                        s^=1;
                    }

                    if(s == 0) {
                        PitchBendEvent* Pevent = new PitchBendEvent(e->channel(), 8192,
                                                                e->track());
                        file->channel(e->channel())->insertEvent(Pevent, clicks);
                    }
                }
            }
        }

        file->protocol()->endAction();
    }

    emit endBar();
}

void Main_Thread::volumeoff_effect() {

    if (!(Selection::instance()->selectedEvents().size() > 0 && file)) {
        emit endBar();
        return;
    }

    if (Selection::instance()->selectedEvents().size() > 0 && file) {

        QList<MidiEvent*> events;

        foreach (MidiEvent* e, Selection::instance()->selectedEvents()) {
            NoteOnEvent *on = dynamic_cast<NoteOnEvent*>(e);
            if(on) {
                int chan = e->channel();
                if(chan >= 0 && chan < 16) {
                    events.append(e);
                }
            }
        }

        int max_counter = events.count();

        if(!max_counter) {
            emit endBar();
            return;
        }

        int modcount = max_counter / 100;
        if(modcount < 1) modcount = 1;

        file->protocol()->startNewAction("Volume Off Effect", 0);

        int tick_1 = 999999999, tick_2 = -1;

        int chan[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        MidiTrack* track[16];

        int counter = 0;
        foreach (MidiEvent* e, events) {
            NoteOnEvent *on = dynamic_cast<NoteOnEvent*>(e);

            if (on) {
                int note = on->note();
                MidiEvent* off =(MidiEvent* ) on->offEvent();
                on->setNote(note);

                if((counter % modcount) == 0) {
                    emit setBar(100 * counter /  max_counter);
                }

                counter++;

                if(on->midiTime() < tick_1) tick_1 = on->midiTime();

                int c = on->channel();
                if(c >= 0 && c < 16) {
                    chan[c] = 1;
                    track[c] = e->track();
                }

                if(off){

                    if(off->midiTime() > tick_2) tick_2 = off->midiTime();

                }
            }
        }

        if(tick_1 != 999999999 && tick_2 != -1) {

            for(int n = 0; n < 16; n++) {

                if(chan[n]) {

                    // remove channel volume events from this interval...
                    foreach (MidiEvent* event2, *(file->eventsBetween(tick_1-10, tick_2+10))) {
                        ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event2);
                        if (toRemove && event2->channel()==n && toRemove->control() == 7) {
                            file->channel(n)->removeEvent(toRemove);
                        }
                    }


                    // get last value for channel volume
                    int old_vol = 100;
                    foreach (MidiEvent* event2, *(file->eventsBetween(0, tick_2+10))) {
                        ControlChangeEvent* chanVol = dynamic_cast<ControlChangeEvent*>(event2);
                        if (chanVol && event2->channel()==n && chanVol->control() == 7) {
                            old_vol = chanVol->value();

                        }
                    }

                    // progressive decay channel volume
                    int clicks, nclicks, vol, volstep;

                    nclicks = (tick_2-tick_1)/8;
                    if(nclicks < 1) nclicks = 1;
                    volstep = old_vol/8;
                    if(volstep < 0) volstep = 1;

                    vol = old_vol;

                    for(clicks = tick_1; clicks < tick_2; clicks+= nclicks) {

                        ControlChangeEvent* Pevent = new ControlChangeEvent(n, 7, vol, track[n]);
                        file->channel(n)->insertEvent(Pevent, clicks);
                        vol -= volstep; if(vol < 0) break;
                    }

                    // restore channel volume

                    ControlChangeEvent* Pevent = new ControlChangeEvent(n, 7, old_vol, track[n]);
                    file->channel(n)->insertEvent(Pevent, tick_2 + 25);

                }
            }
        }


        file->protocol()->endAction();
    }

    emit endBar();
}

void Main_Thread::choppy_audio_effect() {

    if (!(Selection::instance()->selectedEvents().size() > 0 && file)) {
        emit endBar();
        return;
    }

    if (Selection::instance()->selectedEvents().size() > 0 && file) {

        QList<MidiEvent*> events;

        foreach (MidiEvent* e, Selection::instance()->selectedEvents()) {
            NoteOnEvent *on = dynamic_cast<NoteOnEvent*>(e);
            if(on) {
                int chan = e->channel();
                if(chan >= 0 && chan < 16 && chan != 9) {
                    events.append(e);
                }
            }
        }

        int max_counter = events.count();

        if(!max_counter) {
            emit endBar();
            return;
        }

        int modcount = max_counter / 100;
        if(modcount < 1) modcount = 1;

        file->protocol()->startNewAction("Choppy Audio Effect", 0);

        int tick_1 = 999999999, tick_2 = -1;

        int chan[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        MidiTrack* track[16];

        int counter = 0;
        foreach (MidiEvent* e, events) {
            NoteOnEvent *on = dynamic_cast<NoteOnEvent*>(e);

            if (on) {
                int note = on->note();
                MidiEvent* off =(MidiEvent* ) on->offEvent();
                on->setNote(note);

                if((counter % modcount) == 0) {
                    emit setBar(100 * counter /  max_counter);
                }

                counter++;

                if(on->midiTime() < tick_1) tick_1 = on->midiTime();

                int c = on->channel();
                if(c >= 0 && c < 16) {
                    chan[c] = 1;
                    track[c] = e->track();
                }

                if(off){

                    if(off->midiTime() > tick_2) tick_2 = off->midiTime();

                }
            }
        }

        if(tick_1 != 999999999 && tick_2 != -1) {

            for(int n = 0; n < 16; n++) {

                if(chan[n]) {

                    // remove channel volume events from this interval...
                    foreach (MidiEvent* event2, *(file->eventsBetween(tick_1-10, tick_2+10))) {
                        ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event2);
                        if (toRemove && event2->channel()==n && toRemove->control() == 7) {
                            file->channel(n)->removeEvent(toRemove);
                        }
                    }


                    // get last value for channel volume
                    int old_vol = 100;
                    foreach (MidiEvent* event2, *(file->eventsBetween(0, tick_2+10))) {
                        ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event2);
                        if (toRemove && event2->channel()==n && toRemove->control() == 7) {
                            old_vol = toRemove->value();

                        }
                    }


                    int clicks, s = 1;
                    for(clicks = tick_1; clicks < tick_2; clicks+= 25) {

                        ControlChangeEvent* Pevent = new ControlChangeEvent(n, 7, old_vol/2 + old_vol/2 * s, track[n]);
                        file->channel(n)->insertEvent(Pevent, clicks);
                        s^= 1;
                    }

                    // restore channel volume

                    if(s == 1) {
                        ControlChangeEvent* Pevent = new ControlChangeEvent(n, 7, old_vol, track[n]);
                        file->channel(n)->insertEvent(Pevent, tick_2);
                    }

                }
            }
        }


        file->protocol()->endAction();
    }

    emit endBar();
}

void Main_Thread::mute_audio_effect() {

    if (!(Selection::instance()->selectedEvents().size() > 0 && file)) {
        emit endBar();
        return;
    }

    if (Selection::instance()->selectedEvents().size() > 0 && file) {

        QList<MidiEvent*> events;

        foreach (MidiEvent* e, Selection::instance()->selectedEvents()) {
            NoteOnEvent *on = dynamic_cast<NoteOnEvent*>(e);
            if(on) {
                int chan = e->channel();
                if(chan >= 0 && chan < 16 && chan != 9) {
                    events.append(e);
                }
            }
        }

        int max_counter = events.count();

        if(!max_counter) {
            emit endBar();
            return;
        }

        int modcount = max_counter / 100;
        if(modcount < 1) modcount = 1;

        file->protocol()->startNewAction("Mute Audio Effect", 0);

        int counter = 0;
        foreach (MidiEvent* e, events) {
            NoteOnEvent *on = dynamic_cast<NoteOnEvent*>(e);

            if (on) {
                int note = on->note();
                MidiEvent* off =(MidiEvent* ) on->offEvent();
                on->setNote(note);

                if((counter % modcount) == 0) {
                    emit setBar(100 * counter /  max_counter);
                }

                counter++;

                if(off){
                    int tick_1 = on->midiTime();
                    int tick_2 = off->midiTime();
                    int mute_tick = 100;

                    if((tick_2 - tick_1) > mute_tick) {

                        // get last value for channel volume
                        int old_vol = 100;
                        foreach (MidiEvent* event2, *(file->eventsBetween(0, tick_1))) {
                            ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event2);
                            if (toRemove && event2->channel() == e->channel() && toRemove->control() == 7) {
                                if(toRemove->value() != 0) old_vol = toRemove->value();

                            }
                        }

                        // remove channel volume events from this interval...
                        foreach (MidiEvent* event2, *(file->eventsBetween(tick_1, tick_2))) {
                            ControlChangeEvent* toRemove = dynamic_cast<ControlChangeEvent*>(event2);
                            if (toRemove && event2->channel()== e->channel() && toRemove->control() == 7) {
                                file->channel(e->channel())->removeEvent(toRemove);
                            }
                        }

                        ControlChangeEvent* Pevent = new ControlChangeEvent(e->channel(), 7, old_vol, e->track());
                        file->channel(e->channel())->insertEvent(Pevent, tick_1);

                        ControlChangeEvent* Pevent1 = new ControlChangeEvent(e->channel(), 7, 0, e->track());
                        file->channel(e->channel())->insertEvent(Pevent1, tick_1 + mute_tick);

                        ControlChangeEvent* Pevent2 = new ControlChangeEvent(e->channel(), 7, old_vol, e->track());
                        file->channel(e->channel())->insertEvent(Pevent2, tick_2);
                    }

                }
            }
        }

        file->protocol()->endAction();
    }

    emit endBar();

}

void Main_Thread::conv_pattern_note() {

    if (!(Selection::instance()->selectedEvents().size() > 0 && file)) {
        emit endBar();
        return;
    }

    if (Selection::instance()->selectedEvents().size() > 0 && file) {

        QList<MidiEvent*> events;

        foreach (MidiEvent* e, Selection::instance()->selectedEvents()) {
            NoteOnEvent *on = dynamic_cast<NoteOnEvent*>(e);
            if(on) {
                int chan = e->channel();
                if(chan >= 0 && chan < 16 && chan != 9) {
                    events.append(e);
                }
            }
        }

        int max_counter = events.count();

        if(!max_counter) {
            emit endBar();
            return;
        }

        //qWarning("num. notes %i", max_counter);

        int modcount = max_counter / 100;
        if(modcount < 1) modcount = 1;

        file->protocol()->startNewAction("Pattern Note", 0);

        int counter = 0;
        foreach (MidiEvent* e, events) {
            NoteOnEvent *on = dynamic_cast<NoteOnEvent*>(e);

            if (on) {
                int note = on->note();
                MidiEvent* off =(MidiEvent* ) on->offEvent();
                on->setNote(note);

                if((counter % modcount) == 0) {
                    emit setBar(100 * counter /  max_counter);
                }

                counter++;

                if(off){

                    int clicks;

                    int lapse = 50;

                    if((off->midiTime() - on->midiTime()) > lapse)
                    for(clicks = on->midiTime(); clicks < off->midiTime(); clicks+= lapse) {
                        int clicks2 = off->midiTime() - clicks;
                        if(clicks2 > (lapse - 2)) clicks2 = lapse - 2;
                        clicks2+= clicks;

                        NoteOnEvent* on1 = file->channel(e->channel())->
                        insertNote(on->note(), clicks,
                                   clicks2,
                                   on->velocity(),
                                   e->track());
                        EventTool::selectEvent(on1, false, false);

                        // remove old note
                        EventTool::deselectEvent(on);
                        file->channel(e->channel())->removeEvent(on);
                    }

                }
            }
        }

        file->protocol()->endAction();
    }

    emit endBar();
}

void Main_Thread::NotesCorrection(int mode) {

    if(mode != 0 && !(Selection::instance()->selectedEvents().size() > 0 && file)) {
        emit endBar();
        return;
    }

    int max_counter = (mode == 0) ? (file->eventsBetween(0, file->endTick()))->count()
                              : Selection::instance()->selectedEvents().count();

    if(!max_counter) {
        emit endBar();
        return;
    }

    int counter = 0;

    // create lists for channels

    QList<MidiEvent *> notesListChan[16];

    // add notes (only notes) in separated channels

    max_counter = 0;

    foreach(MidiEvent* e, (mode == 0) ? *(file->eventsBetween(0, file->endTick())) : Selection::instance()->selectedEvents()) {
        NoteOnEvent *on = dynamic_cast<NoteOnEvent*>(e);
        if(on) {
            int chan = e->channel();
            if(chan >= 0 && chan < 16 && (((mode == 3) && chan != 9) || ((mode != 3) && chan == 9))) { // only notes chans
                max_counter++;
                notesListChan[chan].append(e);
            }
        }
    }

    if(!max_counter) {
        emit endBar();
        return;
    }

    //qWarning("num. notes %i", max_counter);

    if(mode == 3) file->protocol()->startNewAction("Strech Correction", 0);
    else if(mode == 2) file->protocol()->startNewAction("Long Notes Correction", 0);
    else if(mode != 0) file->protocol()->startNewAction("Overlapped Correction", 0);

    int modcount = max_counter / 100;
    if(modcount < 1) modcount = 1;

    // correction notes in channel

    for(int chan = 0; chan < 16; chan++) {
        foreach(MidiEvent* e, notesListChan[chan]) {
             NoteOnEvent *on = dynamic_cast<NoteOnEvent*>(e);
             if(mode != 0 && (counter % modcount) == 0) {
                 emit setBar(100 * counter /  max_counter);
             }

             counter++;

             if (on) {
                 int offtime = on->offEvent()->midiTime();
                 if(mode == 3) offtime = file->endTick();

                 foreach (MidiEvent* e2, file->channel(chan)->eventMap()->values()) {
                     NoteOnEvent *on2 = dynamic_cast<NoteOnEvent*>(e2);
                     if(on && on2 && on != on2 && e->channel() == e2->channel() &&
                             ((mode != 2 && mode != 3) ? on->note() == on2->note() : 1 == 1)) {

                         if(on2->midiTime() >= on->midiTime()
                         && on2->midiTime() <= offtime
                         && (on2->midiTime() - on->midiTime()) > 2) {

                             if(mode < 2 || ((mode == 2 || mode == 3) && on->note() == on2->note()) ||
                                     ((mode == 2 || mode == 3) && on->note() != on2->note() &&
                                      on2->midiTime() > (on->midiTime() + 10) &&
                                      offtime > on2->midiTime())) {

                                 on->offEvent()->setMidiTime(on2->midiTime() - 2);

                                 offtime = on->offEvent()->midiTime();
                             }

                         } else if(!Selection::instance()->selectedEvents().contains(on2) &&
                                   on->note() == on2->note() && // unselected note long overlapped case
                                   on->midiTime() > on2->midiTime() &&
                                   on->midiTime() <= on2->offEvent()->midiTime() &&
                                   (on->midiTime() - on2->midiTime()) > 2) {

                                 on2->offEvent()->setMidiTime(on->midiTime() - 2);
                         }
                     }
                 }
             }
        }
    }


    if(mode != 0) {
        file->protocol()->endAction();
    }

    emit endBar();
}

void Main_Thread::builddichord(int d3, int d5, int d7) {

    int dichord = 0;

    if(d3 == 666 && d7 == 666) dichord = 1;

    if(!(Selection::instance()->selectedEvents().size() > 0 && file)) {
        emit endBar();
        return;
    }

    if(Selection::instance()->selectedEvents().size() > 0 && file) {

        QList<MidiEvent*> events;

        foreach (MidiEvent* e, Selection::instance()->selectedEvents()) {
            NoteOnEvent *on = dynamic_cast<NoteOnEvent*>(e);
            if(on) {
                int chan = e->channel();
                if(chan >= 0 && chan < 16 && chan != 9) { // only notes chans

                    events.append(e);
                }
            }
        }

        file->protocol()->startNewAction("Build Chord Notes", 0);

        int max_counter = events.count();

        if(!max_counter) {
            emit endBar();
            return;
        }

        int counter = 0;

        int modcount = max_counter / 100;
        if(modcount < 1) modcount = 1;


        foreach(MidiEvent* e, events) {
            NoteOnEvent *on = dynamic_cast<NoteOnEvent*>(e);

            if((counter % modcount) == 0) {
                emit setBar(100 * counter /  max_counter);
            }

            counter++;

            if(on) {

                int note = on->note();
                MidiEvent* off =(MidiEvent* ) on->offEvent();
                on->setNote(note);

                if(off && d3 != 666){
                    int note1 = note + d3;
                    if(note1 < 0)  note1 += 12;
                    if(note > 127) note1 -= 12;

                    MidiEvent* a = file->channel(e->channel())->insertNote(note1,
                                        on->midiTime(), off->midiTime(), on->velocity() * pnote3 / 20, e->track());

                    EventTool::selectEvent(a, false);
                }

                if(off && d5 != 666){
                    int note1 = note + d5;
                    if(note1 < 0)  note1+= 12;
                    if(note > 127) note1-= 12;

                    MidiEvent* a = file->channel(e->channel())->insertNote(note1,
                                        on->midiTime(), off->midiTime(),  ((dichord) ? on->velocity() * pnote3 / 20 : on->velocity() * pnote5 / 20), e->track());

                    EventTool::selectEvent(a, false);
                }

                if(off && d7 != 666){
                    int note1 = note + d7;
                    if(note1 < 0)  note1+= 12;
                    if(note > 127) note1-= 12;

                    MidiEvent* a=file->channel(e->channel())->insertNote(note1,
                                    on->midiTime(), off->midiTime(), on->velocity() * pnote7 / 20, e->track());

                    EventTool::selectEvent(a, false);
                }
            }
        }

        NotesCorrection(0);

        file->protocol()->endAction();


    }
}

void Main_Thread::buildCMajorProg(int d3, int d5, int d7) {

    if (!(Selection::instance()->selectedEvents().size() > 0 && file)) return;

    if (Selection::instance()->selectedEvents().size() > 0 && file) {

        QList<MidiEvent*> events;

        foreach (MidiEvent* e, Selection::instance()->selectedEvents()) {
            NoteOnEvent *on = dynamic_cast<NoteOnEvent*>(e);
            if(on) {
                int chan = e->channel();
                if(chan >= 0 && chan < 16 && chan != 9) { // only notes chans

                    events.append(e);
                }
            }
        }

        file->protocol()->startNewAction("Build C Major", 0);

        foreach (MidiEvent* e, events) {
            NoteOnEvent *on = dynamic_cast<NoteOnEvent*>(e);

            if (on) {
                int note = on->note();

                MidiEvent* off =(MidiEvent* ) on->offEvent();

                int note1 = note / 12 * 12 + chord_noteM(note % 12, 1);

                if(note1 < 0) note1 += 12;
                if(note1 > 127) note1 -= 12;

                on->setNote(note1);

                if(off) {

                    note1 = note / 12 * 12 + chord_noteM(note % 12, 3);
                    note1+= d3;

                    if(note1 < 0) note1+= 12;
                    if(note1 > 127) note1-= 12;

                    MidiEvent* a = file->channel(e->channel())->insertNote(note1,
                                    on->midiTime(), off->midiTime(), on->velocity() * pnote3 / 20, e->track());

                    EventTool::selectEvent(a, false);

                    note1 = note / 12 * 12+chord_noteM(note % 12, 5);
                    note1+= d5;

                    if(note1 < 0) note1+= 12;
                    if(note1 > 127) note1-= 12;

                    a = file->channel(e->channel())->insertNote(note1,
                               on->midiTime(), off->midiTime(), on->velocity() * pnote5 / 20, e->track());
                    EventTool::selectEvent(a, false);

                    if(d7 != 666) {
                        note1= note / 12 * 12 + chord_noteM(note % 12, 7);
                        note1+= d7;

                        if(note1 < 0) note1+= 12;
                        if(note1 > 127) note1-= 12;

                        MidiEvent* a = file->channel(e->channel())->insertNote(note1,
                                        on->midiTime(), off->midiTime(), on->velocity() * pnote7 / 20, e->track());
                        EventTool::selectEvent(a, false);
                    }
                }
            }
        }

        NotesCorrection(0);

        file->protocol()->endAction();
    }
}

void Main_Thread::buildCMinorProg(int d3, int d5, int d7) {

    if (!(Selection::instance()->selectedEvents().size() > 0 && file)) return;

    if (Selection::instance()->selectedEvents().size() > 0 && file) {

        QList<MidiEvent*> events;

        foreach (MidiEvent* e, Selection::instance()->selectedEvents()) {
            NoteOnEvent *on = dynamic_cast<NoteOnEvent*>(e);
            if(on) {
                int chan = e->channel();
                if(chan >= 0 && chan < 16 && chan != 9) { // only notes chans

                    events.append(e);
                }
            }
        }

        file->protocol()->startNewAction("Build C Minor", 0);

        foreach (MidiEvent* e, events) {
            NoteOnEvent *on = dynamic_cast<NoteOnEvent*>(e);

            if (on) {
                int note = on->note();
                MidiEvent* off = (MidiEvent* ) on->offEvent();
                int note1= note / 12 * 12 + chord_notem((note % 12), 1);

                if(note1 < 0) note1+= 12;
                if(note1 > 127) note1-= 12;

                on->setNote(note1);

                if(off) {

                    note1 = note / 12 * 12 + chord_notem(((note % 12)), 3);
                    note1+= d3;

                    if(note1 < 0) note1 += 12;
                    if(note1 > 127) note1 -= 12;

                    MidiEvent* a = file->channel(e->channel())->insertNote(note1,
                                    on->midiTime(), off->midiTime(), on->velocity() * pnote3 / 20, e->track());

                    EventTool::selectEvent(a, false);

                    note1 = note / 12 * 12 + chord_notem(((note % 12)), 5);
                    note1+= d5;

                    if(note1 < 0) note1 += 12;
                    if(note1 > 127) note1-= 12;

                    a = file->channel(e->channel())->insertNote(note1,
                            on->midiTime(), off->midiTime(), on->velocity() * pnote5 / 20, e->track());

                    EventTool::selectEvent(a, false);

                    if(d7 != 666) {
                        note1= note / 12 * 12 +chord_notem(note % 12, 7); //7
                        note1+= d7;
                        if(note1 < 0) note1+= 12;
                        if(note1 > 127) note1-= 12;

                        MidiEvent* a=file->channel(e->channel())->insertNote(note1,
                                        on->midiTime(), off->midiTime(), on->velocity() * pnote7 / 20, e->track());
                        EventTool::selectEvent(a, false);
                    }
                }
            }
        }

        NotesCorrection(0);

        file->protocol()->endAction();
    }

}

/*************************************************************************/
/* Main_Thread section finish                                            */
/*************************************************************************/

#ifdef USE_FLUIDSYNTH
FluidDialog *fluid_control= NULL;

void MainWindow::FluidControl(){

    if (!MidiOutput::isConnected()) {
        QMessageBox::information(MW, "Fluid Synth Control", "Fluid Synth is not connected\n(Try MIDI/Settings)");
        return;
    }

    if(MidiOutput::outputPort()!=FLUID_SYNTH_NAME) {
        QMessageBox::information(MW, "Fluid Synth Control", "MIDI port is not Fluid Synth\n(Try MIDI/Settings)");
        return;
    }
    if(fluid_control) {

        delete fluid_control;
    }
    fluid_control= new FluidDialog(MW);
    if(!fluid_control) return;
    fluid_control->setModal(false);
    fluid_control->show();
    fluid_control->raise();
    fluid_control->activateWindow();
    QCoreApplication::processEvents();

}

void MainWindow::FluidSaveAsWav() {

    if (!file)
        return;
    if (MidiPlayer::isPlaying()) return;

    QString oldPath = file->path();

    QString path;
    if(oldPath.endsWith(".mid")) {
        path =  oldPath;
        path.remove(path.lastIndexOf(".mid"), 20);
        path+=".wav";
    } else if(oldPath.endsWith(".kar")) {
        path =  oldPath;
        path.remove(path.lastIndexOf(".kar"), 20);
        path+=".wav";
    } else path = startDirectory+"/default.wav";

    QString savePath = QFileDialog::getSaveFileName(this, "Save WAV file",
                                                    path, "WAV Files (*.wav)");

    if(savePath.isEmpty()) {
        return; // canceled
    }

    QFile *wav = new QFile(savePath);
    if(wav->open(QIODevice::WriteOnly | QIODevice::Truncate)) {

        fluid_output->MIDtoWAV(wav, mw_matrixWidget, file);
        delete wav;

    }

}

void MainWindow::FluidSaveAsMp3() {

    if (!file)
        return;

    if (MidiPlayer::isPlaying()) return;

    QString oldPath = file->path();

    QString path;
    if(oldPath.endsWith(".mid")) {
        path =  oldPath;
        path.remove(path.lastIndexOf(".mid"), 20);
        path+=".mp3";
    } else if(oldPath.endsWith(".kar")) {
        path =  oldPath;
        path.remove(path.lastIndexOf(".kar"), 20);
        path+=".mp3";
    } else path = startDirectory+"/default.mp3";

    QString savePath = QFileDialog::getSaveFileName(this, "Save MP3 file",
                                                    path, "MP3 Files (*.mp3)");

    if(savePath.isEmpty()) {
        return; // canceled
    }

    QString file_mp3 = QApplication::applicationDirPath() + "/encoders/lame.exe";

    if(!QFile::exists(file_mp3)) {
        QMessageBox::critical(this, "MP3 Saving", "lame.exe not found!");
        return;
    }

    QFile *wav = new QFile(savePath + ".wav");

    if(wav->open(QIODevice::WriteOnly | QIODevice::Truncate)) {


        //////////
        QString oldPath = file->path();

        QString path2;
        if(oldPath.endsWith(".mid")) {
            path2 =  oldPath;
            path2.remove(path2.lastIndexOf(".mid"), 20);
            path2+=".mic.wav";
        } else if(oldPath.endsWith(".kar")) {
            path2 =  oldPath;
            path2.remove(path2.lastIndexOf(".kar"), 20);
            path2+=".mic.wav";
        } else path2 = startDirectory+"/default.mic.wav";

        fluid_output->MIDtoWAV(wav, mw_matrixWidget, file);

        bool exist = wav->exists();
        delete wav;

        if(exist) {

            QFile::remove(savePath);

            QProcess *process = new QProcess(this);

            QStringList args;
            args.append("--disptime");
            args.append("5");
            args.append("-b");
            int index = fluid_output->fluid_settings->value("mp3_bitrate").toInt();
            int tab_bitrate[] = {128, 160, 192, 224, 256, 320};
            args.append(QString::number(tab_bitrate[index]));

            if(fluid_output->fluid_settings->value("mp3_vbr").toBool())
                args.append("--vbr-new");
            else
                args.append("--cbr");

            if(fluid_output->fluid_settings->value("mp3_hq").toBool()) {
                args.append("-q");
                args.append("0");
            }

            args.append("-m");
            if(fluid_output->fluid_settings->value("mp3_mode").toBool())
                args.append("s");
            else
                args.append("j");


            QString title = fluid_output->fluid_settings->value("mp3_title").toString();

            if(title.isEmpty()) {

                // get name from filename

                title = savePath.mid(savePath.lastIndexOf("/") + 1);
                title = title.mid(title.lastIndexOf("\\") + 1);
                if(title.endsWith(".mp3")) {

                    title.remove(title.lastIndexOf(".mp3"), 20);

                }
            }

            if(fluid_output->fluid_settings->value("mp3_id3").toBool()) {
                args.append("--tt");
                args.append(title);
                args.append("--ta");
                args.append(fluid_output->fluid_settings->value("mp3_artist").toString());
                args.append("--tl");
                args.append(fluid_output->fluid_settings->value("mp3_album").toString());
                args.append("--tg");
                args.append(fluid_output->fluid_settings->value("mp3_genre").toString());
                args.append("--ty");
                args.append(QString::number(fluid_output->fluid_settings->value("mp3_year").toInt()));
                args.append("--tn");
                args.append(QString::number(fluid_output->fluid_settings->value("mp3_track").toInt()));
            }

            args.append(savePath + ".wav");
            args.append(savePath);

            int r = process->execute(file_mp3, args);

            QFile::remove(savePath + ".wav");

            if(r) QMessageBox::critical(this, "MP3 Saving", "Error!");
            else QMessageBox::information(this, "MP3 Saving", "Succeeded");

            delete process;
        }
    }

}

void MainWindow::FluidSaveAsFlac() {

    if (!file)
        return;

    if (MidiPlayer::isPlaying()) return;

    QString oldPath = file->path();

    QString path;
    if(oldPath.endsWith(".mid")) {
        path =  oldPath;
        path.remove(path.lastIndexOf(".mid"), 20);
        path+=".flac";
    } else if(oldPath.endsWith(".kar")) {
        path =  oldPath;
        path.remove(path.lastIndexOf(".kar"), 20);
        path+=".flac";
    } else path = startDirectory+"/default.flac";

    QString savePath = QFileDialog::getSaveFileName(this, "Save FLAC file",
                                                    path, "FLAC Files (*.flac)");

    if(savePath.isEmpty()) {
        return; // canceled
    }

    QString file_flac = QApplication::applicationDirPath() + "/encoders/flac.exe";

    if(!QFile::exists(file_flac)) {
        QMessageBox::critical(this, "FLAC Saving", "flac.exe not found!");
        return;
    }

    QFile *wav = new QFile(savePath + ".wav");
    if(wav->open(QIODevice::WriteOnly | QIODevice::Truncate)) {

        // FLAC is not float32 compatible: use int16
        int isf = fluid_output->wav_is_float;
        fluid_output->wav_is_float = 0;
        fluid_output->fluid_settings->setValue("Wav to Float", fluid_output->wav_is_float);

        fluid_output->MIDtoWAV(wav, mw_matrixWidget, file);

        bool exist = wav->exists();
        delete wav;

        // restore wav_is_float
        fluid_output->wav_is_float = isf;
        fluid_output->fluid_settings->setValue("Wav to Float", fluid_output->wav_is_float);

        if(exist) {
            QFile::remove(savePath);

            QProcess *process = new QProcess(this);

            QStringList args;

            args.append("-f");

            QString title = fluid_output->fluid_settings->value("mp3_title").toString();

            if(title.isEmpty()) {

                // get name from filename

                title = savePath.mid(savePath.lastIndexOf("/") + 1);
                title = title.mid(title.lastIndexOf("\\") + 1);
                if(title.endsWith(".flac")) {

                    title.remove(title.lastIndexOf(".flac"), 20);

                }
            }

            if(fluid_output->fluid_settings->value("mp3_id3").toBool()) {
                args.append("--tag=TITLE=""" + title + """");
                args.append("--tag=ARTIST=""" + fluid_output->fluid_settings->value("mp3_artist").toString() + """");
                args.append("--tag=ALBUM=""" + fluid_output->fluid_settings->value("mp3_album").toString() + """");
                args.append("--tag=GENRE=""" + fluid_output->fluid_settings->value("mp3_genre").toString() + """");
                args.append("--tag=YEAR=""" + QString::number(fluid_output->fluid_settings->value("mp3_year").toInt()) + """");
                args.append("--tag=TRACKNUMBER=""" + QString::number(fluid_output->fluid_settings->value("mp3_track").toInt()) + """");
            }

            args.append("-o");
            args.append(savePath);
            args.append(savePath + ".wav");

            int r = process->execute(file_flac, args);

            QFile::remove(savePath + ".wav");

            if(r) QMessageBox::critical(this, "FLAC Saving", "Error!");
            else QMessageBox::information(this, "FLAC Saving", "Succeeded");

            delete process;
        }
    }

}
#endif

void MainWindow::ImportSF2Names() {


    unsigned char name[0x26];
    unsigned short preset, bank;
    unsigned dat, block;
    long long size;
    long long pos;

    QString sf2Dir = QDir::rootPath();

    if (_settings->value("sf2_path").toString() != "") {
        sf2Dir = _settings->value("sf2_path").toString();
    } else {
        _settings->setValue("sf2_path", sf2Dir);
    }

    QString newPath = QFileDialog::getOpenFileName(this, "Import names from  SF2/SF3 file",
        sf2Dir, "SF2/SF3 Files(*.sf2 *.sf3)");
    if(newPath.isEmpty()) { // canceled
        if(QMessageBox::information(this, "Import names from  SF2/SF3 file aborted",
            "Restore Midieditor original names?",
            QMessageBox::Yes | QMessageBox::No)
                != QMessageBox::Yes) return;

        itHaveInstrumentList = 0;
        itHaveDrumList = 0;

        int n, m;
        for(n = 0; n < 129; n++)
            for(m = 0; m < 128; m++)
                InstrumentList[n].name[m]="undefined";
        channelWidget->update();
        _settings->setValue("instrumentList", "");

        return;
    }

    sf2Dir = newPath;

     QFile* f = new QFile(sf2Dir);


    if (!f->open(QIODevice::ReadOnly)) {
        QMessageBox::information(NULL, "Error openning file", sf2Dir);
        return; // file not found
    }

    _settings->setValue("sf2_path", sf2Dir.toUtf8());


    itHaveInstrumentList = 0;
    itHaveDrumList = 0;
    int r;

    int n, m;
    for(n = 0; n < 129; n++)
        for(m = 0; m < 128; m++)
            InstrumentList[n].name[m]="undefined";

    name[0] = 0;
    r = f->read((char *) name, 4);
    if(r != 4 && name[0] != 'R' && name[1] != 'I' && name[2] != 'F' && name[3] != 'F') {
        f->close();
        QMessageBox::information(NULL, "Error!: no RIFF code", sf2Dir);
        return; // no RIFF code
    }

    dat = 0;
    r = f->read((char  *) &dat, 4);
    if(r != 4 || dat == 0) {
        QMessageBox::information(NULL, "Error reading size", sf2Dir);
        f->close(); return; // error reading size
    }

    size = (long long) dat;

    name[0] = 0;
    r = f->read((char *) name, 4);
    size-= 4;

    if(r != 4
       && name[0] != 's' && name[1] != 'f' && name[2] != 'b' && name[3] != 'k') {
        f->close();
        QMessageBox::information(NULL, "Error!: sound font names chunk not found", sf2Dir);
        return; // no sound font name
    }

    int flag = 0;

    pos = 12;
    f->seek(pos);

    name[4]=0;

    while(size > 0) {
        if(flag == 0) { // reading LIST

            name[0] = 0;
            r = f->read((char *) name, 4);
            size-= 4;

            if(r != 4 ||
               name[0] != 'L' || name[1] != 'I' || name[2] != 'S' || name[3] != 'T') {
                f->close();
                goto save_instrument_list; // end of LIST
            }

            block = 0; size-= 4;
            r = f->read((char  *) &block, 4);

            if(r != 4 || block == 0) {
                f->close();
                goto save_instrument_list; // end of LIST
            }

            pos+= 8;
            name[0] = 0;

            r = f->read((char *) name, 4);

            if(r != 4 ||
                name[0] != 'p' || name[1] != 'd' || name[2] != 't' || name[3] != 'a') {

                if(r != 4) { // error reading
                    f->close();
                    QMessageBox::information(NULL, "Error!: reading file", sf2Dir);
                    return;
                }

                pos += block;
                f->seek(pos);
                size-= block;
                continue; // to the next LIST

            } else { // to read pdta section
                block -= 4;
                size = block;
                pos += 4;
                f->seek(pos);
                flag = 1;
            }
        }

        if(flag == 1) { // pdta

            name[0]=0;

            r = f->read((char *) name, 4);

            if(r != 4) { // error reading
                f->close();
                QMessageBox::information(NULL, "Error!: reading file", sf2Dir);
                return;
            }

            r=f->read((char  *) &block, 4);

            if(r != 4) { // error reading
                f->close();
                QMessageBox::information(NULL, "Error!: reading file", sf2Dir);
                return;
            }

            if(name[0]!='p' || name[1]!='h' || name[2]!='d' || name[3]!='r') {
                pos += 8 + block; // skip block
                f->seek(pos);
                size-= block+8;
                continue;
            } else { // to read instruments
                size=block;
                pos += 8;
                f->seek(pos);
                flag = 2;
            }

        }

        if(flag == 2) {
            r = f->read((char *) name, 0x26); size-=0x26;
            pos += 0x26;
            if(r != 0x26) { // error reading
                f->close();
                QMessageBox::information(NULL, "Error!: reading file", sf2Dir);
                return;
            }

            preset = name[0x14] | (name[0x15]<<8);
            bank = name[0x16] | (name[0x17]<<8);
            name[0x14] = 0; // \0 add

            if(!strcmp((char *) name, "EOP")) { // END OF LIST
                f->close();
                goto save_instrument_list;
            }

            if(bank > 128 || preset > 127) continue; //ignore it
            InstrumentList[bank].name[preset]= QByteArray((char *) name);

            itHaveInstrumentList = 1;
            if(bank == 128) itHaveDrumList = 1;
            //if(bank == 128)
            //    MSG_OUT("n %i:%i %s\n", bank, preset, name);

        }

    }

    f->close();

save_instrument_list:


    if(newPath.endsWith(".sf2")) {

        newPath.remove(newPath.lastIndexOf(".sf2"), 20);
        newPath += ".lsf";

    } else if(newPath.endsWith(".sf3")) {
        newPath.remove(newPath.lastIndexOf(".sf3"), 20);
        newPath += ".lsf";
    } else newPath += ".lsf";

    if(newPath.isEmpty()) {
        QMessageBox::information(this, "Save List SF2 names", "save aborted!");
        return;
    }

    f = new QFile(newPath);

    if (!f->open(QIODevice::WriteOnly | QIODevice :: Text)) {
        QMessageBox::information(this, "Error Creating", newPath);
        return;
    }

    for(int n=0; n< 129; n++)
        for(int m=0; m<128; m++) {
            if(InstrumentList[n].name[m] != "undefined") {
                char head[32]="";
                sprintf(head,"%3.3i:%3.3i = ", n, m);
                QByteArray line;
                line.append(head);
                line.append(InstrumentList[n].name[m]);
                line.append('\n');
                if(f->write(line) < 0) {
                    QMessageBox::information(this, "Error Writting", newPath);
                }
            }
        }
    f->close();

    _settings->setValue("instrumentList", newPath);

    channelWidget->update();

    QMessageBox::information(this, "Import SF2 names finished and saved at:", newPath);

}


void MainWindow::PianoPlay()
{
    MyInstrument* d = new MyInstrument(MW, mw_matrixWidget, file, MY_PIANO);

    d->setModal(true);
    d->exec();

    delete d;

}

void MainWindow::DrumPlay()
{
    MyInstrument* d = new MyInstrument(MW, mw_matrixWidget, file, MY_DRUM);

    d->setModal(true);
    d->exec();

    delete d;

}


void MainWindow::DrumRhythmBox()
{
    if(!Play_Thread::rhythmThread) { // create thread to play from rhythm box
        Play_Thread::rhythmThread = new Play_Thread();
        if(!Play_Thread::rhythmThread) return;
        Play_Thread::rhythmThread->start(QThread::HighPriority); // TimeCriticalPriority
    }

    MyInstrument* d = new MyInstrument(MW, mw_matrixWidget, file, MY_RHYTHMBOX);

    d->setModal(true);
    d->exec();

    delete d;

}

void MainWindow::midi_text_edit() {

    TextEventEdit* d = new TextEventEdit(file, 16, this, TEXTEVENT_NEW_TEXT);

    d->exec();
    delete d;
}

void MainWindow::midi_lyrik_edit() {

    TextEventEdit* d = new TextEventEdit(file, 16, this, TEXTEVENT_NEW_LYRIK);

    d->exec();
    delete d;
}

void MainWindow::midi_track_name_edit() {

    TextEventEdit* d = new TextEventEdit(file, 16, this, TEXTEVENT_NEW_TRACK_NAME);

    d->exec();
    delete d;
}
void MainWindow::midi_marker_edit() {

    TextEventEdit* d = new TextEventEdit(file, 16, this, TEXTEVENT_NEW_MARKER);

    d->exec();
    delete d;
}

void MainWindow::finger_pattern() {

    FingerPatternDialog* d = new FingerPatternDialog(this, _settings);

    d->exec();
    delete d;
}

#ifdef USE_FLUIDSYNTH

QProcess *VSTprocess = NULL;

#include <QSharedMemory>

QSharedMemory *sharedVSText = NULL;
QSharedMemory *sharedAudioBuffer = NULL;

QSystemSemaphore *sys_sema_in = NULL;
QSystemSemaphore *sys_sema_out = NULL;
QSystemSemaphore *sys_sema_inW = NULL;

QMutex * externalMux = NULL;

VST_EXT *vst_ext = NULL;

void MainWindow::remote_VST_exit() {

    int ret = 0;

    if(vst_ext) {
        vst_ext->terminate();
        vst_ext->wait(1000);
    }

    DELETE(vst_ext);

    if(externalMux) {
        externalMux->lock();
        externalMux->unlock();
    }

    DELETE(externalMux) // first this (disable fluidsynth VST_proc::VST_external_mix ())
    VST_proc::VST_external_send_message(0, 0xAD105, 0, 0); // send 'ADIOS'
    DELETE(sys_sema_in) // and now delete this
    DELETE(sys_sema_out)

    DELETE(sys_sema_inW)

    if(sharedVSText)
        sharedVSText->detach();

    if(sharedAudioBuffer)
        sharedAudioBuffer->detach();

    DELETE(sharedVSText)
    DELETE(sharedAudioBuffer)

    if(VSTprocess) {
        VSTprocess->terminate();
        if(VSTprocess->waitForFinished(2000)) ret = 1;
    }

    DELETE(VSTprocess)

    qDebug("remote_VST_exit() %i", ret);

}

void MainWindow::remote_VST() {

    if(sys_sema_in) return;

    QString key = QString::number(this->centralWidget()->winId());

    sharedVSText = new QSharedMemory("remoteVST_mess" + key);

    if(sharedVSText && sharedVSText->attach()) {
        // previous remoteVST working!
        int * dat = ((int *) sharedVSText->data());
        sys_sema_inW = new QSystemSemaphore("VST_MAIN_SemaIn" + key);
        if(sys_sema_inW) { // Ask if other MidiEditor is working
            dat[0] = 0x1234;
            dat[1] = 0;
            dat[0x39] = 0;
            sys_sema_inW->release();
            QThread::msleep(200); // wait a time...
            if(dat[0x39] == (int) 0xCACABACA) {
                dat[0x39] = 0;
                // other Midieditor is working here!
                QMessageBox::information(this, "remoteVST", "You cannot use remoteVST for 32 bits plugings because other Midieditor instance is using it!");
                DELETE(sharedVSText)
                DELETE(sys_sema_inW)
                return;
            }

            else sys_sema_inW->acquire();

            DELETE(sys_sema_inW)
        }

        QSystemSemaphore *sys_sema_in2 = new QSystemSemaphore("VST_IN_SemaIn" + key);
        if(sys_sema_in2) {
            int ret = 0;
            dat[0x1000] = 0;
            dat[0x40] = (int) 0xCACABACA; // send an exit message to remoteVST
            sys_sema_in2->release();

            QThread::msleep(50); // wait a short time...

            if(dat[0x1000] == (int) 0xCACABACA) {
                sharedVSText = NULL;

                // exiting from remoteVST
                QThread::msleep(2000); // wait a time...
                ret = 1;
                qDebug("remoteVST exiting...");
            }

            if(ret == 0) {
                QMessageBox::information(this, "remoteVST", "You cannot use remoteVST for 32 bits plugings because other remoteVST process is hang!");
                return;
            }
        }


    }
    DELETE(sharedVSText)

    VSTprocess = new QProcess(this);
    if(!VSTprocess) exit(-66);
    QStringList args;
    args.append(QString::number(this->centralWidget()->winId()));

    VSTprocess->start(QApplication::applicationDirPath() + "/encoders/remoteVST.exe", args);

    if(!VSTprocess->waitForStarted()) {
        qWarning("remoteVST Process cannot start");
        DELETE(VSTprocess);
        //exit(-1);
    }

    if(VSTprocess) {
        int error = VSTprocess->error();
        if(error == QProcess::FailedToStart || error == QProcess::Crashed) {
            qWarning("remoteVST Process cannot start");
            DELETE(VSTprocess);
            //exit(-1);
        }
    }

    if(VSTprocess && VSTprocess->exitStatus()) {
        qWarning("remoteVST Process cannot start");
        DELETE(VSTprocess);
        //exit(-1);
    }


    QMutex *Mux = new QMutex(QMutex::NonRecursive);
    QSystemSemaphore * _sys_sema_in = new QSystemSemaphore("VST_IN_SemaIn" + key, 0);
    sys_sema_out = new QSystemSemaphore("VST_IN_SemaOut" + key, 0);
    sys_sema_inW = new QSystemSemaphore("VST_MAIN_SemaIn" + key, 0);

    if(!VSTprocess)
        goto exit_with_error;

    if(!_sys_sema_in || !sys_sema_out || !sys_sema_inW) goto exit_with_error;
    //exit(-67);

    sys_sema_out->acquire(); // wait to external process

    sharedVSText = new QSharedMemory("remoteVST_mess" + key);

    if(!sharedVSText || !sharedVSText->attach()) {
        exit(-16);
    }

    sharedAudioBuffer = new QSharedMemory("remoteVST_audio" + key);

    if(!sharedAudioBuffer || !sharedAudioBuffer->attach()) {
        exit(-17);
    }

    vst_ext = new VST_EXT(this);
    vst_ext->start(QThread::NormalPriority);

    sys_sema_in = _sys_sema_in;
    externalMux = Mux;

    return;

exit_with_error:

    DELETE(Mux)
    DELETE(_sys_sema_in)
    DELETE(sys_sema_in)
    DELETE(sys_sema_out)
    DELETE(sys_sema_inW)
    DELETE(sharedVSText)
    DELETE(sharedAudioBuffer)

    QMessageBox::critical(this, "remoteVST", "fail starting remoteVST for 32 bits");

    return;
}



void MainWindow::sysExChecker(MidiFile* mf) {
    bool proto = false;

    // preset datas
    float synth_gain;
    int synth_chanvolume[16];
    int audio_changain[16];
    int audio_chanbalance[16];

    bool filter_dist_on[16];
    float filter_dist_gain[16];

    bool filter_locut_on[16];
    float filter_locut_freq[16];
    float filter_locut_gain[16];
    float filter_locut_res[16];

    bool filter_hicut_on[16];
    float filter_hicut_freq[16];
    float filter_hicut_gain[16];
    float filter_hicut_res[16];

    float level_WaveModulator[16];
    float freq_WaveModulator[16];

    // deletes old sysEx methods
    foreach (MidiEvent* event, mf->channel(16)->eventMap()->values()) {
        SysExEvent* sys = dynamic_cast<SysExEvent*>(event);
        if(sys) {

            synth_gain = 1.0;

            // initialize values
            for(int n = 0; n < 16; n++) {
                synth_chanvolume[n] = 127; // initialize expresion
                audio_chanbalance[n] = 0;
                audio_changain[n] = 0;

                filter_dist_on[n] = false;
                filter_dist_gain[n] = 0.0;

                filter_locut_on[n] = false;
                filter_locut_freq[n] = 500;
                filter_locut_gain[n] = 0.0;
                filter_locut_res[n] = 0.0;

                filter_hicut_on[n] = false;
                filter_hicut_freq[n] = 2500;
                filter_hicut_gain[n] = 0.0;
                filter_hicut_res[n] = 0.0;

                level_WaveModulator[n] = 0.0;
                freq_WaveModulator[n] = 0.0;

            }

            QByteArray c = sys->data();

            int current_tick = sys->midiTime();

            // delete 'R'
            if(c[0] == (char) 0 && c[1] == (char) 0x66 && c[2] == (char) 0x66 &&  c[3]=='R') {
                // Begin a new ProtocolAction
                if(!proto) {
                    //mf->protocol()->startNewAction("SYSEx old events replaced");
                    mf->protocol()->addEmptyAction("SYSEx old events replaced");

                    proto = true;
                }

                char id2[4]= {0x0, 0x66, 0x66, 0x70};
                int BOOL;

                int dtick= mf->tick(150);


                foreach (MidiEvent* event,
                         *(mf->eventsBetween(current_tick-dtick, current_tick+dtick))) {

                    SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

                    if(sys) {
                        QByteArray b = sys->data();

                        if(b[1] == id2[1] && b[2] == id2[2] && b[3] == 'R'){

                            mf->channel(16)->removeEvent(event);

                            int entries = 13 * 1;
                            char id[4];

                            int flag = 1;

                            int n = b[3] & 0xf;

                            if(b[3] == 'R') {
                                flag = 2;
                                n = b[0] & 0xf;
                            }

                            QDataStream qd(&b,
                                           QIODevice::ReadOnly);
                            qd.startTransaction();

                            qd.readRawData((char *) id, 4);

                            if(flag == 1 && (id[0]!=id2[0] || id[1]!=id2[1] || id[2]!=id2[2] || (id[3] & 0xF0) != 0x70)) {
                                continue;
                            }

                            if(flag == 2 && (id[1]!=id2[1] || id[2]!=id2[2] || id[3] != 'R')) {
                                continue;
                            }

                            if(decode_sys_format(qd, (void *) &entries)<0) {

                                continue;
                            }

                            if(flag == 2 && n == 0 && entries != (13 + 1)) {
                                //QMessageBox::information(this, "Ehhh!", "wentries differents");
                                continue;
                            }

                            if((n != 0 || flag == 1) && entries != 13) {
                                //QMessageBox::information(this, "Ehhh!", "entries differents");
                                continue;
                            }

                            if((flag == 2 && n == 0) || flag == 1)
                                decode_sys_format(qd, (void *) &synth_gain);

                            decode_sys_format(qd, (void *) &synth_chanvolume[n]);
                            decode_sys_format(qd, (void *) &audio_changain[n]);
                            decode_sys_format(qd, (void *) &audio_chanbalance[n]);

                            decode_sys_format(qd, (void *) &BOOL);
                            filter_dist_on[n]= (BOOL) ? true : false;
                            decode_sys_format(qd, (void *) &filter_dist_gain[n]);

                            decode_sys_format(qd, (void *) &BOOL);
                            filter_locut_on[n]= (BOOL) ? true : false;
                            decode_sys_format(qd, (void *) &filter_locut_freq[n]);
                            decode_sys_format(qd, (void *) &filter_locut_gain[n]);
                            decode_sys_format(qd, (void *) &filter_locut_res[n]);

                            decode_sys_format(qd, (void *) &BOOL);
                            filter_hicut_on[n]= (BOOL) ? true : false;
                            decode_sys_format(qd, (void *) &filter_hicut_freq[n]);
                            decode_sys_format(qd, (void *) &filter_hicut_gain[n]);
                            decode_sys_format(qd, (void *) &filter_hicut_res[n]);


                        }

                    } // sys
                }


                QByteArray b;

                char idM[4]= {0x10, 0x66, 0x66, 'R'};

                QDataStream qd(&b, QIODevice::WriteOnly); // save header
                // write the header
                if(qd.writeRawData((const char *) idM, 4)<0) {
                    b.clear();
                    break;
                }

                int entries = 15 * 16 + 1;

                encode_sys_format(qd, (void *) &entries);

                for(int n = 0; n < 16; n++) {

                    if(n == 0)
                        encode_sys_format(qd, (void *) &synth_gain);

                    encode_sys_format(qd, (void *) &synth_chanvolume[n]);
                    encode_sys_format(qd, (void *) &audio_changain[n]);
                    encode_sys_format(qd, (void *) &audio_chanbalance[n]);

                    if(filter_dist_on[n]) BOOL=1; else BOOL=0;
                    encode_sys_format(qd, (void *) &BOOL);
                    encode_sys_format(qd, (void *) &filter_dist_gain[n]);

                    if(filter_locut_on[n]) BOOL=1; else BOOL=0;
                    encode_sys_format(qd, (void *) &BOOL);
                    encode_sys_format(qd, (void *) &filter_locut_freq[n]);
                    encode_sys_format(qd, (void *) &filter_locut_gain[n]);
                    encode_sys_format(qd, (void *) &filter_locut_res[n]);

                    if(filter_hicut_on[n]) BOOL=1; else BOOL=0;
                    encode_sys_format(qd, (void *) &BOOL);
                    encode_sys_format(qd, (void *) &filter_hicut_freq[n]);
                    encode_sys_format(qd, (void *) &filter_hicut_gain[n]);
                    encode_sys_format(qd, (void *) &filter_hicut_res[n]);

                    encode_sys_format(qd, (void *) &level_WaveModulator[n]);
                    encode_sys_format(qd, (void *) &freq_WaveModulator[n]);
                }

                // delete events


                foreach (MidiEvent* event,
                         *(mf->eventsBetween(current_tick-dtick, current_tick+dtick))) {

                    SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

                    if(sys) {
                        c = sys->data();

                        if(c[1]==idM[1] && c[2]==idM[2] && c[3]=='R'){
                            mf->channel(16)->removeEvent(sys);
                        }

                    }

                }


                SysExEvent *sys_event = new SysExEvent(16, b, mf->track(0));
                mf->channel(16)->insertEvent(sys_event, current_tick);

                ///)

            } else

            // delete 'P'
            if(c[1] == (char) 0x66 && c[2] == (char) 0x66 &&  c[3]=='P') {
                // Begin a new ProtocolAction
                if(!proto) {
                    //mf->protocol()->startNewAction("SYSEx old events replaced");
                    mf->protocol()->addEmptyAction("SYSEx old events replaced");

                    proto = true;
                }

                mf->channel(16)->removeEvent(event);

                //////////////////////////
                char id2[4] = {0x0, 0x66, 0x66, 'P'}; // global mixer

                // transform
                if(1) {

                    int entries = 13 * 16 + 1;
                    char id[4];
                    int BOOL;

                    QDataStream qd(&c,
                                   QIODevice::ReadOnly);
                    qd.startTransaction();

                    qd.readRawData((char *) id, 4);

                    if(id[0] != id2[0] || id[1] != id2[1] || id[2] != id2[2] || id[3] != 'P') {
                        continue;
                    }


                    if(decode_sys_format(qd, (void *) &entries) < 0) {

                        continue;
                    }

                    if(entries != 13 * 16 + 1) {
                       continue;
                    }


                    // decode values

                    decode_sys_format(qd, (void *) &synth_gain);

                    for(int n = 0; n < 16; n++) {

                        decode_sys_format(qd, (void *) &synth_chanvolume[n]);
                        decode_sys_format(qd, (void *) &audio_changain[n]);
                        decode_sys_format(qd, (void *) &audio_chanbalance[n]);

                        decode_sys_format(qd, (void *) &BOOL);
                        filter_dist_on[n]= (BOOL) ? true : false;
                        decode_sys_format(qd, (void *) &filter_dist_gain[n]);

                        decode_sys_format(qd, (void *) &BOOL);
                        filter_locut_on[n]= (BOOL) ? true : false;
                        decode_sys_format(qd, (void *) &filter_locut_freq[n]);
                        decode_sys_format(qd, (void *) &filter_locut_gain[n]);
                        decode_sys_format(qd, (void *) &filter_locut_res[n]);

                        decode_sys_format(qd, (void *) &BOOL);
                        filter_hicut_on[n]= (BOOL) ? true : false;
                        decode_sys_format(qd, (void *) &filter_hicut_freq[n]);
                        decode_sys_format(qd, (void *) &filter_hicut_gain[n]);
                        decode_sys_format(qd, (void *) &filter_hicut_res[n]);

                    }

                    // store presets new 'R'

                    {

                        QByteArray b;

                        entries = 15 * 16 + 1;


                        char id2[4]= {0x10, 0x66, 0x66, 'R'};
                        QDataStream qd(&b,
                                       QIODevice::WriteOnly); // save header


                        // write the header
                        if(qd.writeRawData((const char *) id2, 4)<0) {
                            b.clear();
                            break;
                        }

                        encode_sys_format(qd, (void *) &entries);

                        for(int n = 0; n < 16; n++) {

                            if(n == 0)
                                encode_sys_format(qd, (void *) &synth_gain);

                            encode_sys_format(qd, (void *) &synth_chanvolume[n]);
                            encode_sys_format(qd, (void *) &audio_changain[n]);
                            encode_sys_format(qd, (void *) &audio_chanbalance[n]);

                            if(filter_dist_on[n]) BOOL=1; else BOOL=0;
                            encode_sys_format(qd, (void *) &BOOL);
                            encode_sys_format(qd, (void *) &filter_dist_gain[n]);

                            if(filter_locut_on[n]) BOOL=1; else BOOL=0;
                            encode_sys_format(qd, (void *) &BOOL);
                            encode_sys_format(qd, (void *) &filter_locut_freq[n]);
                            encode_sys_format(qd, (void *) &filter_locut_gain[n]);
                            encode_sys_format(qd, (void *) &filter_locut_res[n]);

                            if(filter_hicut_on[n]) BOOL=1; else BOOL=0;
                            encode_sys_format(qd, (void *) &BOOL);
                            encode_sys_format(qd, (void *) &filter_hicut_freq[n]);
                            encode_sys_format(qd, (void *) &filter_hicut_gain[n]);
                            encode_sys_format(qd, (void *) &filter_hicut_res[n]);

                            encode_sys_format(qd, (void *) &level_WaveModulator[n]);
                            encode_sys_format(qd, (void *) &freq_WaveModulator[n]);

                        }

                        // delete events

                        int dtick= mf->tick(150);


                        foreach (MidiEvent* event,
                                 *(mf->eventsBetween(current_tick-dtick, current_tick+dtick))) {

                            SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

                            if(sys) {
                                c = sys->data();
                                // delete for individual chans
                                if(c[0]==id[0] && c[1]==id[1] && c[2]==id[2] && (c[3] & 0xF0)==(char) 0x70){
                                    mf->channel(event->channel())->removeEvent(sys);
                                }
                                if(c[0]==id[0] && c[1]==id[1] && c[2]==id[2] && c[3]=='P'){
                                    mf->channel(16)->removeEvent(sys);
                                }
                                if(c[1]==id[1] && c[2]==id[2] && c[3]=='R'){
                                    mf->channel(16)->removeEvent(sys);
                                }

                            }

                        }


                        SysExEvent *sys_event = new SysExEvent(16, b, mf->track(0));
                        mf->channel(16)->insertEvent(sys_event, current_tick);
                    }

                }
            }
        }
    }

    if(proto) {
        mf->protocol()->endAction();
    }

}

#endif

