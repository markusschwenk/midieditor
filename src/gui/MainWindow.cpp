/*
 * MidiEditor
 * Copyright (C) 2010  Markus Schwenk
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

#include "MainWindow.h"

#ifdef USE_FLUIDSYNTH
#include "../fluid/fluidsynth_proc.h"
#include "../fluid/FluidDialog.h"
#include "../VST/VST.h"
#else
#define DELETE(x) {if(x) delete x; x= NULL;}
#endif


#include <QKeySequence>

#ifdef IS_QT5
#define QKeyMulti(x) (x)
#define QKeyMulti2(x, y) (x | y)
#else
#include <QKeyCombination>

#define QKeyMulti(x) QKeySequence(QKeyCombination(x))
#define QKeyMulti2(x, y) QKeySequence(QKeyCombination(y, x))
#endif


#include <QAction>
#include <QActionGroup>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QMultiMap>
#include <QProcess>
#include <QScrollArea>
#include <QSettings>
#include <QStringList>
#include <QTabWidget>
#include <QTextEdit>
#include <QTextStream>
#include <QToolBar>
#include <QToolButton>
#include <QDesktopServices>

#include "Appearance.h"
#include "AboutDialog.h"
#include "ChannelListWidget.h"
#include "ClickButton.h"
#include "DonateDialog.h"
#include "EventWidget.h"
#include "FileLengthDialog.h"
#include "InstrumentChooser.h"
#include "SoundEffectChooser.h"

#include "MatrixWidget.h"
#include "MiscWidget.h"
#include "NToleQuantizationDialog.h"
#include "ProtocolWidget.h"
#include "RecordDialog.h"
#include "SelectionNavigator.h"
#include "SettingsDialog.h"
#include "TrackListWidget.h"
#include "TransposeDialog.h"
#include "TweakTarget.h"

#include "../tool/EraserTool.h"
#include "../tool/EventMoveTool.h"
#include "../tool/EventTool.h"
#include "../tool/MeasureTool.h"
#include "../tool/NewNoteTool.h"
#include "../tool/SelectTool.h"
#include "../tool/Selection.h"
#include "../tool/SizeChangeTool.h"
#include "../tool/StandardTool.h"
#include "../tool/TempoTool.h"
#include "../tool/TimeSignatureTool.h"
#include "../tool/Tool.h"
#include "../tool/ToolButton.h"

#include "../Terminal.h"
#include "../protocol/Protocol.h"

#ifdef ENABLE_REMOTE
#include "../remote/RemoteServer.h"
#endif

#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/NoteOnEvent.h"
#include "../MidiEvent/OffEvent.h"
#include "../MidiEvent/OnEvent.h"
#include "../MidiEvent/TextEvent.h"
#include "../MidiEvent/TimeSignatureEvent.h"
#include "../MidiEvent/TempoChangeEvent.h"
#include "../MidiEvent/ProgChangeEvent.h"
#include "../MidiEvent/ControlChangeEvent.h"
#include "../MidiEvent/PitchBendEvent.h"
#include "../MidiEvent/KeyPressureEvent.h"
#include "../MidiEvent/ChannelPressureEvent.h"
#include "../MidiEvent/KeySignatureEvent.h"
#include "../MidiEvent/SysExEvent.h"
#include "../MidiEvent/UnknownEvent.h"
#include "../midi/Metronome.h"
#include "../midi/MidiChannel.h"
#include "../midi/MidiFile.h"
#include "../midi/MidiInput.h"
#include "../midi/MidiOutput.h"
#include "../midi/MidiPlayer.h"
#include "../midi/MidiTrack.h"
#include "../midi/PlayerThread.h"
#include "../midi/MidiInControl.h"
#include "../gui/MidiEditorInstrument.h"

#include "../UpdateManager.h"
#include "CompleteMidiSetupDialog.h"
#include "UpdateDialog.h"
#include "AutomaticUpdateDialog.h"
#include <QtCore/qmath.h>

MidiInControl *MidiIn_control = NULL;

instrument_list InstrumentList[129];

int itHaveInstrumentList = 0;
int itHaveDrumList = 0;

void MainWindow::addInstrumentNames()
{
    QString path = _settings->value("Main/instrumentList").toString();

    itHaveInstrumentList=0;
    itHaveDrumList=0;

    int n,m;
    for(n=0; n< 129; n++)
        for(m=0; m<128; m++)
            InstrumentList[n].name[m]="undefined";

    if (path == "" || !QFile(path).exists()) return;

    QFile* f = new QFile(path);

    if (!f->open(QIODevice::ReadOnly | QIODevice :: Text)) {
        return;
    }

        n=0;
        while ( ! f->atEnd ()) {
           QByteArray line = f->readLine ();
           int bank=0, inst=0;

           line[line.length()-1]=0;
           int r=sscanf(line, "%d:%d = ",&bank,&inst);
           if(r<0) {
               char mess[256]="";
               sprintf(mess, "Error reading instruments list at line %i", n);
               QMessageBox::information(this, "Information", mess);
               break;
           }
           if(bank<0 || bank>129) {
               char mess[256]="";
               sprintf(mess, "Error in bank instruments list at line %i", n);
               QMessageBox::information(this, "Information", mess);
               break;
           }

           if((inst<0) || (inst>128)) {
               char mess[256]="";
               sprintf(mess, "Error in bank instruments list at line %i", n);
               QMessageBox::information(this, "Information", mess);
               break;
           }

           InstrumentList[bank].name[inst]= line.mid(10);
           itHaveInstrumentList=1;
           if(bank==128) itHaveDrumList=1;
           n++;

    }
    f->close();

}

MainWindow::~MainWindow() {

    if (MidiPlayer::isPlaying() || MidiInput::recording()) {
        stop();
    }

    if(finger_main) {
        delete finger_main;
        finger_main = NULL;
    }

    MidiPlayer::deinit_sequencer();

    MidiInControl::my_exit();
    MyInstrument::exit_and_clean_MyInstrument();


#ifdef USE_FLUIDSYNTH

    VST_proc::VST_exit();

    if(fluid_output) {

        delete fluid_output;
        fluid_output = NULL;
    }

    remote_VST_exit();

#endif

    if(file)
        delete file;
}

void MainWindow::mousePressEvent(QMouseEvent* ) {
#ifdef USE_FLUIDSYNTH
    VST_proc::VST_external_show(-1); // force to see external windows
#endif
}

void MainWindow::paintEvent(QPaintEvent* event) {

    QMainWindow::paintEvent(event);

#ifdef CUSTOM_MIDIEDITOR
    // Estwald Color Changes
    static QPixmap p1(":/run_environment/graphics/custom/Midicustom.png");
    static QPixmap p2(":/run_environment/graphics/icon.png");
    static int one = 1;

    if(one) {
        one = 0;

        p2 =  p2.scaled(70, 70);
    }

    QPainter* painter = new QPainter(this);
    if(!painter)
        return;
    int w = painter->window().width();
    if(w < 1280)
        w = 1280;
    painter->drawPixmap(w - 308, 30, p2);
    painter->drawPixmap(w - 240, 20, p1);

    delete painter;
#endif
}

static int decouple_splitter_flag = 0;

void MainWindow::decouple_splitter() {

    int fixedWidth = this->width();

    int idx = mainSplitter->indexOf(rightSplitter);

    decouple_splitter_flag^=1;

    if(decouple_splitter_flag) {

        if (idx != -1) {
            mainSplitter->widget(idx)->hide();
            mainSplitter->widget(idx)->setParent(NULL);
        }

        rightSplitter->setParent(this);
        rightSplitter->setWindowFlags(Qt::Window | Qt::WindowSystemMenuHint |
                                      Qt::WindowTitleHint | Qt::WindowMinimizeButtonHint);

        rightSplitter->setWindowTitle("Tool Tracks/Channels");
        rightSplitter->move(fixedWidth - rightSplitter->width() - 32, 32);

        rightSplitter->setFixedSize(rightSplitter->width(), rightSplitter->height());
        rightSplitter->show();
        rightSplitter->update();

    } else {

        rightSplitter->setParent(mainSplitter);
        mainSplitter->addWidget(rightSplitter);
    }

    leftSplitter->updateGeometry();
    mainSplitter->updateGeometry();

    mainSplitter->update();
    leftSplitter->update();

}

MainWindow::MainWindow(QString initFile)
    : QMainWindow()
    , _initFile(initFile)
{
    file = 0;

    MidiFile *empty = new MidiFile();
    MidiInput::file = empty;
    MidiOutput::file = empty;

#ifdef USE_FLUIDSYNTH
    VST_proc::VST_LeslieReset();
    VST_proc::VST_WahWahReset(44100);
#endif

    tabMatrixWidget = NULL;

    startDirectory = QDir::homePath() + "/Midieditor";
    QDir d;
    d.mkdir(startDirectory);
    d.mkdir(startDirectory + "/settings");
    //_settings = new QSettings(QString("MidiEditor"), QString("NONE"));
    _settings = new QSettings(startDirectory + "/settings/MidiEditor.ini", QSettings::IniFormat);

    Notes_util(this);
    MidiInControl::init_MidiInControl(this, _settings);

    finger_main = new FingerPatternDialog(NULL, _settings, MidiInControl::cur_pairdev); // create timer loop

    EventSplitterTabPos = 2;

    shadow_selection = _settings->value("Main/shadow_selection", true).toBool();

    skipVSTLoad = 0;

    for (int i = 0; i < 516; i++) {
        MidiOutput::file->Bank_MIDI[i]=0;
        MidiOutput::file->Prog_MIDI[i]=0;
        if(i < 17)
            OctaveChan_MIDI[i]=0;
    }

    _moveSelectedEventsToChannelMenu = 0;
    _moveSelectedNotesToChannelMenu = 0;
    _moveSelectedEventsToTrackMenu = 0;

    Appearance::init(_settings);

    if (!itHaveInstrumentList)
        addInstrumentNames();

#ifdef ENABLE_REMOTE
    bool ok;
    int port = _settings->value("Main/udp_client_port", -1).toInt(&ok);
    QString ip = _settings->value("Main/udp_client_ip", "").toString();
#endif
    bool alternativeStop = _settings->value("Main/alt_stop", false).toBool();
    MidiOutput::isAlternativePlayer = alternativeStop;
    bool ticksOK;
    int ticksPerQuarter = _settings->value("Main/ticks_per_quarter", 192).toInt(&ticksOK);
    MidiFile::defaultTimePerQuarter = ticksPerQuarter;
    bool magnet = _settings->value("Main/magnet", false).toBool();
    EventTool::enableMagnet(magnet);

    MidiInput::setThruEnabled(_settings->value("Main/thru", true).toBool());
    Metronome::setEnabled(_settings->value("Main/metronome", false).toBool());
    bool loudnessOk;
    Metronome::setLoudness(_settings->value("Main/metronome_loudness", 100).toInt(&loudnessOk));

#ifdef ENABLE_REMOTE
    _remoteServer = new RemoteServer();
    _remoteServer->setIp(ip);
    _remoteServer->setPort(port);
    _remoteServer->tryConnect();

    connect(_remoteServer, SIGNAL(playRequest()), this, SLOT(play()));
    connect(_remoteServer, SIGNAL(stopRequest(bool, bool)), this, SLOT(stop(bool, bool)));
    connect(_remoteServer, SIGNAL(recordRequest()), this, SLOT(record()));
    connect(_remoteServer, SIGNAL(backRequest()), this, SLOT(back()));
    connect(_remoteServer, SIGNAL(forwardRequest()), this, SLOT(forward()));
    connect(_remoteServer, SIGNAL(pauseRequest()), this, SLOT(pause()));

    connect(MidiPlayer::playerThread(),
        SIGNAL(timeMsChanged(int)), _remoteServer, SLOT(setTime(int)));
    connect(MidiPlayer::playerThread(),
        SIGNAL(meterChanged(int, int)), _remoteServer, SLOT(setMeter(int, int)));
    connect(MidiPlayer::playerThread(),
        SIGNAL(tonalityChanged(int)), _remoteServer, SLOT(setTonality(int)));
    connect(MidiPlayer::playerThread(),
        SIGNAL(measureChanged(int, int)), _remoteServer, SLOT(setMeasure(int)));

#endif
#ifndef CUSTOM_MIDIEDITOR
    UpdateManager::setAutoCheckUpdatesEnabled(_settings->value("auto_update_after_prompt", false).toBool());
    connect(UpdateManager::instance(), SIGNAL(updateDetected(Update*)), this, SLOT(updateDetected(Update*)));
#endif
    _quantizationGrid = _settings->value("Main/quantization", 3).toInt();

    // metronome
    connect(MidiPlayer::playerThread(),
        SIGNAL(measureChanged(int, int)), Metronome::instance(), SLOT(measureUpdate(int, int)));
    connect(MidiPlayer::playerThread(),
        SIGNAL(measureUpdate(int, int)), Metronome::instance(), SLOT(measureUpdate(int, int)));
    connect(MidiPlayer::playerThread(),
        SIGNAL(meterChanged(int, int)), Metronome::instance(), SLOT(meterChanged(int, int)));
    connect(MidiPlayer::playerThread(),
        SIGNAL(playerStopped()), Metronome::instance(), SLOT(playbackStopped()));
    connect(MidiPlayer::playerThread(),
        SIGNAL(playerStarted()), Metronome::instance(), SLOT(playbackStarted()));

    startDirectory = QDir::homePath() + "/Midieditor";
    //QDir d;
    d.mkdir(startDirectory);
    d.mkdir(startDirectory + "/file_cache");

    if (_settings->value("Main/open_path").toString() != "") {
        startDirectory = _settings->value("Main/open_path").toString();
    } else {
        _settings->setValue("Main/open_path", startDirectory);
    }

    // read recent paths
    _recentFilePaths = _settings->value("Main/recent_file_list").toStringList();

    EditorTool::setMainWindow(this);

#ifdef USE_FLUIDSYNTH
    main_widget = this;
#endif

    setWindowTitle(QApplication::applicationName() + " " + QApplication::applicationVersion());
    setWindowIcon(QIcon(":/run_environment/graphics/icon.png"));

#ifdef CUSTOM_MIDIEDITOR_GUI
    // Estwald Color Changes

    QPalette palette = this->palette();
    palette.setBrush(QPalette::Window, background1);
    setPalette(palette);
    setBackgroundRole(QPalette::Window);
    setAutoFillBackground(true);

    QPalette palette2 = this->palette();
    palette2.setBrush(QPalette::Window, background2);
#endif

    QWidget* central = new QWidget(this);
    QGridLayout* centralLayout = new QGridLayout(central);
    centralLayout->setContentsMargins(3, 3, 3, 5);

    // there is a vertical split
    /*QSplitter**/ mainSplitter = new QSplitter(Qt::Horizontal, central);
    //mainSplitter->setHandleWidth(0);

    // The left side
    /*QSplitter**/ leftSplitter = new QSplitter(Qt::Vertical, mainSplitter);
    leftSplitter->setHandleWidth(0);
    mainSplitter->addWidget(leftSplitter);
    leftSplitter->setContentsMargins(0, 0, 0, 0);

    // The right side

    rightSplitter = new QSplitter(Qt::Vertical, mainSplitter);

    mainSplitter->addWidget(rightSplitter);

    // Set the sizes of mainSplitter
    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 0);
    mainSplitter->setContentsMargins(0, 0, 0, 0);

    // the channelWidget and the trackWidget are tabbed
    //QTabWidget*
            upperTabWidget = new QTabWidget(rightSplitter);

    rightSplitter->addWidget(upperTabWidget);
    rightSplitter->setContentsMargins(0, 0, 0, 0);

    // protocolList and EventWidget are tabbed
    lowerTabWidget = new QTabWidget(rightSplitter);

#ifdef CUSTOM_MIDIEDITOR_GUI
    // Estwald Color Changes
    lowerTabWidget->setPalette(palette2);
    lowerTabWidget->setAutoFillBackground(true);
    lowerTabWidget->setBackgroundRole(QPalette::Window);
#endif

    rightSplitter->addWidget(lowerTabWidget);

    // MatrixArea
    matrixArea = new QWidget(leftSplitter);

#ifdef CUSTOM_MIDIEDITOR_GUI
    // Estwald Color Changes
    QPalette paletteM = matrixArea->palette();
    paletteM.setColor(QPalette::Window, QColor(0xc0c0c0));
    setBackgroundRole(QPalette::Window);
    matrixArea->setPalette(paletteM);
    matrixArea->setAutoFillBackground(true);
#endif

    leftSplitter->addWidget(matrixArea);
    matrixArea->setContentsMargins(0, 0, 0, 0);
    mw_matrixWidget = new MatrixWidget(matrixArea);
    mw_matrixWidget->bpm_rhythm_ms = _settings->value("Main/get_bpm_ms", 250).toInt();
    mw_matrixWidget->visible_Controlflag = _settings->value("Main/visible_Controlflag", true).toBool();
    mw_matrixWidget->visible_PitchBendflag = _settings->value("Main/visible_PitchBendflag", true).toBool();
    mw_matrixWidget->visible_TimeLineArea3 = _settings->value("Main/visible_TimeLineArea3", true).toBool();
    mw_matrixWidget->visible_TimeLineArea4 = _settings->value("Main/visible_TimeLineArea4", true).toBool();
    mw_matrixWidget->visible_karaoke = _settings->value("Main/visible_karaoke", true).toBool();
    mw_matrixWidget->shadow_selection = shadow_selection;

    vert = new QScrollBar(Qt::Vertical, matrixArea);

    vert->setVisible(false);
#ifdef CUSTOM_MIDIEDITOR_GUI
    // Estwald Color Changes

    QString scrollSS = QString::fromUtf8("QScrollBar {color: black; background-color: #a0a070;} \n");
    vert->setStyleSheet(scrollSS);
#endif

    QGridLayout* matrixAreaLayout = new QGridLayout(matrixArea);
    matrixAreaLayout->setHorizontalSpacing(6);
    QWidget* placeholder0 = new QWidget(matrixArea);
    placeholder0->setFixedHeight(50);
    matrixAreaLayout->setContentsMargins(0, 0, 0, 0);
    matrixAreaLayout->addWidget(mw_matrixWidget, 0, 0, 2, 1);
    matrixAreaLayout->addWidget(placeholder0, 0, 1, 1, 1);
    matrixAreaLayout->addWidget(vert, 1, 1, 1, 1);
    matrixAreaLayout->setColumnStretch(0, 1);
    matrixArea->setLayout(matrixAreaLayout);

    bool screenLocked = _settings->value("Main/screen_locked", false).toBool();
    mw_matrixWidget->setScreenLocked(screenLocked);
    int div = _settings->value("Main/div", 2).toInt();
    mw_matrixWidget->setDiv(div);

    // VelocityArea
    velocityArea = new QWidget(leftSplitter);

#ifdef CUSTOM_MIDIEDITOR_GUI
    // Estwald Color Changes
    QPalette paletteV = velocityArea->palette();
    paletteV.setColor(QPalette::Window, QColor(0xc0c0c0));
    paletteV.setColor(QPalette::Text, QColor(0x303030));
    paletteV.setColor(QPalette::Base, Qt::white);
    velocityArea->setPalette(paletteV);
    setBackgroundRole(QPalette::Window);
    velocityArea->setAutoFillBackground(true);
#endif

    velocityArea->setContentsMargins(0, 0, 0, 0);
    leftSplitter->addWidget(velocityArea);
    hori = new QScrollBar(Qt::Horizontal, velocityArea);

    hori->setVisible(false);

#ifdef CUSTOM_MIDIEDITOR_GUI
    // Estwald Color Changes
    hori->setStyleSheet(scrollSS);
#endif

    velocityArea->setVisible(false);
    hori->setSingleStep(500);
    hori->setPageStep(5000);
    QGridLayout* velocityAreaLayout = new QGridLayout(velocityArea);
    velocityAreaLayout->setContentsMargins(0, 0, 0, 0);
    velocityAreaLayout->setHorizontalSpacing(6);
    QWidget *_miscWidgetControl = new QWidget(velocityArea);
    _miscWidgetControl->setFixedWidth(110 - velocityAreaLayout->horizontalSpacing());
    velocityAreaLayout->addWidget(_miscWidgetControl, 0, 0, 1, 1);


    QGroupBox *_miscBackground = new QGroupBox(_miscWidgetControl);
    _miscBackground->setStyleSheet(QString::fromUtf8("background-color: #E5E5E5;"));
   // _miscWidgetControl->setObjectName(QString::fromUtf8("groupBox"));

    _miscBackground->setGeometry(QRect(4, 4, 110 - velocityAreaLayout->horizontalSpacing() - 8, 768));

    // there is a Scrollbar on the right side of the velocityWidget doing
    // nothing but making the VelocityWidget as big as the matrixWidget
    QScrollBar* scrollNothing = new QScrollBar(Qt::Vertical, velocityArea);

#ifdef CUSTOM_MIDIEDITOR_GUI
    // Estwald Color Changes
    scrollNothing->setStyleSheet(scrollSS);
#endif

    scrollNothing->setMinimum(0);
    scrollNothing->setMaximum(0);
    velocityAreaLayout->addWidget(scrollNothing, 0, 2, 1, 1);
    velocityAreaLayout->addWidget(hori, 1, 1, 1, 1);
    velocityAreaLayout->setRowStretch(0, 1);
    velocityArea->setLayout(velocityAreaLayout);

    tabMatrixWidget = new QTabWidget(mw_matrixWidget);

    tabMatrixWidget->setObjectName(QString::fromUtf8("tabMatrixWidget"));
    tabMatrixWidget->setContentsMargins(1, 1, 1, 1);

    QString style = QString::fromUtf8(
                "QTabWidget::pane { border-top: 2px solid #C2C7CB;}\n"
                "QTabWidget::tab-bar {left: 5px; }\n"
                "QTabBar::tab { background: qlineargradient(x1: 0, y1: 0, x2: 0.5, y2: 0.5, stop: 0 #FFD2BB, stop: 1 #f0f0c0);\n"
                "border: 2px solid #C4C4C3; border-bottom-color: #C2C7CB;\n"
                "min-width: 39px; padding: 2px; font: bold 9pt 'Arial';}\n"
                "QTabBar::tab:selected {color: #0000ff; min-width: 39px; font: bold 9pt 'Arial';\n"
                "border-color: #000000; border-bottom-color: #000000; \n"
                "background: qlineargradient(x1: 0, y1: 0, x2: 0.5, y2: 0.5, stop: 0 #62FF62, stop: 1 #c0ffc0);}\n"
                "QTabBar::tab:!selected {color: #000000; margin-top: 2px;}\n"

                "QTabBar::tab:disabled {color: #c0c0c0; background: #f0f0f0; margin-top: 2px;}\n");

    tabMatrixWidget->setStyleSheet(style);


    _miscWidget = new MiscWidget(mw_matrixWidget, tabMatrixWidget/*velocityArea*/);
    _miscWidget->setContentsMargins(0, 0, 0, 0);

    QFont f = tabMatrixWidget->tabBar()->font();
    f.setBold(true);
    tabMatrixWidget->tabBar()->setFont(f);
    tabMatrixWidget->addTab(_miscWidget , "Tracks");
    tabMatrixWidget->setUsesScrollButtons(false);
    tabMatrixWidget->setDocumentMode(true);
    tabMatrixWidgetMode = 0;

    _disableWithPlay.append(tabMatrixWidget);

    for(int n = 0; n < 16; n++) {

        tabMatrixWidget->tabBar()->insertTab(0, QString::number(15 - n));
        tabMatrixWidget->tabBar()->setTabEnabled(0, false);

    }

    tabMatrixWidget->tabBar()->addTab("Tracks+");
    tabMatrixWidget->tabBar()->setTabVisible(17, false);

    tabMatrixWidget->setCurrentIndex(16);
    tabMatrixWidget->tabBar()->setCurrentIndex(0);

    connect(tabMatrixWidget, static_cast<void (QTabWidget::*)(int)>(&QTabWidget::currentChanged), [=](int index)
    {
        //qWarning("tabMatrixWidget %i %i %i", tabMatrixWidgetMode, index, file->numTracks());
        if(tabMatrixWidgetMode != 2 && index >= file->numTracks() &&  index < 16)
            index = file->numTracks() - 1;

        if((tabMatrixWidgetMode != 2 && index >= file->numTracks()) || index >= 16) {

            if(index == 16) {

                if(tabMatrixWidgetMode == 0) {

                    tabMatrixWidgetMode = 2;
                    tabMatrixWidget->tabBar()->setTabText(16, "Channels");
                    upperTabWidget->setCurrentIndex(1);

                } else if(tabMatrixWidgetMode == 1) {

                    tabMatrixWidgetMode = 0;
                    tabMatrixWidget->tabBar()->setTabText(16, "Tracks");
                    upperTabWidget->setCurrentIndex(0);

                } else {

                    tabMatrixWidgetMode = 0;
                    if(NewNoteTool::editTrack() >= 16)
                        tabMatrixWidgetMode = 1;

                    tabMatrixWidget->tabBar()->setTabText(16, "Tracks");
                    upperTabWidget->setCurrentIndex(0);

                }

            } else if(index == 17) {

                if(tabMatrixWidgetMode == 1) {

                    tabMatrixWidgetMode = 2;
                    tabMatrixWidget->tabBar()->setTabText(16, "Channels");
                    upperTabWidget->setCurrentIndex(1);

                } else if(tabMatrixWidgetMode == 0) {

                    tabMatrixWidgetMode = 1;
                    tabMatrixWidget->tabBar()->setTabText(16, "Tracks");
                    upperTabWidget->setCurrentIndex(0);

                } else {

                    tabMatrixWidgetMode = 1;
                    tabMatrixWidget->tabBar()->setTabText(16, "Tracks");
                    upperTabWidget->setCurrentIndex(0);

                }
            }

            if(tabMatrixWidgetMode == 0) {

                index = NewNoteTool::editTrack();

                if(index >= 16) index = 0;
                index &= 15;

                tabMatrixWidget->setCurrentIndex(index & 15);

                NewNoteTool::setEditTrack(index);
                editTrack(index);
                updateTabMatrixWidget();
                msDelay(500);
                return;

            } else if(tabMatrixWidgetMode == 1) {

                index = 16;
                if(index >= file->numTracks()) {
                    index = 0;
                    tabMatrixWidgetMode = 0;
                }

                tabMatrixWidget->setCurrentIndex(index & 15);

                NewNoteTool::setEditTrack(index);
                editTrack(index);
                updateTabMatrixWidget();
                msDelay(500);

                return;


            } else if(tabMatrixWidgetMode == 2) {

                index = NewNoteTool::editChannel();
                tabMatrixWidget->setCurrentIndex(index);

                editChannel(index, true, NewNoteTool::editTrack());
                updateTabMatrixWidget();
                msDelay(500);

                return;
            }

            msDelay(500);

            return;
        }

        if(tabMatrixWidgetMode == 0) {

            if(index >= file->numTracks())
                index = 0;

            NewNoteTool::setEditTrack(index);
            editTrack(index);

        } else if(tabMatrixWidgetMode == 1) {

            index = index + 16;

            if(index >= file->numTracks())
                index = 0;

            NewNoteTool::setEditTrack(index);
            editTrack(index);

        } else if(tabMatrixWidgetMode == 2) {

            NewNoteTool::setEditChannel(index);
            editChannel(index, true, NewNoteTool::editTrack());
        }

        msDelay(500);

    });

    connect(upperTabWidget, static_cast<void (QTabWidget::*)(int)>(&QTabWidget::tabBarClicked), [=](int index)
    {

        if(index == 1) {
            tabMatrixWidgetMode = 2;
            tabMatrixWidget->tabBar()->setTabText(16, "Channels");
            upperTabWidget->setCurrentIndex(1);
            NewNoteTool::setEditChannel(0);
            editChannel(0, true, NewNoteTool::editTrack());
            tabMatrixWidget->tabBar()->setCurrentIndex(0);
            updateTabMatrixWidget();

        }

    });


    connect(tabMatrixWidget, static_cast<void (QTabWidget::*)(int)>(&QTabWidget::tabBarDoubleClicked), [=](int index)
    {

        if(tabMatrixWidgetMode == 2 && index < 16) {
            Tool::selectCurrentChanOnly^= true;
            NewNoteTool::setEditChannel(index);
            editChannel(index, true, NewNoteTool::editTrack());
        }

        if(tabMatrixWidgetMode != 2 && index < 16) {
            tabMatrixWidgetMode = 2;
            tabMatrixWidget->tabBar()->setTabText(16, "Channels");
            upperTabWidget->setCurrentIndex(1);
            NewNoteTool::setEditChannel(0);
            editChannel(0, true, NewNoteTool::editTrack());
            tabMatrixWidget->tabBar()->setCurrentIndex(0);
            updateTabMatrixWidget();

        }

        msDelay(500);

    });

    connect(this, SIGNAL(tabMatrixWidgetIndex(int)), tabMatrixWidget, SLOT(setCurrentIndex(int)));

    velocityAreaLayout->addWidget(tabMatrixWidget, 0, 1, 1, 1);

    // controls for velocity widget
    _miscControlLayout = new QGridLayout(_miscWidgetControl);
    _miscControlLayout->setHorizontalSpacing(0);

    _miscWidgetControl->setLayout(_miscControlLayout);
    _miscMode = new QComboBox(_miscBackground/*_miscWidgetControl*/);
    for (int i = 0; i < MiscModeEnd; i++) {
        _miscMode->addItem(MiscWidget::modeToString(i));
    }
    _miscMode->view()->setMinimumWidth(_miscMode->minimumSizeHint().width());
    //_miscControlLayout->addWidget(new QLabel("Mode:", _miscWidgetControl), 0, 0, 1, 3);
    _miscControlLayout->addWidget(_miscMode, 1, 0, 1, 3);
    connect(_miscMode, SIGNAL(currentIndexChanged(int)), this, SLOT(changeMiscMode(int)));

    //_miscControlLayout->addWidget(new QLabel("Control:", _miscWidgetControl), 2, 0, 1, 3);
    _miscController = new QComboBox(_miscBackground/*_miscWidgetControl*/);
    for (int i = 0; i < 128; i++) {
        _miscController->addItem(MidiFile::controlChangeName(i));
    }
    _miscController->view()->setMinimumWidth(_miscController->minimumSizeHint().width());
    _miscControlLayout->addWidget(_miscController, 3, 0, 1, 3);
    connect(_miscController, SIGNAL(currentIndexChanged(int)), _miscWidget, SLOT(setControl(int)));

    //_miscControlLayout->addWidget(new QLabel("Channel:", _miscWidgetControl), 4, 0, 1, 3);
    _miscChannel = new QComboBox(_miscBackground/*_miscWidgetControl*/);
    for (int i = 0; i < 15; i++) {
        _miscChannel->addItem("Channel " + QString::number(i));
    }
    _miscChannel->view()->setMinimumWidth(_miscChannel->minimumSizeHint().width());
    _miscControlLayout->addWidget(_miscChannel, 5, 0, 1, 3);
    connect(_miscChannel, SIGNAL(currentIndexChanged(int)), _miscWidget, SLOT(setChannel(int)));
    //_miscControlLayout->setRowStretch(6, 1);
    _miscMode->setCurrentIndex(0);
    _miscChannel->setEnabled(false);
    _miscController->setEnabled(false);

    setSingleMode = new QAction(QIcon(":/run_environment/graphics/tool/misc_single.png"), "Single mode", this);
    setSingleMode->setCheckable(true);
    setFreehandMode = new QAction(QIcon(":/run_environment/graphics/tool/misc_freehand.png"), "Free-hand mode", this);
    setFreehandMode->setCheckable(true);
    setLineMode = new QAction(QIcon(":/run_environment/graphics/tool/misc_line.png"), "Line mode", this);
    setLineMode->setCheckable(true);

    QActionGroup* group = new QActionGroup(this);
    group->setExclusive(true);
    group->addAction(setSingleMode);
    group->addAction(setFreehandMode);
    group->addAction(setLineMode);
    setSingleMode->setChecked(true);
    connect(group, SIGNAL(triggered(QAction*)), this, SLOT(selectModeChanged(QAction*)));

    QToolButton* btnSingle = new QToolButton(_miscBackground /*_miscWidgetControl*/);
    btnSingle->setDefaultAction(setSingleMode);
    QToolButton* btnHand = new QToolButton(_miscWidgetControl);
    btnHand->setDefaultAction(setFreehandMode);
    QToolButton* btnLine = new QToolButton(_miscWidgetControl);
    btnLine->setDefaultAction(setLineMode);

    _miscControlLayout->addWidget(btnSingle, 9, 0, 1, 1);
    _miscControlLayout->addWidget(btnHand, 9, 1, 1, 1);
    _miscControlLayout->addWidget(btnLine, 9, 2, 1, 1);
    _miscControlLayout->setRowStretch(10, 1);

    // Set the sizes of leftSplitter
    leftSplitter->setStretchFactor(0, 8);
    leftSplitter->setStretchFactor(1, 1);

    // Track
    QWidget* tracks = new QWidget(upperTabWidget);

#ifdef CUSTOM_MIDIEDITOR_GUI
    // Estwald Color Changes
    tracks->setPalette(palette2);
    tracks->setBackgroundRole(QPalette::Window);
    tracks->setAutoFillBackground(true);
#endif

    QGridLayout* tracksLayout = new QGridLayout(tracks);
    tracks->setLayout(tracksLayout);
    QToolBar* tracksTB = new QToolBar(tracks);
    tracksTB->setIconSize(QSize(20, 20));
    tracksLayout->addWidget(tracksTB, 0, 0, 1, 1);

    QAction* newTrack = new QAction("Add track", this);
    newTrack->setIcon(QIcon(":/run_environment/graphics/tool/add.png"));
    connect(newTrack, SIGNAL(triggered()), this,
        SLOT(addTrack()));
    tracksTB->addAction(newTrack);

    tracksTB->addSeparator();

    _allTracksAudible = new QAction("All tracks audible", this);
    _allTracksAudible->setIcon(QIcon(":/run_environment/graphics/tool/all_audible.png"));
    connect(_allTracksAudible, SIGNAL(triggered()), this,
        SLOT(unmuteAllTracks()));
    tracksTB->addAction(_allTracksAudible);

    _allTracksMute = new QAction("Mute all tracks", this);
    _allTracksMute->setIcon(QIcon(":/run_environment/graphics/tool/all_mute.png"));
    connect(_allTracksMute, SIGNAL(triggered()), this,
        SLOT(muteAllTracks()));
    tracksTB->addAction(_allTracksMute);

    tracksTB->addSeparator();

    _allTracksVisible = new QAction("Show all tracks", this);
    _allTracksVisible->setIcon(QIcon(":/run_environment/graphics/tool/all_visible.png"));
    connect(_allTracksVisible, SIGNAL(triggered()), this,
        SLOT(allTracksVisible()));
    tracksTB->addAction(_allTracksVisible);

    _allTracksInvisible = new QAction("Hide all tracks", this);
    _allTracksInvisible->setIcon(QIcon(":/run_environment/graphics/tool/all_invisible.png"));
    connect(_allTracksInvisible, SIGNAL(triggered()), this,
        SLOT(allTracksInvisible()));
    tracksTB->addAction(_allTracksInvisible);

    _trackWidget = new TrackListWidget(tracks);
    connect(_trackWidget, SIGNAL(trackRenameClicked(int)), this, SLOT(renameTrack(int)), Qt::QueuedConnection);
    connect(_trackWidget, SIGNAL(trackRemoveClicked(int)), this, SLOT(removeTrack(int)), Qt::QueuedConnection);
    connect(_trackWidget, SIGNAL(trackClicked(MidiTrack*)), this, SLOT(editTrackAndChannel(MidiTrack*)), Qt::QueuedConnection);

    tracksLayout->addWidget(_trackWidget, 1, 0, 1, 1);
    upperTabWidget->addTab(tracks, "Tracks");

    // Channels
    QWidget* channels = new QWidget(upperTabWidget);

#ifdef CUSTOM_MIDIEDITOR_GUI
    // Estwald Color Changes
    channels->setPalette(palette2);
    channels->setAutoFillBackground(true);
    channels->setBackgroundRole(QPalette::Window);
#endif
 
    QGridLayout* channelsLayout = new QGridLayout(channels);
    channels->setLayout(channelsLayout);
    QToolBar* channelsTB = new QToolBar(channels);
    channelsTB->setIconSize(QSize(20, 20));
    channelsLayout->addWidget(channelsTB, 0, 0, 1, 1);

    _allChannelsAudible = new QAction("All channels audible", this);
    _allChannelsAudible->setIcon(QIcon(":/run_environment/graphics/tool/all_audible.png"));
    connect(_allChannelsAudible, SIGNAL(triggered()), this,
        SLOT(unmuteAllChannels()));
    channelsTB->addAction(_allChannelsAudible);

    _allChannelsMute = new QAction("Mute all channels", this);
    _allChannelsMute->setIcon(QIcon(":/run_environment/graphics/tool/all_mute.png"));
    connect(_allChannelsMute, SIGNAL(triggered()), this,
        SLOT(muteAllChannels()));
    channelsTB->addAction(_allChannelsMute);

    channelsTB->addSeparator();

    _allChannelsVisible = new QAction("Show all channels", this);
    _allChannelsVisible->setIcon(QIcon(":/run_environment/graphics/tool/all_visible.png"));
    connect(_allChannelsVisible, SIGNAL(triggered()), this,
        SLOT(allChannelsVisible()));
    channelsTB->addAction(_allChannelsVisible);

    _allChannelsInvisible = new QAction("Hide all channels", this);
    _allChannelsInvisible->setIcon(QIcon(":/run_environment/graphics/tool/all_invisible.png"));
    connect(_allChannelsInvisible, SIGNAL(triggered()), this,
        SLOT(allChannelsInvisible()));
    channelsTB->addAction(_allChannelsInvisible);

    channelWidget = new ChannelListWidget(channels);
    connect(channelWidget, SIGNAL(channelChanged()), this, SLOT(updateChannel()), Qt::QueuedConnection);
    connect(channelWidget, SIGNAL(channelStateChanged()), this, SLOT(updateChannelMenu()), Qt::QueuedConnection);
    connect(channelWidget, SIGNAL(selectInstrumentClicked(int)), this, SLOT(setInstrumentForChannel(int)), Qt::QueuedConnection);
    connect(channelWidget, SIGNAL(selectSoundEffectClicked(int)), this, SLOT(setSoundEffectForChannel(int)), Qt::QueuedConnection);
#ifdef USE_FLUIDSYNTH
    connect(channelWidget, SIGNAL(LoadVSTClicked(int, int)), this, SLOT(setLoadVSTForChannel(int, int)), Qt::QueuedConnection);
    connect(this, SIGNAL(ToggleViewVST(int, bool)), channelWidget, SLOT(ToggleViewVST(int, bool)), Qt::QueuedConnection);

#endif
    channelsLayout->addWidget(channelWidget, 1, 0, 1, 1);
    upperTabWidget->addTab(channels, "Channels");
    upperTabWidget->setCurrentIndex(1);// set channels by default

    QString in_ports[MAX_INPUT_DEVICES];

    for(int n = 0; n < MAX_INPUT_DEVICES; n++) {
        in_ports[n] = (n == 0) ? _settings->value("Midi/in_port", "").toString()
                                : _settings->value("Midi/in_port" + QString::number(n), "").toString();
        //qWarning("in_port %i %s", n, in_ports[n].toLocal8Bit().constData());

    }

    QString out_ports[MAX_OUTPUT_DEVICES];

    for(int n = 0; n < MAX_OUTPUT_DEVICES; n++) {
        out_ports[n] = (n == 0) ? _settings->value("Midi/out_port", "").toString()
                                : _settings->value("Midi/out_port" + QString::number(n), "").toString();
        //qWarning("out_port %i %s", n, out_ports[n].toLocal8Bit().constData());

    }

    MidiOutput::AllTracksToOne = _settings->value("Midi/AllTracksToOne", false).toBool();
    MidiOutput::FluidSynthTracksAuto = _settings->value("Midi/FluidSynthTracksAuto", true).toBool();
    MidiOutput::SaveMidiOutDatas = _settings->value("Midi/SaveMidiOutDatas", false).toBool();

    MidiOutput::update_config();

    // terminal
    Terminal::initTerminal(_settings->value("Main/start_cmd", "").toString(),
        in_ports,
        out_ports);

    //upperTabWidget->addTab(Terminal::terminal()->console(), "Terminal");

    // decouple button

    QPushButton *decoupleBtn = new QPushButton;
    decoupleBtn->setIcon(QIcon(":/run_environment/graphics/channelwidget/decouple.png"));
    decoupleBtn->setFixedSize(48, 24);

    QWidget *buttonContainer = new QWidget(upperTabWidget);
    QHBoxLayout *layout = new QHBoxLayout(buttonContainer);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(decoupleBtn);
    layout->setAlignment(decoupleBtn, Qt::AlignRight | Qt::AlignTop);

    buttonContainer->move(270, 0);
    connect(decoupleBtn, &QPushButton::clicked, this, &MainWindow::decouple_splitter);


    // Protocollist
    protocolWidget = new ProtocolWidget(lowerTabWidget);

    lowerTabWidget->addTab(protocolWidget, "Protocol");

    _eventWidget = new EventWidget(upperTabWidget);
    Selection::_eventWidget = _eventWidget;
    EventSplitterTabPos = upperTabWidget->count();
    upperTabWidget->addTab(_eventWidget, "Event");
    MidiEvent::setEventWidget(_eventWidget);

    _disableLoading.append(_miscWidget);
    _disableLoading.append(channelWidget);
    _disableLoading.append(protocolWidget);
    _disableLoading.append(mw_matrixWidget);
    _disableLoading.append(_trackWidget);
    _disableLoading.append(_eventWidget);

#ifdef CUSTOM_MIDIEDITOR_GUI
    // Estwald Color Changes
    palette2.setBrush(QPalette::Base, background1);
    _eventWidget->setPalette(palette2);
    _eventWidget->setBackgroundRole(QPalette::Window);
    _eventWidget->setAutoFillBackground(true);
#endif

    // below add two rows for choosing track/channel new events shall be assigned to
    QWidget* chooser = new QWidget(rightSplitter);
    chooser->setMinimumWidth(350);

#ifdef CUSTOM_MIDIEDITOR_GUI
    // Estwald Color Changes
    chooser->setPalette(palette2);
    chooser->setBackgroundRole(QPalette::Window);
    chooser->setAutoFillBackground(true);
#endif

    rightSplitter->addWidget(chooser);
    QGridLayout* chooserLayout = new QGridLayout(chooser);
    QLabel* trackchannelLabel = new QLabel("Add new events to ...");
    chooserLayout->addWidget(trackchannelLabel, 0, 0, 1, 2);
    QLabel* channelLabel = new QLabel("Channel: ", chooser);
    chooserLayout->addWidget(channelLabel, 2, 0, 1, 1);
    _chooseEditChannel = new QComboBox(chooser);
    for (int i = 0; i < 16; i++) {
        _chooseEditChannel->addItem("Channel " + QString::number(i));
    }
    connect(_chooseEditChannel, SIGNAL(activated(int)), this, SLOT(editChannel(int)));

    chooserLayout->addWidget(_chooseEditChannel, 2, 1, 1, 1);
    QLabel* trackLabel = new QLabel("Track: ", chooser);
    chooserLayout->addWidget(trackLabel, 1, 0, 1, 1);
    _chooseEditTrack = new QComboBox(chooser);
    chooserLayout->addWidget(_chooseEditTrack, 1, 1, 1, 1);
    connect(_chooseEditTrack, SIGNAL(activated(int)), this, SLOT(editTrack(int)));
    chooserLayout->setColumnStretch(1, 1);
    // connect Scrollbars and Widgets
    connect(vert, SIGNAL(valueChanged(int)), mw_matrixWidget,
        SLOT(scrollYChanged(int)));
    connect(hori, SIGNAL(valueChanged(int)), mw_matrixWidget,
        SLOT(scrollXChanged(int)));

    connect(channelWidget, SIGNAL(channelStateChanged()), mw_matrixWidget,
        SLOT(repaint()));
    /*
    connect(mw_matrixWidget, SIGNAL(sizeChanged(int, int, int, int)), this,
        SLOT(matrixSizeChanged(int, int, int, int)));

    connect(mw_matrixWidget, SIGNAL(scrollChanged(int, int, int, int)), this,
        SLOT(scrollPositionsChanged(int, int, int, int)));
    */
    setCentralWidget(central);

#ifdef CUSTOM_MIDIEDITOR_GUI
    // Estwald Color Changes
    QFont font;
    font.setPointSize(11);
    font.setBold(true);
    menuBar()->setFont(font);
    menuBar()->setStyleSheet(QString::fromUtf8(
    "QMenuBar {color: white; background: #401080; border: 1px solid black;}\n"));
#endif

    QWidget* buttons = setupActions(central);

    rightSplitter->setStretchFactor(0, 8);
    rightSplitter->setStretchFactor(1, 2);

    // Add the Widgets to the central Layout
    centralLayout->setSpacing(0);
    centralLayout->addWidget(buttons, 0, 0);
    centralLayout->addWidget(mainSplitter, 1, 0);
    centralLayout->setRowStretch(1, 1);
    central->setLayout(centralLayout);

    if (_settings->value("Main/colors_from_channel", true).toBool()) {
        colorsByChannel();
    } else {
        colorsByTrack();
    }
    copiedEventsChanged();
    setAcceptDrops(true);

    currentTweakTarget = new TimeTweakTarget(this);
    selectionNavigator = new SelectionNavigator(this);

    QTimer::singleShot(250, this, SLOT(loadInitFile()));
#ifndef CUSTOM_MIDIEDITOR
    if (UpdateManager::autoCheckForUpdates()) {
       // QTimer::singleShot(500, UpdateManager::instance(), SLOT(checkForUpdates()));
    }

    // display dialogs, depending on number of starts of the software
    bool ok;
    int numStart = _settings->value("Main/numStart_v3.5", -1).toInt(&ok);
    if (numStart == 15) {
        QTimer::singleShot(300, this, SLOT(donate()));
    }
    if (numStart == 10 && !UpdateManager::autoCheckForUpdates()) {
        QTimer::singleShot(300, this, SLOT(promtUpdatesDeactivatedDialog()));
    }
#endif


}


void MainWindow::updateChannel()
{
    MainWindow::editChannel(NewNoteTool::editChannel());

}

void MainWindow::msDelay(int ms) {

    qint64 one = QDateTime::currentMSecsSinceEpoch();
    qint64 diff;
    do {

        QCoreApplication::processEvents();

        diff = QDateTime::currentMSecsSinceEpoch() - one;

    } while(diff < ms);

}

void MainWindow::loadInitFile()
{
    bool restore_step = false;
    bool restore_load = false;
    msDelay(250);

    // splitter correction
    leftSplitter->setFixedWidth(width() - rightSplitter->width());

    if(MidieditorMaster) {
        QFile c(QDir::homePath() + "/Midieditor/file_cache/_anti_crash_");
        if(c.exists()) {
            if(QMessageBox::question(this, "MidiEditor", "Midieditor closed unexpectedly the last time.\nDo you want to recover the last copied file?") == QMessageBox::Yes) {
                hori->setVisible(true);
                vert->setVisible(true);
                velocityArea->setVisible(true);
#ifdef CUSTOM_MIDIEDITOR_GUI
                // Estwald Color Changes
                QPalette paletteM = matrixArea->palette();
                //paletteM.setColor(QPalette::Window, QColor(0x303030));
                paletteM.setBrush(QPalette::Window, background3);
                matrixArea->setPalette(paletteM);
                matrixArea->setBackgroundRole(QPalette::Window);
                matrixArea->setAutoFillBackground(true);

                QPalette paletteV = velocityArea->palette();
                //paletteV.setColor(QPalette::Window, QColor(0x303030));
                paletteV.setBrush(QPalette::Window, background3);
                paletteV.setColor(QPalette::Text, QColor(0x303030));
                paletteV.setColor(QPalette::Base, Qt::white);
                velocityArea->setPalette(paletteV);
                velocityArea->setBackgroundRole(QPalette::Window);
                velocityArea->setAutoFillBackground(true);
                restore_step = true;

#endif

                if(restore_backup(1))
                    restore_load = true;

            }
        }

    }

    hori->setVisible(true);
    vert->setVisible(true);
    velocityArea->setVisible(true);

#ifdef CUSTOM_MIDIEDITOR_GUI
    // Estwald Color Changes

    if(!restore_step) {

        QPalette paletteM = matrixArea->palette();
        //paletteM.setColor(QPalette::Window, QColor(0x303030));
        paletteM.setBrush(QPalette::Window, background3);
        matrixArea->setPalette(paletteM);
        matrixArea->setBackgroundRole(QPalette::Window);
        matrixArea->setAutoFillBackground(true);

        QPalette paletteV = velocityArea->palette();
        paletteV.setColor(QPalette::Window, QColor(0x303030));
        paletteV.setBrush(QPalette::Window, background3);
        paletteV.setColor(QPalette::Text, QColor(0x303030)); 
        paletteV.setColor(QPalette::Base, Qt::white);
        velocityArea->setPalette(paletteV);
        velocityArea->setBackgroundRole(QPalette::Window);
        velocityArea->setAutoFillBackground(true);

    }

#endif

    if(!restore_load) {
        if (_initFile != "")
            loadFile(_initFile);
        else {
            newFile();
        }
    }

#ifdef USE_FLUIDSYNTH
    connect(this, SIGNAL(signal_remote_VST()), this, SLOT(remote_VST()), Qt::BlockingQueuedConnection);
#endif

}

void MainWindow::dropEvent(QDropEvent* ev)
{
    QList<QUrl> urls = ev->mimeData()->urls();
    foreach (QUrl url, urls) {
        QString newFile = url.toLocalFile();
        if (!newFile.isEmpty()) {
            loadFile(newFile);
            break;
        }
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent* ev)
{
    ev->accept();
}

void MainWindow::scrollPositionsChanged(int startMs, int maxMs, int startLine,
    int maxLine)
{
    hori->setMaximum(maxMs);
    hori->setValue(startMs);
    vert->setMaximum(maxLine);
    vert->setValue(startLine);
}

void MainWindow::setFile(MidiFile* file)
{
    MidiFile* old_file = this->file;

    // don?t paste destroyed old MidiFile...
    EventTool::copiedEvents->clear();

    tabMatrixWidgetMode = 0;
    updateTabMatrixWidget();

#ifdef USE_FLUIDSYNTH
    if(fluid_output) {
        fluid_output->_file = file;
        MidiOutput::forceDrum(file->DrumUseCh9);

    }

#endif

    EventTool::clearSelection();
    stdTool->buttonClick();

    disconnect(mw_matrixWidget, SIGNAL(sizeChanged(int, int, int, int)), this,
        SLOT(matrixSizeChanged(int, int, int, int)));

    disconnect(mw_matrixWidget, SIGNAL(scrollChanged(int, int, int, int)), this,
        SLOT(scrollPositionsChanged(int, int, int, int)));

    // destroy old file connection
    if(this->file && this->file != file && this->file != NULL) {
        disconnect(this->file->protocol(), SIGNAL(actionFinished()), protocolWidget, SLOT(protocolChanged()));
        disconnect(this->file->protocol(), SIGNAL(actionFinished()), _trackWidget, SLOT(update()));

        disconnect(this->file, SIGNAL(trackChanged()), this, SLOT(updateTrackMenu()));
        disconnect(this->file, SIGNAL(cursorPositionChanged()), channelWidget, SLOT(update()));
        disconnect(this->file, SIGNAL(recalcWidgetSize()), mw_matrixWidget, SLOT(calcSizes()));
        disconnect(this->file->protocol(), SIGNAL(actionFinished()), this, SLOT(markEdited()));
        disconnect(this->file->protocol(), SIGNAL(actionFinished()), eventWidget(), SLOT(reload()));
        disconnect(this->file->protocol(), SIGNAL(actionFinished()), this, SLOT(checkEnableActionsForSelection()));

        QCoreApplication::processEvents();
    }

    if(!file) {
        mw_matrixWidget->disable_ev = false;
        return;
    }

    Selection::setFile(file);
    Metronome::instance()->setFile(file);
    protocolWidget->setFile(file);
    channelWidget->setFile(file);
    _trackWidget->setFile(file);

#ifdef ENABLE_REMOTE
    _remoteServer->setFile(file);
#endif
    eventWidget()->setFile(file);

    Tool::setFile(file);
    this->file = file;

    connect(file, SIGNAL(trackChanged()), this, SLOT(updateTrackMenu()));
    setWindowTitle(QApplication::applicationName() + " - " + file->path() + "[*]");
    connect(file, SIGNAL(cursorPositionChanged()), channelWidget, SLOT(update()));
    connect(file, SIGNAL(recalcWidgetSize()), mw_matrixWidget, SLOT(calcSizes()));
    connect(file->protocol(), SIGNAL(actionFinished()), this, SLOT(markEdited()));
    connect(file->protocol(), SIGNAL(actionFinished()), eventWidget(), SLOT(reload()));
    connect(file->protocol(), SIGNAL(actionFinished()), this, SLOT(checkEnableActionsForSelection()));

    mw_matrixWidget->registerRelayout();
    mw_matrixWidget->disable_ev = false;
    mw_matrixWidget->setFile(file);

    DELETE(old_file); // destroy old file

    connect(mw_matrixWidget, SIGNAL(sizeChanged(int, int, int, int)), this,
        SLOT(matrixSizeChanged(int, int, int, int)));

    connect(mw_matrixWidget, SIGNAL(scrollChanged(int, int, int, int)), this,
        SLOT(scrollPositionsChanged(int, int, int, int)));

    mw_matrixWidget->repaint();

    file->blockSignals(false);

    updateChannelMenu();
    updateTrackMenu();

    mw_matrixWidget->update();
    _miscWidget->update();

    checkEnableActionsForSelection();

#ifdef USE_FLUIDSYNTH
    VST_proc::VST_setParent(this);

    if(!skipVSTLoad) {
        VST_proc::VST_LoadfromMIDIfile();
        msDelay(2000); // time to loading VST modules
    }
    else {

        for(int i = 0; i < PRE_CHAN; i++)
            if((((skipVSTLoad >> (i & 15)) & 1) != 0))
                VST_proc::VST_unload(i);

    }

    if(skipVSTLoad) {
        // delete VST datas

        int current_tick = 0;
        int dtick= file->tick(150);

        QByteArray c;

        again:

        foreach (MidiEvent* event,
                 *(file->eventsBetween(current_tick-dtick, current_tick+dtick))) {

            SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

            if(sys) {

                c = sys->data();

                if((((skipVSTLoad >> (c[0] & 15)) & 1) != 0) && c[1] == (char) 0x66 && c[2] == (char) 0x66 && (c[3] == 'V' || c[3] == 'W')){

                    file->channel(event->channel())->removeEvent(event);

                    goto again;

                }
            }
        }

        skipVSTLoad = 0;

        for(int i = 0; i < 17; i++)
            _skipvstload2[i]->setChecked(false);

        VST_proc::VST_LoadfromMIDIfile();

    }
#endif

    emit channelWidget->WidgeUpdate(); // update octave for channel displayed

    tabMatrixWidgetMode = 0;
    updateTabMatrixWidget();

    emit tabMatrixWidgetIndex(NewNoteTool::editTrack());

    file->calcMaxTime();

}

MidiFile* MainWindow::getFile()
{
    return file;
}

MatrixWidget* MainWindow::matrixWidget()
{
    return mw_matrixWidget;
}

void MainWindow::matrixSizeChanged(int maxScrollTime, int maxScrollLine,
    int vX, int vY)
{
    vert->setMaximum(maxScrollLine);
    hori->setMaximum(maxScrollTime);
    vert->setValue(vY);
    hori->setValue(vX);

    mw_matrixWidget->repaint();
}

void MainWindow::playStop()
{
    static int one = 1;

    if(one) {

        one = 0;

        if (MidiPlayer::isPlaying()) {
            stop();
        } else {
            play();
        }

        msDelay(100);

        one = 1;
    }
}

void MainWindow::recordStop(int ms)
{
    static int one = 0;
    if(one)
        return;

    one = 1;

    if (MidiPlayer::isPlaying() || MidiInput::recording()) {

        stop();
        msDelay(500);

    } else {

        if(ms >= 0) {
            MidiInput::setTime(ms);
            file->setCursorTick(file->tick(ms));
        } else {
            MidiInput::setTime(file->msOfTick(file->cursorTick()));
        }

        msDelay(50); // very important
        record();
    }

    one = 0;
}

void MainWindow::play()
{
    if (!MidiOutput::isConnected()) {
        CompleteMidiSetupDialog* d = new CompleteMidiSetupDialog(this, false, true);
        d->setModal(true);
        d->exec();
        delete d;
        return;
    }

    MidiOutput::is_playing = 1;

#ifdef USE_FLUIDSYNTH

    for(int n = 0; n < PRE_CHAN; n++) {

        VST_proc::VST_DisableButtons(n, true);

    }

    VST_proc::VST_external_show(-1);

    fluid_output->frames = fluid_output->time_frame.msecsSinceStartOfDay();

    if(MidiOutput::outputPort()==FLUID_SYNTH_NAME) {
        if(fluid_output->it_have_error == 2) {
            QMessageBox::information(this, "Apuff!", "Fluid Synth engine is stopped\nbe cause a don't recoverable error");
            return;
        } else if(fluid_output->sf2_id < 0) {
            QMessageBox::information(this, "Eeeh!", "You needs a Sound File (SF2) to play!");
            return;
        }
    }
#endif

    stdTool->buttonClick();

    if (file && !MidiInput::recording() && !MidiPlayer::isPlaying()) {
        mw_matrixWidget->timeMsChanged(file->msOfTick(file->cursorTick()), true);
#ifdef USE_FLUIDSYNTH


        FluidActionExportWav->setEnabled(false);
        FluidActionExportMp3->setEnabled(false);
        FluidActionExportFlac->setEnabled(false);
#endif
        _miscWidget->setEnabled(false);
        channelWidget->setEnabled(false);
        protocolWidget->setEnabled(false);
        mw_matrixWidget->setEnabled(false);
        _trackWidget->setEnabled(false);
        eventWidget()->setEnabled(false);

        foreach (QWidget* w, _disableWithPlay) {
            if(w)
                w->setEnabled(false);

        }

        MidiPlayer::play(file);

        connect(MidiPlayer::playerThread(),
            SIGNAL(playerStopped()), this, SLOT(stop()));

#ifdef __WINDOWS_MM__
        connect(MidiPlayer::playerThread(),
            SIGNAL(timeMsChanged(int)), mw_matrixWidget, SLOT(timeMsChanged(int)));
#endif
#ifdef ENABLE_REMOTE
        _remoteServer->play();
#endif

    }
}

void MainWindow::record()
{
#ifdef USE_FLUIDSYNTH
    for(int n = 0; n <= PRE_CHAN; n++) {

        VST_proc::VST_show(n, false);
    }
#endif


    if (!MidiOutput::isConnected() || !MidiInput::isConnected(-1)) {
        CompleteMidiSetupDialog* d = new CompleteMidiSetupDialog(this, !MidiInput::isConnected(0), !MidiOutput::isConnected());
        d->setModal(true);
        d->exec();
        delete d;
        return;
    }

    if (!file) {
        newFile();
    }

    if (!MidiInput::recording() && !MidiPlayer::isPlaying()) {
        // play current file

        if (file) {

            if (file->pauseTick() >= 0) {
                file->setCursorTick(file->pauseTick());
                file->setPauseTick(-1);
            }

            mw_matrixWidget->timeMsChanged(file->msOfTick(file->cursorTick()), true);

#ifdef USE_FLUIDSYNTH
            FluidActionExportWav->setEnabled(false);
            FluidActionExportMp3->setEnabled(false);
            FluidActionExportFlac->setEnabled(false);
#endif
            _miscWidget->setEnabled(false);
            channelWidget->setEnabled(false);
            protocolWidget->setEnabled(false);
            mw_matrixWidget->setEnabled(false);
            _trackWidget->setEnabled(false);
            eventWidget()->setEnabled(false);

            foreach (QWidget* w, _disableWithPlay) {
                if(w)
                    w->setEnabled(false);

            }
#ifdef ENABLE_REMOTE
            _remoteServer->record();
#endif
            MidiPlayer::play(file, 1);
            MidiInput::startInput();
            //MidiInControl::wait_record(this);
            /*
            if(MidiInControl::wait_record(this) < 0) {
                stop(); return;
            }
            */
            MidiPlayer::start();
            connect(MidiPlayer::playerThread(),
                SIGNAL(playerStopped()), this, SLOT(stop()));
#ifdef __WINDOWS_MM__
            connect(MidiPlayer::playerThread(),
                SIGNAL(timeMsChanged(int)), mw_matrixWidget, SLOT(timeMsChanged(int)));
#endif
            if(MidiInControl::wait_record(this) < 0) {
                stop(); return;
            }
        }
    }
}

void MainWindow::pause()
{
    if (file) {
        if (MidiPlayer::isPlaying()) {
            file->setPauseTick(file->tick(MidiPlayer::timeMs()));
            stop(false, false, false);
        }
    }

#ifdef USE_FLUIDSYNTH
    VST_proc::VST_external_show(-1);
#endif

}

void MainWindow::stop(bool autoConfirmRecord, bool addEvents, bool resetPause)
{

    if (!file) {
#ifdef USE_FLUIDSYNTH
        VST_proc::VST_external_show(-1);
#endif
        return;
    }

    disconnect(MidiPlayer::playerThread(),
        SIGNAL(playerStopped()), this, SLOT(stop()));

    if (resetPause) {
        file->setPauseTick(-1);
        mw_matrixWidget->update();
        channelWidget->update();
    }

    if (!MidiInput::recording() && MidiPlayer::isPlaying()) {
        MidiPlayer::stop();

#ifdef ENABLE_REMOTE
        _remoteServer->stop();
#endif
        foreach (QWidget* w, _disableWithPlay) {
            if(w)
                w->setEnabled(true);

        }

#ifdef USE_FLUIDSYNTH

        FluidActionExportWav->setEnabled(true);
        FluidActionExportMp3->setEnabled(true);
        FluidActionExportFlac->setEnabled(true);

#endif
        _miscWidget->setEnabled(true);
        channelWidget->setEnabled(true);
        _trackWidget->setEnabled(true);
        protocolWidget->setEnabled(true);
        mw_matrixWidget->setEnabled(true);
        eventWidget()->setEnabled(true);
        mw_matrixWidget->timeMsChanged(MidiPlayer::timeMs(), true);
        _trackWidget->setEnabled(true);
        channelWidget->update();

        panic();
        ProgChangeEvent* pevent = 0;
        ControlChangeEvent* cevent = 0;
        pevent = new ProgChangeEvent(NewNoteTool::editChannel(), getFile()->Prog_MIDI[NewNoteTool::editChannel()], 0);
        cevent = new ControlChangeEvent(NewNoteTool::editChannel(), 0x0, getFile()->Bank_MIDI[NewNoteTool::editChannel()], 0);

        MidiOutput::sendCommandDelete(cevent, getFile()->track(NewNoteTool::editTrack())->device_index());
        MidiOutput::sendCommandDelete(pevent, getFile()->track(NewNoteTool::editTrack())->device_index());

        #ifdef USE_FLUIDSYNTH

            bool disable = VST_proc::VST_mix_disable(true);

            QThread::msleep(100);

            for(int n = 0; n < PRE_CHAN; n++) {

                    VST_proc::VST_DisableButtons(n, false);

                    VST_proc::VST_MIDInotesOff(n);
            }

            VST_proc::VST_MIDIend();

            VST_proc::VST_external_show(-1);

            VST_proc::VST_mix_disable(disable);

        #endif
    }

    MidiTrack* track = file->track(NewNoteTool::editTrack());

    if (!track) {
        return;
    }

    if (MidiInput::recording()) {
        MidiPlayer::stop();

        foreach (QWidget* w, _disableWithPlay) {
            if(w)
                w->setEnabled(true);

        }

#ifdef USE_FLUIDSYNTH

        FluidActionExportWav->setEnabled(true);
        FluidActionExportMp3->setEnabled(true);
        FluidActionExportFlac->setEnabled(true);

#endif
        _miscWidget->setEnabled(true);
        channelWidget->setEnabled(true);
        _trackWidget->setEnabled(true);
        protocolWidget->setEnabled(true);
        mw_matrixWidget->setEnabled(true);
        eventWidget()->setEnabled(true);
        mw_matrixWidget->timeMsChanged(MidiPlayer::timeMs(), true);
        _trackWidget->setEnabled(true);
        channelWidget->update();

        panic();
        ProgChangeEvent* pevent = 0;
        ControlChangeEvent* cevent = 0;
        pevent = new ProgChangeEvent(NewNoteTool::editChannel(), MidiOutput::file->Prog_MIDI[NewNoteTool::editChannel()], 0);
        cevent = new ControlChangeEvent(NewNoteTool::editChannel(), 0x0, MidiOutput::file->Bank_MIDI[NewNoteTool::editChannel()], 0);

        MidiOutput::sendCommandDelete(cevent, getFile()->track(NewNoteTool::editTrack())->device_index());
        MidiOutput::sendCommandDelete(pevent, getFile()->track(NewNoteTool::editTrack())->device_index());
#ifdef USE_FLUIDSYNTH
        FluidActionExportWav->setEnabled(true);
        FluidActionExportMp3->setEnabled(true);
        FluidActionExportFlac->setEnabled(true);
#endif
        _miscWidget->setEnabled(true);
        channelWidget->setEnabled(true);
        protocolWidget->setEnabled(true);
        mw_matrixWidget->setEnabled(true);
        _trackWidget->setEnabled(true);
        eventWidget()->setEnabled(true);
#ifdef ENABLE_REMOTE
        _remoteServer->stop();
#endif
        QMultiMap<int, MidiEvent*> events = MidiInput::endInput(track);

        if (events.isEmpty() && !autoConfirmRecord) {
            QMessageBox::information(this, "Information", "No events recorded.");
        } else {
            RecordDialog* dialog = new RecordDialog(file, events, _settings, this);
            dialog->setModal(true);
            if (!autoConfirmRecord) {
                dialog->show();
            } else {
                if (addEvents) {
                    dialog->enter();
                }
            }
        }
    }
}

void MainWindow::forward()
{
    if (!file)
        return;

    QList<TimeSignatureEvent*>* eventlist = new QList<TimeSignatureEvent*>;
    int ticksleft;
    int oldTick = file->cursorTick();
    if (file->pauseTick() >= 0) {
        oldTick = file->pauseTick();
    }
    if (MidiPlayer::isPlaying() && !MidiInput::recording()) {
        oldTick = file->tick(MidiPlayer::timeMs());
        stop(true);
    }
    file->measure(oldTick, oldTick, &eventlist, &ticksleft);

    int newTick = oldTick - ticksleft + eventlist->last()->ticksPerMeasure();
    file->setPauseTick(-1);
    if (newTick <= file->endTick()) {
        file->setCursorTick(newTick);
        mw_matrixWidget->timeMsChanged(file->msOfTick(newTick), true);
    }
    mw_matrixWidget->update();
}

void MainWindow::back()
{
    if (!file)
        return;

    QList<TimeSignatureEvent*>* eventlist = new QList<TimeSignatureEvent*>;
    int ticksleft;
    int oldTick = file->cursorTick();
    if (file->pauseTick() >= 0) {
        oldTick = file->pauseTick();
    }
    if (MidiPlayer::isPlaying() && !MidiInput::recording()) {
        oldTick = file->tick(MidiPlayer::timeMs());
        stop(true);
    }
    file->measure(oldTick, oldTick, &eventlist, &ticksleft);
    int newTick = oldTick;
    if (ticksleft > 0) {
        newTick -= ticksleft;
    } else {
        newTick -= eventlist->last()->ticksPerMeasure();
    }
    file->measure(newTick, newTick, &eventlist, &ticksleft);
    if (ticksleft > 0) {
        newTick -= ticksleft;
    }
    file->setPauseTick(-1);
    if (newTick >= 0) {
        file->setCursorTick(newTick);
        mw_matrixWidget->timeMsChanged(file->msOfTick(newTick), true);
    }
    mw_matrixWidget->update();
}

void MainWindow::backToBegin()
{
    if (!file)
        return;

    file->setPauseTick(0);
    file->setCursorTick(0);

    mw_matrixWidget->update();
}

void MainWindow::forwardMarker()
{
    if (!file)
        return;

    int oldTick = file->cursorTick();
    if (file->pauseTick() >= 0) {
        oldTick = file->pauseTick();
    }
    if (MidiPlayer::isPlaying() && !MidiInput::recording()) {
        oldTick = file->tick(MidiPlayer::timeMs());
        stop(true);
    }

    int newTick = -1;

    foreach (MidiEvent* event, file->channel(16)->eventMap()->values()) {
        int eventTick = event->midiTime();
        if (eventTick <= oldTick) continue;
        TextEvent* textEvent = dynamic_cast<TextEvent*>(event);

        if (textEvent && textEvent->type() == TextEvent::MARKER) {
            newTick = eventTick;
            break;
        }
    }

    if (newTick < 0) return;
    file->setPauseTick(newTick);
    file->setCursorTick(newTick);
    mw_matrixWidget->timeMsChanged(file->msOfTick(newTick), true);
    mw_matrixWidget->update();
}

void MainWindow::backMarker()
{
    if (!file)
        return;

    int oldTick = file->cursorTick();
    if (file->pauseTick() >= 0) {
        oldTick = file->pauseTick();
    }
    if (MidiPlayer::isPlaying() && !MidiInput::recording()) {
        oldTick = file->tick(MidiPlayer::timeMs());
        stop(true);
    }

    int newTick = 0;
    QList<MidiEvent*> events = file->channel(16)->eventMap()->values();

    for (int eventNumber = events.size() - 1; eventNumber >= 0; eventNumber--) {
        MidiEvent* event = events.at(eventNumber);
        int eventTick = event->midiTime();
        if (eventTick >= oldTick) continue;
        TextEvent* textEvent = dynamic_cast<TextEvent*>(event);

        if (textEvent && textEvent->type() == TextEvent::MARKER) {
            newTick = eventTick;
            break;
        }
    }

    file->setPauseTick(newTick);
    file->setCursorTick(newTick);
    mw_matrixWidget->timeMsChanged(file->msOfTick(newTick), true);
    mw_matrixWidget->update();
}

void MainWindow::save()
{

    if (!file)
        return;

    MidiOutput::saveTrackDevices();

    if (QFile(file->path()).exists()) {

        bool printMuteWarning = false;

        for (int i = 0; i < 16; i++) {
            MidiChannel* ch = file->channel(i);
            if (ch->mute()) {
                printMuteWarning = true;
            }
        }
        foreach (MidiTrack* track, *(file->tracks())) {
            if (track->muted()) {
                printMuteWarning = true;
            }
        }


        if (printMuteWarning) {
            MessageBoxinformation(this, "Channels/Tracks mute",
                                     "One or more channels/tracks are not audible. They will be audible in the saved file.",
                                     "Save file", 0, 0);
        }

        if (!file->save(file->path())) {
            QMessageBox::warning(this, "Error", QString("The file could not be saved. Please make sure that the destination directory exists and that you have the correct access rights to write into this directory."));
        } else {

            if(MidieditorMaster) {
                file->protocol()->number_saved = file->protocol()->number;
                QFile fname(QDir::homePath() + "/Midieditor/file_cache/_copy_path.1");
                QFile fmidi(QDir::homePath() + "/Midieditor/file_cache/_copy_midi.1");

                if(fname.exists() && fmidi.exists()) {

                    bool error = false;
                    QByteArray file_path;

                    if(fname.open(QIODevice::ReadOnly)) {

                        file_path = fname.read(1024);
                        fname.close();

                        if(file_path.isEmpty())
                            error = true;

                    } else
                        error = true;

                    if(!error) {

                        if (!file->save(QDir::homePath() + "/Midieditor/file_cache/_copy_midi.1")) {
                            fname.remove();
                            fmidi.remove();

                        }

                    }
                }
                setWindowModified(false);
            }
        }
    } else {
        saveas();
    }
}

void MainWindow::saveas()
{

    if (!file)
        return;

    QString oldPath = file->path();
    QFile* f = new QFile(oldPath);
    QString dir = startDirectory;
    if (f->exists()) {
        QFileInfo(*f).dir().path();
    }
    QString newPath = QFileDialog::getSaveFileName(this, "Save file as...",
        dir, "MIDI Files(*.mid *.midi *.kar);;All Files(*)");

    if (newPath == "") {
        return;
    }


    // automatically add '.mid' extension
    if (!newPath.endsWith(".mseq", Qt::CaseInsensitive) && !newPath.endsWith(".kar", Qt::CaseInsensitive) && !newPath.endsWith(".mid", Qt::CaseInsensitive) && !newPath.endsWith(".midi", Qt::CaseInsensitive)) {
        newPath.append(".mid");
    }

    if (file->save(newPath)) {

        bool printMuteWarning = false;

        for (int i = 0; i < 16; i++) {
            MidiChannel* ch = file->channel(i);
            if (ch->mute() || !ch->visible()) {
                printMuteWarning = true;
            }
        }
        foreach (MidiTrack* track, *(file->tracks())) {
            if (track->muted() || track->hidden()) {
                printMuteWarning = true;
            }
        }

        if (printMuteWarning) {
            MessageBoxinformation(this, "Channels/Tracks mute",
                "One or more channels/tracks are not audible. They will be audible in the saved file.",
                "Save file", 0, 0);
        }

        file->protocol()->number_saved = file->protocol()->number;

        file->setPath(newPath);
        setWindowTitle(QApplication::applicationName() + " - " + file->path() + "[*]");
        updateRecentPathsList();
        setWindowModified(false);
    } else {
        QMessageBox::warning(this, "Error", QString("The file could not be saved. Please make sure that the destination directory exists and that you have the correct access rights to write into this directory."));
    }
}

void MainWindow::load()
{
    QString oldPath = startDirectory;
    if (file) {
        oldPath = file->path();
        if (file->protocol()->testFileModified()) {
            switch (MessageBoxQuestion(this, "Save file?", "Save file " + file->path() + " before closing?", "Save", "Close without saving", "Cancel")) {
                case 0: {
                    // save
                    if (QFile(file->path()).exists()) {
                        if(file->save(file->path())) {
                            setWindowModified(false);
                        } else {
                            QMessageBox::warning(this, "Error", QString("The file could not be saved. Please make sure that the destination directory exists and that you have the correct access rights to write into this directory."));
                        }
                    } else {
                        saveas();
                    }
                    break;
                }
                case 1: {
                    // close
                    break;
                }
                case 2: {
                    // break
                    return;
                }
            }
        }
    }

    QFile* f = new QFile(oldPath);
    QString dir = startDirectory;
    if (f->exists()) {
        QFileInfo(*f).dir().path();
    }
    QString newPath = QFileDialog::getOpenFileName(this, "Open file",
        dir, "MIDI Files(*.mid *.midi *.kar *.mseq);;All Files(*)");

    if (!newPath.isEmpty()) {
        openFile(newPath);
    }
}

void MainWindow::loadFile(QString nfile)
{
    QString oldPath = startDirectory;
    if (file) {
        oldPath = file->path();
        if (file->protocol()->testFileModified()) {
            switch (MessageBoxQuestion(this, "Save file?", "Save file " + file->path() + " before closing?", "Save", "Close without saving", "Cancel")) {
            case 0: {
                // save
                if (QFile(file->path()).exists()) {
                    if(file->save(file->path())) {
                        setWindowModified(false);
                    } else {
                        QMessageBox::warning(this, "Error", QString("The file could not be saved. Please make sure that the destination directory exists and that you have the correct access rights to write into this directory."));
                    }
                } else {
                    saveas();
                }
                break;
            }
            case 1: {
                // close
                break;
            }
            case 2: {
                // break
                return;
            }
            }
        }
    }
    if (!nfile.isEmpty()) {
        openFile(nfile);
    }
}

bool MainWindow::restore_backup(int number)
{
    if(!MidieditorMaster)
        return false;

    QFile fname(QDir::homePath() + "/Midieditor/file_cache/_copy_path." + QString::number(number));
    QFile fmidi(QDir::homePath() + "/Midieditor/file_cache/_copy_midi." + QString::number(number));

    if(!fname.exists())
        return false;
    if(!fmidi.exists())
        return false;

    bool error = false;
    QByteArray file_path;

    if(fname.open(QIODevice::ReadOnly)) {

        file_path = fname.read(1024);
        fname.close();

        if(!file_path.isEmpty()) {
            int index = file_path.indexOf((char) 0);
            if(index >= 0) {
                file_path.truncate(index);
            }
        }

        if(file_path.isEmpty())
            setWindowTitle(QApplication::applicationName() + " - Untitled Document[*]");

            //error = true;

    } else
        error = true;

    if(error) {

        QFile::remove(QDir::homePath() + "/Midieditor/file_cache/_copy_path." + QString::number(number));
        QFile::remove(QDir::homePath() + "/Midieditor/file_cache/_copy_midi." + QString::number(number));
        return false;
    }


    // clone the file to avoid undesirable locks and writes to the backup file
    if(QFile::exists(QDir::homePath() + "/Midieditor/file_cache/_cache_midi_.mid") &&!QFile::remove(QDir::homePath() + "/Midieditor/file_cache/_cache_midi_.mid")) {
        QMessageBox::warning(this, "Error", QString("The file _cache_midi_.mid is locked."));
        return false;
    }
    if(!fmidi.copy(QDir::homePath() + "/Midieditor/file_cache/_cache_midi_.mid")) {
        QMessageBox::warning(this, "Error", QString("The file _cache_midi_.mid cannot be created"));
        return false;
    }

    NewNoteTool::setEditChannel(0);
    NewNoteTool::setEditTrack(1);

    foreach (QWidget* w, _disableLoading) {
        if(w)
            w->setEnabled(false);

    }

    mw_matrixWidget->disable_ev = true;

    bool ok = true;
    MidiFile* mf = new MidiFile(QDir::homePath() + "/Midieditor/file_cache/_cache_midi_.mid", &ok);
    if(!mf) {

        if(mf->tracks()->count() == 1)
            NewNoteTool::setEditTrack(0);

        MidiInput::file = mf;
        MidiOutput::file = mf;

        mw_matrixWidget->disable_ev = false;

        foreach (QWidget* w, _disableLoading) {
            if(w)
                w->setEnabled(true);

        }
        return false;
    }

    QString filePath = QString::fromUtf8(file_path);

    mf->setPath(filePath);

    if (mf && ok) {
        stop();
        mf->lock_backup(true);

        MidiInput::file = mf;
        MidiOutput::file = mf;

        // Load Track Devices Info

        QString info;

        MidiOutput::loadTrackDevices();

        setFile(mf);

        if(!filePath.isEmpty())
            updateRecentPathsList();
        else
            setWindowTitle(QApplication::applicationName() + " - Untitled Document[*]");


        setWindowModified(false);

#ifdef USE_FLUIDSYNTH

        if(fluid_control) { // anti-crash!
            fluid_control->disable_mainmenu = true;
            fluid_control->dis();
            fluid_control->deleteLater();
            fluid_control = NULL;
        }

        sysExChecker(mf);

        // get COPYRIGHT event
        foreach (MidiEvent* event, *(getFile()->eventsBetween(0, 10))) {
            TextEvent* te = dynamic_cast<TextEvent*>(event);
            if (te && te->channel()== 16 && te->type() == TextEvent::COPYRIGHT) {
                info = te->text();
                break;
            }
        }

        QString s = info.mid(info.indexOf("title: "));
        if(!s.isEmpty()) {
            s = s.mid(7);
            s = s.left(s.indexOf("\n"));

            fluid_output->fluid_settings->setValue("mp3_title", s);
        } else {
            QString name;

            if(filePath.endsWith(".mseq", Qt::CaseInsensitive)) {
                name =  filePath;
                name.remove(name.lastIndexOf(".mseq", -1, Qt::CaseInsensitive), 20);
            } else if(filePath.endsWith(".mid", Qt::CaseInsensitive) || filePath.endsWith(".midi", Qt::CaseInsensitive)) {
                name =  filePath;
                name.remove(name.lastIndexOf(".mid", -1, Qt::CaseInsensitive), 20);
            } else if(filePath.endsWith(".kar", Qt::CaseInsensitive)) {
                name =  filePath;
                name.remove(name.lastIndexOf(".kar", -1, Qt::CaseInsensitive), 20);
            }

            name = name.mid(name.lastIndexOf("/") + 1);
            name = name.mid(name.lastIndexOf("\\") + 1);
            fluid_output->fluid_settings->setValue("mp3_title", name);
        }

        s = info.mid(info.indexOf("artist: "));
        if(!s.isEmpty()) {
            s = s.mid(8);
            s = s.left(s.indexOf("\n"));

            fluid_output->fluid_settings->setValue("mp3_artist", s);
        } /* else
            fluid_output->fluid_settings->setValue("mp3_artist", "");*/

        s = info.mid(info.indexOf("album: "));
        if(!s.isEmpty()) {
            s = s.mid(7);
            s = s.left(s.indexOf("\n"));

            fluid_output->fluid_settings->setValue("mp3_album", s);
        } else
            fluid_output->fluid_settings->setValue("mp3_album", "");

        s = info.mid(info.indexOf("genre: "));
        if(!s.isEmpty()) {
            s = s.mid(7);
            s = s.left(s.indexOf("\n"));

            fluid_output->fluid_settings->setValue("mp3_genre", s);
        } else
            fluid_output->fluid_settings->setValue("mp3_genre", "");

        s = info.mid(info.indexOf("mp3_year: "));
        if(!s.isEmpty()) {
            s = s.mid(10);
            s = s.left(s.indexOf("\n"));

            fluid_output->fluid_settings->setValue("mp3_year", s.toInt());
        } else
            fluid_output->fluid_settings->setValue("mp3_year", QDate::currentDate().year());

        s = info.mid(info.indexOf("mp3_track: "));
        if(!s.isEmpty()) {
            s = s.mid(11);
            s = s.left(s.indexOf("\n"));

            fluid_output->fluid_settings->setValue("mp3_track", s.toInt());
        } else
            fluid_output->fluid_settings->setValue("mp3_track", 1);

        s = info.mid(info.indexOf("mp3_bitrate: "));
        if(!s.isEmpty()) {
            s = s.mid(13);
            s = s.left(s.indexOf("\n"));
            fluid_output->fluid_settings->setValue("mp3_bitrate", s.toInt());
        } else
            fluid_output->fluid_settings->setValue("mp3_bitrate", 5);

        s = info.mid(info.indexOf("mp3_mode: "));
        if(!s.isEmpty()) {
            s = s.mid(10);
            s = s.left(s.indexOf("\n"));
            fluid_output->fluid_settings->setValue("mp3_mode", s.toInt() ? true : false);
        }  else
            fluid_output->fluid_settings->setValue("mp3_mode", true);

        s = info.mid(info.indexOf("mp3_vbr: "));
        if(!s.isEmpty()) {
            s = s.mid(9);
            s = s.left(s.indexOf("\n"));
            fluid_output->fluid_settings->setValue("mp3_vbr", s.toInt() ? true : false);
        } else
            fluid_output->fluid_settings->setValue("mp3_vbr", false);


        s = info.mid(info.indexOf("mp3_hq: "));
        if(!s.isEmpty()) {
            s = s.mid(8);
            s = s.left(s.indexOf("\n"));
            fluid_output->fluid_settings->setValue("mp3_hq", s.toInt() ? true : false);
        } else
            fluid_output->fluid_settings->setValue("mp3_hq", false);

        fluid_output->fluid_settings->setValue("mp3_id3", true);

#endif
        foreach (QWidget* w, _disableLoading) {
            if(w)
                w->setEnabled(true);

        }

        msDelay(50);

        if(!filePath.isEmpty())
            QMessageBox::warning(this, "Warning", QString("This file has been recovered from the backup stack and will overwrite the original file if it is saved"));

        QCoreApplication::processEvents();
        mf->lock_backup(false);

        check_overlapped_notes();

    } else {

        mw_matrixWidget->disable_ev = false;

        foreach (QWidget* w, _disableLoading) {
            if(w)
                w->setEnabled(true);
        }

        QMessageBox::warning(this, "Error", QString("The file is damaged and cannot be opened. "));
        DELETE(mf);

    }

    return true;
}

void MainWindow::openFile(QString filePath)
{

    bool ok = true;

    QFile nf(filePath);

    if (!nf.exists()) {

        QMessageBox::warning(this, "Error", QString("The file [" + filePath + "]does not exist!"));
        return;
    }

    startDirectory = QFileInfo(nf).absoluteDir().path() + "/";

    NewNoteTool::setEditChannel(0);
    NewNoteTool::setEditTrack(1);

    foreach (QWidget* w, _disableLoading) {
        if(w)
            w->setEnabled(false);

    }

    mw_matrixWidget->disable_ev = true;

    MidiFile* mf = new MidiFile(filePath, &ok);

    if (mf && ok) {
        stop();

        if(mf->tracks()->count() == 1)
            NewNoteTool::setEditTrack(0);

        mf->lock_backup(true);

        MidiInput::file = mf;
        MidiOutput::file = mf;

        // Load Track Devices Info

        QString info;

        MidiOutput::loadTrackDevices();

        setFile(mf);

        updateRecentPathsList();

        setWindowModified(false);

        info.clear();

#ifdef USE_FLUIDSYNTH

        if(fluid_control) { // anti-crash!
            fluid_control->disable_mainmenu = true;
            fluid_control->dis();
            fluid_control->deleteLater();
            fluid_control = NULL;
        }

        sysExChecker(mf);

        // get COPYRIGHT event
        foreach (MidiEvent* event, *(getFile()->eventsBetween(0, 10))) {
            TextEvent* te = dynamic_cast<TextEvent*>(event);
            if (te && te->channel()== 16 && te->type() == TextEvent::COPYRIGHT) {
                info = te->text();
                break;
            }
        }

        QString s = info.mid(info.indexOf("title: "));
        if(!s.isEmpty()) {
            s = s.mid(7);
            s = s.left(s.indexOf("\n"));

            fluid_output->fluid_settings->setValue("mp3_title", s);
        } else {
            QString name;

            if(filePath.endsWith(".mseq", Qt::CaseInsensitive)) {
                name =  filePath;
                name.remove(name.lastIndexOf(".mseq", -1, Qt::CaseInsensitive), 20);
            } else if(filePath.endsWith(".mid", Qt::CaseInsensitive) || filePath.endsWith(".midi", Qt::CaseInsensitive)) {
                name =  filePath;
                name.remove(name.lastIndexOf(".mid", -1, Qt::CaseInsensitive), 20);
            } else if(filePath.endsWith(".kar", Qt::CaseInsensitive)) {
                name =  filePath;
                name.remove(name.lastIndexOf(".kar", -1, Qt::CaseInsensitive), 20);
            }

            name = name.mid(name.lastIndexOf("/") + 1);
            name = name.mid(name.lastIndexOf("\\") + 1);
            fluid_output->fluid_settings->setValue("mp3_title", name);
        }

        s = info.mid(info.indexOf("artist: "));
        if(!s.isEmpty()) {
            s = s.mid(8);
            s = s.left(s.indexOf("\n"));

            fluid_output->fluid_settings->setValue("mp3_artist", s);
        } /* else
            fluid_output->fluid_settings->setValue("mp3_artist", "");*/

        s = info.mid(info.indexOf("album: "));
        if(!s.isEmpty()) {
            s = s.mid(7);
            s = s.left(s.indexOf("\n"));

            fluid_output->fluid_settings->setValue("mp3_album", s);
        } else
            fluid_output->fluid_settings->setValue("mp3_album", "");

        s = info.mid(info.indexOf("genre: "));
        if(!s.isEmpty()) {
            s = s.mid(7);
            s = s.left(s.indexOf("\n"));

            fluid_output->fluid_settings->setValue("mp3_genre", s);
        } else
            fluid_output->fluid_settings->setValue("mp3_genre", "");

        s = info.mid(info.indexOf("mp3_year: "));
        if(!s.isEmpty()) {
            s = s.mid(10);
            s = s.left(s.indexOf("\n"));

            fluid_output->fluid_settings->setValue("mp3_year", s.toInt());
        } else
            fluid_output->fluid_settings->setValue("mp3_year", QDate::currentDate().year());

        s = info.mid(info.indexOf("mp3_track: "));
        if(!s.isEmpty()) {
            s = s.mid(11);
            s = s.left(s.indexOf("\n"));

            fluid_output->fluid_settings->setValue("mp3_track", s.toInt());
        } else
            fluid_output->fluid_settings->setValue("mp3_track", 1);

        s = info.mid(info.indexOf("mp3_bitrate: "));
        if(!s.isEmpty()) {
            s = s.mid(13);
            s = s.left(s.indexOf("\n"));
            fluid_output->fluid_settings->setValue("mp3_bitrate", s.toInt());
        } else
            fluid_output->fluid_settings->setValue("mp3_bitrate", 5);

        s = info.mid(info.indexOf("mp3_mode: "));
        if(!s.isEmpty()) {
            s = s.mid(10);
            s = s.left(s.indexOf("\n"));
            fluid_output->fluid_settings->setValue("mp3_mode", s.toInt() ? true : false);
        }  else
            fluid_output->fluid_settings->setValue("mp3_mode", true);

        s = info.mid(info.indexOf("mp3_vbr: "));
        if(!s.isEmpty()) {
            s = s.mid(9);
            s = s.left(s.indexOf("\n"));
            fluid_output->fluid_settings->setValue("mp3_vbr", s.toInt() ? true : false);
        } else
            fluid_output->fluid_settings->setValue("mp3_vbr", false);


        s = info.mid(info.indexOf("mp3_hq: "));
        if(!s.isEmpty()) {
            s = s.mid(8);
            s = s.left(s.indexOf("\n"));
            fluid_output->fluid_settings->setValue("mp3_hq", s.toInt() ? true : false);
        } else
            fluid_output->fluid_settings->setValue("mp3_hq", false);

        fluid_output->fluid_settings->setValue("mp3_id3", true);



#endif
        QCoreApplication::processEvents();
        mf->lock_backup(false);

        check_overlapped_notes();

        foreach (QWidget* w, _disableLoading) {
            if(w)
                w->setEnabled(true);

        }

        msDelay(50);

    } else {

        mw_matrixWidget->disable_ev = false;

        foreach (QWidget* w, _disableLoading) {
            if(w)
                w->setEnabled(true);

        }
        QMessageBox::warning(this, "Error", QString("The file is damaged and cannot be opened. "));
        DELETE(mf);
    }
}

void MainWindow::clean_undo_redo_list()
{
    if (file) {

        file->cleanProtocol();
        channelWidget->OctaveUpdate();
    }

    updateTrackMenu();

#ifdef USE_FLUIDSYNTH
    VST_proc::VST_UpdatefromMIDIfile();
#endif

}

void MainWindow::limit_undo_list() {
    limit_undoAction->setChecked(limit_undoAction->isChecked());
    Protocol::limitUndoAction(limit_undoAction->isChecked());
    _settings->setValue("Main/limit_undo_action", limit_undoAction->isChecked());
}

void MainWindow::redo()
{
    if (file) {
        file->protocol()->redo(true);
        channelWidget->OctaveUpdate();
    }

    updateTrackMenu();

    if (file) {

#ifdef USE_FLUIDSYNTH

        VST_proc::VST_UpdatefromMIDIfile();
#endif

    }

}

void MainWindow::undo()
{
    if (file) {
        file->protocol()->undo(true);
        channelWidget->OctaveUpdate();
    }

    updateTrackMenu();

    if (file) {

#ifdef USE_FLUIDSYNTH

        VST_proc::VST_UpdatefromMIDIfile();
#endif

    }

}

EventWidget* MainWindow::eventWidget()
{
    return _eventWidget;
}

void MainWindow::muteAllChannels()
{
    if (!file)
        return;
    file->protocol()->startNewAction("Mute all channels");

    int track_index = 0;
    if(file->MultitrackMode)
        track_index = file->track(NewNoteTool::editTrack())->device_index();

    for (int i = 0; i < 19; i++) {
        file->channel(i)->setMute(true, track_index);
    }
    file->protocol()->endAction();
    channelWidget->update();
}

void MainWindow::unmuteAllChannels()
{
    if (!file)
        return;
    file->protocol()->startNewAction("All channels audible");

    int track_index = 0;
    if(file->MultitrackMode)
        track_index = file->track(NewNoteTool::editTrack())->device_index();

    for (int i = 0; i < 19; i++) {
        file->channel(i)->setMute(false, track_index);
    }
    file->protocol()->endAction();
    channelWidget->update();
}

void MainWindow::allChannelsVisible()
{
    if (!file)
        return;
    file->protocol()->startNewAction("All channels visible");

    int track_index = 0;
    if(file->MultitrackMode)
        track_index = file->track(NewNoteTool::editTrack())->device_index();

    for (int i = 0; i < 19; i++) {
        file->channel(i)->setVisible(true, track_index);
    }
    file->protocol()->endAction();
    channelWidget->update();
}

void MainWindow::allChannelsInvisible()
{
    if (!file)
        return;
    file->protocol()->startNewAction("Hide all channels");

    int track_index = 0;
    if(file->MultitrackMode)
        track_index = file->track(NewNoteTool::editTrack())->device_index();

    for (int i = 0; i < 19; i++) {
        file->channel(i)->setVisible(false, track_index);
    }
    file->protocol()->endAction();
    channelWidget->update();
}

void MainWindow::closeEvent(QCloseEvent* event)
{

    if (file && !file->protocol()->testFileModified()) {
        if(QMessageBox::question(this, "MidiEditor", "Do you want to exit?") == QMessageBox::Yes) {

        } else {
            // break
            event->ignore();
            return;
        }
    } else if(file){
        switch (MessageBoxQuestion(this, "Save file?", "Save file " + file->path() + " before closing?", "Save", "Close without saving", "Cancel")) {
        case 0: {
            // save
            if (QFile(file->path()).exists()) {
                if(file->save(file->path())) {
                    setWindowModified(false);
                } else {
                    QMessageBox::warning(this, "Error", QString("The file could not be saved. Please make sure that the destination directory exists and that you have the correct access rights to write into this directory."));
                }

            } else {
                saveas();

            }
            break;
        }
        case 1: {
            // close

            break;
        }
        case 2: {
            // break
            event->ignore();
            return;
        }
        }
    }

    for(int n = 0; n < MAX_OUTPUT_DEVICES; n++) {

        //qWarning("out_port %i %s", n, MidiOutput::outputPort(n).toLocal8Bit().constData());
        if(n == 0)
            _settings->setValue("Midi/out_port", MidiOutput::outputPort(n));
        else
            _settings->setValue("Midi/out_port" + QString::number(n), MidiOutput::outputPort(n));

    }

    _settings->setValue("Midi/AllTracksToOne", MidiOutput::AllTracksToOne);
    _settings->setValue("Midi/FluidSynthTracksAuto", MidiOutput::FluidSynthTracksAuto);
    _settings->setValue("Midi/SaveMidiOutDatas", MidiOutput::SaveMidiOutDatas);

    for(int n = 0; n < MAX_INPUT_DEVICES; n++) {

        //qWarning("in_port %i %s", n, MidiInput::inputPort(n).toLocal8Bit().constData());
        if(n == 0)
            _settings->setValue("Midi/in_port", MidiInput::inputPort(n));
        else
            _settings->setValue("Midi/in_port" + QString::number(n), MidiInput::inputPort(n));

    }

#ifdef ENABLE_REMOTE
    if (_remoteServer->clientIp() != "") {
        _settings->setValue("Main/udp_client_ip", _remoteServer->clientIp());
    }
    if (_remoteServer->clientPort() > 0) {
        _settings->setValue("Main/udp_client_port", _remoteServer->clientPort());
    }
    _remoteServer->stopServer();
#endif

    bool ok;
    int numStart = _settings->value("Main/numStart_v3.5", -1).toInt(&ok);
    _settings->setValue("Main/numStart_v3.5", numStart + 1);

    // save the current Path
    _settings->setValue("Main/open_path", startDirectory);
    _settings->setValue("Main/alt_stop", MidiOutput::isAlternativePlayer);
    _settings->setValue("Main/ticks_per_quarter", MidiFile::defaultTimePerQuarter);
    _settings->setValue("Main/screen_locked", mw_matrixWidget->screenLocked());
    _settings->setValue("Main/magnet", EventTool::magnetEnabled());

    _settings->setValue("Main/div", mw_matrixWidget->div());
    _settings->setValue("Main/colors_from_channel", mw_matrixWidget->colorsByChannel());

    _settings->setValue("Main/metronome", Metronome::enabled());
    _settings->setValue("Main/metronome_loudness", Metronome::loudness());
    _settings->setValue("Main/thru", MidiInput::thru());
    _settings->setValue("Main/quantization", _quantizationGrid);
#ifndef CUSTOM_MIDIEDITOR
    _settings->setValue("auto_update_after_prompt", UpdateManager::autoCheckForUpdates());
#endif
    _settings->setValue("Main/has_prompted_for_updates", true); // Happens on first start

    Appearance::writeSettings(_settings);
    if(file)
        file->backup(false);


    event->accept();
}

void MainWindow::donate()
{
    DonateDialog* d = new DonateDialog(this);
    d->setModal(true);
    d->show();
}

void MainWindow::about()
{
    AboutDialog* d = new AboutDialog(this);
    d->setModal(true);
    d->show();
}

void MainWindow::setFileLengthMs()
{
    if (!file)
        return;

    FileLengthDialog* d = new FileLengthDialog(file, this);
    d->setModal(true);
    d->show();
}

void MainWindow::setStartDir(QString dir)
{
    startDirectory = dir;
}

void MainWindow::newFile()
{
    if (file) {
        if (file->protocol()->testFileModified()) {
            switch (MessageBoxQuestion(this, "Save file?", "Save file " + file->path() + " before closing?", "Save", "Close without saving", "Cancel")) {
            case 0: {
                // save
                if (QFile(file->path()).exists()) {
                    file->save(file->path());
                } else {
                    saveas();
                }
                break;
            }
            case 1: {
                // close
                break;
            }
            case 2: {
                // break
                return;
            }
            }
        }
    }

    NewNoteTool::setEditChannel(0);
    NewNoteTool::setEditTrack(1);

    foreach (QWidget* w, _disableLoading) {
        if(w)
            w->setEnabled(false);

    }

    mw_matrixWidget->disable_ev = true;

    // create new File
    MidiFile* f = new MidiFile();

    if(f->tracks()->count() == 1)
        NewNoteTool::setEditTrack(0);

    MidiInput::file = f;
    MidiOutput::file = f;

    setFile(f);

    editTrack(NewNoteTool::editTrack());

    setWindowTitle(QApplication::applicationName() + " - Untitled Document[*]");

    QString info;

#ifdef USE_FLUIDSYNTH
    if(fluid_control) { // anti-crash!
        fluid_control->disable_mainmenu = true;
        fluid_control->dis();
        fluid_control->deleteLater();
        fluid_control = NULL;
    }

    // get COPYRIGHT event
    foreach (MidiEvent* event, *(getFile()->eventsBetween(0, 10))) {
        TextEvent* te = dynamic_cast<TextEvent*>(event);
        if (te && te->channel()== 16 && te->type() == TextEvent::COPYRIGHT) {
            info = te->text();
            break;
        }
    }

    QString s = info.mid(info.indexOf("title: "));
    if(!s.isEmpty()) {
        s = s.mid(7);
        s = s.left(s.indexOf("\n"));
        fluid_output->fluid_settings->setValue("mp3_title", s);
    } else
        fluid_output->fluid_settings->setValue("mp3_title", "");

    s = info.mid(info.indexOf("artist: "));
    if(!s.isEmpty()) {
        s = s.mid(8);
        s = s.left(s.indexOf("\n"));
        fluid_output->fluid_settings->setValue("mp3_artist", s);
    } /* else
        fluid_output->fluid_settings->setValue("mp3_artist", "");*/

    s = info.mid(info.indexOf("album: "));
    if(!s.isEmpty()) {
        s = s.mid(7);
        s = s.left(s.indexOf("\n"));
        fluid_output->fluid_settings->setValue("mp3_album", s);
    } else
        fluid_output->fluid_settings->setValue("mp3_album", "");

    s = info.mid(info.indexOf("genre: "));
    if(!s.isEmpty()) {
        s = s.mid(7);
        s = s.left(s.indexOf("\n"));
        fluid_output->fluid_settings->setValue("mp3_genre", s);
    } else
        fluid_output->fluid_settings->setValue("mp3_genre", "");

    s = info.mid(info.indexOf("mp3_year: "));
    if(!s.isEmpty()) {
        s = s.mid(10);
        s = s.left(s.indexOf("\n"));
        fluid_output->fluid_settings->setValue("mp3_year", s.toInt());
    } else
        fluid_output->fluid_settings->setValue("mp3_year", QDate::currentDate().year());

    s = info.mid(info.indexOf("mp3_track: "));
    if(!s.isEmpty()) {
        s = s.mid(11);
        s = s.left(s.indexOf("\n"));
        fluid_output->fluid_settings->setValue("mp3_track", s.toInt());
    } else
        fluid_output->fluid_settings->setValue("mp3_track", 1);

    s = info.mid(info.indexOf("mp3_bitrate: "));
    if(!s.isEmpty()) {
        s = s.mid(13);
        s = s.left(s.indexOf("\n"));
        fluid_output->fluid_settings->setValue("mp3_bitrate", s.toInt());
    } else
        fluid_output->fluid_settings->setValue("mp3_bitrate", 5);

    s = info.mid(info.indexOf("mp3_mode: "));
    if(!s.isEmpty()) {
        s = s.mid(10);
        s = s.left(s.indexOf("\n"));
        fluid_output->fluid_settings->setValue("mp3_mode", s.toInt() ? true : false);
    }  else
        fluid_output->fluid_settings->setValue("mp3_mode", true);

    s = info.mid(info.indexOf("mp3_vbr: "));
    if(!s.isEmpty()) {
        s = s.mid(9);
        s = s.left(s.indexOf("\n"));
        fluid_output->fluid_settings->setValue("mp3_vbr", s.toInt() ? true : false);
    } else
        fluid_output->fluid_settings->setValue("mp3_vbr", false);


    s = info.mid(info.indexOf("mp3_hq: "));
    if(!s.isEmpty()) {
        s = s.mid(8);
        s = s.left(s.indexOf("\n"));
        fluid_output->fluid_settings->setValue("mp3_hq", s.toInt() ? true : false);
    } else
        fluid_output->fluid_settings->setValue("mp3_hq", false);

    //fluid_output->fluid_settings->setValue("mp3_id3", 0);
#endif

    foreach (QWidget* w, _disableLoading) {
        if(w)
            w->setEnabled(true);

    }

    msDelay(50);


}

void MainWindow::panic()
{
    MidiPlayer::panic();
#ifdef USE_FLUIDSYNTH
    VST_proc::VST_WahWahReset(fluid_output->_sample_rate);
#endif
}

void MainWindow::screenLockPressed(bool enable)
{
    mw_matrixWidget->setScreenLocked(enable);
}

void MainWindow::scaleSelection() {
    bool ok;
    double scale = QInputDialog::getDouble(this, "Scalefactor",
        "Scalefactor:", 1.0, 0, 2147483647, 17, &ok);
    if (ok && scale > 0 && Selection::instance()->selectedEvents().size() > 0 && file) {
        // find minimum
        int minTime = 2147483647;
        foreach (MidiEvent* e, Selection::instance()->selectedEvents()) {
            if (e->midiTime() < minTime) {
                minTime = e->midiTime();
            }
        }

        file->protocol()->startNewAction("Scale events", 0);
        foreach (MidiEvent* e, Selection::instance()->selectedEvents()) {
            e->setMidiTime((e->midiTime() - minTime) * scale + minTime);
            OnEvent* on = dynamic_cast<OnEvent*>(e);
            if (on) {
                MidiEvent* off =(MidiEvent* ) on->offEvent();
                off->setMidiTime((off->midiTime() - minTime) * scale + minTime);
            }
        }
        file->protocol()->endAction();
    }
}

void MainWindow::alignLeft()
{
    if (Selection::instance()->selectedEvents().size() > 1 && file) {
        // find minimum
        int minTime = 2147483647;
        foreach (MidiEvent* e, Selection::instance()->selectedEvents()) {
            if (e->midiTime() < minTime) {
                minTime = e->midiTime();
            }
        }

        file->protocol()->startNewAction("Align left", new QImage(":/run_environment/graphics/tool/align_left.png"));
        foreach (MidiEvent* e, Selection::instance()->selectedEvents()) {
            int onTime = e->midiTime();
            e->setMidiTime(minTime);
            OnEvent* on = dynamic_cast<OnEvent*>(e);
            if (on) {
                MidiEvent* off =(MidiEvent* ) on->offEvent();
                off->setMidiTime(minTime + (off->midiTime() - onTime));
            }
        }
        file->protocol()->endAction();
    }
}

void MainWindow::alignRight()
{
    if (Selection::instance()->selectedEvents().size() > 1 && file) {
        // find maximum
        int maxTime = 0;
        foreach (MidiEvent* e, Selection::instance()->selectedEvents()) {
            OnEvent* on = dynamic_cast<OnEvent*>(e);
            if (on) {
                MidiEvent* off = on->offEvent();
                if (off->midiTime() > maxTime) {
                    maxTime = off->midiTime();
                }
            }
        }

        file->protocol()->startNewAction("Align right", new QImage(":/run_environment/graphics/tool/align_right.png"));
        foreach (MidiEvent* e, Selection::instance()->selectedEvents()) {
            int onTime = e->midiTime();
            OnEvent* on = dynamic_cast<OnEvent*>(e);
            if (on) {
                MidiEvent* off = on->offEvent();
                e->setMidiTime(maxTime - (off->midiTime() - onTime));
                off->setMidiTime(maxTime);
            }
        }
        file->protocol()->endAction();
    }
}

void MainWindow::equalize()
{
    if (Selection::instance()->selectedEvents().size() > 1 && file) {
        // find average
        int avgStart = 0;
        int avgTime = 0;
        int count = 0;
        foreach (MidiEvent* e, Selection::instance()->selectedEvents()) {
            OnEvent* on = dynamic_cast<OnEvent*>(e);
            if (on) {
                MidiEvent* off = on->offEvent();
                avgStart += e->midiTime();
                avgTime += (off->midiTime() - e->midiTime());
                count++;
            }
        }
        if (count > 1) {
            avgStart /= count;
            avgTime /= count;

            file->protocol()->startNewAction("Equalize", new QImage(":/run_environment/graphics/tool/equalize.png"));
            foreach (MidiEvent* e, Selection::instance()->selectedEvents()) {
                OnEvent* on = dynamic_cast<OnEvent*>(e);
                if (on) {
                    MidiEvent* off = on->offEvent();
                    e->setMidiTime(avgStart);
                    off->setMidiTime(avgStart + avgTime);
                }
            }
        }
        file->protocol()->endAction();
    }
}

void MainWindow::deleteSelectedEvents()
{
    bool showsSelected = false;
    if (Tool::currentTool()) {
        EventTool* eventTool = dynamic_cast<EventTool*>(Tool::currentTool());
        if (eventTool) {
            showsSelected = eventTool->showsSelection();
        }
    }
    if (showsSelected && Selection::instance()->selectedEvents().size() > 0 && file) {

        int selected = Selection::instance()->selectedEvents().size();
        file->protocol()->startNewAction("Remove event(s) (" + QString::number(selected) + ")");
        foreach (MidiEvent* ev, Selection::instance()->selectedEvents()) {
            file->channel(ev->channel())->removeEvent(ev);
        }
        Selection::instance()->clearSelection();
        eventWidget()->reportSelectionChangedByTool();
        file->protocol()->endAction();
    }
}

void MainWindow::deleteChannel(QAction* action)
{

    if (!file) {
        return;
    }

    int num = action->data().toInt();
    file->protocol()->startNewAction("Remove all events from channel " + QString::number(num));
    foreach (MidiEvent* event, file->channel(num)->eventMap()->values()) {
        if (event->track() == file->track(NewNoteTool::editTrack()) && Selection::instance()->selectedEvents().contains(event)) {
            EventTool::deselectEvent(event);
        }
    }

    file->channel(num)->deleteAllEvents();
    file->protocol()->endAction();
}

void MainWindow::moveSelectedEventsToChannel(QAction* action)
{

    if (!file) {
        return;
    }

    QDialog *d= new QDialog(this,  Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    d->resize(397, 253);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(d);
    buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
    buttonBox->setGeometry(QRect(30, 170, 341, 32));
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    QLabel* label = new QLabel(d);
    label->setObjectName(QString::fromUtf8("label"));
    label->setGeometry(QRect(72, 40, 277, 91));
    label->setText(QCoreApplication::translate("Dialog4", "<html><head/><body><p><span style=\" font-size:11pt; font-weight:600;\">Warning:</span><span style=\" font-size:11pt;\"> this copy all events to the</span></p><p><span style=\" font-size:11pt;\"> new channel deleting the matching events!</span><br/></p></body></html>", nullptr));
    QObject::connect(buttonBox, SIGNAL(accepted()), d, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), d, SLOT(reject()));

    if(d->exec()==QDialog::Rejected) return;

    int num = action->data().toInt();
    MidiChannel* channel = file->channel(num);

    int track_index = file->track(NewNoteTool::editTrack())->device_index();

    if (Selection::instance()->selectedEvents().size() > 0) {
        file->protocol()->startNewAction("Move selected events to channel " + QString::number(num));
        foreach (MidiEvent* ev, Selection::instance()->selectedEvents()) {
            TempoChangeEvent* tempo = dynamic_cast<TempoChangeEvent*>(ev);
            TimeSignatureEvent* time = dynamic_cast<TimeSignatureEvent*>(ev);
            KeySignatureEvent* key = dynamic_cast<KeySignatureEvent*>(ev);
            TextEvent* text = dynamic_cast<TextEvent*>(ev);
            SysExEvent* sys = dynamic_cast<SysExEvent*>(ev);
            UnknownEvent* unk = dynamic_cast<UnknownEvent*>(ev);

            ProgChangeEvent* prog = dynamic_cast<ProgChangeEvent*>(ev);
            ControlChangeEvent* ctrl = dynamic_cast<ControlChangeEvent*>(ev);
            PitchBendEvent* pitch = dynamic_cast<PitchBendEvent*>(ev);
            KeyPressureEvent* key1 = dynamic_cast<KeyPressureEvent*>(ev);
            ChannelPressureEvent* cpress = dynamic_cast<ChannelPressureEvent*>(ev);

            if(tempo || time || key1 || text/* || sys*/ || unk) continue;

            if(sys) {
                QByteArray c = sys->data();
                int _current_tick = ev->midiTime();

                // sysEx chan 0x70
                if((c[3] & 0xF0) == 0x70 && c[0] == (char) track_index && c[1] == (char) 0x66 && c[2] == (char) 0x66) {
                    int chan = c[3] & 0xF;
                    if(chan != num) {
                        foreach (MidiEvent* event, *(file->eventsBetween(_current_tick-10, _current_tick+10))) {
                            SysExEvent* sys2 = dynamic_cast<SysExEvent*>(event);
                            if(sys2) {
                                QByteArray d = sys2->data();
                                if((d[3] & 0xF0) == 0x70 && d[0] == (char) track_index && d[1] == (char) 0x66 && d[2] == (char) 0x66)
                                    if((d[3] & 0xF) == num)
                                        file->channel(16)->removeEvent(event);
                            }
                        }

                        c[3] = 0x70 | (num & 0xf);

                        sys->setData(c);
                        sys->save();
                    }
                }
                else
                    // sysEx chan 'W'
                    if(c[3] == 'W'  && c[1] == (char) 0x66 && c[2] == (char) 0x66) {
                        int chan = c[0] & 0xF;
                        if(chan != num) {
                            foreach (MidiEvent* event, *(file->eventsBetween(_current_tick-10, _current_tick+10))) {
                                SysExEvent* sys2 = dynamic_cast<SysExEvent*>(event);
                                if(sys2) {
                                    QByteArray d = sys2->data();

                                    chan = num | (c[0] & 0xf0);
                                    if(c[3] == d[3] && c[3] == 'W' && chan == d[0] && c[1] == d[1] && c[2] == d[2])
                                        file->channel(16)->removeEvent(event);
                                }
                            }

                            c[0] = num | (c[0] & 0xf0);

                            sys->setData(c);
                            sys->save();
                        }
                    } else
                        continue;
            }

            if(prog || ctrl || pitch || key || cpress) {
                int _current_tick = ev->midiTime();
                // borra eventos repetidos proximos
                foreach (MidiEvent* event, *(file->eventsBetween(_current_tick-10, _current_tick+10))) {
                    ProgChangeEvent* prog2 = dynamic_cast<ProgChangeEvent*>(event);
                    ControlChangeEvent* ctrl2 = dynamic_cast<ControlChangeEvent*>(event);
                    PitchBendEvent* pitch2 = dynamic_cast<PitchBendEvent*>(event);
                    KeyPressureEvent* key2 = dynamic_cast<KeyPressureEvent*>(event);
                    ChannelPressureEvent* cpress2 = dynamic_cast<ChannelPressureEvent*>(event);
                    if(event->channel()==num && event->track() == file->track(NewNoteTool::editTrack())) {
                        if ((ctrl && ctrl2 && ctrl2->control()==ctrl->control()) ||
                            (prog && prog2) ||
                            (pitch && pitch2) ||
                            (key1 && key2) ||
                            (cpress && cpress2) ) {
                            file->channel(num)->removeEvent(event);
                        }
                    }
                }


                file->channel(ev->channel())->removeEvent(ev);
                ev->setChannel(num, true);

            }

            OnEvent* onevent = dynamic_cast<OnEvent*>(ev);
            if (onevent) {
                NoteOnEvent* notevent = dynamic_cast<NoteOnEvent*>(ev);

                int _ini_tick = ev->midiTime();
                int _end_tick = onevent->offEvent()->midiTime();

                // borra eventos repetidos proximos
                foreach (MidiEvent* event, channel->eventMap()->values()) {
                    OnEvent* onevent2 = dynamic_cast<OnEvent*>(event);
                    NoteOnEvent* notevent2 = dynamic_cast<NoteOnEvent*>(event);

                    if(event->channel()==num && event->track() == file->track(NewNoteTool::editTrack())) {
                        if (notevent2 && onevent2 && notevent->note()==notevent2->note() &&
                                ((onevent2->midiTime() >=_ini_tick && onevent2->midiTime() <=_end_tick) ||
                                (onevent2->offEvent()->midiTime() >=_ini_tick && onevent2->offEvent()->midiTime() <=_end_tick))) {
                            file->channel(num)->removeEvent(event);

                        }
                    }
                }

                file->channel(ev->channel())->removeEvent(ev);
                ev->setChannel(num, true);
                ev->setTrack(file->track(NewNoteTool::editTrack()), true);
                channel->insertEvent(onevent->offEvent(), onevent->offEvent()->midiTime());
                onevent->offEvent()->setChannel(num);
                onevent->offEvent()->setTrack(file->track(NewNoteTool::editTrack()));
            }
            channel->insertEvent(ev, ev->midiTime());
        }

        file->protocol()->endAction();
    }
}

void MainWindow::moveSelectedNotesToChannel(QAction* action)
{

    if (!file) {
        return;
    }

    QDialog *d= new QDialog(this,  Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    d->resize(397, 253);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(d);
    buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
    buttonBox->setGeometry(QRect(30, 170, 341, 32));
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    QLabel* label = new QLabel(d);
    label->setObjectName(QString::fromUtf8("label"));
    label->setGeometry(QRect(72, 40, 277, 91));
    label->setText(QCoreApplication::translate("Dialog4", "<html><head/><body><p><span style=\" font-size:11pt; font-weight:600;\">Warning:</span><span style=\" font-size:11pt;\"> this copy all notes to the</span></p><p><span style=\" font-size:11pt;\"> new channel deleting the matching notes!</span><br/></p></body></html>", nullptr));
    QObject::connect(buttonBox, SIGNAL(accepted()), d, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), d, SLOT(reject()));

    if(d->exec()==QDialog::Rejected) return;

    int num = action->data().toInt();
    MidiChannel* channel = file->channel(num);

    if (Selection::instance()->selectedEvents().size() > 0) {
        file->protocol()->startNewAction("Move selected notes to channel " + QString::number(num));
        foreach (MidiEvent* ev, Selection::instance()->selectedEvents()) {
            OnEvent* onevent = dynamic_cast<OnEvent*>(ev);
            if (onevent) {
                NoteOnEvent* notevent = dynamic_cast<NoteOnEvent*>(ev);

                int _ini_tick = ev->midiTime();
                int _end_tick = onevent->offEvent()->midiTime();

                // borra eventos repetidos proximos
                foreach (MidiEvent* event, channel->eventMap()->values()) {
                    OnEvent* onevent2 = dynamic_cast<OnEvent*>(event);
                    NoteOnEvent* notevent2 = dynamic_cast<NoteOnEvent*>(event);

                    if(event->channel()==num && event->track() == file->track(NewNoteTool::editTrack())) {
                        if (notevent2 && onevent2 && notevent->note()==notevent2->note() &&
                                ((onevent2->midiTime() >=_ini_tick && onevent2->midiTime() <=_end_tick) ||
                                (onevent2->offEvent()->midiTime() >=_ini_tick && onevent2->offEvent()->midiTime() <=_end_tick))) {
                            file->channel(num)->removeEvent(event);
                        }
                    }
                }

                file->channel(ev->channel())->removeEvent(ev);
                ev->setChannel(num, true);
                ev->setTrack(file->track(NewNoteTool::editTrack()), true);

                channel->insertEvent(onevent->offEvent(), onevent->offEvent()->midiTime());
                onevent->offEvent()->setChannel(num);
                onevent->offEvent()->setTrack(file->track(NewNoteTool::editTrack()));

                channel->insertEvent(ev, ev->midiTime());
            }
        }

        file->protocol()->endAction();
    }
}

void MainWindow::moveSelectedEventsToTrack(QAction* action)
{

    if (!file) {
        return;
    }

    int num = action->data().toInt();
    MidiTrack* track = file->track(num);

    if (Selection::instance()->selectedEvents().size() > 0) {
        file->protocol()->startNewAction("Move selected events to track " + QString::number(num));
        foreach (MidiEvent* ev, Selection::instance()->selectedEvents()) {
            TempoChangeEvent* tcev = dynamic_cast<TempoChangeEvent*>(ev);
            TimeSignatureEvent* tsev = dynamic_cast<TimeSignatureEvent*>(ev);
            SysExEvent* sys = dynamic_cast<SysExEvent*>(ev);
            UnknownEvent* unk = dynamic_cast<UnknownEvent*>(ev);
            if(tcev || tsev || sys || unk)
                ev->setTrack(file->track(0), true);
            else {
                ev->setTrack(track, true);
                OnEvent* onevent = dynamic_cast<OnEvent*>(ev);
                if (onevent && onevent->offEvent()) {
                    onevent->offEvent()->setTrack(track);
                }
            }
        }

        file->protocol()->endAction();
    }
}

void MainWindow::updateRecentPathsList()
{

    // if file opened put it at the top of the list
    if (file) {

        QString currentPath = file->path();
        QStringList newList;
        currentPath.replace("\\", "/");
        newList.append(currentPath);


        foreach (QString str, _recentFilePaths) {
            str.replace("\\", "/");
            if (str != currentPath && newList.size() < 16) {
                newList.append(str);
            }
        }

        _recentFilePaths = newList;

        _recentFilePaths.removeDuplicates();
    }

    // save list
    QVariant list(_recentFilePaths);
    _settings->setValue("Main/recent_file_list", list);

    // update menu
    _recentPathsMenu->clear();
    foreach (QString path, _recentFilePaths) {
        QFile f(path);
        QString name = QFileInfo(f).fileName();

        QVariant variant(path);
        QAction* openRecentFileAction = new QAction(name, this);
        openRecentFileAction->setData(variant);
        _recentPathsMenu->addAction(openRecentFileAction);

    }
}

void MainWindow::openRecent(QAction* action)
{

    QString path = action->data().toString();

    if (file) {
        QString oldPath = file->path();

        if (file->protocol()->testFileModified()) {
            switch (MessageBoxQuestion(this, "Save file?", "Save file " + file->path() + " before closing?", "Save", "Close without saving", "Cancel")) {
            case 0: {
                // save
                if (QFile(file->path()).exists()) {
                    file->save(file->path());
                } else {
                    saveas();
                }
                break;
            }
            case 1: {
                // close
                break;
            }
            case 2: {
                // break
                return;
            }
            }
        }
    }

    openFile(path);
}

void MainWindow::updateChannelMenu()
{

    int index = (file) ? (file->MultitrackMode ? file->track(NewNoteTool::editTrack())->device_index() : 0) : 0;
    // delete channel events menu
    foreach (QAction* action, _deleteChannelMenu->actions()) {
        int channel = action->data().toInt();
        if (file) {

            if(channel!=9) action->setText(QString::number(channel) + " "
            + MidiFile::instrumentName(file->Bank_MIDI[channel + 4 * (index != 0) + 16 * index],
                                       file->Prog_MIDI[channel + 4 * (index != 0) + 16 * index]));
            else action->setText(QString::number(channel) + " " + MidiFile::drumName(MidiOutput::file->Prog_MIDI[channel + 4 * (index != 0) + 16 * index]));

        }
    }

    // move events to channel...
    foreach (QAction* action, _moveSelectedEventsToChannelMenu->actions()) {
        int channel = action->data().toInt();
        if (file) {
            if(channel!=9) action->setText(QString::number(channel) + " " +
               MidiFile::instrumentName(file->Bank_MIDI[channel + 4 * (index != 0) + 16 * index],
                                        file->Prog_MIDI[channel + 4 * (index != 0) + 16 * index]));
            else action->setText(QString::number(channel) + " " +
                                 MidiFile::drumName(file->Prog_MIDI[channel + 4 * (index != 0) + 16 * index]));

        }
    }

    // move notes to channel...
    foreach (QAction* action, _moveSelectedNotesToChannelMenu->actions()) {
        int channel = action->data().toInt();
        if (file) {
            if(channel!=9) action->setText(QString::number(channel) + " " +
                           MidiFile::instrumentName(file->Bank_MIDI[channel + 4 * (index != 0) + 16 * index],
                                                    file->Prog_MIDI[channel + 4 * (index != 0) + 16 * index]));
            else action->setText(QString::number(channel) + " " +
                                 MidiFile::drumName(file->Prog_MIDI[channel + 4 * (index != 0) + 16 * index]));

        }
    }


    // paste events to channel...
    foreach (QAction* action, _pasteToChannelMenu->actions()) {
        int channel = action->data().toInt();
        if (file && channel >= 0) {
                if(channel!=9) action->setText(QString::number(channel) + " " +
                               MidiFile::instrumentName(file->Bank_MIDI[channel + 4 * (index != 0) + 16 * index], file->channel(channel)->progAtTick(0, file->track(NewNoteTool::editTrack()))));
                else action->setText(QString::number(channel) + " " + MidiFile::drumName(file->channel(channel)->progAtTick(0, file->track(NewNoteTool::editTrack()))));

            }
    }

    // select all events from channel...
    foreach (QAction* action, _selectAllFromChannelMenu->actions()) {
        int channel = action->data().toInt();
        if (file) {
            if(channel!=9) action->setText(QString::number(channel) + " " + MidiFile::instrumentName(file->Bank_MIDI[channel], file->channel(channel)->progAtTick(0, file->track(NewNoteTool::editTrack()))));
            else action->setText(QString::number(channel) + " " + MidiFile::drumName(file->channel(channel)->progAtTick(0, file->track(NewNoteTool::editTrack()))));
        }
    }

    _chooseEditChannel->setCurrentIndex(NewNoteTool::editChannel());
}

void MainWindow::updateTrackMenu()
{

    _moveSelectedEventsToTrackMenu->clear();
    _chooseEditTrack->clear();
    _selectAllFromTrackMenu->clear();

    if (!file) {
        return;
    }

    if(file->protocol()->currentTrackDeleted || NewNoteTool::editTrack() >= file->numTracks()) {

            if(file->numTracks() > 1)
                NewNoteTool::setEditTrack(1);
            else
                NewNoteTool::setEditTrack(0);

        tabMatrixWidgetMode = 0;
    }

    updateTabMatrixWidget();

    for (int i = 0; i < file->numTracks(); i++) {
        QVariant variant(i);
        QAction* moveToTrackAction = new QAction(QString::number(i) + " " +
                                                 file->tracks()->at(i)->name(), this);
        moveToTrackAction->setData(variant);
        _moveSelectedEventsToTrackMenu->addAction(moveToTrackAction);
    }

    for (int i = 0; i < file->numTracks(); i++) {
        QVariant variant(i);
        QAction* select = new QAction(QString::number(i) + " " + file->tracks()->at(i)->name(), this);
        select->setData(variant);
        _selectAllFromTrackMenu->addAction(select);
    }

    for (int i = 0; i < file->numTracks(); i++) {
        _chooseEditTrack->addItem("Track " + QString::number(i) + ": " + file->tracks()->at(i)->name());
    }

    if(file->protocol()->currentTrackDeleted || NewNoteTool::editTrack() >= file->numTracks()) {

        file->protocol()->currentTrackDeleted = false;

        if(file->numTracks() > 1)
            NewNoteTool::setEditTrack(1);
        else
            NewNoteTool::setEditTrack(0);

        tabMatrixWidgetMode = 0;

        int ntrack = NewNoteTool::editTrack();

        tabMatrixWidget->setCurrentIndex(ntrack);
        MidiInput::track_index = ntrack;
        FingerPatternDialog::_track_index = ntrack;
    }

    _chooseEditTrack->setCurrentIndex(NewNoteTool::editTrack());

    _pasteToTrackMenu->clear();
    QActionGroup* pasteTrackGroup = new QActionGroup(this);
    pasteTrackGroup->setExclusive(true);

    bool checked = false;
    for (int i = -2; i < file->numTracks(); i++) {
        QVariant variant(i);
        QString text = QString::number(i);
        if (i == -2) {
            text = "Same as selected for new events";
        } else if (i == -1) {
            text = "Keep track";
        } else {
            text = "Track " + QString::number(i) + ": " + file->tracks()->at(i)->name();
        }
        QAction* pasteToTrackAction = new QAction(text, this);
        pasteToTrackAction->setData(variant);
        pasteToTrackAction->setCheckable(true);
        _pasteToTrackMenu->addAction(pasteToTrackAction);
        pasteTrackGroup->addAction(pasteToTrackAction);
        if (i == EventTool::pasteTrack()) {
            pasteToTrackAction->setChecked(true);
            checked = true;
        }
    }
    if (!checked) {
        _pasteToTrackMenu->actions().first()->setChecked(true);
        EventTool::setPasteTrack(0);
    }

    // recover file menu update

    if(_recover_file) {
        int i = 0;
        _recover_file->clear();
        _recover_file->addAction(_recover_file_act[i]);
        for(int n = 1; n < 17; n++) {
            if(_recover_file_act[n]) {
                delete _recover_file_act[n];
                _recover_file_act[n] = NULL;
            }
        }
        i++;
        for(int n = 1; n < 17; n++) {
            QFile f(QDir::homePath() + "/Midieditor/file_cache/_copy_path." + QString::number(n));
            QFile m(QDir::homePath() + "/Midieditor/file_cache/_copy_midi." + QString::number(n));
            if(f.exists() && m.exists()) {

                QByteArray file_readed;
                QByteArray desc_readed;
                QString file_name;
                QString description;

                if(f.open(QIODevice::ReadOnly)) {


                    file_readed = f.read(1024);
                    if(f.seek(1024)) {
                        desc_readed = f.read(1024);
                    }

                    f.close();
                }

                if(!file_readed.isEmpty()) {
                    int index = file_readed.indexOf((char) 0);
                    if(index >= 0) {
                        file_readed.truncate(index);
                    }

                }

                if(!desc_readed.isEmpty()) {
                    int index = desc_readed.indexOf((char) 0);
                    if(index >= 0) {
                        desc_readed.truncate(index);
                    }
                }

                if(file_readed.isEmpty())
                    file_readed.append("Untitled Document");
                file_name = QString::fromUtf8(file_readed);

                if(!desc_readed.isEmpty())
                    description = " -> " + QString::fromUtf8(desc_readed);

                file_name = file_name.mid(file_name.lastIndexOf("/") + 1);
                file_name = file_name.mid(file_name.lastIndexOf("\\") + 1);
                file_name.truncate(32); // only 32 char for descriptive file
                file_name = QString::number(n) +": " + file_name + description;


                _recover_file_act[i] = new QAction(file_name, this);

                _recover_file_act[i]->setCheckable(false);
                _recover_file_act[i]->setChecked(false);
                _recover_file->addAction(_recover_file_act[i]);

                connect(_recover_file_act[i], QOverload<bool>::of(&QAction::triggered), [=](bool){
                    if(restore_backup(n))
                        return;
                });

            }

        }
    }
    // ends recover file menu update
}


void MainWindow::keyPressEvent(QKeyEvent* event)
{
    mw_matrixWidget->takeKeyPressEvent(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent* event)
{
    mw_matrixWidget->takeKeyReleaseEvent(event);
}

void MainWindow::showEventWidget(bool show)
{
    if (show) {
        lowerTabWidget->setCurrentIndex(1);
    } else {
        lowerTabWidget->setCurrentIndex(0);
    }
}

void MainWindow::renameTrackMenuClicked(QAction* action)
{
    int track = action->data().toInt();
    renameTrack(track);
}

void MainWindow::renameTrack(int tracknumber)
{

    if (!file) {
        return;
    }

    file->protocol()->startNewAction("Edit Track Name");

    bool ok;
    QString text = QInputDialog::getText(this, "Set Track Name",
        "Track name (Track " + QString::number(tracknumber) + ")", QLineEdit::Normal,
        file->tracks()->at(tracknumber)->name(), &ok);
    if (ok && !text.isEmpty()) {
        file->tracks()->at(tracknumber)->setName(text);
    }

    file->protocol()->endAction();
    updateTrackMenu();
}

void MainWindow::removeTrackMenuClicked(QAction* action)
{
    int track = action->data().toInt();
    removeTrack(track);
}

void MainWindow::removeTrack(int tracknumber)
{

    if (!file) {
        return;
    }
    MidiTrack* track = file->track(tracknumber);
    file->protocol()->startNewAction("Remove track");
    foreach (MidiEvent* event, Selection::instance()->selectedEvents()) {
        if (event->track() == track) {
            EventTool::deselectEvent(event);
        }
    }
    if (!file->removeTrack(track)) {
        QMessageBox::warning(this, "Error", QString("The selected track can\'t be removed!\n It\'s the last track of the file."));
    }
    file->protocol()->endAction();

    if(NewNoteTool::editTrack() >= file->numTracks()) {
        if(file->numTracks() > 1)
            NewNoteTool::setEditTrack(1);
        else
            NewNoteTool::setEditTrack(0);
    }

    updateTrackMenu();
}

void MainWindow::updateTabMatrixWidget()
{

    if(!file)
        return;

    if(file->numTracks() <= 16) {
        tabMatrixWidget->tabBar()->setTabVisible(17, false);

        if(tabMatrixWidgetMode == 1) {
            tabMatrixWidgetMode = 0;
        }
    }


    if(tabMatrixWidgetMode == 0) {

        int n = 0;
        tabMatrixWidget->tabBar()->setTabToolTip(16, "Mode Selector. Click to change to Channels");
        tabMatrixWidget->tabBar()->setTabText(16, "Tracks");
        tabMatrixWidget->tabBar()->setTabToolTip(17, "Mode Selector. Click to change to Tracks >= 16");
        tabMatrixWidget->tabBar()->setTabText(17, "Tracks+");
        upperTabWidget->setCurrentIndex(0);

        for(; (n < 16) && (n < file->numTracks()); n++) {

            tabMatrixWidget->tabBar()->setTabEnabled(n, true);
            tabMatrixWidget->tabBar()->setTabIcon(n, QIcon());
            tabMatrixWidget->tabBar()->setTabText(n, QString::number(n));
            tabMatrixWidget->tabBar()->setTabToolTip(n, "Select Track " + QString::number(n) +
                                                " " + file->track(n)->name() + "\n Double Click to change to Channels");
        }

        for(; n < 16; n++) {

            tabMatrixWidget->tabBar()->setTabEnabled(n, false);
            tabMatrixWidget->tabBar()->setTabIcon(n, QIcon());
            tabMatrixWidget->tabBar()->setTabText(n, QString::number(n));
            tabMatrixWidget->tabBar()->setTabToolTip(n, "Empty track");
        }

        if(file->numTracks() > 16)
            tabMatrixWidget->tabBar()->setTabVisible(17, true);
        else
            tabMatrixWidget->tabBar()->setTabVisible(17, false);

    } else if(tabMatrixWidgetMode == 1) {

        int n = 0;

        tabMatrixWidget->tabBar()->setTabToolTip(16, "Mode Selector. Click to change to Tracks < 16");
        tabMatrixWidget->tabBar()->setTabText(16, "Tracks");
        tabMatrixWidget->tabBar()->setTabToolTip(17, "Mode Selector. Click to change to Channels");
        tabMatrixWidget->tabBar()->setTabText(17, "Tracks+");
        upperTabWidget->setCurrentIndex(0);

        for(; (n < 16) && ((n + 16) < file->numTracks()); n++) {

            tabMatrixWidget->tabBar()->setTabEnabled(n, true);
            tabMatrixWidget->tabBar()->setTabIcon(n, QIcon());
            tabMatrixWidget->tabBar()->setTabText(n, QString::number(n + 16));
            tabMatrixWidget->tabBar()->setTabToolTip(n, "Select Track " + QString::number(n + 16) +
                                                " " + file->track(n + 16)->name() + "\n Click to change to Channels");
        }

        for(; n < 16; n++) {

            tabMatrixWidget->tabBar()->setTabEnabled(n, false);
            tabMatrixWidget->tabBar()->setTabIcon(n, QIcon());
            tabMatrixWidget->tabBar()->setTabText(n, QString::number(n + 16));
            tabMatrixWidget->tabBar()->setTabToolTip(n, "Empty track");
        }

        if(file->numTracks() > 16)
            tabMatrixWidget->tabBar()->setTabVisible(17, true);

    } else if(tabMatrixWidgetMode == 2) {

        tabMatrixWidget->tabBar()->setTabToolTip(16, "Mode Selector. Click to change to Tracks");
        tabMatrixWidget->tabBar()->setTabText(16, "Channels");
        upperTabWidget->setCurrentIndex(1);

        for(int n = 0; n < 16; n++) {

            tabMatrixWidget->tabBar()->setTabEnabled(n, true);
            QPixmap p(20, 12);
            p.fill(QColor(0, 0, 0, 0));

            QPainter pixPaint(&p);
            pixPaint.setPen(Qt::white);
            pixPaint.setBrush(QBrush(file->channel(n)->color()->rgb()));
            pixPaint.drawRoundedRect(0, 0, 19, 11, 2, 2);

            QIcon ico(p);

            tabMatrixWidget->tabBar()->setTabIcon(n, ico);
            tabMatrixWidget->tabBar()->setTabText(n, QString::number(n));
            tabMatrixWidget->tabBar()->setTabToolTip(n, "Select Channel " + QString::number(n) +
                QString(".\n Use double click to enable/disable Channel only selection"));
        }

        tabMatrixWidget->tabBar()->setTabVisible(17, false);
    }


}

void MainWindow::addTrack()
{

    if (file && file->numTracks() < 32) { // 32 tracks supported
/*
for Tracks = 32
Bank_MIDI[516]; 16 * (number of tracks) + 4
Prog_MIDI[516];
MAX_OUTPUT_DEVICES 31  (32 - 1) Track0 == Track1
*/

        bool ok;
        QString text = QInputDialog::getText(this, "Set Track Name",
            "Track name (New Track)", QLineEdit::Normal,
            "New Track", &ok);
        if (ok && !text.isEmpty()) {

            file->protocol()->startNewAction("Add track");
            file->addTrack();
            file->tracks()->at(file->numTracks() - 1)->setName(text);
            file->protocol()->endAction();

            updateTrackMenu();
        }
    }

    //updateTabMatrixWidget();
}

void MainWindow::muteAllTracks()
{
    if (!file)
        return;
    file->protocol()->startNewAction("Mute all tracks");
    foreach (MidiTrack* track, *(file->tracks())) {
        track->setMuted(true);
    }
    file->protocol()->endAction();
    _trackWidget->update();
}

void MainWindow::unmuteAllTracks()
{
    if (!file)
        return;
    file->protocol()->startNewAction("All tracks audible");
    foreach (MidiTrack* track, *(file->tracks())) {
        track->setMuted(false);
    }
    file->protocol()->endAction();
    _trackWidget->update();
}

void MainWindow::allTracksVisible()
{
    if (!file)
        return;
    file->protocol()->startNewAction("Show all tracks");
    foreach (MidiTrack* track, *(file->tracks())) {
        track->setHidden(false);
    }
    file->protocol()->endAction();
    _trackWidget->update();
}

void MainWindow::allTracksInvisible()
{
    if (!file)
        return;
    file->protocol()->startNewAction("Hide all tracks");
    foreach (MidiTrack* track, *(file->tracks())) {
        track->setHidden(true);
    }
    file->protocol()->endAction();
    _trackWidget->update();
}

void MainWindow::showTrackMenuClicked(QAction* action)
{
    int track = action->data().toInt();
    if (file) {
        file->protocol()->startNewAction("Show track");
        file->track(track)->setHidden(!(action->isChecked()));
        updateTrackMenu();
        _trackWidget->update();
        file->protocol()->endAction();
    }
}

void MainWindow::muteTrackMenuClicked(QAction* action)
{
    int track = action->data().toInt();
    if (file) {
        file->protocol()->startNewAction("Mute track");
        file->track(track)->setMuted(action->isChecked());
        updateTrackMenu();
        _trackWidget->update();
        file->protocol()->endAction();
    }
}

void MainWindow::selectAllFromChannel(QAction* action)
{

    if (!file) {
        return;
    }
    int channel = action->data().toInt();
    file->protocol()->startNewAction("Select all events from channel " + QString::number(channel));
    EventTool::clearSelection();
    file->channel(channel)->setVisible(true, (!file->MultitrackMode) ? -1 : file->track(NewNoteTool::editTrack())->device_index());
    foreach (MidiEvent* e, file->channel(channel)->eventMap()->values()) {
        if (e->track()->hidden()) {
            e->track()->setHidden(false);
        }

        if (e->track() == file->track(NewNoteTool::editTrack()))
            EventTool::selectEvent(e, false);
    }

    file->protocol()->endAction();
}

void MainWindow::selectAllFromTrack(QAction* action)
{

    if (!file) {
        return;
    }

    int track = action->data().toInt();
    file->protocol()->startNewAction("Select all events from track " + QString::number(track));
    EventTool::clearSelection();
    file->track(track)->setHidden(false);
    for (int channel = 0; channel < 16; channel++) {
        foreach (MidiEvent* e, file->channel(channel)->eventMap()->values()) {
            if (e->track()->number() == track) {
                file->channel(e->channel())->setVisible(true);
                EventTool::selectEvent(e, false);
            }
        }
    }
    file->protocol()->endAction();
}

void MainWindow::selectAll()
{

    if (!file) {
        return;
    }

    file->protocol()->startNewAction("Select all");

    for (int i = 0; i < 16; i++) {
        foreach (MidiEvent* event, file->channel(i)->eventMap()->values()) {
            EventTool::selectEvent(event, false, true);
        }
    }

    file->protocol()->endAction();
}

void MainWindow::transposeNSemitones()
{

    if (!file) {
        return;
    }

    QList<NoteOnEvent*> events;
    foreach (MidiEvent* event, Selection::instance()->selectedEvents()) {
        NoteOnEvent* on = dynamic_cast<NoteOnEvent*>(event);
        if (on) {
            events.append(on);
        }
    }

    if (events.isEmpty()) {
        return;
    }

    TransposeDialog* d = new TransposeDialog(events, file, this);
    d->setModal(true);
    d->show();
}

void MainWindow::copy()
{
    EventTool::copyAction();
}

void MainWindow::paste()
{
    EventTool::pasteAction();
}

void MainWindow::paste2()
{
    EventTool::pasteAction2();
}

void MainWindow::markEdited()
{
    setWindowModified(true);
}

void MainWindow::colorsByChannel()
{
    mw_matrixWidget->setColorsByChannel();
    _colorsByChannel->setChecked(true);
    _colorsByTracks->setChecked(false);
    mw_matrixWidget->registerRelayout();
    mw_matrixWidget->update();
    _miscWidget->update();
}

void MainWindow::colorsByTrack()
{
    mw_matrixWidget->setColorsByTracks();
    _colorsByChannel->setChecked(false);
    _colorsByTracks->setChecked(true);
    mw_matrixWidget->registerRelayout();
    mw_matrixWidget->update();
    _miscWidget->update();
}

void MainWindow::editChannel(int i, bool assign, int ntrack)
{
    i &= 15;

    NewNoteTool::setEditChannel(i);

    if(ntrack < 0)
        ntrack = NewNoteTool::editTrack();
    if(ntrack >= file->numTracks())
        ntrack = 0;

    // assign channel to track
    if (assign && file && file->track(ntrack)) {
        file->track(ntrack)->assignChannel(i);
    }

    MidiOutput::setStandardChannel(i);

    int prog = file->channel(i)->progAtTick(file->cursorTick(), file->track(ntrack));
    MidiOutput::sendProgram(i, prog, file->MultitrackMode ? file->track(ntrack)->device_index(): 0);

    if(tabMatrixWidgetMode == 2) {
        updateTabMatrixWidget();
        emit tabMatrixWidgetIndex(i);
    }

    updateChannelMenu();
    MainWindow::updateAll();
}

void MainWindow::editTrack(int i, bool assign)
{
    NewNoteTool::setEditTrack(i);

    // assign channel to track
    if (assign && file && file->track(i)) {
        file->track(i)->assignChannel(NewNoteTool::editChannel());
    }

    MidiInput::track_index = file->track(i)->device_index();
    FingerPatternDialog::_track_index = file->track(i)->device_index();

#ifdef USE_FLUIDSYNTH

    int track_ind = file->track(i)->fluid_index() * 32;

    for(int n = 0; n < 32; n++) {
        ToggleViewVST((n & 31), false);
    }

    for(int n = 0; n < 32; n++) {

        bool flag = false;

        if(VST_proc::VST_isLoaded(track_ind + n))
            flag = true;

        ToggleViewVST((n & 31), flag);
    }
#endif

    if(tabMatrixWidgetMode != 2) {
        if(i < 16)
            tabMatrixWidgetMode = 0;
        else
            tabMatrixWidgetMode = 1;

        updateTabMatrixWidget();
        emit tabMatrixWidgetIndex(i & 15);
    }

    updateTrackMenu();
    updateChannelMenu();
    MainWindow::updateAll();
}

void MainWindow::editTrackAndChannel(MidiTrack* track)
{
    editTrack(track->number(), false);

    if (track->assignedChannel() > -1) {
        editChannel(track->assignedChannel(), false, track->number());
    }

    tabMatrixWidgetMode = 0;
    if(track->number() > 15)
        tabMatrixWidgetMode = 1;
    updateTabMatrixWidget();

}

void MainWindow::setInstrumentForChannel(int i)
{
    InstrumentChooser* d = new InstrumentChooser(file, i, file->track(NewNoteTool::editTrack()), this);
    d->setModal(true);
    d->exec();
    delete d;

    if (i == NewNoteTool::editChannel()) {
        editChannel(i);
    }
    updateChannelMenu();
}

void MainWindow::instrumentChannel(QAction* action)
{
    if (file) {
        setInstrumentForChannel(action->data().toInt());
    }
}

void MainWindow::setSoundEffectForChannel(int i)
{
    SoundEffectChooser* d = new SoundEffectChooser(file, i, this, SOUNDEFFECTCHOOSER_ALL);
    d->exec();
    delete d;

    updateChannelMenu();
    channelWidget->update();
}

#ifdef USE_FLUIDSYNTH

void MainWindow::setLoadVSTForChannel(int channel, int flag)
{
    int index = file->track(NewNoteTool::editTrack())->fluid_index() % 3;
    VST_chan vst(this, channel + index * 32, flag);
    if(!flag) vst.exec();

    updateChannelMenu();
    channelWidget->update();
}

#endif

void MainWindow::SoundEffectChannel(QAction* action)
{
    if (file) {
        setSoundEffectForChannel(action->data().toInt());
    }

}

void MainWindow::spreadSelection()
{

    if (!file) {
        return;
    }

    bool ok;
    float numMs = QInputDialog::getDouble(this, "Set spread-time",
        "Spread time [ms]", 10,
        5, 500, 2, &ok);

    if (!ok) {
        numMs = 1;
    }

    QMultiMap<int, int> spreadChannel[19];

    foreach (MidiEvent* event, Selection::instance()->selectedEvents()) {
        if (!spreadChannel[event->channel()].values(event->line()).contains(event->midiTime())) {
            spreadChannel[event->channel()].insert(event->line(), event->midiTime());
        }
    }

    file->protocol()->startNewAction("Spread events");
    int numSpreads = 0;
    for (int i = 0; i < 19; i++) {

        MidiChannel* channel = file->channel(i);

        QList<int> seenBefore;

        foreach (int line, spreadChannel[i].keys()) {

            if (seenBefore.contains(line)) {
                continue;
            }

            seenBefore.append(line);

            foreach (int position, spreadChannel[i].values(line)) {

                QList<MidiEvent*> eventsWithAllLines = channel->eventMap()->values(position);

                QList<MidiEvent*> events;
                foreach (MidiEvent* event, eventsWithAllLines) {
                    if (event->line() == line) {
                        events.append(event);
                    }
                }

                //spread events for the channel at the given position
                int num = events.count();
                if (num > 1) {

                    float timeToInsert = file->msOfTick(position) + numMs * num / 2;

                    for (int y = 0; y < num; y++) {

                        MidiEvent* toMove = events.at(y);

                        toMove->setMidiTime(file->tick(timeToInsert), true);
                        numSpreads++;

                        timeToInsert -= numMs;
                    }
                }
            }
        }
    }
    file->protocol()->endAction();

    QMessageBox::information(this, "Spreading done", QString("Spreaded " + QString::number(numSpreads) + " events"));
}

void MainWindow::manual()
{
    QDesktopServices::openUrl(QUrl("http://www.midieditor.org/index.php?category=manual&subcategory=editor-and-components", QUrl::TolerantMode));
}

void MainWindow::changeMiscMode(int mode)
{
    _miscWidget->setMode(mode);
    if (mode == VelocityEditor || mode == TempoEditor) {
        _miscChannel->setEnabled(false);
    } else {
        _miscChannel->setEnabled(true);
    }
    if (mode == ControllEditor || mode == KeyPressureEditor) {
        _miscController->setEnabled(true);
        _miscController->clear();

        if (mode == ControllEditor) {
            for (int i = 0; i < 128; i++) {
                _miscController->addItem(MidiFile::controlChangeName(i));
            }
        } else {
            for (int i = 0; i < 128; i++) {
                _miscController->addItem("Note: " + QString::number(i));
            }
        }

        _miscController->view()->setMinimumWidth(_miscController->minimumSizeHint().width());
    } else {
        _miscController->setEnabled(false);
    }
}

void MainWindow::selectModeChanged(QAction* action)
{
    if (action == setSingleMode) {
        _miscWidget->setEditMode(SINGLE_MODE);
    }
    if (action == setLineMode) {
        _miscWidget->setEditMode(LINE_MODE);
    }
    if (action == setFreehandMode) {
        _miscWidget->setEditMode(MOUSE_MODE);
    }
}

QWidget* MainWindow::setupActions(QWidget* parent)
{

    // Menubar
    QMenu* fileMB = menuBar()->addMenu("File");
    QMenu* editMB = menuBar()->addMenu("Edit");
    QMenu* toolsMB = menuBar()->addMenu("Tools");
    QMenu* notesMB = menuBar()->addMenu("Notes");
    QMenu* viewMB = menuBar()->addMenu("View");
    QMenu* playbackMB = menuBar()->addMenu("Playback");
    QMenu* midiMB = menuBar()->addMenu("Midi");
    QMenu* helpMB = menuBar()->addMenu("Help");

    _disableWithPlay.append(fileMB);
    _disableWithPlay.append(editMB);
    _disableWithPlay.append(toolsMB);
    _disableWithPlay.append(notesMB);

    _disableLoading.append(menuBar());

    // File
    QAction* newAction = new QAction("New", this);
    newAction->setShortcut(QKeySequence::New);
    newAction->setIcon(QIcon(":/run_environment/graphics/tool/new.png"));
    connect(newAction, SIGNAL(triggered()), this, SLOT(newFile()));
    fileMB->addAction(newAction);

    QAction* loadAction = new QAction("Open...", this);
    loadAction->setShortcut(QKeySequence::Open);
    loadAction->setIcon(QIcon(":/run_environment/graphics/tool/load.png"));
    connect(loadAction, SIGNAL(triggered()), this, SLOT(load()));
    fileMB->addAction(loadAction);

    _recentPathsMenu = new QMenu("Open recent..", this);
    _recentPathsMenu->setIcon(QIcon(":/run_environment/graphics/tool/noicon.png"));
    fileMB->addMenu(_recentPathsMenu);
    connect(_recentPathsMenu, SIGNAL(triggered(QAction*)), this, SLOT(openRecent(QAction*)));

    updateRecentPathsList();

    _skipvstload = new QMenu("Skip VST SysEx events loading file", fileMB);
    fileMB->addMenu(_skipvstload);

    skipVSTLoad = 0;

    for (int i = 0; i < 17; i++) {
        _skipvstload2[i] = (i == 0) ? new QAction("All Channels", this)
                                          : new QAction("channel " + QString::number(i - 1), this);
        _skipvstload2[i]->setCheckable((i == 0) ? false : true);
        _skipvstload2[i]->setChecked(false);
        _skipvstload->addAction(_skipvstload2[i]);

        skipVSTLoad = 0;

        connect(_skipvstload2[i], QOverload<bool>::of(&QAction::triggered), [=](bool){
            if(i == 0) {
                if(skipVSTLoad == 0) skipVSTLoad = 0xffff;
                else if(skipVSTLoad == 0xffff) skipVSTLoad = 0x0;
                else skipVSTLoad = 0xffff;


                for(int n = 0; n < 16; n++)
                    _skipvstload2[n + 1]->setChecked(((skipVSTLoad >> n) & 1) != 0);
            } else {
                skipVSTLoad ^= (1 << (i - 1));
                _skipvstload2[i]->setChecked((skipVSTLoad & (1 << (i - 1))) != 0);
            }
        });
    }

    // recover file menu


    if(MidieditorMaster) {
        bool recover_flag = false;
        _recover_file = new QMenu("Recover file from Backup list", fileMB);
        int i = 0;
        _recover_file_act[0] = NULL;
        for(int n = 1; n < 17; n++) {
            _recover_file_act[n] = NULL;
            QFile f(QDir::homePath() + "/Midieditor/file_cache/_copy_path." + QString::number(n));
            QFile m(QDir::homePath() + "/Midieditor/file_cache/_copy_midi." + QString::number(n));
            if(f.exists() && m.exists()) {
                if(!recover_flag) {
                    fileMB->addMenu(_recover_file);
                    _recover_file_act[i] =  new QAction("Reset list", this);

                    _recover_file_act[i]->setCheckable(false);
                    _recover_file_act[i]->setChecked(false);
                    _recover_file->addAction(_recover_file_act[i]);

                    connect(_recover_file_act[i], QOverload<bool>::of(&QAction::triggered), [=](bool){
                        _recover_file->clear();
                        _recover_file->addAction(_recover_file_act[0]);

                        for(int n = 1; n < 17; n++) {
                           DELETE(_recover_file_act[n]);
                           QFile::remove(QDir::homePath() + "/Midieditor/file_cache/_copy_path." + QString::number(n));
                           QFile::remove(QDir::homePath() + "/Midieditor/file_cache/_copy_midi." + QString::number(n));
                        }

                    });

                    i++;

                    recover_flag = true;
                }

                QByteArray file_readed;
                QByteArray desc_readed;
                QString file_name;
                QString description;

                if(f.open(QIODevice::ReadOnly)) {


                    file_readed = f.read(1024);
                    if(f.seek(1024)) {
                        desc_readed = f.read(1024);
                    }

                    f.close();
                }

                if(!file_readed.isEmpty()) {
                    int index = file_readed.indexOf((char) 0);
                    if(index >= 0) {
                        file_readed.truncate(index);
                    }
                }

                if(!desc_readed.isEmpty()) {
                    int index = desc_readed.indexOf((char) 0);
                    if(index >= 0) {
                        desc_readed.truncate(index);
                    }
                }

                if(file_readed.isEmpty())
                    file_readed.append("Untitled Document");
                file_name = QString::fromUtf8(file_readed);

                if(!desc_readed.isEmpty())
                    description = " -> " + QString::fromUtf8(desc_readed);

                file_name= description;

                file_name = file_name.mid(file_name.lastIndexOf("/") + 1);
                file_name = file_name.mid(file_name.lastIndexOf("\\") + 1);
                file_name.truncate(32); // only 32 char for descriptive file
                file_name = QString::number(n) +": " + file_name + description;


                _recover_file_act[i] = new QAction(file_name, this);

                _recover_file_act[i]->setCheckable(false);
                _recover_file_act[i]->setChecked(false);
                _recover_file->addAction(_recover_file_act[i]);

                connect(_recover_file_act[i], QOverload<bool>::of(&QAction::triggered), [=](bool){
                    if(restore_backup(n))
                        return;
                });

            }

        }

        if(!recover_flag) {
            int i = 0;
            fileMB->addMenu(_recover_file);
            _recover_file_act[i] =  new QAction("Reset list", this);

            _recover_file_act[i]->setCheckable(false);
            _recover_file_act[i]->setChecked(false);
            _recover_file->addAction(_recover_file_act[i]);


            connect(_recover_file_act[i], QOverload<bool>::of(&QAction::triggered), [=](bool){
                _recover_file->clear();
                _recover_file->addAction(_recover_file_act[0]);
                for(int n = 1; n < 17; n++) {
                   DELETE(_recover_file_act[n]);
                   QFile::remove(QDir::homePath() + "/Midieditor/file_cache/_copy_path." + QString::number(n));
                   QFile::remove(QDir::homePath() + "/Midieditor/file_cache/_copy_midi." + QString::number(n));
                }

            });

        }
    } else {

        _recover_file = NULL;
    }
    fileMB->addSeparator();

    QAction* saveAction = new QAction("Save", this);
    saveAction->setShortcut(QKeySequence::Save);
    saveAction->setIcon(QIcon(":/run_environment/graphics/tool/save.png"));
    connect(saveAction, SIGNAL(triggered()), this, SLOT(save()));
    fileMB->addAction(saveAction);

    QAction* saveAsAction = new QAction("Save as...", this);
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    saveAsAction->setIcon(QIcon(":/run_environment/graphics/tool/saveas.png"));
    connect(saveAsAction, SIGNAL(triggered()), this, SLOT(saveas()));
    fileMB->addAction(saveAsAction);

#ifdef USE_FLUIDSYNTH

    FluidActionExportWav = new QAction("Export to WAV file...", this);

    connect(FluidActionExportWav, SIGNAL(triggered()), this, SLOT(FluidSaveAsWav()));
    fileMB->addAction(FluidActionExportWav);

    FluidActionExportMp3 = new QAction("Export to MP3 file...", this);

    connect(FluidActionExportMp3, SIGNAL(triggered()), this, SLOT(FluidSaveAsMp3()));
    fileMB->addAction(FluidActionExportMp3);

    FluidActionExportFlac = new QAction("Export to FLAC file...", this);

    connect(FluidActionExportFlac, SIGNAL(triggered()), this, SLOT(FluidSaveAsFlac()));
    fileMB->addAction(FluidActionExportFlac);
#endif

    fileMB->addSeparator();

    ActionExportMSEQ = new QAction("Export to Sequencer file (MSEQ)...", this);

    connect(ActionExportMSEQ, SIGNAL(triggered()), this, SLOT(SaveAsMSEQ()));
    fileMB->addAction(ActionExportMSEQ);

    fileMB->addSeparator();

    QAction* ImportSF2NamesAction = new QAction("Import Instrument names from .SF2/.SF3 file", this);
    connect(ImportSF2NamesAction, SIGNAL(triggered()), this, SLOT(ImportSF2Names()));
    fileMB->addAction(ImportSF2NamesAction);

    fileMB->addSeparator();

    QAction* quitAction = new QAction("Quit", this);
    quitAction->setShortcut(QKeySequence::Quit);
    quitAction->setIcon(QIcon(":/run_environment/graphics/tool/noicon.png"));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));
    fileMB->addAction(quitAction);

    // Edit

    undoAction = new QAction("Undo", this);
    undoAction->setShortcut(QKeySequence::Undo);
    undoAction->setIcon(QIcon(":/run_environment/graphics/tool/undo.png"));
    connect(undoAction, SIGNAL(triggered()), this, SLOT(undo()));
    editMB->addAction(undoAction);

    redoAction = new QAction("Redo", this);
    redoAction->setShortcut(QKeySequence::Redo);
    redoAction->setIcon(QIcon(":/run_environment/graphics/tool/redo.png"));
    connect(redoAction, SIGNAL(triggered()), this, SLOT(redo()));
    editMB->addAction(redoAction);

    editMB->addSeparator();
    clearundoredoAction = new QAction("Reset Undo/Redo list", this);
    connect(clearundoredoAction, SIGNAL(triggered()), this, SLOT(clean_undo_redo_list()));
    editMB->addAction(clearundoredoAction);

    limit_undoAction = new QAction("Limit list Undo to 64 steps", this);
    limit_undoAction->setCheckable(true);
    limit_undoAction->setChecked(_settings->value("Main/limit_undo_action", true).toBool());
    Protocol::limitUndoAction(limit_undoAction->isChecked());

    connect(limit_undoAction, SIGNAL(triggered()), this, SLOT(limit_undo_list()));
    editMB->addAction(limit_undoAction);

    editMB->addSeparator();


    QAction* selectAllAction = new QAction("Select all", this);
    selectAllAction->setToolTip("Select all visible events");
    selectAllAction->setShortcut(QKeySequence::SelectAll);
    connect(selectAllAction, SIGNAL(triggered()), this, SLOT(selectAll()));
    editMB->addAction(selectAllAction);

    _selectAllFromChannelMenu = new QMenu("Select all events from channel...", editMB);
    editMB->addMenu(_selectAllFromChannelMenu);
    connect(_selectAllFromChannelMenu, SIGNAL(triggered(QAction*)), this, SLOT(selectAllFromChannel(QAction*)));

    for (int i = 0; i < 16; i++) {
        QVariant variant(i);
        QAction* delChannelAction = new QAction(QString::number(i), this);
        delChannelAction->setData(variant);
        _selectAllFromChannelMenu->addAction(delChannelAction);
    }

    _selectAllFromTrackMenu = new QMenu("Select all events from track...", editMB);
    editMB->addMenu(_selectAllFromTrackMenu);
    connect(_selectAllFromTrackMenu, SIGNAL(triggered(QAction*)), this, SLOT(selectAllFromTrack(QAction*)));

    for (int i = 0; i < 16; i++) {
        QVariant variant(i);
        QAction* delChannelAction = new QAction(QString::number(i), this);
        delChannelAction->setData(variant);
        _selectAllFromTrackMenu->addAction(delChannelAction);
    }

    editMB->addSeparator();

    QAction* navigateSelectionUpAction = new QAction("Navigate selection up", editMB);
    navigateSelectionUpAction->setShortcut(Qt::Key_Up);
    connect(navigateSelectionUpAction, SIGNAL(triggered()), this, SLOT(navigateSelectionUp()));
    editMB->addAction(navigateSelectionUpAction);

    QAction* navigateSelectionDownAction = new QAction("Navigate selection down", editMB);
    navigateSelectionDownAction->setShortcut(Qt::Key_Down);
    connect(navigateSelectionDownAction, SIGNAL(triggered()), this, SLOT(navigateSelectionDown()));
    editMB->addAction(navigateSelectionDownAction);

    QAction* navigateSelectionLeftAction = new QAction("Navigate selection left", editMB);
    navigateSelectionLeftAction->setShortcut(Qt::Key_Left);
    connect(navigateSelectionLeftAction, SIGNAL(triggered()), this, SLOT(navigateSelectionLeft()));
    editMB->addAction(navigateSelectionLeftAction);

    QAction* navigateSelectionRightAction = new QAction("Navigate selection right", editMB);
    navigateSelectionRightAction->setShortcut(Qt::Key_Right);
    connect(navigateSelectionRightAction, SIGNAL(triggered()), this, SLOT(navigateSelectionRight()));
    editMB->addAction(navigateSelectionRightAction);

    editMB->addSeparator();

    QAction* copyAction = new QAction("Copy events", this);
    _activateWithSelections.append(copyAction);
    copyAction->setIcon(QIcon(":/run_environment/graphics/tool/copy.png"));
    copyAction->setShortcut(QKeySequence::Copy);
    connect(copyAction, SIGNAL(triggered()), this, SLOT(copy()));
    editMB->addAction(copyAction);

    _pasteAction = new QAction("Paste events", this);
    _pasteAction->setToolTip("Paste events at cursor position");
    _pasteAction->setShortcut(QKeySequence::Paste);
    _pasteAction->setIcon(QIcon(":/run_environment/graphics/tool/paste.png"));
    connect(_pasteAction, SIGNAL(triggered()), this, SLOT(paste()));

    QAction *_pasteAction2 = new QAction("Paste events from last copy (external file cache)", this);
    _pasteAction2->setToolTip("Paste events at cursor position from last copy (external file cache).\n\n"
                              "When you use 'Copy' a file is created in the file-cache directory simultaneously.\n"
                              "This makes it possible to copy and paste between different Midi files or between different instances of Midieditor");
    _pasteAction2->setIcon(QIcon(":/run_environment/graphics/tool/paste_ext.png"));
    connect(_pasteAction2, SIGNAL(triggered()), this, SLOT(paste2()));

    _pasteToTrackMenu = new QMenu("Paste to track...");
    _pasteToChannelMenu = new QMenu("Paste to channel...");
    QMenu* pasteOptionsMenu = new QMenu("Paste options...");
    pasteOptionsMenu->addMenu(_pasteToChannelMenu);
    QActionGroup* pasteChannelGroup = new QActionGroup(this);
    pasteChannelGroup->setExclusive(true);
    connect(_pasteToChannelMenu, SIGNAL(triggered(QAction*)), this, SLOT(pasteToChannel(QAction*)));
    connect(_pasteToTrackMenu, SIGNAL(triggered(QAction*)), this, SLOT(pasteToTrack(QAction*)));

    for (int i = -2; i < 16; i++) {
        QVariant variant(i);
        QString text = QString::number(i);
        if (i == -2) {
            text = "Same as selected for new events";
        }
        if (i == -1) {
            text = "Keep channel";
        }
        QAction* pasteToChannelAction = new QAction(text, this);
        pasteToChannelAction->setData(variant);
        pasteToChannelAction->setCheckable(true);
        _pasteToChannelMenu->addAction(pasteToChannelAction);
        pasteChannelGroup->addAction(pasteToChannelAction);
        pasteToChannelAction->setChecked(i < 0);
    }
    pasteOptionsMenu->addMenu(_pasteToTrackMenu);
    editMB->addAction(_pasteAction);
    editMB->addAction(_pasteAction2);
    editMB->addMenu(pasteOptionsMenu);

    editMB->addSeparator();

    QAction* configAction = new QAction("Settings...", this);
    configAction->setIcon(QIcon(":/run_environment/graphics/tool/config.png"));
    connect(configAction, SIGNAL(triggered()), this, SLOT(openConfig()));
    editMB->addAction(configAction);

    // Tools
    QMenu* toolsToolsMenu = new QMenu("Current tool...", toolsMB);

    stdTool = new StandardTool();
    Tool::setCurrentTool(stdTool);
    stdToolAction = new ToolButton(stdTool, QKeySequence(Qt::Key_F1), toolsToolsMenu);
    toolsToolsMenu->addAction(stdToolAction);
    stdTool->buttonClick();

    QAction* newNoteAction = new ToolButton(new NewNoteTool(), QKeySequence(Qt::Key_F2), toolsToolsMenu);
    toolsToolsMenu->addAction(newNoteAction);
    QAction* removeNotesAction = new ToolButton(new EraserTool(), QKeySequence(Qt::Key_F3), toolsToolsMenu);
    toolsToolsMenu->addAction(removeNotesAction);

    toolsToolsMenu->addSeparator();

    QAction* selectMultiTrack = new QAction("Select Events in Multitrack mode", this);
    selectMultiTrack->setIcon(QIcon(":/run_environment/graphics/tool/select_multitrack.png"));
    //selectMultiTrack->setImage(":/run_environment/graphics/tool/select_box.png");
    selectMultiTrack->setToolTip("Select Events in Multitrack mode.\n\nSelects all events from any channel and any track when it is checked.\n\n"
                                 "Paste will work differently, ignoring channel or track paste options.");
    selectMultiTrack->setCheckable(true);
    selectMultiTrack->setChecked(SelectTool::selectMultiTrack);

    connect(selectMultiTrack, &QAction::triggered, this, [=]() {

        SelectTool::selectMultiTrack = selectMultiTrack->isChecked();

        EventTool::clearSelection();
        EventTool::copiedEvents->clear();
        mw_matrixWidget->DeletePixmap();
        _miscWidget->update();
        mw_matrixWidget->update();
        copiedEventsChanged();

    });


    toolsToolsMenu->addAction(selectMultiTrack);


    QAction* selectSingleAction = new ToolButton(new SelectTool(SELECTION_TYPE_SINGLE), QKeySequence(Qt::Key_F4), toolsToolsMenu);
    toolsToolsMenu->addAction(selectSingleAction);
    QAction* selectBoxAction = new ToolButton(new SelectTool(SELECTION_TYPE_BOX), QKeySequence(Qt::Key_F5), toolsToolsMenu);
    toolsToolsMenu->addAction(selectBoxAction);
    QAction* selectBoxAction2 = new ToolButton(new SelectTool(SELECTION_TYPE_BOX2), 0, toolsToolsMenu);
    toolsToolsMenu->addAction(selectBoxAction2);
    QAction* selectBoxAction3 = new ToolButton(new SelectTool(SELECTION_TYPE_BOX3), 0, toolsToolsMenu);
    toolsToolsMenu->addAction(selectBoxAction3);
    QAction* selectLeftAction = new ToolButton(new SelectTool(SELECTION_TYPE_LEFT), QKeySequence(Qt::Key_F6), toolsToolsMenu);
    toolsToolsMenu->addAction(selectLeftAction);
    QAction* selectRightAction = new ToolButton(new SelectTool(SELECTION_TYPE_RIGHT), QKeySequence(Qt::Key_F7), toolsToolsMenu);
    toolsToolsMenu->addAction(selectRightAction);
    QAction* selectCursorAction = new ToolButton(new SelectTool(SELECTION_TYPE_CURSOR), 0/*QKeySequence(Qt::Key_F7)*/, toolsToolsMenu);
    toolsToolsMenu->addAction(selectCursorAction);

    toolsToolsMenu->addSeparator();

    QAction* moveAllAction = new ToolButton(new EventMoveTool(true, true), QKeySequence(Qt::Key_F8), toolsToolsMenu);
    _activateWithSelections.append(moveAllAction);
    toolsToolsMenu->addAction(moveAllAction);
    QAction* moveLRAction = new ToolButton(new EventMoveTool(false, true), QKeySequence(Qt::Key_F9), toolsToolsMenu);
    _activateWithSelections.append(moveLRAction);
    toolsToolsMenu->addAction(moveLRAction);
    QAction* moveUDAction = new ToolButton(new EventMoveTool(true, false), QKeySequence(Qt::Key_F10), toolsToolsMenu);
    _activateWithSelections.append(moveUDAction);
    toolsToolsMenu->addAction(moveUDAction);
    QAction* sizeChangeAction = new ToolButton(new SizeChangeTool(), QKeySequence(Qt::Key_F11), toolsToolsMenu);
    _activateWithSelections.append(sizeChangeAction);
    toolsToolsMenu->addAction(sizeChangeAction);

    toolsToolsMenu->addSeparator();

    QAction* measureAction= new ToolButton(new MeasureTool(), QKeySequence(Qt::Key_F12), toolsToolsMenu);
    toolsToolsMenu->addAction(measureAction);
    QAction* timeSignatureAction= new ToolButton(new TimeSignatureTool(), QKeySequence(Qt::Key_F13), toolsToolsMenu);
    toolsToolsMenu->addAction(timeSignatureAction);
    QAction* tempoAction= new ToolButton(new TempoTool(), QKeySequence(Qt::Key_F14), toolsToolsMenu);
    toolsToolsMenu->addAction(tempoAction);

    toolsMB->addMenu(toolsToolsMenu);

    // Tweak

    QMenu* tweakMenu = new QMenu("Tweak...", toolsMB);

    QAction* tweakTimeAction = new QAction("Time", tweakMenu);
    tweakTimeAction->setShortcut(Qt::Key_1);
    tweakTimeAction->setCheckable(true);
    connect(tweakTimeAction, SIGNAL(triggered()), this, SLOT(tweakTime()));
    tweakMenu->addAction(tweakTimeAction);

    QAction* tweakStartTimeAction = new QAction("Start time", tweakMenu);
    tweakStartTimeAction->setShortcut(Qt::Key_2);
    tweakStartTimeAction->setCheckable(true);
    connect(tweakStartTimeAction, SIGNAL(triggered()), this, SLOT(tweakStartTime()));
    tweakMenu->addAction(tweakStartTimeAction);

    QAction* tweakEndTimeAction = new QAction("End time", tweakMenu);
    tweakEndTimeAction->setShortcut(Qt::Key_3);
    tweakEndTimeAction->setCheckable(true);
    connect(tweakEndTimeAction, SIGNAL(triggered()), this, SLOT(tweakEndTime()));
    tweakMenu->addAction(tweakEndTimeAction);

    QAction* tweakNoteAction = new QAction("Note", tweakMenu);
    tweakNoteAction->setShortcut(Qt::Key_4);
    tweakNoteAction->setCheckable(true);
    connect(tweakNoteAction, SIGNAL(triggered()), this, SLOT(tweakNote()));
    tweakMenu->addAction(tweakNoteAction);

    QAction* tweakValueAction = new QAction("Value", tweakMenu);
    tweakValueAction->setShortcut(Qt::Key_5);
    tweakValueAction->setCheckable(true);
    connect(tweakValueAction, SIGNAL(triggered()), this, SLOT(tweakValue()));
    tweakMenu->addAction(tweakValueAction);

    QActionGroup* tweakTargetActionGroup = new QActionGroup(this);
    tweakTargetActionGroup->setExclusive(true);
    tweakTargetActionGroup->addAction(tweakTimeAction);
    tweakTargetActionGroup->addAction(tweakStartTimeAction);
    tweakTargetActionGroup->addAction(tweakEndTimeAction);
    tweakTargetActionGroup->addAction(tweakNoteAction);
    tweakTargetActionGroup->addAction(tweakValueAction);
    tweakTimeAction->setChecked(true);

    tweakMenu->addSeparator();

    QAction* tweakSmallDecreaseAction = new QAction("Small decrease", tweakMenu);
    tweakSmallDecreaseAction->setShortcut(Qt::Key_9);
    connect(tweakSmallDecreaseAction, SIGNAL(triggered()), this, SLOT(tweakSmallDecrease()));
    tweakMenu->addAction(tweakSmallDecreaseAction);

    QAction* tweakSmallIncreaseAction = new QAction("Small increase", tweakMenu);
    tweakSmallIncreaseAction->setShortcut(Qt::Key_0);
    connect(tweakSmallIncreaseAction, SIGNAL(triggered()), this, SLOT(tweakSmallIncrease()));
    tweakMenu->addAction(tweakSmallIncreaseAction);

    QAction* tweakMediumDecreaseAction = new QAction("Medium decrease", tweakMenu);
    tweakMediumDecreaseAction->setShortcut(Qt::Key_9 | Qt::ALT);
    connect(tweakMediumDecreaseAction, SIGNAL(triggered()), this, SLOT(tweakMediumDecrease()));
    tweakMenu->addAction(tweakMediumDecreaseAction);

    QAction* tweakMediumIncreaseAction = new QAction("Medium increase", tweakMenu);
    tweakMediumIncreaseAction->setShortcut(QKeyMulti(Qt::Key_0 | Qt::ALT));
    connect(tweakMediumIncreaseAction, SIGNAL(triggered()), this, SLOT(tweakMediumIncrease()));
    tweakMenu->addAction(tweakMediumIncreaseAction);

    QAction* tweakLargeDecreaseAction = new QAction("Large decrease", tweakMenu);
    tweakLargeDecreaseAction->setShortcut(QKeyMulti2(Qt::Key_9 , Qt::ALT | Qt::SHIFT));
    connect(tweakLargeDecreaseAction, SIGNAL(triggered()), this, SLOT(tweakLargeDecrease()));
    tweakMenu->addAction(tweakLargeDecreaseAction);

    QAction* tweakLargeIncreaseAction = new QAction("Large increase", tweakMenu);
    tweakLargeIncreaseAction->setShortcut(QKeyMulti2(Qt::Key_0 , Qt::ALT | Qt::SHIFT));
    connect(tweakLargeIncreaseAction, SIGNAL(triggered()), this, SLOT(tweakLargeIncrease()));
    tweakMenu->addAction(tweakLargeIncreaseAction);

    toolsMB->addMenu(tweakMenu);

    QAction* deleteAction = new QAction("Remove events", this);
    _activateWithSelections.append(deleteAction);
    deleteAction->setToolTip("Remove selected events");
    deleteAction->setShortcut(QKeySequence::Delete);
    deleteAction->setIcon(QIcon(":/run_environment/graphics/tool/eraser.png"));
    connect(deleteAction, SIGNAL(triggered()), this, SLOT(deleteSelectedEvents()));
    toolsMB->addAction(deleteAction);

    QAction* textAction = new QAction("Insert text event", this);

    textAction->setToolTip("Insert text event");
    textAction->setIcon(QIcon(":/run_environment/graphics/tool/midi_text.png"));
    connect(textAction, SIGNAL(triggered()), this, SLOT(midi_text_edit()));
    toolsMB->addAction(textAction);

    QAction* markerAction = new QAction("Insert marker event", this);

    markerAction->setToolTip("Insert marker event");
    markerAction->setIcon(QIcon(":/run_environment/graphics/tool/midi_marker.png"));
    connect(markerAction, SIGNAL(triggered()), this, SLOT(midi_marker_edit()));
    toolsMB->addAction(markerAction);

    ////////
    // Tweak

    QMenu* textevMenu = new QMenu("Insert more text events...", toolsMB);

    QAction* evLyrikAction = new QAction("Insert Lyrik event", textevMenu);
    evLyrikAction->setToolTip("Insert Lyrik event");
    evLyrikAction->setIcon(QIcon(":/run_environment/graphics/tool/midi_text.png"));
    connect(evLyrikAction, SIGNAL(triggered()), this, SLOT(midi_lyrik_edit()));
    textevMenu->addAction(evLyrikAction);

    QAction* evTracknameAction = new QAction("Insert Track Name event", textevMenu);
    evTracknameAction->setToolTip("Insert Track Name event");
    evTracknameAction->setIcon(QIcon(":/run_environment/graphics/tool/midi_text.png"));
    connect(evTracknameAction, SIGNAL(triggered()), this, SLOT(midi_track_name_edit()));
    textevMenu->addAction(evTracknameAction);

    toolsMB->addMenu(textevMenu);
    //////////

    toolsMB->addSeparator();

    QAction* alignLeftAction = new QAction("Align left", this);
    _activateWithSelections.append(alignLeftAction);
    alignLeftAction->setShortcut(QKeyMulti(Qt::Key_Left | Qt::CTRL));
    alignLeftAction->setIcon(QIcon(":/run_environment/graphics/tool/align_left.png"));
    connect(alignLeftAction, SIGNAL(triggered()), this, SLOT(alignLeft()));
    toolsMB->addAction(alignLeftAction);

    QAction* alignRightAction = new QAction("Align right", this);
    _activateWithSelections.append(alignRightAction);
    alignRightAction->setIcon(QIcon(":/run_environment/graphics/tool/align_right.png"));
    alignRightAction->setShortcut(QKeyMulti(Qt::Key_Right | Qt::CTRL));
    connect(alignRightAction, SIGNAL(triggered()), this, SLOT(alignRight()));
    toolsMB->addAction(alignRightAction);

    QAction* equalizeAction = new QAction("Equalize selection", this);
    _activateWithSelections.append(equalizeAction);
    equalizeAction->setIcon(QIcon(":/run_environment/graphics/tool/equalize.png"));
    equalizeAction->setShortcut(QKeyMulti(Qt::Key_Up | Qt::CTRL));
    connect(equalizeAction, SIGNAL(triggered()), this, SLOT(equalize()));
    toolsMB->addAction(equalizeAction);

    toolsMB->addSeparator();

    QAction* quantizeAction = new QAction("Quantify selection", this);
    _activateWithSelections.append(quantizeAction);
    quantizeAction->setIcon(QIcon(":/run_environment/graphics/tool/quantize.png"));
    quantizeAction->setShortcut(QKeyMulti(Qt::Key_G | Qt::CTRL));
    connect(quantizeAction, SIGNAL(triggered()), this, SLOT(quantizeSelection()));
    toolsMB->addAction(quantizeAction);

    QMenu* quantMenu = new QMenu("Quantization fractions", viewMB);
    QActionGroup* quantGroup = new QActionGroup(viewMB);
    quantGroup->setExclusive(true);

    for (int i = 0; i <= 5; i++) {
        QVariant variant(i);
        QString text = "";

        if (i == 0) {
            text = "Whole note";
        } else if (i == 1) {
            text = "Half note";
        } else if (i == 2) {
            text = "Quarter note";
        } else {
            text = QString::number((int)qPow(2, i)) + "th note";
        }

        QAction* a = new QAction(text, this);
        a->setData(variant);
        quantGroup->addAction(a);
        quantMenu->addAction(a);
        a->setCheckable(true);
        a->setChecked(i == _quantizationGrid);
    }
    connect(quantMenu, SIGNAL(triggered(QAction*)), this, SLOT(quantizationChanged(QAction*)));
    toolsMB->addMenu(quantMenu);

    QAction* quantizeNToleAction = new QAction("Quantify tuplet...", this);
    _activateWithSelections.append(quantizeNToleAction);
    quantizeNToleAction->setShortcut(QKeyMulti2(Qt::Key_H , Qt::CTRL | Qt::SHIFT));
    connect(quantizeNToleAction, SIGNAL(triggered()), this, SLOT(quantizeNtoleDialog()));
    toolsMB->addAction(quantizeNToleAction);

    QAction* quantizeNToleActionRepeat = new QAction("Repeat tuplet quantization", this);
    _activateWithSelections.append(quantizeNToleActionRepeat);
    quantizeNToleActionRepeat->setShortcut(QKeyMulti2(Qt::Key_H , Qt::CTRL));
    connect(quantizeNToleActionRepeat, SIGNAL(triggered()), this, SLOT(quantizeNtole()));
    toolsMB->addAction(quantizeNToleActionRepeat);

    //toolsMB->addSeparator();

    QAction* spreadAction = new QAction("Spread selection", this);
    _activateWithSelections.append(spreadAction);
    connect(spreadAction, SIGNAL(triggered()), this, SLOT(spreadSelection()));
    //toolsMB->addAction(spreadAction);

    toolsMB->addSeparator();

    QAction* addTrackAction = new QAction("Add track...", toolsMB);
    toolsMB->addAction(addTrackAction);
    connect(addTrackAction, SIGNAL(triggered()), this, SLOT(addTrack()));

    toolsMB->addSeparator();

    _deleteChannelMenu = new QMenu("Remove events from channel...", toolsMB);
    toolsMB->addMenu(_deleteChannelMenu);
    connect(_deleteChannelMenu, SIGNAL(triggered(QAction*)), this, SLOT(deleteChannel(QAction*)));

    for (int i = 0; i < 16; i++) {
        QVariant variant(i);
        QAction* delChannelAction = new QAction(QString::number(i), this);
        delChannelAction->setData(variant);
        _deleteChannelMenu->addAction(delChannelAction);
    }

    _moveSelectedEventsToChannelMenu = new QMenu("Move events to channel...", editMB);
    toolsMB->addMenu(_moveSelectedEventsToChannelMenu);
    connect(_moveSelectedEventsToChannelMenu, SIGNAL(triggered(QAction*)), this, SLOT(moveSelectedEventsToChannel(QAction*)));

    for (int i = 0; i < 16; i++) {
        QVariant variant(i);
        QAction* moveToChannelAction = new QAction(QString::number(i), this);
        moveToChannelAction->setData(variant);
        _moveSelectedEventsToChannelMenu->addAction(moveToChannelAction);
    }

    _moveSelectedNotesToChannelMenu = new QMenu("Move notes to channel...", editMB);
    toolsMB->addMenu(_moveSelectedNotesToChannelMenu);
    connect(_moveSelectedNotesToChannelMenu, SIGNAL(triggered(QAction*)), this, SLOT(moveSelectedNotesToChannel(QAction*)));

    for (int i = 0; i < 16; i++) {
        QVariant variant(i);
        QAction* moveToChannelAction = new QAction(QString::number(i), this);
        moveToChannelAction->setData(variant);
        _moveSelectedNotesToChannelMenu->addAction(moveToChannelAction);
    }

    _moveSelectedEventsToTrackMenu = new QMenu("Move events to track...", editMB);
    toolsMB->addMenu(_moveSelectedEventsToTrackMenu);
    connect(_moveSelectedEventsToTrackMenu, SIGNAL(triggered(QAction*)), this, SLOT(moveSelectedEventsToTrack(QAction*)));

    toolsMB->addSeparator();

    QAction* transposeAction = new QAction("Transpose selection...", this);
    _activateWithSelections.append(transposeAction);
    transposeAction->setShortcut(QKeyMulti2(Qt::Key_T , Qt::CTRL));
    connect(transposeAction, SIGNAL(triggered()), this, SLOT(transposeNSemitones()));
    toolsMB->addAction(transposeAction);

    toolsMB->addSeparator();

    QAction* setFileLengthMs = new QAction("Set file duration", this);
    connect(setFileLengthMs, SIGNAL(triggered()), this, SLOT(setFileLengthMs()));
    toolsMB->addAction(setFileLengthMs);

    QAction* scaleSelection = new QAction("Scale events", this);
    _activateWithSelections.append(scaleSelection);
    connect(scaleSelection, SIGNAL(triggered()), this, SLOT(scaleSelection()));
    toolsMB->addAction(scaleSelection);

    toolsMB->addSeparator();

    QAction* magnetAction = new QAction("Magnet", editMB);
    toolsMB->addAction(magnetAction);
    magnetAction->setShortcut(QKeyMulti2(Qt::Key_M , Qt::CTRL));
    magnetAction->setIcon(QIcon(":/run_environment/graphics/tool/magnet.png"));
    magnetAction->setCheckable(true);
    magnetAction->setChecked(false);
    magnetAction->setChecked(EventTool::magnetEnabled());
    connect(magnetAction, SIGNAL(toggled(bool)), this, SLOT(enableMagnet(bool)));

    QAction* velocityscale = new QAction("Velocity Scale", this);
    _activateWithSelections.append(velocityscale);
    connect(velocityscale, SIGNAL(triggered()), this, SLOT(velocityScale()));
    notesMB->addAction(velocityscale);

    notesMB->addSeparator();


    QAction* overlappedNotesAllTracks = new QAction("Overlapped Notes Correction for All Tracks", this);
    _activateWithSelections.append(overlappedNotesAllTracks);
    connect(overlappedNotesAllTracks, SIGNAL(triggered()), this, SLOT(overlappedNotesCorrectionAllTracks()));
    notesMB->addAction(overlappedNotesAllTracks);

    QAction* overlappedNotes = new QAction("Overlapped Notes Correction", this);
    _activateWithSelections.append(overlappedNotes);
    connect(overlappedNotes, SIGNAL(triggered()), this, SLOT(overlappedNotesCorrection()));
    notesMB->addAction(overlappedNotes);

    QAction* longNotes = new QAction("Long Notes Correction", this);
    _activateWithSelections.append(longNotes);
    connect(longNotes, SIGNAL(triggered()), this, SLOT(longNotesCorrection()));
    notesMB->addAction(longNotes);

    QAction* stretchNotes = new QAction("Stretch Notes", this);
    _activateWithSelections.append(stretchNotes);
    connect(stretchNotes, SIGNAL(triggered()), this, SLOT(stretchNotes()));
    notesMB->addAction(stretchNotes);

    notesMB->addSeparator();

    QMenu* buildPowers = new QMenu("Build Power Chords from Single Notes", notesMB);
    notesMB->addMenu(buildPowers);

    QAction* powerchordSelection = new QAction("Power Chord", this);
    _activateWithSelections.append(powerchordSelection);
    connect(powerchordSelection, SIGNAL(triggered()), this, SLOT(buildpowerchord()));
    buildPowers->addAction(powerchordSelection);

    QAction* powerchordInvSelection = new QAction("Power Chord Inverted", this);
    _activateWithSelections.append(powerchordInvSelection);
    connect(powerchordInvSelection, SIGNAL(triggered()), this, SLOT(buildpowerchordInv()));
    buildPowers->addAction(powerchordInvSelection);

    QAction* power2chordSelection = new QAction("Power Chord Extended", this);
    _activateWithSelections.append(power2chordSelection);
    connect(power2chordSelection, SIGNAL(triggered()), this, SLOT(buildpowerpowerchord()));
    buildPowers->addAction(power2chordSelection);

    QMenu* buildChords = new QMenu("Build Chords from Single Notes", notesMB);
    notesMB->addMenu(buildChords);

    QAction* buildMajorSelection = new QAction("Major Chord (CM)", this);
    _activateWithSelections.append(buildMajorSelection);
    connect(buildMajorSelection, SIGNAL(triggered()), this, SLOT(buildMajor()));
    buildChords->addAction(buildMajorSelection);

    QAction* buildMinorSelection = new QAction("Minor Chord (Cm)", this);
    _activateWithSelections.append(buildMinorSelection);
    connect(buildMinorSelection, SIGNAL(triggered()), this, SLOT(buildMinor()));
    buildChords->addAction(buildMinorSelection);

    QAction* buildAugSelection = new QAction("Augmented Chord (Caug)", this);
    _activateWithSelections.append(buildAugSelection);
    connect(buildAugSelection, SIGNAL(triggered()), this, SLOT(buildAug()));
    buildChords->addAction(buildAugSelection);

    QAction* buildDisSelection = new QAction("Diminished Chord (Cdim)", this);
    _activateWithSelections.append(buildDisSelection);
    connect(buildDisSelection, SIGNAL(triggered()), this, SLOT(buildDis()));
    buildChords->addAction(buildDisSelection);

    QAction* buildSeventhSelection = new QAction("Seventh Chord (C7)", this);
    _activateWithSelections.append(buildSeventhSelection);
    connect(buildSeventhSelection, SIGNAL(triggered()), this, SLOT(buildSeventh()));
    buildChords->addAction(buildSeventhSelection);

    QAction* buildMajorSeventhSelection = new QAction("Major Chord (C7M)", this);
    _activateWithSelections.append(buildMajorSeventhSelection);
    connect(buildMajorSeventhSelection, SIGNAL(triggered()), this, SLOT(buildMajorSeventh()));
    buildChords->addAction(buildMajorSeventhSelection);

    QAction* buildMinorSeventhSelection = new QAction("7 Minor Chord (C7m)", this);
    _activateWithSelections.append(buildMinorSeventhSelection);
    connect(buildMinorSeventhSelection, SIGNAL(triggered()), this, SLOT(buildMinorSeventh()));
    buildChords->addAction(buildMinorSeventhSelection);

    QAction* buildMinorSeventhMajorSelection = new QAction("Minor with 7 Major Chord Cm(7M)", this);
    _activateWithSelections.append(buildMinorSeventhMajorSelection);
    connect(buildMinorSeventhMajorSelection, SIGNAL(triggered()), this, SLOT(buildMinorSeventhMajor()));
    buildChords->addAction(buildMinorSeventhMajorSelection);

    notesMB->addSeparator();

    QMenu* buildPChords = new QMenu("Build Progression Chords from Single Notes", notesMB);
    notesMB->addMenu(buildPChords);

    QAction* buildCMajorSelection = new QAction("C Major (CM)", this);
    _activateWithSelections.append(buildCMajorSelection);
    connect(buildCMajorSelection, SIGNAL(triggered()), this, SLOT(buildCMajor()));
    buildPChords->addAction(buildCMajorSelection);

    QAction* buildCMajorInv1Selection = new QAction("C Major Inversion 1 (CM)", this);
    _activateWithSelections.append(buildCMajorInv1Selection);
    connect(buildCMajorInv1Selection, SIGNAL(triggered()), this, SLOT(buildCMajorInv1()));
    buildPChords->addAction(buildCMajorInv1Selection);

    QAction* buildCMajorInv2Selection = new QAction("C Major Inversion 2 (CM)", this);
    _activateWithSelections.append(buildCMajorInv2Selection);
    connect(buildCMajorInv2Selection, SIGNAL(triggered()), this, SLOT(buildCMajorInv2()));
    buildPChords->addAction(buildCMajorInv2Selection);

    QAction* buildCMinorSelection = new QAction("C Minor (Cm)", this);
    _activateWithSelections.append(buildCMinorSelection);
    connect(buildCMinorSelection, SIGNAL(triggered()), this, SLOT(buildCMinor()));
    buildPChords->addAction(buildCMinorSelection);

    QAction* buildCMinorInv1Selection = new QAction("C Minor Inversion 1 (Cm)", this);
    _activateWithSelections.append(buildCMinorInv1Selection);
    connect(buildCMinorInv1Selection, SIGNAL(triggered()), this, SLOT(buildCMinorInv1()));
    buildPChords->addAction(buildCMinorInv1Selection);

    QAction* buildCMinorInv2Selection = new QAction("C Minor Inversion 2 (Cm)", this);
    _activateWithSelections.append(buildCMinorInv2Selection);
    connect(buildCMinorInv2Selection, SIGNAL(triggered()), this, SLOT(buildCMinorInv2()));
    buildPChords->addAction(buildCMinorInv2Selection);

    notesMB->addSeparator();

    QAction* adjustVel = new QAction("Adjust Velocity for Chords", this);

    connect(adjustVel, SIGNAL(triggered()), this, SLOT(setChordVelocityProp()));
    notesMB->addAction(adjustVel);

    notesMB->addSeparator();

    QAction* pitchbend1 = new QAction("Pitch Bend Effect 1", this);
    _activateWithSelections.append(pitchbend1);
    connect(pitchbend1, SIGNAL(triggered()), this, SLOT(pitchbend_effect1()));
    notesMB->addAction(pitchbend1);

    QAction* volumeoff = new QAction("Volume Off Effect", this);
    _activateWithSelections.append(volumeoff);
    connect(volumeoff, SIGNAL(triggered()), this, SLOT(volumeoff_effect()));
    notesMB->addAction(volumeoff);

    QAction* choppy_audio = new QAction("Choppy Audio Effect", this);
    _activateWithSelections.append(choppy_audio);
    connect(choppy_audio, SIGNAL(triggered()), this, SLOT(choppy_audio_effect()));
    notesMB->addAction(choppy_audio);

    QAction* mute_audio = new QAction("Mute Audio Effect", this);
    _activateWithSelections.append(mute_audio);
    connect(mute_audio, SIGNAL(triggered()), this, SLOT(mute_audio_effect()));
    notesMB->addAction(mute_audio);

    notesMB->addSeparator();

    QAction* pattern_note = new QAction("Pattern Note", this);
    _activateWithSelections.append(pattern_note);
    connect(pattern_note, SIGNAL(triggered()), this, SLOT(conv_pattern_note()));
    notesMB->addAction(pattern_note);

    QAction* finger_pattern = new QAction("Finger Pattern Utility", this);
    //_activateWithSelections.append(finger_pattern);
    connect(finger_pattern, SIGNAL(triggered()), this, SLOT(finger_pattern()));
    notesMB->addAction(finger_pattern);

    // View
    QMenu* zoomMenu = new QMenu("Zoom...", viewMB);
    QAction* zoomHorOutAction = new QAction("Horizontal out", this);
    zoomHorOutAction->setShortcut(QKeyMulti2(Qt::Key_Minus , Qt::CTRL));
    zoomHorOutAction->setIcon(QIcon(":/run_environment/graphics/tool/zoom_hor_out.png"));
    connect(zoomHorOutAction, SIGNAL(triggered()),
        mw_matrixWidget, SLOT(zoomHorOut()));
    zoomMenu->addAction(zoomHorOutAction);

    QAction* zoomHorInAction = new QAction("Horizontal in", this);
    zoomHorInAction->setIcon(QIcon(":/run_environment/graphics/tool/zoom_hor_in.png"));
    zoomHorInAction->setShortcut(QKeyMulti2(Qt::Key_Plus , Qt::CTRL));
    connect(zoomHorInAction, SIGNAL(triggered()),
        mw_matrixWidget, SLOT(zoomHorIn()));
    zoomMenu->addAction(zoomHorInAction);

    QAction* zoomVerOutAction = new QAction("Vertical out", this);
    zoomVerOutAction->setIcon(QIcon(":/run_environment/graphics/tool/zoom_ver_out.png"));
    zoomVerOutAction->setShortcut(QKeyMulti2(Qt::Key_Minus , Qt::CTRL | Qt::ALT));
    connect(zoomVerOutAction, SIGNAL(triggered()),
        mw_matrixWidget, SLOT(zoomVerOut()));
    zoomMenu->addAction(zoomVerOutAction);

    QAction* zoomVerInAction = new QAction("Vertical in", this);
    zoomVerInAction->setIcon(QIcon(":/run_environment/graphics/tool/zoom_ver_in.png"));
    zoomVerInAction->setShortcut(QKeyMulti2(Qt::Key_Plus , Qt::CTRL | Qt::ALT));
    connect(zoomVerInAction, SIGNAL(triggered()),
        mw_matrixWidget, SLOT(zoomVerIn()));
    zoomMenu->addAction(zoomVerInAction);

    zoomMenu->addSeparator();

    QAction* zoomStdAction = new QAction("Restore default", this);
    zoomStdAction->setShortcut(QKeyMulti2(Qt::Key_0 , Qt::CTRL));
    connect(zoomStdAction, SIGNAL(triggered()),
        mw_matrixWidget, SLOT(zoomStd()));
    zoomMenu->addAction(zoomStdAction);

    viewMB->addMenu(zoomMenu);

    viewMB->addSeparator();

    viewMB->addAction(_allChannelsVisible);
    viewMB->addAction(_allChannelsInvisible);
    viewMB->addAction(_allTracksVisible);
    viewMB->addAction(_allTracksInvisible);

    viewMB->addSeparator();

    QMenu* colorMenu = new QMenu("Colors ...", viewMB);
    _colorsByChannel = new QAction("From channels", this);
    _colorsByChannel->setCheckable(true);
    connect(_colorsByChannel, SIGNAL(triggered()), this, SLOT(colorsByChannel()));
    colorMenu->addAction(_colorsByChannel);

    _colorsByTracks = new QAction("From tracks", this);
    _colorsByTracks->setCheckable(true);
    connect(_colorsByTracks, SIGNAL(triggered()), this, SLOT(colorsByTrack()));
    colorMenu->addAction(_colorsByTracks);

    viewMB->addMenu(colorMenu);

    viewMB->addSeparator();

    QMenu* divMenu = new QMenu("Raster", viewMB);
    QActionGroup* divGroup = new QActionGroup(viewMB);
    divGroup->setExclusive(true);

    for (int i = -1; i <= 6; i++) {
        QVariant variant(i);
        QString text = "Off";
        if (i == 0) {
            text = "Whole note";
        } else if (i == 1) {
            text = "Half note";
        } else if (i == 2) {
            text = "Quarter note";
        } else if (i > 0) {
            if(i >= 6)
                text = "Get from Rhythm Box";
            else
                text = QString::number((int)qPow(2, i)) + "th note";
        }
        QAction* a = new QAction(text, this);
        a->setData(variant);
        divGroup->addAction(a);
        divMenu->addAction(a);
        a->setCheckable(true);
        a->setChecked(i == mw_matrixWidget->div());
    }
    connect(divMenu, SIGNAL(triggered(QAction*)), this, SLOT(divChanged(QAction*)));
    viewMB->addMenu(divMenu);

    viewMB->addSeparator();
#if 0
    QAction *_viewRightPan = new QAction("Right Pannel Mode", this);
    _viewRightPan->setCheckable(true);
    _viewRightPan->setChecked(rightSplitterMode);
    viewMB->addAction(_viewRightPan);
    connect(_viewRightPan, QOverload<bool>::of(&QAction::triggered), [=](bool){
        rightSplitterMode^=1;
         _settings->setValue("Main/rightSplitterMode", rightSplitterMode);

        EventTool::clearSelection();
        delete Selection::instance();

        if(!rightSplitterMode) {
            upperTabWidget->removeTab(EventSplitterTabPos);
            delete _eventWidget;
            _eventWidget = new EventWidget(lowerTabWidget);
            Selection::_eventWidget = _eventWidget;
            EventSplitterTabPos = lowerTabWidget->count();
            lowerTabWidget->addTab(_eventWidget, "Event");

        } else {
            lowerTabWidget->removeTab(EventSplitterTabPos);
            delete _eventWidget;
            _eventWidget = new EventWidget(upperTabWidget);
            Selection::_eventWidget = _eventWidget;
            EventSplitterTabPos = upperTabWidget->count();
            upperTabWidget->addTab(_eventWidget, "Event");

        }

        MidiEvent::setEventWidget(_eventWidget);
        Selection::setFile(file);
        _eventWidget->setFile(file);
        connect(file->protocol(), SIGNAL(actionFinished()), eventWidget(), SLOT(reload()));
        _eventWidget->update();

        if(rightSplitterMode) {
            upperTabWidget->setFixedSize(rightSplitter->width(), rightSplitter->height() * 7/10 - 18);
            rightSplitter->setStretchFactor(0, 8);
            rightSplitter->setStretchFactor(1, 2);

        } else {
            upperTabWidget->setFixedSize(rightSplitter->width(), rightSplitter->height() * 5/10 - 18);
            rightSplitter->setStretchFactor(0, 5);
            rightSplitter->setStretchFactor(1, 5);

        }

        rightSplitter->update();

    });
#endif

    QAction *_backShadowSel = new QAction("Notes Selection Background Shadow", this);
    _backShadowSel->setCheckable(true);
    _backShadowSel->setChecked(shadow_selection);
    viewMB->addAction(_backShadowSel);
    connect(_backShadowSel, QOverload<bool>::of(&QAction::triggered), [=](bool){
        shadow_selection^=1;
         _settings->setValue("Main/shadow_selection", shadow_selection);
         mw_matrixWidget->shadow_selection = shadow_selection;
         mw_matrixWidget->registerRelayout();
         mw_matrixWidget->update();

    });

    viewMB->addSeparator();

    QMenu* textMenu = new QMenu("Text/Karaoke Events", viewMB);
    viewMB->addMenu(textMenu);

    viewMB->addSeparator();

    QAction *_viewKaraoke = new QAction("View karaoke text", this);
    _viewKaraoke->setCheckable(true);
    _viewKaraoke->setChecked(mw_matrixWidget->visible_TimeLineArea4);
    textMenu->addAction(_viewKaraoke);
    connect(_viewKaraoke, QOverload<bool>::of(&QAction::triggered), [=](bool){
        mw_matrixWidget->visible_karaoke^= 1;
        _settings->setValue("Main/visible_karaoke", mw_matrixWidget->visible_karaoke);
        mw_matrixWidget->registerRelayout();

    });

    QMenu* timMenu = new QMenu("Time Line Edit Box", viewMB);
    viewMB->addMenu(timMenu);

    viewMB->addSeparator();

    QAction *_viewCtrl = new QAction("View Control Events", this);
    _viewCtrl->setCheckable(true);
    _viewCtrl->setChecked(mw_matrixWidget->visible_Controlflag);
    timMenu->addAction(_viewCtrl);
    connect(_viewCtrl, QOverload<bool>::of(&QAction::triggered), [=](bool){
        mw_matrixWidget->visible_Controlflag^= 1;
        _settings->setValue("Main/visible_Controlflag", mw_matrixWidget->visible_Controlflag);
        mw_matrixWidget->registerRelayout();

    });

    QAction *_viewPitch = new QAction("View Pitch Events", this);
    _viewPitch->setCheckable(true);
    _viewPitch->setChecked(mw_matrixWidget->visible_PitchBendflag);
    timMenu->addAction(_viewPitch);
    connect(_viewPitch, QOverload<bool>::of(&QAction::triggered), [=](bool){
        mw_matrixWidget->visible_PitchBendflag^= 1;
        _settings->setValue("Main/visible_PitchBendflag", mw_matrixWidget->visible_PitchBendflag);
        mw_matrixWidget->registerRelayout();

    });

    QAction *_viewTim3 = new QAction("View Prog and SysEx* Events", this);
    _viewTim3->setCheckable(true);
    _viewTim3->setChecked(mw_matrixWidget->visible_TimeLineArea3);
    timMenu->addAction(_viewTim3);
    connect(_viewTim3, QOverload<bool>::of(&QAction::triggered), [=](bool){
        mw_matrixWidget->visible_TimeLineArea3^= 1;
        _settings->setValue("Main/visible_TimeLineArea3", mw_matrixWidget->visible_TimeLineArea3);
        mw_matrixWidget->registerRelayout();

    });

    QAction *_viewTim4 = new QAction("View Text Events", this);
    _viewTim4->setCheckable(true);
    _viewTim4->setChecked(mw_matrixWidget->visible_TimeLineArea4);
    timMenu->addAction(_viewTim4);
    connect(_viewTim4, QOverload<bool>::of(&QAction::triggered), [=](bool){
        mw_matrixWidget->visible_TimeLineArea4^= 1;
        _settings->setValue("Main/visible_TimeLineArea4", mw_matrixWidget->visible_TimeLineArea4);
        mw_matrixWidget->registerRelayout();

    });

    // Playback
    QAction* playStopAction = new QAction("PlayStop", this);
    QList<QKeySequence> playStopActionShortcuts;
    playStopActionShortcuts << QKeySequence(Qt::Key_Space)
                            << QKeyMulti(Qt::Key_K)
                            << QKeyMulti2(Qt::Key_P , Qt::CTRL);
    playStopAction->setShortcuts(playStopActionShortcuts);
    connect(playStopAction, SIGNAL(triggered()), this, SLOT(playStop()));
    playbackMB->addAction(playStopAction);

    QAction* playAction = new QAction("Play", this);
    playAction->setIcon(QIcon(":/run_environment/graphics/tool/play.png"));
    connect(playAction, SIGNAL(triggered()), this, SLOT(play()));
    playbackMB->addAction(playAction);

    QAction* pauseAction = new QAction("Pause", this);
    pauseAction->setIcon(QIcon(":/run_environment/graphics/tool/pause.png"));
#ifdef Q_OS_MAC
    pauseAction->setShortcut(QKeyMulti2(Qt::Key_Space , Qt::META));
#else
    pauseAction->setShortcut(QKeyMulti2(Qt::Key_Space , Qt::CTRL));
#endif
    connect(pauseAction, SIGNAL(triggered()), this, SLOT(pause()));

#ifdef USE_FLUIDSYNTH
    connect(fluid_output, SIGNAL(pause_player()), this, SLOT(pause()));
    connect(fluid_output, SIGNAL(message_timeout(QString, QString)), this, SLOT(message_timeout(QString, QString)));
#endif
    playbackMB->addAction(pauseAction);

    QAction* recAction = new QAction("Record", this);
    recAction->setIcon(QIcon(":/run_environment/graphics/tool/record.png"));
    recAction->setShortcut(QKeyMulti2(Qt::Key_R , Qt::CTRL));
    connect(recAction, SIGNAL(triggered()), this, SLOT(record()));
    playbackMB->addAction(recAction);

    QAction* stopAction = new QAction("Stop", this);
    stopAction->setIcon(QIcon(":/run_environment/graphics/tool/stop.png"));
    connect(stopAction, SIGNAL(triggered()), this, SLOT(stop()));
    playbackMB->addAction(stopAction);

    playbackMB->addSeparator();

    QAction* backToBeginAction = new QAction("Back to begin", this);
    backToBeginAction->setIcon(QIcon(":/run_environment/graphics/tool/back_to_begin.png"));
    QList<QKeySequence> backToBeginActionShortcuts;
    backToBeginActionShortcuts << QKeyMulti2(Qt::Key_Up , Qt::ALT)
                               << QKeyMulti2(Qt::Key_Home , Qt::ALT)
                               << QKeyMulti2(Qt::Key_J , Qt::SHIFT);
    backToBeginAction->setShortcuts(backToBeginActionShortcuts);
    connect(backToBeginAction, SIGNAL(triggered()), this, SLOT(backToBegin()));
    playbackMB->addAction(backToBeginAction);

    QAction* backAction = new QAction("Previous measure", this);
    backAction->setIcon(QIcon(":/run_environment/graphics/tool/back.png"));
    QList<QKeySequence> backActionShortcuts;
    backActionShortcuts << QKeyMulti2(Qt::Key_Left , Qt::ALT)
                        << QKeyMulti(Qt::Key_J);
    backAction->setShortcuts(backActionShortcuts);
    connect(backAction, SIGNAL(triggered()), this, SLOT(back()));
    playbackMB->addAction(backAction);

    QAction* forwAction = new QAction("Next measure", this);
    forwAction->setIcon(QIcon(":/run_environment/graphics/tool/forward.png"));
    QList<QKeySequence> forwActionShortcuts;
    forwActionShortcuts << QKeyMulti2(Qt::Key_Right , Qt::ALT)
                        << QKeyMulti(Qt::Key_L);
    forwAction->setShortcuts(forwActionShortcuts);
    connect(forwAction, SIGNAL(triggered()), this, SLOT(forward()));
    playbackMB->addAction(forwAction);

    playbackMB->addSeparator();

    QAction* backMarkerAction = new QAction("Previous marker", this);
    backMarkerAction->setIcon(QIcon(":/run_environment/graphics/tool/back_marker.png"));
    QList<QKeySequence> backMarkerActionShortcuts;
    backMarkerAction->setShortcut(QKeyMulti2(Qt::Key_Comma , Qt::ALT));
    connect(backMarkerAction, SIGNAL(triggered()), this, SLOT(backMarker()));
    playbackMB->addAction(backMarkerAction);

    QAction* forwMarkerAction = new QAction("Next marker", this);
    forwMarkerAction->setIcon(QIcon(":/run_environment/graphics/tool/forward_marker.png"));
    QList<QKeySequence> forwMarkerActionShortcuts;
    forwMarkerAction->setShortcut(QKeyMulti2(Qt::Key_Period , Qt::ALT));
    connect(forwMarkerAction, SIGNAL(triggered()), this, SLOT(forwardMarker()));
    playbackMB->addAction(forwMarkerAction);

    playbackMB->addSeparator();

    QMenu* speedMenu = new QMenu("Playback speed...");
    connect(speedMenu, SIGNAL(triggered(QAction*)), this, SLOT(setSpeed(QAction*)));

    QList<double> speeds;
    speeds.append(0.25);
    speeds.append(0.5);
    speeds.append(0.75);
    speeds.append(1);
    speeds.append(1.25);
    speeds.append(1.5);
    speeds.append(1.75);
    speeds.append(2);
    QActionGroup* speedGroup = new QActionGroup(this);
    speedGroup->setExclusive(true);

    foreach (double s, speeds) {
        QAction* speedAction = new QAction(QString::number(s), this);
        speedAction->setData(QVariant::fromValue(s));
        speedMenu->addAction(speedAction);
        speedGroup->addAction(speedAction);
        speedAction->setCheckable(true);
        speedAction->setChecked(s == 1);
    }

    playbackMB->addMenu(speedMenu);

    playbackMB->addSeparator();

    playbackMB->addAction(_allChannelsAudible);
    playbackMB->addAction(_allChannelsMute);
    playbackMB->addAction(_allTracksAudible);
    playbackMB->addAction(_allTracksMute);

    playbackMB->addSeparator();

    QAction* lockAction = new QAction("Lock screen while playing", this);
    lockAction->setIcon(QIcon(":/run_environment/graphics/tool/screen_unlocked.png"));
    lockAction->setCheckable(true);
    connect(lockAction, SIGNAL(toggled(bool)), this, SLOT(screenLockPressed(bool)));
    playbackMB->addAction(lockAction);
    lockAction->setChecked(mw_matrixWidget->screenLocked());

    QAction* metronomeAction = new QAction("Metronome", this);
    metronomeAction->setIcon(QIcon(":/run_environment/graphics/tool/metronome.png"));
    metronomeAction->setCheckable(true);
    metronomeAction->setChecked(Metronome::enabled());
    connect(metronomeAction, SIGNAL(toggled(bool)), this, SLOT(enableMetronome(bool)));
    playbackMB->addAction(metronomeAction);

    // Midi
    QAction* configAction2 = new QAction("Settings...", this);
    configAction2->setIcon(QIcon(":/run_environment/graphics/tool/config.png"));
    connect(configAction2, SIGNAL(triggered()), this, SLOT(openConfig()));
    midiMB->addAction(configAction2);

    QAction* thruAction = new QAction("Connect Midi In/Out", this);
    thruAction->setIcon(QIcon(":/run_environment/graphics/tool/connection.png"));
    thruAction->setCheckable(true);
    thruAction->setChecked(MidiInput::thru());
    connect(thruAction, SIGNAL(toggled(bool)), this, SLOT(enableThru(bool)));
    midiMB->addAction(thruAction);

    QAction* VirtualDevAction = new QAction("Virtual keyboard device", this);
    VirtualDevAction->setIcon(QIcon(":/run_environment/graphics/custom/virtualkb.png"));
    connect(VirtualDevAction, SIGNAL(triggered()), this, SLOT(VirtualKeyboard()));

    midiMB->addAction(VirtualDevAction);

    QAction* MidiInAction = new QAction("MIDI In Control", this);
    MidiInAction->setIcon(QIcon(":/run_environment/graphics/tool/config_midiin.png"));
    connect(MidiInAction, SIGNAL(triggered()), this, SLOT(DMidiInControl()));
    midiMB->addAction(MidiInAction);

    midiMB->addSeparator();

#ifdef USE_FLUIDSYNTH

    QAction* FluidAction = new QAction("Fluid Synth Control", this);
    FluidAction->setIcon(QIcon(":/run_environment/graphics/tool/config_fluid.png"));
    connect(FluidAction, SIGNAL(triggered()), this, SLOT(FluidControl()));
    midiMB->addAction(FluidAction);

 #endif

    QAction* PianoAction = new QAction("Piano Play/Record", this);
    PianoAction->setIcon(QIcon(":/run_environment/graphics/channelwidget/instrument.png"));
    connect(PianoAction, SIGNAL(triggered()), this, SLOT(PianoPlay()));

    midiMB->addAction(PianoAction);

    QAction* DrumAction = new QAction("Drum Play/Record", this);
    DrumAction->setIcon(QIcon(":/run_environment/graphics/tool/drum.png"));
    connect(DrumAction, SIGNAL(triggered()), this, SLOT(DrumPlay()));
    midiMB->addAction(DrumAction);

    QAction* DrumBoxAction = new QAction("Drum Rhythm Box", this);
    DrumBoxAction->setIcon(QIcon(":/run_environment/graphics/tool/drumbox.png"));
    connect(DrumBoxAction, SIGNAL(triggered()), this, SLOT(DrumRhythmBox()));
    midiMB->addAction(DrumBoxAction);


    midiMB->addSeparator();

    QAction* panicAction = new QAction("Midi panic", this);
    panicAction->setIcon(QIcon(":/run_environment/graphics/tool/panic.png"));
    panicAction->setShortcut(QKeySequence(Qt::Key_Escape));
    connect(panicAction, SIGNAL(triggered()), this, SLOT(panic()));
    midiMB->addAction(panicAction);

    // Help
    QAction* manualAction = new QAction("Manual", this);
    connect(manualAction, SIGNAL(triggered()), this, SLOT(manual()));
    helpMB->addAction(manualAction);

    QAction* aboutAction = new QAction("About MidiEditor", this);
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));
    helpMB->addAction(aboutAction);

#ifndef CUSTOM_MIDIEDITOR
    QAction* donateAction = new QAction("Donate", this);
    connect(donateAction, SIGNAL(triggered()), this, SLOT(donate()));
    helpMB->addAction(donateAction);
#endif
    QWidget* buttonBar = new QWidget(parent);
    QGridLayout* btnLayout = new QGridLayout(buttonBar);

    _disableLoading.append(buttonBar);

    buttonBar->setLayout(btnLayout);
    btnLayout->setSpacing(0);
    buttonBar->setContentsMargins(0, 0, 0, 0);

    QToolBar* fileTB = new QToolBar("File", buttonBar);

    _disableWithPlay.append((QWidget *) fileTB);

    fileTB->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    fileTB->setFloatable(false);
    fileTB->setContentsMargins(0, 0, 0, 0);
    fileTB->layout()->setSpacing(0);
    fileTB->setIconSize(QSize(35, 35));
    fileTB->addAction(newAction);

#ifdef CUSTOM_MIDIEDITOR_GUI
    QString ss = "QToolBar { border: 0px } QToolBar::separator {background: darkGray; width: 2px;  height: 10px;\n"
                 "margin-left: 3px; margin-right: 3px; }";

    // Estwald Color Changes
    fileTB->setStyleSheet(ss);
#else
    fileTB->setStyleSheet("QToolBar { border: 0px }");
#endif

    QAction* loadAction2 = new QAction("Open...", this);
    loadAction2->setIcon(QIcon(":/run_environment/graphics/tool/load.png"));
    connect(loadAction2, SIGNAL(triggered()), this, SLOT(load()));
    loadAction2->setMenu(_recentPathsMenu);
    fileTB->addAction(loadAction2);

    fileTB->addAction(saveAction);
    fileTB->addSeparator();

    fileTB->addAction(undoAction);
    fileTB->addAction(redoAction);
    fileTB->addSeparator();

    btnLayout->addWidget(fileTB, 0, 0, 2, 1);

    if (QApplication::arguments().contains("--large-playback-toolbar")) {

        QToolBar* playTB = new QToolBar("Playback", buttonBar);

#ifdef CUSTOM_MIDIEDITOR_GUI
    // Estwald Color Changes
    playTB->setStyleSheet(ss);
#endif

        playTB->setFloatable(false);
        playTB->setContentsMargins(0, 0, 0, 0);
        playTB->layout()->setSpacing(0);
        playTB->setIconSize(QSize(35, 35));

        playTB->addAction(backToBeginAction);
        playTB->addAction(backMarkerAction);
        playTB->addAction(backAction);
        playTB->addAction(playAction);
        playTB->addAction(pauseAction);
        playTB->addAction(stopAction);
        playTB->addAction(recAction);
        playTB->addAction(forwAction);
        playTB->addAction(forwMarkerAction);
        playTB->addSeparator();

        btnLayout->addWidget(playTB, 0, 1, 2, 1);
    }

    QToolBar* upperTB = new QToolBar(buttonBar);
    QToolBar* lowerTB = new QToolBar(buttonBar);
    _disableWithPlay.append((QWidget *) upperTB);
    btnLayout->addWidget(upperTB, 0, 2, 1, 1);
    btnLayout->addWidget(lowerTB, 1, 2, 1, 1);
    upperTB->setFloatable(false);
    upperTB->setContentsMargins(0, 0, 0, 0);
    upperTB->layout()->setSpacing(0);
    upperTB->setIconSize(QSize(20, 20));
    lowerTB->setFloatable(false);
    lowerTB->setContentsMargins(0, 0, 0, 0);
    lowerTB->layout()->setSpacing(0);
    lowerTB->setIconSize(QSize(20, 20));

#ifdef CUSTOM_MIDIEDITOR_GUI
    // Estwald Color Changes
    lowerTB->setStyleSheet(ss);
    upperTB->setStyleSheet(ss);
#else
    lowerTB->setStyleSheet("QToolBar { border: 0px }");
    upperTB->setStyleSheet("QToolBar { border: 0px }");
#endif

    lowerTB->addAction(copyAction);

    pasteActionTB = new QAction("Paste events", this);
    pasteActionTB->setToolTip("Paste events at cursor position");
    pasteActionTB->setIcon(QIcon(":/run_environment/graphics/tool/paste.png"));
    connect(pasteActionTB, SIGNAL(triggered()), this, SLOT(paste()));
    pasteActionTB->setMenu(pasteOptionsMenu);

    lowerTB->addAction(pasteActionTB);

    lowerTB->addAction(_pasteAction2);

    lowerTB->addSeparator();

    lowerTB->addAction(zoomHorInAction);
    lowerTB->addAction(zoomHorOutAction);
    lowerTB->addAction(zoomVerInAction);
    lowerTB->addAction(zoomVerOutAction);

    lowerTB->addSeparator();

    if (!QApplication::arguments().contains("--large-playback-toolbar")) {

        lowerTB->addAction(backToBeginAction);
        lowerTB->addAction(backMarkerAction);
        lowerTB->addAction(backAction);
        lowerTB->addAction(playAction);
        lowerTB->addAction(pauseAction);
        lowerTB->addAction(stopAction);
        lowerTB->addAction(recAction);
        lowerTB->addAction(forwAction);
        lowerTB->addAction(forwMarkerAction);
        lowerTB->addSeparator();
    }

    lowerTB->addAction(lockAction);
    lowerTB->addAction(metronomeAction);
    lowerTB->addSeparator();
    lowerTB->addAction(thruAction);
    lowerTB->addAction(panicAction);
    lowerTB->addAction(MidiInAction);

#ifdef USE_FLUIDSYNTH
    lowerTB->addAction(FluidAction);
#endif

    lowerTB->addAction(VirtualDevAction);

    lowerTB->addSeparator();
    lowerTB->addAction(PianoAction);
    lowerTB->addAction(DrumAction);
    lowerTB->addAction(DrumBoxAction);

    upperTB->addAction(stdToolAction);
    upperTB->addAction(newNoteAction);
    upperTB->addAction(removeNotesAction);
    upperTB->addAction(textAction);
    upperTB->addAction(markerAction);
    upperTB->addSeparator();
    upperTB->addAction(selectMultiTrack);
    upperTB->addAction(selectSingleAction);
    upperTB->addAction(selectBoxAction);
    upperTB->addAction(selectBoxAction2);
    upperTB->addAction(selectBoxAction3);
    upperTB->addAction(selectLeftAction);
    upperTB->addAction(selectRightAction);
    upperTB->addAction(selectCursorAction);
    upperTB->addSeparator();
    upperTB->addAction(moveAllAction);
    upperTB->addAction(moveLRAction);
    upperTB->addAction(moveUDAction);
    upperTB->addAction(sizeChangeAction);
    upperTB->addSeparator();
    upperTB->addAction(measureAction);
    upperTB->addAction(timeSignatureAction);
    upperTB->addAction(tempoAction);
    upperTB->addSeparator();
    upperTB->addAction(alignLeftAction);
    upperTB->addAction(alignRightAction);
    upperTB->addAction(equalizeAction);
    upperTB->addAction(quantizeAction);
    upperTB->addAction(magnetAction);

    btnLayout->setColumnStretch(4, 1);

    return buttonBar;
}

void MainWindow::pasteToChannel(QAction* action)
{
    EventTool::setPasteChannel(action->data().toInt());
}

void MainWindow::pasteToTrack(QAction* action)
{
    EventTool::setPasteTrack(action->data().toInt());
}

void MainWindow::divChanged(QAction* action)
{
    _settings->setValue("Main/get_bpm_ms", MyInstrument::get_bpm_ms());
    mw_matrixWidget->bpm_rhythm_ms = MyInstrument::get_bpm_ms();
    mw_matrixWidget->setDiv(action->data().toInt());
}

void MainWindow::enableMagnet(bool enable)
{
    EventTool::enableMagnet(enable);
}

void MainWindow::openConfig()
{
    if (MidiPlayer::isPlaying() || MidiInput::recording()) {

        stop();
    }

    if(MidiIn_control)
        MidiIn_control->hide();

#ifdef ENABLE_REMOTE
    SettingsDialog* d = new SettingsDialog("Settings", _settings, _remoteServer, this);
#else
    SettingsDialog* d = new SettingsDialog("Settings", _settings, 0, this);
#endif
    connect(d, SIGNAL(settingsChanged()), this, SLOT(updateAllTrk()));
    d->show();

    int x = (this->width() - d->width())/2;
    if(x < 0) x = 0;

    d->move(x, 0);


}

void MainWindow::enableMetronome(bool enable)
{
    Metronome::setEnabled(enable);
}

void MainWindow::enableThru(bool enable)
{
    MidiInput::setThruEnabled(enable);
}

void MainWindow::quantizationChanged(QAction* action)
{
    _quantizationGrid = action->data().toInt();
}

void MainWindow::quantizeSelection()
{

    if (!file) {
        return;
    }

    // get list with all quantization ticks
    QList<int> ticks = file->quantization(_quantizationGrid);

    file->protocol()->startNewAction("Quantify events", new QImage(":/run_environment/graphics/tool/quantize.png"));
    foreach (MidiEvent* e, Selection::instance()->selectedEvents()) {
        int onTime = e->midiTime();
        e->setMidiTime(quantize(onTime, ticks));
        OnEvent* on = dynamic_cast<OnEvent*>(e);
        if (on) {
            MidiEvent* off = on->offEvent();
            off->setMidiTime(quantize(off->midiTime(), ticks)-1);
            if (off->midiTime() <= on->midiTime()) {
                int idx = ticks.indexOf(off->midiTime()+1);
                if ((idx >= 0) && (ticks.size() > idx + 1)) {
                    off->setMidiTime(ticks.at(idx + 1) - 1);
                }
            }
        }
    }
    file->protocol()->endAction();
}

int MainWindow::quantize(int t, QList<int> ticks)
{

    int min = -1;

    for (int j = 0; j < ticks.size(); j++) {

        if (min < 0) {
            min = j;
            continue;
        }

        int i = ticks.at(j);

        int dist = t - i;

        int a = qAbs(dist);
        int b = qAbs(t - ticks.at(min));

        if (a < b) {
            min = j;
        }

        if (dist < 0) {
            return ticks.at(min);
        }
    }
    return ticks.last();
}

void MainWindow::quantizeNtoleDialog()
{

    if (!file || Selection::instance()->selectedEvents().isEmpty()) {
        return;
    }

    NToleQuantizationDialog* d = new NToleQuantizationDialog(this);
    d->setModal(true);
    if (d->exec()) {

        quantizeNtole();
    }
}

void MainWindow::quantizeNtole()
{

    if (!file || Selection::instance()->selectedEvents().isEmpty()) {
        return;
    }

    // get list with all quantization ticks
    QList<int> ticks = file->quantization(_quantizationGrid);

    file->protocol()->startNewAction("Quantify tuplet", new QImage(":/run_environment/graphics/tool/quantize.png"));

    // find minimum starting time
    int startTick = -1;
    foreach (MidiEvent* e, Selection::instance()->selectedEvents()) {
        int onTime = e->midiTime();
        if ((startTick < 0) || (onTime < startTick)) {
            startTick = onTime;
        }
    }

    // quantize start tick
    startTick = quantize(startTick, ticks);

    // compute new quantization grid
    QList<int> ntoleTicks;
    int ticksDuration = (NToleQuantizationDialog::replaceNumNum * file->ticksPerQuarter() * 4) / (qPow(2, NToleQuantizationDialog::replaceDenomNum));
    int fractionSize = ticksDuration / NToleQuantizationDialog::ntoleNNum;

    for (int i = 0; i <= NToleQuantizationDialog::ntoleNNum; i++) {
        ntoleTicks.append(startTick + i * fractionSize);
    }

    // quantize
    foreach (MidiEvent* e, Selection::instance()->selectedEvents()) {
        int onTime = e->midiTime();
        e->setMidiTime(quantize(onTime, ntoleTicks));
        OnEvent* on = dynamic_cast<OnEvent*>(e);
        if (on) {
            MidiEvent* off = on->offEvent();
            off->setMidiTime(quantize(off->midiTime(), ntoleTicks));
            if (off->midiTime() == on->midiTime()) {
                int idx = ntoleTicks.indexOf(off->midiTime());
                if ((idx >= 0) && (ntoleTicks.size() > idx + 1)) {
                    off->setMidiTime(ntoleTicks.at(idx + 1));
                } else if ((ntoleTicks.size() == idx + 1)) {
                    on->setMidiTime(ntoleTicks.at(idx - 1));
                }
            }
        }
    }
    file->protocol()->endAction();
}

void MainWindow::setSpeed(QAction* action)
{
    double d = action->data().toDouble();
    MidiPlayer::setSpeedScale(d);
}

void MainWindow::checkEnableActionsForSelection()
{
    bool enabled = Selection::instance()->selectedEvents().size() > 0;
    foreach (QAction* action, _activateWithSelections) {
        action->setEnabled(enabled);
    }
    if (_moveSelectedEventsToChannelMenu) {
        _moveSelectedEventsToChannelMenu->setEnabled(enabled);
    }
    if (_moveSelectedNotesToChannelMenu) {
        _moveSelectedNotesToChannelMenu->setEnabled(enabled);
    }
    if (_moveSelectedEventsToTrackMenu) {
        _moveSelectedEventsToTrackMenu->setEnabled(enabled);
    }
    if (Tool::currentTool() && Tool::currentTool()->button() && !Tool::currentTool()->button()->isEnabled()) {
        stdToolAction->trigger();
    }
    if (file) {
        undoAction->setEnabled(file->protocol()->stepsBack() > 1);
        redoAction->setEnabled(file->protocol()->stepsForward() > 0);
    }
}

void MainWindow::toolChanged()
{
    checkEnableActionsForSelection();
    _miscWidget->update();
    mw_matrixWidget->update();
}

void MainWindow::copiedEventsChanged()
{
    bool enable = EventTool::copiedEvents->size() > 0;
    _pasteAction->setEnabled(enable);
    pasteActionTB->setEnabled(enable);
}

#ifndef CUSTOM_MIDIEDITOR
void MainWindow::updateDetected(Update* update)
{

    UpdateDialog* d = new UpdateDialog(update, this);
    d->setModal(true);
    d->exec();
    delete d;

}
#endif
void MainWindow::promtUpdatesDeactivatedDialog() {

    AutomaticUpdateDialog* d = new AutomaticUpdateDialog(this);
    d->setModal(true);
    d->exec();
    delete d;
}

void MainWindow::updateAll() {
    mw_matrixWidget->registerRelayout();
    mw_matrixWidget->update();
    channelWidget->update();
    _trackWidget->update();
    _miscWidget->update();

}

void MainWindow::updateAllTrk() {

    editTrack(NewNoteTool::editTrack(), false);
    updateAll();

}

void MainWindow::tweakTime() {
    delete currentTweakTarget;
    currentTweakTarget = new TimeTweakTarget(this);
}

void MainWindow::tweakStartTime() {
    delete currentTweakTarget;
    currentTweakTarget = new StartTimeTweakTarget(this);
}

void MainWindow::tweakEndTime() {
    delete currentTweakTarget;
    currentTweakTarget = new EndTimeTweakTarget(this);
}

void MainWindow::tweakNote() {
    delete currentTweakTarget;
    currentTweakTarget = new NoteTweakTarget(this);
}

void MainWindow::tweakValue() {
    delete currentTweakTarget;
    currentTweakTarget = new ValueTweakTarget(this);
}

void MainWindow::tweakSmallDecrease() {
    currentTweakTarget->smallDecrease();
}

void MainWindow::tweakSmallIncrease() {
    currentTweakTarget->smallIncrease();
}

void MainWindow::tweakMediumDecrease() {
    currentTweakTarget->mediumDecrease();
}

void MainWindow::tweakMediumIncrease() {
    currentTweakTarget->mediumIncrease();
}

void MainWindow::tweakLargeDecrease() {
    currentTweakTarget->largeDecrease();
}

void MainWindow::tweakLargeIncrease() {
    currentTweakTarget->largeIncrease();
}

void MainWindow::navigateSelectionUp() {
    selectionNavigator->up();
}

void MainWindow::navigateSelectionDown() {
    selectionNavigator->down();
}

void MainWindow::navigateSelectionLeft() {
    selectionNavigator->left();
}

void MainWindow::navigateSelectionRight() {
    selectionNavigator->right();
}

void MainWindow::update_channel_list() {
    channelWidget->update();
}

void MainWindow::message_timeout(QString title, QString message) {

    QMessageBox *mb = new QMessageBox(this);
    mb->setWindowTitle(title);
    mb->setText(message);
    mb->setIcon(QMessageBox::Critical);
    mb->setStandardButtons(QMessageBox::Ok);
    mb->show();

    // Espera 3 segundos y luego simula un clic en "OK"

    QTimer::singleShot(3000, mb->button(QMessageBox::Ok), &QAbstractButton::click);

    mb->exec();
    delete mb;
}

void MainWindow::DMidiInControl() {

    if(!MidiIn_control) {
        MidiIn_control = new MidiInControl((QWidget*) this);
        MidiInControl::MidiIn_toexit(MidiIn_control);
    }

    if(!MidiIn_control) return;

    int x = (this->width() - MidiIn_control->width())/2;
    if(x < 0) x = 0;

    MidiIn_control->move(x, 0);

    MidiIn_control->MIDI_INPUT->clear();

    foreach (QString name, MidiInput::inputPorts(MidiInControl::cur_pairdev)) {
        if(name != MidiInput::inputPort(MidiInControl::cur_pairdev)) continue;
        MidiIn_control->MIDI_INPUT->addItem(name);
        break;
    }
/*
    if(fl)
        MidiIn_control->tabMIDIin1->setDisabled(true);
    else*/
        MidiIn_control->tabMIDIin1->setDisabled(false);

    MidiIn_control->VST_reset();
    MidiIn_control->update_win();

    MidiIn_control->setModal(false);
    MidiIn_control->show();
    MidiIn_control->raise();
    MidiIn_control->activateWindow();
    MyVirtualKeyboard::overlap();
    QCoreApplication::processEvents();
}

