#include "MidiInControl.h"


#include <QDir>
#include <QFileInfo>
#include <QFileDialog>
#include <QScrollBar>

////////////////////////////////////////////////////////////////////////
// InputActionListWidget
////////////////////////////////////////////////////////////////////////

InputActionListWidget::InputActionListWidget(QWidget* parent, MidiInControl * MidiIn) : QListWidget(parent) {

    this->MidiIn = MidiIn;

    setAutoScroll(false);
    setSelectionMode(QAbstractItemView::NoSelection);

    setStyleSheet("QListWidget {background-color: #e0e0c0;}\n"
                  "QListWidget::item QGroupBox {font: bold; border-bottom: 2px solid blue;\n"
                  "border: 2px solid black; border-radius: 7px; margin-top: 1ex;}\n"
                  "QGroupBox::indicator:unchecked {image: url(:/run_environment/graphics/custom/unchecked.png);}\n"
                  "QGroupBox::indicator:checked {image: url(:/run_environment/graphics/custom/checked.png);}\n"
                  "QGroupBox QCheckBox::indicator:unchecked {image: url(:/run_environment/graphics/custom/unchecked.png);}\n"
                  "QGroupBox QCheckBox::indicator:checked {image: url(:/run_environment/graphics/custom/checked.png);}\n"
                  "QGroupBox::title {subcontrol-origin: margin; left: 12px;\n"
                  "subcontrol-position: top left; padding: 0 2px; }");

    updateList();

    connect(this, &QListWidget::itemClicked, this, [=](QListWidgetItem* i)
    {
        selected = i->data(Qt::UserRole).toInt();

        for(int n = 0; n < this->items.count(); n++) {
            if(n == selected)
                items.at(n)->is_selected = true;
            else
                items.at(n)->is_selected = false;
        }

        update();

    });

}

void InputActionListWidget::updateList() {

    items.clear();
    clear();

    pairdev = MidiInControl::cur_pairdev;

    selected = 0;

    for(int n = 0; (n < 4) || (n < MidiInControl::action_effects[MidiInControl::cur_pairdev].count()); n++) {
        InputActionItem* widget = new InputActionItem(this, MidiInControl::cur_pairdev, n);
        if(n == selected)
            widget->is_selected = true;
        else
            widget->is_selected = false;

        QListWidgetItem* item = new QListWidgetItem();
        item->setSizeHint(QSize(0, 140));
        item->setData(Qt::UserRole, n);
        addItem(item);
        setItemWidget(item, widget);
        items.append(widget);
    }

}

extern QBrush background1;

void InputActionItem::paintEvent(QPaintEvent* event) {

    QWidget::paintEvent(event);

    // Estwald Color Changes
    QPainter *p = new QPainter(this);
    if(!p) return;

    p->fillRect(0, 0, width(), height(), background1);

    p->setPen(0x808080);
    p->drawLine(0, height() - 1, width(), height() - 1);

    if(is_selected) {
        QColor c(0x80ffff);
        c.setAlpha(32);
        p->fillRect(0, 0, width(), height() - 2, c);
    }

    delete p;
}

int InputActionItem::loadActionSettings(InputActionData &inActiondata, int index, int number) {

    inActiondata.status = 1;
    inActiondata.device = -1; // none
    inActiondata.channel = 17; // user
    inActiondata.event = 0; // note
    inActiondata.control_note = -1;
    inActiondata.category = CATEGORY_MIDI_EVENTS;
    inActiondata.action = 0;
    inActiondata.action2 = 0;
    inActiondata.function = 0;
    inActiondata.min = 0;
    inActiondata.max = 127;
    inActiondata.lev0 = 14;
    inActiondata.lev1 = 15;
    inActiondata.lev2 = 16;
    inActiondata.lev3 = 15;
    inActiondata.bypass = -1;

    if(MidiInControl::action_effects[index].count() > number) {
        inActiondata = MidiInControl::action_effects[index].at(number);

        return 1;
    }

    return 0;
}

void InputActionItem::saveActionSettings(InputActionData &inActiondata, int index, int number) {

    QByteArray d;

    d.append((const char *) &inActiondata, 16);

    if(MidiInControl::action_effects[index].count() > number)
        MidiInControl::action_effects[index][number] = inActiondata;
    else
        MidiInControl::action_effects[index].append(inActiondata);


    MidiInControl::_settings->setValue(MidiInControl::ActionGP + "/inActiondata" + QString::number(index) + " " + QString::number(number), d);

}

InputActionItem::InputActionItem(InputActionListWidget* parent, int index, int number) : QWidget(parent) {

    _number = number;
    _index  = index;

    inActiondata.status = 1;
    inActiondata.device = -1; // none
    inActiondata.channel = 17; // user
    inActiondata.event = 0; // note
    inActiondata.control_note = -1;
    inActiondata.function = 0;
    inActiondata.category = CATEGORY_MIDI_EVENTS;
    inActiondata.action = 0;
    inActiondata.action2 = 0;
    inActiondata.min = 0;
    inActiondata.max = 127;
    inActiondata.lev0 = 14;
    inActiondata.lev1 = 15;
    inActiondata.lev2 = 16;
    inActiondata.lev3 = 16;
    inActiondata.bypass = -1;

    loadActionSettings(inActiondata, _index, _number);

    groupActionBox = new QGroupBox(this);
    groupActionBox->setObjectName(QString::fromUtf8("groupActionBox"));
    groupActionBox->setGeometry(QRect(9, 10, 931 - 180, 121));
    groupActionBox->setTitle("Action " + QString::number(number));
    groupActionBox->setCheckable(true);
    TOOLTIP(groupActionBox, "Action group: list of actions that will be carried out when a note is pressed,\n"
                            "a change control is received or one of the device's pedals is actuated.\n\n"
                            "If the box is unchecked the settings cannot be modified");

    device_label = new QLabel(groupActionBox);
    device_label->setObjectName(QString::fromUtf8("device_label"));
    device_label->setGeometry(QRect(10, 20, 81, 20));
    device_label->setAlignment(Qt::AlignCenter);
    device_label->setText("Device Input");

    deviceBox = new QComboBox(groupActionBox);
    deviceBox->setObjectName(QString::fromUtf8("deviceBox"));
    deviceBox->setGeometry(QRect(10, 50, 81, 22));
    TOOLTIP(deviceBox, "Choose the source device here for the action ");

    deviceBox->addItem("EMPTY", -1);
    int dev = index * 2;
    for(int n = 0; n < MAX_INPUT_DEVICES; n++) {
        if(n == dev || (n == ((dev + 1) /*&& !(index & 1)*/)))
            deviceBox->addItem("Midi Dev " + QString::number(n), n);
    }

    for(int n = 0; n < deviceBox->count(); n++) {
        if(deviceBox->itemData(n).toInt() == inActiondata.device) {
            deviceBox->setCurrentIndex(n);
            break;
        }
    }

    channel_label = new QLabel(groupActionBox);
    channel_label->setObjectName(QString::fromUtf8("channel_label"));
    channel_label->setGeometry(QRect(100, 20, 61, 20));
    channel_label->setAlignment(Qt::AlignCenter);
    channel_label->setText("Channel");

    channelBox = new QComboBox(groupActionBox);
    channelBox->setObjectName(QString::fromUtf8("channelBox"));
    channelBox->setGeometry(QRect(100, 50, 61, 22));
    TOOLTIP(channelBox, "Choose here the MIDI channel of the event that will start the action.\n"
                        "'All' will listen to all channels.\n"
                        "'User' to use the input channel associated with the device");

    for(int n = 0; n < 18; n++) {
        if(n == 0)
            channelBox->addItem("All", 16);
        else if(n == 1)
            channelBox->addItem("User", 17);
        else
            channelBox->addItem("Ch " + QString::number(n - 2), n - 2);

    }


    for(int n = 0; n < channelBox->count(); n++) {
        if(channelBox->itemData(n).toInt() == inActiondata.channel) {
            channelBox->setCurrentIndex(n);
            break;
        }
    }

    event_label = new QLabel(groupActionBox);
    event_label->setObjectName(QString::fromUtf8("event_label"));
    event_label->setGeometry(QRect(170, 20, 141, 20));
    event_label->setAlignment(Qt::AlignCenter);
    event_label->setText("Event type");

    eventBox = new QComboBox(groupActionBox);
    eventBox->setObjectName(QString::fromUtf8("eventBox"));
    eventBox->setGeometry(QRect(170, 50, 131, 22));
    eventBox->addItem("Note Event", EVENT_NOTE);
    eventBox->addItem("Control Event", EVENT_CONTROL);

    if(inActiondata.category == CATEGORY_SWITCH) {
        if(inActiondata.event > EVENT_CONTROL)
            inActiondata.event = EVENT_NOTE;
    } else {
        eventBox->addItem("Sustain Pedal", EVENT_SUSTAIN);
        eventBox->addItem("Expression Pedal", EVENT_EXPRESSION);
        eventBox->addItem("Sustain Pedal Inv", EVENT_SUSTAIN_INV);
        eventBox->addItem("Expression Pedal Inv", EVENT_EXPRESSION_INV);
        eventBox->addItem("Aftertouch", EVENT_AFTERTOUCH);
        eventBox->addItem("Modulation Wheel", EVENT_MODULATION_WHEEL);
        eventBox->addItem("Pitch Bend", EVENT_PITCH_BEND);
    }

    TOOLTIP(eventBox, "Choose here the type of event that will start the action.");

    for(int n = 0; n < eventBox->count(); n++) {
        if(eventBox->itemData(n).toInt() == inActiondata.event) {
            eventBox->setCurrentIndex(n);
            break;
        }
    }

    control_label = new QLabel(groupActionBox);
    control_label->setObjectName(QString::fromUtf8("control_label"));
    control_label->setGeometry(QRect(310, 20, 92, 20));
    control_label->setAlignment(Qt::AlignCenter);
    control_label->setText("Control/Note");

    controlBox = new QComboBox(groupActionBox);
    controlBox->setObjectName(QString::fromUtf8("controlBox"));
    controlBox->setGeometry(QRect(310, 50, 91, 22));
    controlBox->setStyleSheet("QComboBox QAbstractItemView {min-width: 300px;}");
    TOOLTIP(controlBox, "Choose here the value (note or control number) of the event that will start the action.\n"
                        "'Get It' will wait a few seconds reading the device input until you press/act an event.\n"
                        "If when you act on an event it doesn't take it, check the input channel (use 'ALL' if you don't know it)");

    controlBox->setCurrentIndex(setControlList(controlBox, inActiondata, inActiondata.control_note));

    for(int n = 0; n < controlBox->count(); n++) {
        if(controlBox->itemData(n).toInt() == inActiondata.control_note) {
            controlBox->setCurrentIndex(n);
            break;
        }
    }

    functionlabel = new QLabel(groupActionBox);
    functionlabel->setObjectName(QString::fromUtf8("functionlabel"));
    functionlabel->setGeometry(QRect(410, 20, 81, 20));
    functionlabel->setAlignment(Qt::AlignCenter);
    functionlabel->setText("Function");

    functionBox = new QComboBox(groupActionBox);
    functionBox->setObjectName(QString::fromUtf8("functionBox"));
    functionBox->setGeometry(QRect(410, 50, 81, 22));
    TOOLTIP(functionBox, "Choose here the type of function that the event will perform:\n"
                         "Ev. Val: takes the value associated with the event to perform the action.\n"
                         "Ev. Val Clip: takes and trims the value associated with the event to perform the action using the min/max controls.\n"
                         "Button: takes the value or event type to perform the action as a button.\n"
                         "Switch: takes the value or event type to perform the action as a switch.\n");

    for(int n = 0; n < functionBox->count(); n++) {
        if(functionBox->itemData(n).toInt() == inActiondata.function) {
            functionBox->setCurrentIndex(n);
            break;
        }
    }

    minlabel = new QLabel(groupActionBox);
    minlabel->setObjectName(QString::fromUtf8("minlabel"));
    minlabel->setGeometry(QRect(788 - 180, 3, 61, 20));
    minlabel->setAlignment(Qt::AlignCenter);
    minlabel->setText("Min / Off");

    mindial = new QDialE(groupActionBox, QDIALE_GRAY_NOTCH_BLACKDOT);
    mindial->setObjectName(QString::fromUtf8("mindial"));
    mindial->setGeometry(QRect(787 - 180, 19, 61, 64));
    mindial->setMinimum(0);
    mindial->setMaximum(127);
    mindial->setNotchTarget(16.000000000000000);
    mindial->setNotchesVisible(true);
    TOOLTIP(mindial, "For actions that require a value, use the value of this control as the minimum or shutdown value");

    minval_label = new QLabel(groupActionBox);
    minval_label->setObjectName(QString::fromUtf8("minval_label"));
    minval_label->setGeometry(QRect(803 - 180, 73 , 31, 19));
    minval_label->setAlignment(Qt::AlignCenter);
    minval_label->setText("0");

    maxlabel = new QLabel(groupActionBox);
    maxlabel->setObjectName(QString::fromUtf8("maxlabel"));
    maxlabel->setGeometry(QRect(860 - 180, 3, 61, 20));
    maxlabel->setAlignment(Qt::AlignCenter);
    maxlabel->setText("Max / On");

    maxdial = new QDialE(groupActionBox, QDIALE_GRAY_NOTCH_BLACKDOT);
    maxdial->setObjectName(QString::fromUtf8("maxdial"));
    maxdial->setGeometry(QRect(859 - 180, 19, 61, 64));
    maxdial->setMaximum(127);
    maxdial->setNotchTarget(16.000000000000000);
    maxdial->setNotchesVisible(true);
    TOOLTIP(maxdial, "For actions that require a value, use the value of this control as the maximum or power-on value");

    maxval_label = new QLabel(groupActionBox);
    maxval_label->setObjectName(QString::fromUtf8("maxval_label"));
    maxval_label->setGeometry(QRect(874 - 180, 73, 31, 20));
    maxval_label->setAlignment(Qt::AlignCenter);
    maxval_label->setText("0");

    /******************************************************/
    // for BPM

    minlabelBPM = new QLabel(groupActionBox);
    minlabelBPM->setObjectName(QString::fromUtf8("minlabelBPM"));
    minlabelBPM->setGeometry(QRect(788 - 180, 3, 61, 20));
    minlabelBPM->setAlignment(Qt::AlignCenter);
    minlabelBPM->setText("BPM Min");

    mindialBPM = new QDialE(groupActionBox, QDIALE_GRAY_NOTCH_BLACKDOT);
    mindialBPM->setObjectName(QString::fromUtf8("mindialBPM"));
    mindialBPM->setGeometry(QRect(787 - 180, 19, 61, 64));
    mindialBPM->setMinimum(5);
    mindialBPM->setMaximum(500);
    mindialBPM->setNotchTarget(32.000000000000000);
    mindialBPM->setNotchesVisible(true);
    TOOLTIP(mindialBPM, "Beats Per Minute (BPM) used in the sequencer when the action stops (off sequencer value).");

    minval_labelBPM = new QLabel(groupActionBox);
    minval_labelBPM->setObjectName(QString::fromUtf8("minval_labelBPM"));
    minval_labelBPM->setGeometry(QRect(803 - 180, 73 , 31, 19));
    minval_labelBPM->setAlignment(Qt::AlignCenter);
    minval_labelBPM->setText("0");

    maxlabelBPM = new QLabel(groupActionBox);
    maxlabelBPM->setObjectName(QString::fromUtf8("maxlabelBPM"));
    maxlabelBPM->setGeometry(QRect(860 - 180, 3, 61, 20));
    maxlabelBPM->setAlignment(Qt::AlignCenter);
    maxlabelBPM->setText("BPM Max");
    TOOLTIP(maxlabelBPM, "Beats Per Minute (BPM) used in the sequencer when the action starts (On sequencer value).");

    maxdialBPM = new QDialE(groupActionBox, QDIALE_GRAY_NOTCH_BLACKDOT);
    maxdialBPM->setObjectName(QString::fromUtf8("maxdial"));
    maxdialBPM->setGeometry(QRect(859 - 180, 19, 61, 64));
    maxdialBPM->setMinimum(5);
    maxdialBPM->setMaximum(500);
    maxdialBPM->setNotchTarget(32.000000000000000);
    maxdialBPM->setNotchesVisible(true);

    maxval_labelBPM = new QLabel(groupActionBox);
    maxval_labelBPM->setObjectName(QString::fromUtf8("maxval_label"));
    maxval_labelBPM->setGeometry(QRect(874 - 180, 73, 31, 20));
    maxval_labelBPM->setAlignment(Qt::AlignCenter);
    maxval_labelBPM->setText("0");

    /******************************************************/
    // for autochord

    lev0label = new QLabel(groupActionBox);
    lev0label->setObjectName(QString::fromUtf8("lev0label"));
    lev0label->setGeometry(QRect(716 - 180, 3, 61, 20));
    lev0label->setAlignment(Qt::AlignCenter);
    lev0label->setText("Level [3]");

    lev0dial = new QDialE(groupActionBox, QDIALE_GRAY_NOTCH_BLACKDOT);
    lev0dial->setObjectName(QString::fromUtf8("lev0dial"));
    lev0dial->setGeometry(QRect(715 - 180, 19, 61, 64));
    lev0dial->setMaximum(20);
    lev0dial->setNotchTarget(1.000000000000000);
    lev0dial->setNotchesVisible(true);
    TOOLTIP(lev0dial, "Scale value to adjust the velocity of the third note in chords");

    lev0val_label = new QLabel(groupActionBox);
    lev0val_label->setObjectName(QString::fromUtf8("lev0val_label"));
    lev0val_label->setGeometry(QRect(730 - 180, 73, 31, 20));
    lev0val_label->setAlignment(Qt::AlignCenter);
    lev0val_label->setText("000");

    lev1label = new QLabel(groupActionBox);
    lev1label->setObjectName(QString::fromUtf8("lev1label"));
    lev1label->setGeometry(QRect(788 - 180, 3, 61, 20));
    lev1label->setAlignment(Qt::AlignCenter);
    lev1label->setText("Level [5]");

    lev1dial = new QDialE(groupActionBox, QDIALE_GRAY_NOTCH_BLACKDOT);
    lev1dial->setObjectName(QString::fromUtf8("lev1dial"));
    lev1dial->setGeometry(QRect(787 - 180, 19, 61, 64));
    lev1dial->setMaximum(20);
    lev1dial->setNotchTarget(1.000000000000000);
    lev1dial->setNotchesVisible(true);
    TOOLTIP(lev1dial, "Scale value to adjust the velocity of the fifth note in chords");

    lev1val_label = new QLabel(groupActionBox);
    lev1val_label->setObjectName(QString::fromUtf8("lev1val_label"));
    lev1val_label->setGeometry(QRect(803 - 180, 73, 31, 19));
    lev1val_label->setAlignment(Qt::AlignCenter);
    lev1val_label->setText("000");

    lev2label = new QLabel(groupActionBox);
    lev2label->setObjectName(QString::fromUtf8("lev2label"));
    lev2label->setGeometry(QRect(860 - 180, 3, 61, 20));
    lev2label->setAlignment(Qt::AlignCenter);
    lev2label->setText("Level [7]");

    lev2dial = new QDialE(groupActionBox, QDIALE_GRAY_NOTCH_BLACKDOT);
    lev2dial->setObjectName(QString::fromUtf8("lev2dial"));
    lev2dial->setGeometry(QRect(859 - 180, 19, 61, 64));
    lev2dial->setMaximum(20);
    lev2dial->setNotchTarget(1.000000000000000);
    lev2dial->setNotchesVisible(true);
    TOOLTIP(lev2dial, "Scale value to adjust the velocity of the seventh note in chords");

    lev2val_label = new QLabel(groupActionBox);
    lev2val_label->setObjectName(QString::fromUtf8("lev2val_label"));
    lev2val_label->setGeometry(QRect(874 - 180, 73, 31, 20));
    lev2val_label->setAlignment(Qt::AlignCenter);
    lev2val_label->setText("000");

    /******************************************************/

    categorylabel = new QLabel(groupActionBox);
    categorylabel->setObjectName(QString::fromUtf8("categorylabel"));
    categorylabel->setGeometry(QRect(8, 70, 53, 20));
    categorylabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    categorylabel->setText("Category:");

    categoryBox = new QComboBox(groupActionBox);
    categoryBox->setObjectName(QString::fromUtf8("categoryBox"));
    categoryBox->setGeometry(QRect(12, 90, 290, 22));
    categoryBox->addItem("Midi Events", CATEGORY_MIDI_EVENTS);
    categoryBox->addItem("Autochord", CATEGORY_AUTOCHORD);
    categoryBox->addItem("Fluidsynth Control", CATEGORY_FLUIDSYNTH);
    categoryBox->addItem("VST1 Control", CATEGORY_VST1);
    categoryBox->addItem("VST2 Control", CATEGORY_VST2);
    categoryBox->addItem("Sequencer", CATEGORY_SEQUENCER);
    categoryBox->addItem("Finger Pattern", CATEGORY_FINGERPATTERN);
    categoryBox->addItem("Switch (Pedals, Aftertouch, Modulation and Pitch Bend)", CATEGORY_SWITCH);
    categoryBox->addItem("General", CATEGORY_GENERAL);
    categoryBox->addItem("None (do nothing)", CATEGORY_NONE);
    TOOLTIP(categoryBox, "Category to which the action belongs");

    for(int n = 0; n < categoryBox->count(); n++) {
        if(categoryBox->itemData(n).toInt() == inActiondata.category) {
            categoryBox->setCurrentIndex(n);
            break;
        }
    }

    actionlabel = new QLabel(groupActionBox);
    actionlabel->setObjectName(QString::fromUtf8("actionlabel"));
    actionlabel->setGeometry(QRect(305, 70, 40, 20));
    actionlabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    actionlabel->setText("Action:");

    actionBox = new QComboBox(groupActionBox);
    actionBox->setObjectName(QString::fromUtf8("actionBox"));
    actionBox->setGeometry(QRect(309, 90, /*430*/210, 22));
    TOOLTIP(actionBox, "Action to take");

    for(int n = 0; n < actionBox->count(); n++) {
        if(actionBox->itemData(n).toInt() == inActiondata.action) {
            actionBox->setCurrentIndex(n);
            break;
        }
    }

    QLabel *actionlabel2 = new QLabel(groupActionBox);
    actionlabel2->setObjectName(QString::fromUtf8("actionlabel2"));
    actionlabel2->setGeometry(QRect(309 + 220, 70, 80, 20));
    actionlabel2->setAlignment(Qt::AlignLeft |Qt::AlignVCenter);
    actionlabel2->setText("Action Off:");

    actionBox2 = new QComboBox(groupActionBox);
    actionBox2->setObjectName(QString::fromUtf8("actionBox"));
    actionBox2->setGeometry(QRect(309 + 220, 90, 210, 22));
    TOOLTIP(actionBox2, "Shutdown action upon completion");

    for(int n = 0; n < actionBox2->count(); n++) {
        if(actionBox2->itemData(n).toInt() == inActiondata.action2) {
            actionBox2->setCurrentIndex(n);
            break;
        }
    }

    QComboBox *bypassBox = new QComboBox(groupActionBox);
    bypassBox->setObjectName(QString::fromUtf8("bypassBox"));
    bypassBox->setGeometry(QRect(609 + 6, 90, 124, 22));
    bypassBox->addItem("Without Bypass", -1);
    TOOLTIP(bypassBox, "Bypass to the device where the action will be performed.\n"
                       "This allows an event from one MIDI device to perform an action associated\n"
                       "with a different MIDI device (in practice affecting a different MIDI output\n"
                       "channel within MidiEditor).");

    for(int n = 0; n < MAX_INPUT_DEVICES; n++) {
        bypassBox->addItem("Bypass to Device " + QString::number(n), n);
    }


    for(int n = 0; n < bypassBox->count(); n++) {
        if(bypassBox->itemData(n).toInt() == inActiondata.bypass) {
            bypassBox->setCurrentIndex(n);
            break;
        }
    }

    connect(bypassBox, QOverload<int>::of(&QComboBox::activated), [=](int v)
    {
        inActiondata.bypass = bypassBox->itemData(v).toInt();

        for(int n = 0; n < bypassBox->count(); n++) {
            if(bypassBox->itemData(n).toInt() == inActiondata.bypass) {
                bypassBox->setCurrentIndex(n);
                break;
            }
        }

        saveActionSettings(inActiondata, _index, _number);

    });


    connect(mindial, SIGNAL(valueChanged(int)), minval_label, SLOT(setNum(int)));
    connect(maxdial, SIGNAL(valueChanged(int)), maxval_label, SLOT(setNum(int)));
    connect(mindialBPM, SIGNAL(valueChanged(int)), minval_labelBPM, SLOT(setNum(int)));
    connect(maxdialBPM, SIGNAL(valueChanged(int)), maxval_labelBPM, SLOT(setNum(int)));


    connect(lev0dial, SIGNAL(valueChanged(int)), lev0val_label, SLOT(setNum(int)));
    connect(lev1dial, SIGNAL(valueChanged(int)), lev1val_label, SLOT(setNum(int)));
    connect(lev2dial, SIGNAL(valueChanged(int)), lev2val_label, SLOT(setNum(int)));


    connect(groupActionBox, QOverload<bool>::of(&QGroupBox::toggled), [=](bool f)
    {
        inActiondata.status &= 0xfe;
        if(f) {
            groupActionBox->setTitle("Action " + QString::number(_number));
            inActiondata.status|= 1;
        } else
            groupActionBox->setTitle("Action " + QString::number(_number) + "    (Data Locked)");


        saveActionSettings(inActiondata, _index, _number);
    });


    connect(mindial, QOverload<int>::of(&QDialE::valueChanged), [=](int v)
    {

        inActiondata.min = v;
        mindial->setValue(v);
        saveActionSettings(inActiondata, _index, _number);
    });

    connect(maxdial, QOverload<int>::of(&QDialE::valueChanged), [=](int v)
    {

        inActiondata.max = v;
        maxdial->setValue(v);
        saveActionSettings(inActiondata, _index, _number);

    });

    connect(mindialBPM, QOverload<int>::of(&QDialE::valueChanged), [=](int v)
    {

        inActiondata.min = v/5;
        mindialBPM->setValue(v);
        saveActionSettings(inActiondata, _index, _number);
    });

    connect(maxdialBPM, QOverload<int>::of(&QDialE::valueChanged), [=](int v)
    {

        inActiondata.max = v/5;
        maxdialBPM->setValue(v);
        saveActionSettings(inActiondata, _index, _number);
    });


    connect(lev0dial, QOverload<int>::of(&QDialE::valueChanged), [=](int v)
    {
        inActiondata.lev0 = v;
        lev0dial->setValue(inActiondata.lev0);
        saveActionSettings(inActiondata, _index, _number);
    });

    connect(lev1dial, QOverload<int>::of(&QDialE::valueChanged), [=](int v)
    {
        inActiondata.lev1 = v;
        lev1dial->setValue(inActiondata.lev1);
        saveActionSettings(inActiondata, _index, _number);
    });

    connect(lev2dial, QOverload<int>::of(&QDialE::valueChanged), [=](int v)
    {
        inActiondata.lev2 = v;
        lev2dial->setValue(inActiondata.lev2);
        saveActionSettings(inActiondata, _index, _number);
    });


    if((inActiondata.category == CATEGORY_SEQUENCER &&
         ((inActiondata.action >= SEQUENCER_SCALE_1_UP && inActiondata.action <= SEQUENCER_SCALE_ALL_UP) ||
          (inActiondata.action >= SEQUENCER_SCALE_1_DOWN && inActiondata.action <= SEQUENCER_SCALE_ALL_DOWN)))) {

        mindialBPM->setValue(inActiondata.min * 5);
        maxdialBPM->setValue(inActiondata.max * 5);
    } else {
        mindial->setValue(inActiondata.min);
        maxdial->setValue(inActiondata.max);
    }

    lev0dial->setValue(inActiondata.lev0);
    lev1dial->setValue(inActiondata.lev1);
    lev2dial->setValue(inActiondata.lev2);

    controlBox->setCurrentIndex(setControlList(controlBox, inActiondata, inActiondata.control_note));


    connect(functionBox, QOverload<int>::of(&QComboBox::activated), [=](int v)
    {
        inActiondata.function = functionBox->itemData(v).toInt();

        for(int n = 0; n < functionBox->count(); n++) {
            if(functionBox->itemData(n).toInt() == inActiondata.function) {
                functionBox->setCurrentIndex(n);
                break;
            }
        }

        saveActionSettings(inActiondata, _index, _number);

    });

    connect(controlBox, QOverload<int>::of(&QComboBox::activated), [=](int v)
    {
        inActiondata.control_note = controlBox->itemData(v).toInt();

        if(inActiondata.control_note < 0) {

            MidiInControl::_current_note = -1;
            MidiInControl::_current_ctrl = -1;

            MidiInControl::_current_chan = inActiondata.channel;

            if(MidiInControl::_current_chan == 17) {
                if(inActiondata.device < 0 || !(inActiondata.device & 1))
                    MidiInControl::_current_chan = MidiInControl::inchannelUp(inActiondata.device/2);
                else
                    MidiInControl::_current_chan = MidiInControl::inchannelDown(inActiondata.device/2);

                if(MidiInControl::_current_chan < 0)
                    MidiInControl::_current_chan = 16;
            }

            MidiInControl::_current_device = inActiondata.device;

            int ret = -1;
            if(inActiondata.event == EVENT_NOTE)
                ret = parent->MidiIn->get_key();
            if(inActiondata.event == EVENT_CONTROL)
                ret = parent->MidiIn->get_ctrl();
            if(ret >= 0) {

                if(inActiondata.channel == 16) {
                    inActiondata.channel = MidiInControl::_current_chan;
                    for(int n = 0; n < channelBox->count(); n++) {
                        if(channelBox->itemData(n).toInt() == inActiondata.channel) {
                            channelBox->setCurrentIndex(n);
                            break;
                        }
                    }
                }
                inActiondata.control_note =  ret;
                for(int n = 0; n < controlBox->count(); n++) {
                    if(controlBox->itemData(n).toInt() == inActiondata.control_note) {

                        v = n;
                        break;
                    }
                }

            }
        }

        controlBox->setCurrentIndex(v);


        saveActionSettings(inActiondata, _index, _number);

    });

    connect(eventBox, QOverload<int>::of(&QComboBox::activated), [=](int v)
    {

        inActiondata.event = eventBox->itemData(v).toInt();
        eventBox->setCurrentIndex(v);

        controlBox->setCurrentIndex(setControlList(controlBox, inActiondata, inActiondata.control_note));

        emit functionBox->activated(setFunctionList(functionBox, inActiondata, inActiondata.function));

        saveActionSettings(inActiondata, _index, _number);
    });

    connect(channelBox, QOverload<int>::of(&QComboBox::activated), [=](int v)
    {
        inActiondata.channel = channelBox->itemData(v).toInt();
        channelBox->setCurrentIndex(v);

        saveActionSettings(inActiondata, _index, _number);
    });

    connect(deviceBox, QOverload<int>::of(&QComboBox::activated), [=](int v)
    {
        inActiondata.device = deviceBox->itemData(v).toInt();

        emit channelBox->activated(1);

        for(int n = 0; n < categoryBox->count(); n++) {
            if(categoryBox->itemData(n).toInt() == inActiondata.category) {

                emit categoryBox->activated(n);
                break;
            }
        }

        saveActionSettings(inActiondata, _index, _number);

    });


    if(inActiondata.category == CATEGORY_VST1 || inActiondata.category == CATEGORY_VST2) { // VST
        actionBox->setGeometry(QRect(309, 90, /*430*/210, 22));
        actionlabel2->setVisible(true);
        actionBox2->setVisible(true);
        bypassBox->setGeometry(QRect(309 + 220, 50, 124, 22));

    } else {
        actionBox->setGeometry(QRect(309, 90, 430 - 130, 22));
        actionlabel2->setVisible(false);
        actionBox2->setVisible(false);
        bypassBox->setGeometry(QRect(609 + 6, 90, 124, 22));
    }

    connect(actionBox, QOverload<int>::of(&QComboBox::activated), [=](int v)
    {
        int old = inActiondata.action;
        inActiondata.action = actionBox->itemData(v).toInt();
        actionBox->setCurrentIndex(v);

        if(inActiondata.category == CATEGORY_AUTOCHORD) { // autochord

            minlabel->setVisible(false);
            mindial->setVisible(false);
            minval_label->setVisible(false);
            maxlabel->setVisible(false);
            maxdial->setVisible(false);
            maxval_label->setVisible(false);

            minlabelBPM->setVisible(false);
            mindialBPM->setVisible(false);
            minval_labelBPM->setVisible(false);
            maxlabelBPM->setVisible(false);
            maxdialBPM->setVisible(false);
            maxval_labelBPM->setVisible(false);

            lev0label->setVisible(true);
            lev0dial->setVisible(true);
            lev0val_label->setVisible(true);
            lev1label->setVisible(true);
            lev1dial->setVisible(true);
            lev1val_label->setVisible(true);
            lev2label->setVisible(true);
            lev2dial->setVisible(true);
            lev2dial->setDisabled(true);
            lev2val_label->setVisible(true);
            lev0dial->setValue(inActiondata.lev0);
            lev1dial->setValue(inActiondata.lev1);
            lev0dial->setValue(inActiondata.lev2);

        } else if((inActiondata.category == CATEGORY_SEQUENCER &&
                  ((inActiondata.action >= SEQUENCER_SCALE_1_UP && inActiondata.action <= SEQUENCER_SCALE_ALL_UP) ||
                   (inActiondata.action >= SEQUENCER_SCALE_1_DOWN && inActiondata.action <= SEQUENCER_SCALE_ALL_DOWN)))) {

            minlabel->setVisible(false);
            mindial->setVisible(false);
            minval_label->setVisible(false);
            maxlabel->setVisible(false);
            maxdial->setVisible(false);
            maxval_label->setVisible(false);

            minlabelBPM->setVisible(true);
            mindialBPM->setVisible(true);
            minval_labelBPM->setVisible(true);
            maxlabelBPM->setVisible(true);
            maxdialBPM->setVisible(true);
            maxval_labelBPM->setVisible(true);
            mindialBPM->setValue(inActiondata.min * 5);
            maxdialBPM->setValue(inActiondata.max * 5);

            lev0label->setVisible(false);
            lev0dial->setVisible(false);
            lev0val_label->setVisible(false);
            lev1label->setVisible(false);
            lev1dial->setVisible(false);
            lev1val_label->setVisible(false);
            lev2label->setVisible(false);
            lev2dial->setVisible(false);
            lev2val_label->setVisible(false);

        } else {

            if(inActiondata.category == CATEGORY_SWITCH) {

                minlabel->setVisible(false);
                mindial->setVisible(false);
                minval_label->setVisible(false);
                maxlabel->setVisible(false);
                maxdial->setVisible(false);
                maxval_label->setVisible(false);
                mindial->setValue(inActiondata.min);
                maxdial->setValue(inActiondata.max);

            } else {
                minlabel->setVisible(true);
                mindial->setVisible(true);
                minval_label->setVisible(true);
                maxlabel->setVisible(true);
                maxdial->setVisible(true);
                maxval_label->setVisible(true);
                mindial->setValue(inActiondata.min);
                maxdial->setValue(inActiondata.max);
            }

            minlabelBPM->setVisible(false);
            mindialBPM->setVisible(false);
            minval_labelBPM->setVisible(false);
            maxlabelBPM->setVisible(false);
            maxdialBPM->setVisible(false);
            maxval_labelBPM->setVisible(false);

            lev0label->setVisible(false);
            lev0dial->setVisible(false);
            lev0val_label->setVisible(false);
            lev1label->setVisible(false);
            lev1dial->setVisible(false);
            lev1val_label->setVisible(false);
            lev2label->setVisible(false);
            lev2dial->setVisible(false);
            lev2val_label->setVisible(false);
        }

        if((inActiondata.category == CATEGORY_SEQUENCER &&
             ((inActiondata.action >= SEQUENCER_ON_1_UP && inActiondata.action <= SEQUENCER_ON_4_UP) ||
              (inActiondata.action >= SEQUENCER_ON_1_DOWN && inActiondata.action <= SEQUENCER_ON_4_DOWN))) ||
            (inActiondata.category == CATEGORY_FINGERPATTERN  &&
             ((inActiondata.action >= FINGERPATTERN_PICK_UP && inActiondata.action <= FINGERPATTERN_3_UP) ||
              (inActiondata.action >= FINGERPATTERN_PICK_DOWN && inActiondata.action <= FINGERPATTERN_1_DOWN))) ||
            inActiondata.category == CATEGORY_VST1 ||
            inActiondata.category == CATEGORY_VST2) {
            minlabel->setVisible(false);
            mindial->setVisible(false);
            minval_label->setVisible(false);
            maxlabel->setVisible(false);
            maxdial->setVisible(false);
            maxval_label->setVisible(false);
        }


        if(old != inActiondata.action && inActiondata.category == CATEGORY_MIDI_EVENTS &&
            (inActiondata.action == ACTION_PITCHBEND_UP || inActiondata.action == ACTION_MODULATION_WHEEL_UP ||
             inActiondata.action == ACTION_PITCHBEND_DOWN || inActiondata.action == ACTION_MODULATION_WHEEL_DOWN))
            mindial->setValue(64);

        if(inActiondata.category == CATEGORY_VST1 || inActiondata.category == CATEGORY_VST2) {
            if(inActiondata.action & 32) {
                if(!(inActiondata.action2 & 32))
                    inActiondata.action2 = inActiondata.action;
            } else {
                if(inActiondata.action2 & 32)
                    inActiondata.action2 = inActiondata.action;
            }
        }

        emit functionBox->activated(setFunctionList(functionBox, inActiondata, inActiondata.function));

        if(inActiondata.category == CATEGORY_VST1 || inActiondata.category == CATEGORY_VST2)
            emit actionBox2->activated(setAction2List(actionBox2, inActiondata, inActiondata.action2));

        saveActionSettings(inActiondata, _index, _number);

    });

    connect(actionBox2, QOverload<int>::of(&QComboBox::activated), [=](int v)
    {
        inActiondata.action2 = actionBox2->itemData(v).toInt();
        actionBox2->setCurrentIndex(v);

        saveActionSettings(inActiondata, _index, _number);

    });


    connect(categoryBox, QOverload<int>::of(&QComboBox::activated), [=](int v)
    {
        inActiondata.category = v;

        categoryBox->setCurrentIndex(inActiondata.category);

        eventBox->clear();
        eventBox->addItem("Note Event", EVENT_NOTE);
        eventBox->addItem("Control Event", EVENT_CONTROL);

        if(inActiondata.category == CATEGORY_SWITCH) {
            if(inActiondata.event > EVENT_CONTROL)
                inActiondata.event = EVENT_NOTE;
        } else {
            eventBox->addItem("Sustain Pedal", EVENT_SUSTAIN);
            eventBox->addItem("Expression Pedal", EVENT_EXPRESSION);
            eventBox->addItem("Sustain Pedal Inv", EVENT_SUSTAIN_INV);
            eventBox->addItem("Expression Pedal Inv", EVENT_EXPRESSION_INV);
            eventBox->addItem("Aftertouch", EVENT_AFTERTOUCH);
            eventBox->addItem("Modulation Wheel", EVENT_MODULATION_WHEEL);
            eventBox->addItem("Pitch Bend", EVENT_PITCH_BEND);
        }

        for(int n = 0; n < eventBox->count(); n++) {
            if(eventBox->itemData(n).toInt() == inActiondata.event) {
                eventBox->setCurrentIndex(n);
                break;
            }
        }


        if(inActiondata.category == CATEGORY_VST1 || inActiondata.category == CATEGORY_VST2) { // VST
            actionBox->setGeometry(QRect(309, 90, /*430*/210, 22));
            actionlabel2->setVisible(true);
            actionBox2->setVisible(true);
            bypassBox->setGeometry(QRect(309 + 220, 50, 124, 22));

        } else {
            actionBox->setGeometry(QRect(309, 90, 430 - 130, 22));
            actionlabel2->setVisible(false);
            actionBox2->setVisible(false);
            bypassBox->setGeometry(QRect(609 + 6, 90, 124, 22));
        }

        emit actionBox->activated(setActionList(actionBox, inActiondata, inActiondata.action));

        if(inActiondata.category == CATEGORY_VST1 || inActiondata.category == CATEGORY_VST2)
            emit actionBox2->activated(setAction2List(actionBox2, inActiondata, inActiondata.action2));

        if(inActiondata.category == CATEGORY_AUTOCHORD) { // autochord

            minlabel->setVisible(false);
            mindial->setVisible(false);
            minval_label->setVisible(false);
            maxlabel->setVisible(false);
            maxdial->setVisible(false);
            maxval_label->setVisible(false);

            minlabelBPM->setVisible(false);
            mindialBPM->setVisible(false);
            minval_labelBPM->setVisible(false);
            maxlabelBPM->setVisible(false);
            maxdialBPM->setVisible(false);
            maxval_labelBPM->setVisible(false);

            lev0label->setVisible(true);
            lev0dial->setVisible(true);
            lev0val_label->setVisible(true);
            lev1label->setVisible(true);
            lev1dial->setVisible(true);
            lev1val_label->setVisible(true);
            lev2label->setVisible(true);
            lev2dial->setVisible(true);
            lev2dial->setDisabled(true);
            lev2val_label->setVisible(true);
            lev0dial->setValue(inActiondata.lev0);
            lev1dial->setValue(inActiondata.lev1);
            lev0dial->setValue(inActiondata.lev2);

        } else if((inActiondata.category == CATEGORY_SEQUENCER &&
                  ((inActiondata.action >= SEQUENCER_SCALE_1_UP && inActiondata.action <= SEQUENCER_SCALE_ALL_UP) ||
                   (inActiondata.action >= SEQUENCER_SCALE_1_DOWN && inActiondata.action <= SEQUENCER_SCALE_ALL_DOWN)))) {

            minlabel->setVisible(false);
            mindial->setVisible(false);
            minval_label->setVisible(false);
            maxlabel->setVisible(false);
            maxdial->setVisible(false);
            maxval_label->setVisible(false);

            minlabelBPM->setVisible(true);
            mindialBPM->setVisible(true);
            minval_labelBPM->setVisible(true);
            maxlabelBPM->setVisible(true);
            maxdialBPM->setVisible(true);
            maxval_labelBPM->setVisible(true);
            mindialBPM->setValue(inActiondata.min * 5);
            maxdialBPM->setValue(inActiondata.max * 5);

            lev0label->setVisible(false);
            lev0dial->setVisible(false);
            lev0val_label->setVisible(false);
            lev1label->setVisible(false);
            lev1dial->setVisible(false);
            lev1val_label->setVisible(false);
            lev2label->setVisible(false);
            lev2dial->setVisible(false);
            lev2val_label->setVisible(false);
        } else {

            if(inActiondata.category == CATEGORY_SWITCH) {

                minlabel->setVisible(false);
                mindial->setVisible(false);
                minval_label->setVisible(false);
                maxlabel->setVisible(false);
                maxdial->setVisible(false);
                maxval_label->setVisible(false);
                mindial->setValue(inActiondata.min);
                maxdial->setValue(inActiondata.max);

            } else {

                minlabel->setVisible(true);
                mindial->setVisible(true);
                minval_label->setVisible(true);
                maxlabel->setVisible(true);
                maxdial->setVisible(true);
                maxval_label->setVisible(true);
                mindial->setValue(inActiondata.min);;
                maxdial->setValue(inActiondata.max);
            }

            minlabelBPM->setVisible(false);
            mindialBPM->setVisible(false);
            minval_labelBPM->setVisible(false);
            maxlabelBPM->setVisible(false);
            maxdialBPM->setVisible(false);
            maxval_labelBPM->setVisible(false);

            lev0label->setVisible(false);
            lev0dial->setVisible(false);
            lev0val_label->setVisible(false);
            lev1label->setVisible(false);
            lev1dial->setVisible(false);
            lev1val_label->setVisible(false);
            lev2label->setVisible(false);
            lev2dial->setVisible(false);
            lev2val_label->setVisible(false);
        }

        if((inActiondata.category == CATEGORY_SEQUENCER &&
             ((inActiondata.action >= SEQUENCER_ON_1_UP && inActiondata.action <= SEQUENCER_ON_4_UP) ||
              (inActiondata.action >= SEQUENCER_ON_1_DOWN && inActiondata.action <= SEQUENCER_ON_4_DOWN))) ||
            (inActiondata.category == CATEGORY_FINGERPATTERN  &&
             ((inActiondata.action >= FINGERPATTERN_PICK_UP && inActiondata.action <= FINGERPATTERN_3_UP) ||
              (inActiondata.action >= FINGERPATTERN_PICK_DOWN && inActiondata.action <= FINGERPATTERN_1_DOWN))) ||
            inActiondata.category == CATEGORY_VST1 ||
            inActiondata.category == CATEGORY_VST2) {
            minlabel->setVisible(false);
            mindial->setVisible(false);
            minval_label->setVisible(false);
            maxlabel->setVisible(false);
            maxdial->setVisible(false);
            maxval_label->setVisible(false);
        }

        saveActionSettings(inActiondata, _index, _number);
    });

    if(inActiondata.category == CATEGORY_VST1 || inActiondata.category == CATEGORY_VST2) { // VST
        actionBox->setGeometry(QRect(309, 90, /*430*/210, 22));
        actionlabel2->setVisible(true);
        actionBox2->setVisible(true);
    } else {
        actionBox->setGeometry(QRect(309, 90, 430 - 130, 22));
        actionlabel2->setVisible(false);
        actionBox2->setVisible(false);
    }

    emit actionBox->activated(setActionList(actionBox, inActiondata, inActiondata.action));

    if(inActiondata.category == CATEGORY_VST1 || inActiondata.category == CATEGORY_VST2)
        emit actionBox2->activated(setAction2List(actionBox2, inActiondata, inActiondata.action2));


    if(inActiondata.category == CATEGORY_AUTOCHORD) { // autochord

        minlabel->setVisible(false);
        mindial->setVisible(false);
        minval_label->setVisible(false);
        maxlabel->setVisible(false);
        maxdial->setVisible(false);
        maxval_label->setVisible(false);

        minlabelBPM->setVisible(false);
        mindialBPM->setVisible(false);
        minval_labelBPM->setVisible(false);
        maxlabelBPM->setVisible(false);
        maxdialBPM->setVisible(false);
        maxval_labelBPM->setVisible(false);

        lev0label->setVisible(true);
        lev0dial->setVisible(true);
        lev0val_label->setVisible(true);
        lev1label->setVisible(true);
        lev1dial->setVisible(true);
        lev1val_label->setVisible(true);
        lev2label->setVisible(true);
        lev2dial->setVisible(true);
        lev2dial->setDisabled(true);
        lev2val_label->setVisible(true);
        lev0dial->setValue(inActiondata.lev0);
        lev1dial->setValue(inActiondata.lev1);
        lev0dial->setValue(inActiondata.lev2);

    } else if((inActiondata.category == CATEGORY_SEQUENCER &&
              ((inActiondata.action >= SEQUENCER_SCALE_1_UP && inActiondata.action <= SEQUENCER_SCALE_ALL_UP) ||
               (inActiondata.action >= SEQUENCER_SCALE_1_DOWN && inActiondata.action <= SEQUENCER_SCALE_ALL_DOWN)))) {

        minlabel->setVisible(false);
        mindial->setVisible(false);
        minval_label->setVisible(false);
        maxlabel->setVisible(false);
        maxdial->setVisible(false);
        maxval_label->setVisible(false);

        minlabelBPM->setVisible(true);
        mindialBPM->setVisible(true);
        minval_labelBPM->setVisible(true);
        maxlabelBPM->setVisible(true);
        maxdialBPM->setVisible(true);
        maxval_labelBPM->setVisible(true);
        mindialBPM->setValue(inActiondata.min * 5);
        maxdialBPM->setValue(inActiondata.max * 5);

        lev0label->setVisible(false);
        lev0dial->setVisible(false);
        lev0val_label->setVisible(false);
        lev1label->setVisible(false);
        lev1dial->setVisible(false);
        lev1val_label->setVisible(false);
        lev2label->setVisible(false);
        lev2dial->setVisible(false);
        lev2val_label->setVisible(false);
    } else {

        if(inActiondata.category == CATEGORY_SWITCH) {

            minlabel->setVisible(false);
            mindial->setVisible(false);
            minval_label->setVisible(false);
            maxlabel->setVisible(false);
            maxdial->setVisible(false);
            maxval_label->setVisible(false);
            mindial->setValue(inActiondata.min);
            maxdial->setValue(inActiondata.max);

        } else {

            minlabel->setVisible(true);
            mindial->setVisible(true);
            minval_label->setVisible(true);
            maxlabel->setVisible(true);
            maxdial->setVisible(true);
            maxval_label->setVisible(true);
            mindial->setValue(inActiondata.min);
            maxdial->setValue(inActiondata.max);
        }

        minlabelBPM->setVisible(false);
        mindialBPM->setVisible(false);
        minval_labelBPM->setVisible(false);
        maxlabelBPM->setVisible(false);
        maxdialBPM->setVisible(false);
        maxval_labelBPM->setVisible(false);

        lev0label->setVisible(false);
        lev0dial->setVisible(false);
        lev0val_label->setVisible(false);
        lev1label->setVisible(false);
        lev1dial->setVisible(false);
        lev1val_label->setVisible(false);
        lev2label->setVisible(false);
        lev2dial->setVisible(false);
        lev2val_label->setVisible(false);
    }

    if((inActiondata.category == CATEGORY_SEQUENCER &&
         ((inActiondata.action >= SEQUENCER_ON_1_UP && inActiondata.action <= SEQUENCER_ON_4_UP) ||
          (inActiondata.action >= SEQUENCER_ON_1_DOWN && inActiondata.action <= SEQUENCER_ON_4_DOWN))) ||
        (inActiondata.category == CATEGORY_FINGERPATTERN  &&
         ((inActiondata.action >= FINGERPATTERN_PICK_UP && inActiondata.action <= FINGERPATTERN_3_UP) ||
          (inActiondata.action >= FINGERPATTERN_PICK_DOWN && inActiondata.action <= FINGERPATTERN_1_DOWN))) ||

            inActiondata.category == CATEGORY_VST1 ||
        inActiondata.category == CATEGORY_VST2) {

        minlabel->setVisible(false);
        mindial->setVisible(false);
        minval_label->setVisible(false);
        maxlabel->setVisible(false);
        maxdial->setVisible(false);
        maxval_label->setVisible(false);
    }

    groupActionBox->setChecked((inActiondata.status & 1) != 0);

}

int InputActionItem::setFunctionList(QComboBox *box, InputActionData &inActiondata, int val) {
    val = 0; // default
    switch(inActiondata.category) {
    case CATEGORY_AUTOCHORD: // Autochord
    case CATEGORY_VST1: // VST Control 1
    case CATEGORY_VST2: // VST Control 2
    case CATEGORY_SWITCH:
        if(box) {
            box->clear();
            box->addItem("Button", 2);
            box->addItem("Switch", 3);

            for(int n = 0; n < box->count(); n++) {
                if(inActiondata.function == box->itemData(n).toInt()) {

                    val = n;
                    break;

                }

            }
        }
        return val;

    default:

        val = 0;

        if(box) {
            box->clear();

            if(inActiondata.event == EVENT_SUSTAIN || inActiondata.event == EVENT_SUSTAIN_INV) { // sustain pedal

                box->addItem("Button", 2);
                box->addItem("Switch", 3);

            } else {

                box->addItem("Ev. Val ", 0);
                box->addItem("Ev. Val Clip", 1);
                box->addItem("Button", 2);
                box->addItem("Switch", 3);

            }

            for(int n = 0; n < box->count(); n++) {
                if(inActiondata.function == box->itemData(n).toInt()) {

                    val = n;
                    break;

                }

            }
        }
        return val;
    }
}

int InputActionItem::setControlList(QComboBox *box, InputActionData &inActiondata, int val) {

    switch(inActiondata.event) {
    case EVENT_NOTE: // NOTE
        val = 0; // default
        if(box) {
            box->clear();
            box->addItem("Get It", -1);
            if(inActiondata.control_note == -1)
                val = 0;
            for(int n = 0; n < 128; n++) {
                if(inActiondata.control_note == n)
                    val = n + 1;
                box->addItem(QString::asprintf("%s %i", MidiInControl::notes[n % 12], n / 12 - 1), n);
            }


        }
        return val;

    case EVENT_CONTROL: // CONTROL
        val = 0; // default
        if(box) {
            box->clear();
            box->addItem("Get It", -1);
            if(inActiondata.control_note == -1)
                val = 0;
            for(int n = 0; n < 128; n++) {
                if(inActiondata.control_note == n)
                    val = n + 1;
                box->addItem(QString::asprintf("CC %i - ", n) + MidiFile::controlChangeName(n), n);
            }

        }
        return val;


    default: // PEDALS, EVENT_MODULATION_WHEEL, EVENT_PITCH_BEND

        val = 0;

        if(box) {
            box->clear();
            box->addItem("No Switch", -1);
            if(inActiondata.control_note == -1)
                val = 0;
            for(int n = 0; n < 32; n++) {
                if(inActiondata.control_note == n)
                    val = n + 1;
                box->addItem("Use Switch " + QString::number(n), n);
            }
        }

        return val;
    }
}

int InputActionItem::setActionList(QComboBox *box, InputActionData &inActiondata, int val) {

    val = 0; // default

    switch(inActiondata.category) {
        case CATEGORY_MIDI_EVENTS: // Midi Events
            if(box) {
                box->clear();

                if(inActiondata.device < 0 || !(inActiondata.device & 1)) {
                    box->addItem("Pitch Bend - UP", 0);
                    box->addItem("Modulation Wheel - UP", 1);
                    box->addItem("Sustain - UP", 2);
                    box->addItem("Sostenuto - UP", 3);
                    box->addItem("Reverb level - UP", 4);
                    box->addItem("Chorus level - UP", 5);

                    box->addItem("Program Change - UP", 6);
                    box->addItem("Channel Volume Control - UP", 7);
                    box->addItem("Pan Control - UP", 8);

                    box->addItem("Attack Control - UP", 9);
                    box->addItem("Release Control - UP", 10);
                    box->addItem("Decay Control - UP", 11);
                }

                box->addItem("Pitch Bend - DOWN", 32);
                box->addItem("Modulation Wheel - DOWN", 33);
                box->addItem("Sustain - DOWN", 34);
                box->addItem("Sostenuto - DOWN", 35);
                box->addItem("Reverb level - DOWN", 36);
                box->addItem("Chorus level - DOWN", 37);

                box->addItem("Program Change - DOWN", 38);
                box->addItem("Channel Volume Control - DOWN", 39);
                box->addItem("Pan Control - DOWN", 40);

                box->addItem("Attack Control - DOWN", 41);
                box->addItem("Release Control - DOWN", 42);
                box->addItem("Decay Control - DOWN", 43);

                for(int n = 0; n < box->count(); n++) {
                    if(inActiondata.action == box->itemData(n).toInt()) {

                        val = n;
                        break;

                    }

                }
            }
            return val;

        case CATEGORY_AUTOCHORD: // Autochord
            if(box) {
                box->clear();

                if(inActiondata.device < 0 || !(inActiondata.device & 1)) {
                    box->addItem("Power Chord - UP", 0);
                    box->addItem("Power Chord Extended - UP", 1);
                    box->addItem("C Major Chord (CM) - UP", 2);
                    box->addItem("C Major Chord Progression (CM) - UP", 3);
                }

                box->addItem("Power Chord - DOWN", 32);
                box->addItem("Power Chord Extended - DOWN", 33);
                box->addItem("C Major Chord (CM) - DOWN", 34);
                box->addItem("C Major Chord Progression (CM) - DOWN", 35);

                for(int n = 0; n < box->count(); n++) {
                    if(inActiondata.action == box->itemData(n).toInt()) {

                        val = n;
                        break;

                    }

                }
            }
            return val;

        case CATEGORY_FLUIDSYNTH: // Fluidsynth Control
            if(box) {
                box->clear();
                box->addItem("Mixer Chan: Wah-Wah Control (internally uses CC: 9)", 31);
                box->addItem("Mixer Chan: Leslie On/Off (internally uses CC: 85)", 32);
                box->addItem("Mixer Chan: Leslie Deep Bass (internally uses CC: 86)", 33);
                box->addItem("Mixer Chan: Leslie Rot Bass (internally uses CC: 87)", 34);
                box->addItem("Mixer Chan: Leslie Freq Cut (internally uses CC: 88)", 35);
                box->addItem("Mixer Chan: Leslie Deep Treble (internally uses CC: 89)", 36);
                box->addItem("Mixer Chan: Leslie Rot Treble (internally uses CC: 90)", 37);

                box->addItem("Midi Chan: Volume (internally uses CC: 20 & CC: 7)", 20);
                box->addItem("Mixer Chan: Pan (internally uses CC: 21)", 21);
                box->addItem("Mixer Chan: Gain (internally uses CC: 22)", 22);
                box->addItem("Mixer Chan: Distortion Level (internally uses CC: 23)", 23);
                box->addItem("Mixer Chan: Low Cut Gain (internally uses CC: 24)", 24);
                box->addItem("Mixer Chan: Low Cut Frequency (internally uses CC: 25)", 25);
                box->addItem("Mixer Chan: High Cut Gain (internally uses CC: 26)", 26);
                box->addItem("Mixer Chan: High Cut Frequency (internally uses CC: 27)", 27);
                box->addItem("Mixer Chan: Low Cut Resonance (internally uses CC: 28)", 28);
                box->addItem("Mixer Chan: High Cut Resonance (internally uses CC: 29)", 29);
                box->addItem("Mixer Main: Main Volume (internally uses CC: 30)", 30);

                for(int n = 0; n < box->count(); n++) {
                    if(inActiondata.action == box->itemData(n).toInt()) {

                        val = n;
                        break;

                    }

                }
            }
            return val;

        case CATEGORY_VST1: // VST Control 1
            if(box) {
                box->clear();

                if(inActiondata.device < 0 || !(inActiondata.device & 1)) {
                    box->addItem("VST1: Disable Preset - UP", 0);
                    for(int n = 0; n < 8; n++)
                        box->addItem("VST1: Set Preset " + QString::number(n) + " - UP", n + 1);

                }

                box->addItem("VST1: Disable Preset - DOWN", 32);
                for(int n = 0; n < 8; n++)
                    box->addItem("VST1: Set Preset " + QString::number(n) + " - DOWN", 33 + n);

                for(int n = 0; n < box->count(); n++) {
                    if(inActiondata.action == box->itemData(n).toInt()) {

                        val = n;
                        break;

                    }

                }
            }

            return val;

        case CATEGORY_VST2: // VST Control 2
            if(box) {
                box->clear();
                if(inActiondata.device < 0 || !(inActiondata.device & 1)) {
                    box->addItem("VST2: Disable Preset - UP", 0);
                    for(int n = 0; n < 8; n++)
                        box->addItem("VST2: Set Preset " + QString::number(n) + " - UP", n + 1);

                }

                box->addItem("VST2: Disable Preset - DOWN", 32);
                for(int n = 0; n < 8; n++)
                    box->addItem("VST2: Set Preset " + QString::number(n) + " - DOWN", 33 + n);

                for(int n = 0; n < box->count(); n++) {
                    if(inActiondata.action == box->itemData(n).toInt()) {

                        val = n;
                        break;

                    }

                }
            }

            return val;

        case CATEGORY_SEQUENCER: // SEQUENCER
            if(box) {
                box->clear();

                if(inActiondata.device < 0 || !(inActiondata.device & 1)) {
                    box->addItem("SEQUENCER ON/OFF #1 - UP", SEQUENCER_ON_1_UP);
                    box->addItem("SEQUENCER ON/OFF #2 - UP", SEQUENCER_ON_2_UP);
                    box->addItem("SEQUENCER ON/OFF #3 - UP", SEQUENCER_ON_3_UP);
                    box->addItem("SEQUENCER ON/OFF #4 - UP", SEQUENCER_ON_4_UP);

                    box->addItem("SEQUENCER TIME SCALE #1 - UP", SEQUENCER_SCALE_1_UP);
                    box->addItem("SEQUENCER TIME SCALE #2 - UP", SEQUENCER_SCALE_2_UP);
                    box->addItem("SEQUENCER TIME SCALE #3 - UP", SEQUENCER_SCALE_3_UP);
                    box->addItem("SEQUENCER TIME SCALE #4 - UP", SEQUENCER_SCALE_4_UP);
                    box->addItem("SEQUENCER TIME SCALE ALL - UP", SEQUENCER_SCALE_ALL_UP);

                    box->addItem("SEQUENCER VOLUME #1 - UP", SEQUENCER_VOLUME_1_UP);
                    box->addItem("SEQUENCER VOLUME #2 - UP", SEQUENCER_VOLUME_2_UP);
                    box->addItem("SEQUENCER VOLUME #3 - UP", SEQUENCER_VOLUME_3_UP);
                    box->addItem("SEQUENCER VOLUME #4 - UP", SEQUENCER_VOLUME_4_UP);
                    box->addItem("SEQUENCER VOLUME ALL - UP", SEQUENCER_VOLUME_ALL_UP);
                }

                box->addItem("SEQUENCER ON/OFF #1 - DOWN", SEQUENCER_ON_1_DOWN);
                box->addItem("SEQUENCER ON/OFF #2 - DOWN", SEQUENCER_ON_2_DOWN);
                box->addItem("SEQUENCER ON/OFF #3 - DOWN", SEQUENCER_ON_3_DOWN);
                box->addItem("SEQUENCER ON/OFF #4 - DOWN", SEQUENCER_ON_4_DOWN);

                box->addItem("SEQUENCER TIME SCALE #1 - DOWN", SEQUENCER_SCALE_1_DOWN);
                box->addItem("SEQUENCER TIME SCALE #2 - DOWN", SEQUENCER_SCALE_2_DOWN);
                box->addItem("SEQUENCER TIME SCALE #3 - DOWN", SEQUENCER_SCALE_3_DOWN);
                box->addItem("SEQUENCER TIME SCALE #4 - DOWN", SEQUENCER_SCALE_4_DOWN);
                box->addItem("SEQUENCER TIME SCALE ALL - DOWN", SEQUENCER_SCALE_ALL_DOWN);

                box->addItem("SEQUENCER VOLUME #1 - DOWN", SEQUENCER_VOLUME_1_DOWN);
                box->addItem("SEQUENCER VOLUME #2 - DOWN", SEQUENCER_VOLUME_2_DOWN);
                box->addItem("SEQUENCER VOLUME #3 - DOWN", SEQUENCER_VOLUME_3_DOWN);
                box->addItem("SEQUENCER VOLUME #4 - DOWN", SEQUENCER_VOLUME_4_DOWN);
                box->addItem("SEQUENCER VOLUME ALL - DOWN", SEQUENCER_VOLUME_ALL_DOWN);

                for(int n = 0; n < box->count(); n++) {
                    if(inActiondata.action == box->itemData(n).toInt()) {
                        val = n;
                        break;
                    }
                }
            }

            return val;

        case CATEGORY_FINGERPATTERN: // FINGER PATTERN
            if(box) {
                box->clear();

                if(inActiondata.device < 0 || !(inActiondata.device & 1)) {
                    box->addItem("FINGER PATTERN PICK ON/OFF - UP", FINGERPATTERN_PICK_UP);
                    box->addItem("FINGER PATTERN ON/OFF #1 - UP", FINGERPATTERN_1_UP);
                    box->addItem("FINGER PATTERN ON/OFF #2 - UP", FINGERPATTERN_2_UP);
                    box->addItem("FINGER PATTERN ON/OFF #3 - UP", FINGERPATTERN_3_UP);
                    box->addItem("FINGER PATTERN SCALE TIME - UP", FINGERPATTERN_SCALE_UP);
                }

                box->addItem("FINGER PATTERN PICK ON/OFF - DOWN", FINGERPATTERN_PICK_DOWN);
                box->addItem("FINGER PATTERN ON/OFF #1 - DOWN", FINGERPATTERN_1_DOWN);
                box->addItem("FINGER PATTERN ON/OFF #2 - DOWN", FINGERPATTERN_2_DOWN);
                box->addItem("FINGER PATTERN ON/OFF #3 - DOWN", FINGERPATTERN_3_DOWN);
                box->addItem("FINGER PATTERN SCALE TIME - DOWN", FINGERPATTERN_SCALE_DOWN);

                for(int n = 0; n < box->count(); n++) {
                    if(inActiondata.action == box->itemData(n).toInt()) {

                        val = n;
                        break;

                    }

                }
            }

            return val;

        case CATEGORY_SWITCH: // Autochord
            if(box) {
                box->clear();

                for(int n = 0; n < 10; n++) {
                    box->addItem("Switch " + QString::number(n), n);
                }

                for(int n = 0; n < box->count(); n++) {
                    if(inActiondata.action == box->itemData(n).toInt()) {

                        val = n;
                        break;

                    }

                }
            }
            return val;

        case CATEGORY_GENERAL:
            if(box) {
                box->clear();

                box->addItem("Play / Stop", 0);
                box->addItem("Record / Stop", 1);
                box->addItem("Record from (2 secs) / Stop", 2);
                box->addItem("Stop", 3);
                box->addItem("Forward", 4);
                box->addItem("Back", 5);

                for(int n = 0; n < box->count(); n++) {
                    if(inActiondata.action == box->itemData(n).toInt()) {

                        val = n;
                        break;

                    }

                }
            }
            return val;

        default:

            val = 0;

            if(box) {
                box->clear();
            }
            return val;
    }
}

int InputActionItem::setAction2List(QComboBox *box, InputActionData &inActiondata, int val) {

    val = 0;

    switch(inActiondata.category) {

    case CATEGORY_VST1: // VST Control 1
        if(box) {
            box->clear();

            if((inActiondata.device < 0 || !(inActiondata.device & 1)) && !(inActiondata.action & 32)) {
                box->addItem("VST1: Disable Preset - UP", 0);
                for(int n = 0; n < 8; n++)
                    box->addItem("VST1: Set Preset " + QString::number(n) + " - UP", n + 1);

            }

            if(inActiondata.action & 32) {
                box->addItem("VST1: Disable Preset - DOWN", 32);
                for(int n = 0; n < 8; n++)
                    box->addItem("VST1: Set Preset " + QString::number(n) + " - DOWN", 33 + n);
            }

            if(inActiondata.action & 32) {
                if(!(inActiondata.action2 & 32))
                    inActiondata.action2 = inActiondata.action;
            } else {
                if(inActiondata.action2 & 32)
                    inActiondata.action2 = inActiondata.action;
            }


            val = 0;  // default

            for(int n = 0; n < box->count(); n++) {
                if(inActiondata.action2 == box->itemData(n).toInt()) {

                    val = n;
                    break;

                }

            }
        }

        return val;

    case CATEGORY_VST2: // VST Control 2
        if(box) {
            box->clear();
            if((inActiondata.device < 0 || !(inActiondata.device & 1)) && !(inActiondata.action & 32)) {
                box->addItem("VST2: Disable Preset - UP", 0);
                for(int n = 0; n < 8; n++)
                    box->addItem("VST2: Set Preset " + QString::number(n) + " - UP", n + 1);

            }

            if(inActiondata.action & 32) {
                box->addItem("VST2: Disable Preset - DOWN", 32);
                for(int n = 0; n < 8; n++)
                    box->addItem("VST2: Set Preset " + QString::number(n) + " - DOWN", 33 + n);
            }

            val = 0; // default

            for(int n = 0; n < box->count(); n++) {
                if(inActiondata.action2 == box->itemData(n).toInt()) {

                    val = n;
                    break;

                }

            }

        }

        return val;


    default:

        val = 0;

        if(box) {
            box->clear();
            val = 0; // default
        }
        return val;
    }
}

////////////////////////////////////////////////////////////////////////
// Sequencer Tab
////////////////////////////////////////////////////////////////////////

InputSequencerListWidget::InputSequencerListWidget(QWidget* parent, MidiInControl * MidiIn) : QListWidget(parent) {

    this->MidiIn = MidiIn;

    settings = new QSettings(QDir::homePath() + "/Midieditor/settings/sequencer.ini", QSettings::IniFormat);

    setAutoScroll(false);
    setSelectionMode(QAbstractItemView::NoSelection);
    setStyleSheet("QListWidget {background-color: #e0e0c0;}\n"
                  "QListWidget::item QGroupBox {font: bold; border-bottom: 2px solid blue;\n"
                  "border: 2px solid black; border-radius: 7px; margin-top: 1ex;}\n"
                  "QGroupBox::indicator:unchecked {image: url(:/run_environment/graphics/custom/unchecked.png);}\n"
                  "QGroupBox::indicator:checked {image: url(:/run_environment/graphics/custom/checked.png);}\n"
                  "QGroupBox QCheckBox::indicator:unchecked {image: url(:/run_environment/graphics/custom/unchecked.png);}\n"
                  "QGroupBox QCheckBox::indicator:checked {image: url(:/run_environment/graphics/custom/checked.png);}\n"
                  "QGroupBox::title {subcontrol-origin: margin; left: 12px;\n"
                  "subcontrol-position: top left; padding: 0 2px; }");

    updateList();

    connect(MidiInControl::osd_dialog, &OSDDialog::OSDtimeUpdate, this, [=]() {

        emit updateButtons();

    });

    connect(this, QOverload<QListWidgetItem*>::of(&QListWidget::itemClicked), [=](QListWidgetItem* i)
    {
        selected = i->data(Qt::UserRole).toInt();

        if(selected & 32)
            selected = 4 + (selected & 31);

        for(int n = 0; n < this->items.count(); n++) {
            if(n == selected)
                items.at(n)->is_selected = true;
            else
                items.at(n)->is_selected = false;
        }

        update();

    });

    QScrollBar * bar = this->verticalScrollBar();

           // scroll fix positions (UP/DOWN Sequencer areas)
    connect(bar, QOverload<int>::of(&QDial::valueChanged), [=](int num)
    {
        static int no_reentry = 0;
        if(!no_reentry) {
            no_reentry = 1;

            num*= 2;
            if(num >= 4)
                num = 4;
            else
                num = 0;

            bar->setValue(num);

            no_reentry = 0;
        }
    });

}

void InputSequencerListWidget::updateList() {

    items.clear();
    clear();

    pairdev = MidiInControl::cur_pairdev;

    selected = 0;

    for(int n = 0; n < 8; n++) {

        InputSequencerItem* widget = new InputSequencerItem(this, pairdev, (n >= 4) ? n + 28 : n);

        if(n == selected)
            widget->is_selected = true;
        else
            widget->is_selected = false;

        QListWidgetItem* item = new QListWidgetItem();
        item->setSizeHint(QSize(0, 140));
        item->setData(Qt::UserRole, (n >= 4) ? n + 28 : n);
        addItem(item);
        setItemWidget(item, widget);
        items.append(widget);

    }

}

void InputSequencerItem::paintEvent(QPaintEvent* event) {

    QWidget::paintEvent(event);

    // Estwald Color Changes
    QPainter *p = new QPainter(this);
    if(!p) return;

    p->fillRect(0, 0, width(), height(), background1);

    p->setPen(0x808080);
    p->drawLine(0, height() - 1, width(), height() - 1);

    if(is_selected) {
        QColor c(0x80ffff);
        c.setAlpha(32);
        p->fillRect(0, 0, width(), height() - 2, c);
    }

    delete p;
}

void InputSequencerItem::load_Seq(QString path)
{
    int ind = _index * 2 + ((_number & 32) ? 1 : 0);

           // stop sequencer
    if(MidiPlayer::fileSequencer[ind])
        MidiPlayer::fileSequencer[ind]->setMode(-1, 0);

    QString seqDir = QDir::homePath() + "/Midieditor";

    if(path == "" || path == "Get It") {

        if (_parent->settings->value("sequencer_path").toString() != "") {
            seqDir = _parent->settings->value("sequencer_path").toString();
        } else {
            _parent->settings->setValue("sequencer_path", seqDir);
        }

        QString newPath = QFileDialog::getOpenFileName(this, "Open Sequencer file",
                                                       seqDir, ".mid/.mseq(*.mid *.mseq *.midi)");
        if(newPath.isEmpty()) {

            return; // canceled
        }

        seqDir = newPath;

        QString seqDirectory = QFileInfo(seqDir).absoluteDir().path() + "/";

        _parent->settings->setValue("sequencer_path", seqDirectory);

        int last = -2;

        for(int n = 0; n < comboFile->count(); n++) {
            if((comboFile->itemData(n).toInt()) > last) {
                last = comboFile->itemData(n).toInt();
                if(last > 0) last &= 31;
            }
        }

        last++;

        if(last < 0)
            last = 0;

        QString savetome;

        if(1) { // scroll always
            int n = 0;

            for(; n < 7; n++) {
                QString entry = MidiInControl::SequencerGP + QString("/seq-") + QString::number(_index) + "-" + QString::number(_number)
                                + "-" + QString::number(n);
                QString entry2 = MidiInControl::SequencerGP + QString("/seq-") + QString::number(_index) + "-" + QString::number(_number)
                                 + "-" + QString::number(n + 1);
                if(n == 0) {
                    // save deleted settings
                    savetome = _parent->settings->value(entry, "").toString();
                }
                _parent->settings->setValue(entry, _parent->settings->value(entry2, ""));
            }

            last = 7;
        }

        QString entry = MidiInControl::SequencerGP + QString("/seq-") + QString::number(_index) + "-" + QString::number(_number)
                        + "-" + QString::number(last);

        int k = 0;
        for(; k < 8; k++) { // test duplicated entries

            QString entry1 = MidiInControl::SequencerGP + QString("/seq-") + QString::number(_index) + "-" + QString::number(_number)
                             + "-" + QString::number(7 - k);

            if(seqDir == _parent->settings->value(entry1, "").toString()) {

                if(seqDir == savetome)
                    savetome = "";

                       // use deleted settings here
                _parent->settings->setValue(entry1, savetome);
                savetome = "";
            }
        }

        _parent->settings->setValue(entry, seqDir);

        comboFile->clear();

        comboFile->addItem("Get It", -1);

        int n;

        for(n = 0; n < 8; n++) { // test exist files

            QString entry = MidiInControl::SequencerGP + QString("/seq-") + QString::number(_index) + "-" + QString::number(_number)
                            + "-" + QString::number(7 - n);

            if (_parent->settings->value(entry).toString() != "") {

                if(QFile(_parent->settings->value(entry).toString()).exists()) {

                    comboFile->addItem(_parent->settings->value(entry).toString(), n);
                } else
                    _parent->settings->setValue(entry, "");
            }
        }

               //comboFile->setCurrentIndex(1);

    } else seqDir = path;

    QString entry = MidiInControl::SequencerGP + QString("/seq-") + QString::number(_index) + "-" + QString::number(_number)
                    + "-select";
    _parent->settings->setValue(entry, seqDir);

    comboFile->setCurrentText(_parent->settings->value(entry).toString());


    QString entry1 = MidiInControl::SequencerGP + QString("/seq-") + QString::number(_index) + "-" + QString::number(_number)
                     + "-";

    int vol = _parent->settings->value(entry1 + "DialVol", 127).toInt();

    if(vol < 0)
        vol = 0;
    if(vol > 127)
        vol = 127;
    DialVol->setValue(vol);

    int beats = _parent->settings->value(entry1 + "DialBeats", 120).toInt();

    if(beats < 1)
        beats = 1;
    if(beats > 508)
        beats = 508;

    DialBeats->setValue(beats);

    MidiPlayer::unload_sequencer(_index * 8 + (_number & 3) + 4 * (_number >= 32));
    bool ok = true;
    MidiFile* seq1  = new MidiFile(seqDir, &ok);
    if(seq1 && ok) {

        if(MidiPlayer::play_sequencer(seq1, _index * 8 + (_number & 3) + 4 * (_number >= 32), beats, vol) < 0) {
            delete seq1;
        }

        unsigned int buttons = 0;

        if(seq1->is_multichannel_sequencer) {
            checkRhythm->setChecked(true);
            buttons |= SEQ_FLAG_AUTORHYTHM;
            _parent->settings->setValue(entry1 + "AutoRhythm", true);
        }

        if(_parent->settings->value(entry1 + "ButtonLoop", false).toBool())
            buttons |= SEQ_FLAG_LOOP;
        if(_parent->settings->value(entry1 + "ButtonAuto", false).toBool())
            buttons |= SEQ_FLAG_INFINITE;
        if(_parent->settings->value(entry1 + "AutoRhythm", false).toBool())
            buttons |= SEQ_FLAG_AUTORHYTHM;

        int ind = _index * 2 + ((_number & 32) ? 1 : 0);

        if(MidiPlayer::fileSequencer[ind]) {
            MidiPlayer::fileSequencer[ind]->setButtons(buttons, _number & 3);

        }


    }

    ButtonLoop->setChecked(_parent->settings->value(entry1 + "ButtonLoop", false).toBool());
    ButtonAuto->setChecked(_parent->settings->value(entry1 + "ButtonAuto", false).toBool());
    checkRhythm->setChecked(_parent->settings->value(entry1 + "AutoRhythm", false).toBool());
    msDelay(25);


    unsigned int buttons = 0;

    // stop sequencer
    if(MidiPlayer::fileSequencer[ind])
        MidiPlayer::fileSequencer[ind]->setMode(-1, 0);

    if(MidiPlayer::fileSequencer[ind])
        buttons = MidiPlayer::fileSequencer[ind]->getButtons(_number & 3);

    if(checkRhythm->isChecked()) {
        //_parent->settings->setValue(entry1 + "AutoRhythm", true);
        buttons |= SEQ_FLAG_AUTORHYTHM;

    } else {
        //_parent->settings->setValue(entry1 + "AutoRhythm", false);
        buttons &= ~SEQ_FLAG_AUTORHYTHM;
    }

    // disable other autorhythm sequencer(s)
    if(buttons & SEQ_FLAG_AUTORHYTHM) {
        for(int ind1 = 0; ind1 < 16; ind1++) {
            if(ind1 == ind)
                continue;
            if(MidiPlayer::fileSequencer[ind1] && MidiOutput::sequencer_enabled[ind1] >= 0) {
                if(MidiPlayer::fileSequencer[ind1]->autorhythm)
                    MidiPlayer::fileSequencer[ind1]->setMode(-1, 0);
            }
        }

    }

           // prepare sequencer
    if(MidiPlayer::fileSequencer[ind])
        MidiPlayer::fileSequencer[ind]->setMode(_number & 3, buttons);


}

InputSequencerItem::InputSequencerItem(InputSequencerListWidget* parent, int index, int number) : QWidget(parent) {

    _number = number;
    _index  = index;
    _parent = parent;

    QString entry1 = MidiInControl::SequencerGP + QString("/seq-") + QString::number(_index) + "-" + QString::number(_number)
                     + "-";

    groupSeq = new QGroupBox(this);
    groupSeq->setObjectName(QString::fromUtf8("groupSeq") + QString::number(index * 64 + number));
    groupSeq->setGeometry(QRect(9, 10, 751, 106));
    groupSeq->setFlat(false);
    groupSeq->setCheckable(true);
    groupSeq->setChecked(_parent->settings->value(entry1 + "Checked", true).toBool());
    TOOLTIP(groupSeq, "Sequencer group: list of sequencer options for the device pair\n\n"
                      "If the box is unchecked the settings cannot be modified");

    if(groupSeq->isChecked()) {

        if(number & 32)
            groupSeq->setTitle("Sequencer DOWN " + QString::number((number & 3) + 1));
        else
            groupSeq->setTitle("Sequencer UP " + QString::number((number & 3) + 1));

    } else {

        if(number & 32)
            groupSeq->setTitle("Sequencer DOWN " + QString::number((number & 3) + 1) + "    (Data Locked)");
        else
            groupSeq->setTitle("Sequencer UP " + QString::number((number & 3) + 1) + "    (Data Locked)");
    }

    connect(groupSeq, QOverload<bool>::of(&QGroupBox::toggled), [=](bool f)
    {
        _parent->settings->setValue(entry1 + "Checked", f);

        if(f) {
            if(number & 32)
                groupSeq->setTitle("Sequencer DOWN " + QString::number((number & 3) + 1));
            else
                groupSeq->setTitle("Sequencer UP " + QString::number((number & 3) + 1));

        } else {

            if(number & 32)
                groupSeq->setTitle("Sequencer DOWN " + QString::number((number & 3) + 1) + "    (Data Locked)");
            else
                groupSeq->setTitle("Sequencer UP " + QString::number((number & 3) + 1) + "    (Data Locked)");

        }

    });

    ButtonOn = new QPushButtonE(groupSeq);
    ButtonOn->setObjectName(QString::fromUtf8("ButtonOn") + QString::number(index * 64 + number));
    ButtonOn->setGeometry(QRect(9, 19, 75, 23));
    ButtonOn->setText("On");
    ButtonOn->setCheckable(true);
    TOOLTIP(ButtonOn, "Turn this sequencer on/off.\n"
                      "There can only be one sequencer connected per UP/DOWN device and\n"
                      "will wait for a note to be pressed except in the case of 'autorhythm' mode.");

    ButtonLoop = new QPushButtonE(groupSeq);
    ButtonLoop->setObjectName(QString::fromUtf8("ButtonLoop") + QString::number(index * 64 + number));
    ButtonLoop->setGeometry(QRect(8, 46, 75, 23));
    ButtonLoop->setText("Loop");
    ButtonLoop->setCheckable(true);
    TOOLTIP(ButtonLoop, "The sequencer will repeat the notes in a loop");

    ButtonAuto = new QPushButtonE(groupSeq);
    ButtonAuto->setObjectName(QString::fromUtf8("ButtonAuto") + QString::number(index * 64 + number));
    ButtonAuto->setGeometry(QRect(7, 73, 75, 23));
    ButtonAuto->setText("Auto");
    ButtonAuto->setCheckable(true);
    TOOLTIP(ButtonAuto, "The sequencer plays without holding the note.\n"
                        "Holding down the note for a while will stop playback when you release it.");

    checkRhythm = new QCheckBox(groupSeq);
    checkRhythm->setObjectName(QString::fromUtf8("checkRhythm"));
    checkRhythm->setGeometry(QRect(250, 73, 96, 22));
    checkRhythm->setChecked(_parent->settings->value(entry1 + "AutoRhythm", false).toBool());
    checkRhythm->setText("Auto Rhythm");
    TOOLTIP2(checkRhythm, "Autorhythm mode is a special mode to play 4 instrument channels + Drum channel\n"
                         "as accompaniment or background melody on output channels 12 to 15 (from Midi file 0 to 3)\n"
                         "and 9 for Drum, leaving the device UP /DOWN free to play notes.", 0);

    ButtonLoop->setChecked(_parent->settings->value(entry1 + "ButtonLoop", false).toBool());
    ButtonAuto->setChecked(_parent->settings->value(entry1 + "ButtonAuto", false).toBool());

    int ind = index * 2 + ((number & 32) ? 1 : 0);
    int num = MidiOutput::sequencer_enabled[ind];


    if(num == (number & 31) && (((ind & 1) && (number & 32)) || (!(ind & 1) && !(number & 32))))
        ButtonOn->setChecked(true);
    else
        ButtonOn->setChecked(false);

    connect(_parent, &InputSequencerListWidget::updateButtons, this, [=]()
    {
        for(int n = 0; n < parent->items.count(); n++) {

            InputSequencerItem * item =  parent->items.at(n);

            int ind = item->_index * 2 + ((item->_number & 32) ? 1 : 0);

            if(MidiOutput::sequencer_enabled[ind] == (item->_number & 31))
                item->ButtonOn->setChecked(true);
            else
                item->ButtonOn->setChecked(false);

        }

    });

    connect(checkRhythm, &QCheckBox::clicked, this, [=]()
    {
        unsigned int buttons = 0;

               // stop sequencer
        if(MidiPlayer::fileSequencer[ind])
            MidiPlayer::fileSequencer[ind]->setMode(-1, 0);

        if(MidiPlayer::fileSequencer[ind])
            buttons = MidiPlayer::fileSequencer[ind]->getButtons(number & 3);

        if(checkRhythm->isChecked()) {
            _parent->settings->setValue(entry1 + "AutoRhythm", true);
            buttons |= SEQ_FLAG_AUTORHYTHM;

        } else {
            _parent->settings->setValue(entry1 + "AutoRhythm", false);
            buttons &= ~SEQ_FLAG_AUTORHYTHM;
        }

               // disable other autorhythm sequencer(s)
        if(buttons & SEQ_FLAG_AUTORHYTHM) {
            for(int ind1 = 0; ind1 < 16; ind1++) {
                if(ind1 == ind)
                    continue;
                if(MidiPlayer::fileSequencer[ind1] && MidiOutput::sequencer_enabled[ind1] >= 0) {
                    if(MidiPlayer::fileSequencer[ind1]->autorhythm)
                        MidiPlayer::fileSequencer[ind1]->setMode(-1, 0);
                }
            }

        }

               // prepare sequencer
        if(MidiPlayer::fileSequencer[ind])
            MidiPlayer::fileSequencer[ind]->setMode(_number & 3, buttons);


    });

    connect(ButtonOn, QOverload<bool>::of(&QPushButtonE::clicked), [=](bool)
    {
        int ind = index * 2 + ((number & 32) ? 1 : 0);

        QString text = "Dev: " + QString::number(ind) + " - SEQUENCER -" + " " + QString::number((number & 31) + 1);

        if(!MidiPlayer::fileSequencer[ind])
            return;

        unsigned int buttons = 0;

               // stop sequencer
        if(MidiPlayer::fileSequencer[ind])
            MidiPlayer::fileSequencer[ind]->setMode(-1, 0);

        if(ButtonOn->isChecked()) {

            if(MidiPlayer::fileSequencer[ind])
                buttons = MidiPlayer::fileSequencer[ind]->getButtons(number & 3);

                   // disable other autorhythm sequencer(s)
            if(buttons & SEQ_FLAG_AUTORHYTHM) {

                for(int ind1 = 0; ind1 < 16; ind1++) {

                    if(ind1 == ind)
                        continue;

                    if(MidiPlayer::fileSequencer[ind1] && MidiOutput::sequencer_enabled[ind1] >= 0) {
                        if(MidiPlayer::fileSequencer[ind1]->autorhythm)
                            MidiPlayer::fileSequencer[ind1]->setMode(-1, 0);
                    }
                }

            }

                   // prepare sequencer
            if(MidiPlayer::fileSequencer[ind])
                MidiPlayer::fileSequencer[ind]->setMode(_number & 3, buttons | SEQ_FLAG_SWITCH_ON);

            if(number & 32)
                text+= " DOWN - ON";
            else
                text+= " UP - ON";
        } else {
            if(number & 32)
                text+= " DOWN - OFF";
            else
                text+= " UP - OFF";
        }


        for(int n = 0; n < parent->items.count(); n++) {
            InputSequencerItem * item =  parent->items.at(n);
            if(item->_index == index && item->_number != _number
                && (item->_number & 32) == (_number & 32))
                item->ButtonOn->setChecked(false);
        }

        MidiInControl::OSD = text;

    });

    connect(ButtonLoop, QOverload<bool>::of(&QPushButtonE::clicked), [=](bool checked)
    {
        _parent->settings->setValue(entry1 + "ButtonLoop", checked);

        unsigned int buttons = 0;

        if(_parent->settings->value(entry1 + "ButtonLoop", false).toBool())
            buttons |= SEQ_FLAG_LOOP;
        if(_parent->settings->value(entry1 + "ButtonAuto", false).toBool())
            buttons |= SEQ_FLAG_INFINITE;
        if(_parent->settings->value(entry1 + "AutoRhythm", false).toBool())
            buttons |= SEQ_FLAG_AUTORHYTHM;

        int ind = index * 2 + ((number & 32) ? 1 : 0);

        if(MidiPlayer::fileSequencer[ind])
            MidiPlayer::fileSequencer[ind]->setButtons(buttons, _number & 3);

    });

    connect(ButtonAuto, QOverload<bool>::of(&QPushButtonE::clicked), [=](bool checked)
    {
        _parent->settings->setValue(entry1 + "ButtonAuto", checked);

        unsigned buttons = 0;

        if(_parent->settings->value(entry1 + "ButtonLoop", false).toBool())
            buttons |= SEQ_FLAG_LOOP;
        if(_parent->settings->value(entry1 + "ButtonAuto", false).toBool())
            buttons |= SEQ_FLAG_INFINITE;
        if(_parent->settings->value(entry1 + "AutoRhythm", false).toBool())
            buttons |= SEQ_FLAG_AUTORHYTHM;

        int ind = index * 2 + ((number & 32) ? 1 : 0);

        if(MidiPlayer::fileSequencer[ind])
            MidiPlayer::fileSequencer[ind]->setButtons(buttons, _number & 3);

    });

    DialVol = new QDialE(groupSeq, QDIALE_GRAY_NOTCH_BLACKDOT);
    DialVol->setObjectName(QString::fromUtf8("DialVol") + QString::number(index * 64 + number));
    DialVol->setGeometry(QRect(98, 22, 61, 64));
    DialVol->setMaximum(127);
    DialVol->setValue(0);
    DialVol->setNotchTarget(16.000000000000000);
    DialVol->setNotchesVisible(true);
    TOOLTIP(DialVol, "Reference velocity as 'volume' of the note sequence.");

    label_vol = new QLabel(groupSeq);
    label_vol->setObjectName(QString::fromUtf8("label_vol") + QString::number(index * 64 + number));
    label_vol->setGeometry(QRect(113, 83, 31, 19));
    label_vol->setAlignment(Qt::AlignCenter);
    label_vol->setText("0");

    labelVol = new QLabel(groupSeq);
    labelVol->setObjectName(QString::fromUtf8("labelVol") + QString::number(index * 64 + number));
    labelVol->setGeometry(QRect(101, 9, 61, 20));
    labelVol->setAlignment(Qt::AlignCenter);
    labelVol->setText("Volume");

    DialBeats = new QDialE(groupSeq, QDIALE_GRAY_NOTCH_BLACKDOT);
    DialBeats->setObjectName(QString::fromUtf8("DialBeats") + QString::number(index * 64 + number));
    DialBeats->setGeometry(QRect(169, 23, 61, 64));
    DialBeats->setMinimum(1);
    DialBeats->setMaximum(508);
    DialBeats->setValue(1);
    DialBeats->setNotchTarget(32.000000000000000);
    DialBeats->setNotchesVisible(true);
    TOOLTIP(DialBeats, "Beats per minute to play the note sequence");

    label_beats = new QLabel(groupSeq);
    label_beats->setObjectName(QString::fromUtf8("label_beats") + QString::number(index * 64 + number));
    label_beats->setGeometry(QRect(185, 82, 31, 19));
    label_beats->setAlignment(Qt::AlignCenter);
    label_beats->setText("0");

    labelBeats = new QLabel(groupSeq);
    labelBeats->setObjectName(QString::fromUtf8("labelBeats") + QString::number(index * 64 + number));
    labelBeats->setGeometry(QRect(169, 9, 61, 20));
    labelBeats->setAlignment(Qt::AlignCenter);
    labelBeats->setText("BPM");

    connect(DialVol, SIGNAL(valueChanged(int)), label_vol, SLOT(setNum(int)));
    connect(DialBeats, SIGNAL(valueChanged(int)), label_beats, SLOT(setNum(int)));

    int vol = _parent->settings->value(entry1 + "DialVol", 127).toInt();

    if(vol < 0)
        vol = 0;
    if(vol > 127)
        vol = 127;
    DialVol->setValue(vol);

    int beats = _parent->settings->value(entry1 + "DialBeats", 120).toInt();

    if(beats < 1)
        beats = 1;
    if(beats > 500)
        beats = 500;

    DialBeats->setValue(beats);

    connect(DialVol, QOverload<int>::of(&QDialE::valueChanged), [=](int v)
    {
        _parent->settings->setValue(entry1 + "DialVol", v);

        int ind = index * 2 + ((number & 32) ? 1 : 0);

        if(MidiPlayer::fileSequencer[ind])
            MidiPlayer::fileSequencer[ind]->setVolume(v, number & 3);

    });

    connect(DialBeats, QOverload<int>::of(&QDialE::valueChanged), [=](int v)
    {
        _parent->settings->setValue(entry1 + "DialBeats", v);

        int ind = index * 2 + ((number & 32) ? 1 : 0);

        if(MidiPlayer::fileSequencer[ind])
            MidiPlayer::fileSequencer[ind]->setScaleTime(v, number & 3);
    });

    comboFile = new QComboBox(groupSeq);
    comboFile->setObjectName(QString::fromUtf8("comboFile") + QString::number(index * 64 + number));
    comboFile->setGeometry(QRect(250, 18, 419 + 64, 22));
    //comboFile->setMaxVisibleItems(32);
    comboFile->setAcceptDrops(true);
    comboFile->setMaxCount(33);
    comboFile->setLayoutDirection(Qt::LeftToRight);
    comboFile->setDuplicatesEnabled(true);
    comboFile->addItem("Get It", -1);
    TOOLTIP2(comboFile, "MIDI/MSEQ file (an MSEQ is a compacted/prepared MIDI) that contains the sequence of notes.\n"
                       "Note that this sequence of notes is injected into the input device as events and therefore\n"
                       "VST plugins that must be connected to the Fluidsynth output device are not directly supported.", 0);

    int n;

    for(n = 0; n < 8; n++) {
        QString entry = MidiInControl::SequencerGP + QString("/seq-") + QString::number(_index) + "-" + QString::number(_number)
                        + "-" + QString::number(7 - n);

        if (_parent->settings->value(entry).toString() != "") {
            if(QFile(_parent->settings->value(entry).toString()).exists()) {

                comboFile->addItem(_parent->settings->value(entry).toString(), n);
            } else
                _parent->settings->setValue(entry, "");
        }
    }

    QString entry = MidiInControl::SequencerGP + QString("/seq-") + QString::number(_index) + "-" + QString::number(_number)
                    + "-select";

    if (_parent->settings->value(entry).toString() != "")
        comboFile->setCurrentText(_parent->settings->value(entry).toString());


    connect(comboFile, SIGNAL(textActivated(QString)), this, SLOT(load_Seq(QString)));

}
