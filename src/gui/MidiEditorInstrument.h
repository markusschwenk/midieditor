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

#ifndef MIDIEDITORINSTRUMENT_H
#define MIDIEDITORINSTRUMENT_H

#include <QSettings>
#include <QDialog>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLineEdit>

#include "MatrixWidget.h"
#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/ProgChangeEvent.h"
#include "../MidiEvent/ControlChangeEvent.h"

#include "../MidiEvent/NoteOnEvent.h"
#include "../MidiEvent/OffEvent.h"
#include "../MidiEvent/TempoChangeEvent.h"
#include "../MidiEvent/TimeSignatureEvent.h"
#include "../midi/MidiChannel.h"
#include "../midi/MidiFile.h"
#include "../midi/MidiInput.h"
#include "../midi/MidiPlayer.h"
#include "../midi/MidiTrack.h"
#include "../midi/PlayerThread.h"
#include "../midi/MidiOutput.h"
#include "../protocol/Protocol.h"
#include "../tool/NewNoteTool.h"

#include <QMessageBox>

typedef struct  {
    int note;
    int start;
    int end;
    int velocity;
    int chan;
    int track;

} data_notes;

class QMGroupBox : public QGroupBox {
    Q_OBJECT
public:
    QMGroupBox(QWidget *parent = NULL): QGroupBox(parent){

    }
public slots:
    void paintEvent(QPaintEvent* event) override;
};


class rhythmData : public QByteArray {

public:

    rhythmData();
    rhythmData(const QByteArray a);

    QByteArray save();
    void set_rhythm_header(int size_map, int speed, int drum_set);
    void get_rhythm_header(int *size_map, int *speed, int *drum_set);
    int get_drum_note(int index);
    int get_drum_velocity(int index);
    void set_drum_note(int index, int note);
    void set_drum_velocity(int index, int velocity);
    void insert(int index, unsigned char *set_map);
    void get(int index, unsigned char *get_map);
    void setName(const QString &name);
    QString getName();

private:
    unsigned char _speed;
    unsigned char _drum_set;
    unsigned char _size_map;
    unsigned char _lines;
    QByteArray set[10];
    QString _name;

    QByteArray & append(char ch) {
        return QByteArray::append(ch);
    }

    QByteArray & append(const QByteArray &ba) {
        return QByteArray::append(ba);
    }

};

class velocityDialog : public QDialog {
    Q_OBJECT
public:
    QDialog *Velocity;
    QSlider *velocitySlider;

    velocityDialog(QWidget * parent);

private:
   QLabel *label;
   QLabel *vlabel;
};

class QMComboBox : public QComboBox {

    Q_OBJECT

public:

    QMComboBox(QWidget *parent, int index);

public slots:

    void mousePressEvent(QMouseEvent *event) override;
private:
    int _index;
};


class QMListWidget : public QListWidget {

    Q_OBJECT

public:

    int buttons;
    static QMListWidget * _QMListWidget;

    QMListWidget(QWidget *parent = nullptr, int flag = 0): QListWidget(parent) {
        setSelectionMode(QAbstractItemView::ExtendedSelection);

        _flag = flag != 0;
        _update = false;
        QMListWidget::_drag = -1;
    }

public slots:

    void update();
    void paintEvent(QPaintEvent* event) override;

protected:

    void dropEvent(QDropEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:

    bool _update;
    int _flag;
    static int _drag;

};

class Play_Thread : public QThread {
    Q_OBJECT
public:

    bool loop;
    static Play_Thread * rhythmThread;

    Play_Thread() {
        loop = true;
    }
   ~Play_Thread() {
        Play_Thread::rhythm_samples.clear();
        qWarning("Play_Thread() destructor");
    }

    void run() override;
    void _fun_timer();

    static QList<QByteArray> rhythm_samples;

};

#define MY_PIANO     0
#define MY_DRUM      1
#define MY_RHYTHMBOX 2

class MyInstrument : public QDialog {
    Q_OBJECT
public:
    QSettings* settings;
    QMGroupBox *myBox;
    QGroupBox *groupBox;
    QPushButton *recordButton;
    QSpinBox *octaveBox;
    QLabel *octavelabel;
    QSpinBox *transBox;
    QLabel *translabel;
    QLabel *recordlabel;
    QSlider *volumeSlider;
    QLabel *vollabel;

    // rhythm
    QMComboBox *drumBox[10];
    QPushButton *pButton; // play sample
    QPushButton *lButton; // loop sample
    QMListWidget *rhythmlist;
    QMListWidget *rhythmSamplelist;
    QLineEdit *titleEdit;
    QPushButton *ptButton; // play track
    QPushButton *ltButton; // loop track
    //QPushButton *rButton; // record
    QCheckBox *fixSpeedBox;
    QCheckBox *forceGetTimeBox;
    QCheckBox *fixTrackSpeedBox;
    QSpinBox *loopBox;

    class editDialog *edit_opcion;
    class editDialog *edit_opcion2;

    MyInstrument(QWidget *_MAINW, MatrixWidget* MW, MidiFile* f, int is_drum);

    static void exit_and_clean_MyInstrument();

public slots:
    void reject() override;
    void paintEvent(QPaintEvent* event) override;
    void paintRhythm(QPainter* painter);
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent*) override;

    void record_clicked();
    void transpose_key(int num);
    void octave_key(int num);
    void volume_changed(int vol);

    void RecNote(int note, int startTick, int endTick, int velocity);
    void RecSave();

    void Init_time(int ms);
    int Get_time();

    void rbackToBegin();
    void rback();
    void rplay();
    void rplay2();
    void rpause();
    void rstop();
    void rforward();

    void new_track();
    void load_track();
    void save_track();
    void load_sample();
    void save_sample();
    void copy_samples();
    void paste_samples();


private:

    int drum_mode;
    MatrixWidget* _MW;
    QWidget *_MAINW;
    NoteOnEvent* pianoEvent;
    MidiFile* file;
    int xx,yy;

    int mstotick;
    double msPerTick;
    int _piano_insert_mode;

    qint64 _system_time;
    int _init_time;
    int is_playing;

    int is_playsync;

    QDialog *QD;
    QTimer* time_updat;
    int I_NEED_TO_UPDATE;
    int _piano_timer;
    QList<data_notes*> recNotes;
    QList<QListWidgetItem *> sampleItems;
    QPixmap *pixmapButton[12];
    QImage *DrumImage[11];

    void save_rhythm_edit();
    void RecRhythm(int init_time);
    void push_track_rhythm(QFile *file = 0, int mode = 0);
    void pop_track_rhythm(QFile *file = 0, int mode = 0);

};

#define EDITDIALOG_NONE 0
#define EDITDIALOG_EDIT 1
#define EDITDIALOG_SET  2
#define EDITDIALOG_INSERT 4
#define EDITDIALOG_REPLACE 8
#define EDITDIALOG_REMOVE 16

#define EDITDIALOG_ALL (EDITDIALOG_EDIT | EDITDIALOG_SET | EDITDIALOG_INSERT | \
                        EDITDIALOG_REPLACE | EDITDIALOG_REMOVE)

#define EDITDIALOG_SAMPLE (EDITDIALOG_EDIT | EDITDIALOG_SET | EDITDIALOG_INSERT | \
                           EDITDIALOG_REMOVE)

class editDialog : public QDialog {
    Q_OBJECT
public:

    int action() {
        int a = _action;
        _action = EDITDIALOG_NONE;
        return a;
    }

    editDialog(QWidget * parent, int flags);
    void setItem(QListWidgetItem *item) {
        _item = item;
    }


private:
    QDialog *_editDialog;
    QPushButton *editButton;
    QPushButton *setButton;
    QPushButton *insertButton;
    QPushButton *replaceButton;
    QPushButton *removeButton;
    QListWidgetItem *_item;
    int _action;
};

#endif // MIDIEDITORINSTRUMENT_H
