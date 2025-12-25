/*
 * MidiEditor
 * Copyright (C) 2010  Markus Schwenk
 *
 * MidiInControl
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


#ifndef MIDIINCONTROL_H
#define MIDIINCONTROL_H

#include "../gui/GuiTools.h"
#include "MidiInput.h"
#include "../gui/MidiEditorInstrument.h"
#include "MidiInActionAndSequencer.h"


#define AUTOCHORD_MAX 255

#define MAX_OTHER_IN 2

#define MAX_LIST_ACTION 128

enum new_effects_ret {
    RET_NEWEFFECTS_NONE = 0,
    RET_NEWEFFECTS_SKIP,
    RET_NEWEFFECTS_SET,
    RET_NEWEFFECTS_BYPASS = 1000
};

class OSDDialog;

class CustomQTabWidget : public QTabWidget
{
    Q_OBJECT

public:
    CustomQTabWidget(QWidget* parent) : QTabWidget(parent) {

    }

public slots:

    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);

        QLinearGradient linearGrad(0, 0, width(), height());

        linearGrad.setColorAt(0, QColor(0x00, 0x50, 0x70));
        linearGrad.setColorAt(0.5, QColor(0x00, 0x80, 0xa0));
        linearGrad.setColorAt(1, QColor(0x00, 0x60, 0x80));

        painter.fillRect(0, 0, width(), height(), linearGrad);
    }

};

class MidiInControl : public QDialog
{
    Q_OBJECT

public:

    static QString OSD;
    static bool split_enable(int pairdev);
    static int note_cut(int pairdev);
    static bool note_duo(int pairdev);
    static int inchannelUp(int pairdev);
    static int inchannelDown(int pairdev);
    static int channelUp(int pairdev);
    static int channelDown(int pairdev);
    static bool fixVelUp(int pairdev);
    static bool fixVelDown(int pairdev);
    static bool autoChordUp(int pairdev);
    static bool autoChordDown(int pairdev);
    static int current_note();
    static bool notes_only(int pairdev);
    static bool events_to_down();
    static int transpose_note_up(int pairdev);
    static int transpose_note_down(int pairdev);

    static bool skip_prgbanks(int pairdev);
    static bool skip_bankonly(int pairdev);

    static void set_note_cut(int v);
    static void set_current_note(int v);
    static void set_output_prog_bank_channel(int channel);
    static void set_prog(int channel, int value);
    static void set_bank(int channel, int value);
    static int skip_key();
    static int set_key(int key, int evt, int device);
    static int set_ctrl(int key, int evt, int device);
    static void set_leds(int pairdev, bool up, bool down, bool up_err = false, bool down_err = false);

    static int autoChordfunUp(int pairdev, int index, int note, int vel);
    static int autoChordfunDown(int pairdev, int index, int note, int vel);
    static int GetNoteChord(int type, int index, int note);

    //static int set_effect(std::vector<unsigned char>* message, bool is_keyboard2 = false);
    //static int set_effectDev(int index, std::vector<unsigned char>* message);

    static void send_live_events();

    // finger
    static int finger_func(int pairdev, std::vector<unsigned char>* message, bool is_keyboard2 = false, bool only_enable = false);

    static int key_live;
    static int key_flag;

    static int cur_pairdev;

    static QWidget *_main;

    static bool VelocityUP_enable[MAX_INPUT_PAIR];
    static bool VelocityDOWN_enable[MAX_INPUT_PAIR];
    static int VelocityUP_scale[MAX_INPUT_PAIR];
    static int VelocityDOWN_scale[MAX_INPUT_PAIR];
    static int VelocityUP_cut[MAX_INPUT_PAIR];
    static int VelocityDOWN_cut[MAX_INPUT_PAIR];

    static QSettings *_settings;
    static int _current_note;
    static int _current_ctrl;
    static int _current_chan;
    static int _current_device;
    static const char notes[12][3];

    static int sustainUPval;
    static int expressionUPval;
    static int sustainDOWNval;
    static int expressionDOWNval;

    static bool invSustainUP[MAX_INPUT_PAIR];
    static bool invExpressionUP[MAX_INPUT_PAIR];
    static bool scalex2ExpressionUP[MAX_INPUT_PAIR];
    static bool invSustainDOWN[MAX_INPUT_PAIR];
    static bool invExpressionDOWN[MAX_INPUT_PAIR];
    static bool scalex2ExpressionDOWN[MAX_INPUT_PAIR];

    static QString ActionGP;
    static QString SequencerGP;

    static InputActionListWidget * InputListAction;
    static InputSequencerListWidget * InputListSequencer;
    static void sequencer_load(int pairdev);
    static void sequencer_unload(int pairdev);

public slots:
    void paintEvent(QPaintEvent *) override;

    void VST_reset();
    void update_win();
    void update_checks();
    void set_split_enable(bool v);
    void set_inchannelUp(int v);
    void set_inchannelDown(int v);
    void set_channelUp(int v);
    void set_channelDown(int v);
    void set_fixVelUp(bool v);
    void set_fixVelDown(bool v);
    void set_autoChordUp(bool v);
    void set_autoChordDown(bool v);

    void set_notes_only(bool v);
    //static void set_events_to_down(bool v);
    void set_transpose_note_up(int v);
    void set_transpose_note_down(int v);

    void split_reset();
    void panic_button();
    void select_instrumentUp();
    void select_instrumentDown();
    void select_SoundEffectUp();
    void select_SoundEffectDown();

    void set_skip_prgbanks(bool v);
    void set_skip_bankonly(bool v);
    void set_record_waits(bool v);

    void setChordDialogUp();
    void setChordDialogDown();

    void reject() override;
    void accept() override;

public:
    QDialog * MIDIin;

    QComboBox *MIDI_INPUT_SEL;

    QGroupBox *groupBoxNote;

    QPushButton *pushButtonFinger;
    QPushButton *pushButtonMessage;
    QGroupBox *groupBoxVelocityUP;
    QLabel *labelViewVelocityUP;
    QDialE *dialVelocityUP;
    QLabel *labelViewScaleVelocityUP;
    QDialE *dialScaleVelocityUP;
    QGroupBox *groupBoxVelocityDOWN;
    QLabel *labelViewVelocityDOWN;
    QDialE *dialVelocityDOWN;
    QLabel *labelViewScaleVelocityDOWN;
    QDialE *dialScaleVelocityDOWN;

    QComboBox *MIDI_INPUT;
    QComboBox *MIDI_INPUT2;
    QLabel *labelIN;
    QLabel *labelIN2;
    QDialogButtonBox *buttonBox;
    QGroupBox *SplitBox;
    QComboBox *NoteBoxCut;
    QComboBox *channelBoxUp;
    QComboBox *channelBoxDown;
    QLabel *labelUp;
    QLabel *labelDown;
    QLabel *labelCut;
    QLabel *tlabelUp;
    QLabel *tlabelDown;
    QSpinBox *tspinBoxUp;
    QSpinBox *tspinBoxDown;
    QCheckBox *vcheckBoxUp;
    QCheckBox *vcheckBoxDown;
    QCheckBox *echeckBox;
    //QCheckBox *echeckBoxDown;
    QCheckBox *achordcheckBoxUp;
    QCheckBox *achordcheckBoxDown;
    QLedBoxE *LEDBoxUp;
    QLedBoxE *LEDBoxDown;
    QPushButton *chordButtonUp;
    QPushButton *chordButtonDown;
    QPushButton *InstButtonUp;
    QPushButton *InstButtonDown;
    QPushButton *effectButtonUp;
    QPushButton *effectButtonDown;

    QComboBox *inchannelBoxUp;
    QLabel *inlabelUp;
    QComboBox *inchannelBoxDown;
    QLabel *inlabelDown;

    QPushButton *rstButton;
    QPushButton *PanicButton;

    QCheckBox *checkBoxPrgBank;
    QCheckBox *bankskipcheckBox;
    QCheckBox *checkBoxWait;

    QWidget *tabMIDIin1;

    QPedalE * sustainUP;
    QPedalE * expressionUP;
    QPedalE * sustainDOWN;
    QPedalE * expressionDOWN;
    QComboBox * seqSel;

    MidiInControl(QWidget* parent);
    ~MidiInControl();

    static void my_exit();

    static int get_key();
    static int get_key(int chan, int pairdev);
    static int get_ctrl();

    static int wait_record(QWidget *parent);
    static int wait_record_thread();
    static void init_MidiInControl(QWidget *main, QSettings *settings);
    static void MidiIn_toexit(MidiInControl *MidiIn);

    static QList<InputActionData> action_effects[MAX_INPUT_DEVICES];
    
    static void seqOn(int seq, int index, bool on);

    static int new_effects(std::vector<unsigned char>* message, int id);
    static void loadActionSettings();
    static void saveActionSettings();

    static OSDDialog *osd_dialog;

    QComboBox *ActionGroup;
    QLineEdit *ActionTitle;
    QComboBox *SequencerGroup;
    QLineEdit *SequencerTitle;

protected:

    bool event(QEvent *event) override {

        if(event->type() == QEvent::Leave) {

        }


        if(event->type() == QEvent::Enter) {
            MyVirtualKeyboard::overlap();

        }

        return QDialog::event(event);
    }

private:

    bool _thru;

    QTimer *time_update;

    void tab_Actions(QWidget *w);
    void tab_Sequencer(QWidget *w);

};

class MidiInControl_chord : public QDialog
{
    Q_OBJECT
public:
    QDialogButtonBox *buttonBox;
    QLabel *label3;
    QSlider *Slider3;
    QLabel *label5;
    QSlider *Slider5;
    QLabel *label7;
    QSlider *Slider7;
    QPushButton *rstButton;
    QComboBox *chordBox;
    QLabel *label3_2;
    QLabel *label5_2;
    QLabel *label7_2;
    QLabel *labelvelocity;

    int *extChord;
    int *extSlider3;
    int *extSlider5;
    int *extSlider7;

    MidiInControl_chord(QWidget* parent);

};

enum keys {
    KEY_PITCHBEND_UP = (1),
    KEY_MODULATION_WHEEL_UP = (1 << 1),
    KEY_SUSTAIN_UP = (1 << 2),
    KEY_SOSTENUTO_UP = (1 << 3),
    KEY_REVERB_UP = (1 << 4),
    KEY_CHORUS_UP = (1 << 5),
    KEY_PROGRAM_CHANGE_UP = (1 << 6),
    KEY_CHAN_VOLUME_UP = (1 << 7),
    KEY_PAN_UP = (1 << 8),
    KEY_ATTACK_UP = (1 << 9),
    KEY_RELEASE_UP = (1 << 10),
    KEY_DECAY_UP = (1 << 11),
    KEY_PEDAL_UP = (1 << 12),
    KEY_EXPRESSION_UP = (1 << 13),

    KEY_PITCHBEND_DOWN = (1 << 16),
    KEY_MODULATION_WHEEL_DOWN = (1 << 17),
    KEY_SUSTAIN_DOWN = (1 << 18),
    KEY_SOSTENUTO_DOWN = (1 << 19),
    KEY_REVERB_DOWN = (1 << 20),
    KEY_CHORUS_DOWN = (1 << 21),
    KEY_PROGRAM_CHANGE_DOWN = (1 << 22),
    KEY_CHAN_VOLUME_DOWN = (1 << 23),
    KEY_PAN_DOWN = (1 << 24),
    KEY_ATTACK_DOWN = (1 << 25),
    KEY_RELEASE_DOWN = (1 << 26),
    KEY_DECAY_DOWN = (1 << 27),
    KEY_PEDAL_DOWN = (1 << 28),
    KEY_EXPRESSION_DOWN = (1 << 29),

    KEY_EXPRESSION_PEDAL = (1 << 30),
    KEY_AFTERTOUCH = (1 << 31)
};

enum keys2 {
    KEY_SEQ_S1_UP = (1),
    KEY_SEQ_S2_UP = (1 << 1),
    KEY_SEQ_S3_UP = (1 << 2),
    KEY_SEQ_S4_UP = (1 << 3),
    KEY_SEQ_SALL_UP = (1 << 4),
    KEY_SEQ_V1_UP = (1 << 5),
    KEY_SEQ_V2_UP = (1 << 6),
    KEY_SEQ_V3_UP = (1 << 7),
    KEY_SEQ_V4_UP = (1 << 8),
    KEY_SEQ_VALL_UP = (1 << 9),

    KEY_SEQ_S1_DOWN = (1 << 10),
    KEY_SEQ_S2_DOWN = (1 << 11),
    KEY_SEQ_S3_DOWN = (1 << 12),
    KEY_SEQ_S4_DOWN = (1 << 13),
    KEY_SEQ_SALL_DOWN = (1 << 14),
    KEY_SEQ_V1_DOWN = (1 << 15),
    KEY_SEQ_V2_DOWN = (1 << 16),
    KEY_SEQ_V3_DOWN = (1 << 17),
    KEY_SEQ_V4_DOWN = (1 << 18),
    KEY_SEQ_VALL_DOWN = (1 << 19),

    KEY_FINGER_UP = (1 << 24),
    KEY_FINGER_DOWN = (1 << 25),
};

class OSDDialog : public QDialog {

    Q_OBJECT

public:

    OSDDialog(QDialog *parent, QString osd) : QDialog(parent, Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
    {
        QFont font;
        font.setPointSize(16);
        font.setBold(true);

        QFontMetrics fm(font);
        QRect r1 = fm.boundingRect(osd);
        int w = r1.width() + r1.x();

        if (this->objectName().isEmpty())
            this->setObjectName(QString::fromUtf8("OSDDialog"));
        this->setWindowTitle("");
        this->setFixedSize(w + 24, 57);

        setFocusPolicy(Qt::NoFocus);
        setStyleSheet("background:transparent;");
        setAttribute(Qt::WA_TranslucentBackground);

        label = new QLabel(this);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(12, 1, w, 46));
        label->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        label->setFont(font);
        label->setText(osd);
        label->setStyleSheet(QString::fromUtf8("color: #e0ffe0"));

        time_update = new QTimer(this);

        connect(time_update, &QTimer::timeout, this, [=]()
        {

            hide();

        });

        time_update2 = new QTimer(this);

        connect(time_update2, &QTimer::timeout, this, [=]()
        {
            if(MidiInControl::OSD != "") {

                QString osd = MidiInControl::OSD;
                setOSD(osd);

                MidiInControl::OSD = "";
                move(16, 16);

                setModal(false);
                show();
                raise();
                //activateWindow();
                MyVirtualKeyboard::overlap();
            }

            emit OSDtimeUpdate();

        });

        time_update->setSingleShot(true);
        time_update->start(1);
        time_update2->setSingleShot(false);
        time_update2->start(50);

    }

    ~OSDDialog() {
        time_update2->stop();
        time_update->stop();
    }

    void setOSD(QString osd) {

        QFont font;
        font.setPointSize(16);
        font.setBold(true);

        QFontMetrics fm(font);
        QRect r1 = fm.boundingRect(osd);
        int w = r1.width() + r1.x();
        this->setFixedSize(w + 24, 57);
        label->setGeometry(QRect(12, 1, w, 46));

        label->setText(osd);
        setVisible(true);
        time_update->stop();
        time_update->start(3000);
    }

signals:

    void OSDtimeUpdate();

protected:

    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);

        painter.setBrush(QBrush(QColor(60, 0, 90, 192)));
        painter.drawRoundedRect(0, 0, width(), height(), 16, 16);

    }

private:

    QLabel *label;

    QTimer *time_update;
    QTimer *time_update2;

};


#endif // MIDIINCONTROL_H
