#ifndef TIMESIGNATUREDIALOG_H
#define TIMESIGNATUREDIALOG_H


#include <QDialog>
#include <QRadioButton>
#include <QSpinBox>
#include <QComboBox>

#include "../midi/MidiFile.h"

class TimeSignatureDialog : public QDialog {

    Q_OBJECT

public:
    TimeSignatureDialog(MidiFile *file, int measure, int startTickOfMeasure, QWidget* parent = 0);

public slots:
    void accept();

private:
    QSpinBox *_beats;
    QComboBox *_beatType;
    QRadioButton *_untilNextMeterChange, *_endOfPiece;

    MidiFile *_file;
    int _measure, _startTickOfMeasure;
};

#endif // TIMESIGNATUREDIALOG_H
