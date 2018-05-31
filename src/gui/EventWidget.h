/*
 * MidiEditor
 * Copyright (C) 2010  Markus Schwenk
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef EVENTWIDGET_H_
#define EVENTWIDGET_H_

#include <QStyledItemDelegate>
#include <QTableWidget>

class MidiEvent;
class EventWidget;
class MidiFile;

class EventWidgetDelegate : public QStyledItemDelegate {

    Q_OBJECT

public:
    EventWidgetDelegate(EventWidget* w, QWidget* parent = 0)
        : QStyledItemDelegate(parent)
    {
        eventWidget = w;
    }
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    void setEditorData(QWidget* editor, const QModelIndex& index) const;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;

private:
    EventWidget* eventWidget;
};

class EventWidget : public QTableWidget {

    Q_OBJECT

public:
    EventWidget(QWidget* parent = 0);

    void setEvents(QList<MidiEvent*> events);
    QList<MidiEvent*> events();
    void removeEvent(MidiEvent* event);

    void setFile(MidiFile* file);
    MidiFile* file();

    enum EventType {
        MidiEventType,
        ChannelPressureEventType,
        ControlChangeEventType,
        KeyPressureEventType,
        KeySignatureEventType,
        NoteEventType,
        PitchBendEventType,
        ProgramChangeEventType,
        SystemExclusiveEventType,
        TempoChangeEventType,
        TextEventType,
        TimeSignatureEventType,
        UnknownEventType
    };

    enum EditorField {
        MidiEventTick,
        MidiEventTrack,
        MidiEventChannel,
        MidiEventNote,
        NoteEventOffTick,
        NoteEventVelocity,
        NoteEventDuration,
        MidiEventValue,
        ControlChangeControl,
        ProgramChangeProgram,
        KeySignatureKey,
        TimeSignatureDenom,
        TimeSignatureNum,
        TextType,
        TextText,
        UnknownType,
        MidiEventData
    };
    QVariant fieldContent(EditorField field);

    EventType type() { return _currentType; }

    QStringList keyStrings();
    int keyIndex(int tonality, bool minor);
    void getKey(int index, int* tonality, bool* minor);

    static QString dataToString(QByteArray data);

    void reportSelectionChangedByTool();

public slots:
    void reload();

signals:
    void selectionChanged(bool);
    void selectionChangedByTool(bool);

private:
    QList<MidiEvent*> _events;

    EventType _currentType;
    EventType computeType();
    QString eventType();

    QList<QPair<QString, EditorField> > getFields();

    MidiFile* _file;
};

#endif
