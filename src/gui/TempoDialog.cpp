#include "TempoDialog.h"

#include <QGridLayout>
#include <QLabel>
#include <QIcon>
#include <QPushButton>
#include <QMap>

#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/TempoChangeEvent.h"
#include "../protocol/Protocol.h"
#include "../midi/MidiChannel.h"

TempoDialog::TempoDialog(MidiFile *file, int startTick, int endTick,  QWidget *parent) : QDialog(parent)
{
    _startTick = startTick;
    _endTick = endTick;
    _file = file;

    setMinimumWidth(550);
    setMaximumHeight(450);
    setWindowTitle(tr("Edit Tempo"));
    setWindowIcon(QIcon(":/run_environment/graphics/icon.png"));
    QGridLayout* layout = new QGridLayout(this);

    QLabel* icon = new QLabel();
    icon->setPixmap(QPixmap(":/run_environment/graphics/midieditor.png").scaledToWidth(80, Qt::SmoothTransformation));
    icon->setFixedSize(80, 80);
    layout->addWidget(icon, 0, 0, 3, 1);

    QLabel* title = new QLabel("<h3>Edit Tempo</h3>", this);
    layout->addWidget(title, 0, 1, 1, 2);
    title->setStyleSheet("color: black");

    // Use smooth transition
    _smoothTransition = new QCheckBox("Smooth Transition");
    layout->addWidget(_smoothTransition, 1, 1, 1, 2);

    QLabel* tip = new QLabel("<html>"
                                 "<body>"
                                 "<p>"
                                 "<b>Tip</b>: Select time range to enter smooth transitions"
                                 "</p>"
                                 "</body>"
                                 "</html>");
    layout->addWidget(tip, 2, 1, 1, 2);
    tip->setStyleSheet("color: black; background-color: white; padding: 5px");

    // identify tempo at start tick
    QMap<int, MidiEvent*> *events = file->tempoEvents();
    QMap<int, MidiEvent*>::iterator it = events->begin();
    int tick = -1;
    MidiEvent *ev = 0;
    while(it != events->end()) {
        if (it.key() <= _startTick && (tick < 0 || tick < it.key())) {
            tick = it.key();
            ev = it.value();
        }
        it++;
    }

    int beats = 120;
    if (ev != 0 && dynamic_cast<TempoChangeEvent*>(ev) != 0) {
        beats = dynamic_cast<TempoChangeEvent*>(ev)->beatsPerQuarter();
    }
    // Beats
    QLabel *beatsLabel = new QLabel("Beats (Quarters) per Minute:");
    layout->addWidget(beatsLabel, 3, 1, 1, 2);
    QLabel *beginLabel = new QLabel("Starting From:");
    layout->addWidget(beginLabel, 4, 1, 1, 1);
    _startBeats = new QSpinBox();
    _startBeats->setMinimum(1);
    _startBeats->setMaximum(500);
    _startBeats->setValue(beats);
    layout->addWidget(_startBeats, 4, 2, 1, 1);

    QLabel *endLabel = new QLabel("New Tempo:");
    layout->addWidget(endLabel, 5, 1, 1, 1);
    _endBeats = new QSpinBox();
    _endBeats->setMinimum(1);
    _endBeats->setMaximum(500);
    _endBeats->setValue(beats);
    layout->addWidget(_endBeats, 5, 2, 1, 1);


    if (_endTick > -1) {
        _smoothTransition->setCheckable(true);
        _smoothTransition->setChecked(true);
        connect(_smoothTransition, SIGNAL(toggled(bool)),
            _startBeats, SLOT(setEnabled(bool)));
        connect(_smoothTransition, SIGNAL(toggled(bool)),
            beginLabel, SLOT(setEnabled(bool)));
    } else {
        _smoothTransition->setChecked(false);
        _smoothTransition->setCheckable(false);
        _smoothTransition->setEnabled(false);
        _startBeats->setEnabled(false);
        beginLabel->setEnabled(false);
    }

    QFrame* f = new QFrame(this);
    f->setFrameStyle(QFrame::HLine | QFrame::Sunken);
    layout->addWidget(f, 9, 0, 1, 3);

    // Accept / Break button
    QPushButton* breakButton = new QPushButton("Cancel");
    connect(breakButton, SIGNAL(clicked()), this, SLOT(hide()));
    QPushButton* acceptButton = new QPushButton("Accept");
    connect(acceptButton, SIGNAL(clicked()), this, SLOT(accept()));

    layout->addWidget(breakButton, 10, 1, 1, 1);
    layout->addWidget(acceptButton, 10, 2, 1, 1);
}

void TempoDialog::accept()
{

    MidiTrack* generalTrack = _file->track(0);
    _file->protocol()->startNewAction("Edit Tempo");

    // Delete all events in range
    QList<MidiEvent*> toRemove;
    QMap<int, MidiEvent*> *events = _file->tempoEvents();
    QMap<int, MidiEvent*>::iterator it = events->begin();
    int fromTick = _startTick;
    int toTick = _startTick;
    if (_endTick > -1) {
        toTick = _endTick;
    }
    while(it != events->end()) {
        if (it.key() >= fromTick && it.key() <= toTick) {
            toRemove.append(it.value());
        }
        it++;
    }
    foreach (MidiEvent *ev, toRemove) {
        _file->channel(17)->removeEvent(ev);
    }

    if (_smoothTransition->isChecked()) {
        int startBeats = _startBeats->value();
        int endBeats = _endBeats->value();

        for (int tick = _startTick; tick < _endTick; tick += 5) {
            int beats = startBeats + (endBeats - startBeats) * (tick - _startTick) / (_endTick - _startTick);
            _file->channel(17)->insertEvent(new TempoChangeEvent(17, 60000000 / beats, generalTrack), tick);
        }
        _file->channel(17)->insertEvent(new TempoChangeEvent(17, 60000000 / endBeats, generalTrack), _endTick);
    } else {
        int beats = _endBeats->value();
        _file->channel(17)->insertEvent(new TempoChangeEvent(17, 60000000 / beats, generalTrack), _startTick);
    }
    hide();

    _file->protocol()->endAction();
}


