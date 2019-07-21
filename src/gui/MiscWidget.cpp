#include "MiscWidget.h"
#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/NoteOnEvent.h"
#include "../midi/MidiChannel.h"
#include "../midi/MidiFile.h"
#include "../midi/MidiTrack.h"
#include "../protocol/Protocol.h"
#include "../tool/EventTool.h"
#include "../tool/NewNoteTool.h"
#include "../tool/SelectTool.h"
#include "../tool/Selection.h"
#include "MatrixWidget.h"

#include "../MidiEvent/ChannelPressureEvent.h"
#include "../MidiEvent/ControlChangeEvent.h"
#include "../MidiEvent/KeyPressureEvent.h"
#include "../MidiEvent/PitchBendEvent.h"

#define LEFT_BORDER_MATRIX_WIDGET 110
#define WIDTH 7

MiscWidget::MiscWidget(MatrixWidget* mw, QWidget* parent)
    : PaintWidget(parent)
{
    setRepaintOnMouseMove(true);
    setRepaintOnMousePress(true);
    setRepaintOnMouseRelease(true);
    matrixWidget = mw;
    edit_mode = SINGLE_MODE;
    mode = VelocityEditor;
    channel = 0;
    controller = 0;
    dragY = 0;
    isDrawingFreehand = false;
    isDrawingLine = false;
    resetState();
    computeMinMax();
    connect(matrixWidget, SIGNAL(objectListChanged()), this, SLOT(update()));
    _dummyTool = new SelectTool(SELECTION_TYPE_SINGLE);
    setFocusPolicy(Qt::ClickFocus);
}

QString MiscWidget::modeToString(int mode)
{
    switch (mode) {
    case VelocityEditor:
        return "Velocity";
    case ControllEditor:
        return "Control Change";
    case PitchBendEditor:
        return "Pitch Bend";
    case KeyPressureEditor:
        return "Key Pressure";
    case ChannelPressureEditor:
        return "Channel Pressure";
    }
    return "";
}

void MiscWidget::setMode(int mode)
{
    this->mode = mode;
    resetState();
    computeMinMax();
}

void MiscWidget::setEditMode(int mode)
{
    this->edit_mode = mode;
    resetState();
    computeMinMax();
}

void MiscWidget::setChannel(int channel)
{
    this->channel = channel;
    resetState();
    computeMinMax();
}

void MiscWidget::setControl(int ctrl)
{
    this->controller = ctrl;
    resetState();
    computeMinMax();
}

void MiscWidget::paintEvent(QPaintEvent* event)
{

    if (!matrixWidget->midiFile())
        return;

    // draw background
    QPainter painter(this);
    QFont f = painter.font();
    f.setPixelSize(9);
    painter.setFont(f);
    QColor c(234, 246, 255);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::gray);
    painter.setBrush(c);
    painter.drawRect(0, 0, width() - 1, height() - 1);

    painter.setPen(QColor(194, 230, 255));
    for (int i = 0; i < 8; i++) {
        painter.drawLine(0, (i * height()) / 8, width(), (i * height()) / 8);
    }

    // divs
    typedef QPair<int, int> TMPPair;
    foreach (TMPPair p, matrixWidget->divs()) {
        painter.drawLine(p.first - LEFT_BORDER_MATRIX_WIDGET, 0, p.first - LEFT_BORDER_MATRIX_WIDGET, height());
    }

    // draw contents
    if (mode == VelocityEditor) {

        QList<MidiEvent*>* list = matrixWidget->velocityEvents();
        foreach (MidiEvent* event, *list) {

            if (!event->file()->channel(event->channel())->visible()) {
                continue;
            }

            if (event->track()->hidden()) {
                continue;
            }

            QColor* c = matrixWidget->midiFile()->channel(event->channel())->color();
            if (!matrixWidget->colorsByChannel()) {
                c = event->track()->color();
            }

            int velocity = 0;
            NoteOnEvent* noteOn = dynamic_cast<NoteOnEvent*>(event);
            if (noteOn) {
                velocity = noteOn->velocity();

                if (velocity > 0) {
                    int h = (height() * velocity) / 128;
                    painter.setBrush(*c);
                    painter.setPen(Qt::lightGray);
                    painter.drawRoundedRect(event->x() - LEFT_BORDER_MATRIX_WIDGET, height() - h, WIDTH, h, 1, 1);
                }
            }
        }

        // paint selected events above all others
        EventTool* t = dynamic_cast<EventTool*>(Tool::currentTool());
        if (t && t->showsSelection()) {
            foreach (MidiEvent* event, Selection::instance()->selectedEvents()) {

                if (!event->file()->channel(event->channel())->visible()) {
                    continue;
                }

                if (event->track()->hidden()) {
                    continue;
                }

                int velocity = 0;
                NoteOnEvent* noteOn = dynamic_cast<NoteOnEvent*>(event);

                if (noteOn && noteOn->midiTime() >= matrixWidget->minVisibleMidiTime() && noteOn->midiTime() <= matrixWidget->maxVisibleMidiTime()) {
                    velocity = noteOn->velocity();

                    if (velocity > 0) {
                        int h = (height() * velocity) / 128;
                        if (edit_mode == SINGLE_MODE && dragging) {
                            h += (dragY - mouseY);
                        }
                        painter.setBrush(Qt::darkBlue);
                        painter.setPen(Qt::lightGray);
                        painter.drawRoundedRect(event->x() - LEFT_BORDER_MATRIX_WIDGET, height() - h, WIDTH, h, 1, 1);
                    }
                }
            }
        }
    }

    // draw content track
    if (mode > VelocityEditor) {

        QColor* c = matrixWidget->midiFile()->channel(0)->color();
        QPen pen(*c);
        pen.setWidth(3);
        painter.setPen(pen);

        QPen circlePen(Qt::darkGray);
        circlePen.setWidth(1);

        QList<MidiEvent*> accordingEvents;
        QList<QPair<int, int> > track = getTrack(&accordingEvents);

        int xOld;
        int yOld;

        for (int i = 0; i < track.size(); i++) {

            int xPix = track.at(i).first;
            int yPix = height() - ((double)track.at(i).second / (double)_max) * height();
            if (edit_mode == SINGLE_MODE) {
                if (i == trackIndex) {
                    if (dragging) {
                        yPix = yPix + mouseY - dragY;
                    }
                }
            }
            if (i > 0) {
                painter.drawLine(xOld, yOld, xPix, yOld);
                painter.drawLine(xPix, yOld, xPix, yPix);
            }
            xOld = xPix;
            yOld = yPix;
        }
        painter.drawLine(xOld, yOld, width(), yOld);

        for (int i = 0; i < track.size(); i++) {

            int xPix = track.at(i).first;
            int yPix = height() - ((double)track.at(i).second / (double)_max) * height();
            if (edit_mode == SINGLE_MODE) {
                if (i == trackIndex) {
                    if (dragging) {
                        yPix = yPix + mouseY - dragY;
                    }
                }
            }

            if (edit_mode == SINGLE_MODE && (dragging || mouseOver)) {

                if (accordingEvents.at(i) && Selection::instance()->selectedEvents().contains(accordingEvents.at(i))) {
                    painter.setBrush(Qt::darkBlue);
                }
                painter.setPen(circlePen);
                if (i == trackIndex) {
                    painter.setBrush(Qt::gray);
                }
                painter.drawEllipse(xPix - 4, yPix - 4, 8, 8);
                painter.setPen(pen);
                painter.setBrush(Qt::NoBrush);
            }
        }
    }

    // draw freehand track
    if (edit_mode == MOUSE_MODE && isDrawingFreehand && freeHandCurve.size() > 0) {

        int xOld;
        int yOld;

        QPen pen(Qt::darkBlue);
        pen.setWidth(3);
        painter.setPen(pen);

        for (int i = 0; i < freeHandCurve.size(); i++) {
            int xPix = freeHandCurve.at(i).first;
            int yPix = freeHandCurve.at(i).second;
            if (i > 0) {
                painter.drawLine(xOld, yOld, xPix, yPix);
            }
            xOld = xPix;
            yOld = yPix;
        }
    }

    // draw line
    if (edit_mode == LINE_MODE && isDrawingLine) {

        QPen pen(Qt::darkBlue);
        pen.setWidth(3);
        painter.setPen(pen);

        painter.drawLine(lineX, lineY, mouseX, mouseY);
    }
}

void MiscWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (edit_mode == SINGLE_MODE) {
        if (mode == VelocityEditor) {
            bool above = dragging;
            if (!above) {
                QList<MidiEvent*>* list = matrixWidget->velocityEvents();
                foreach (MidiEvent* event, *list) {

                    if (!event->file()->channel(event->channel())->visible()) {
                        continue;
                    }

                    if (event->track()->hidden()) {
                        continue;
                    }

                    int velocity = 0;
                    NoteOnEvent* noteOn = dynamic_cast<NoteOnEvent*>(event);
                    if (noteOn) {
                        velocity = noteOn->velocity();

                        if (velocity > 0) {
                            int h = (height() * velocity) / 128;
                            if (!dragging && mouseInRect(event->x() - LEFT_BORDER_MATRIX_WIDGET, height() - h - 5, WIDTH, 10)) {
                                above = true;
                                break;
                            }
                        }
                    }
                }
            }
            if (above) {
                setCursor(Qt::SizeVerCursor);
            } else {
                setCursor(Qt::ArrowCursor);
            }

        } else {

            //other modes
            if (!dragging) {
                trackIndex = -1;
                QList<QPair<int, int> > track = getTrack();
                for (int i = 0; i < track.size(); i++) {

                    int xPix = track.at(i).first;
                    int yPix = height() - ((double)track.at(i).second / (double)_max) * height();

                    if (mouseInRect(xPix - 4, yPix - 4, 8, 8)) {
                        trackIndex = i;
                        //setCursor(Qt::SizeVerCursor);
                        setCursor(Qt::ArrowCursor);
                        break;
                    }
                }

                if (trackIndex == -1) {
                    setCursor(Qt::ArrowCursor);
                }
            } else {
                setCursor(Qt::SizeVerCursor);
            }
        }
    }
    if (edit_mode == MOUSE_MODE) {
        if (isDrawingFreehand) {
            bool ok = true;
            for (int i = 0; i < freeHandCurve.size(); i++) {
                if (freeHandCurve.at(i).first >= event->x()) {
                    ok = false;
                    break;
                }
            }
            if (ok) {
                freeHandCurve.append(QPair<int, int>(event->x(), event->y()));
            }
        }
    }
    PaintWidget::mouseMoveEvent(event);
}

void MiscWidget::mousePressEvent(QMouseEvent* event)
{

    if (edit_mode == SINGLE_MODE) {

        if (mode == VelocityEditor) {

            // check whether selection has to be changed.
            bool clickHandlesSelected = false;
            foreach (MidiEvent* event, Selection::instance()->selectedEvents()) {

                int velocity = 0;
                NoteOnEvent* noteOn = dynamic_cast<NoteOnEvent*>(event);

                if (noteOn && noteOn->midiTime() >= matrixWidget->minVisibleMidiTime() && noteOn->midiTime() <= matrixWidget->maxVisibleMidiTime()) {
                    velocity = noteOn->velocity();
                }
                if (velocity > 0) {
                    int h = (height() * velocity) / 128;
                    if (!dragging && mouseInRect(event->x() - LEFT_BORDER_MATRIX_WIDGET, height() - h - 5, WIDTH, 10)) {
                        clickHandlesSelected = true;
                        break;
                    }
                }
            }

            // find event to select
            bool selectedNew = false;
            if (!clickHandlesSelected) {
                QList<MidiEvent*>* list = matrixWidget->velocityEvents();
                foreach (MidiEvent* event, *list) {

                    if (!event->file()->channel(event->channel())->visible()) {
                        continue;
                    }

                    if (event->track()->hidden()) {
                        continue;
                    }

                    int velocity = 0;
                    NoteOnEvent* noteOn = dynamic_cast<NoteOnEvent*>(event);
                    if (noteOn) {
                        velocity = noteOn->velocity();
                    }
                    if (velocity > 0) {
                        int h = (height() * velocity) / 128;
                        if (!dragging && mouseInRect(event->x() - LEFT_BORDER_MATRIX_WIDGET, height() - h - 5, WIDTH, 10)) {
                            matrixWidget->midiFile()->protocol()->startNewAction("Changed selection");
                            ProtocolEntry* toCopy = _dummyTool->copy();
                            EventTool::selectEvent(event, true);
                            matrixWidget->update();
                            selectedNew = true;
                            _dummyTool->protocol(toCopy, _dummyTool);
                            matrixWidget->midiFile()->protocol()->endAction();
                            break;
                        }
                    }
                }
            }

            // if nothing selected deselect all
            if (Selection::instance()->selectedEvents().size() > 0 && !clickHandlesSelected && !selectedNew) {
                matrixWidget->midiFile()->protocol()->startNewAction("Cleared selection");
                ProtocolEntry* toCopy = _dummyTool->copy();
                EventTool::clearSelection();
                _dummyTool->protocol(toCopy, _dummyTool);
                matrixWidget->midiFile()->protocol()->endAction();
                matrixWidget->update();
            }

            // start drag
            if (Selection::instance()->selectedEvents().size() > 0) {
                dragY = mouseY;
                dragging = true;
            }
        } else {

            //other modes
            trackIndex = -1;
            QList<MidiEvent*> accordingEvents;
            QList<QPair<int, int> > track = getTrack(&accordingEvents);
            for (int i = 0; i < track.size(); i++) {

                int xPix = track.at(i).first;
                int yPix = height() - ((double)track.at(i).second / (double)_max) * height();

                if (!dragging && mouseInRect(xPix - 4, yPix - 4, 8, 8)) {
                    trackIndex = i;

                    if (accordingEvents.at(i)) {
                        matrixWidget->midiFile()->protocol()->startNewAction("Changed selection");
                        ProtocolEntry* toCopy = _dummyTool->copy();
                        EventTool::clearSelection();
                        EventTool::selectEvent(accordingEvents.at(i), true, true);
                        matrixWidget->update();
                        _dummyTool->protocol(toCopy, _dummyTool);
                        matrixWidget->midiFile()->protocol()->endAction();
                    }
                    break;
                }
            }

            if (trackIndex > -1) {
                dragging = true;
                dragY = mouseY;
            }
        }

    } else if (edit_mode == MOUSE_MODE) {
        freeHandCurve.clear();
        isDrawingFreehand = true;
    } else if (edit_mode == LINE_MODE) {
        lineX = event->x();
        lineY = event->y();
        isDrawingLine = true;
    }

    if (event->button() == Qt::LeftButton) {
        PaintWidget::mousePressEvent(event);
        return;
    }
}

void MiscWidget::mouseReleaseEvent(QMouseEvent* event)
{

    if (event->button() == Qt::LeftButton) {
        PaintWidget::mouseReleaseEvent(event);
    }

    if (edit_mode == SINGLE_MODE) {
        if (mode == VelocityEditor) {
            if (dragging) {

                dragging = false;

                int dX = dragY - mouseY;

                if (dX < -3 || dX > 3) {
                    matrixWidget->midiFile()->protocol()->startNewAction("Edited velocity");

                    int dV = 127 * dX / height();
                    foreach (MidiEvent* event, Selection::instance()->selectedEvents()) {
                        NoteOnEvent* noteOn = dynamic_cast<NoteOnEvent*>(event);
                        if (noteOn) {
                            int v = dV + noteOn->velocity();
                            if (v > 127) {
                                v = 127;
                            }
                            if (v < 0) {
                                v = 0;
                            }
                            noteOn->setVelocity(v);
                        }
                    }

                    matrixWidget->midiFile()->protocol()->endAction();
                }
            }
        } else {
            // other modes
            if (dragging) {

                QList<MidiEvent*> accordingEvents;
                getTrack(&accordingEvents);
                MidiEvent* ev = accordingEvents.at(trackIndex);

                MidiTrack* track = matrixWidget->midiFile()->track(NewNoteTool::editTrack());
                if (!track) {
                    return;
                }

                int dX = dragY - mouseY;
                if (dX < -3 || dX > 3) {

                    int v = value(mouseY);

                    QString text = "";
                    switch (mode) {
                    case ControllEditor: {
                        text = "Edited Control Change Events";
                        break;
                    }
                    case PitchBendEditor: {
                        text = "Edited Pitch Bend Events";
                        break;
                    }
                    case KeyPressureEditor: {
                        text = "Edited Key Pressure Events";
                        break;
                    }
                    case ChannelPressureEditor: {
                        text = "Edited Channel Pressure Events";
                        break;
                    }
                    }

                    matrixWidget->midiFile()->protocol()->startNewAction(text);

                    if (ev) {
                        if (v < 0)
                            v = 0;
                        switch (mode) {
                        case ControllEditor: {
                            ControlChangeEvent* ctrl = dynamic_cast<ControlChangeEvent*>(ev);
                            if (ctrl) {
                                if (v > 127)
                                    v = 127;
                                ctrl->setValue(v);
                            }
                            break;
                        }
                        case PitchBendEditor: {
                            PitchBendEvent* event = dynamic_cast<PitchBendEvent*>(ev);
                            if (event) {
                                if (v > 16383)
                                    v = 16383;
                                event->setValue(v);
                            }
                            break;
                        }
                        case KeyPressureEditor: {
                            KeyPressureEvent* event = dynamic_cast<KeyPressureEvent*>(ev);
                            if (event) {
                                if (v > 127)
                                    v = 127;
                                event->setValue(v);
                            }
                            break;
                        }
                        case ChannelPressureEditor: {
                            ChannelPressureEvent* event = dynamic_cast<ChannelPressureEvent*>(ev);
                            if (event) {
                                if (v > 127)
                                    v = 127;
                                event->setValue(v);
                            }
                            break;
                        }
                        }

                    } else {

                        MidiTrack* track = matrixWidget->midiFile()->track(NewNoteTool::editTrack());
                        if (!track) {
                            return;
                        }

                        int tick = matrixWidget->minVisibleMidiTime();

                        if (v < 0)
                            v = 0;
                        if (tick < 0)
                            tick = 0;
                        switch (mode) {
                        case ControllEditor: {
                            if (v > 127)
                                v = 127;
                            ControlChangeEvent* ctrl = new ControlChangeEvent(channel, controller, v, track);
                            matrixWidget->midiFile()->channel(channel)->insertEvent(ctrl, tick);
                            break;
                        }
                        case PitchBendEditor: {
                            if (v > 16383)
                                v = 16383;
                            PitchBendEvent* event = new PitchBendEvent(channel, v, track);
                            matrixWidget->midiFile()->channel(channel)->insertEvent(event, tick);
                            break;
                        }
                        case KeyPressureEditor: {
                            if (v > 127)
                                v = 127;
                            KeyPressureEvent* event = new KeyPressureEvent(channel, v, controller, track);
                            matrixWidget->midiFile()->channel(channel)->insertEvent(event, tick);
                            break;
                        }
                        case ChannelPressureEditor: {
                            if (v > 127)
                                v = 127;
                            ChannelPressureEvent* event = new ChannelPressureEvent(channel, v, track);
                            matrixWidget->midiFile()->channel(channel)->insertEvent(event, tick);
                            break;
                        }
                        }
                    }

                    matrixWidget->midiFile()->protocol()->endAction();
                }

                dragging = false;
                trackIndex = -1;
            } else {

                // insert new event
                int tick = matrixWidget->midiFile()->tick(matrixWidget->msOfXPos(mouseX + LEFT_BORDER_MATRIX_WIDGET));
                int v = value(mouseY);

                QString text = "";
                switch (mode) {
                case ControllEditor: {
                    text = "Inserted Control Change Event";
                    break;
                }
                case PitchBendEditor: {
                    text = "Inserted Pitch Bend Event";
                    break;
                }
                case KeyPressureEditor: {
                    text = "Inserted Key Pressure Event";
                    break;
                }
                case ChannelPressureEditor: {
                    text = "Inserted Channel Pressure Event";
                    break;
                }
                }

                matrixWidget->midiFile()->protocol()->startNewAction(text);

                MidiTrack* track = matrixWidget->midiFile()->track(NewNoteTool::editTrack());
                if (!track) {
                    return;
                }
                if (v < 0)
                    v = 0;
                if (tick < 0)
                    tick = 0;
                switch (mode) {
                case ControllEditor: {
                    if (v > 127)
                        v = 127;
                    ControlChangeEvent* ctrl = new ControlChangeEvent(channel, controller, v, track);
                    matrixWidget->midiFile()->channel(channel)->insertEvent(ctrl, tick);
                    break;
                }
                case PitchBendEditor: {
                    if (v > 16383)
                        v = 16383;
                    PitchBendEvent* event = new PitchBendEvent(channel, v, track);
                    matrixWidget->midiFile()->channel(channel)->insertEvent(event, tick);
                    break;
                }
                case KeyPressureEditor: {
                    if (v > 127)
                        v = 127;
                    KeyPressureEvent* event = new KeyPressureEvent(channel, v, controller, track);
                    matrixWidget->midiFile()->channel(channel)->insertEvent(event, tick);
                    break;
                }
                case ChannelPressureEditor: {
                    if (v > 127)
                        v = 127;
                    ChannelPressureEvent* event = new ChannelPressureEvent(channel, v, track);
                    matrixWidget->midiFile()->channel(channel)->insertEvent(event, tick);
                    break;
                }
                }

                matrixWidget->midiFile()->protocol()->endAction();
            }
        }
    }

    QList<QPair<int, int> > toAlign;

    if (edit_mode == MOUSE_MODE || edit_mode == LINE_MODE) {

        // get track data
        if (edit_mode == MOUSE_MODE) {
            if (isDrawingFreehand) {
                isDrawingFreehand = false;
                // process data
                toAlign = freeHandCurve;
                freeHandCurve.clear();
            }
        } else if (edit_mode == LINE_MODE) {
            if (isDrawingLine) {
                // process data
                isDrawingLine = false;
                if (lineX < mouseX) {
                    toAlign.append(QPair<int, int>(lineX, lineY));
                    toAlign.append(QPair<int, int>(mouseX, mouseY));
                } else if (lineX > mouseX) {
                    toAlign.append(QPair<int, int>(mouseX, mouseY));
                    toAlign.append(QPair<int, int>(lineX, lineY));
                }
            }
        }

        if (toAlign.size() > 0) {

            int minTick = matrixWidget->midiFile()->tick(matrixWidget->msOfXPos(toAlign.first().first + LEFT_BORDER_MATRIX_WIDGET));
            int maxTick = matrixWidget->midiFile()->tick(matrixWidget->msOfXPos(toAlign.last().first + LEFT_BORDER_MATRIX_WIDGET));

            // process data
            if (mode == VelocityEditor) {

                // when any events are selected, only use those. Else all
                // in the range
                QList<MidiEvent*> events;
                if (Selection::instance()->selectedEvents().size() > 0) {
                    foreach (MidiEvent* event, Selection::instance()->selectedEvents()) {
                        NoteOnEvent* noteOn = dynamic_cast<NoteOnEvent*>(event);
                        if (noteOn) {
                            if (noteOn->midiTime() >= minTick && noteOn->midiTime() <= maxTick) {
                                events.append(event);
                            }
                        }
                    }
                } else {
                    QList<MidiEvent*>* list = matrixWidget->velocityEvents();
                    foreach (MidiEvent* event, *list) {

                        if (!event->file()->channel(event->channel())->visible()) {
                            continue;
                        }

                        if (event->track()->hidden()) {
                            continue;
                        }

                        NoteOnEvent* noteOn = dynamic_cast<NoteOnEvent*>(event);
                        if (noteOn) {
                            if (noteOn->midiTime() >= minTick && noteOn->midiTime() <= maxTick) {
                                events.append(event);
                            }
                        }
                    }
                }

                if (events.size() > 0) {

                    matrixWidget->midiFile()->protocol()->startNewAction("Changed velocity");

                    // process per event
                    foreach (MidiEvent* event, events) {

                        NoteOnEvent* noteOn = dynamic_cast<NoteOnEvent*>(event);
                        if (noteOn) {

                            int tick = noteOn->midiTime();
                            int x = matrixWidget->xPosOfMs(matrixWidget->midiFile()->msOfTick(tick)) - LEFT_BORDER_MATRIX_WIDGET;
                            double y = interpolate(toAlign, x);

                            int v = 127 * (height() - y) / height();
                            if (v > 127) {
                                v = 127;
                            }

                            noteOn->setVelocity(v);
                        }
                    }
                    matrixWidget->midiFile()->protocol()->endAction();
                }
            } else {

                QString text = "";
                switch (mode) {
                case ControllEditor: {
                    text = "Edited Control Change Events";
                    break;
                }
                case PitchBendEditor: {
                    text = "Edited Pitch Bend Events";
                    break;
                }
                case KeyPressureEditor: {
                    text = "Edited Key Pressure Events";
                    break;
                }
                case ChannelPressureEditor: {
                    text = "Edited Channel Pressure Events";
                    break;
                }
                }

                matrixWidget->midiFile()->protocol()->startNewAction(text);

                // remove old events
                QList<MidiEvent*>* list = matrixWidget->velocityEvents();
                for (int i = 0; i < list->size(); i++) {
                    if (list->at(i) && list->at(i)->channel() == channel) {
                        if (list->at(i)->midiTime() >= minTick && list->at(i)->midiTime() <= maxTick && filter(list->at(i))) {
                            matrixWidget->midiFile()->channel(channel)->removeEvent(list->at(i));
                        }
                    }
                }

                // compute events
                int stepSize = 10;

                int lastValue = -1;
                for (int tick = minTick; tick <= maxTick; tick += stepSize) {

                    int x = matrixWidget->xPosOfMs(matrixWidget->midiFile()->msOfTick(tick)) - LEFT_BORDER_MATRIX_WIDGET;
                    double y = interpolate(toAlign, x);
                    int v = value(y);
                    if ((lastValue != -1) && (lastValue == v)) {
                        continue;
                    }
                    MidiTrack* track = matrixWidget->midiFile()->track(NewNoteTool::editTrack());
                    if (!track) {
                        return;
                    }
                    lastValue = v;
                    switch (mode) {
                    case ControllEditor: {
                        ControlChangeEvent* ctrl = new ControlChangeEvent(channel, controller, v, track);
                        matrixWidget->midiFile()->channel(channel)->insertEvent(ctrl, tick);
                        break;
                    }
                    case PitchBendEditor: {
                        PitchBendEvent* event = new PitchBendEvent(channel, v, track);
                        matrixWidget->midiFile()->channel(channel)->insertEvent(event, tick);
                        break;
                    }
                    case KeyPressureEditor: {
                        KeyPressureEvent* event = new KeyPressureEvent(channel, v, controller, track);
                        matrixWidget->midiFile()->channel(channel)->insertEvent(event, tick);
                        break;
                    }
                    case ChannelPressureEditor: {
                        ChannelPressureEvent* event = new ChannelPressureEvent(channel, v, track);
                        matrixWidget->midiFile()->channel(channel)->insertEvent(event, tick);
                        break;
                    }
                    }
                }

                matrixWidget->midiFile()->protocol()->endAction();
            }
        }
    }
}

int MiscWidget::value(double y)
{
    int v = _max * (height() - y) / height();
    if (v > _max) {
        v = _max;
    }
    return v;
}

double MiscWidget::interpolate(QList<QPair<int, int> > track, int x)
{

    for (int i = 0; i < track.size(); i++) {

        if (track.at(i).first == x) {
            return (double)track.at(i).second;
        }

        if (track.at(i).first > x) {

            if (i == 0) {
                return (double)track.at(i).second;
            } else {

                return (double)track.at(i - 1).second + (double)(track.at(i).second - track.at(i - 1).second) * (double)(x - track.at(i - 1).first) / (double)(track.at(i).first - track.at(i - 1).first);
            }
        }
    }

    return 0;
}

void MiscWidget::leaveEvent(QEvent* event)
{
    resetState();
    PaintWidget::leaveEvent(event);
}

void MiscWidget::resetState()
{

    dragY = -1;
    dragging = false;
    freeHandCurve.clear();
    isDrawingFreehand = false;
    isDrawingLine = false;

    trackIndex = -1;
    update();
}

void MiscWidget::keyPressEvent(QKeyEvent* event)
{
    if (Tool::currentTool()) {
        if (Tool::currentTool()->pressKey(event->key())) {
            repaint();
        }
    }
}

void MiscWidget::keyReleaseEvent(QKeyEvent* event)
{
    if (Tool::currentTool()) {
        if (Tool::currentTool()->releaseKey(event->key())) {
            repaint();
        }
    }
}

QList<QPair<int, int> > MiscWidget::getTrack(QList<MidiEvent*>* accordingEvents)
{

    QList<QPair<int, int> > track;

    // get list of all events in window
    QList<MidiEvent*>* list = matrixWidget->velocityEvents();

    // get all events before the start tick to find out value before start
    int startTick = matrixWidget->minVisibleMidiTime();
    QMultiMap<int, MidiEvent*>* channelEvents = matrixWidget->midiFile()->channel(channel)->eventMap();
    QMultiMap<int, MidiEvent*>::iterator it = channelEvents->upperBound(startTick);

    bool ok = false;
    int valueBefore = _default;
    MidiEvent* evBef = 0;

    if (channelEvents->size() > 0) {
        bool atEnd = false;
        while (!atEnd) {
            if (it != channelEvents->end() && it.key() <= startTick) {
                QPair<int, int> p = processEvent(it.value(), &ok);
                if (ok) {
                    valueBefore = p.second;
                    evBef = it.value();
                    atEnd = true;
                }
            }
            if (it == channelEvents->begin()) {
                atEnd = true;
            } else {
                it--;
            }
        }
    }
    track.append(QPair<int, int>(0, valueBefore));
    if (accordingEvents) {
        accordingEvents->append(evBef);
    }
    // filter and extract values
    for (int i = list->size() - 1; i >= 0; i--) {
        if (list->at(i) && list->at(i)->channel() == channel) {
            QPair<int, int> p = processEvent(list->at(i), &ok);
            if (ok) {
                if (list->at(i)->midiTime() == startTick) {
                    // remove added event
                    track.removeFirst();
                    if (accordingEvents) {
                        accordingEvents->removeFirst();
                    }
                }
                track.append(p);
                if (accordingEvents) {
                    accordingEvents->append(list->at(i));
                }
            }
        }
    }

    return track;
}

bool MiscWidget::filter(MidiEvent* e)
{
    switch (mode) {
    case ControllEditor: {
        ControlChangeEvent* ctrl = dynamic_cast<ControlChangeEvent*>(e);
        if (ctrl && ctrl->control() == controller) {
            return true;
        } else {
            return false;
        }
    }
    case PitchBendEditor: {
        PitchBendEvent* pitch = dynamic_cast<PitchBendEvent*>(e);
        if (pitch) {
            return true;
        } else {
            return false;
        }
    }
    case KeyPressureEditor: {
        KeyPressureEvent* pressure = dynamic_cast<KeyPressureEvent*>(e);
        if (pressure && pressure->note() == controller) {
            return true;
        } else {
            return false;
        }
    }
    case ChannelPressureEditor: {
        ChannelPressureEvent* pressure = dynamic_cast<ChannelPressureEvent*>(e);
        if (pressure) {
            return true;
        } else {
            return false;
        }
    }
    }
    return false;
}

QPair<int, int> MiscWidget::processEvent(MidiEvent* e, bool* isOk)
{

    *isOk = false;
    QPair<int, int> pair(-1, -1);
    switch (mode) {
    case ControllEditor: {
        ControlChangeEvent* ctrl = dynamic_cast<ControlChangeEvent*>(e);
        if (ctrl && ctrl->control() == controller) {
            int x = ctrl->x() - LEFT_BORDER_MATRIX_WIDGET;
            int y = ctrl->value();
            pair.first = x;
            pair.second = y;
            *isOk = true;
        }
        break;
    }
    case PitchBendEditor: {
        PitchBendEvent* pitch = dynamic_cast<PitchBendEvent*>(e);
        if (pitch) {
            int x = pitch->x() - LEFT_BORDER_MATRIX_WIDGET;
            int y = pitch->value();
            pair.first = x;
            pair.second = y;
            *isOk = true;
        }
        break;
    }
    case KeyPressureEditor: {
        KeyPressureEvent* pressure = dynamic_cast<KeyPressureEvent*>(e);
        if (pressure && pressure->note() == controller) {
            int x = pressure->x() - LEFT_BORDER_MATRIX_WIDGET;
            int y = pressure->value();
            pair.first = x;
            pair.second = y;
            *isOk = true;
        }
        break;
    }
    case ChannelPressureEditor: {
        ChannelPressureEvent* pressure = dynamic_cast<ChannelPressureEvent*>(e);
        if (pressure) {
            int x = pressure->x() - LEFT_BORDER_MATRIX_WIDGET;
            int y = pressure->value();
            pair.first = x;
            pair.second = y;
            *isOk = true;
        }
        break;
    }
    }
    return pair;
}

void MiscWidget::computeMinMax()
{
    switch (mode) {
    case ControllEditor: {
        _max = 127;
        _default = 0;
        break;
    }
    case PitchBendEditor: {
        _max = 16383;
        _default = 0x2000;
        break;
    }
    case KeyPressureEditor: {
        _max = 127;
        _default = 0;
        break;
    }
    case ChannelPressureEditor: {
        _max = 127;
        _default = 0;
        break;
    }
    }
}
