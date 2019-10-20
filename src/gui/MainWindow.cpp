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

#include <QAction>
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
#include <QSplitter>
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
#include "../midi/Metronome.h"
#include "../midi/MidiChannel.h"
#include "../midi/MidiFile.h"
#include "../midi/MidiInput.h"
#include "../midi/MidiOutput.h"
#include "../midi/MidiPlayer.h"
#include "../midi/MidiTrack.h"
#include "../midi/PlayerThread.h"

#include "../UpdateManager.h"
#include "CompleteMidiSetupDialog.h"
#include "UpdateDialog.h"
#include "AutomaticUpdateDialog.h"
#include <QtCore/qmath.h>

MainWindow::MainWindow(QString initFile)
    : QMainWindow()
    , _initFile(initFile)
{
    file = 0;
    _settings = new QSettings(QString("MidiEditor"), QString("NONE"));

    _moveSelectedEventsToChannelMenu = 0;
    _moveSelectedEventsToTrackMenu = 0;

    Appearance::init(_settings);

#ifdef ENABLE_REMOTE
    bool ok;
    int port = _settings->value("udp_client_port", -1).toInt(&ok);
    QString ip = _settings->value("udp_client_ip", "").toString();
#endif
    bool alternativeStop = _settings->value("alt_stop", false).toBool();
    MidiOutput::isAlternativePlayer = alternativeStop;
    bool ticksOK;
    int ticksPerQuarter = _settings->value("ticks_per_quarter", 192).toInt(&ticksOK);
    MidiFile::defaultTimePerQuarter = ticksPerQuarter;
    bool magnet = _settings->value("magnet", false).toBool();
    EventTool::enableMagnet(magnet);

    MidiInput::setThruEnabled(_settings->value("thru", false).toBool());
    Metronome::setEnabled(_settings->value("metronome", false).toBool());
    bool loudnessOk;
    Metronome::setLoudness(_settings->value("metronome_loudness", 100).toInt(&loudnessOk));

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

    UpdateManager::setAutoCheckUpdatesEnabled(_settings->value("auto_update_after_prompt", false).toBool());
    connect(UpdateManager::instance(), SIGNAL(updateDetected(Update*)), this, SLOT(updateDetected(Update*)));
    _quantizationGrid = _settings->value("quantization", 3).toInt();

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

    startDirectory = QDir::homePath();

    if (_settings->value("open_path").toString() != "") {
        startDirectory = _settings->value("open_path").toString();
    } else {
        _settings->setValue("open_path", startDirectory);
    }

    // read recent paths
    _recentFilePaths = _settings->value("recent_file_list").toStringList();

    EditorTool::setMainWindow(this);

    setWindowTitle(QApplication::applicationName() + " " + QApplication::applicationVersion());
    setWindowIcon(QIcon(":/run_environment/graphics/icon.png"));

    QWidget* central = new QWidget(this);
    QGridLayout* centralLayout = new QGridLayout(central);
    centralLayout->setContentsMargins(3, 3, 3, 5);

    // there is a vertical split
    QSplitter* mainSplitter = new QSplitter(Qt::Horizontal, central);
    //mainSplitter->setHandleWidth(0);

    // The left side
    QSplitter* leftSplitter = new QSplitter(Qt::Vertical, mainSplitter);
    leftSplitter->setHandleWidth(0);
    mainSplitter->addWidget(leftSplitter);
    leftSplitter->setContentsMargins(0, 0, 0, 0);

    // The right side
    QSplitter* rightSplitter = new QSplitter(Qt::Vertical, mainSplitter);
    //rightSplitter->setHandleWidth(0);
    mainSplitter->addWidget(rightSplitter);

    // Set the sizes of mainSplitter
    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 0);
    mainSplitter->setContentsMargins(0, 0, 0, 0);

    // the channelWidget and the trackWidget are tabbed
    QTabWidget* upperTabWidget = new QTabWidget(rightSplitter);
    rightSplitter->addWidget(upperTabWidget);
    rightSplitter->setContentsMargins(0, 0, 0, 0);

    // protocolList and EventWidget are tabbed
    lowerTabWidget = new QTabWidget(rightSplitter);
    rightSplitter->addWidget(lowerTabWidget);

    // MatrixArea
    QWidget* matrixArea = new QWidget(leftSplitter);
    leftSplitter->addWidget(matrixArea);
    matrixArea->setContentsMargins(0, 0, 0, 0);
    mw_matrixWidget = new MatrixWidget(matrixArea);
    vert = new QScrollBar(Qt::Vertical, matrixArea);
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

    bool screenLocked = _settings->value("screen_locked", false).toBool();
    mw_matrixWidget->setScreenLocked(screenLocked);
    int div = _settings->value("div", 2).toInt();
    mw_matrixWidget->setDiv(div);

    // VelocityArea
    QWidget* velocityArea = new QWidget(leftSplitter);
    velocityArea->setContentsMargins(0, 0, 0, 0);
    leftSplitter->addWidget(velocityArea);
    hori = new QScrollBar(Qt::Horizontal, velocityArea);
    hori->setSingleStep(500);
    hori->setPageStep(5000);
    QGridLayout* velocityAreaLayout = new QGridLayout(velocityArea);
    velocityAreaLayout->setContentsMargins(0, 0, 0, 0);
    velocityAreaLayout->setHorizontalSpacing(6);
    _miscWidgetControl = new QWidget(velocityArea);
    _miscWidgetControl->setFixedWidth(110 - velocityAreaLayout->horizontalSpacing());

    velocityAreaLayout->addWidget(_miscWidgetControl, 0, 0, 1, 1);
    // there is a Scrollbar on the right side of the velocityWidget doing
    // nothing but making the VelocityWidget as big as the matrixWidget
    QScrollBar* scrollNothing = new QScrollBar(Qt::Vertical, velocityArea);
    scrollNothing->setMinimum(0);
    scrollNothing->setMaximum(0);
    velocityAreaLayout->addWidget(scrollNothing, 0, 2, 1, 1);
    velocityAreaLayout->addWidget(hori, 1, 1, 1, 1);
    velocityAreaLayout->setRowStretch(0, 1);
    velocityArea->setLayout(velocityAreaLayout);

    _miscWidget = new MiscWidget(mw_matrixWidget, velocityArea);
    _miscWidget->setContentsMargins(0, 0, 0, 0);
    velocityAreaLayout->addWidget(_miscWidget, 0, 1, 1, 1);

    // controls for velocity widget
    _miscControlLayout = new QGridLayout(_miscWidgetControl);
    _miscControlLayout->setHorizontalSpacing(0);
    //_miscWidgetControl->setContentsMargins(0,0,0,0);
    //_miscControlLayout->setContentsMargins(0,0,0,0);
    _miscWidgetControl->setLayout(_miscControlLayout);
    _miscMode = new QComboBox(_miscWidgetControl);
    for (int i = 0; i < MiscModeEnd; i++) {
        _miscMode->addItem(MiscWidget::modeToString(i));
    }
    _miscMode->view()->setMinimumWidth(_miscMode->minimumSizeHint().width());
    //_miscControlLayout->addWidget(new QLabel("Mode:", _miscWidgetControl), 0, 0, 1, 3);
    _miscControlLayout->addWidget(_miscMode, 1, 0, 1, 3);
    connect(_miscMode, SIGNAL(currentIndexChanged(int)), this, SLOT(changeMiscMode(int)));

    //_miscControlLayout->addWidget(new QLabel("Control:", _miscWidgetControl), 2, 0, 1, 3);
    _miscController = new QComboBox(_miscWidgetControl);
    for (int i = 0; i < 128; i++) {
        _miscController->addItem(MidiFile::controlChangeName(i));
    }
    _miscController->view()->setMinimumWidth(_miscController->minimumSizeHint().width());
    _miscControlLayout->addWidget(_miscController, 3, 0, 1, 3);
    connect(_miscController, SIGNAL(currentIndexChanged(int)), _miscWidget, SLOT(setControl(int)));

    //_miscControlLayout->addWidget(new QLabel("Channel:", _miscWidgetControl), 4, 0, 1, 3);
    _miscChannel = new QComboBox(_miscWidgetControl);
    for (int i = 0; i < 15; i++) {
        _miscChannel->addItem("Channel " + QString::number(i));
    }
    _miscChannel->view()->setMinimumWidth(_miscChannel->minimumSizeHint().width());
    _miscControlLayout->addWidget(_miscChannel, 5, 0, 1, 3);
    connect(_miscChannel, SIGNAL(currentIndexChanged(int)), _miscWidget, SLOT(setChannel(int)));
    _miscControlLayout->setRowStretch(6, 1);
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

    QToolButton* btnSingle = new QToolButton(_miscWidgetControl);
    btnSingle->setDefaultAction(setSingleMode);
    QToolButton* btnHand = new QToolButton(_miscWidgetControl);
    btnHand->setDefaultAction(setFreehandMode);
    QToolButton* btnLine = new QToolButton(_miscWidgetControl);
    btnLine->setDefaultAction(setLineMode);

    _miscControlLayout->addWidget(btnSingle, 9, 0, 1, 1);
    _miscControlLayout->addWidget(btnHand, 9, 1, 1, 1);
    _miscControlLayout->addWidget(btnLine, 9, 2, 1, 1);

    // Set the sizes of leftSplitter
    leftSplitter->setStretchFactor(0, 8);
    leftSplitter->setStretchFactor(1, 1);

    // Track
    QWidget* tracks = new QWidget(upperTabWidget);
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
    connect(channelWidget, SIGNAL(channelStateChanged()), this, SLOT(updateChannelMenu()), Qt::QueuedConnection);
    connect(channelWidget, SIGNAL(selectInstrumentClicked(int)), this, SLOT(setInstrumentForChannel(int)), Qt::QueuedConnection);
    channelsLayout->addWidget(channelWidget, 1, 0, 1, 1);
    upperTabWidget->addTab(channels, "Channels");

    // terminal
    Terminal::initTerminal(_settings->value("start_cmd", "").toString(),
        _settings->value("in_port", "").toString(),
        _settings->value("out_port", "").toString());
    //upperTabWidget->addTab(Terminal::terminal()->console(), "Terminal");

    // Protocollist
    protocolWidget = new ProtocolWidget(lowerTabWidget);
    lowerTabWidget->addTab(protocolWidget, "Protocol");

    // EventWidget
    _eventWidget = new EventWidget(lowerTabWidget);
    Selection::_eventWidget = _eventWidget;
    lowerTabWidget->addTab(_eventWidget, "Event");
    MidiEvent::setEventWidget(_eventWidget);

    // below add two rows for choosing track/channel new events shall be assigned to
    QWidget* chooser = new QWidget(rightSplitter);
    chooser->setMinimumWidth(350);
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
    connect(mw_matrixWidget, SIGNAL(sizeChanged(int, int, int, int)), this,
        SLOT(matrixSizeChanged(int, int, int, int)));

    connect(mw_matrixWidget, SIGNAL(scrollChanged(int, int, int, int)), this,
        SLOT(scrollPositionsChanged(int, int, int, int)));

    setCentralWidget(central);

    QWidget* buttons = setupActions(central);

    rightSplitter->setStretchFactor(0, 5);
    rightSplitter->setStretchFactor(1, 5);

    // Add the Widgets to the central Layout
    centralLayout->setSpacing(0);
    centralLayout->addWidget(buttons, 0, 0);
    centralLayout->addWidget(mainSplitter, 1, 0);
    centralLayout->setRowStretch(1, 1);
    central->setLayout(centralLayout);

    if (_settings->value("colors_from_channel", false).toBool()) {
        colorsByChannel();
    } else {
        colorsByTrack();
    }
    copiedEventsChanged();
    setAcceptDrops(true);

    currentTweakTarget = new TimeTweakTarget(this);
    selectionNavigator = new SelectionNavigator(this);

    QTimer::singleShot(200, this, SLOT(loadInitFile()));
    if (UpdateManager::autoCheckForUpdates()) {
        QTimer::singleShot(500, UpdateManager::instance(), SLOT(checkForUpdates()));
    }

    // display dialogs, depending on number of starts of the software
    bool ok;
    int numStart = _settings->value("numStart_v3.5", -1).toInt(&ok);
    if (numStart == 15) {
        QTimer::singleShot(300, this, SLOT(donate()));
    }
    if (numStart == 10 && !UpdateManager::autoCheckForUpdates()) {
        QTimer::singleShot(300, this, SLOT(promtUpdatesDeactivatedDialog()));
    }
}

void MainWindow::loadInitFile()
{
    if (_initFile != "")
        loadFile(_initFile);
    else
        newFile();
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

    EventTool::clearSelection();
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
    mw_matrixWidget->setFile(file);
    updateChannelMenu();
    updateTrackMenu();
    mw_matrixWidget->update();
    _miscWidget->update();
    checkEnableActionsForSelection();
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
    if (MidiPlayer::isPlaying()) {
        stop();
    } else {
        play();
    }
}

void MainWindow::play()
{
    if (!MidiOutput::isConnected()) {
        CompleteMidiSetupDialog* d = new CompleteMidiSetupDialog(this, false, true);
        d->setModal(true);
        d->exec();
        return;
    }
    if (file && !MidiInput::recording() && !MidiPlayer::isPlaying()) {
        mw_matrixWidget->timeMsChanged(file->msOfTick(file->cursorTick()), true);

        _miscWidget->setEnabled(false);
        channelWidget->setEnabled(false);
        protocolWidget->setEnabled(false);
        mw_matrixWidget->setEnabled(false);
        _trackWidget->setEnabled(false);
        eventWidget()->setEnabled(false);

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

    if (!MidiOutput::isConnected() || !MidiInput::isConnected()) {
        CompleteMidiSetupDialog* d = new CompleteMidiSetupDialog(this, !MidiInput::isConnected(), !MidiOutput::isConnected());
        d->setModal(true);
        d->exec();
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

            _miscWidget->setEnabled(false);
            channelWidget->setEnabled(false);
            protocolWidget->setEnabled(false);
            mw_matrixWidget->setEnabled(false);
            _trackWidget->setEnabled(false);
            eventWidget()->setEnabled(false);
#ifdef ENABLE_REMOTE
            _remoteServer->record();
#endif
            MidiPlayer::play(file);
            MidiInput::startInput();
            connect(MidiPlayer::playerThread(),
                SIGNAL(playerStopped()), this, SLOT(stop()));
#ifdef __WINDOWS_MM__
            connect(MidiPlayer::playerThread(),
                SIGNAL(timeMsChanged(int)), mw_matrixWidget, SLOT(timeMsChanged(int)));
#endif
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
}

void MainWindow::stop(bool autoConfirmRecord, bool addEvents, bool resetPause)
{

    if (!file) {
        return;
    }

    disconnect(MidiPlayer::playerThread(),
        SIGNAL(playerStopped()), this, SLOT(stop()));

    if (resetPause) {
        file->setPauseTick(-1);
        mw_matrixWidget->update();
    }
    if (!MidiInput::recording() && MidiPlayer::isPlaying()) {
        MidiPlayer::stop();
        _miscWidget->setEnabled(true);
        channelWidget->setEnabled(true);
        _trackWidget->setEnabled(true);
        protocolWidget->setEnabled(true);
        mw_matrixWidget->setEnabled(true);
        eventWidget()->setEnabled(true);
        mw_matrixWidget->timeMsChanged(MidiPlayer::timeMs(), true);
        _trackWidget->setEnabled(true);
#ifdef ENABLE_REMOTE
        _remoteServer->stop();
#endif
        panic();
    }

    MidiTrack* track = file->track(NewNoteTool::editTrack());
    if (!track) {
        return;
    }

    if (MidiInput::recording()) {
        MidiPlayer::stop();
        panic();
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
            QMessageBox::information(this, "Channels/Tracks mute",
                "One or more channels/tracks are not audible. They will be audible in the saved file.",
                "Save file", 0, 0);
        }

        if (!file->save(file->path())) {
            QMessageBox::warning(this, "Error", QString("The file could not be saved. Please make sure that the destination directory exists and that you have the correct access rights to write into this directory."));
        } else {
            setWindowModified(false);
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
        dir);

    if (newPath == "") {
        return;
    }

    // automatically add '.mid' extension
    if (!newPath.endsWith(".mid", Qt::CaseInsensitive) && !newPath.endsWith(".midi", Qt::CaseInsensitive)) {
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
            QMessageBox::information(this, "Channels/Tracks mute",
                "One or more channels/tracks are not audible. They will be audible in the saved file.",
                "Save file", 0, 0);
        }

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
        if (!file->saved()) {
            switch (QMessageBox::question(this, "Save file?", "Save file " + file->path() + " before closing?", "Save", "Close without saving", "Cancel", 0, 2)) {
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

    QFile* f = new QFile(oldPath);
    QString dir = startDirectory;
    if (f->exists()) {
        QFileInfo(*f).dir().path();
    }
    QString newPath = QFileDialog::getOpenFileName(this, "Open file",
        dir, "MIDI Files(*.mid *.midi);;All Files(*)");

    if (!newPath.isEmpty()) {
        openFile(newPath);
    }
}

void MainWindow::loadFile(QString nfile)
{
    QString oldPath = startDirectory;
    if (file) {
        oldPath = file->path();
        if (!file->saved()) {
            switch (QMessageBox::question(this, "Save file?", "Save file " + file->path() + " before closing?", "Save", "Close without saving", "Cancel", 0, 2)) {
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
    if (!nfile.isEmpty()) {
        openFile(nfile);
    }
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

    MidiFile* mf = new MidiFile(filePath, &ok);

    if (ok) {
        stop();
        setFile(mf);
        updateRecentPathsList();
    } else {
        QMessageBox::warning(this, "Error", QString("The file is damaged and cannot be opened. "));
    }
}

void MainWindow::redo()
{
    if (file)
        file->protocol()->redo(true);
    updateTrackMenu();
}

void MainWindow::undo()
{
    if (file)
        file->protocol()->undo(true);
    updateTrackMenu();
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
    for (int i = 0; i < 19; i++) {
        file->channel(i)->setMute(true);
    }
    file->protocol()->endAction();
    channelWidget->update();
}

void MainWindow::unmuteAllChannels()
{
    if (!file)
        return;
    file->protocol()->startNewAction("All channels audible");
    for (int i = 0; i < 19; i++) {
        file->channel(i)->setMute(false);
    }
    file->protocol()->endAction();
    channelWidget->update();
}

void MainWindow::allChannelsVisible()
{
    if (!file)
        return;
    file->protocol()->startNewAction("All channels visible");
    for (int i = 0; i < 19; i++) {
        file->channel(i)->setVisible(true);
    }
    file->protocol()->endAction();
    channelWidget->update();
}

void MainWindow::allChannelsInvisible()
{
    if (!file)
        return;
    file->protocol()->startNewAction("Hide all channels");
    for (int i = 0; i < 19; i++) {
        file->channel(i)->setVisible(false);
    }
    file->protocol()->endAction();
    channelWidget->update();
}

void MainWindow::closeEvent(QCloseEvent* event)
{

    if (!file || file->saved()) {
        event->accept();
    } else {
        switch (QMessageBox::question(this, "Save file?", "Save file " + file->path() + " before closing?", "Save", "Close without saving", "Cancel", 0, 2)) {
        case 0: {
            // save
            if (QFile(file->path()).exists()) {
                file->save(file->path());
                event->accept();
            } else {
                saveas();
                event->accept();
            }
            break;
        }
        case 1: {
            // close
            event->accept();
            break;
        }
        case 2: {
            // break
            event->ignore();
            return;
        }
        }
    }

    if (MidiOutput::outputPort() != "") {
        _settings->setValue("out_port", MidiOutput::outputPort());
    }
    if (MidiInput::inputPort() != "") {
        _settings->setValue("in_port", MidiInput::inputPort());
    }
#ifdef ENABLE_REMOTE
    if (_remoteServer->clientIp() != "") {
        _settings->setValue("udp_client_ip", _remoteServer->clientIp());
    }
    if (_remoteServer->clientPort() > 0) {
        _settings->setValue("udp_client_port", _remoteServer->clientPort());
    }
    _remoteServer->stopServer();
#endif

    bool ok;
    int numStart = _settings->value("numStart_v3.5", -1).toInt(&ok);
    _settings->setValue("numStart_v3.5", numStart + 1);

    // save the current Path
    _settings->setValue("open_path", startDirectory);
    _settings->setValue("alt_stop", MidiOutput::isAlternativePlayer);
    _settings->setValue("ticks_per_quarter", MidiFile::defaultTimePerQuarter);
    _settings->setValue("screen_locked", mw_matrixWidget->screenLocked());
    _settings->setValue("magnet", EventTool::magnetEnabled());

    _settings->setValue("div", mw_matrixWidget->div());
    _settings->setValue("colors_from_channel", mw_matrixWidget->colorsByChannel());

    _settings->setValue("metronome", Metronome::enabled());
    _settings->setValue("metronome_loudness", Metronome::loudness());
    _settings->setValue("thru", MidiInput::thru());
    _settings->setValue("quantization", _quantizationGrid);

    _settings->setValue("auto_update_after_prompt", UpdateManager::autoCheckForUpdates());
    _settings->setValue("has_prompted_for_updates", true); // Happens on first start

    Appearance::writeSettings(_settings);
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
        if (!file->saved()) {
            switch (QMessageBox::question(this, "Save file?", "Save file " + file->path() + " before closing?", "Save", "Close without saving", "Cancel", 0, 2)) {
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

    // create new File
    MidiFile* f = new MidiFile();

    setFile(f);

    editTrack(1);
    setWindowTitle(QApplication::applicationName() + " - Untitled Document[*]");
}

void MainWindow::panic()
{
    MidiPlayer::panic();
}

void MainWindow::screenLockPressed(bool enable)
{
    mw_matrixWidget->setScreenLocked(enable);
}

void MainWindow::scaleSelection()
{
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
                MidiEvent* off = on->offEvent();
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
                MidiEvent* off = on->offEvent();
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

        file->protocol()->startNewAction("Remove event(s)");
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
        if (Selection::instance()->selectedEvents().contains(event)) {
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

    int num = action->data().toInt();
    MidiChannel* channel = file->channel(num);

    if (Selection::instance()->selectedEvents().size() > 0) {
        file->protocol()->startNewAction("Move selected events to channel " + QString::number(num));
        foreach (MidiEvent* ev, Selection::instance()->selectedEvents()) {
            file->channel(ev->channel())->removeEvent(ev);
            ev->setChannel(num, true);
            OnEvent* onevent = dynamic_cast<OnEvent*>(ev);
            if (onevent) {
                channel->insertEvent(onevent->offEvent(), onevent->offEvent()->midiTime());
                onevent->offEvent()->setChannel(num);
            }
            channel->insertEvent(ev, ev->midiTime());
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
            ev->setTrack(track, true);
            OnEvent* onevent = dynamic_cast<OnEvent*>(ev);
            if (onevent) {
                onevent->offEvent()->setTrack(track);
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
        newList.append(currentPath);

        foreach (QString str, _recentFilePaths) {
            if (str != currentPath && newList.size() < 10) {
                newList.append(str);
            }
        }

        _recentFilePaths = newList;
    }

    // save list
    QVariant list(_recentFilePaths);
    _settings->setValue("recent_file_list", list);

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

        if (!file->saved()) {
            switch (QMessageBox::question(this, "Save file?", "Save file " + file->path() + " before closing?", "Save", "Close without saving", "Cancel", 0, 2)) {
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

    // delete channel events menu
    foreach (QAction* action, _deleteChannelMenu->actions()) {
        int channel = action->data().toInt();
        if (file) {
            action->setText(QString::number(channel) + " " + MidiFile::instrumentName(file->channel(channel)->progAtTick(0)));
        }
    }

    // move events to channel...
    foreach (QAction* action, _moveSelectedEventsToChannelMenu->actions()) {
        int channel = action->data().toInt();
        if (file) {
            action->setText(QString::number(channel) + " " + MidiFile::instrumentName(file->channel(channel)->progAtTick(0)));
        }
    }

    // paste events to channel...
    foreach (QAction* action, _pasteToChannelMenu->actions()) {
        int channel = action->data().toInt();
        if (file && channel >= 0) {
            action->setText(QString::number(channel) + " " + MidiFile::instrumentName(file->channel(channel)->progAtTick(0)));
        }
    }

    // select all events from channel...
    foreach (QAction* action, _selectAllFromChannelMenu->actions()) {
        int channel = action->data().toInt();
        if (file) {
            action->setText(QString::number(channel) + " " + MidiFile::instrumentName(file->channel(channel)->progAtTick(0)));
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

    for (int i = 0; i < file->numTracks(); i++) {
        QVariant variant(i);
        QAction* moveToTrackAction = new QAction(QString::number(i) + " " + file->tracks()->at(i)->name(), this);
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
    if (NewNoteTool::editTrack() >= file->numTracks()) {
        NewNoteTool::setEditTrack(0);
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
}

void MainWindow::muteChannel(QAction* action)
{
    int channel = action->data().toInt();
    if (file) {
        file->protocol()->startNewAction("Mute channel");
        file->channel(channel)->setMute(action->isChecked());
        updateChannelMenu();
        channelWidget->update();
        file->protocol()->endAction();
    }
}
void MainWindow::soloChannel(QAction* action)
{
    int channel = action->data().toInt();
    if (file) {
        file->protocol()->startNewAction("Select solo channel");
        for (int i = 0; i < 16; i++) {
            file->channel(i)->setSolo(i == channel && action->isChecked());
        }
        file->protocol()->endAction();
    }
    channelWidget->update();
    updateChannelMenu();
}

void MainWindow::viewChannel(QAction* action)
{
    int channel = action->data().toInt();
    if (file) {
        file->protocol()->startNewAction("Channel visibility changed");
        file->channel(channel)->setVisible(action->isChecked());
        updateChannelMenu();
        channelWidget->update();
        file->protocol()->endAction();
    }
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
    updateTrackMenu();
}

void MainWindow::addTrack()
{

    if (file) {

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
    file->channel(channel)->setVisible(true);
    foreach (MidiEvent* e, file->channel(channel)->eventMap()->values()) {
        if (e->track()->hidden()) {
            e->track()->setHidden(false);
        }
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

void MainWindow::editChannel(int i, bool assign)
{
    NewNoteTool::setEditChannel(i);

    // assign channel to track
    if (assign && file && file->track(NewNoteTool::editTrack())) {
        file->track(NewNoteTool::editTrack())->assignChannel(i);
    }

    MidiOutput::setStandardChannel(i);

    int prog = file->channel(i)->progAtTick(file->cursorTick());
    MidiOutput::sendProgram(i, prog);

    updateChannelMenu();
}

void MainWindow::editTrack(int i, bool assign)
{
    NewNoteTool::setEditTrack(i);

    // assign channel to track
    if (assign && file && file->track(i)) {
        file->track(i)->assignChannel(NewNoteTool::editChannel());
    }
    updateTrackMenu();
}

void MainWindow::editTrackAndChannel(MidiTrack* track)
{
    editTrack(track->number(), false);
    if (track->assignedChannel() > -1) {
        editChannel(track->assignedChannel(), false);
    }
}

void MainWindow::setInstrumentForChannel(int i)
{
    InstrumentChooser* d = new InstrumentChooser(file, i, this);
    d->setModal(true);
    d->exec();

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
    QMenu* viewMB = menuBar()->addMenu("View");
    QMenu* playbackMB = menuBar()->addMenu("Playback");
    QMenu* midiMB = menuBar()->addMenu("Midi");
    QMenu* helpMB = menuBar()->addMenu("Help");

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
    editMB->addMenu(pasteOptionsMenu);

    editMB->addSeparator();

    QAction* configAction = new QAction("Settings...", this);
    configAction->setIcon(QIcon(":/run_environment/graphics/tool/config.png"));
    connect(configAction, SIGNAL(triggered()), this, SLOT(openConfig()));
    editMB->addAction(configAction);

    // Tools
    QMenu* toolsToolsMenu = new QMenu("Current tool...", toolsMB);

    StandardTool* tool = new StandardTool();
    Tool::setCurrentTool(tool);
    stdToolAction = new ToolButton(tool, QKeySequence(Qt::Key_F1), toolsToolsMenu);
    toolsToolsMenu->addAction(stdToolAction);
    tool->buttonClick();

    QAction* newNoteAction = new ToolButton(new NewNoteTool(), QKeySequence(Qt::Key_F2), toolsToolsMenu);
    toolsToolsMenu->addAction(newNoteAction);
    QAction* removeNotesAction = new ToolButton(new EraserTool(), QKeySequence(Qt::Key_F3), toolsToolsMenu);
    toolsToolsMenu->addAction(removeNotesAction);

    toolsToolsMenu->addSeparator();

    QAction* selectSingleAction = new ToolButton(new SelectTool(SELECTION_TYPE_SINGLE), QKeySequence(Qt::Key_F4), toolsToolsMenu);
    toolsToolsMenu->addAction(selectSingleAction);
    QAction* selectBoxAction = new ToolButton(new SelectTool(SELECTION_TYPE_BOX), QKeySequence(Qt::Key_F5), toolsToolsMenu);
    toolsToolsMenu->addAction(selectBoxAction);
    QAction* selectLeftAction = new ToolButton(new SelectTool(SELECTION_TYPE_LEFT), QKeySequence(Qt::Key_F6), toolsToolsMenu);
    toolsToolsMenu->addAction(selectLeftAction);
    QAction* selectRightAction = new ToolButton(new SelectTool(SELECTION_TYPE_RIGHT), QKeySequence(Qt::Key_F7), toolsToolsMenu);
    toolsToolsMenu->addAction(selectRightAction);

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
    tweakMediumDecreaseAction->setShortcut(Qt::Key_9 + Qt::ALT);
    connect(tweakMediumDecreaseAction, SIGNAL(triggered()), this, SLOT(tweakMediumDecrease()));
    tweakMenu->addAction(tweakMediumDecreaseAction);

    QAction* tweakMediumIncreaseAction = new QAction("Medium increase", tweakMenu);
    tweakMediumIncreaseAction->setShortcut(Qt::Key_0 + Qt::ALT);
    connect(tweakMediumIncreaseAction, SIGNAL(triggered()), this, SLOT(tweakMediumIncrease()));
    tweakMenu->addAction(tweakMediumIncreaseAction);

    QAction* tweakLargeDecreaseAction = new QAction("Large decrease", tweakMenu);
    tweakLargeDecreaseAction->setShortcut(Qt::Key_9 + Qt::ALT + Qt::SHIFT);
    connect(tweakLargeDecreaseAction, SIGNAL(triggered()), this, SLOT(tweakLargeDecrease()));
    tweakMenu->addAction(tweakLargeDecreaseAction);

    QAction* tweakLargeIncreaseAction = new QAction("Large increase", tweakMenu);
    tweakLargeIncreaseAction->setShortcut(Qt::Key_0 + Qt::ALT + Qt::SHIFT);
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

    toolsMB->addSeparator();

    QAction* alignLeftAction = new QAction("Align left", this);
    _activateWithSelections.append(alignLeftAction);
    alignLeftAction->setShortcut(QKeySequence(Qt::Key_Left + Qt::CTRL));
    alignLeftAction->setIcon(QIcon(":/run_environment/graphics/tool/align_left.png"));
    connect(alignLeftAction, SIGNAL(triggered()), this, SLOT(alignLeft()));
    toolsMB->addAction(alignLeftAction);

    QAction* alignRightAction = new QAction("Align right", this);
    _activateWithSelections.append(alignRightAction);
    alignRightAction->setIcon(QIcon(":/run_environment/graphics/tool/align_right.png"));
    alignRightAction->setShortcut(QKeySequence(Qt::Key_Right + Qt::CTRL));
    connect(alignRightAction, SIGNAL(triggered()), this, SLOT(alignRight()));
    toolsMB->addAction(alignRightAction);

    QAction* equalizeAction = new QAction("Equalize selection", this);
    _activateWithSelections.append(equalizeAction);
    equalizeAction->setIcon(QIcon(":/run_environment/graphics/tool/equalize.png"));
    equalizeAction->setShortcut(QKeySequence(Qt::Key_Up + Qt::CTRL));
    connect(equalizeAction, SIGNAL(triggered()), this, SLOT(equalize()));
    toolsMB->addAction(equalizeAction);

    toolsMB->addSeparator();

    QAction* quantizeAction = new QAction("Quantify selection", this);
    _activateWithSelections.append(quantizeAction);
    quantizeAction->setIcon(QIcon(":/run_environment/graphics/tool/quantize.png"));
    quantizeAction->setShortcut(QKeySequence(Qt::Key_G + Qt::CTRL));
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
    quantizeNToleAction->setShortcut(QKeySequence(Qt::Key_H + Qt::CTRL + Qt::SHIFT));
    connect(quantizeNToleAction, SIGNAL(triggered()), this, SLOT(quantizeNtoleDialog()));
    toolsMB->addAction(quantizeNToleAction);

    QAction* quantizeNToleActionRepeat = new QAction("Repeat tuplet quantization", this);
    _activateWithSelections.append(quantizeNToleActionRepeat);
    quantizeNToleActionRepeat->setShortcut(QKeySequence(Qt::Key_H + Qt::CTRL));
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

    _moveSelectedEventsToTrackMenu = new QMenu("Move events to track...", editMB);
    toolsMB->addMenu(_moveSelectedEventsToTrackMenu);
    connect(_moveSelectedEventsToTrackMenu, SIGNAL(triggered(QAction*)), this, SLOT(moveSelectedEventsToTrack(QAction*)));

    toolsMB->addSeparator();

    QAction* transposeAction = new QAction("Transpose selection...", this);
    _activateWithSelections.append(transposeAction);
    transposeAction->setShortcut(QKeySequence(Qt::Key_T + Qt::CTRL));
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
    magnetAction->setShortcut(QKeySequence(Qt::Key_M + Qt::CTRL));
    magnetAction->setIcon(QIcon(":/run_environment/graphics/tool/magnet.png"));
    magnetAction->setCheckable(true);
    magnetAction->setChecked(false);
    magnetAction->setChecked(EventTool::magnetEnabled());
    connect(magnetAction, SIGNAL(toggled(bool)), this, SLOT(enableMagnet(bool)));

    // View
    QMenu* zoomMenu = new QMenu("Zoom...", viewMB);
    QAction* zoomHorOutAction = new QAction("Horizontal out", this);
    zoomHorOutAction->setShortcut(QKeySequence(Qt::Key_Minus + Qt::CTRL));
    zoomHorOutAction->setIcon(QIcon(":/run_environment/graphics/tool/zoom_hor_out.png"));
    connect(zoomHorOutAction, SIGNAL(triggered()),
        mw_matrixWidget, SLOT(zoomHorOut()));
    zoomMenu->addAction(zoomHorOutAction);

    QAction* zoomHorInAction = new QAction("Horizontal in", this);
    zoomHorInAction->setIcon(QIcon(":/run_environment/graphics/tool/zoom_hor_in.png"));
    zoomHorInAction->setShortcut(QKeySequence(Qt::Key_Plus + Qt::CTRL));
    connect(zoomHorInAction, SIGNAL(triggered()),
        mw_matrixWidget, SLOT(zoomHorIn()));
    zoomMenu->addAction(zoomHorInAction);

    QAction* zoomVerOutAction = new QAction("Vertical out", this);
    zoomVerOutAction->setIcon(QIcon(":/run_environment/graphics/tool/zoom_ver_out.png"));
    zoomVerOutAction->setShortcut(QKeySequence(Qt::Key_Minus + Qt::CTRL + Qt::ALT));
    connect(zoomVerOutAction, SIGNAL(triggered()),
        mw_matrixWidget, SLOT(zoomVerOut()));
    zoomMenu->addAction(zoomVerOutAction);

    QAction* zoomVerInAction = new QAction("Vertical in", this);
    zoomVerInAction->setIcon(QIcon(":/run_environment/graphics/tool/zoom_ver_in.png"));
    zoomVerInAction->setShortcut(QKeySequence(Qt::Key_Plus + Qt::CTRL + Qt::ALT));
    connect(zoomVerInAction, SIGNAL(triggered()),
        mw_matrixWidget, SLOT(zoomVerIn()));
    zoomMenu->addAction(zoomVerInAction);

    zoomMenu->addSeparator();

    QAction* zoomStdAction = new QAction("Restore default", this);
    zoomStdAction->setShortcut(QKeySequence(Qt::Key_0 + Qt::CTRL));
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

    for (int i = -1; i <= 5; i++) {
        QVariant variant(i);
        QString text = "Off";
        if (i == 0) {
            text = "Whole note";
        } else if (i == 1) {
            text = "Half note";
        } else if (i == 2) {
            text = "Quarter note";
        } else if (i > 0) {
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

    // Playback
    QAction* playStopAction = new QAction("PlayStop", this);
    QList<QKeySequence> playStopActionShortcuts;
    playStopActionShortcuts << QKeySequence(Qt::Key_Space)
                            << QKeySequence(Qt::Key_K)
                            << QKeySequence(Qt::Key_P + Qt::CTRL);
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
    pauseAction->setShortcut(QKeySequence(Qt::Key_Space + Qt::META));
#else
    pauseAction->setShortcut(QKeySequence(Qt::Key_Space + Qt::CTRL));
#endif
    connect(pauseAction, SIGNAL(triggered()), this, SLOT(pause()));
    playbackMB->addAction(pauseAction);

    QAction* recAction = new QAction("Record", this);
    recAction->setIcon(QIcon(":/run_environment/graphics/tool/record.png"));
    recAction->setShortcut(QKeySequence(Qt::Key_R + Qt::CTRL));
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
    backToBeginActionShortcuts << QKeySequence(Qt::Key_Up + Qt::ALT)
                               << QKeySequence(Qt::Key_Home + Qt::ALT)
                               << QKeySequence(Qt::Key_J + Qt::SHIFT);
    backToBeginAction->setShortcuts(backToBeginActionShortcuts);
    connect(backToBeginAction, SIGNAL(triggered()), this, SLOT(backToBegin()));
    playbackMB->addAction(backToBeginAction);

    QAction* backAction = new QAction("Previous measure", this);
    backAction->setIcon(QIcon(":/run_environment/graphics/tool/back.png"));
    QList<QKeySequence> backActionShortcuts;
    backActionShortcuts << QKeySequence(Qt::Key_Left + Qt::ALT)
                        << QKeySequence(Qt::Key_J);
    backAction->setShortcuts(backActionShortcuts);
    connect(backAction, SIGNAL(triggered()), this, SLOT(back()));
    playbackMB->addAction(backAction);

    QAction* forwAction = new QAction("Next measure", this);
    forwAction->setIcon(QIcon(":/run_environment/graphics/tool/forward.png"));
    QList<QKeySequence> forwActionShortcuts;
    forwActionShortcuts << QKeySequence(Qt::Key_Right + Qt::ALT)
                        << QKeySequence(Qt::Key_L);
    forwAction->setShortcuts(forwActionShortcuts);
    connect(forwAction, SIGNAL(triggered()), this, SLOT(forward()));
    playbackMB->addAction(forwAction);

    playbackMB->addSeparator();

    QAction* backMarkerAction = new QAction("Previous marker", this);
    backMarkerAction->setIcon(QIcon(":/run_environment/graphics/tool/back_marker.png"));
    QList<QKeySequence> backMarkerActionShortcuts;
    backMarkerAction->setShortcut(QKeySequence(Qt::Key_Comma + Qt::ALT));
    connect(backMarkerAction, SIGNAL(triggered()), this, SLOT(backMarker()));
    playbackMB->addAction(backMarkerAction);

    QAction* forwMarkerAction = new QAction("Next marker", this);
    forwMarkerAction->setIcon(QIcon(":/run_environment/graphics/tool/forward_marker.png"));
    QList<QKeySequence> forwMarkerActionShortcuts;
    forwMarkerAction->setShortcut(QKeySequence(Qt::Key_Period + Qt::ALT));
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

    QAction* donateAction = new QAction("Donate", this);
    connect(donateAction, SIGNAL(triggered()), this, SLOT(donate()));
    helpMB->addAction(donateAction);

    QWidget* buttonBar = new QWidget(parent);
    QGridLayout* btnLayout = new QGridLayout(buttonBar);
    buttonBar->setLayout(btnLayout);
    btnLayout->setSpacing(0);
    buttonBar->setContentsMargins(0, 0, 0, 0);
    QToolBar* fileTB = new QToolBar("File", buttonBar);

    fileTB->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    fileTB->setFloatable(false);
    fileTB->setContentsMargins(0, 0, 0, 0);
    fileTB->layout()->setSpacing(0);
    fileTB->setIconSize(QSize(35, 35));
    fileTB->addAction(newAction);
    fileTB->setStyleSheet("QToolBar { border: 0px }");
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
    lowerTB->setStyleSheet("QToolBar { border: 0px }");
    upperTB->setStyleSheet("QToolBar { border: 0px }");

    lowerTB->addAction(copyAction);

    pasteActionTB = new QAction("Paste events", this);
    pasteActionTB->setToolTip("Paste events at cursor position");
    pasteActionTB->setIcon(QIcon(":/run_environment/graphics/tool/paste.png"));
    connect(pasteActionTB, SIGNAL(triggered()), this, SLOT(paste()));
    pasteActionTB->setMenu(pasteOptionsMenu);

    lowerTB->addAction(pasteActionTB);

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
    lowerTB->addAction(thruAction);
    lowerTB->addAction(panicAction);

    upperTB->addAction(stdToolAction);
    upperTB->addAction(newNoteAction);
    upperTB->addAction(removeNotesAction);
    upperTB->addSeparator();
    upperTB->addAction(selectSingleAction);
    upperTB->addAction(selectBoxAction);
    upperTB->addAction(selectLeftAction);
    upperTB->addAction(selectRightAction);
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
    mw_matrixWidget->setDiv(action->data().toInt());
}

void MainWindow::enableMagnet(bool enable)
{
    EventTool::enableMagnet(enable);
}

void MainWindow::openConfig()
{
#ifdef ENABLE_REMOTE
    SettingsDialog* d = new SettingsDialog("Settings", _settings, _remoteServer, this);
#else
    SettingsDialog* d = new SettingsDialog("Settings", _settings, 0, this);
#endif
    connect(d, SIGNAL(settingsChanged()), this, SLOT(updateAll()));
    d->show();
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

void MainWindow::updateDetected(Update* update)
{
    UpdateDialog* d = new UpdateDialog(update, this);
    d->setModal(true);
    d->exec();
}

void MainWindow::promtUpdatesDeactivatedDialog() {
    AutomaticUpdateDialog* d = new AutomaticUpdateDialog(this);
    d->setModal(true);
    d->exec();
}

void MainWindow::updateAll() {
    mw_matrixWidget->registerRelayout();
    mw_matrixWidget->update();
    channelWidget->update();
    _trackWidget->update();
    _miscWidget->update();
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
