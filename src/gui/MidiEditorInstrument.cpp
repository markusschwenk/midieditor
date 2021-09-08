/*
 * MidiEditor
 * Copyright (C) 2010  Markus Schwenk
 *
 * MidiEditorPiano
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

#include "MidiEditorInstrument.h"

#ifdef USE_FLUIDSYNTH
#include "../fluid/FluidDialog.h"
#else
#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDial>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSlider>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QWidget>

#include <QFrame>
#include <QTimer>
#include <QDialog>
#include <QObject>
#include <qscrollbar.h>
#include <QFileDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QPainter>

#include "../gui/MainWindow.h"

#endif

#include <QList>
#include <QtCore/qmath.h>

/************************************************/

#include <QToolBar>
#include "ChannelListWidget.h"
#include "MidiEditorInstrument.h"
#include <QDateTime>

static int yr = 40; // window position
static int yr2 = yr + 25 * 10;
static int is_rhythm = 1;

static int max_map_rhythm = 8; // size of data table - 2
static int current_map_rhythm = 0;
static int indx_bpm_rhythm = 16; // speed
static int current_tick_rhythm = 0;
// sample
static int play_rhythm_sample = 0;
static int loop_rhythm_sample = 0;

// track
static int play_rhythm_track = 0;
static int loop_rhythm_track = 0;
static int loop_rhythm_base = 0;
static int record_rhythm_track = 0;
static int play_rhythm_track_index = 0; // start/current position player
static int play_rhythm_track_index_old = 0; // retain start position
static int play_rhythm_track_init = 0; // flag to init
static int max_map_rhythm_track = 8; // size of data table - 2
static int indx_bpm_rhythm_track = 16; // speed

static int default_time = 0;
static int fix_time_sample = 0;
static int force_get_time_sample = 0;

static unsigned char map_rhythm[10][34];
static unsigned char map_rhythm_track[10][34];

static unsigned char map_rhythmData[10][2];
static unsigned char map_drum_rhythm_velocity[128];

static int _no_init = 1;

static void _init_rhythm() {

    max_map_rhythm = 8; // size of data table - 2
    current_map_rhythm = 0;
    indx_bpm_rhythm = 16; // speed
    current_tick_rhythm = 0;
    // sample
    play_rhythm_sample = 0;
    loop_rhythm_sample = 0;
    loop_rhythm_base = 0;

    // track
    play_rhythm_track = 0;
    loop_rhythm_track = 0;
    record_rhythm_track = 0;
    play_rhythm_track_index = 0; // start/current position player
    play_rhythm_track_index_old = 0; // retain start position
    play_rhythm_track_init = 0; // flag to init
    max_map_rhythm_track = 8; // size of data table - 2
    indx_bpm_rhythm_track = 16; // speed

    memset(map_rhythm, 0, sizeof(unsigned char) * 10 * 34);
    memset(map_rhythm_track, 0, sizeof(unsigned char) * 10 * 34);
    memset(map_drum_rhythm_velocity, 127, sizeof(unsigned char) * 128);

    fix_time_sample = 0;
    force_get_time_sample = 0;

    map_rhythm[0][0] = 42;
    map_rhythm[0][1] = 127;
    map_rhythm[1][0] = 38;
    map_rhythm[1][1] = 100;
    map_rhythm[2][0] = 35;
    map_rhythm[2][1] = 127;
    map_rhythm[3][0] = 49;
    map_rhythm[3][1] = 100;
    map_rhythm[4][0] = 55;
    map_rhythm[4][1] = 100;
    map_rhythm[5][0] = 57;
    map_rhythm[5][1] = 100;
    map_rhythm[6][0] = 51;
    map_rhythm[6][1] = 80;
    map_rhythm[7][0] = 44;
    map_rhythm[7][1] = 100;

    map_rhythm[8][0] = 41;
    map_rhythm[8][1] = 127;
    map_rhythm[9][0] = 48;
    map_rhythm[9][1] = 127;

    for(int n = 0; n < 10; n++) {
        map_rhythmData[n][0] = map_rhythm[n][0];
        map_rhythmData[n][1] = map_rhythm[n][1];
        map_drum_rhythm_velocity[map_rhythm[n][0]] = map_rhythm[n][1];
    }


}

#define TIMER_RHYTHM 10
#define TIMER_TICK 25
static int octave_pos = 48;
static int trans_pos = 0;
static int note_volume = 100;

static char piano_keys_map[18] =   {"ZSXDCVGBHNJMQ2W3E"};
static char piano_keys2_map[18]  = {"Q2W3ER5T6Y7UI9O0P"};

static int piano_keys_time[128]; // note duration ms
static int piano_keys_itime[128]; // note starts tick
static char piano_keys_key[128]; // key for note
static int playing_piano=-1; // note from mouse

void MyInstrument::Init_time(int ms) {
    _system_time = QDateTime::currentMSecsSinceEpoch();
    _init_time = ms;
}

int MyInstrument::Get_time() {
    return ((int) (QDateTime::currentMSecsSinceEpoch() - _system_time)) + (_init_time);
}

MyInstrument::MyInstrument(QWidget *MAINW, MatrixWidget* MW, MidiFile* f,  int is_drum) : QDialog(MAINW, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint){

    _MW= MW;
    _MAINW = MAINW;

    QD = this;

    drum_mode = is_drum;
    is_rhythm = (is_drum == 2);

    pianoEvent = NULL;
    file = f;

    _piano_insert_mode = 0;

    playing_piano=-1;
    I_NEED_TO_UPDATE = 0;

    is_playing = (MidiPlayer::isPlaying()) ? 1 : 0;

    foreach (MidiEvent* event, *(file->eventsBetween(0, 50))) {
        TempoChangeEvent* tempo = dynamic_cast<TempoChangeEvent*>(event);
        if(tempo) {
            msPerTick = tempo->msPerTick();
            break;
        }
    }

    if (QD->objectName().isEmpty())
        QD->setObjectName(QString::fromUtf8("MyInstrument"));

    if (!drum_mode)
        QD->setWindowTitle("MidiEditor Piano");
    else if(is_rhythm)
        QD->setWindowTitle("MidiEditor Rhythm Box");
    else
        QD->setWindowTitle("MidiEditor Drum");

    //QD->move(80,580);
    if(drum_mode) {

        if(is_rhythm) {
            QD->setFixedSize(160 + 24 * 32 + 24, 528 + 25 * 2);
        } else {
            QD->setFixedSize(924, 208);
        }

        DrumImage[0] = new QImage(":/run_environment/graphics/drum/closedhihat.png");
        DrumImage[1] = new QImage(":/run_environment/graphics/drum/pedalhihat.png");
        DrumImage[2] = new QImage(":/run_environment/graphics/drum/openhihat.png");
        DrumImage[3] = new QImage(":/run_environment/graphics/drum/crashcymbal.png");
        DrumImage[4] = new QImage(":/run_environment/graphics/drum/ridecymbal.png");
        DrumImage[5] = new QImage(":/run_environment/graphics/drum/tambourine.png");
        DrumImage[6] = new QImage(":/run_environment/graphics/drum/snare1.png");
        DrumImage[7] = new QImage(":/run_environment/graphics/drum/bassdrum.png");
        DrumImage[8] = new QImage(":/run_environment/graphics/drum/bassdrum.png");
        DrumImage[9] = new QImage(":/run_environment/graphics/drum/snare2.png");
        DrumImage[10] = new QImage(":/run_environment/graphics/drum/tom.png");

    } else {
        is_rhythm = 0;
        QD->setFixedSize(924, 136);
    }
    QD->setFocusPolicy(Qt::WheelFocus);

    MainWindow * Q = (MainWindow *) _MAINW;

    edit_opcion = NULL;

    if(is_rhythm) {

        QFont font2;

        font2.setPointSize(10);

        pixmapButton[0] = NULL;

        edit_opcion = new editDialog(this, EDITDIALOG_ALL);
        edit_opcion2 = new editDialog(this, EDITDIALOG_SAMPLE);

        if(_no_init) {
            _no_init = 0;
            _init_rhythm();
        }

        settings = new QSettings(QString("MidiEditor_Instrument"), QString("NONE"));

        QByteArray sa, sb;
        sa.clear();
        sb.clear();
        for(int n = 0; n < 128; n++)
            sa.append(map_drum_rhythm_velocity[n] & 127);
        sb= settings->value("InstVelocity", sa).toByteArray();
        for(int n = 0; n < 128; n++)
            map_drum_rhythm_velocity[n] = sb.at(n) & 127;
        for(int n = 0; n < 10; n++)
            map_rhythm[n][1]
                    = map_rhythmData[n][1]
                    = map_drum_rhythm_velocity[map_rhythm[n][0]];


        for(int n = 0; n < 10; n++) {
            drumBox[n] = new QMComboBox(this, n);
            drumBox[n]->setFont(font2);
            drumBox[n]->setObjectName(QString().asprintf("drumBox %i", n));
            drumBox[n]->setGeometry(QRect(22, yr + 16 + 25 * n, 136, 22));
            drumBox[n]->setToolTip("Select Pad note.\n"
                                   "Clicking with Right button\n"
                                   "set velocity note.");

            drumBox[n]->addItem("Acoustic Bass Drum", 35);
            drumBox[n]->addItem("Bass Drum", 36);
            drumBox[n]->insertSeparator(drumBox[n]->count());
            drumBox[n]->addItem("Side Stick", 37);
            drumBox[n]->addItem("Acoustic Snare", 38);
            drumBox[n]->addItem("Electric snare", 40);
            drumBox[n]->insertSeparator(drumBox[n]->count());
            drumBox[n]->addItem("Low floor Tom", 41);
            drumBox[n]->addItem("High floor Tom", 43);
            drumBox[n]->addItem("Low Tom", 45);
            drumBox[n]->addItem("Low-Mid tom", 47);
            drumBox[n]->addItem("Hi-Mid tom", 48);
            drumBox[n]->addItem("High tom", 50);
            drumBox[n]->insertSeparator(drumBox[n]->count());
            drumBox[n]->addItem("Closed Hi-hat", 42);
            drumBox[n]->addItem("Pedal Hi-hat", 44);
            drumBox[n]->addItem("Open Hi-hat", 46);
            drumBox[n]->insertSeparator(drumBox[n]->count());
            drumBox[n]->addItem("Crash Cymbal 1", 49);
            drumBox[n]->addItem("Crash Cymbal 2", 57);
            drumBox[n]->addItem("Splash Cymbal", 55);
            drumBox[n]->addItem("Chinese Cymbal", 52);
            drumBox[n]->addItem("Ride Cymbal 1", 51);
            drumBox[n]->addItem("Ride Cymbal 2", 59);
            drumBox[n]->insertSeparator(drumBox[n]->count());
            drumBox[n]->addItem("Hand-Clap", 39);
            drumBox[n]->addItem("Tambourine", 54);
            drumBox[n]->insertSeparator(drumBox[n]->count());
            drumBox[n]->addItem("Ride Bell", 53);
            drumBox[n]->addItem("Cowbell", 56);
            drumBox[n]->insertSeparator(drumBox[n]->count());
            drumBox[n]->addItem("Hi Bongo", 60);
            drumBox[n]->addItem("Low Bongo", 61);
            drumBox[n]->insertSeparator(drumBox[n]->count());
            drumBox[n]->addItem("Open Hi Conga", 63);
            drumBox[n]->addItem("Low Conga", 64);
            drumBox[n]->insertSeparator(drumBox[n]->count());
            drumBox[n]->addItem("High Timbale", 65);
            drumBox[n]->addItem("Low Timbale", 66);
            drumBox[n]->insertSeparator(drumBox[n]->count());
            drumBox[n]->addItem("High Agogo", 67);
            drumBox[n]->addItem("Low Agogo", 68);
            drumBox[n]->insertSeparator(drumBox[n]->count());
            drumBox[n]->addItem("Vibraslap", 58);
            drumBox[n]->addItem("Cabasa", 69);
            drumBox[n]->addItem("Maracas", 70);
            drumBox[n]->insertSeparator(drumBox[n]->count());
            drumBox[n]->addItem("Short Whistle", 71);
            drumBox[n]->addItem("Long Whistle", 72);
            drumBox[n]->insertSeparator(drumBox[n]->count());
            drumBox[n]->addItem("Short Guiro", 73);
            drumBox[n]->addItem("Long Guiro", 74);
            drumBox[n]->insertSeparator(drumBox[n]->count());
            drumBox[n]->addItem("Claves", 75);
            drumBox[n]->addItem("Mute Triangle", 80);
            drumBox[n]->insertSeparator(drumBox[n]->count());
            drumBox[n]->addItem("Hi Wood Block", 76);
            drumBox[n]->addItem("Low Wood Block", 77);
            drumBox[n]->insertSeparator(drumBox[n]->count());
            drumBox[n]->addItem("Mute Cuica", 78);
            drumBox[n]->addItem("Open Cuica", 79);


            for(int m = 0; m < drumBox[n]->count(); m++) {
                if(drumBox[n]->itemData(m).toInt() == map_rhythm[n][0])
                    drumBox[n]->setCurrentIndex(m);
            }


            connect(drumBox[n], QOverload<int>::of(&QComboBox::currentIndexChanged),[=](int v)
            {
                int set = drumBox[n]->itemData(v).toInt() & 127;
                map_rhythm[n][0] = set;
                map_rhythm[n][1] = map_drum_rhythm_velocity[set];

            });

        }

        int yb2 = yr2 + 12;

        pButton = new QPushButton(this);
        pButton->setObjectName(QString::fromUtf8("pButton"));
        pButton->setGeometry(QRect(22, yb2 + 16, 29, 16));
        pButton->setToolTip("Play/Stop Editor sample.");
        pButton->setText(QString());
        pButton->setCheckable(true);
        pButton->setAutoDefault(false);
        pButton->setFlat(false);
        pButton->setDefault(true);
        pButton->setStyleSheet(QString::fromUtf8(
                                   "QPushButton{\n"
                "background-image: url(:/run_environment/graphics/fluid/icon_button_off.png);\n"
                "background-position: center center;}\n"
                "QPushButton::checked {\n"
                "background-image: url(:/run_environment/graphics/fluid/icon_button_on.png);\n"
                "}"));
        pButton->setChecked((play_rhythm_sample) ? true : false);
        QLabel *plabel = new QLabel(this);
        plabel->setObjectName(QString::fromUtf8("plabel"));
        plabel->setGeometry(QRect(22, yb2, 29, 16));
        plabel->setAlignment(Qt::AlignLeading|Qt::AlignHCenter|Qt::AlignVCenter);
        plabel->setText("Play");
        connect(pButton, QOverload<bool>::of(&QPushButton::clicked),[=](bool)
        {
            if(!play_rhythm_sample && play_rhythm_track) {

                play_rhythm_track = 0;
                play_rhythm_track_index = -2;
                play_rhythm_track_init = 0;

                ptButton->setChecked(false);
                rhythmlist->setDisabled(false);
                rhythmSamplelist->setDisabled(false);
                rhythmlist->setCurrentRow(play_rhythm_track_index_old);
                rhythmlist->update();

            }

            play_rhythm_sample^= 1;
            current_map_rhythm = 0;
            current_tick_rhythm = 0;
        });

        lButton = new QPushButton(this);
        lButton->setObjectName(QString::fromUtf8("lButton"));
        lButton->setGeometry(QRect(22+ 34, yb2 + 16, 29, 16));
        lButton->setToolTip("Infinite loop to Play Editor sample.");
        lButton->setText(QString());
        lButton->setCheckable(true);
        lButton->setAutoDefault(false);
        lButton->setFlat(false);
        lButton->setDefault(true);
        lButton->setStyleSheet(QString::fromUtf8(
                                   "QPushButton{\n"
                "background-image: url(:/run_environment/graphics/fluid/icon_button_off.png);\n"
                "background-position: center center;}\n"
                "QPushButton::checked {\n"
                "background-image: url(:/run_environment/graphics/fluid/icon_button_on.png);\n"
                "}"));
        lButton->setChecked((loop_rhythm_sample) ? true : false);
        QLabel *llabel = new QLabel(this);
        llabel->setObjectName(QString::fromUtf8("llabel"));
        llabel->setGeometry(QRect(22 + 34, yb2, 29, 16));
        llabel->setAlignment(Qt::AlignLeading|Qt::AlignHCenter|Qt::AlignVCenter);
        llabel->setText("Loop");
        connect(lButton, QOverload<bool>::of(&QPushButton::clicked),[=](bool)
        {
            loop_rhythm_sample^= 1;
            if(!play_rhythm_sample) {

                if(play_rhythm_track) {
                    play_rhythm_track = 0;
                    play_rhythm_track_index = -2;
                    play_rhythm_track_init = 0;

                    ptButton->setChecked(false);
                    rhythmlist->setDisabled((play_rhythm_track) ? true : false);
                    rhythmSamplelist->setDisabled((play_rhythm_track) ? true : false);
                    rhythmlist->setCurrentRow(play_rhythm_track_index_old);
                    rhythmlist->update();
                }

                pButton->setChecked(true);
                play_rhythm_sample = 1;
            }

        });

        QPushButton *cleanButton = new QPushButton(this);
        cleanButton->setObjectName(QString::fromUtf8("cleanButton"));
        cleanButton->setGeometry(QRect(22, yb2 + 36, 34 + 29, 21));
        cleanButton->setToolTip("Clean Editor sample.");
        cleanButton->setText("Clean");
        cleanButton->setCheckable(true);
        cleanButton->setStyleSheet(QString::fromUtf8("QPushButton{\n"
                                                    "background-color:#505050;\n"
                                                    "border-left:2px solid #F0F0F0;\n"
                                                    "border-top: 2px solid #F0F0F0;\n"
                                                    "border-right:2px solid #C0C0C0;\n"
                                                    "border-bottom:2px solid #C0C0C0;\n"
                                                    "color:#EFEFEF;}\n"
                                                    "QPushButton:checked{\n"
                                                    "background-color:#404040;\n"
                                                    "border-left:2px solid #C0C0C0;\n"
                                                    "border-top: 2px solid #C0C0C0;\n"
                                                    "border-right:2px solid #F0F0F0;\n"
                                                    "border-bottom:2px solid #F0F0F0;\n"
                                                    "color:#80FF80;}\n"));

        connect(cleanButton, QOverload<bool>::of(&QPushButton::clicked),[=](bool)
        {

            play_rhythm_track = 0;
            play_rhythm_sample = 0;
            record_rhythm_track = 0;
            pButton->setChecked(false);
            ptButton->setChecked(false);
            //rButton->setChecked(false);
            QThread::msleep(100);
            rhythmlist->setDisabled(false);
            rhythmSamplelist->setDisabled(false);

            play_rhythm_sample = 0;
            current_map_rhythm = 0;
            current_tick_rhythm = 0;


            max_map_rhythm = 8; // size of data table - 2
            indx_bpm_rhythm = 16;
            memset(map_rhythm, 0, sizeof(unsigned char) * 10 * 34);

            fix_time_sample = 0;
            force_get_time_sample = 0;

            for (int n = 0; n < 10; n++) {
                map_rhythm[n][0] = map_rhythmData[n][0];
                map_rhythm[n][1] = map_rhythmData[n][1];
            }

            fixSpeedBox->setChecked((fix_time_sample) ? true : false);
            forceGetTimeBox->setChecked((force_get_time_sample) ? true : false);
            titleEdit->setText(QString("Sample"));

            save_rhythm_edit();

            for(int n = 0; n < 10; n++) {
                for(int m = 0; m < drumBox[n]->count(); m++) {
                    if(drumBox[n]->itemData(m).toInt() == map_rhythm[n][0])
                        drumBox[n]->setCurrentIndex(m);
                }
            }

            QThread::msleep(250);
            cleanButton->setChecked(false);
            update();

        });

        QToolBar* sampleMB = new QToolBar(QD);
        sampleMB->move(22, yb2 + 36 + 24);
        sampleMB->setIconSize(QSize(20, 20));
       // playbackMB
        sampleMB->setStyleSheet(QString::fromUtf8("background-color: #d0d0d0;"));
        QAction* loadAction = new QAction("Open drum sample", this);
        loadAction->setShortcut(QKeySequence::Open);
        loadAction->setIcon(QIcon(":/run_environment/graphics/tool/load.png"));
        connect(loadAction, SIGNAL(triggered()), this, SLOT(load_sample()));
        sampleMB->addAction(loadAction);

        QAction* saveAction = new QAction("Save Editor drum sample", this);
        saveAction->setShortcut(QKeySequence::Save);
        saveAction->setIcon(QIcon(":/run_environment/graphics/tool/save.png"));
        connect(saveAction, SIGNAL(triggered()), this, SLOT(save_sample()));
        sampleMB->addAction(saveAction);


        titleEdit = new QLineEdit(this);
        titleEdit->setFont(font2);
        titleEdit->setObjectName(QString::fromUtf8("titleEdit"));
        titleEdit->setGeometry(QRect(163 + 24 * 25 - 33,  yr2 + 14 + 25, 167 + 33, 21));
        titleEdit->setToolTip("Title/name of the sample\n"
                              "used to store or identify the sample");
        titleEdit->setMaxLength(24);
        titleEdit->setClearButtonEnabled(false);
        titleEdit->setText(QString("Sample"));

        fixSpeedBox = new QCheckBox(this);
        fixSpeedBox->setObjectName(QString::fromUtf8("fixSpeedBox"));
        fixSpeedBox->setGeometry(QRect(163, yr2 + 14 + 25, 70, 17));
        fixSpeedBox->setToolTip("Fix Speed of the sample.\n"
                                "When it is fixed and you store the sample, the editor uses the\n"
                                "internal sample value, when sample is loaded in the editor area\n"
                                "or played.\n\n"
                                "Otherwise this value is skipped and editor uses the current speed\n"
                                "(with the exception when it is edited directly from the library)\n\n"
                                "Fix/Unfix this value can be convenient when it mixing samples with\n"
                                "different speeds or it needs to be used at an specific speed\n"
                                "or to use the patterns freely, ignoring the internal speed.\n");
        fixSpeedBox->setChecked((fix_time_sample) ? true : false);
        fixSpeedBox->setText("Fix speed");
        connect(fixSpeedBox, QOverload<bool>::of(&QCheckBox::clicked), [=](bool checked)
        {
            if(checked)
                fix_time_sample = 1;
            else
                fix_time_sample = 0;

            save_rhythm_edit();
        });

        forceGetTimeBox = new QCheckBox(this);
        forceGetTimeBox->setObjectName(QString::fromUtf8("forceGetTimeBox"));
        forceGetTimeBox->setGeometry(QRect(163 + 75, yr2 + 14 + 25, 100, 17));
        forceGetTimeBox->setToolTip("Force get the internal speed of the sample\n"
                                "when it is loaded (EDIT, double clicking...)\n"
                                "in the editor area from the track samples.");
        forceGetTimeBox->setChecked((force_get_time_sample) ? true : false);
        forceGetTimeBox->setText("Force Get Speed");

        connect(forceGetTimeBox, QOverload<bool>::of(&QCheckBox::clicked), [=](bool checked)
        {
            if(checked)
                force_get_time_sample = 1;
            else
                force_get_time_sample = 0;

            save_rhythm_edit();
        });


        rhythmlist = new QMListWidget(this, 0);
        rhythmlist->setObjectName(QString::fromUtf8("rhythmlist"));
        rhythmlist->setGeometry(QRect(163, yr2 + 3 + 25 * 5, 24 * 32 , 65));
        rhythmlist->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        rhythmlist->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        rhythmlist->setFlow(QListWidget::LeftToRight);
        rhythmlist->setSpacing(2);
        rhythmlist->setAcceptDrops(true);
        rhythmlist->setDragEnabled(true);
        rhythmlist->setDefaultDropAction(Qt::MoveAction);
        rhythmlist->setToolTip("This is the Track section where will\n"
                               "you compose a piece dragging items from\n"
                               "Libray area, that you can save or record\n"
                               "more later using the corresponding\n"
                               "toolbar buttons\n\n"
                               "Use Double Click to edit and\n"
                               "Right Button for more options\n\n"
                               "Multiple selection and Drag & Drop\n"
                               "is allowed to move samples or to copy\n"
                               "(select, click to drag, Ctrl Key pressed\n"
                               "and drop, to copy) in this area.\n\n"
                               "Drop operation to Library area is not allowed");
        rhythmlist->setStyleSheet(QString::fromUtf8("background-color: #d0d0b0"));

        rhythmSamplelist = new QMListWidget(this, 1);
        rhythmSamplelist->setObjectName(QString::fromUtf8("rhythmSamplelist"));
        rhythmSamplelist->setGeometry(QRect(163, yr2 + 3 + 25 * 8, 24 * 32, 65));
        rhythmSamplelist->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        rhythmSamplelist->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        rhythmSamplelist->setFlow(QListWidget::LeftToRight);
        rhythmSamplelist->setSpacing(2);
        rhythmSamplelist->setAcceptDrops(true);
        rhythmSamplelist->setDragEnabled(true);
        rhythmSamplelist->setDefaultDropAction(Qt::CopyAction);
        rhythmSamplelist->setToolTip("This is the Library section. Track is generated dragging\n"
                                     "items (considered basic items or basic pieces) from here.\n\n"
                                     "Red Sample represent Editor area.\n\n"
                                     "Green samples represent the basic samples of a track. (a track\n"
                                     "usually contains multiple repeating samples)\n"
                                     "Green Samples items are inserted from the Editor sample (Red)\n"
                                     "or when one track is loaded (.drum track).\n\n"
                                     "White samples is a library element loaded as an internal resource\n"
                                     "or using the 'Add Library' button and you cannot to modify it\n\n"
                                     "Use Double Click to edit in the Editor area and\n"
                                     "Right Button for more options (except in library (white) items).\n\n"
                                     "Single selection and Drag & Drop to move in the Library area or\n"
                                     "copy to the Track area is allowed");

        rhythmSamplelist->setStyleSheet(QString::fromUtf8("background-color: #d0d0b0"));
        rhythmSamplelist->setSelectionMode(QAbstractItemView::SingleSelection);

        QFile *drum_resource = new QFile(":/run_environment/resource.drum");
        if(drum_resource && drum_resource->open(QIODevice::ReadOnly)) {

             pop_track_rhythm(drum_resource, 1);
             drum_resource->close();

        } else {// insert editor entry

            QListWidgetItem* item = new QListWidgetItem();
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            rhythmData rhythm = rhythmData();

            rhythm.setName(titleEdit->text());

            rhythm.set_rhythm_header(max_map_rhythm, indx_bpm_rhythm | ((fix_time_sample) ? 0 : 128), 0);
            for(int n = 0; n < 10; n++)
                rhythm.insert(n, &map_rhythm[n][0]);

            item->setData(Qt::UserRole, rhythm.save());
            rhythmSamplelist->addItem(item);
            rhythmSamplelist->setCurrentRow(0);
            rhythmSamplelist->update();
        }

        pop_track_rhythm();

        connect(rhythmlist, QOverload<QListWidgetItem*>::of(&QListWidget::itemDoubleClicked),[=](QListWidgetItem *i)
        {
            if(rhythmlist->count() <= 0) return;

            current_map_rhythm = 0; // EDIT

            current_tick_rhythm = 0;
            play_rhythm_sample = 0;
            pButton->setChecked(false);

            memset(map_rhythm, 0, sizeof(unsigned char) * 10 * 34);
            QByteArray a = i->data(Qt::UserRole).toByteArray();
            rhythmData rhythm = rhythmData(a);

            titleEdit->setText(rhythm.getName());
            int ticks;
            rhythm.get_rhythm_header(&max_map_rhythm, &ticks, NULL);
            if(!(ticks & 128) || force_get_time_sample) {
                 indx_bpm_rhythm = ticks & 127;
                 if(!(ticks & 128))
                    fix_time_sample = 1;
                 else
                    fix_time_sample = 0;
                 fixSpeedBox->setChecked((fix_time_sample) ? true : false);
            } else {
                 fix_time_sample = 0;
                 fixSpeedBox->setChecked(false);
            }


            for(int n = 0; n < 10; n++) {
                rhythm.get(n, &map_rhythm[n][0]);

                // ignore velocity from sample
                map_rhythm[n][1] = map_drum_rhythm_velocity[map_rhythm[n][0] & 127];

                for(int m = 0; m < drumBox[n]->count(); m++) {
                    if(drumBox[n]->itemData(m).toInt() == rhythm.get_drum_note(n))
                        drumBox[n]->setCurrentIndex(m);
                }

            }
            save_rhythm_edit();
            QD->update();

        });

        connect(rhythmlist, QOverload<QListWidgetItem*>::of(&QListWidget::itemPressed),[=](QListWidgetItem *i)
        {

            if(rhythmlist->count() <= 0) return;

            if(rhythmlist->buttons != Qt::RightButton) {

                return;
            }

            QRect rect = rhythmlist->visualItemRect(i);

            edit_opcion->move(rect.x() + QD->x() + rhythmlist->x()  ,
                              rect.y() + QD->y() + rhythmlist->y() + 32);
            edit_opcion->setItem(i);
            edit_opcion->setModal(true);
            edit_opcion->exec();

            int action = edit_opcion->action();

            if(action == EDITDIALOG_REMOVE) { // delete

                QList <QListWidgetItem *> list = rhythmlist->selectedItems();
                if(list.count() > 1) {

                    if(QMessageBox::information(this, "Rhythm Box Editor",
                        "Are you sure to REMOVE multiple samples?",
                        QMessageBox::Yes | QMessageBox::No)
                            != QMessageBox::Yes) return;

                    int row = 0;

                    foreach(QListWidgetItem *item, list) {

                        row = rhythmlist->row(item);

                        if(row < 0) continue;

                        delete rhythmlist->takeItem(row);
                    }

                    rhythmlist->setCurrentRow(row);
                    rhythmlist->update();
                    return;
                } else {
                    if(QMessageBox::information(this, "Rhythm Box Editor",
                        "Are you sure to REMOVE this sample?",
                        QMessageBox::Yes | QMessageBox::No)
                            != QMessageBox::Yes) return;
                }

                int row = rhythmlist->row(i);

                if(row < 0) {

                    return;
                }

                delete rhythmlist->takeItem(row);
                rhythmlist->setCurrentRow(row);
                rhythmlist->update();


            } else if(action == EDITDIALOG_REPLACE) { // delete

                int row = rhythmlist->row(i);

                if(row < 0) {

                    return;
                }

                if(titleEdit->text() == "" || titleEdit->text().count() < 4
                        || titleEdit->text().toUtf8().at(0) < 65) {
                    QMessageBox::information(this, "Rhythm Box Editor",
                                        "sample name too short or invalid",
                                        QMessageBox::Ok);
                    return;
                }

                if(QMessageBox::information(this, "Rhythm Box Editor",
                    "Are you sure to REPLACE all samples\nwith the same name of the track?",
                    QMessageBox::Yes | QMessageBox::No)
                        != QMessageBox::Yes) return;

                current_tick_rhythm = 0;
                play_rhythm_sample = 0;
                pButton->setChecked(false);

                rhythmData rhythmSet = rhythmData();

                rhythmSet.setName(titleEdit->text());

                rhythmSet.set_rhythm_header(max_map_rhythm, indx_bpm_rhythm | ((fix_time_sample) ? 0 : 128), 0);
                for(int n = 0; n < 10; n++)
                    rhythmSet.insert(n, &map_rhythm[n][0]);

                QByteArray a = i->data(Qt::UserRole).toByteArray();
                rhythmData rhythm = rhythmData(a);

                for(int n = 0; n < rhythmlist->count(); n++) {
                    QByteArray b = rhythmlist->item(n)->data(Qt::UserRole).toByteArray();
                    rhythmData rhythm2 = rhythmData(b);

                    if(rhythm.getName() == rhythm2.getName()) {

                        rhythmlist->item(n)->setData(Qt::UserRole, rhythmSet.save());
                        rhythmlist->update();
                    }
                }

                QD->update();

            } else if(action == EDITDIALOG_EDIT) { // edit

                current_map_rhythm = 0;

                current_tick_rhythm = 0;
                play_rhythm_sample = 0;
                pButton->setChecked(false);

                memset(map_rhythm, 0, sizeof(unsigned char) * 10 * 34);
                QByteArray a = i->data(Qt::UserRole).toByteArray();
                rhythmData rhythm = rhythmData(a);

                titleEdit->setText(rhythm.getName());
                int ticks;
                rhythm.get_rhythm_header(&max_map_rhythm, &ticks, NULL);
                if(!(ticks & 128) || force_get_time_sample) {
                     indx_bpm_rhythm = ticks & 127;
                     if(!(ticks & 128))
                        fix_time_sample = 1;
                     else
                        fix_time_sample = 0;
                     fixSpeedBox->setChecked((fix_time_sample) ? true : false);
                } else {
                     fix_time_sample = 0;
                     fixSpeedBox->setChecked(false);
                }

                for(int n = 0; n < 10; n++) {

                    rhythm.get(n, &map_rhythm[n][0]);

                    // ignore velocity from sample
                    map_rhythm[n][1] = map_drum_rhythm_velocity[map_rhythm[n][0] & 127];

                    for(int m = 0; m < drumBox[n]->count(); m++) {
                        if(drumBox[n]->itemData(m).toInt() == rhythm.get_drum_note(n))
                            drumBox[n]->setCurrentIndex(m);
                    }

                }
                save_rhythm_edit();
                QD->update();

            } else if(action == EDITDIALOG_INSERT
                      || action == EDITDIALOG_SET) { // set / insert

               current_tick_rhythm = 0;
               play_rhythm_sample = 0;
               pButton->setChecked(false);

               if(titleEdit->text() == "" || titleEdit->text().count() < 4
                       || titleEdit->text().toUtf8().at(0) < 65) {
                   QMessageBox::information(this, "Rhythm Box Editor",
                                       "sample name too short or invalid",
                                       QMessageBox::Ok);
                   return;
               }

               rhythmData rhythm = rhythmData();

               rhythm.setName(titleEdit->text());

               rhythm.set_rhythm_header(max_map_rhythm, indx_bpm_rhythm | ((fix_time_sample) ? 0 : 128), 0);
               for(int n = 0; n < 10; n++)
                   rhythm.insert(n, &map_rhythm[n][0]);

               if(action == EDITDIALOG_INSERT) {
                   int row = rhythmlist->row(i);

                   if(row >= 0) {
                       QListWidgetItem* i = new QListWidgetItem();
                       i->setFlags(i->flags() | Qt::ItemIsUserCheckable);
                       i->setData(Qt::UserRole, rhythm.save());
                       rhythmlist->insertItem(row, i);                       
                       rhythmlist->update();

                   } else {
                       i->setData(Qt::UserRole, rhythm.save());
                       rhythmlist->update();
                   }

                   QD->update();
               } else {

                   i->setData(Qt::UserRole, rhythm.save());
                   rhythmlist->update();
                   QD->update();
               }

           }

        });

        ///////////////////////

        connect(rhythmSamplelist, QOverload<QListWidgetItem*>::of(&QListWidget::itemDoubleClicked),[=](QListWidgetItem *i)
        {
            if(rhythmSamplelist->count() <= 0) return;

            current_map_rhythm = 0; // EDIT

            current_tick_rhythm = 0;
            play_rhythm_sample = 0;
            pButton->setChecked(false);

            memset(map_rhythm, 0, sizeof(unsigned char) * 10 * 34);
            QByteArray a = i->data(Qt::UserRole).toByteArray();
            rhythmData rhythm = rhythmData(a);

            titleEdit->setText(rhythm.getName());

            int ticks;
            rhythm.get_rhythm_header(&max_map_rhythm, &ticks, NULL);

            if(!(ticks & 128)) {
                 indx_bpm_rhythm = ticks;
                 fix_time_sample = 1;
                 fixSpeedBox->setChecked(true);
            } else {
                 fix_time_sample = 0;
                 fixSpeedBox->setChecked(false);
                 indx_bpm_rhythm = ticks & 127; // from library pass
            }

            for(int n = 0; n < 10; n++) {

                rhythm.get(n, &map_rhythm[n][0]);

                // ignore velocity from sample
                map_rhythm[n][1] = map_drum_rhythm_velocity[map_rhythm[n][0] & 127];

                for(int m = 0; m < drumBox[n]->count(); m++) {
                    if(drumBox[n]->itemData(m).toInt() == rhythm.get_drum_note(n))
                        drumBox[n]->setCurrentIndex(m);
                }

            }
            save_rhythm_edit();
            QD->update();

        });

        connect(rhythmSamplelist, QOverload<QListWidgetItem*>::of(&QListWidget::itemPressed),[=](QListWidgetItem *i)
        {

            if(rhythmSamplelist->count() <= 0) return;

            if(rhythmSamplelist->buttons != Qt::RightButton) {

                return;
            }

            if(!(i->flags() & Qt::ItemIsUserCheckable)) return;

            QRect rect = rhythmSamplelist->visualItemRect(i);

            edit_opcion2->move(rect.x() + QD->x() + rhythmSamplelist->x()  ,
                              rect.y() + QD->y() + rhythmSamplelist->y() + 32);
            edit_opcion2->setItem(i);
            edit_opcion2->setModal(true);
            edit_opcion2->exec();

            int action = edit_opcion2->action();

            if(action == EDITDIALOG_REMOVE) { // delete

                int row = rhythmSamplelist->row(i);

                if(row <= 0) {

                    return;
                }

                if(QMessageBox::information(this, "Rhythm Box Editor",
                "Library object is selected\nAre you sure to REMOVE this sample?",
                QMessageBox::Yes | QMessageBox::No)
                    != QMessageBox::Yes) return;

                delete rhythmSamplelist->takeItem(row);
                rhythmSamplelist->setCurrentRow(row);

                rhythmSamplelist->update();


            } else if(action == EDITDIALOG_EDIT) { // edit

                current_map_rhythm = 0;

                current_tick_rhythm = 0;
                play_rhythm_sample = 0;
                pButton->setChecked(false);

                memset(map_rhythm, 0, sizeof(unsigned char) * 10 * 34);
                QByteArray a = i->data(Qt::UserRole).toByteArray();
                rhythmData rhythm = rhythmData(a);

                titleEdit->setText(rhythm.getName());
                int ticks;
                rhythm.get_rhythm_header(&max_map_rhythm, &ticks, NULL);
                if(!(ticks & 128)) {
                    indx_bpm_rhythm = ticks;
                     fix_time_sample = 1;
                     fixSpeedBox->setChecked(true);
                } else {
                     fix_time_sample = 0;
                     fixSpeedBox->setChecked(false);
                     indx_bpm_rhythm = ticks & 127; // from library pass
                }

                for(int n = 0; n < 10; n++) {

                    rhythm.get(n, &map_rhythm[n][0]);

                    // ignore velocity from sample
                    map_rhythm[n][1] = map_drum_rhythm_velocity[map_rhythm[n][0] & 127];

                    for(int m = 0; m < drumBox[n]->count(); m++) {
                        if(drumBox[n]->itemData(m).toInt() == rhythm.get_drum_note(n))
                            drumBox[n]->setCurrentIndex(m);
                    }

                }
                save_rhythm_edit();
                QD->update();

            } else if(action == EDITDIALOG_INSERT
                      || action == EDITDIALOG_SET) { // set / insert

                if(titleEdit->text() == "" || titleEdit->text().count() < 4
                        || titleEdit->text().toUtf8().at(0) < 65) {
                    QMessageBox::information(this, "Rhythm Box Editor",
                                        "sample name too short or invalid",
                                        QMessageBox::Ok);
                    return;
                }

                /////////////////////

                current_tick_rhythm = 0;
                play_rhythm_sample = 0;
                pButton->setChecked(false);

                rhythmData rhythmSet = rhythmData();

                rhythmSet.setName(titleEdit->text());

                rhythmSet.set_rhythm_header(max_map_rhythm, indx_bpm_rhythm | ((fix_time_sample) ? 0 : 128), 0);
                for(int n = 0; n < 10; n++)
                    rhythmSet.insert(n, &map_rhythm[n][0]);
                QByteArray c = rhythmSet.save();

                QByteArray a = i->data(Qt::UserRole).toByteArray();
                rhythmData rhythm = rhythmData(a);
                // skip 0
                for(int n = 1; n < rhythmSamplelist->count() && action != EDITDIALOG_SET; n++) {
                    QByteArray b = rhythmSamplelist->item(n)->data(Qt::UserRole).toByteArray();
                    rhythmData rhythm2 = rhythmData(b);
                    if(c == b) {
                        QMessageBox::information(this, "Rhythm Box Editor",
                        "Library object exist with the same datas",
                        QMessageBox::Ok);
                        return;
                    }
                    if(rhythmSet.getName() == rhythm2.getName()) {
                        QMessageBox::information(this, "Rhythm Box Editor",
                        "Library object exist with the same name",
                        QMessageBox::Ok);
                        return;
                    }
                }

                if(action == EDITDIALOG_SET)
                    if(QMessageBox::information(this, "Rhythm Box Editor",
                    "Library object is selected\nAre you sure to SET this sample?",
                    QMessageBox::Yes | QMessageBox::No)
                        != QMessageBox::Yes) return;


               if(action == EDITDIALOG_INSERT) {
                   int row = rhythmSamplelist->row(i);

                   if(row >= 0) {
                       QListWidgetItem* i = new QListWidgetItem();

                       i->setFlags(i->flags() | Qt::ItemIsUserCheckable);
                       i->setData(Qt::UserRole, rhythmSet.save());

                       rhythmSamplelist->insertItem(row + 1, i);
                       rhythmSamplelist->update();

                   } else {
                       i->setData(Qt::UserRole, rhythm.save());
                       rhythmSamplelist->update();
                   }

                   QD->update();
               } else {
                   i->setData(Qt::UserRole, rhythm.save());
                   rhythmSamplelist->update();
                   QD->update();
               }

           }

       });

        //

        connect(titleEdit, QOverload<const QString &>::of(&QLineEdit::textChanged), [=](const QString)
        {
            save_rhythm_edit();
            QD->update();
        });

        yb2 = yr2 + 3 + 25 * 5 - 8;
        ptButton = new QPushButton(this);
        ptButton->setObjectName(QString::fromUtf8("ptButton"));
        ptButton->setGeometry(QRect(22, yb2 + 16, 29, 16));
        ptButton->setToolTip("Play/Stop the track");
        ptButton->setText(QString());
        ptButton->setCheckable(true);
        ptButton->setAutoDefault(false);
        ptButton->setFlat(false);
        ptButton->setDefault(true);
        ptButton->setStyleSheet(QString::fromUtf8(
                                   "QPushButton{\n"
                "background-image: url(:/run_environment/graphics/fluid/icon_button_off.png);\n"
                "background-position: center center;}\n"
                "QPushButton::checked {\n"
                "background-image: url(:/run_environment/graphics/fluid/icon_button_on.png);\n"
                "}"));
        ptButton->setChecked((play_rhythm_track) ? true : false);
        QLabel *ptlabel = new QLabel(this);
        ptlabel->setObjectName(QString::fromUtf8("ptlabel"));
        ptlabel->setGeometry(QRect(22, yb2, 29, 16));
        ptlabel->setAlignment(Qt::AlignLeading|Qt::AlignHCenter|Qt::AlignVCenter);
        ptlabel->setText("Play");
        connect(ptButton, QOverload<bool>::of(&QPushButton::clicked),[=](bool)
        {
            if(rhythmlist->count() <= 0) {
                play_rhythm_track = 0;
                ptButton->setChecked(false);
                return;
            }
            play_rhythm_sample = 0;
            pButton->setChecked(false);

            rhythmlist->update();

            if(!play_rhythm_track) {
                Play_Thread::rhythm_samples.clear();
                for(int n = 0; n < rhythmlist->count(); n++)
                    Play_Thread::rhythm_samples.append(rhythmlist->item(n)->data(Qt::UserRole).toByteArray());
            }

            play_rhythm_track^= 1;

            rhythmlist->setDisabled((play_rhythm_track) ? true : false);
            rhythmSamplelist->setDisabled((play_rhythm_track) ? true : false);
            play_rhythm_track_index = 0;

            play_rhythm_track_index =
                    play_rhythm_track_index_old = rhythmlist->currentRow();

            play_rhythm_track_init = play_rhythm_track;

            current_map_rhythm = 0;
            current_tick_rhythm = 0;
        });

        ltButton = new QPushButton(this);
        ltButton->setObjectName(QString::fromUtf8("ltButton"));
        ltButton->setGeometry(QRect(22 + 34, yb2 + 16, 29, 16));
        ltButton->setToolTip("Enables/disable the Loop in track");
        ltButton->setText(QString());
        ltButton->setCheckable(true);
        ltButton->setAutoDefault(false);
        ltButton->setFlat(false);
        ltButton->setDefault(true);
        ltButton->setStyleSheet(QString::fromUtf8(
                                   "QPushButton{\n"
                "background-image: url(:/run_environment/graphics/fluid/icon_button_off.png);\n"
                "background-position: center center;}\n"
                "QPushButton::checked {\n"
                "background-image: url(:/run_environment/graphics/fluid/icon_button_on.png);\n"
                "}"));
        ltButton->setChecked((loop_rhythm_track) ? true : false);
        QLabel *ltlabel = new QLabel(this);
        ltlabel->setObjectName(QString::fromUtf8("ltlabel"));
        ltlabel->setGeometry(QRect(22 + 34, yb2, 29, 16));
        ltlabel->setAlignment(Qt::AlignLeading|Qt::AlignHCenter|Qt::AlignVCenter);
        ltlabel->setText("Loop");
        connect(ltButton, QOverload<bool>::of(&QPushButton::clicked),[=](bool)
        {
            loop_rhythm_track = (loop_rhythm_track) ? 0 : 1;

        });

        fixTrackSpeedBox = new QCheckBox(this);
        fixTrackSpeedBox->setObjectName(QString::fromUtf8("fixTrackSpeedBox"));
        fixTrackSpeedBox->setToolTip("Checked to skip the internal sample\n"
                                     "speed when you play the track");
        fixTrackSpeedBox->setGeometry(QRect(22, yb2 + 40, 140, 17));
        fixTrackSpeedBox->setChecked(default_time ? true : false);
        fixTrackSpeedBox->setText("Use speed from editor");

        connect(fixTrackSpeedBox, QOverload<bool>::of(&QCheckBox::clicked), [=](bool checked)
        {
            if(checked)
                default_time = 1;
            else
                default_time = 0;

            save_rhythm_edit();
        });

        QLabel *looplabel = new QLabel(this);
        looplabel->setObjectName(QString::fromUtf8("looplabel"));
        looplabel->setGeometry(QRect(22, yb2 + 40 + 24, 61, 16));
        looplabel->setAlignment(Qt::AlignLeading|Qt::AlignHCenter|Qt::AlignVCenter);
        looplabel->setText("N Loops");

        loopBox = new QSpinBox(this);
        loopBox->setObjectName(QString::fromUtf8("loopBox"));
        loopBox->setGeometry(QRect(22, yb2 + 40 + 24 + 16, 61, 22));
        loopBox->setToolTip("Number of loops when you play/record the track\n"
                            "(0 = infinite playing, in recording is ignored)");
        QFont font;
        font.setPointSize(12);
        loopBox->setFont(font);
        loopBox->setWrapping(false);
        loopBox->setAlignment(Qt::AlignCenter);
        loopBox->setMaximum(32);
        loopBox->setValue(loop_rhythm_base);

        connect(loopBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int v)
        {
            loop_rhythm_base = v;

        });

        QPushButton *libraryButton = new QPushButton(this);
        libraryButton->setObjectName(QString::fromUtf8("libraryButton"));
        libraryButton->setGeometry(QRect(22, yb2 + 110, 34 + 29, 38));
        libraryButton->setText("Add\nLibrary");
        libraryButton->setCheckable(true);
        libraryButton->setToolTip("Load news samples\nfrom .drum file\nto the library");
        libraryButton->setStyleSheet(QString::fromUtf8("QPushButton{\n"
                                                    "background-color:#505050;\n"
                                                    "border-left:2px solid #F0F0F0;\n"
                                                    "border-top: 2px solid #F0F0F0;\n"
                                                    "border-right:2px solid #C0C0C0;\n"
                                                    "border-bottom:2px solid #C0C0C0;\n"
                                                    "color:#EFEFEF;}\n"
                                                    "QPushButton:checked{\n"
                                                    "background-color:#404040;\n"
                                                    "border-left:2px solid #C0C0C0;\n"
                                                    "border-top: 2px solid #C0C0C0;\n"
                                                    "border-right:2px solid #F0F0F0;\n"
                                                    "border-bottom:2px solid #F0F0F0;\n"
                                                    "color:#80FF80;}\n"));

        connect(libraryButton, QOverload<bool>::of(&QPushButton::clicked),[=](bool)
        {
            QString appdir = settings->value("drum_path_library").toString();

            if(appdir.isEmpty())
                appdir = settings->value("drum_path").toString();
            if(appdir.isEmpty())
                appdir = QDir::homePath();

            QString newPath = QFileDialog::getOpenFileName(this, "Open Drum Track file",
                appdir, "Drum Files (*.drum)");
            if(newPath.isEmpty()) {
                libraryButton->setChecked(false);
                return; // canceled
            }

            QFile *drum_resource = new QFile(newPath);
            if(drum_resource && drum_resource->open(QIODevice::ReadOnly)) {

                 pop_track_rhythm(drum_resource, 1);
                 drum_resource->close();
                 settings->setValue("drum_path_library", QFileInfo(newPath).absoluteDir().absolutePath());


            }
            QThread::msleep(250);
            libraryButton->setChecked(false);
            update();

        });

    }

    QToolBar* playbackMB = new QToolBar(QD);
    playbackMB->move(164, 0);
    playbackMB->setIconSize(QSize(20, 20));
    playbackMB->setStyleSheet(QString::fromUtf8("background-color: #d0d0d0;"));

    if(is_rhythm) {

        QAction* newAction = new QAction("New drum track", this);
        newAction->setShortcut(QKeySequence::New);
        newAction->setIcon(QIcon(":/run_environment/graphics/tool/new.png"));
        connect(newAction, SIGNAL(triggered()), this, SLOT(new_track()));
        playbackMB->addAction(newAction);

        QAction* loadAction = new QAction("Open drum track", this);
        loadAction->setShortcut(QKeySequence::Open);
        loadAction->setIcon(QIcon(":/run_environment/graphics/tool/load.png"));
        connect(loadAction, SIGNAL(triggered()), this, SLOT(load_track()));
        playbackMB->addAction(loadAction);

        QAction* saveAction = new QAction("Save drum track", this);
        saveAction->setShortcut(QKeySequence::Save);
        saveAction->setIcon(QIcon(":/run_environment/graphics/tool/save.png"));
        connect(saveAction, SIGNAL(triggered()), this, SLOT(save_track()));
        playbackMB->addAction(saveAction);
        playbackMB->addSeparator();

        QAction* copyAction = new QAction("Copy samples from track", this);
        copyAction->setIcon(QIcon(":/run_environment/graphics/tool/copy.png"));
        copyAction->setShortcut(QKeySequence::Copy);
        connect(copyAction, SIGNAL(triggered()), this, SLOT(copy_samples()));
        playbackMB->addAction(copyAction);

        QAction* pasteAction = new QAction("Paste samples", this);
        pasteAction->setToolTip("Paste samples at selected track position");
        pasteAction->setShortcut(QKeySequence::Paste);
        pasteAction->setIcon(QIcon(":/run_environment/graphics/tool/paste.png"));
        connect(pasteAction, SIGNAL(triggered()), this, SLOT(paste_samples()));
        playbackMB->addAction(pasteAction);
        playbackMB->addSeparator();
    }


    QAction* backToBeginAction = new QAction("Back to begin", this);
    backToBeginAction->setIcon(QIcon(":/run_environment/graphics/tool/back_to_begin.png"));
    connect(backToBeginAction, SIGNAL(triggered()), this, SLOT(rbackToBegin()));
    playbackMB->addAction(backToBeginAction);

    QAction* backAction = new QAction("Previous measure", this);
    backAction->setIcon(QIcon(":/run_environment/graphics/tool/back.png"));
    connect(backAction, SIGNAL(triggered()), this, SLOT(rback()));
    playbackMB->addAction(backAction);

    QAction* playAction = new QAction("Play", this);
    playAction->setIcon(QIcon(":/run_environment/graphics/tool/play.png"));
    connect(playAction, SIGNAL(triggered()), this, SLOT(rplay()));
    playbackMB->addAction(playAction);

    if(is_rhythm) {
        QAction* playAction2 = new QAction("Play Sync", this);
        playAction2->setIcon(QIcon(":/run_environment/graphics/tool/play2.png"));
        connect(playAction2, SIGNAL(triggered()), this, SLOT(rplay2()));
        playbackMB->addAction(playAction2);
    }

    QAction* pauseAction = new QAction("Pause", this);
    pauseAction->setIcon(QIcon(":/run_environment/graphics/tool/pause.png"));
    connect(pauseAction, SIGNAL(triggered()), this, SLOT(rpause()));
    playbackMB->addAction(pauseAction);

    /////////
    if(is_rhythm) {
        QAction* recordAction = new QAction("Record the Drum track", this);
        recordAction->setIcon(QIcon(":/run_environment/graphics/tool/record2.png"));
        recordAction->setCheckable(true);
        recordAction->setChecked(false);

            connect(recordAction, QOverload<bool>::of(&QAction::triggered),[=](bool)
            {
                int init_time = -1;

                if(is_playsync && play_rhythm_track) {
                    ptButton->setChecked(false);
                    emit ptButton->clicked();
                    is_playsync = 0;
                }

                if(MidiInput::recording()) {

                    return;
                }

                record_rhythm_track^= 1;
                if(MidiPlayer::isPlaying()) {

                    init_time = MidiPlayer::timeMs();
                    rstop();
                } else {

                    if(file->pauseTick() >= 0)
                        init_time = file->msOfTick(file->pauseTick());
                }

                if(rhythmlist->count() <= 0) {
                    record_rhythm_track = 0;

                    return;
                }

                play_rhythm_track = 0;
                play_rhythm_sample = 0;
                pButton->setChecked(false);
                ptButton->setChecked(false);
                QThread::msleep(100);

                rhythmlist->update();

                rhythmlist->setDisabled(true);
                rhythmSamplelist->setDisabled(true);
                play_rhythm_track_index = 0;
                play_rhythm_track_index = 0;

                play_rhythm_track_init = 0;

                current_map_rhythm = 0;
                current_tick_rhythm = 0;

                recordAction->setChecked(true);
                RecRhythm(init_time);
                RecSave();
                record_rhythm_track = 0;

                rhythmlist->setDisabled(false);
                rhythmSamplelist->setDisabled(false);
                recordAction->setChecked(false);

            });

        playbackMB->addAction(recordAction);
    }

    ////////
    QAction* stopAction = new QAction("Stop", this);
    stopAction->setIcon(QIcon(":/run_environment/graphics/tool/stop.png"));
    connect(stopAction, SIGNAL(triggered()), this, SLOT(rstop()));
    playbackMB->addAction(stopAction);

    QAction* forwAction = new QAction("Next measure", this);
    forwAction->setIcon(QIcon(":/run_environment/graphics/tool/forward.png"));
    connect(forwAction, SIGNAL(triggered()), this, SLOT(rforward()));
    playbackMB->addAction(forwAction);

    playbackMB->addSeparator();

    // instrument
    QAction* instrumentAction;

    if (!drum_mode)
        instrumentAction = new QAction(QIcon(":/run_environment/graphics/channelwidget/instrument.png"), "Select instrument", playbackMB);
    else
        instrumentAction = new QAction(QIcon(":/run_environment/graphics/tool/drum.png"), "Select instrument", playbackMB);

    playbackMB->addAction(instrumentAction);
    connect(instrumentAction, QOverload<bool>::of(&QAction::triggered),[=]()
    {
        emit Q->channelWidget->selectInstrumentClicked((drum_mode) ? 9 : NewNoteTool::editChannel());
    });

    QComboBox *_chooseEditChannel = NULL;

    if(!drum_mode) {

        _chooseEditChannel = new QComboBox(QD);

        for (int i = 0; i < 16; i++) {
            _chooseEditChannel->addItem("Channel " + QString::number(i));
        }

        _chooseEditChannel->setFocusPolicy(Qt::NoFocus);
        _chooseEditChannel->setFixedWidth(180);
        _chooseEditChannel->setFixedHeight(24);
        _chooseEditChannel->setStyleSheet(QString::fromUtf8("background-color: white;"));
    }

    playbackMB->addSeparator();

    if(!drum_mode) {
        playbackMB->addWidget(_chooseEditChannel);
        playbackMB->addSeparator();
    }

    QAction* panicAction = new QAction("Midi panic", this);
    panicAction->setIcon(QIcon(":/run_environment/graphics/tool/panic.png"));
    panicAction->setShortcut(QKeySequence(Qt::Key_Escape));
    connect(panicAction, SIGNAL(triggered()), Q, SLOT(panic()));
    playbackMB->addAction(panicAction);
    playbackMB->addSeparator();

    if(!drum_mode) {
        connect(_chooseEditChannel, QOverload<int>::of(&QComboBox::currentIndexChanged),[=](int v)
        {
            Q->editChannel(v);
            QD->setFocus();
        });

        _chooseEditChannel->setCurrentIndex(NewNoteTool::editChannel());
    }
    setFocus();

    if(!is_rhythm) {
        groupBox = new QGroupBox(QD);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        groupBox->setGeometry(QRect(800, 2, 120, /*91+32*/132));
        groupBox->setFlat(false);
        groupBox->setStyleSheet(QString::fromUtf8("color: white"));

        recordButton = new QPushButton(groupBox);
        recordButton->setObjectName(QString::fromUtf8("recordButton"));
        recordButton->setGeometry(QRect(14, 61, 29, 16));
        recordButton->setText(QString());
        recordButton->setCheckable(true);
        recordButton->setAutoDefault(false);
        recordButton->setFlat(false);
        recordButton->setDefault(true);
        recordButton->setStyleSheet(QString::fromUtf8(
                "QPushButton{\n"
                "background-image: url(:/run_environment/graphics/fluid/icon_button_off.png);\n"
                "background-position: center center;}\n"
                "QPushButton::checked {\n"
                "background-image: url(:/run_environment/graphics/fluid/icon_button_on.png);\n"
                "}"));
        recordButton->setChecked((_piano_insert_mode) ? true : false);
        recordlabel = new QLabel(groupBox);
        recordlabel->setObjectName(QString::fromUtf8("recordlabel"));
        recordlabel->setGeometry(QRect(11, 45, 47, 16));
        recordlabel->setText("Record");

        QFont font;
            font.setPointSize(12);

        octaveBox = new QSpinBox(groupBox);
        octaveBox->setObjectName(QString::fromUtf8("octaveBox"));
        octaveBox->setGeometry(QRect(10, 20, 42, 22));
        octaveBox->setFont(font);
        octaveBox->setKeyboardTracking(false);
        octaveBox->setStyleSheet(QString::fromUtf8("color: black"));
        octaveBox->setMinimum(-1);
        octaveBox->setMaximum(5);
        octaveBox->setValue(octave_pos/12-1);
        if(drum_mode) octaveBox->setDisabled(true);

        octavelabel = new QLabel(groupBox);
        octavelabel->setObjectName(QString::fromUtf8("octavelabel"));
        octavelabel->setGeometry(QRect(7, 0, 51, 16));
        octavelabel->setAlignment(Qt::AlignCenter);
        octavelabel->setText("Octave");

        transBox = new QSpinBox(groupBox);
        transBox->setObjectName(QString::fromUtf8("transBox"));
        transBox->setGeometry(QRect(70, 20, 42, 22));
        transBox->setMinimum(-12);
        transBox->setMaximum(12);
        transBox->setValue(trans_pos);
        transBox->setStyleSheet(QString::fromUtf8("color: black"));
        transBox->setFont(font);
        transBox->setKeyboardTracking(false);
        if(drum_mode) transBox->setDisabled(true);

        translabel = new QLabel(groupBox);
        translabel->setObjectName(QString::fromUtf8("translabel"));
        translabel->setGeometry(QRect(67, 0, 51, 16));
        translabel->setAlignment(Qt::AlignCenter);
        translabel->setText("Transpose");

        volumeSlider = new QSlider(groupBox);
        volumeSlider->setObjectName(QString::fromUtf8("volumeSlider"));
        volumeSlider->setGeometry(QRect(15, 92, 91, 22));
        volumeSlider->setMaximum(127);
        volumeSlider->setValue(note_volume);
        volumeSlider->setOrientation(Qt::Horizontal);
        volumeSlider->setTickPosition(QSlider::TicksBelow);
        volumeSlider->setTickInterval(10);
        vollabel = new QLabel(groupBox);
        vollabel->setObjectName(QString::fromUtf8("vollabel"));
        vollabel->setGeometry(QRect(41, 80, 47, 13));
        vollabel->setAlignment(Qt::AlignCenter);
        vollabel->setText("Velocity");
        groupBox->setTitle(QString());

    }

    for(int n = 0; n < 128; n++) {
        piano_keys_time[n] = -1;
        piano_keys_key[n] = 0;
    }

    int key_pos = (octave_pos < 36) ? 36 : octave_pos;
    for(int n = 0; n < 18; n++) {
        if(n < 12) piano_keys_key[key_pos + n] = piano_keys_map[n];
         piano_keys_key[key_pos + n + 12] = piano_keys2_map[n];
    }

    if(!is_rhythm) {
        QObject::connect(recordButton, SIGNAL(clicked()), this, SLOT(record_clicked()));
        QObject::connect(octaveBox, SIGNAL(valueChanged(int)), this, SLOT(octave_key(int)));
        QObject::connect(transBox, SIGNAL(valueChanged(int)), this, SLOT(transpose_key(int)));
        QObject::connect(volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(volume_changed(int)));
    } else {
        myBox = new QMGroupBox(this); // special layer
        myBox->setObjectName("QMGroupBox");
        myBox->setGeometry(0, yr2 + 25 * 5 + 16 - 70, width(), 200);
        myBox->setVisible(true);

    }

    time_updat = new QTimer();
    time_updat->callOnTimeout(this,  [=]()
    {
        static int count =0;

        if(!is_rhythm) {
            count++;
            if(count > 1000/TIMER_TICK) {
                count = 0;
                QD->setFocus();
            }
        }

        if(is_rhythm) {


            if(!rhythmlist) return;

            static int count = 0;
            static int count2 = 0;
            static int count3 = 0;

            if((myBox->isVisible() && myBox->underMouse()) ||
               rhythmlist->underMouse() ||
               rhythmSamplelist->underMouse()) {
                if(myBox->isVisible()) myBox->setVisible(false);
                count = 99; count2 = 0; count3 = 320;
            } else {
                count++;
                if(count == 100) {
                    if(count2 & 32) {
                         if(count3 < 320 && !myBox->isVisible()) myBox->setVisible(true);
                         count3++; if(count3 >= 320) count3 = 320;
                    } else {
                        if(myBox->isVisible()) myBox->setVisible(false);
                   }
                    count = 99;
                    count2++;
                }
            }

            if(play_rhythm_track) {
                if(play_rhythm_track_index == -1) return;
                if(play_rhythm_track_index == -2) {
                    play_rhythm_track = 0;
                    if(ptButton)
                        ptButton->setChecked(false);
                    if(!rhythmlist->isEnabled()) {

                        rhythmlist->setDisabled(false);
                        rhythmSamplelist->setDisabled(false);
                        for(int n = 0; n < rhythmlist->count(); n++)
                            rhythmlist->item(n)->setBackground(QBrush(QColor(0xf0 ,0xa0, 0x50, 0xff)));

                        rhythmlist->setCurrentRow(play_rhythm_track_index_old);
                        return;
                    }
                } else {
                    rhythmlist->item(play_rhythm_track_index)->setBackground(QBrush(QColor(0x70, 0xa0, 0xa0, 0xff)));
                    rhythmlist->setCurrentRow(play_rhythm_track_index);
                }
            }

            if(!play_rhythm_sample) {
                    if(pButton) {
                        if(pButton->isChecked())
                            pButton->setChecked(false);
                }
            }  else {
                // display step line
                this->repaint(163, yr2 + 16, 24 * 32, 24);

            }

            return;
        }


        if(is_playing && _piano_insert_mode && !MidiPlayer::isPlaying()) recordButton->click(); //record_clicked();


        if(I_NEED_TO_UPDATE) {
            I_NEED_TO_UPDATE = 0;
            QD->update();
        }

    });

    recNotes.clear();
    sampleItems.clear();
    _piano_timer = -1;

    time_updat->setSingleShot(false);
    time_updat->start((is_rhythm) ? TIMER_RHYTHM : TIMER_TICK);

}

void MyInstrument::reject() {

    if(is_rhythm) {
        QByteArray sa;
        sa.clear();
        for(int n = 0; n < 128; n++)
            sa.append(map_drum_rhythm_velocity[n] & 127);
        settings->setValue("InstVelocity", sa);
        delete settings;
        settings = NULL;
    } else if(drum_mode) {

        for(int n = 0; n < 11; n++)
            if(DrumImage[n]) delete DrumImage[n];

    }

    if(rhythmlist && is_rhythm) {
       sampleItems.clear();
       push_track_rhythm();
    }

    time_updat->stop();
    delete time_updat;

    if(playing_piano>=0) {
        playing_piano=-1;

        QByteArray a;
        a.clear();
        a.append(0x80 | MidiOutput::standardChannel()); // note off
        a.append(playing_piano);
        a.append((char ) 0);
        MidiOutput::sendCommand(a);

    }

    for(int n = 0; n < 128; n++) {
        if(piano_keys_time[n]>=0) {
            piano_keys_time[n]=-1;

        }
    }
    MidiPlayer::panic();

    while (!recNotes.isEmpty())
        delete recNotes.takeFirst();

    hide();
}

void MyInstrument::RecNote(int note, int startTick, int endTick, int velocity)
{

    data_notes * dnote = new data_notes;
    dnote->note = note;
    dnote->velocity = velocity;
    dnote->start = startTick;
    dnote->end = endTick;
    dnote->chan = (drum_mode) ? 9 : NewNoteTool::editChannel();
    dnote->track = NewNoteTool::editTrack();

    recNotes.append(dnote);

    return;
}

void MyInstrument::RecSave() {

    if(recNotes.isEmpty()) return;

    file->protocol()->startNewAction("Create notes from Piano");

    int last = 0;

    foreach (data_notes * dnote, recNotes) {
        int end = dnote->end/msPerTick;
        if(end > last) last = end;

        NoteOnEvent* on = file->channel(dnote->chan)->
        insertNote(dnote->note, dnote->start/msPerTick,
                   end,
                   dnote->velocity,
                   file->track(dnote->track));

        delete dnote;
        dnote = NULL;

        EventTool::selectEvent(on, false, false);
    }

    file->protocol()->endAction();
    recNotes.clear();

    last+= 5;

    file->setCursorTick(last);
    file->setPauseTick(last);
    int ms = file->msOfTick(file->cursorTick()) - 500;
    if(ms < 0) ms = 0;
    _MW->timeMsChanged(ms, true);

    _piano_timer = -1;

}


void MyInstrument::RecRhythm(int init_time) {

    recNotes.clear();
   _piano_timer = -1;

   int play_rhythm_track_index = 0;
   int play_rhythm_track_init = 1;
   int current_map_rhythm = 0;
   int current_tick_rhythm = 0;
   int max_map_rhythm_track = 8;
   int indx_bpm_rhythm_track = 0;
   int mloop_rhythm_track = (loop_rhythm_track) ? 1 : 0;

   unsigned char map_rhythm_track[10][34];

   int time = (init_time < 0) ? file->msOfTick(file->cursorTick()) : init_time;

   while(play_rhythm_track_index >= 0) {


       if((play_rhythm_track_init || current_map_rhythm >= max_map_rhythm_track) && rhythmlist) {
           play_rhythm_track_init = 0;
           // load datas
           while(1) { // find sample datas
               if(play_rhythm_track_index == -1) {
                   play_rhythm_track_index = 0;
                   current_map_rhythm = 0;
                   max_map_rhythm_track = 1;
               }

               if(current_map_rhythm >= max_map_rhythm_track) {
                   play_rhythm_track_index++;
                   if(play_rhythm_track_index >= rhythmlist->count()) {

                       if(mloop_rhythm_track) {

                           if(mloop_rhythm_track >= loop_rhythm_base) {
                               mloop_rhythm_track = 1;
                               play_rhythm_track_index = -2;
                               break;

                           }

                           mloop_rhythm_track++;
                           play_rhythm_track_index = 0;
                       } else {

                           play_rhythm_track_index = -2;
                           break;
                       }

                       //play_rhythm_track_index = -2;

                       //break;
                   }
               }

               current_map_rhythm = 0;
               indx_bpm_rhythm_track = 16;

               current_tick_rhythm = 0;

               memset(map_rhythm_track, 0, sizeof(unsigned char) * 10 * 34);

               if(rhythmlist->count() <= 0) {

                   play_rhythm_track_index = -2;
                   break;
               }

               QByteArray a = rhythmlist->item(play_rhythm_track_index)->data(Qt::UserRole).toByteArray();
               if(!a.size()) {
                   play_rhythm_track_index = -2;
                   break;
               }

               rhythmData rhythm = rhythmData(a);
               rhythmlist->item(play_rhythm_track_index)->setBackground(QBrush(QColor(0x70, 0xa0, 0xa0, 0xff)));
               rhythmlist->setCurrentRow(play_rhythm_track_index);
               rhythm.get_rhythm_header(&max_map_rhythm_track, &indx_bpm_rhythm_track, NULL);

               if(default_time || (indx_bpm_rhythm_track & 128))
                   indx_bpm_rhythm_track = indx_bpm_rhythm & 127;

               if(max_map_rhythm_track <= 0)
                   continue; // empty

               for(int n = 0; n < 10; n++) {
                   rhythm.get(n, &map_rhythm_track[n][0]);
               }

               break;
           }

       }

       if(play_rhythm_track_index >= 0) {

           for(int n = 0; n < 10; n++) {

               int key = -1, vel;
               int play = 0;

               play |= (map_rhythm_track[n][current_map_rhythm + 2] == 1) && (current_tick_rhythm == 0);
               play |= (map_rhythm_track[n][current_map_rhythm + 2] == 2) && (current_tick_rhythm == 1);
               play |= (map_rhythm_track[n][current_map_rhythm + 2] == 3) &&
                       (current_tick_rhythm == 0 || current_tick_rhythm == 1);

               key = map_rhythm_track[n][0];
               vel = map_rhythm_track[n][1];

               if(key >= 0) {
                   if(play) {

                       int time2 = ((1000 * 64 / (indx_bpm_rhythm_track & 127))/16) -1;
                       if(time2 > 100) time2 = time + 100; else time2+= time;

                       RecNote(key, time, time2, vel);
                   }
               }// play
           } // n

           current_tick_rhythm++;
           if(current_tick_rhythm >= 2) {
               current_tick_rhythm = 0;
               current_map_rhythm++;
           }

       } else
           break;


       time += ((1000 * 64 / (indx_bpm_rhythm_track & 127))/16);
    }

}

static QByteArray *pop_push_track_rhythm = NULL;

void MyInstrument::exit_and_clean_MyInstrument() {

    if(Play_Thread::rhythmThread) {
        Play_Thread::rhythmThread->loop = false;
        Play_Thread::rhythmThread->wait();
        delete Play_Thread::rhythmThread;
    }

    Play_Thread::rhythmThread = NULL;

    if(pop_push_track_rhythm) {
        pop_push_track_rhythm->clear();
        delete pop_push_track_rhythm;
    }

    pop_push_track_rhythm = NULL;

}

void MyInstrument::push_track_rhythm(QFile *file, int mode) {


    if(!rhythmlist)
        return;

    if(!file && (mode >= 128))
        return;

    if(!file) {

        if(!pop_push_track_rhythm)
            pop_push_track_rhythm = new QByteArray();
        else
            pop_push_track_rhythm->clear();
    }

    QDataStream* stream;

    if(file)
        stream = new QDataStream(file);
    else
        stream = new QDataStream(pop_push_track_rhythm, QIODevice::WriteOnly);


    stream->setByteOrder(QDataStream::LittleEndian);
    quint32 tempByte32;

    if(mode < 128) {
        (*stream) << (quint8) 'D';
        (*stream) << (quint8) 'T';
        (*stream) << (quint8) 'R';
        (*stream) << (quint8) '0';
        tempByte32 = rhythmlist->count();
        (*stream) << tempByte32;
    }

    for(int n = 0; n < ((mode >= 128) ? 1 : rhythmlist->count()); n++) {

        QByteArray a;

        if(mode >= 128) a = rhythmSamplelist->item(0)->data(Qt::UserRole).toByteArray();
            else
                a = rhythmlist->item(n)->data(Qt::UserRole).toByteArray();

        (*stream) << (quint8) 'D';
        (*stream) << (quint8) 'S';
        (*stream) << (quint8) 'R';
        (*stream) << (quint8) '0';
        tempByte32 = a.size();
        (*stream) << tempByte32;
        for(int n = 0; n < a.count(); n++)
            (*stream) << (quint8) a.at(n);
    }

    if(mode < 128) {
        (*stream) << (quint8) 'E';
        (*stream) << (quint8) 'O';
        (*stream) << (quint8) 'F';
        (*stream) << (quint8)  (indx_bpm_rhythm & 127);
    }

}


void MyInstrument::pop_track_rhythm(QFile *file, int mode) {

    QMListWidget *rh;
    int nrow = 0;

    if(mode < 128) {
        if(mode != 1)
            rh = rhythmlist;
        else {
            rh = rhythmSamplelist;
            if(!rh) return;
        }

        if(mode == 1 && !file) return;

        if((!pop_push_track_rhythm && !file) || !rh)
            return;
    } else {
        if(!file)
            return;
    }

    QDataStream* stream;

    if(file)
        stream = new QDataStream(file);
    else
        stream = new QDataStream(pop_push_track_rhythm, QIODevice::ReadOnly);

    stream->setByteOrder(QDataStream::LittleEndian);

    quint8 temp8;
    quint32 nsamples = 0;
    quint32 size_sample;

    if(mode < 128) {
        (*stream) >> temp8;
        if(temp8 != 'D')
            return;
        (*stream) >> temp8;
        if(temp8 != 'T')
            return;
        (*stream) >> temp8;
        if(temp8 != 'R')
            return;
        (*stream) >> temp8;
        if(temp8 != '0')
            return;
    
        (*stream) >> nsamples;
    
        if(mode == 0) {
            rh->clear();
        } else {
            for(int m = rhythmSamplelist->count() - 1 ; m >= 0 ; m--) {
                if(rhythmSamplelist->item(m)->flags() & Qt::ItemIsUserCheckable) {
                    delete rhythmSamplelist->takeItem(m);
                }
            }
        }
    } else {
        rh = rhythmSamplelist;
    }

    for(int n = 0; n < ((mode >= 128) ? 1 : (int) nsamples); n++) {

        QByteArray a;

        (*stream) >> temp8;
        if(temp8 != 'D')
            return;
        (*stream) >> temp8;
        if(temp8 != 'S')
            return;
        (*stream) >> temp8;
        if(temp8 != 'R')
            return;
        (*stream) >> temp8;
        if(temp8 != '0')
            return;

        (*stream) >> size_sample;

        a.clear();
        for(int n = 0; n < (int) size_sample; n++) {
            (*stream) >> temp8;
            a.append(temp8);
        }

        {
            QListWidgetItem* item = new QListWidgetItem();

            rhythmData rhythm = rhythmData(a);
            item->setText("TempName");

            if(mode < 128) {
                if(mode != 1)
                    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                else
                    item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);

                item->setData(Qt::UserRole, rhythm.save());
                if(mode!= 1) rh->addItem(item);
                else {
                    int fl = 0;
                    for(int n = 0; n < rh->count(); n++) {
                        if(rh->item(n)->data(Qt::UserRole).toByteArray() ==
                               item->data(Qt::UserRole).toByteArray()) {
                            fl = 1; break; // find if it exist...
                        }
                    }
                    if(!fl) {
                        rh->insertItem(nrow, item);
                        nrow++;
                    } else delete item;
                }
            } else {
                item->setData(Qt::UserRole, rhythm.save());
                delete rh->takeItem(0);
                rh->insertItem(0, item);
            }


        }

    }

    if(mode < 128) {

        (*stream) >> temp8;
        if(temp8 == 'E') {

            (*stream) >> temp8;

            if(temp8 == 'O') {

                (*stream) >> temp8;

                if(temp8 == 'F') {

                    (*stream) >> temp8;
                    indx_bpm_rhythm = ((int) temp8) & 127;
                }
            }
        }
    }

    rh->setCurrentRow(0);
    rh->update();
    rh->paintEvent(NULL);

    if(mode < 128) {
        if(mode != 1) {

            // delete old samples
            for(int m = rhythmSamplelist->count() - 1 ; m > 0 ; m--) {
                if(rhythmSamplelist->item(m)->flags() & Qt::ItemIsUserCheckable) {
                    delete rhythmSamplelist->takeItem(m);
                }
            }

            int insert = 1;
            for(int n = 0; n < rhythmlist->count(); n++) {

                int flag = 0;
                QByteArray a = rhythmlist->item(n)->data(Qt::UserRole).toByteArray();
                for(int m = 1; m < rhythmSamplelist->count(); m++) {
                    QByteArray b = rhythmSamplelist->item(m)->data(Qt::UserRole).toByteArray();
                    if(a == b) {
                       flag = 1; break;
                    }
                }

                if(!flag) {
                    QListWidgetItem* item2 = new QListWidgetItem();

                    item2->setText("TempName");
                    item2->setFlags(item2->flags() | Qt::ItemIsUserCheckable);
                    item2->setData(Qt::UserRole, a);
                    rhythmSamplelist->insertItem(insert, item2);
                    insert++;
                }
            }

            rhythmSamplelist->update();

            if(rhythmlist->count()) {

                current_map_rhythm = 0; // EDIT
                current_tick_rhythm = 0;
                play_rhythm_sample = 0;
                pButton->setChecked(false);
                rhythmlist->setCurrentRow(0);

                memset(map_rhythm, 0, sizeof(unsigned char) * 10 * 34);
                QByteArray a = rhythmlist->item(0)->data(Qt::UserRole).toByteArray();
                rhythmData rhythm = rhythmData(a);

                titleEdit->setText(rhythm.getName());
                int ticks;
                rhythm.get_rhythm_header(&max_map_rhythm, &ticks, NULL);
                if(!(ticks & 128)) {
                     indx_bpm_rhythm = ticks;
                     fix_time_sample = 1;
                     fixSpeedBox->setChecked(true);
                } else {
                     fix_time_sample = 0;
                     fixSpeedBox->setChecked(false);
                }


                for(int n = 0; n < 10; n++) {
                    rhythm.get(n, &map_rhythm[n][0]);
                    for(int m = 0; m < drumBox[n]->count(); m++) {
                        if(drumBox[n]->itemData(m).toInt() == rhythm.get_drum_note(n))
                            drumBox[n]->setCurrentIndex(m);
                    }

                }
                save_rhythm_edit();
                QD->update();
            }
        } else {

            // insert editor entry
            {
                QListWidgetItem* item = new QListWidgetItem();
                item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                rhythmData rhythm = rhythmData();

                rhythm.setName(titleEdit->text());

                rhythm.set_rhythm_header(max_map_rhythm, indx_bpm_rhythm | ((fix_time_sample) ? 0 : 128), 0);
                for(int n = 0; n < 10; n++)
                    rhythm.insert(n, &map_rhythm[n][0]);

                item->setData(Qt::UserRole, rhythm.save());
                //rhythmSamplelist->addItem(item);
                rhythmSamplelist->insertItem(0, item);
                rhythmSamplelist->setCurrentRow(0);
                rhythmSamplelist->update();
            }

        }
    } else { // sample
        memset(map_rhythm, 0, sizeof(unsigned char) * 10 * 34);
        QByteArray a = rhythmSamplelist->item(0)->data(Qt::UserRole).toByteArray();
        rhythmData rhythm = rhythmData(a);

        titleEdit->setText(rhythm.getName());
        int ticks;
        rhythm.get_rhythm_header(&max_map_rhythm, &ticks, NULL);
        if(!(ticks & 128)) {
             indx_bpm_rhythm = ticks;
             fix_time_sample = 1;
             fixSpeedBox->setChecked(true);
        } else {
             fix_time_sample = 0;
             fixSpeedBox->setChecked(false);
        }

        for(int n = 0; n < 10; n++) {
            rhythm.get(n, &map_rhythm[n][0]);
            for(int m = 0; m < drumBox[n]->count(); m++) {
                if(drumBox[n]->itemData(m).toInt() == rhythm.get_drum_note(n))
                    drumBox[n]->setCurrentIndex(m);
            }

        }
        save_rhythm_edit();
        QD->update();

        //////////
    }

}

void MyInstrument::new_track() {

    if(pop_push_track_rhythm) {
        pop_push_track_rhythm->clear();
        delete pop_push_track_rhythm;
    }
    pop_push_track_rhythm = NULL;

    play_rhythm_track = 0;
    play_rhythm_sample = 0;
    record_rhythm_track = 0;
    pButton->setChecked(false);
    ptButton->setChecked(false);
    //rButton->setChecked(false);
    QThread::msleep(100);
    rhythmlist->setDisabled(false);
    rhythmSamplelist->setDisabled(false);

    // delete old samples
    for(int m = rhythmSamplelist->count() - 1 ; m > 0 ; m--) {
        if(rhythmSamplelist->item(m)->flags() & Qt::ItemIsUserCheckable) {
            delete rhythmSamplelist->takeItem(m);
        }
    }

    // delete old samples
    for(int m = rhythmlist->count() - 1 ; m >= 0 ; m--) {
            delete rhythmlist->takeItem(m);
    }

    _init_rhythm();
    pButton->setChecked((play_rhythm_sample) ? true : false);
    lButton->setChecked((loop_rhythm_sample) ? true : false);
    titleEdit->setText(QString("Sample"));
    ltButton->setChecked((loop_rhythm_track) ? true : false);
    //rButton->setChecked((record_rhythm_track) ? true : false);
    fixSpeedBox->setChecked(fix_time_sample ? true : false);
    loopBox->setValue(loop_rhythm_base);
    save_rhythm_edit();

    for(int n = 0; n < 10; n++)
        map_rhythm[n][1]
                = map_rhythmData[n][1]
                = map_drum_rhythm_velocity[map_rhythm[n][0]];

    for(int n = 0; n < 10; n++) {
        for(int m = 0; m < drumBox[n]->count(); m++) {
            if(drumBox[n]->itemData(m).toInt() == map_rhythm[n][0])
                drumBox[n]->setCurrentIndex(m);
        }
    }


    update();

}

void MyInstrument::load_track() {

    QString appdir = settings->value("drum_path").toString();
    if(appdir.isEmpty())
        appdir = QDir::homePath();

    play_rhythm_track = 0;
    play_rhythm_sample = 0;
    record_rhythm_track = 0;
    pButton->setChecked(false);
    ptButton->setChecked(false);
    //rButton->setChecked(false);
    QThread::msleep(100);
    rhythmlist->setDisabled(false);
    rhythmSamplelist->setDisabled(false);

    QString newPath = QFileDialog::getOpenFileName(this, "Open Drum Track file",
        appdir, "Drum Files (*.drum)");
    if(newPath.isEmpty()) {
        return; // canceled
    }

    QFile *drum = new QFile(newPath);
    if(drum && drum->open(QIODevice::ReadOnly)) {

        pop_track_rhythm(drum);
        for(int n = 0; n < 10; n++)
            map_rhythm[n][1]
                    = map_rhythmData[n][1]
                    = map_drum_rhythm_velocity[map_rhythm[n][0]];
        drum->close();
        settings->setValue("drum_path", QFileInfo(newPath).absoluteDir().absolutePath());

    }  

}

void MyInstrument::save_track() {

    play_rhythm_track = 0;
    play_rhythm_sample = 0;
    record_rhythm_track = 0;
    pButton->setChecked(false);
    ptButton->setChecked(false);
    //rButton->setChecked(false);
    QThread::msleep(100);
    rhythmlist->setDisabled(false);
    rhythmSamplelist->setDisabled(false);

    QString appdir;

    appdir = settings->value("drum_path_sample").toString();
    if(appdir.isEmpty())
        appdir = QDir::homePath();

    appdir+= "/default.drum";

    QString savePath = QFileDialog::getSaveFileName(this, "Save Drum Track file",
                                                    appdir, "Drum Files (*.drum)");


    if(savePath.isEmpty()) {
        QMessageBox::information(this, "Save Drum Track file", "save aborted!");
        return; // canceled
    }

    if(!savePath.endsWith(".drum")) {
        savePath += ".drum";
    }

    settings->setValue("drum_path", QFileInfo(savePath).absoluteDir().absolutePath());

    QFile *drum = new QFile(savePath);
    if(drum && drum->open(QIODevice::WriteOnly | QIODevice::Truncate)) {

        push_track_rhythm(drum);
        drum->close();

    }


}

void MyInstrument::load_sample() {

    QString appdir = settings->value("drum_path_sample").toString();
    if(appdir.isEmpty())
        appdir = settings->value("drum_path").toString();
    if(appdir.isEmpty())
        appdir = QDir::homePath();

    play_rhythm_track = 0;
    play_rhythm_sample = 0;
    record_rhythm_track = 0;
    pButton->setChecked(false);
    ptButton->setChecked(false);

    QThread::msleep(100);
    rhythmlist->setDisabled(false);
    rhythmSamplelist->setDisabled(false);

    QString newPath = QFileDialog::getOpenFileName(this, "Open Drum Sample file",
        appdir, "Drum Files (*.sdrm)");
    if(newPath.isEmpty()) {
        return; // canceled
    }

    QFile *drum = new QFile(newPath);
    if(drum && drum->open(QIODevice::ReadOnly)) {

        pop_track_rhythm(drum, 128);
        drum->close();
        settings->setValue("drum_path_sample", QFileInfo(newPath).absoluteDir().absolutePath());

    }

}

void MyInstrument::save_sample() {

    QString appdir;

    appdir = settings->value("drum_path_sample").toString();
    if(appdir.isEmpty())
        appdir = QDir::homePath();

    appdir+= "/" + titleEdit->text() + ".sdrm";

    play_rhythm_track = 0;
    play_rhythm_sample = 0;
    record_rhythm_track = 0;
    pButton->setChecked(false);
    ptButton->setChecked(false);
    //rButton->setChecked(false);
    QThread::msleep(100);
    rhythmlist->setDisabled(false);
    rhythmSamplelist->setDisabled(false);

    QString savePath = QFileDialog::getSaveFileName(this, "Save Drum Sample file",
                                                    appdir, "Drum Files (*.sdrm)");

    if(savePath.isEmpty()) {
        QMessageBox::information(this, "Save Drum Sample file", "save aborted!");
        return; // canceled
    }

    if(!savePath.endsWith(".sdrm")) {
        savePath += ".sdrm";
    }

    settings->setValue("drum_path_sample", QFileInfo(savePath).absoluteDir().absolutePath());

    QFile *drum = new QFile(savePath);
    if(drum && drum->open(QIODevice::WriteOnly | QIODevice::Truncate)) {

        save_rhythm_edit();

        push_track_rhythm(drum, 128);

        drum->close();

    }

}

void MyInstrument::copy_samples() {

    QList<QListWidgetItem *> items = rhythmlist->selectedItems();

    if(items.count()) {
        sampleItems.clear();
        for(int m = 0 ; m < items.count(); m++) {
            sampleItems.append(items.at(m));
        }
    }
}

void MyInstrument::paste_samples() {

    int row = rhythmlist->currentRow();

    if(row < 0)
        return;

    if(sampleItems.count()) {

        for(int m = sampleItems.count() - 1 ; m >= 0 ; m--) {
            QListWidgetItem *newI = new  QListWidgetItem();
            *newI = *sampleItems.at(m);
            rhythmlist->insertItem(row + 1, newI);
        }
    }

    rhythmlist->update();
}

static int dx[12]={0, 14, 22, 36, 44, 66, 80, 88, 102, 110, 124, 132};

void MyInstrument::paintEvent(QPaintEvent* /*event*/) {
    QPainter* painter = new QPainter(this);
    QFont font = painter->font();
    font.setPixelSize(12);
    painter->setFont(font);
    painter->setClipping(false);

    if(is_rhythm) {
        paintRhythm(painter);
        delete painter;
        return;
    }


    int key_pos = (octave_pos >= 48) ? 48 : octave_pos+12;

    int yi= 32;

    painter->fillRect(0, 0, width(), height(), QColor(0x20,0x27,0x20,0xff));

    QBrush brush(Qt::darkGray, Qt::Dense4Pattern);

    painter->setBrush(brush);
    painter->setPen(Qt::darkGray);
    painter->drawRoundedRect(4, 4, 156, 24, 8, 8);

    painter->drawRoundedRect(21*22+ 163-6-((drum_mode) ? 156+16+4+10 : 0), 4, ((drum_mode) ? (156+16+10)*2 : 156+16+4), 24, 8, 8);

    int n;

    //painter->setPen(Qt::darkGray);
    painter->setPen(Qt::black);
    painter->setBrush(Qt::white);

    if(!drum_mode) {

        painter->drawRect(4, yi+1, 22 * (7*5+1), 100);

        painter->setBrush(Qt::NoBrush);

    // white
        painter->setPen(Qt::black);
        for(n = 0; n < 61; n++) {
            int m= n % 12;
            int x= (n/12) * (22 * 7) + dx[m];
            if(!(m==1 || m==3 || m==6 || m==8 || m==10)) {
               painter->drawRect(x+4, yi, 21, 102);
               if(piano_keys_time[key_pos + n - 12] >=0)
                   painter->fillRect(x+5, yi+2, 21-1, 100, QColor(0x80,0x10,0x10, 0xb0));
            }
        }

        // black

        painter->setBrush(Qt::black);
        painter->setPen(QPen(QColor(0x404040), 2, Qt::SolidLine));

        for(n = 0; n < 61; n++) {
            int m= n % 12;
            int x= (n/12) * (22 * 7) + dx[m];
            if(m==1 || m==3 || m==6 || m==8 || m==10) {
                painter->drawRect(x+4, yi+2, 14, 60);
                if(piano_keys_time[key_pos + n - 12] >=0)
                    painter->fillRect(x+5, yi+2+1, 14-2, 60-2, QColor(0xf0, 0x30 ,0x30, 0x60));
            }
        }

        key_pos = 48;
        // display keys
        char cad[2];
        cad[1]=0;
        for(n = 0; n < 61; n++) {
            int m= n % 12;
            int x= (n/12) * (22 * 7) + dx[m];

            cad[0] = piano_keys_key[-12 + key_pos + n];
            if(m==1 || m==3 || m==6 || m==8 || m==10) {
                painter->setPen(Qt::white);
                painter->drawText(x+7, yi+40, cad);
            } else {
                painter->setPen(Qt::black);
                painter->drawText(x+11, yi+80, cad);
            }

        }
    } else { // drum
        painter->drawRect(4, yi+1, 22 * (7*5+1), 172);

        int yy = yi + 1 + 4;
        int xx = 4 + 4;

        painter->setBrush(Qt::NoBrush);
        font.setPixelSize(16);
        painter->setFont(font);

        if(piano_keys_time[42] >=0)
            painter->fillRect(xx, yy, 64, 64+16 , QColor(0xf0, 0x30 ,0x30, 0x60));
        painter->drawImage(QRect(xx, yy, 64, 64), *DrumImage[0]);
        painter->drawRect(xx, yy, 64, 64+16);
        painter->drawText(xx+16+12, yy + 64+4, "1"); // closed hi-hat
        xx+= 68;

        if(piano_keys_time[44] >=0)
            painter->fillRect(xx, yy, 64, 64+16 , QColor(0xf0, 0x30 ,0x30, 0x60));
        painter->drawImage(QRect(xx, yy, 64, 64), *DrumImage[1]);
        painter->drawRect(xx, yy, 64, 64+16);
        painter->drawText(xx+16+12, yy + 64+4, "2"); // pedal hi-hat
        xx+= 68;

        if(piano_keys_time[46] >=0)
            painter->fillRect(xx, yy, 64, 64+16 , QColor(0xf0, 0x30 ,0x30, 0x60));
        painter->drawImage(QRect(xx, yy, 64, 64), *DrumImage[2]);
        painter->drawRect(xx, yy, 64, 64+16);
        painter->drawText(xx+16+12, yy + 64+4, "3"); // open hi-hat
        xx+= 68;

        if(piano_keys_time[49] >=0)
            painter->fillRect(xx, yy, 64, 64+16 , QColor(0xf0, 0x30 ,0x30, 0x60));
        painter->drawImage(QRect(xx, yy, 64, 64), *DrumImage[3]);
        painter->drawRect(xx, yy, 64, 64+16);
        painter->drawText(xx+16+12, yy + 64+4, "4"); // crash cymbal
        xx+= 68;

        if(piano_keys_time[57] >=0)
            painter->fillRect(xx, yy, 64, 64+16 , QColor(0xf0, 0x30 ,0x30, 0x60));
        painter->drawImage(QRect(xx, yy, 64, 64), DrumImage[3]->mirrored(true, false));
        painter->drawRect(xx, yy, 64, 64+16);
        painter->drawText(xx+16+12, yy + 64+4, "5"); // crash cymbal2
        xx+= 68;

        if(piano_keys_time[52] >=0)
            painter->fillRect(xx, yy, 64, 64+16 , QColor(0xf0, 0x30 ,0x30, 0x60));
        painter->drawImage(QRect(xx, yy, 64, 64), *DrumImage[4]);
        painter->drawRect(xx, yy, 64, 64+16);
        painter->drawText(xx+16+12, yy + 64+4, "6"); // chinese cymbal
        xx+= 68;

        if(piano_keys_time[51] >=0)
            painter->fillRect(xx, yy, 64, 64+16 , QColor(0xf0, 0x30 ,0x30, 0x60));
        painter->drawImage(QRect(xx, yy, 64, 64), *DrumImage[4]);
        painter->drawRect(xx, yy, 64, 64+16);
        painter->drawText(xx+16+12, yy + 64+4, "7"); // ride cymbal
        xx+= 68;

        if(piano_keys_time[55] >=0)
            painter->fillRect(xx, yy, 64, 64+16 , QColor(0xf0, 0x30 ,0x30, 0x60));
        painter->drawImage(QRect(xx, yy, 64, 64), DrumImage[4]->mirrored(true, false));
        painter->drawRect(xx, yy, 64, 64+16);
        painter->drawText(xx+16+12, yy + 64+4, "8"); // splash cymbal
        xx+= 68;

        if(piano_keys_time[53] >=0)
            painter->fillRect(xx, yy, 64, 64+16 , QColor(0xf0, 0x30 ,0x30, 0x60));
        QPoint p[4];
        p[0]= QPoint(xx + 6, yy + 10);
        p[1]= QPoint(xx+2 , yy + 14);
        p[2]= QPoint(xx + 20 , yy + 24);
        p[3]= p[0];
        painter->setBrush(Qt::darkGray);
        painter->drawPolygon(p, 4);
        painter->setBrush(Qt::NoBrush);
        painter->drawImage(QRect(xx, yy, 64, 64), DrumImage[4]->mirrored(true, false));
        painter->drawRect(xx, yy, 64, 64+16);
        painter->drawText(xx+16+12, yy + 64+4, "9"); // ride bell
        xx+= 68;

        if(piano_keys_time[54] >=0)
            painter->fillRect(xx, yy, 64, 64+16 , QColor(0xf0, 0x30 ,0x30, 0x60));
        painter->drawImage(QRect(xx, yy, 64, 64), *DrumImage[5]);
        painter->drawRect(xx, yy, 64, 64+16);
        painter->drawText(xx+16+12, yy + 64+4, "0"); // tambourine
        xx+= 68;

        yy+= 84;
        xx = 4 + 4;

        if(piano_keys_time[38] >=0)
            painter->fillRect(xx, yy, 64, 64+16 , QColor(0xf0, 0x30 ,0x30, 0x60));
        painter->drawImage(QRect(xx, yy-4, 64, 64), *DrumImage[6]);
        painter->drawRect(xx, yy, 64, 64+16);
        painter->drawText(xx+20, yy + 64-2, "QW"); // Acoustic Snare
        painter->drawText(xx+20, yy + 64-2+16, "AS");
        xx+= 68;



        if(piano_keys_time[35] >=0)
            painter->fillRect(xx, yy, 64, 64+16 , QColor(0xf0, 0x30 ,0x30, 0x60));
        painter->drawImage(QRect(xx, yy-4, 64, 64), *DrumImage[7]);
        painter->drawRect(xx, yy, 64, 64+16);
        painter->drawText(xx+20, yy + 64-2+16, "ZX"); // Acoustic Bass Drum
        xx+= 68;

        if(piano_keys_time[36] >=0)
            painter->fillRect(xx, yy, 64, 64+16 , QColor(0xf0, 0x30 ,0x30, 0x60));
        painter->drawImage(QRect(xx, yy-4, 64, 64), *DrumImage[8]);
        painter->drawRect(xx, yy, 64, 64+16);
        painter->drawText(xx+20, yy + 64-2+16, "CV"); // Bass Drum
        xx+= 68;

        if(piano_keys_time[40] >=0)
            painter->fillRect(xx, yy, 64, 64+16 , QColor(0xf0, 0x30 ,0x30, 0x60));
        painter->drawImage(QRect(xx, yy-4, 64, 64), *DrumImage[9]);
        painter->drawRect(xx, yy, 64, 64+16);
        painter->drawText(xx+20, yy + 64-2, "ER"); // Electric snare
        painter->drawText(xx+20, yy + 64-2+16, "DF");
        xx+= 68;

        if(piano_keys_time[41] >=0)
            painter->fillRect(xx, yy, 64, 64+16 , QColor(0xf0, 0x30 ,0x30, 0x60));
        painter->drawImage(QRect(xx, yy-4, 64, 64), *DrumImage[10]);
        painter->drawRect(xx, yy, 64, 64+16);
        painter->drawText(xx+20, yy + 64-2+16, "TG"); // Low Floor Tom
        xx+= 68;

        if(piano_keys_time[43] >=0)
            painter->fillRect(xx, yy, 64, 64+16 , QColor(0xf0, 0x30 ,0x30, 0x60));
        painter->drawImage(QRect(xx, yy-4, 64, 64), *DrumImage[10]);
        painter->drawRect(xx, yy, 64, 64+16);
        painter->drawText(xx+20, yy + 64-2+16, "YH"); // High Floor Tom
        xx+= 68;

        if(piano_keys_time[45] >=0)
            painter->fillRect(xx, yy, 64, 64+16 , QColor(0xf0, 0x30 ,0x30, 0x60));
        painter->drawImage(QRect(xx, yy-4, 64, 56), *DrumImage[10]);
        painter->drawRect(xx, yy, 64, 64+16);
        painter->drawText(xx+20, yy + 64-2+16, "UJ"); // Low Tom
        xx+= 68;

        if(piano_keys_time[47] >=0)
            painter->fillRect(xx, yy, 64, 64+16 , QColor(0xf0, 0x30 ,0x30, 0x60));
        painter->drawImage(QRect(xx, yy-4, 64, 56), *DrumImage[10]);
        painter->drawRect(xx, yy, 64, 64+16);
        painter->drawText(xx+20, yy + 64-2+16, "IK"); // Low Mid Floor Tom
        xx+= 68;

        if(piano_keys_time[48] >=0)
            painter->fillRect(xx, yy, 64, 64+16 , QColor(0xf0, 0x30 ,0x30, 0x60));
        painter->drawImage(QRect(xx, yy-4, 64, 48), *DrumImage[10]);
        painter->drawRect(xx, yy, 64, 64+16);
        painter->drawText(xx+20, yy + 64-2+16, "OL"); // Hi Mid Tom
        xx+= 68;

        if(piano_keys_time[50] >=0)
            painter->fillRect(xx, yy, 64, 64+16 , QColor(0xf0, 0x30 ,0x30, 0x60));
        painter->drawImage(QRect(xx, yy-4, 64, 48), *DrumImage[10]);
        painter->drawRect(xx, yy, 64, 64+16);
        painter->drawText(xx+16+12, yy + 64-2+16, "P"); // High Tom
        xx+= 68;


    }

    delete painter;
}

void MyInstrument::paintRhythm(QPainter* painter) {

    QFont font = painter->font();
    font.setPixelSize(12);

    QBrush brush(QColor(0xa0, 0xc0, 0xc0, 0xff));

    QLinearGradient linearGrad(QPointF(0, 0), QPointF(width(), 1));

    //////////////////

    if(!pixmapButton[0]) { // build pads buttons for editor

        QLinearGradient linearGrad(QPointF(0, 0), QPointF(22, 16));

        for(int n = 0; n < 12; n++) {
            pixmapButton[n] = new QPixmap(24, 18);
            pixmapButton[n]->fill(Qt::transparent);
            QPainter pixpainter(pixmapButton[n]);

            int type = n & 3;

            linearGrad.setColorAt(0, QColor(0x30, 0x30, 0x30));
            linearGrad.setColorAt(0.5, QColor(0x20, 0x40, 0x60));
            linearGrad.setColorAt(1, QColor(0x20, 0x20, 0x60));

            if(n < 8) {
                switch(type) {
                    case 0:
                    case 2:
                    case 3:
                        if(!(n & 4)) {

                            linearGrad.setColorAt(0, QColor(0x20, 0x30, 0x50));
                            linearGrad.setColorAt(0.5, QColor(0x20, 0x45, 0x60));
                            linearGrad.setColorAt(1, QColor(0x20, 0x45, 0x70));

                        } else {

                            linearGrad.setColorAt(0, QColor(0x30, 0x30, 0x60));
                            linearGrad.setColorAt(0.5, QColor(0x70, 0x70, 0xa0));
                            linearGrad.setColorAt(1, QColor(0x70, 0x70, 0xa0));

                        }
                    break;
                    case 1:
                        linearGrad.setColorAt(0, Qt::green);
                        linearGrad.setColorAt(0.5, QColor(0xc0, 0xf0, 0x0));
                        linearGrad.setColorAt(1, Qt::yellow);
                    break;
                }
        } else {
                if(n == 8) {
                    linearGrad.setColorAt(0, QColor(0x30, 0x70, 0x70));
                    linearGrad.setColorAt(0.5, QColor(0x30, 0xa0, 0xa0));
                    linearGrad.setColorAt(1, QColor(0x30, 0xa0, 0xa0));
                } else if(n == 9) {
                    linearGrad.setColorAt(0, QColor(0x30, 0xa0, 0xa0));
                    linearGrad.setColorAt(0.5, Qt::cyan);
                    linearGrad.setColorAt(1, Qt::cyan);
                } else if(n == 10) {
                    linearGrad.setColorAt(0, QColor(0x30, 0x70, 0x30));
                    linearGrad.setColorAt(0.5, QColor(0x70, 0xa0, 0x70));
                    linearGrad.setColorAt(1, QColor(0x70, 0xa0, 0x70));
                } else if(n == 11) {
                    linearGrad.setColorAt(0, QColor(0xc0, 0xc0, 0xc0));
                    linearGrad.setColorAt(0.5, QColor(0xf0, 0xf0, 0xc0));
                    linearGrad.setColorAt(1, QColor(0xf0, 0xf0, 0xc0));
                }
        }

            QBrush brush(linearGrad);

            pixpainter.setBrush(brush);

            if(n == 8 || n == 9) {
                pixpainter.setPen(QColor(0x30, 0x30, 0x30));

                pixpainter.drawRoundedRect(0, 0, 22, 16, 2, 2);
            } else if(n == 10 || n == 11) {
                pixpainter.setPen(QColor(0x30, 0x80, 0x30));

                pixpainter.drawRoundedRect(0, 0, 22, 16, 2, 2);
            } else {

                pixpainter.setPen(Qt::transparent);

                pixpainter.drawRoundedRect(0, 0, 22, 16, 2, 2);


                if(type == 2 || type == 3) {
                    linearGrad.setColorAt(0, Qt::green);
                    linearGrad.setColorAt(0.5, QColor(0xc0,0xf0,0x0));
                    linearGrad.setColorAt(1, Qt::yellow);
                }

                if(type == 2)
                    pixpainter.fillRect(11, 0, 11, 16, linearGrad);
                if(type == 3) {
                    pixpainter.fillRect(1, 0, 7, 16, linearGrad);
                    pixpainter.fillRect(15, 0, 7, 16, linearGrad);
                }

                pixpainter.setBrush(Qt::NoBrush);

                if(!(n & 4))
                    pixpainter.setPen(QColor(0x30, 0x30, 0x80));
                else
                    pixpainter.setPen(QColor(0x30, 0x30, 0x30));

                pixpainter.drawRoundedRect(0, 0, 22, 16, 2, 2);
            }
        }


    }

    //////////////////

    linearGrad.setColorAt(0, QColor(0xa0, 0xc0, 0xc0));
    linearGrad.setColorAt(0.5, QColor(0xc0, 0xd0, 0xe0));
    linearGrad.setColorAt(1, QColor(0xa0, 0xc0, 0xc0));
    QBrush brush2(linearGrad);

    painter->setPen(QColor(0x70, 0xa0, 0xa0, 0xff));
    painter->setBrush(brush2);
    painter->drawRoundedRect(12, yr + 8, 160 + 32 * 24, yr2  + 19  + 25 * 4 - yr - 16,
                             8, 8);

    painter->drawRoundedRect(12, yr2 + 19  + 25 * 4, 160 + 32 * 24, 25 * 6 + 8,
                             8, 8);

    brush.setColor(Qt::black);

    for(int n = 0; n < 10; n++) {
        for(int m = 0; m < 32; m++) {

            int type = (map_rhythm[n][m + 2] & 3);
            if((m + 1) <= max_map_rhythm) type|= 4;
            // draw pads for drum buttons
            painter->drawPixmap(163 + m * 24,
                                yr + 18 + 25 * n,
                                *pixmapButton[type]);

        }
    }

    for(int m = 0; m < 32; m++) {

        brush.setColor(QColor(0x30, 0xa0, 0xa0));

        int index = (play_rhythm_sample && m == current_map_rhythm) ? 11 : 9;
        index = ((m + 1) <= max_map_rhythm) ? index : 8;
        painter->drawPixmap(163 + m * 24, yr2 + 16, *pixmapButton[index]);
        painter->setPen(Qt::black);
        painter->drawText(163 + m * 24 + 4 + 4 * (m < 9), yr2 + 16 + 12,
                          QString().setNum(m + 1));
    }

    painter->setPen(Qt::black);
    painter->setBackground(Qt::white);
    painter->setBackgroundMode(Qt::OpaqueMode);
    painter->drawText(163 - 42, yr2 + 16 + 12, "Steps:");
    painter->setBackgroundMode(Qt::TransparentMode);

    for(int m = 0; m < 32; m++) {

        painter->drawPixmap(163 + m * 24, yr2 + 16 + 25 * 2, *pixmapButton[((m + 1) == (indx_bpm_rhythm & 127)) ? 11 : 10]);
        painter->drawPixmap(163 + m * 24, yr2 + 16 + 25 * 2 + 20, *pixmapButton[((m + 33) == (indx_bpm_rhythm & 127)) ? 11 : 10]);

        painter->setPen(Qt::black);
        QString s;
        s.setNum(480 * (m + 1) / 64);
        int pix = 1 + ((s.count() == 2) ? 3 : 0)  + ((s.count() == 1) ? 7 : 0);
        painter->drawText(163 + m * 24 + pix, yr2 + 16 + 25 * 2  + 12,
                          s);

        s.setNum(480 * (m + 33) / 64);
        //pix = s.count() * 4 + 1;
        painter->drawText(163 + m * 24 + 1 , yr2 + 16 + 25 * 2  + 32, s);
    }

    painter->setBackgroundMode(Qt::OpaqueMode);
    painter->drawText(163 - 42, yr2 + 16 + 25 * 2 + 16, "Speed:");

    painter->drawText(163 - 40, yr2 + 25 * 5 + 24, "Track: ");
    painter->drawText(163 - 40, yr2 + 25 * 5 + 75 + 24, "Library: ");
    painter->drawText(163 + 24 * 25 - 73,  yr2 + 28 + 25, "Title: ");
    painter->setBackgroundMode(Qt::TransparentMode);
    painter->drawText(163 - 42, yr2 + 16 + 25 * 2 + 16 + 16, " BPM");

    brush.setColor(QColor(0xc0, 0xf0, 0xc0, 0xff));
    painter->setBrush(brush);
    for(int n = 0; n < 10; n++) {
        painter->drawEllipse(22 - 8 , yr + 16 + 25 * n + 7, 6, 6);
    }

}


void MyInstrument::keyPressEvent(QKeyEvent* event) {

    if(is_rhythm) return;

    if(is_playing && _piano_insert_mode && !MidiPlayer::isPlaying()) return;

    QByteArray a;

    if (!event->isAutoRepeat()) {

        int eventkey= event->key();

        int key= -1;
        int vel = 127;

        if(drum_mode) {
            if(eventkey == 'Z') {key = 35; vel = 127;} // Acoustic Bass Drum
            if(eventkey == 'X') {key = 35; vel = 80;}
            if(eventkey == 'C') {key = 36; vel = 127;} // Bass Drum
            if(eventkey == 'V') {key = 36; vel = 80;}

            if(eventkey == 'Q') {key = 38; vel = 127;} // Acoustic Snare
            if(eventkey == 'A') {key = 38; vel = 64;}
            if(eventkey == 'W') {key = 38; vel = 96;}
            if(eventkey == 'S') {key = 38; vel = 48;}

            if(eventkey == 'E') {key = 40; vel = 127;} // Electric snare
            if(eventkey == 'D') {key = 40; vel = 64;}
            if(eventkey == 'R') {key = 40; vel = 96;}
            if(eventkey == 'F') {key = 40; vel = 48;}


            if(eventkey == '1') {key = 42; vel = 127;} // closed hi-hat
            if(eventkey == '2') {key = 44; vel = 127;} // pedal hi-hat
            if(eventkey == '3') {key = 46; vel = 100;} // open hi-hat

            if(eventkey == '4') {key = 49; vel = 100;} // crash cymbal
            if(eventkey == '7') {key = 51; vel = 100;} // ride cymbal
            if(eventkey == '8') {key = 55; vel = 100;} // splash cymbal
            if(eventkey == '5') {key = 57; vel = 100;} // crash cymbal 2
            if(eventkey == '6') {key = 52; vel = 100;} // chinese cymbal
            if(eventkey == '9') {key = 53; vel = 100;} // ride bell
            if(eventkey == '0') {key = 54; vel = 100;} // tambourine

            if(eventkey == 'T') {key = 41; vel = 127;} // Low floor tom
            if(eventkey == 'Y') {key = 43; vel = 127;} // High floor tom
            if(eventkey == 'U') {key = 45; vel = 127;} // Low tom
            if(eventkey == 'I') {key = 47; vel = 127;} // Low mid tom
            if(eventkey == 'O') {key = 48; vel = 127;} // Hi mid tom
            if(eventkey == 'P') {key = 50; vel = 127;} // High tom

            if(eventkey == 'G') {key = 41; vel = 100;} // Low floor tom
            if(eventkey == 'H') {key = 43; vel = 100;} // High floor tom
            if(eventkey == 'J') {key = 45; vel = 100;} // Low tom
            if(eventkey == 'K') {key = 47; vel = 100;} // Low mid tom
            if(eventkey == 'L') {key = 48; vel = 100;} // Hi mid tom

            piano_keys_key[key] = vel;

        } else {
            for(int n = 0; n < 18; n++) {
                if(eventkey == piano_keys_map[n]) {key = n + octave_pos; break;}
                if(eventkey == piano_keys2_map[n]) {key = n + octave_pos + 12 ; break;}
            }
        }

        int key2 = key + ((drum_mode) ? 0 : trans_pos);

        if(key2 < 0 || key2 > 127) key2= 0;

        if(key>=0 && piano_keys_time[key]<0) {
            int note = key;
            if(!MidiPlayer::isPlaying()) {
                // starts the record here
                if(_piano_timer < 0) {
                    is_playing = 0;
                    Init_time(file->msOfTick(file->cursorTick()));
                    piano_keys_itime[note]= _piano_timer =  file->msOfTick(file->cursorTick());
                } else
                    piano_keys_itime[note]= _piano_timer = Get_time();
            } else {
                is_playing = 1;
                if(_piano_timer < 0) {
                    Init_time(MidiPlayer::timeMs());
                    piano_keys_itime[note]= MidiPlayer::timeMs();

                } else
                    piano_keys_itime[note]= MidiPlayer::timeMs();
            }

            piano_keys_time[note]= TIMER_TICK; // minimun

            if(drum_mode) {

                a.clear();
                a.append(0xC9);
                a.append(Prog_MIDI[9]); // program
                MidiOutput::sendCommand(a);

            } else {

                a.clear();
                a.append(0xB0 | MidiOutput::standardChannel()); // bank
                a.append((char ) 0);
                a.append(Bank_MIDI[MidiOutput::standardChannel()]);
                MidiOutput::sendCommand(a);

                a.clear();
                a.append(0xC0 | MidiOutput::standardChannel());
                a.append(Prog_MIDI[MidiOutput::standardChannel()]); // program
                MidiOutput::sendCommand(a);
            }

            // play note

            a.clear(); // note on
            a.append(0x90 | ((drum_mode) ? 9 : MidiOutput::standardChannel()));
            a.append(key2);
            a.append((char ) ((drum_mode) ? vel : volumeSlider->value()));
            MidiOutput::sendCommand(a);

            QD->update();

        }

    }

}

void MyInstrument::keyReleaseEvent(QKeyEvent* event) {

    if(is_rhythm) return;

    // playing from keyboard
    if (!event->isAutoRepeat()) {
        QByteArray a;

        int eventkey= event->key();

        int key= -1;

        int vel = 127;

        if(drum_mode) {
            if(eventkey == 'Z') {key = 35; vel = 127;} // Acoustic Bass Drum
            if(eventkey == 'X') {key = 35; vel = 80;}
            if(eventkey == 'C') {key = 36; vel = 127;} // Bass Drum
            if(eventkey == 'V') {key = 36; vel = 80;}

            if(eventkey == 'Q') {key = 38; vel = 127;} // Acoustic Snare
            if(eventkey == 'A') {key = 38; vel = 64;}
            if(eventkey == 'W') {key = 38; vel = 96;}
            if(eventkey == 'S') {key = 38; vel = 48;}

            if(eventkey == 'E') {key = 40; vel = 127;} // Electric snare
            if(eventkey == 'D') {key = 40; vel = 64;}
            if(eventkey == 'R') {key = 40; vel = 96;}
            if(eventkey == 'F') {key = 40; vel = 48;}


            if(eventkey == '1') {key = 42; vel = 127;} // closed hi-hat
            if(eventkey == '2') {key = 44; vel = 127;} // pedal hi-hat
            if(eventkey == '3') {key = 46; vel = 100;} // open hi-hat

            if(eventkey == '4') {key = 49; vel = 100;} // crash cymbal
            if(eventkey == '7') {key = 51; vel = 100;} // ride cymbal
            if(eventkey == '8') {key = 55; vel = 100;} // splash cymbal
            if(eventkey == '5') {key = 57; vel = 100;} // crash cymbal 2
            if(eventkey == '6') {key = 52; vel = 80;} // chinese cymbal
            if(eventkey == '9') {key = 53; vel = 100;} // ride bell
            if(eventkey == '0') {key = 54; vel = 100;} // tambourine

            if(eventkey == 'T') {key = 41; vel = 127;} // Low floor tom
            if(eventkey == 'Y') {key = 43; vel = 127;} // High floor tom
            if(eventkey == 'U') {key = 45; vel = 127;} // Low tom
            if(eventkey == 'I') {key = 47; vel = 127;} // Low mid tom
            if(eventkey == 'O') {key = 48; vel = 127;} // Hi mid tom
            if(eventkey == 'P') {key = 50; vel = 127;} // High tom

            if(eventkey == 'G') {key = 41; vel = 100;} // Low floor tom
            if(eventkey == 'H') {key = 43; vel = 100;} // High floor tom
            if(eventkey == 'J') {key = 45; vel = 100;} // Low tom
            if(eventkey == 'K') {key = 47; vel = 100;} // Low mid tom
            if(eventkey == 'L') {key = 48; vel = 100;} // Hi mid tom

        } else {

            for(int n = 0; n < 18; n++) {
                if(eventkey == piano_keys_map[n]) {key = n + octave_pos; break;}
                if(eventkey == piano_keys2_map[n]) {key = n + octave_pos + 12; break;}
            }
        }

        int key2 = key + ((drum_mode) ? 0 : trans_pos);
        if(key2 < 0 || key2 > 127) key2= 0;

        if(key>=0) {
            int note = key;

            a.clear();
            a.append(0x80 | MidiOutput::standardChannel()); // note off
            a.append(key2);
            a.append((char ) 0);
            MidiOutput::sendCommand(a);

            if(_piano_insert_mode) {

                if(drum_mode) {
                    RecNote(key2,
                        piano_keys_itime[note],
                        piano_keys_itime[note] + 60,
                        vel);

                } else {
                    RecNote(key2,
                        piano_keys_itime[note],
                        (MidiPlayer::isPlaying()) ? MidiPlayer::timeMs() :
                        Get_time(),
                        volumeSlider->value());
                }

            }

            piano_keys_time[note]=-1;
            I_NEED_TO_UPDATE = 1;

        }

    }
}

void MyInstrument::mousePressEvent(QMouseEvent* event) {
    int key= -1;

    if(!is_rhythm && drum_mode) return;
    if(is_playing && _piano_insert_mode && !MidiPlayer::isPlaying()) return;

    xx= event->x();
    yy= event->y();

    QByteArray a;

    if(is_rhythm) {
        for(int n = 0; n < 10; n++) { // sample editor
            for(int m = 0; m < 32; m++) {

                int x = 163 + m * 24;
                int y = yr + 18 + 25 * n;
            //

                if(event->button() == Qt::LeftButton && xx >= x && xx < (x + 22) && yy >= y && yy < (y + 16)) {

                    map_rhythm[n][m + 2]++;

                    if(map_rhythm[n][m + 2] > 3)
                        map_rhythm[n][m + 2] = 0;
                    save_rhythm_edit();
                    QD->update();
                }

                if(event->button() == Qt::RightButton && xx >= x && xx < (x + 22) && yy >= y && yy < (y + 16)) {

                    map_rhythm[n][m + 2] = 0;
                    save_rhythm_edit();
                    QD->update();
                }
            }
        }

        for(int m = 0; m < 32; m++) {

            int x = 163 + m * 24;
            int y = yr2 + 18;

            // sample editor steps
            if(xx >= x && xx < (x + 22) && yy >= y && yy < (y + 16)) {
                max_map_rhythm = m + 1;
                save_rhythm_edit();
                QD->update();
                   // sample editor speed
            } else if(xx >= x && xx < (x + 22) && yy >= (y + 50) && yy < (y + 66)) {
                indx_bpm_rhythm = m + 1;
                save_rhythm_edit();
                QD->update();
                   // sample editor speed 2
            } else if(xx >= x && xx < (x + 22) && yy >= (y + 70) && yy < (y + 86)) {
                indx_bpm_rhythm = m + 33;
                save_rhythm_edit();
                QD->update();
            }
        }

        // play drum
        for(int n = 0; n < 10; n++) {

            int x = 22 - 8 - 6;
            int y = yr + 16 + 25 * n + 7 - 6;

            if(xx >= x && xx < (x + 12) && yy >= y && yy < (y + 12)) {


                // play note

                a.clear();
                a.append(0xC9);
                a.append(Prog_MIDI[9]); // program
                MidiOutput::sendCommand(a);
                a.clear();
                a.append(0x99); // note on
                a.append(map_rhythm[n][0]);
                a.append(map_rhythm[n][1]);
                MidiOutput::sendCommand(a);

                QThread::msleep(100);

                a.clear();
                a.append(0x89);
                a.append(map_rhythm[n][0]);
                a.append(map_rhythm[n][1]);
                MidiOutput::sendCommand(a);

            }
        }
        return;
    }

    int key_pos = (octave_pos >= 48) ? 48 : octave_pos + 12;

    // black
    for(int n = 0; n < 61; n++) {
        int m= n % 12;
        int x= 4 + (n/12) * (22 * 7) + dx[m];
        if((m==1 || m==3 || m==6 || m==8 || m==10)) {
           if(xx > x && yy > 34
                   && xx < (x+14) &&  yy < (60+34))
           {key = n + key_pos - 12; break;}

        }
    }
    // white
    if(key < 0)
    for(int n = 0; n < 61; n++) {
        int m= n % 12;
        int x= 4 + (n/12) * (22 * 7) + dx[m];
        if(!(m==1 || m==3 || m==6 || m==8 || m==10)) {
           if(xx > x && yy > 34
                   && xx < (x+23) &&  yy < (100+34))
           {key = n + key_pos - 12; break;}

        }
    }

    int key2 = key + trans_pos;
    if(key2 < 0 || key2 > 127) key2= 0;

    if (key>=0 && playing_piano < 0) {
        // play note from the mouse

        if(playing_piano>=0) {
            piano_keys_time[playing_piano] = -1;
            playing_piano=-1;

            a.clear(); // note off
            a.append(0x80 | MidiOutput::standardChannel());
            a.append(playing_piano);
            a.append((char ) 0);
            MidiOutput::sendCommand(a);
        }

        playing_piano= key;
        int note = playing_piano;

        if(!MidiPlayer::isPlaying()) {
            // starts the record here
            if(_piano_timer < 0) {
                is_playing = 0;
                Init_time(file->msOfTick(file->cursorTick()));
                piano_keys_itime[note]= _piano_timer =  file->msOfTick(file->cursorTick());
            } else
                piano_keys_itime[note]= _piano_timer = Get_time();
        } else {
            is_playing = 1;
            if(_piano_timer < 0) {
                Init_time(MidiPlayer::timeMs());
                piano_keys_itime[note]= MidiPlayer::timeMs();

            } else
                piano_keys_itime[note]= MidiPlayer::timeMs();
        }

        piano_keys_time[note]= TIMER_TICK; // minimun

        if(drum_mode) playing_piano = -1;


        a.clear();
        a.append(0xB0 | MidiOutput::standardChannel()); // bank
        a.append((char ) 0);
        a.append(Bank_MIDI[MidiOutput::standardChannel()]);
        MidiOutput::sendCommand(a);

        a.clear();
        a.append(0xC0 | MidiOutput::standardChannel()); // program
        a.append(Prog_MIDI[MidiOutput::standardChannel()]);
        MidiOutput::sendCommand(a);

        a.clear();
        a.append(0x90 | MidiOutput::standardChannel()); //note on
        a.append(key2);
        a.append((char ) volumeSlider->value());
        MidiOutput::sendCommand(a);

        QD->update();

    }

}

void MyInstrument::mouseReleaseEvent(QMouseEvent* /*event*/) {

    if(is_rhythm) {
        return;
    }

    if(drum_mode) return;

    QByteArray a;

    // playing from the mouse
    if(playing_piano>=0 && piano_keys_time[playing_piano]>=0) {
        int note = playing_piano;

        int key2 = note + trans_pos;
        if(key2 < 0 || key2 > 127) key2= 0;

        a.clear(); // note off
        a.append(0x80 | MidiOutput::standardChannel());
        a.append(key2);
        a.append((char ) 0);
        MidiOutput::sendCommand(a);

        if(_piano_insert_mode) {
            RecNote(key2,
                piano_keys_itime[note],
                    (MidiPlayer::isPlaying()) ? MidiPlayer::timeMs() :
                    Get_time(),
                volumeSlider->value());

        }

        piano_keys_time[note]=-1;

        playing_piano = -1;
        I_NEED_TO_UPDATE = 1;
    }

}

void MyInstrument::mouseDoubleClickEvent(QMouseEvent* event) {
    mousePressEvent(event);
}

void MyInstrument::record_clicked() {

    _piano_insert_mode^=1;

    if(MidiInput::recording()) {
        _piano_insert_mode = 0;
        recordButton->setChecked(false);
        return;
    }


    is_playing = (MidiPlayer::isPlaying()) ? 1 : 0;

    for(int n = 0; n < 128; n++)
          piano_keys_time[n]= -1;

    if(_piano_insert_mode) {
        _piano_timer = -1;
    } else {
        if(recNotes.isEmpty()) return;
        if(QMessageBox::question(this, "MidiEditorPiano", "Save Recorded Notes?") ==
            QMessageBox::Yes) RecSave();
        else {
            while (!recNotes.isEmpty())
                delete recNotes.takeFirst();
        }
    }
}

void MyInstrument::octave_key(int num) {
    octave_pos = num * 12 + 12;

    for(int n = 0; n < 128; n++) {
        piano_keys_time[n] = -1;
        piano_keys_key[n] = 0;
    }

    int key_pos = (octave_pos < 36) ? 36 : octave_pos;
    for(int n = 0; n < 18; n++) {
        if(n < 12) piano_keys_key[key_pos + n] = piano_keys_map[n];
        piano_keys_key[key_pos + n + 12] = piano_keys2_map[n];
    }

    MidiPlayer::panic();
    QD->update();
    QD->setFocus();
}

void MyInstrument::transpose_key(int num) {
    trans_pos = num;

    for(int n = 0; n < 128; n++) {
        piano_keys_time[n] = -1;
    }

    MidiPlayer::panic();
    QD->update();
    QD->setFocus();
}

void MyInstrument::volume_changed(int vol) {

    note_volume = vol;

}

void MyInstrument::rbackToBegin() {
    if(_piano_insert_mode)
        return;
    MainWindow * Q = (MainWindow *) _MAINW;
    Q->backToBegin();
    if(is_playsync && play_rhythm_track) {
        ptButton->setChecked(false);
        emit ptButton->clicked();
        is_playsync = 0;
    }
}

void MyInstrument::rback() {
    if(_piano_insert_mode)
        return;
    MainWindow * Q = (MainWindow *) _MAINW;
    Q->back();
    if(is_playsync && play_rhythm_track) {
        ptButton->setChecked(false);
        emit ptButton->clicked();
        is_playsync = 0;
    }
}

void MyInstrument::rplay() {
    if(_piano_insert_mode && !MidiPlayer::isPlaying() && _piano_timer >= 0)
        return;
    _piano_timer = -1;
    MainWindow * Q = (MainWindow *) _MAINW;
    Q->play();
    if(is_playsync && play_rhythm_track) {
        ptButton->setChecked(false);
        emit ptButton->clicked();
        is_playsync = 0;
    }
    is_playsync = 0;
}

void MyInstrument::rplay2() {
    if(_piano_insert_mode && !MidiPlayer::isPlaying() && _piano_timer >= 0)
        return;

    _piano_timer = -1;
    MainWindow * Q = (MainWindow *) _MAINW;
    Q->play();

    if(play_rhythm_track) {
        ptButton->setChecked(false);
        emit ptButton->clicked();
        is_playsync = 0;
    }

    if(!play_rhythm_track) {
        ptButton->setChecked(true);
        emit ptButton->clicked();
        is_playsync = 1;
    }
}

void MyInstrument::rpause() {
    if(_piano_insert_mode && !MidiPlayer::isPlaying())
        return;
    MainWindow * Q = (MainWindow *) _MAINW;
    Q->pause();

    if(is_playsync && play_rhythm_track) {
        ptButton->setChecked(false);
        emit ptButton->clicked();
        is_playsync = 0;
    }
}

void MyInstrument::rstop() {
    MainWindow * Q = (MainWindow *) _MAINW;
    Q->stop();

    if(is_playsync && play_rhythm_track) {
        ptButton->setChecked(false);
        emit ptButton->clicked();
        is_playsync = 0;
    }
}

void MyInstrument::rforward() {
    if(_piano_insert_mode)
        return;
    MainWindow * Q = (MainWindow *) _MAINW;
    Q->forward();

    if(is_playsync && play_rhythm_track) {
        ptButton->setChecked(false);
        emit ptButton->clicked();
        is_playsync = 0;
    }
}

void MyInstrument::save_rhythm_edit() {

    QListWidgetItem* item = new QListWidgetItem();
    int row = rhythmSamplelist->currentRow();

    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    rhythmData rhythm = rhythmData();

    rhythm.setName(titleEdit->text());

    rhythm.set_rhythm_header(max_map_rhythm, indx_bpm_rhythm | ((fix_time_sample) ? 0 : 128), 0);
    for(int n = 0; n < 10; n++)
        rhythm.insert(n, &map_rhythm[n][0]);

    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    item->setText(QString().asprintf("Editor\n") + rhythm.getName());
    item->setData(Qt::UserRole, rhythm.save());
    item->setBackground(QBrush(QColor(0xe0, 0x40, 0x40, 0xff)));
    rhythmSamplelist->insertItem(0, item);
    if(rhythmSamplelist->count() > 1)
        delete rhythmSamplelist->takeItem(1);
    rhythmSamplelist->setCurrentRow(row);
    rhythmSamplelist->update();

}

/************************************************************************************/
// pop-up editDialog

editDialog::editDialog(QWidget * parent, int flags) : QDialog(parent, Qt::Popup | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
    _editDialog = this;
    _action = EDITDIALOG_NONE;

    if (_editDialog->objectName().isEmpty())
        _editDialog->setObjectName(QString::fromUtf8("_editDialog"));
    _editDialog->setEnabled(true);
    _editDialog->setFixedSize(138, 188);
    _editDialog->setWindowTitle(QString::fromUtf8(""));
    _editDialog->setSizeGripEnabled(false);

    int y = 19;

    if(flags & EDITDIALOG_EDIT) {
        editButton = new QPushButton(_editDialog);
        editButton->setObjectName(QString::fromUtf8("editButton"));
        editButton->setGeometry(QRect(20, y, 101, 23));
        editButton->setText("Edit");

        connect(editButton, QOverload<bool>::of(&QPushButton::clicked),[=](bool)
        {
            _action = (EDITDIALOG_EDIT);
            hide();
        });

        y+= 30;
    }

    if(flags & EDITDIALOG_SET) {
        setButton = new QPushButton(_editDialog);
        setButton->setObjectName(QString::fromUtf8("setButton"));
        setButton->setGeometry(QRect(20, y, 101, 23));
        setButton->setText("Set");

        connect(setButton, QOverload<bool>::of(&QPushButton::clicked),[=](bool)
        {
            _action = (EDITDIALOG_SET);
            hide();
        });

        y+= 30;
    }

    if(flags & EDITDIALOG_INSERT) {
        insertButton = new QPushButton(_editDialog);
        insertButton->setObjectName(QString::fromUtf8("insertButton"));
        insertButton->setGeometry(QRect(20, y, 101, 23));
        insertButton->setText("Insert");

        connect(insertButton, QOverload<bool>::of(&QPushButton::clicked),[=](bool)
        {
            _action = (EDITDIALOG_INSERT);
            hide();
        });

        y+= 30;
    }

    if(flags & EDITDIALOG_REPLACE) {
        replaceButton = new QPushButton(_editDialog);
        replaceButton->setObjectName(QString::fromUtf8("replaceButton"));
        replaceButton->setGeometry(QRect(20, y, 101, 23));
        replaceButton->setText("Replace");

        connect(replaceButton, QOverload<bool>::of(&QPushButton::clicked),[=](bool)
        {
            _action = (EDITDIALOG_REPLACE);
            hide();
        });

        y+= 30;
    }

    if(flags & EDITDIALOG_REMOVE) {

        removeButton = new QPushButton(_editDialog);
        removeButton->setObjectName(QString::fromUtf8("removeButton"));
        removeButton->setGeometry(QRect(20, y, 101, 23));
        removeButton->setText("Remove");

        connect(removeButton, QOverload<bool>::of(&QPushButton::clicked),[=](bool)
        {
            _action = (EDITDIALOG_REMOVE);
            hide();
        });

        y+= 30;
    }

    QMetaObject::connectSlotsByName(_editDialog);
}

/************************************************************************************/
// QMListWidget

int QMListWidget::_drag = -1;
QMListWidget * QMListWidget::_QMListWidget = NULL;

void QMListWidget::dropEvent(QDropEvent *event) {
    int type = event->type();

    if(type == QEvent::Drop) {

        if(_flag == 0 && QMListWidget::_drag == 1) {
            int m = QMListWidget::_QMListWidget->currentRow();
            QListWidgetItem *i = NULL;
            QString s;
            if(m >= 0) {
                i = QMListWidget::_QMListWidget->item(m);
            }
            if(i) {
                QByteArray a = i->data(Qt::UserRole).toByteArray();
                rhythmData rhythm = rhythmData(a);

                s = rhythm.getName();
            }

            if(m < 0 || !i || s == "" || s.count() < 4
                   || s.toUtf8().at(0) < 65) {
                event->ignore();
                QMListWidget::_drag = -1;
                update();
                QMessageBox::information(this, "Rhythm Box Editor",
                                    "sample name too short or invalid",
                                    QMessageBox::Ok);
                return;
            }

        }

        if(_flag == 1 && QMListWidget::_drag == 0) {
            event->ignore();
            QMListWidget::_drag = -1;
            update();
            return;
        }

        if(_flag == 1 && QMListWidget::_drag == 1) {
            event->ignore();

            int m = currentRow();

            if(m >= 0) {
                for(int n = 0; n < count(); n++) {
                    QRect r = visualItemRect(item(n));
                    if(event->pos().x() >= r.x() && event->pos().x() <  r.x() + r.width() &&
                       event->pos().y() >= r.y() && event->pos().y() <  r.y() + r.height()) {

                        if(n != 0 && m != 0 && (
                                    ((item(n)->flags() & Qt::ItemIsUserCheckable) && (item(m)->flags() & Qt::ItemIsUserCheckable))
                                    || (!(item(n)->flags() & Qt::ItemIsUserCheckable) && !(item(m)->flags() & Qt::ItemIsUserCheckable)))) {

                            QListWidgetItem items; //swap items
                            items = *item(n);
                            *item(n) = *item(m);
                            *item(m) = items;
                        }
                    }
                }
            }
            update();
            return;
        }

    QListWidget::dropEvent(event);

    QMListWidget::_drag = -1;
    update();
    }

}

void QMListWidget::dragEnterEvent(QDragEnterEvent *event) {

    if(QMListWidget::_drag == -1) {
        QMListWidget::_drag = _flag;
        QMListWidget::_QMListWidget = this;
    }

    QListWidget::dragEnterEvent(event);

}

void QMListWidget::update() {

    _update = true;
    QMListWidget::_drag = -1;

    QListWidget::update();
}

void QMListWidget::mousePressEvent(QMouseEvent* event) {

        buttons = event->button();
        QListWidget::mousePressEvent(event);
}

void QMListWidget::paintEvent(QPaintEvent* event){

    if(_update) {
        _update = false;

        for(int n = 0; n < count(); n++) {

            QByteArray a = item(n)->data(Qt::UserRole).toByteArray();
            rhythmData rhythm2 = rhythmData(a);
                item(n)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            if(!(item(n)->flags() & Qt::ItemIsUserCheckable) && _flag) {
                item(n)->setText(QString().asprintf("Library\n") + rhythm2.getName());
                item(n)->setBackground(QBrush(QColor(0xe0, 0xe0, 0xf0, 0xff)));
            } else if(_flag != 0 && n == 0) {
                item(n)->setText(QString().asprintf("Editor\n") + rhythm2.getName());
                item(n)->setBackground(QBrush(QColor(0xe0, 0x40, 0x40, 0xff)));
            }
             else {
                if(!_flag) item(n)->setFlags(item(n)->flags() | Qt::ItemIsUserCheckable);
                item(n)->setText(QString().asprintf("< %i >\n", n + (_flag == 0)) + rhythm2.getName());
                if(_flag)
                    item(n)->setBackground(QBrush(QColor(0x70, 0xa0, 0x70, 0xff)));
                else
                    item(n)->setBackground(QBrush(QColor(0xf0 ,0xa0, 0x50, 0xff)));
            }

        }
    }

    if(!event) return;
    QListWidget::paintEvent(event);
}

/************************************************************************************/
// rhythmData

rhythmData::rhythmData() {
    _speed = 8;
    _drum_set = 0;
    _size_map = 2;
    _lines = 10;

    clear();
    append(_speed);
    append(_drum_set);
    append(_size_map);
    append(_lines);

    for(int n = 0; n < _lines; n++) {
        set[n].append(map_rhythmData[n][0]);
        set[n].append(map_rhythmData[n][1]);
    }

    append(0); // name empty
    append(0);
};

rhythmData::rhythmData(const QByteArray a) {

    int size = a.size();
    if(size < 4) {
        rhythmData();
        return;
    }

    size -= 4;
    int c = 4;

    _speed    = a.at(0);
    _drum_set = a.at(1);
    _size_map = a.at(2);
    _lines    = a.at(3);

    if(_lines > 10  || size < (_size_map *_lines + 1)) { // error
        rhythmData();
        return;
    }

    int n = 0;

    for(; n < _lines; n++) {

        set[n].clear();

        for(int m = 0; m < _size_map; m++) {
            set[n].append(a.at(c));
            c++;
        }
    }

    for(; n < 10; n++) {
        set[n].clear();
        set[n].append(map_rhythmData[n][0]);
        set[n].append(map_rhythmData[n][1]);
        for(int m = 2; m < _size_map; m++) {
            set[n].append((char) 0);
        }
    }

    _lines = 10;

    _name.clear();
    _name.append((a.constData() + c));
}

QByteArray rhythmData::save() {

    clear();
    append(_speed);
    append(_drum_set);
    append(_size_map);
    append(_lines);

    for(int n = 0; n < 10; n++)
        append(set[n]);

    append(_name.toUtf8());

    return *this;
}

void rhythmData::set_rhythm_header(int size_map, int speed, int drum_set) {

    if(size_map >= 0)
        _size_map = size_map + 2;
    if(speed >= 0)
        _speed = speed;
    if(drum_set >= 0)
        _drum_set = drum_set;
}

void rhythmData::get_rhythm_header(int *size_map, int *speed, int *drum_set) {

    if(size_map)
        *size_map = _size_map - 2;
    if(speed) {
        if(!(_speed & 127)) // _speed must be from 1 to 127;
            _speed |= 16;

        *speed = _speed;
    }
    if(drum_set)
        *drum_set = _drum_set;
}

int rhythmData::get_drum_note(int index) {
    if(index < 0 || index > 9) return 0;
    return set[index].at(0);
}

int rhythmData::get_drum_velocity(int index) {
    if(index < 0 || index > 9) return 0;
    return set[index].at(1);
}

void rhythmData::set_drum_note(int index, int note) {

    note &= 127;
    if(index < 0 || index > 9) return;
    set[index].remove(0, 1);
    set[index].insert(0, (unsigned char) note);
}

void rhythmData::set_drum_velocity(int index, int velocity) {

    velocity &= 127;
    if(index < 0 || index > 9) return;
    set[index].remove(1, 1);
    set[index].insert(1, (unsigned char) velocity);
}

void rhythmData::insert(int index, unsigned char *set_map) {

    if(index < 0 || index > 9) return;
    set[index].clear();
    for(int n = 0; n < _size_map; n++)
        set[index].append(set_map[n]);

}

void rhythmData::get(int index, unsigned char *get_map) {

    if(index < 0 || index > 9) return;

    for(int n = 0; n < _size_map; n++)
        get_map[n] = set[index].at(n);
}

void rhythmData::setName(const QString &name) {
    _name = name;
}

QString rhythmData::getName() {
    return _name;
}

QList<QByteArray> Play_Thread::rhythm_samples;
Play_Thread * Play_Thread::rhythmThread = NULL;

void Play_Thread::_fun_timer() {

    QByteArray a;

    if(play_rhythm_track) {
        if(!Play_Thread::rhythm_samples.count() || play_rhythm_track_index == -2) return;
        if((play_rhythm_track_init || current_map_rhythm >= max_map_rhythm_track) ) {
            play_rhythm_track_init = 0;
            // load datas
            while(1) { // find sample datas
                if(play_rhythm_track_index == -1) {
                    play_rhythm_track_index = 0;
                    current_map_rhythm = 0;
                    max_map_rhythm_track = 1;
                }

                if(current_map_rhythm >= 2) {
                    play_rhythm_track_index++;
                    if(play_rhythm_track_index >= Play_Thread::rhythm_samples.count()) {

                        if(loop_rhythm_track) {

                            if(loop_rhythm_base > 0) {

                                if(loop_rhythm_track >= loop_rhythm_base) {
                                    loop_rhythm_track = 1;
                                    play_rhythm_track_index = -2;
                                    return;
                                }

                                loop_rhythm_track++;
                            }
                            play_rhythm_track_index = 0;
                        }
                        else {

                            play_rhythm_track_index = -2;
                            return;
                        }

                    }
                }

                current_map_rhythm = 0;
                indx_bpm_rhythm_track = 8;

                current_tick_rhythm = 0;
                play_rhythm_sample = 0;

                memset(map_rhythm_track, 0, sizeof(unsigned char) * 10 * 34);

                if(Play_Thread::rhythm_samples.count() <= 0) {

                    play_rhythm_track_index = -2;
                    return;
                }

                QByteArray a = Play_Thread::rhythm_samples.at(play_rhythm_track_index);

                if(!a.size()) {
                    play_rhythm_track_index = -2;
                    return;
                }

                rhythmData rhythm = rhythmData(a);
                rhythm.get_rhythm_header(&max_map_rhythm_track, &indx_bpm_rhythm_track, NULL);

                if(default_time || (indx_bpm_rhythm_track & 128))
                    indx_bpm_rhythm_track = indx_bpm_rhythm;

                if(max_map_rhythm_track <= 0)
                    continue; // empty

                for(int n = 0; n < 10; n++) {
                    rhythm.get(n, &map_rhythm_track[n][0]);
                }

                break;
            }

        }

        if(play_rhythm_track_index >= 0) {

            for(int n = 0; n < 10; n++) {

                int key = -1, vel;
                int play = 0;

                play |= (map_rhythm_track[n][current_map_rhythm + 2] == 1) && (current_tick_rhythm == 0);
                play |= (map_rhythm_track[n][current_map_rhythm + 2] == 2) && (current_tick_rhythm == 1);
                play |= (map_rhythm_track[n][current_map_rhythm + 2] == 3) &&
                        (current_tick_rhythm ==0 || current_tick_rhythm == 1);

                key = map_rhythm_track[n][0];
                vel = map_rhythm_track[n][1];

                if(key >= 0) {
                    if(play) {
                        a.clear();
                        a.append(0xC9); // program
                        a.append(Prog_MIDI[9]);
                        MidiOutput::sendCommand(a);
                        a.clear();
                        a.append(0x99); // note on
                        a.append(key);
                        a.append(vel);
                        MidiOutput::sendCommand(a);

                    } else {

                        a.clear();
                        a.append(0x89); // note off
                        a.append(key);
                        a.append((char) 0);
                        MidiOutput::sendCommand(a);
                    }
                }// play
            } // n

            current_tick_rhythm++;
            if(current_tick_rhythm >= 2) {
                current_tick_rhythm = 0;
                current_map_rhythm++;
            }

        }

        return;
    }


    if(!play_rhythm_sample) return;

    if(current_map_rhythm >= max_map_rhythm) {
        if(!loop_rhythm_sample) {
            play_rhythm_sample = 0;

        }

        current_map_rhythm = 0;
    }

    for(int n = 0; n < 10; n++) {

        int key = -1, vel;
        int play = 0;

        play |= (map_rhythm[n][current_map_rhythm + 2] == 1) && (current_tick_rhythm == 0);
        play |= (map_rhythm[n][current_map_rhythm + 2] == 2) && (current_tick_rhythm == 1);
        play |= (map_rhythm[n][current_map_rhythm + 2] == 3) &&
                (current_tick_rhythm ==0 || current_tick_rhythm == 1);

        key = map_rhythm[n][0];
        vel = map_rhythm[n][1];

        if(key >= 0) {
            if(play) {

                a.clear();
                a.append(0xC9); // program
                a.append(Prog_MIDI[9]);
                MidiOutput::sendCommand(a);
                a.clear();
                a.append(0x99); // note on
                a.append(key);
                a.append(vel);
                MidiOutput::sendCommand(a);


            } else {
                a.clear();
                a.append(0x89); // note off
                a.append(key);
                a.append((char) 0);
                MidiOutput::sendCommand(a);
            }
        }// play
    } // n

    current_tick_rhythm++;
    if(current_tick_rhythm >= 2) {
        current_tick_rhythm = 0;
        current_map_rhythm++;
        if(current_map_rhythm >= max_map_rhythm) {

            if(!loop_rhythm_sample) {
                play_rhythm_sample = 0; 
            }

            current_map_rhythm = 0;
        }

    }
    return;

}

void Play_Thread::run() {

    qint64 one, two;

    one = QDateTime::currentMSecsSinceEpoch();

    while(loop) {
        _fun_timer();
        two = QDateTime::currentMSecsSinceEpoch();
        int v_msleep;

        if(play_rhythm_track)
            v_msleep = ((1000 * 64 / indx_bpm_rhythm_track)/16);
        else
            v_msleep = ((1000 * 64 / indx_bpm_rhythm)/16);

        one = two - one;
        if(one >= (v_msleep * 2 - 1)) one = v_msleep;
        else if(one > v_msleep) one = v_msleep * 2 -  one;
        else one = v_msleep;
        msleep(one);

        one = two;
    }

}

void QMGroupBox::paintEvent(QPaintEvent* /*event*/) {
    QPainter* painter = new QPainter(this);
    QFont font = painter->font();
    font.setPixelSize(12);
    painter->setFont(font);
    painter->setClipping(false);

    painter->drawImage(324, 70, QImage(":/run_environment/graphics/tool/drumbox-res.png"));

    delete painter;
}


velocityDialog::velocityDialog(QWidget * parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint) {

    Velocity = this;

    if (Velocity->objectName().isEmpty())
        Velocity->setObjectName(QString::fromUtf8("Velocity"));
    Velocity->setWindowTitle("");
    Velocity->setFixedSize(242, 122);

    velocitySlider = new QSlider(Velocity);
    velocitySlider->setObjectName(QString::fromUtf8("velocitySlider"));
    velocitySlider->setGeometry(QRect(30, 60, 181, 22));
    velocitySlider->setMaximum(127);
    velocitySlider->setValue(64);
    velocitySlider->setOrientation(Qt::Horizontal);
    velocitySlider->setTickPosition(QSlider::TicksAbove);
    velocitySlider->setTickInterval(64);

    label = new QLabel(Velocity);
    label->setObjectName(QString::fromUtf8("label"));
    label->setGeometry(QRect(100, 20, 47, 13));
    label->setText("Velocity");
    label->setAlignment(Qt::AlignCenter);

    vlabel = new QLabel(Velocity);
    vlabel->setObjectName(QString::fromUtf8("vlabel"));
    vlabel->setGeometry(QRect(110, 90, 21, 21));
    vlabel->setStyleSheet(QString::fromUtf8("background: #ffffff"));
    vlabel->setAlignment(Qt::AlignCenter);
    vlabel->setNum(0);
    vlabel->setText(QString());

    QObject::connect(velocitySlider, SIGNAL(valueChanged(int)), vlabel, SLOT(setNum(int)));
    QMetaObject::connectSlotsByName(Velocity);
}

QMComboBox::QMComboBox(QWidget *parent, int index) : QComboBox(parent) {
    _index = index;
}


void QMComboBox::mousePressEvent(QMouseEvent *event) {
    if(event->button() == Qt::RightButton) {

       int n = this->currentData().toInt();
       velocityDialog * d = new velocityDialog(this);
       d->Velocity->setWindowTitle(this->currentText());
       d->velocitySlider->setValue(map_drum_rhythm_velocity[n]);
       d->exec();

       map_rhythm[_index][1]
               = map_rhythmData[_index][1]
               = map_drum_rhythm_velocity[n] = d->velocitySlider->value();

    }

    QComboBox::mousePressEvent(event);

}


