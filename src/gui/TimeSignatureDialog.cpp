#include "TimeSignatureDialog.h"

#include <QGridLayout>
#include <QLabel>
#include <QIcon>
#include <QPushButton>
#include <QButtonGroup>
#include <QComboBox>
#include <QtCore/qmath.h>
#include <QMap>
#include <QMessageBox>

#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/TimeSignatureEvent.h"
#include "../protocol/Protocol.h"
#include "../midi/MidiChannel.h"

TimeSignatureDialog::TimeSignatureDialog(MidiFile *file, int measure, int measureStartTick,  QWidget *parent) : QDialog(parent)
{
    _measure = measure;
    _file = file;
    _startTickOfMeasure = measureStartTick;

    setMinimumWidth(550);
    setMaximumHeight(450);
    setWindowTitle(tr("Change Meter"));
    setWindowIcon(QIcon(":/run_environment/graphics/icon.png"));
    QGridLayout* layout = new QGridLayout(this);

    QLabel* icon = new QLabel();
    icon->setPixmap(QPixmap(":/run_environment/graphics/midieditor.png").scaledToWidth(80, Qt::SmoothTransformation));
    icon->setFixedSize(80, 80);
    layout->addWidget(icon, 0, 0, 3, 1);

    QLabel* title = new QLabel("<h3>Edit Time Signature</h3>", this);
    layout->addWidget(title, 0, 1, 1, 2);
    title->setStyleSheet("color: black");

    // Beats
    QLabel *beatsLabel = new QLabel("Number of Beats:");
    _beats = new QSpinBox();
    _beats->setMinimum(1);
    _beats->setValue(4);

    layout->addWidget(beatsLabel, 1, 1, 1, 1);
    layout->addWidget(_beats, 1, 2, 1, 1);

    // beattype
    QLabel *beatTypeLabel = new QLabel("Beat Type:");
    _beatType = new QComboBox();
    for (int i = 0; i < 5; i++){
        _beatType->addItem(QString::number((int)qPow(2, i)));
    }
    _beatType->setCurrentIndex(2); // quarters

    layout->addWidget(beatTypeLabel, 2, 1, 1, 1);
    layout->addWidget(_beatType, 2, 2, 1, 1);

    // Use new Meter....
    QLabel *untilLabel = new QLabel("Use new Time Signature...");
    layout->addWidget(untilLabel, 3, 1, 1, 2);
    QButtonGroup *rangeGroup = new QButtonGroup;

    // End of song
    _endOfPiece = new QRadioButton("... Until End of Song");
    layout->addWidget(_endOfPiece, 4, 1, 1, 2);
    rangeGroup->addButton(_endOfPiece);

    // until next change
    _untilNextMeterChange = new QRadioButton("... Until Next Time Signature");
    _untilNextMeterChange->setChecked(true);
    layout->addWidget(_untilNextMeterChange, 5, 1, 1, 2);
    rangeGroup->addButton(_untilNextMeterChange);

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

void TimeSignatureDialog::accept()
{

    bool ok;
    int numerator = _beats->text().toInt(&ok);
    if (!ok) {
        QMessageBox::information(this, "Error", "Cannot read input field for number of beats.");
        return;
    }
    int denum = _beatType->currentIndex();
    MidiTrack* generalTrack = _file->track(0);
    _file->protocol()->startNewAction("Change Time Signature");

    QMap<int, MidiEvent*> *timeSignatureEvents = _file->timeSignatureEvents();
    bool hasTimeSignatureChangesAfter = false;
    foreach(int tick, timeSignatureEvents->keys()) {
        if (tick > _startTickOfMeasure) {
            hasTimeSignatureChangesAfter = true;
            break;
        }
    }

    TimeSignatureEvent *newEvent = new TimeSignatureEvent(18, numerator, denum, 24, 8, generalTrack);
    newEvent->setFile(_file);
    QList<MidiEvent*> eventsToDelete;

    if (_endOfPiece->isChecked() || (_untilNextMeterChange->isChecked() && !hasTimeSignatureChangesAfter)){
        // We delete all events after and insert a single new one.
        QMap<int, MidiEvent*>::Iterator it = timeSignatureEvents->begin();
        while(it != timeSignatureEvents->end()) {
            if (it.key() >= _startTickOfMeasure) {
                eventsToDelete.append(it.value());
            }
            it++;
        }
    } else if (_untilNextMeterChange->isChecked()){

        // until next meter change and we have change events after.
        int tickFromNextChangeEvent = -1;
        foreach(int tick, timeSignatureEvents->keys()) {
            if (tick > _startTickOfMeasure && (tickFromNextChangeEvent < 0 || tickFromNextChangeEvent > tick)) {
                tickFromNextChangeEvent = tick;
            }
        }
        if ((tickFromNextChangeEvent - _startTickOfMeasure) % newEvent->ticksPerMeasure() != 0) {
            // alert
            QMessageBox::information(this, "Error", "The next Time Signature Event would not be on Downbeat.");
            return;
        }
        // Remove time signature in the same measure
        if (timeSignatureEvents->value(_startTickOfMeasure) != 0) {
            eventsToDelete.append(timeSignatureEvents->value(_startTickOfMeasure));
        }
    }

    foreach (MidiEvent *eventToDelete, eventsToDelete){
        _file->channel(18)->removeEvent(eventToDelete);
    }

    _file->channel(18)->insertEvent(newEvent, _startTickOfMeasure);
    hide();

    _file->protocol()->endAction();
}


