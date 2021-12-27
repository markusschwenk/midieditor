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

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QDial>
#include <QSettings>
#include <QTimer>

#define AUTOCHORD_MAX 255

class MidiInControl : public QDialog
{
    Q_OBJECT

public:

    static bool split_enable();
    static int note_cut();
    static bool note_duo();
    static int inchannelUp();
    static int inchannelDown();
    static int channelUp();
    static int channelDown();
    static bool fixVelUp();
    static bool fixVelDown();
    static bool autoChordUp();
    static bool autoChordDown();
    static int current_note();
    static bool notes_only();
    static bool events_to_down();
    static int transpose_note_up();
    static int transpose_note_down();

    static bool key_effect();

    static bool skip_prgbanks();
    static bool skip_bankonly();

    static void set_note_cut(int v);
    static void set_current_note(int v);
    static void set_output_prog_bank_channel(int channel);
    static void set_prog(int channel, int value);
    static void set_bank(int channel, int value);
    static void set_key(int key);
    static void set_leds(bool up, bool down);
    static int autoChordfunUp(int index, int note, int vel);
    static int autoChordfunDown(int index, int note, int vel);
    static int GetNoteChord(int type, int index, int note);

    static int set_effect(std::vector<unsigned char>* message);

    static void send_live_events();

    // finger
    static int finger_func(std::vector<unsigned char>* message);

    static int key_live;
    static int key_flag;

    static bool VelocityUP_enable;
    static bool VelocityDOWN_enable;
    static int VelocityUP_scale;
    static int VelocityDOWN_scale;
    static int VelocityUP_cut;
    static int VelocityDOWN_cut;
    static int expression_mode;
    static int aftertouch_mode;

public slots:
    void paintEvent(QPaintEvent *) override;

    void VST_reset();
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
    static void set_events_to_down(bool v);
    void set_transpose_note_up(int v);
    void set_transpose_note_down(int v);

    void split_reset();
    void panic_button();
    void select_instrumentUp();
    void select_instrumentDown();
    void select_SoundEffectUp();
    void select_SoundEffectDown();

    void set_key_effect(bool v);
    void set_note_effect1_value(int v);
    void set_note_effect1_type(int v);
    void set_note_effect1_fkeypressed(bool v);
    void set_note_effect1_usevel(bool v);
    void set_note_effect2_value(int v);
    void set_note_effect2_type(int v);
    void set_note_effect2_fkeypressed(bool v);
    void set_note_effect2_usevel(bool v);
    void set_note_effect3_value(int v);
    void set_note_effect3_type(int v);
    void set_note_effect3_fkeypressed(bool v);
    void set_note_effect3_usevel(bool v);
    void set_note_effect4_value(int v);
    void set_note_effect4_type(int v);
    void set_note_effect4_fkeypressed(bool v);
    void set_note_effect4_usevel(bool v);

    void set_skip_prgbanks(bool v);
    void set_skip_bankonly(bool v);
    void set_record_waits(bool v);

    void setChordDialogUp();
    void setChordDialogDown();

    void reject() override;
    void accept() override;

public:
    QDialog * MIDIin;

    QGroupBox *groupBoxNote;
    QComboBox *comboBoxExpression;
    QComboBox *comboBoxAfterTouch;
    QPushButton *pushButtonFinger;
    QPushButton *pushButtonMessage;
    QGroupBox *groupBoxVelocityUP;
    QLabel *labelViewVelocityUP;
    QDial *dialVelocityUP;
    QLabel *labelViewScaleVelocityUP;
    QDial *dialScaleVelocityUP;
    QGroupBox *groupBoxVelocityDOWN;
    QLabel *labelViewVelocityDOWN;
    QDial *dialVelocityDOWN;
    QLabel *labelViewScaleVelocityDOWN;
    QDial *dialScaleVelocityDOWN;

    QComboBox *MIDI_INPUT;
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
    QCheckBox *echeckBoxDown;
    QCheckBox *achordcheckBoxUp;
    QCheckBox *achordcheckBoxDown;
    QCheckBox *LEDBoxUp;
    QCheckBox *LEDBoxDown;
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

    QGroupBox *groupBoxEffect;

    QComboBox *typeBoxEffect1;
    QCheckBox *LEDBoxEffect1;
    QCheckBox *pressedBoxEffect1;
    QCheckBox *useVelocityBoxEffect1;
    QComboBox *NoteBoxEffect1;
    QLabel *labelEffect1;
    QSlider *horizontalSliderPitch1;
    QLabel *labelPitch1;
    QLabel *VlabelPitch1;
    QComboBox *VSTBoxPresetOff1;
    QComboBox *VSTBoxPresetOn1;

    QComboBox *typeBoxEffect2;
    QCheckBox *LEDBoxEffect2;
    QCheckBox *pressedBoxEffect2;
    QCheckBox *useVelocityBoxEffect2;
    QComboBox *NoteBoxEffect2;
    QLabel *labelEffect2;
    QSlider *horizontalSliderPitch2;
    QLabel *labelPitch2;
    QLabel *VlabelPitch2;
    QComboBox *VSTBoxPresetOff2;
    QComboBox *VSTBoxPresetOn2;

    QComboBox *typeBoxEffect3;
    QCheckBox *LEDBoxEffect3;
    QCheckBox *pressedBoxEffect3;
    QCheckBox *useVelocityBoxEffect3;
    QComboBox *NoteBoxEffect3;
    QLabel *labelEffect3;
    QSlider *horizontalSliderPitch3;
    QLabel *labelPitch3;
    QLabel *VlabelPitch3;
    QComboBox *VSTBoxPresetOff3;
    QComboBox *VSTBoxPresetOn3;

    QComboBox *typeBoxEffect4;
    QCheckBox *LEDBoxEffect4;
    QCheckBox *pressedBoxEffect4;
    QCheckBox *useVelocityBoxEffect4;
    QComboBox *NoteBoxEffect4;
    QLabel *labelEffect4;
    QSlider *horizontalSliderPitch4;
    QLabel *labelPitch4;
    QLabel *VlabelPitch4;
    QComboBox *VSTBoxPresetOff4;
    QComboBox *VSTBoxPresetOn4;

    QCheckBox *checkBoxPrgBank;
    QCheckBox *bankskipcheckBox;
    QCheckBox *checkBoxWait;

    MidiInControl(QWidget* parent);
    ~MidiInControl();

    static void my_exit();

    static int get_key();

    static int wait_record(QWidget *parent);
    static int wait_record_thread();
    static void init_MidiInControl(QSettings *settings);
    static void MidiIn_toexit(MidiInControl *MidiIn);

private:
    bool _thru;

    QTimer *time_updat;

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

#endif // MIDIINCONTROL_H
