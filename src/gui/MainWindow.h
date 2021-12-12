/*
 * MidiEditor
 * Copyright (C) 2010  Markus Schwenk
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include <QCloseEvent>
#include <QMainWindow>
#include <QScrollBar>
#include <QSettings>
#include <QSplitter>
#include <QDateTime>
#include <QSemaphore>

class MatrixWidget;
class MidiEvent;
class MidiFile;
class ChannelListWidget;
class ProtocolWidget;
class EventWidget;
class ClickButton;
class QStringList;
class QTabWidget;
class QMenu;
class TrackListWidget;
class QComboBox;
class RemoteServer;
class MiscWidget;
class QGridLayout;
class MidiTrack;
class QShowEvent;
class Update;
class SelectionNavigator;
class TweakTarget;
class FluidWidget;

typedef struct {
    QByteArray name[128];
} instrument_list;

extern instrument_list InstrumentList[129];

class MainWindow : public QMainWindow {

    Q_OBJECT

public:
    MainWindow(QString initFile = "");
    ~MainWindow();

    void setFile(MidiFile* f);
    MidiFile* getFile();
    MatrixWidget* matrixWidget();
    EventWidget* eventWidget();
    void setStartDir(QString dir);
    void setInitFile(const char* file);
    void addInstrumentNames();
    void update_channel_list();

    void Notes_util(QWidget * _MW); // Notes_util.cpp

    QSettings* getSettings() {
        return _settings;
    }

    QTabWidget* upperTabWidget;
    QTabWidget* lowerTabWidget;

    bool rightSplitterMode;
    int EventSplitterTabPos;

    ChannelListWidget* channelWidget;

protected:
    void dropEvent(QDropEvent* ev) override;
    void dragEnterEvent(QDragEnterEvent* ev) override;

#ifdef USE_FLUIDSYNTH
signals:
    int signal_remote_VST();
    void ToggleViewVST(int channel, bool on);
#endif

public slots:
    void setChordVelocityProp();
    void velocity_accept();
    void updateAll();
    void loadInitFile();
    void matrixSizeChanged(int maxScrollTime, int maxScrollLine, int vX, int vY);
    void play();
    void playStop();
    void stop(bool autoConfirmRecord = false, bool addEvents = true, bool resetPause = true);
    void pause();
    void forward();
    void back();
    void backToBegin();
    void forwardMarker();
    void backMarker();
    void load();
    void loadFile(QString file);
    void openFile(QString filePath);
    void save();
    void saveas();
    void undo();
    void redo();
    void muteAllChannels();
    void unmuteAllChannels();
    void allChannelsVisible();
    void allChannelsInvisible();
    void muteAllTracks();
    void unmuteAllTracks();
    void allTracksVisible();
    void allTracksInvisible();
    void donate();
    void about();
    void setFileLengthMs();
    void scrollPositionsChanged(int startMs, int maxMs, int startLine,
        int maxLine);
    void record();
    void newFile();
    void panic();
    void screenLockPressed(bool enable);
    void scaleSelection();
    void alignLeft();
    void alignRight();
    void equalize();
    void deleteSelectedEvents();
    void deleteChannel(QAction* action);
    void moveSelectedEventsToChannel(QAction* action);
    void moveSelectedNotesToChannel(QAction* action);
    void moveSelectedEventsToTrack(QAction* action);
    void updateRecentPathsList();
    void openRecent(QAction* action);
    void updateChannelMenu();
    void updateTrackMenu();
    void muteChannel(QAction* action);
    void soloChannel(QAction* action);
    void viewChannel(QAction* action);
    void instrumentChannel(QAction* action);
    void SoundEffectChannel(QAction* action);

    void renameTrackMenuClicked(QAction* action);
    void removeTrackMenuClicked(QAction* action);
    void showEventWidget(bool show);
    void showTrackMenuClicked(QAction* action);
    void muteTrackMenuClicked(QAction* action);

    void renameTrack(int tracknumber);
    void removeTrack(int tracknumber);

    void velocityScale();
    void NotesCorrection(int mode);
    void overlappedNotesCorrection();
    void longNotesCorrection();

    void buildCMajorProg(int d3, int d5, int d7);
    void buildCMinorProg(int d3, int d5, int d7);

    void buildCMajor();
    void buildCMinor();
    void buildCMajorInv1();
    void buildCMinorInv1();
    void buildCMajorInv2();
    void buildCMinorInv2();

    void builddichord(int d3, int d5, int d7);

    void buildpowerchord();
    void buildpowerchordInv();
    void buildpowerpowerchord();
    void buildMajor();
    void buildMinor();
    void buildAug();
    void buildDis();
    void buildSeventh();
    void buildMajorSeventh();
    void buildMinorSeventh();
    void buildMinorSeventhMajor();

    void pitchbend_effect1();
    void volumeoff_effect();
    void choppy_audio_effect();
    void conv_pattern_note();

    void setInstrumentForChannel(int i);
    void setSoundEffectForChannel(int i);
#ifdef USE_FLUIDSYNTH
    void setLoadVSTForChannel(int channel, int flag);
    void remote_VST();
    void remote_VST_exit();
#endif

    void message_timeout(QString title, QString message);

    void spreadSelection();
    void copy();
    void paste();

    void addTrack();

    void selectAll();

    void transposeNSemitones();

    void markEdited();

    void colorsByChannel();
    void colorsByTrack();

    void editChannel(int i, bool assign = true);
    void editTrack(int i, bool assign = true);
    void editTrackAndChannel(MidiTrack* track);

    void manual();

    void changeMiscMode(int mode);
    void selectModeChanged(QAction* action);

    void pasteToChannel(QAction* action);
    void pasteToTrack(QAction* action);

    void selectAllFromChannel(QAction* action);
    void selectAllFromTrack(QAction* action);

    void divChanged(QAction* action);
    void quantizationChanged(QAction*);

    void enableMagnet(bool enable);

    void openConfig();

    void enableMetronome(bool enable);
    void enableThru(bool enable);
    void DMidiInControl();

    void midi_text_edit();
    void midi_marker_edit();
    void midi_lyrik_edit();
    void midi_track_name_edit();

 #ifdef USE_FLUIDSYNTH
    static void FluidControl();
    void FluidSaveAsWav();
    void FluidSaveAsMp3();
    void FluidSaveAsFlac();
 #endif

    void ImportSF2Names();

    void PianoPlay();
    void DrumPlay();
    void DrumRhythmBox();

    void quantizeSelection();
    void quantizeNtoleDialog();
    void quantizeNtole();

    void setSpeed(QAction*);

    void checkEnableActionsForSelection();
    void toolChanged();
    void copiedEventsChanged();

    void updateDetected(Update* update);
    void promtUpdatesDeactivatedDialog();

    void tweakTime();
    void tweakStartTime();
    void tweakEndTime();
    void tweakNote();
    void tweakValue();
    void tweakSmallDecrease();
    void tweakSmallIncrease();
    void tweakMediumDecrease();
    void tweakMediumIncrease();
    void tweakLargeDecrease();
    void tweakLargeIncrease();

    void navigateSelectionUp();
    void navigateSelectionDown();
    void navigateSelectionLeft();
    void navigateSelectionRight();

protected:
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;


private:
#ifdef USE_FLUIDSYNTH

    QAction* FluidActionExportWav;
    QAction* FluidActionExportMp3;
    QAction* FluidActionExportFlac;

#endif

    QSplitter* mainSplitter;
    QSplitter* rightSplitter;

    MatrixWidget* mw_matrixWidget;
    QScrollBar *vert, *hori;
    //ChannelListWidget* channelWidget;
    ProtocolWidget* protocolWidget;
    TrackListWidget* _trackWidget;
    MidiFile* file;
    QString startDirectory, _initFile;
    EventWidget* _eventWidget;
    QSettings* _settings;
    QStringList _recentFilePaths;
    QMenu *_recentPathsMenu, *_deleteChannelMenu,
        *_moveSelectedEventsToTrackMenu, *_moveSelectedEventsToChannelMenu, *_moveSelectedNotesToChannelMenu,
        *_pasteToTrackMenu, *_pasteToChannelMenu, *_selectAllFromTrackMenu, *_selectAllFromChannelMenu;

    QAction *_colorsByChannel, *_colorsByTracks;
    QComboBox *_chooseEditTrack, *_chooseEditChannel;

#ifdef ENABLE_REMOTE
    RemoteServer* _remoteServer;
#endif

    QWidget* _miscWidgetControl;
    QGridLayout* _miscControlLayout;

    QComboBox *_miscMode, *_miscController, *_miscChannel;
    QAction *setSingleMode, *setLineMode, *setFreehandMode, *_allChannelsVisible, *_allChannelsInvisible, *_allTracksAudible, *_allTracksMute,
        *_allChannelsAudible, *_allChannelsMute, *_allTracksVisible, *_allTracksInvisible, *stdToolAction, *undoAction, *redoAction, *_pasteAction, *pasteActionTB;
    MiscWidget* _miscWidget;

    QWidget* setupActions(QWidget* parent);

    int _quantizationGrid;
    int quantize(int t, QList<int> ticks);
    QList<QAction*> _activateWithSelections;

    TweakTarget* currentTweakTarget;
    SelectionNavigator* selectionNavigator;
};

#endif
