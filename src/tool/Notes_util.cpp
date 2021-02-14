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

#include "../protocol/Protocol.h"

#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/NoteOnEvent.h"
#include "../MidiEvent/OffEvent.h"
#include "../MidiEvent/TextEvent.h"
#include "../MidiEvent/PitchBendEvent.h"

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

void MainWindow::longNotesCorrection() {
    NotesCorrection(2);
}

void MainWindow::overlappedNotesCorrection() {
    NotesCorrection(1);
}

void MainWindow::NotesCorrection(int mode) {

    if(mode != 0 && !(Selection::instance()->selectedEvents().size() > 0 && file)) return;

    if(mode != 0) file->protocol()->startNewAction("Overlapped Correction", 0);

    foreach(MidiEvent* e, (mode == 0) ? *(file->eventsBetween(0, file->endTick())) : Selection::instance()->selectedEvents()) {
        NoteOnEvent *on = dynamic_cast<NoteOnEvent*>(e);

        if (on) {
            foreach (MidiEvent* e2, *(file->eventsBetween(0, file->endTick()))) {
                NoteOnEvent *on2 = dynamic_cast<NoteOnEvent*>(e2);
                if(on && on2 && on != on2 && e->channel() == e2->channel() &&
                        ((mode != 2) ? on->note() == on2->note() : 1 == 1)) {

                    if(on2->midiTime()>=on->midiTime()
                    && on2->midiTime()<=on->offEvent()->midiTime()
                    && (on2->midiTime()-on->midiTime()) > 2) {
                       on->offEvent()->setMidiTime(on2->midiTime() - 2);

                    }
                }
            }
        }
    }

    if(mode != 0) file->protocol()->endAction();

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
    buildCMajorProg(0, 0, 666);
}

void MainWindow::buildCMinor() {
    buildCMinorProg(0, 0, 666);
}

void MainWindow::buildCMajorInv1() {
    buildCMajorProg(-12, -12, 666);
}

void MainWindow::buildCMinorInv1() {
    buildCMinorProg(-12, -12, 666);
}

void MainWindow::buildCMajorInv2() {
    buildCMajorProg(0, -12, 666);
}

void MainWindow::buildCMinorInv2() {
    buildCMinorProg(0, -12, 666);
}

void MainWindow::buildCMajorProg(int d3, int d5, int d7) {

    if (!(Selection::instance()->selectedEvents().size() > 0 && file)) return;

    if (Selection::instance()->selectedEvents().size() > 0 && file) {

        QList<MidiEvent*> events;

        foreach (MidiEvent* e, Selection::instance()->selectedEvents()) {

            if(e) events.append(e);
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

                    MidiEvent* a=file->channel(e->channel())->insertNote(note1,
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

void MainWindow::buildCMinorProg(int d3, int d5, int d7) {

    if (!(Selection::instance()->selectedEvents().size() > 0 && file)) return;

    if (Selection::instance()->selectedEvents().size() > 0 && file) {

        QList<MidiEvent*> events;

        foreach (MidiEvent* e, Selection::instance()->selectedEvents()) {

            if(e) events.append(e);
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

                    MidiEvent* a =file->channel(e->channel())->insertNote(note1,
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

void MainWindow::buildpowerchord() {
    builddichord(666, 7, 666);
}

void MainWindow::buildpowerchordInv() {
    builddichord(666, -5, 666);
}

void MainWindow::buildpowerpowerchord() {
    builddichord(7, 12, 666);
}

void MainWindow::buildMajor() {
    builddichord(4, 7, 666);
}

void MainWindow::buildMinor() {
    builddichord(3, 7, 666);
}

void MainWindow::buildAug() {
    builddichord(4, 8, 666);
}

void MainWindow::buildDis() {
    builddichord(3, 6, 666);
}

void MainWindow::buildSeventh() {
    builddichord(4, 7, 10);
}

void MainWindow::buildMajorSeventh() {
    builddichord(4, 7, 11);
}

void MainWindow::buildMinorSeventh() {
    builddichord(3, 7, 10);
}

void MainWindow::buildMinorSeventhMajor() {
    builddichord(3, 7, 11);
}

void MainWindow::builddichord(int d3, int d5, int d7) {

    int dichord = 0;

    if(d3 == 666 && d7 == 666) dichord = 1;

    if(!(Selection::instance()->selectedEvents().size() > 0 && file)) return;

    if(Selection::instance()->selectedEvents().size() > 0 && file) {

        QList<MidiEvent*> events;

        foreach (MidiEvent* e, Selection::instance()->selectedEvents()) {

            if(e) events.append(e);
        }

        file->protocol()->startNewAction("Build Chord Notes", 0);

        foreach(MidiEvent* e, events) {
            NoteOnEvent *on = dynamic_cast<NoteOnEvent*>(e);

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

        file->protocol()->endAction();

        NotesCorrection(0);
    }
}

void MainWindow::pitchbend_effect1() {

    if (!(Selection::instance()->selectedEvents().size() > 0 && file)) return;

    if (Selection::instance()->selectedEvents().size() > 0 && file) {

        QList<MidiEvent*> events;
        foreach (MidiEvent* e, Selection::instance()->selectedEvents()) {

            if(e) events.append(e);
        }

        file->protocol()->startNewAction("Pitch Bend Effect1", 0);

        foreach (MidiEvent* e, events) {
            NoteOnEvent *on = dynamic_cast<NoteOnEvent*>(e);

            if (on) {
                int note = on->note();
                MidiEvent* off =(MidiEvent* ) on->offEvent();
                on->setNote(note);

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
}

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
    } else path="default.wav";

    QString savePath = QFileDialog::getSaveFileName(this, "Save WAV file",
                                                    path, "WAV Files (*.wav)");

    if(savePath.isEmpty()) {
        return; // canceled
    }

    QFile *wav = new QFile(savePath);
    if(wav->open(QIODevice::WriteOnly | QIODevice::Truncate)) {

        fluid_output->MIDtoWAV(wav, mw_matrixWidget, file);

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

void MainWindow::midi_marker_edit() {

    TextEventEdit* d = new TextEventEdit(file, 16, this, TEXTEVENT_NEW_MARKER);

    d->exec();
    delete d;
}


