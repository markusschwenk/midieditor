#ifndef TEMPODIALOG_H
#define TEMPODIALOG_H


#include <QDialog>
#include <QSpinBox>
#include <QCheckBox>

#include "../midi/MidiFile.h"

class TempoDialog : public QDialog {

    Q_OBJECT

public:
    TempoDialog(MidiFile *file, int startTick, int endTick=-1, QWidget* parent = 0);

public slots:
    void accept();

private:
    QSpinBox *_endBeats, *_startBeats;
    QCheckBox *_smoothTransition;
    int _startTick, _endTick;
    MidiFile *_file;
};

#endif // TEMPODIALOG_H
