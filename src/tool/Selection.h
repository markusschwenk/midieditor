#ifndef SELECTION_H
#define SELECTION_H

#include "../protocol/ProtocolEntry.h"

#include <QList>
class MidiEvent;
class EventWidget;

class Selection : public ProtocolEntry {

public:
    Selection(MidiFile* file);
    Selection(Selection& other);

    virtual ProtocolEntry* copy();
    virtual void reloadState(ProtocolEntry* entry);

    virtual MidiFile* file();

    static Selection* instance();
    static void setFile(MidiFile* file);

    QList<MidiEvent*> selectedEvents();
    void setSelection(QList<MidiEvent*> selections);
    void clearSelection();

    static EventWidget* _eventWidget;

private:
    QList<MidiEvent*> _selectedEvents;
    static Selection* _selectionInstance;
    MidiFile* _file;
};

#endif
