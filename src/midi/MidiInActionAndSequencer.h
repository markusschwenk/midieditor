#ifndef MIDIINACTIONANDSEQUENCER_H
#define MIDIINACTIONANDSEQUENCER_H

class InputSequencerListWidget;
class InputActionListWidget;
class MidiInControl;

#include "../gui/GuiTools.h"


typedef struct {
    char status;
    char device;
    char channel;
    char event;
    char control_note;
    char category;
    char action;
    char action2;
    char function;
    char min;
    char max;
    char lev0;
    char lev1;
    char lev2;
    char lev3;
    char bypass;
} InputActionData;

enum category {
    CATEGORY_MIDI_EVENTS = 0,
    CATEGORY_AUTOCHORD,
    CATEGORY_FLUIDSYNTH,
    CATEGORY_VST1,
    CATEGORY_VST2,
    CATEGORY_SEQUENCER,
    CATEGORY_FINGERPATTERN,
    CATEGORY_SWITCH,
    CATEGORY_GENERAL,
    CATEGORY_NONE
};

enum action {
    ACTION_PITCHBEND_UP = 0,
    ACTION_MODULATION_WHEEL_UP,
    ACTION_SUSTAIN_UP,
    ACTION_SOSTENUTO_UP,
    ACTION_REVERB_UP,
    ACTION_CHORUS_UP,
    ACTION_PROGRAM_CHANGE_UP,
    ACTION_CHAN_VOLUME_UP,
    ACTION_PAN_UP,
    ACTION_ATTACK_UP,
    ACTION_RELEASE_UP,
    ACTION_DECAY_UP,

    ACTION_PITCHBEND_DOWN = 32,
    ACTION_MODULATION_WHEEL_DOWN,
    ACTION_SUSTAIN_DOWN,
    ACTION_SOSTENUTO_DOWN,
    ACTION_REVERB_DOWN,
    ACTION_CHORUS_DOWN,
    ACTION_PROGRAM_CHANGE_DOWN,
    ACTION_CHAN_VOLUME_DOWN,
    ACTION_PAN_DOWN,
    ACTION_ATTACK_DOWN,
    ACTION_RELEASE_DOWN,
    ACTION_DECAY_DOWN,
};

enum function {
    FUNCTION_VAL = 0,
    FUNCTION_VAL_CLIP,
    FUNCTION_VAL_BUTTON,
    FUNCTION_VAL_SWITCH,
};

enum event {
    EVENT_NOTE = 0,
    EVENT_CONTROL,
    EVENT_SUSTAIN,
    EVENT_EXPRESSION,
    EVENT_SUSTAIN_INV,
    EVENT_EXPRESSION_INV,
    EVENT_AFTERTOUCH,
    EVENT_MODULATION_WHEEL,
    EVENT_PITCH_BEND
};

enum sequencer {
    SEQUENCER_ON_1_UP = 0,
    SEQUENCER_ON_2_UP,
    SEQUENCER_ON_3_UP,
    SEQUENCER_ON_4_UP,
    SEQUENCER_SCALE_1_UP,
    SEQUENCER_SCALE_2_UP,
    SEQUENCER_SCALE_3_UP,
    SEQUENCER_SCALE_4_UP,
    SEQUENCER_SCALE_ALL_UP,
    SEQUENCER_VOLUME_1_UP,
    SEQUENCER_VOLUME_2_UP,
    SEQUENCER_VOLUME_3_UP,
    SEQUENCER_VOLUME_4_UP,
    SEQUENCER_VOLUME_ALL_UP,

    SEQUENCER_ON_1_DOWN = 32,
    SEQUENCER_ON_2_DOWN,
    SEQUENCER_ON_3_DOWN,
    SEQUENCER_ON_4_DOWN,
    SEQUENCER_SCALE_1_DOWN,
    SEQUENCER_SCALE_2_DOWN,
    SEQUENCER_SCALE_3_DOWN,
    SEQUENCER_SCALE_4_DOWN,
    SEQUENCER_SCALE_ALL_DOWN,
    SEQUENCER_VOLUME_1_DOWN,
    SEQUENCER_VOLUME_2_DOWN,
    SEQUENCER_VOLUME_3_DOWN,
    SEQUENCER_VOLUME_4_DOWN,
    SEQUENCER_VOLUME_ALL_DOWN,

};

enum fingerpattern {
    FINGERPATTERN_PICK_UP = 0,
    FINGERPATTERN_1_UP,
    FINGERPATTERN_2_UP,
    FINGERPATTERN_3_UP,
    FINGERPATTERN_SCALE_UP,

    FINGERPATTERN_PICK_DOWN = 32,
    FINGERPATTERN_1_DOWN,
    FINGERPATTERN_2_DOWN,
    FINGERPATTERN_3_DOWN,
    FINGERPATTERN_SCALE_DOWN,
};

enum vst {
    VST_DIS_UP = 0,
    VST_PRE1_UP,
    VST_PRE2_UP,
    VST_PRE3_UP,
    VST_PRE4_UP,
    VST_PRE5_UP,
    VST_PRE6_UP,
    VST_PRE7_UP,

    VST_DIS_DOWN = 32,
    VST_PRE1_DOWN,
    VST_PRE2_DOWN,
    VST_PRE3_DOWN,
    VST_PRE4_DOWN,
    VST_PRE5_DOWN,
    VST_PRE6_DOWN,
    VST_PRE7_DOWN,
};


class InputActionItem : public QWidget {

    Q_OBJECT

public:

    InputActionItem(InputActionListWidget* parent, int index, int number);
    ~InputActionItem() {
        //qWarning("delete ActionItem");
    }

    InputActionData inActiondata;

    int setControlList(QComboBox *box, InputActionData &inActiondata, int val = 0);
    int setFunctionList(QComboBox *box, InputActionData &inActiondata, int val = 0);
    int setActionList(QComboBox *box, InputActionData &inActiondata, int val = 0);
    int setAction2List(QComboBox *box, InputActionData &inActiondata, int val = 0);

    int loadActionSettings(InputActionData &inActiondata, int index, int number);
    void saveActionSettings(InputActionData &inActiondata, int index, int number);

    bool is_selected;

protected:

    void paintEvent(QPaintEvent* event) override;

private:

    QDialogButtonBox *buttonBox;

    // tab3
    QGroupBox *groupActionBox;
    QLabel *device_label;
    QComboBox *deviceBox;
    QLabel *channel_label;
    QComboBox *channelBox;
    QLabel *event_label;
    QComboBox *eventBox;
    QLabel *control_label;
    QComboBox *controlBox;
    QLabel *functionlabel;
    QComboBox *functionBox;

    QLabel *minlabel;
    QDialE *mindial;
    QLabel *minval_label;
    QLabel *maxlabel;
    QDialE *maxdial;
    QLabel *maxval_label;

    QLabel *minlabelBPM;
    QDialE *mindialBPM;
    QLabel *minval_labelBPM;
    QLabel *maxlabelBPM;
    QDialE *maxdialBPM;
    QLabel *maxval_labelBPM;

    QLabel *lev0label;
    QDialE *lev0dial;
    QLabel *lev0val_label;

    QLabel *lev1label;
    QDialE *lev1dial;
    QLabel *lev1val_label;

    QLabel *lev2label;
    QDialE *lev2dial;
    QLabel *lev2val_label;

    QLabel *categorylabel;
    QComboBox *categoryBox;
    QLabel *actionlabel;
    QComboBox *actionBox;
    QComboBox *actionBox2;

    int _number;
    int _index;
};

class InputActionListWidget : public QListWidget {

    Q_OBJECT

public:

    InputActionListWidget(QWidget* parent, MidiInControl* MidiIn);
    void updateList();

    QList<InputActionItem*> items;
    int pairdev;

    int selected;

    MidiInControl * MidiIn;
};

class InputSequencerItem : public QWidget {

    Q_OBJECT

public:

    InputSequencerItem(InputSequencerListWidget* parent, int index, int number);

    bool is_selected;

public slots:

    void load_Seq(QString path);

protected:

    void paintEvent(QPaintEvent* event) override;

private:

    int _number;
    int _index;
    InputSequencerListWidget* _parent;

    QGroupBox *groupSeq;
    QPushButtonE *ButtonOn;
    QPushButtonE *ButtonLoop;
    QPushButtonE *ButtonAuto;
    QCheckBox *checkRhythm;
    QDialE *DialVol;
    QLabel *label_vol;
    QLabel *labelVol;
    QDialE *DialBeats;
    QLabel *label_beats;
    QLabel *labelBeats;
    QComboBox *comboFile;

};

class InputSequencerListWidget : public QListWidget {

    Q_OBJECT

public:

    InputSequencerListWidget(QWidget* parent, MidiInControl* MidiIn);

    void updateList();

    QList<InputSequencerItem*> items;
    int pairdev;

    MidiInControl * MidiIn;

    QSettings *settings;

    int selected;

signals:

    void updateButtons();
};

#endif // MIDIINACTIONANDSEQUENCER_H
