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

#include "MatrixWidget.h"
#include <QMessageBox>
#include "InstrumentChooser.h"
#include "SoundEffectChooser.h"
#include "TextEventEdit.h"
#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/ProgChangeEvent.h"
#include "../MidiEvent/ControlChangeEvent.h"
#include "../MidiEvent/PitchBendEvent.h"
#include "../MidiEvent/SysExEvent.h"
#include "../MidiEvent/NoteOnEvent.h"
#include "../MidiEvent/OffEvent.h"
#include "../MidiEvent/TextEvent.h"
#include "../MidiEvent/TempoChangeEvent.h"
#include "../MidiEvent/TimeSignatureEvent.h"
#include "../midi/MidiChannel.h"
#include "../midi/MidiFile.h"
#include "../midi/MidiInput.h"
#include "../midi/MidiPlayer.h"
#include "../midi/MidiTrack.h"
#include "../midi/PlayerThread.h"
#include "../midi/MidiOutput.h"
#include "../protocol/Protocol.h"
#include "../tool/NewNoteTool.h"
#include "../tool/EditorTool.h"
#include "../tool/EventTool.h"
#include "../tool/Selection.h"
#include "../tool/Tool.h"

#ifdef USE_FLUIDSYNTH
#include "../fluid/FluidDialog.h"
#include "../VST/VST.h"
#else
#define DELETE(x) {if(x) delete x; x= NULL;}
#endif

#include <QList>
#include <QtCore/qmath.h>

int _cur_edit = -1;

extern QString *midi_text[8];
extern int text_ev[8];

#define NUM_LINES 139
#define PIXEL_PER_S 100
#define PIXEL_PER_LINE 11
#define PIXEL_PER_EVENT 15

//QByteArray conv_char8_spanish_to_utf8(QByteArray a);


MatrixWidget::MatrixWidget(QWidget* parent)
    : PaintWidget(parent)
{

    pixmap = 0;
    _div = 2;

    for(int n = 0; n < 8; n++) {
        text_ev[n] = -1;
        midi_text[n] = NULL;
    }

    x_mouse = -1;
    y_mouse = -1;
    note_mouse = 0;
    chan_mouse = 0;
    vel_mouse = 0;
    trk_mouse = 0;

    paint_in_use = false;
    disable_ev = false;
    screen_locked = false;
    visible_Controlflag = true;
    visible_PitchBendflag = true;
    visible_TimeLineArea3 = true;
    visible_TimeLineArea4 = true;
    startTimeX = 0;
    startLineY = 50;
    endTimeX = 0;
    endLineY = 0;
    file = 0;
    scaleX = 1;
    pianoEvent = new NoteOnEvent(0, 100, MidiOutput::standardChannel(), 0);
    scaleY = 1;
    lineNameWidth = 110;
    timeHeight = 50;
    currentTempoEvents = new QList<MidiEvent*>;
    currentTimeSignatureEvents = new QList<TimeSignatureEvent*>;
    msOfFirstEventInList = 0;
    objects = new QList<MidiEvent*>;
    velocityObjects = new QList<MidiEvent*>;
    EditorTool::setMatrixWidget(this);
    setMouseTracking(true);
    setFocusPolicy(Qt::ClickFocus);

    setRepaintOnMouseMove(false);
    setRepaintOnMousePress(false);
    setRepaintOnMouseRelease(false);

    connect(MidiPlayer::playerThread(), SIGNAL(timeMsChanged(int)),
        this, SLOT(timeMsChanged(int)));

    pixmap = 0;
    _div = 2;

}

void MatrixWidget::setScreenLocked(bool b)
{
    screen_locked = b;
}

bool MatrixWidget::screenLocked()
{
    return screen_locked;
}

void MatrixWidget::timeMsChanged(int ms, bool ignoreLocked)
{

    if(disable_ev)
        return;

    if (!file)
        return;

    int x = xPosOfMs(ms);

    if ((!screen_locked || ignoreLocked) && (x < lineNameWidth || ms < startTimeX || ms > endTimeX || x > width() - 100)) {

        // return if the last tick is already shown
        if (file->maxTime() <= endTimeX && ms >= startTimeX) {
            repaint();
            return;
        }

        // sets the new position and repaints
        emit scrollChanged(ms, (file->maxTime() - endTimeX + startTimeX), startLineY,
            NUM_LINES - (endLineY - startLineY));
    } else {
        repaint();
    }
}

void MatrixWidget::scrollXChanged(int scrollPositionX)
{
    if(disable_ev)
        return;

    if (!file)
        return;

    startTimeX = scrollPositionX;
    endTimeX = startTimeX + ((width() - lineNameWidth) * 1000) / (PIXEL_PER_S * scaleX);

    // more space than needed: scale x
    if (endTimeX - startTimeX > file->maxTime()) {
        endTimeX = file->maxTime();
        startTimeX = 0;
    } else if (startTimeX < 0) {
        endTimeX -= startTimeX;
        startTimeX = 0;
    } else if (endTimeX > file->maxTime()) {
        startTimeX += file->maxTime() - endTimeX;
        endTimeX = file->maxTime();
    }
    registerRelayout();
    repaint();
}

void MatrixWidget::scrollYChanged(int scrollPositionY)
{
    if(disable_ev)
        return;

    if (!file)
        return;

    startLineY = scrollPositionY;

    double space = height() - timeHeight;
    double lineSpace = scaleY * PIXEL_PER_LINE;
    double linesInWidget = space / lineSpace;
    endLineY = startLineY + linesInWidget;

    if (endLineY > NUM_LINES) {
        int d = endLineY - NUM_LINES;
        endLineY = NUM_LINES;
        startLineY -= d;
        if (startLineY < 0) {
            startLineY = 0;
        }
    }
    registerRelayout();
    repaint();
}

void MatrixWidget::paintEvent(QPaintEvent* /*event*/)
{

    if (!file)
        return;

    if(paint_in_use)
        return;
    paint_in_use = true;

    QPainter* painter = new QPainter(this);
    if(!painter) {
        return;
    }

    QFont font = painter->font();
    font.setPixelSize(12);
    painter->setFont(font);
    painter->setClipping(false);

    bool totalRepaint = !pixmap;

    if (totalRepaint) {

        this->pianoKeys.clear();
        pixmap = new QPixmap(width(), height());
        if(!pixmap)
            return;
        QPainter* pixpainter = new QPainter(pixmap);

        if(!pixpainter) {

            DELETE(pixmap);
            painter->end();
            DELETE(painter);
            return;
        }

        if (!QApplication::arguments().contains("--no-antialiasing")) {
            pixpainter->setRenderHint(QPainter::Antialiasing);
        }

        // dark gray shade
        pixpainter->fillRect(0, 0, width(), height(), Qt::darkGray);

        QFont f = pixpainter->font();
        f.setPixelSize(12);
        pixpainter->setFont(f);
        pixpainter->setClipping(false);

        for (int i = 0; i < objects->length(); i++) {
            objects->at(i)->setShown(false);
            OnEvent* onev = dynamic_cast<OnEvent*>(objects->at(i));
            if (onev && onev->offEvent()) {
                onev->offEvent()->setShown(false);
            }
        }

        objects->clear();
        velocityObjects->clear();
        currentTempoEvents->clear();
        currentTimeSignatureEvents->clear();
        currentDivs.clear();

        startTick = file->tick(startTimeX, endTimeX, &currentTempoEvents,
            &endTick, &msOfFirstEventInList);

        TempoChangeEvent* ev = dynamic_cast<TempoChangeEvent*>(
            currentTempoEvents->at(0));
        if (!ev) {
            painter->fillRect(0, 0, width(), height(), Qt::red);
            pixpainter->end();
            DELETE(pixpainter);
            DELETE(pixmap);
            painter->end();
            DELETE(painter);
            paint_in_use = false;
            return;
        }
        int numLines = endLineY - startLineY;
        if (numLines == 0) {
            pixpainter->end();
            DELETE(pixpainter);
            DELETE(pixmap);
            painter->end();
            DELETE(painter);
            paint_in_use = false;
            return;
        }

        // fill background of the line descriptions
        pixpainter->fillRect(PianoArea, COLOR_BACKGROUND);

        // fill the pianos background white
        int pianoKeys = numLines;
        if (endLineY > 127) {
            pianoKeys -= (endLineY - 127);
        }
        if (pianoKeys > 0) {
            pixpainter->fillRect(0, timeHeight, lineNameWidth - 10,
                pianoKeys * lineHeight(), COLOR_PIANO_NOTE);
        }

        // draw lines, pianokeys and linenames

        for (int i = startLineY; i <= endLineY; i++) {
            if(i < 0) i = 0;
            int startLine = yPosOfLine(i);
            QColor c;
            c = COLOR_PIANO_LINES1;

            if ( !( (1 << (i % 12)) & sharp_strip_mask ) ){
                c = COLOR_PIANO_LINES2;
            }

            if (i > 127) {
                c = COLOR_PIANO_LINES1;

                if (i % 2 == 1) {
                    c = COLOR_PIANO_LINES2;
                }
            }

            pixpainter->fillRect(lineNameWidth, startLine, width(),
                startLine + lineHeight(), c);
        }


#ifdef CUSTOM_MIDIEDITOR_GUI
        // Estwald Color Changes
        static QPixmap p1(":/run_environment/graphics/custom/drum.png");
        int x = width()/2, y = height()/2;
        pixpainter->setOpacity(0.15);
        pixpainter->drawPixmap(x - 144 , y - 139, 400 , 300, p1);
        pixpainter->setOpacity(1);

#endif
        // paint measures and timeline
        pixpainter->fillRect(0, 0, width(), timeHeight, COLOR_BACKGROUND);

        pixpainter->setClipping(true);
        pixpainter->setClipRect(lineNameWidth, 0, width() - lineNameWidth - 2,
            height());

        pixpainter->setPen(Qt::darkGray);
        pixpainter->setBrush(COLOR_MEASURES_BK);
        pixpainter->drawRect(lineNameWidth, 2, width() - lineNameWidth - 1, timeHeight - 2);
        pixpainter->setPen(Qt::black);

        pixpainter->fillRect(0, timeHeight - 3, width(), 3, COLOR_BACKGROUND);

        // paint time (ms)
        int numbers = (width() - lineNameWidth) / 80;
        if (numbers > 0) {
            int step = (endTimeX - startTimeX) / numbers;
            int realstep = 1;
            int nextfak = 2;
            int tenfak = 1;
            while (realstep <= step) {
                realstep = nextfak * tenfak;
                if (nextfak == 1) {
                    nextfak++;
                    continue;
                }
                if (nextfak == 2) {
                    nextfak = 5;
                    continue;
                }
                if (nextfak == 5) {
                    nextfak = 1;
                    tenfak *= 10;
                }
            }
            int startNumber = ((startTimeX) / realstep);
            startNumber *= realstep;
            if (startNumber < startTimeX) {
                startNumber += realstep;
            }
            pixpainter->setPen(COLOR_MEASURES1);
            while (startNumber < endTimeX) {
                int pos = xPosOfMs(startNumber);
                QString text = "";
                int hours = startNumber / (60000 * 60);
                int remaining = startNumber - (60000 * 60) * hours;
                int minutes = remaining / (60000);
                remaining = remaining - minutes * 60000;
                int seconds = remaining / 1000;
                int ms = remaining - 1000 * seconds;

                text += QString::number(hours) + ":";
                text += QString("%1:").arg(minutes, 2, 10, QChar('0'));
                text += QString("%1").arg(seconds, 2, 10, QChar('0'));
                text += QString(".%1").arg(ms / 10, 2, 10, QChar('0'));
                int textlength = QFontMetrics(pixpainter->font()).horizontalAdvance(text);
                if (startNumber > 0) {
                    pixpainter->drawText(pos - textlength / 2, timeHeight / 2 - 6, text);
                }
                pixpainter->drawLine(pos, timeHeight / 2 - 1, pos, timeHeight);
                startNumber += realstep;
            }
        }

        // draw measures
        int measure = file->measure(startTick, endTick, &currentTimeSignatureEvents);

        TimeSignatureEvent* currentEvent = currentTimeSignatureEvents->at(0);
        int i = 0;
        if (!currentEvent) {
            pixpainter->end();
            DELETE(pixpainter);
            DELETE(pixmap);
            painter->end();
            DELETE(painter);
            paint_in_use = false;
            return;
        }
        int tick = currentEvent->midiTime();
        while (tick + currentEvent->ticksPerMeasure() <= startTick) {
            tick += currentEvent->ticksPerMeasure();
        }

        while (tick < endTick) {
            TimeSignatureEvent* measureEvent = currentTimeSignatureEvents->at(i);
            int xfrom = xPosOfMs(msOfTick(tick));
            if(_div < 6)  // no Rhythm Box raster
                currentDivs.append(QPair<int, int>(xfrom, tick));
            measure++;
            int measureStartTick = tick;
            tick += currentEvent->ticksPerMeasure();
            if (i < currentTimeSignatureEvents->length() - 1) {
                if (currentTimeSignatureEvents->at(i + 1)->midiTime() <= tick) {
                    currentEvent = currentTimeSignatureEvents->at(i + 1);
                    tick = currentEvent->midiTime();
                    i++;
                }
            }
            int xto = xPosOfMs(msOfTick(tick));
            pixpainter->setBrush(COLOR_MEASURES2);
            pixpainter->setPen(Qt::NoPen);
            pixpainter->drawRoundedRect(xfrom + 2, timeHeight / 2 + 4, xto - xfrom - 4, timeHeight / 2 - 10, 5, 5);
            if (tick > startTick) {
                pixpainter->setPen(COLOR_MEASURES1);
                if(_div < 6)  // no Rhythm Box raster
                    pixpainter->drawLine(xfrom, timeHeight / 2, xfrom, height());
                else
                    pixpainter->drawLine(xfrom, timeHeight / 2, xfrom, timeHeight / 2 + 16);
                QString text = "Measure " + QString::number(measure - 1);
                int textlength = QFontMetrics(pixpainter->font()).horizontalAdvance(text);
                if (textlength > xto - xfrom) {
                    text = QString::number(measure - 1);
                    textlength = QFontMetrics(pixpainter->font()).horizontalAdvance(text);
                }
                int pos = (xfrom + xto) / 2;
                pixpainter->setPen(Qt::white);
                pixpainter->drawText(pos - textlength / 2, timeHeight - 9, text);

                if (_div >= 0) {

                    if(_div >= 6) { // Rhythm Box raster

                        if(!bpm_rhythm_ms)
                            bpm_rhythm_ms = 250;

                        double time_bmp = bpm_rhythm_ms;

                        QPen oldPen = pixpainter->pen();
                        QPen dashPen = QPen(Qt::lightGray, 1, Qt::DashLine);

                        pixpainter->setPen(dashPen);

                        double time_count = (time_bmp - fmod((double) (msOfTick(measureStartTick)), time_bmp));
                        time_count+= msOfTick(measureStartTick);

                        double time_end = msOfTick(measureStartTick + measureEvent->ticksPerMeasure());

                        while (time_count <= time_end) {     
                            int xDiv = xPosOfMs((int)time_count); // high precision timing
                            currentDivs.append(QPair<int, int>(xDiv, file->tick(time_count)/*divTick*/));
                            time_count+= time_bmp;
                            pixpainter->drawLine(xDiv, timeHeight, xDiv, height());

                        }

                        pixpainter->setPen(oldPen);
                    } else {
                        double metronomeDiv = 4 / (double)qPow(2, _div);
                        int ticksPerDiv = metronomeDiv * file->ticksPerQuarter();
                        int startTickDiv = ticksPerDiv;
                        QPen oldPen = pixpainter->pen();
                        QPen dashPen = QPen(Qt::lightGray, 1, Qt::DashLine);
                        pixpainter->setPen(dashPen);
                        while (startTickDiv < measureEvent->ticksPerMeasure()) {
                            int divTick = startTickDiv + measureStartTick;
                            int xDiv = xPosOfMs(msOfTick(divTick));
                            currentDivs.append(QPair<int, int>(xDiv, divTick));
                            pixpainter->drawLine(xDiv, timeHeight, xDiv, height());
                            startTickDiv += ticksPerDiv;
                        }

                        pixpainter->setPen(oldPen);
                    }

                }
            }
        }

        // line between time texts and matrixarea
        pixpainter->setPen(Qt::gray);
        pixpainter->drawLine(0, timeHeight, width(), timeHeight);
        pixpainter->drawLine(lineNameWidth, timeHeight, lineNameWidth, height());

        pixpainter->setPen(Qt::black);

        // paint the events
        pixpainter->setClipping(true);
        pixpainter->setClipRect(lineNameWidth, timeHeight, width() - lineNameWidth,
            height() - timeHeight);

        listAreaSel.clear();

        // other tracks
        for (int i = 0; i < 19; i++) {
            paintChannel(pixpainter, i, 0);
        }

        // current track
        int ch = NewNoteTool::editChannel();

        for (int i = 0; i < 19; i++) {
            if(i != ch)
                paintChannel(pixpainter, i, 1);
        }

        // current channel
        paintChannel(pixpainter, ch, 2);

        // draw shadow background areas
        pixpainter->setPen(Qt::NoPen);
        QColor c(0xa0c0c0);
        c.setAlpha(128);
        pixpainter->setBrush(QBrush(c, Qt::Dense4Pattern));

        QMultiMap<int, int>::iterator itx = listAreaSel.lowerBound(0);

        while (itx != listAreaSel.end()) {

            int x = itx.key();
            int w = itx.value();
            if(w > -1) {

                pixpainter->drawRect(x, 0, w, height());

            }

            itx++;
        }

        listAreaSel.clear();

        pixpainter->setClipping(false);

        pixpainter->setPen(Qt::black);

        pixpainter->end();
        delete pixpainter;
    }

    painter->drawPixmap(0, 0, *pixmap);

    painter->setRenderHint(QPainter::Antialiasing);

    // draw the piano / linenames
    for (int i = startLineY; i <= endLineY; i++) {
        int startLine = yPosOfLine(i);
        if (i >= 0 && i <= 127) {
            paintPianoKey(painter, 127 - i, 0, startLine,
                lineNameWidth, lineHeight());
        } else {
            QString text = "";
            switch (i) {
            case MidiEvent::CONTROLLER_LINE: {
                text = "Control Change";
                break;
            }
            case MidiEvent::TEMPO_CHANGE_EVENT_LINE: {
                text = "Tempo Change";
                break;
            }
            case MidiEvent::TIME_SIGNATURE_EVENT_LINE: {
                text = "Time Signature";
                break;
            }
            case MidiEvent::KEY_SIGNATURE_EVENT_LINE: {
                text = "Key Signature.";
                break;
            }
            case MidiEvent::PROG_CHANGE_LINE: {
                text = "Program Change";
                break;
            }
            case MidiEvent::KEY_PRESSURE_LINE: {
                text = "Key Pressure";
                break;
            }
            case MidiEvent::CHANNEL_PRESSURE_LINE: {
                text = "Channel Pressure";
                break;
            }
            case MidiEvent::TEXT_EVENT_LINE: {
                text = "Text";
                break;
            }
            case MidiEvent::PITCH_BEND_LINE: {
                text = "Pitch Bend";
                break;
            }
            case MidiEvent::SYSEX_LINE: {
                text = "System Exclusive";
                break;
            }
            case MidiEvent::UNKNOWN_LINE: {
                text = "(Unknown)";
                break;
            }
            }
            painter->setPen(COLOR_INK_EVENT);
            font = painter->font();
            font.setPixelSize(10);
            painter->setFont(font);
            int textlength = QFontMetrics(font).horizontalAdvance(text);
            painter->drawText(lineNameWidth - 15 - textlength, startLine + lineHeight(), text);
        }
    }
    if (Tool::currentTool()) {
        painter->setClipping(true);
        painter->setClipRect(ToolArea);
        Tool::currentTool()->draw(painter);
        painter->setClipping(false);
    }

    if (enabled && mouseInRect(TimeLineArea)) {
        painter->setPen(Qt::red);
        painter->drawLine(mouseX, 0, mouseX, height());
        painter->setPen(Qt::black);
    }

    if (MidiPlayer::isPlaying()) {
        painter->setPen(Qt::red);
        int x = xPosOfMs(MidiPlayer::timeMs());
        if (x >= lineNameWidth) {
            painter->drawLine(x, 0, x, height());
        }
        painter->setPen(Qt::black);
    }

    // paint the cursorTick of file
    if (midiFile()->cursorTick() >= startTick && midiFile()->cursorTick() <= endTick) {
        painter->setPen(Qt::darkGray);
        int x = xPosOfMs(msOfTick(midiFile()->cursorTick()));
        painter->drawLine(x, 0, x, height());
        QPointF points[3] = {
            QPointF(x - 8, timeHeight / 2 + 2),
            QPointF(x + 8, timeHeight / 2 + 2),
            QPointF(x, timeHeight - 2),
        };

        painter->setBrush(QBrush(QColor(194, 230, 255), Qt::SolidPattern));

        painter->drawPolygon(points, 3);
        painter->setPen(Qt::gray);
    }

    // paint the pauseTick of file if >= 0
    if (!MidiPlayer::isPlaying() && midiFile()->pauseTick() >= startTick && midiFile()->pauseTick() <= endTick) {
        int x = xPosOfMs(msOfTick(midiFile()->pauseTick()));

        QPointF points[3] = {
            QPointF(x - 8, timeHeight / 2 + 2),
            QPointF(x + 8, timeHeight / 2 + 2),
            QPointF(x, timeHeight - 2),
        };

        painter->setBrush(QBrush(Qt::gray, Qt::SolidPattern));

        painter->drawPolygon(points, 3);
    }

    // paint edit cursor
#ifdef USE_FLUIDSYNTH
    if(!fluid_control) _cur_edit = -666;
#endif

    if (_cur_edit >= startTick && _cur_edit <= endTick) {
        painter->setPen(Qt::darkGray);
        int x = xPosOfMs(msOfTick(_cur_edit));
        painter->drawLine(x, 0, x, height());
        QPointF points[3] = {
            QPointF(x - 5, timeHeight / 2 + 2),
            QPointF(x + 5, timeHeight / 2 + 2),
            QPointF(x, timeHeight - 2),
        };

        painter->setBrush(QBrush(QColor(255, 230, 194 ), Qt::SolidPattern));

        painter->drawPolygon(points, 3);
        painter->setPen(Qt::gray);
    }


    // border
    painter->setPen(Qt::gray);
    painter->drawLine(width() - 1, height() - 1, lineNameWidth, height() - 1);
    painter->drawLine(width() - 1, height() - 1, width() - 1, 2);

    // if the recorder is recording, show red circle
    if (MidiInput::recording()) {
        painter->setBrush(Qt::red);
        painter->drawEllipse(width() - 20, timeHeight + 5, 15, 15);
    }

    // shows karaoke text events
    if(visible_karaoke && (midi_text[0] || midi_text[1] || midi_text[2]
            || midi_text[3] || midi_text[4] || midi_text[5]
            || midi_text[6] || midi_text[7])) {

        font.setPixelSize(24);
        painter->setFont(font);

        QString str, str2;

        int n, m;

        for(n = 0; n < 8; n++) {
            if(midi_text[n] && MidiPlayer::timeMs() <= (msOfTick(text_ev[n]) )) {
                break;
            }
        }

        for(m = 0; m <= n; m++) {
            if(m < 8 && midi_text[m]) str+= *midi_text[m];
        }

        QFontMetrics fm(font);
        QRect r1 = fm.boundingRect(str);
        QRect r2;

        int xx = (width() - r1.width()) / 2;
        int yy = timeHeight + 4 + (80 - r1.height())/2;

        if(n < 8) {

            if((n + 1) < 8) {
                for(m = n + 1; m < 8; m++) {
                    if(midi_text[m]) str2+= *midi_text[m];
                }

                r2 = fm.boundingRect(str2);

                xx = (width() - r1.width() - r2.width() - 8) / 2;
                if(r2.height() > r1.height()) yy = timeHeight + 4 + (80 - r2.height())/2;
            }
        }

        int ancho = r1.width() + r2.width() + 8 + 16;

        if(ancho < 480) ancho = 480;

        painter->setPen(Qt::black);
        QBrush brush(QColor(0x10, 0x30, 0x20), Qt::DiagCrossPattern);
        painter->setBrush(brush);

        painter->setClipping(false);
        painter->fillRect((width() - ancho - 8) / 2, timeHeight, ancho + 8, 88, QColor(0, 0xc0, 0xc0));
        painter->fillRect((width() - ancho) / 2, timeHeight + 4, ancho, 80, QColor(0, 0x20, 0x10));

        painter->drawRect((width() - ancho) / 2, timeHeight + 4, ancho, 80);
        painter->setBrush(Qt::black);
        painter->setPen(Qt::white);

        painter->setPen(QColor(0xFF, 0xFF, 0x80));
        if(n < 8) {

            painter->drawText(xx, yy, r1.width() + 8, 80, 0, str);
            painter->setPen(Qt::white);
            painter->drawText(xx + r1.width() + 8, yy, r2.width() + 8, 80, 0, str2);

        } else
            painter->drawText(xx, yy, r1.width() + 8, 80, Qt::AlignLeft, str);
    }

    if(x_mouse >= 0 && note_mouse >= 0 && !MidiPlayer::isPlaying() ) {

        painter->setPen(Qt::black);
        if(chan_mouse >= 0 && chan_mouse < 16 && OctaveChan_MIDI[chan_mouse])
            painter->setBrush(QColor(0xffffc0));
        else
            painter->setBrush(Qt::white);

        QFont f = painter->font();
        QFont f2(f);
        f2.setBold(true);
        f2.setPixelSize(12);
        painter->setFont(f2);

        static QString note[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

        QString text;

        if(note_mouse < 128)
            text = "(" + note[note_mouse %12] + " " + QString::number(note_mouse/12 - 1) + ") " +
                    "ch: " + QString::number(chan_mouse) + " vel: " + QString::number(vel_mouse) + " track: " + QString::number(trk_mouse);
        else text = "(event) ch: " + QString::number(chan_mouse)  + " track: " + QString::number(trk_mouse);
        int textlength = QFontMetrics(f2).horizontalAdvance(text);

        painter->drawLine(x_mouse, y_mouse, x_mouse, y_mouse - 24);
        painter->drawRoundedRect(x_mouse - textlength/2 - 8, y_mouse - 20 - 12, textlength + 16, 16, 5, 5);
        painter->drawText(x_mouse - textlength/2, y_mouse - 20, text);
        painter->setFont(f);
    }

    painter->end();
    delete painter;

    // if MouseRelease was not used, delete it
    mouseReleased = false;

    if (totalRepaint) {
        emit objectListChanged();
    }

    paint_in_use = false;
}

void MatrixWidget::paintChannel(QPainter* painter, int channel, int view)
{
    // Estwald Visible
    bool chan16hide = false;
    if(!file)
        return;

    if(!file->MultitrackMode && view != 0) // ignore view 1-2
        return;

    if (channel >= 16 && !file->channel(channel)->visible()) {
       chan16hide = true;
    }

    int track_index = file->track(NewNoteTool::editTrack())->device_index();

    QColor cC = *file->channel(channel)->color();

    ////////////////////////////////////////

    // draw shadow background for selected events
    if(this->shadow_selection)
    {
        QMultiMap<int, MidiEvent*>* map = file->channelEvents(channel);
        QMultiMap<int, MidiEvent*>::iterator it = map->lowerBound(0/*startTick*/);

        while (it != map->end() && it.key() <= endTick) {
            MidiEvent* event = it.value();

            if (!file->channel(channel)->visible(file->MultitrackMode ? event->track()->device_index() : 0)) {
                if(channel != 16) {
                    it++;
                    continue;
                }
            }

            if((!file->MultitrackMode && view != 0) ||
                    (file->MultitrackMode && ((view == 0 && event->track()->number() == NewNoteTool::editTrack()) ||
                                              (view != 0 && event->track()->number() != NewNoteTool::editTrack())))) {
                it++;
                continue;
            }

            OnEvent* onEvent = dynamic_cast<OnEvent*>(event);
            if(onEvent) {
                if(onEvent->offEvent()) {
                    if(onEvent->offEvent()->midiTime() < startTick) {
                        it++;
                        continue;
                    }
                    // veeery long event note pass here
                } else if(onEvent->midiTime() < startTick) {
                    it++;
                    continue;
                }
            } else if(!event || event->midiTime() < startTick) { // skip events before of startTick
                it++;
                continue;
            }

            int tick = event->midiTime();


            if (tick >= startTick && tick <= endTick) {

                OffEvent* offEvent = dynamic_cast<OffEvent*>(event);
                OnEvent* onEvent = dynamic_cast<OnEvent*>(event);

                int x, width;

                if (onEvent || offEvent) {

                    if (onEvent) {
                        offEvent = onEvent->offEvent();
                    } else if (offEvent) {
                        onEvent = dynamic_cast<OnEvent*>(offEvent->onEvent());
                    }

                    width = xPosOfMs(msOfTick(offEvent->midiTime())) - xPosOfMs(msOfTick(onEvent->midiTime()));
                    x = xPosOfMs(msOfTick(onEvent->midiTime()));

                    event = onEvent;
                    if (objects->contains(event)) {
                        it++;
                        continue;
                    }

                    if (!(event->track()->hidden())) {

                        if (Selection::instance()->selectedEvents().contains(event)) {
                            if(listAreaSel.value(x-2) < (width + 4))
                                listAreaSel.replace(x-2, (width + 4));

                        }
                    }
                }
            }

            it++;
        }

        // reduce shadow background areas
        QMultiMap<int, int>::iterator itx = listAreaSel.lowerBound(0);

        while (itx != listAreaSel.end()) {

            int x = itx.key();
            int w = itx.value();
            QMultiMap<int, int>::iterator itx2 = listAreaSel.lowerBound(0);
            bool modified = false;

            while (itx2 != listAreaSel.end()) {
                int x2 = itx2.key();
                int w2 = itx2.value();

                if(itx != itx2 && x2 > x && x2 < (x + w + 32 /* attractor */)) {
                     w = ((x2 + w2) > (x + w)) ? x2 + w2  - x : w;

                     listAreaSel.replace(x, w);
                     listAreaSel.remove(x2);
                     modified = true;

                }

                itx2++;
            }

            if(!modified)
                itx++;
            else
                itx = listAreaSel.lowerBound(0);

        }

    }

    // filter events
    QMultiMap<int, MidiEvent*>* map = file->channelEvents(channel);


    QMultiMap<int, MidiEvent*>::iterator it = map->lowerBound(0/*startTick*/);
    while (it != map->end() && it.key() <= endTick) {
        MidiEvent* event = it.value();
        SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

        if (!file->channel(channel)->visible(file->MultitrackMode ? event->track()->device_index() : 0)) {
            if(channel != 16) {
                it++;
                continue;
            }

        }

        if((!file->MultitrackMode && view != 0) ||
                (file->MultitrackMode && ((view == 0 && (event->track()->number() == NewNoteTool::editTrack() || channel == 16)) ||
                                          (view == 1 && channel != 16 && event->track()->number() != NewNoteTool::editTrack())))) {
            it++;
            continue;
        }

        if(sys) {
            QByteArray c = sys->data();
#ifdef USE_FLUIDSYNTH
            if(c[1]== (char) 0x66 && c[2]==(char) 0x66 && c[3]=='V') {
#ifndef VISIBLE_VST_SYSEX
                it++; continue;
#endif
            } else if(chan16hide && c[1]== (char) 0x66 && c[2]==(char) 0x66 && c[3]=='W') {
                if(!file->channel(c[0] & 15)->visible((c[0]/32) % 3)) {
                    it++; continue;
                }
            } else if(chan16hide && c[1]== (char) 0x66 && c[2]==(char) 0x66 && (c[3] & 0x70) == 0x70) {
                if(!file->channel(c[3] & 15)->visible()) {
                    it++; continue;
                }
            } else if(chan16hide) {
                it++; continue;
            }
#else
            if(chan16hide) {
                it++; continue;
            }
#endif
        }
        OnEvent* onEvent = dynamic_cast<OnEvent*>(event);
        if(onEvent) {
            if(onEvent->offEvent()) {
                if(onEvent->offEvent()->midiTime() < startTick) {
                    it++;
                    continue;
                }
                // veeery long event note pass here
            } else if(onEvent->midiTime() < startTick) {
                it++;
                continue;
            }
        } else if(!event || event->midiTime() < startTick) { // skip events before of startTick
            it++;
            continue;
        }

        if(!(event->track()->hidden())) {// draw ctrl lines

            if(event->channel() == 16 || !file->MultitrackMode || (file->MultitrackMode
                                         && (event->track()->number() == NewNoteTool::editTrack()))) {

                ProgChangeEvent* prg = dynamic_cast<ProgChangeEvent*>(event);
                ControlChangeEvent* ctrl = dynamic_cast<ControlChangeEvent*>(event);
                PitchBendEvent* pitch = dynamic_cast<PitchBendEvent*>(event);
                SysExEvent* sys = dynamic_cast<SysExEvent*>(event);
                TextEvent* text = dynamic_cast<TextEvent*>(event);

                if(chan16hide && !sys) {
                    it++; continue;
                }


                if(!visible_Controlflag) ctrl = NULL;
                if(!visible_PitchBendflag) pitch = NULL;
                if(!visible_TimeLineArea3) {prg = NULL; sys = NULL;}
                if(!visible_TimeLineArea4) text = NULL;

                if(text && (text->type() == TextEvent::TEXT ||
                            text->type() == TextEvent::MARKER ||
                            text->type() == TextEvent::LYRIK ||
                            text->type() == TextEvent::TRACKNAME)){
                    int x = xPosOfMs(msOfTick(text->midiTime()));
                    int wd = 16;

                    painter->setPen(QPen(QColor(0xf08080), 1, Qt::DashLine));
                    painter->setClipping(false);
                    painter->drawLine(x, 50-24, x, yPosOfLine(event->line()));
                    painter->setPen(Qt::SolidLine);

                    if((x)<lineNameWidth) {wd+=(x)-lineNameWidth;x=lineNameWidth;}
                    else if((x+8)>this->width()) {wd-=(x+8)-this->width();}

                    painter->setPen(0x804040);
                    painter->setBrush(QColor(0x60C08080));
                    painter->drawRect(x, 50-24, wd, 8);
                    painter->setClipping(true);
                }

                if(prg){
                    int x = xPosOfMs(msOfTick(prg->midiTime()));
                    int wd = 16;

                    painter->setPen(QPen(QColor(0xf08080), 1, Qt::DashLine));
                    painter->setClipping(false);
                    painter->drawLine(x, 50-16, x, yPosOfLine(event->line()));
                    painter->setPen(Qt::SolidLine);

                    if((x)<lineNameWidth) {wd+=(x)-lineNameWidth;x=lineNameWidth;}
                    else if((x+8)>this->width()) {wd-=(x+8)-this->width();}

                    painter->setPen(0x804040);
                    painter->setBrush(QColor(0xf08080));
                    painter->drawRect(x, 50-16, wd, 8);
                    painter->setClipping(true);
                }

                if(sys){
                    QByteArray d= sys->data();

                    if(((d[0]==(char) track_index && (d[3] & 0xF0)==0x70) || (d[0]==(char) 0x10 && d[3]=='R'))
                            && d[1]==(char) 0x66 && d[2]==(char) 0x66 &&
                            (d[3]=='R' || (d[3] & 0xF0)==0x70)) {
                        int x = xPosOfMs(msOfTick(sys->midiTime()));
                        int wd = 16;

                        painter->setPen(QPen(QColor(0x908090), 1, Qt::DashLine));
                        painter->setClipping(false);
                        painter->drawLine(x, 50-16, x, yPosOfLine(event->line()));
                        painter->setPen(Qt::SolidLine);

                        if((x)<lineNameWidth) {wd+=(x)-lineNameWidth;x=lineNameWidth;}
                        else if((x+8)>this->width()) {wd-=(x+8)-this->width();}

                        painter->setPen(0x504050);
                        painter->setBrush(QColor(0x908090));
                        painter->drawRect(x, 50-16, wd, 8);
                        painter->setClipping(true);
                    } else if(d[1]==(char) 0x66 && d[2]==(char) 0x66 &&
                              d[3]=='W') {
                        int x = xPosOfMs(msOfTick(sys->midiTime()));
                        int wd = 16;

                        painter->setPen(QPen(QColor(0x908090), 1, Qt::DashLine));
                        painter->setClipping(false);
                        painter->drawLine(x, 50-16, x, yPosOfLine(event->line()));
                        painter->setPen(Qt::SolidLine);

                        if((x)<lineNameWidth) {wd+=(x)-lineNameWidth;x=lineNameWidth;}
                        else if((x+8)>this->width()) {wd-=(x+8)-this->width();}

                        painter->setPen(0x504050);
                        painter->setBrush(QColor(0x90c0c0));
                        painter->drawRect(x, 50-16, wd, 8);
                        painter->setClipping(true);
                    }
                }

                if(ctrl && ctrl->control() != 0 && ctrl->control() != 3){
                    int x = xPosOfMs(msOfTick(ctrl->midiTime()));
                    int wd = 16;

                    painter->setPen(QPen(QColor(0xff69b4), 1, Qt::DashLine));
                    painter->drawLine(x, 50, x, yPosOfLine(event->line()));
                    painter->setClipping(false);
                    painter->setPen(Qt::SolidLine);

                    if((x)<lineNameWidth) {wd+=(x)-lineNameWidth;x=lineNameWidth;}
                    else if((x+8)>this->width()) {wd-=(x+8)-this->width();}
                    // painter->fillRect(x, 50-8, wd, 8 , 0xff69b4);
                    painter->setPen(0x803050);
                    painter->setBrush(QColor(0xff69b4));
                    painter->drawRect(x, 50-8, wd, 8);
                    painter->setClipping(true);
                }


                if(pitch){
                    int x = xPosOfMs(msOfTick(pitch->midiTime()));
                    int wd = 16;

                    painter->setPen(QPen(QColor(0xff69b4), 1, Qt::DashLine));
                    painter->drawLine(x, 50, x, yPosOfLine(event->line()));
                    painter->setClipping(false);
                    painter->setPen(Qt::SolidLine);
                    if((x)<lineNameWidth) {wd+=(x)-lineNameWidth;x=lineNameWidth;}
                    else if((x+8)>this->width()) {wd-=(x+8)-this->width();}
                    painter->setPen(0x803050);
                    painter->setBrush(QColor(0xff69b4));
                    painter->drawRect(x, 50-8, wd, 8);
                    painter->setClipping(true);
                }
            }
        }


        if (eventInWidget(event)) {
            // insert all Events in objects, set their coordinates
            // Only onEvents are inserted. When there is an On
            // and an OffEvent, the OnEvent will hold the coordinates
            int line = event->line();


            OffEvent* offEvent = dynamic_cast<OffEvent*>(event);
            OnEvent* onEvent = dynamic_cast<OnEvent*>(event);

            int x, width;
            int y = yPosOfLine(line);
            int height = lineHeight();

            int displaced = 0;

            if (onEvent || offEvent) {

                // visual octave correction for notes
                if(channel >= 0 && channel < 16 && OctaveChan_MIDI[channel]) {
                    if((channel == 9 && event->track()->fluid_index() == 0) ||
                            ((channel == 9 && event->track()->fluid_index() != 0
                        && MidiOutput::isFluidsynthConnected(event->track()->device_index()) && event->file()->DrumUseCh9))) {
                        //OctaveChan_MIDI[channel] = 0;
                        line = event->line();
                    } else {
                        line = (127 - line) + OctaveChan_MIDI[channel] * 12;
                        if(line < 0) line = 0;
                        if(line > 127) line = 127;
                        line = 127 - line;
                        y = yPosOfLine(line);
                        displaced = 1;
                    }
                }

                if (onEvent) {
                    offEvent = onEvent->offEvent();
                } else if (offEvent) {
                    onEvent = dynamic_cast<OnEvent*>(offEvent->onEvent());
                }

                width = xPosOfMs(msOfTick(offEvent->midiTime())) - xPosOfMs(msOfTick(onEvent->midiTime()));
                x = xPosOfMs(msOfTick(onEvent->midiTime()));

                event = onEvent;
                if (objects->contains(event)) {
                    it++;
                    continue;
                }
            } else {
                width = PIXEL_PER_EVENT;
                x = xPosOfMs(msOfTick(event->midiTime()));
            }

            event->setX(x);
            event->setY(y);
            event->setWidth(width);
            event->setHeight(height);


            if (!(event->track()->hidden())) {

                if(/*channel == 16 || */!file->MultitrackMode ||
                        (file->MultitrackMode && event->track()->number() == NewNoteTool::editTrack())) {
                    // disabled from CUSTOM 11
                    if (0 && !_colorsByChannels) {
                        cC = *event->track()->color();
                    } else
                        cC = *file->channel(event->channel())->color();

                    cC.setAlpha(255);

                } else {
                    cC = QColor(0xb0b0b0);
                    cC.setAlpha(32);
                }

                if(displaced)
                    event->draw2(painter, cC, Qt::Dense1Pattern, (view == 2));
                else
                    event->draw(painter, cC, (view == 2));

                if (Selection::instance()->selectedEvents().contains(event)) {
                    QColor cs(Qt::gray);
                    cs.setAlpha(cC.alpha());
                    painter->setPen(cs);
                    painter->drawLine(lineNameWidth, y, this->width(), y);
                    painter->drawLine(lineNameWidth, y + height, this->width(), y + height);
                    painter->setPen(Qt::black);
                }
                objects->prepend(event);
            }
        }

        if (!(event->track()->hidden())) {
            // append event to velocityObjects if its not a offEvent and if it
            // is in the x-Area
            OffEvent* offEvent = dynamic_cast<OffEvent*>(event);
            if (!offEvent && event->midiTime() >= startTick && event->midiTime() <= endTick && !velocityObjects->contains(event)) {
                event->setX(xPosOfMs(msOfTick(event->midiTime())));

                velocityObjects->prepend(event);
            }
        }
        it++;


    }


}

void MatrixWidget::paintPianoKey(QPainter* painter, int number, int x, int y,
    int width, int height)
{
    int borderRight = 10;
    width = width - borderRight;
    if (number >= 0 && number <= 127) {

        double scaleHeightBlack = 0.5;
        double scaleWidthBlack = 0.6;

        bool isBlack = false;
        bool blackOnTop = false;
        bool blackBeneath = false;
        QString name = "";

        switch (number % 12) {
        case 0: {
            // C
            blackOnTop = true;
            name = "";
            int i = number / 12;
            //if(i<4){
            //	name="C";{
            //		for(int j = 0; j<3-i; j++){
            //			name+="'";
            //		}
            //	}
            //} else {
            //	name = "c";
            //	for(int j = 0; j<i-4; j++){
            //		name+="'";
            //	}
            //}
            name = "C" + QString::number(i - 1);
            break;
        }
        // Cis
        case 1: {
            isBlack = true;
            break;
        }
        // D
        case 2: {
            blackOnTop = true;
            blackBeneath = true;
            break;
        }
        // Dis
        case 3: {
            isBlack = true;
            break;
        }
        // E
        case 4: {
            blackBeneath = true;
            break;
        }
        // F
        case 5: {
            blackOnTop = true;
            break;
        }
        // fis
        case 6: {
            isBlack = true;
            break;
        }
        // G
        case 7: {
            blackOnTop = true;
            blackBeneath = true;
            break;
        }
        // gis
        case 8: {
            isBlack = true;
            break;
        }
        // A
        case 9: {
            blackOnTop = true;
            blackBeneath = true;
            break;
        }
        // ais
        case 10: {
            isBlack = true;
            break;
        }
        // H
        case 11: {
            blackBeneath = true;
            break;
        }
        }

        if (127 - number == startLineY) {
            blackOnTop = false;
        }

        bool selected = mouseY >= y && mouseY <= y + height && mouseX > lineNameWidth && mouseOver;
        foreach (MidiEvent* event, Selection::instance()->selectedEvents()) {
            if (event->line() == 127 - number) {
                selected = true;
                break;
            }
        }

        QPolygon keyPolygon;

        bool inRect = false;
        if (isBlack) {
            painter->drawLine(x, y + height / 2, x + width, y + height / 2);
            y += (height - height * scaleHeightBlack) / 2;
            QRect playerRect;
            playerRect.setX(x);
            playerRect.setY(y);
            playerRect.setWidth(width * scaleWidthBlack);
            playerRect.setHeight(height * scaleHeightBlack + 0.5);
            QColor c = Qt::black;
            if (mouseInRect(playerRect)) {
                c = QColor(200, 200, 200);
                inRect = true;
            }
            painter->fillRect(playerRect, c);

            keyPolygon.append(QPoint(x, y));
            keyPolygon.append(QPoint(x, y + height * scaleHeightBlack));
            keyPolygon.append(QPoint(x + width * scaleWidthBlack, y + height * scaleHeightBlack));
            keyPolygon.append(QPoint(x + width * scaleWidthBlack, y));
            pianoKeys.insert(number, playerRect);

        } else {

            if (!blackOnTop) {
                keyPolygon.append(QPoint(x, y));
                keyPolygon.append(QPoint(x + width, y));
            } else {
                keyPolygon.append(QPoint(x, y - height * scaleHeightBlack / 2));
                keyPolygon.append(QPoint(x + width * scaleWidthBlack, y - height * scaleHeightBlack / 2));
                keyPolygon.append(QPoint(x + width * scaleWidthBlack, y - height * scaleHeightBlack));
                keyPolygon.append(QPoint(x + width, y - height * scaleHeightBlack));
            }
            if (!blackBeneath) {
                painter->drawLine(x, y + height, x + width, y + height);
                keyPolygon.append(QPoint(x + width, y + height));
                keyPolygon.append(QPoint(x, y + height));
            } else {
                keyPolygon.append(QPoint(x + width, y + height + height * scaleHeightBlack));
                keyPolygon.append(QPoint(x + width * scaleWidthBlack, y + height + height * scaleHeightBlack));
                keyPolygon.append(QPoint(x + width * scaleWidthBlack, y + height + height * scaleHeightBlack / 2));
                keyPolygon.append(QPoint(x, y + height + height * scaleHeightBlack / 2));
            }
            inRect = mouseInRect(x, y, width, height);
            pianoKeys.insert(number, QRect(x, y, width, height));
        }

        if (isBlack) {
            if (inRect) {
                painter->setBrush(Qt::lightGray);
            } else if (selected) {
                painter->setBrush(Qt::darkGray);
            } else {
                painter->setBrush(Qt::black);
            }
        } else {
            if (inRect) {
                painter->setBrush(Qt::darkGray);
            } else if (selected) {
                painter->setBrush(Qt::lightGray);
            } else {
                painter->setBrush(COLOR_PIANO_NOTE);
            }
        }
        painter->setPen(Qt::darkGray);
        painter->drawPolygon(keyPolygon, Qt::OddEvenFill);

        if (name != "") {
            painter->setPen(Qt::gray);
            int textlength = QFontMetrics(painter->font()).horizontalAdvance(name);
            painter->drawText(x + width - textlength - 2, y + height - 1, name);
            painter->setPen(Qt::black);
        }
        if (inRect && enabled) {
            // mark the current Line
            QColor lineColor = QColor(0, 0, 100, 40);
            painter->fillRect(x + width + borderRight, yPosOfLine(127 - number),
                this->width() - x - width - borderRight, height, lineColor);
        }
    }
}

void MatrixWidget::setFile(MidiFile* f)
{
    file = f;

    scaleX = 1;
    scaleY = 1;

    for(int n = 0; n < 8; n++) {
        text_ev[n] = -1;
        if(midi_text[n]) delete midi_text[n];
        midi_text[n] = NULL;
    }

    x_mouse = -1;
    y_mouse = -1;
    note_mouse = 0;
    chan_mouse = 0;
    vel_mouse = 0;
    trk_mouse = 0;

    startTimeX = 0;
    // Roughly vertically center on Middle C.
    startLineY = 50;

    connect(file->protocol(), SIGNAL(actionFinished()), this,
        SLOT(registerRelayout()));
    connect(file->protocol(), SIGNAL(actionFinished()), this, SLOT(update()));

    calcSizes();

    // scroll down to see events
    int maxNote = -1;
    for (int channel = 0; channel < 16; channel++) {

        QMultiMap<int, MidiEvent*>* map = file->channelEvents(channel);
        if(!map)
            continue;

        QMultiMap<int, MidiEvent*>::iterator it = map->lowerBound(0);

        while (it != map->end()) {
            NoteOnEvent* onev = dynamic_cast<NoteOnEvent*>(it.value());
            if (onev && eventInWidget(onev)) {
                if (onev->line() < maxNote || maxNote < 0) {
                    maxNote = onev->line();
                }
            }
            it++;
        }
    }

    if (maxNote - 5 > 0) {
        startLineY = maxNote - 5;
    }

    calcSizes();

}

void MatrixWidget::calcSizes()
{
    if(disable_ev)
        return;

    if (!file) {
        return;
    }

    int time = file->maxTime();
    int timeInWidget = ((width() - lineNameWidth) * 1000) / (PIXEL_PER_S * scaleX);

    ToolArea = QRectF(lineNameWidth, timeHeight, width() - lineNameWidth,
        height() - timeHeight);
    PianoArea = QRectF(0, timeHeight, lineNameWidth, height() - timeHeight);
    TimeLineArea = QRectF(lineNameWidth, 0, width() - lineNameWidth, timeHeight);
    TimeLineArea2 = QRectF(lineNameWidth, timeHeight-7, width() - lineNameWidth, 8);
    TimeLineArea3 = QRectF(lineNameWidth, timeHeight-15, width() - lineNameWidth, 8);
    TimeLineArea4 = QRectF(lineNameWidth, timeHeight-23, width() - lineNameWidth, 8);

    scrollXChanged(startTimeX);
    scrollYChanged(startLineY);

    emit sizeChanged(time - timeInWidget, NUM_LINES - endLineY + startLineY, startTimeX,
        startLineY);
}

MidiFile* MatrixWidget::midiFile()
{
    return file;
}

void MatrixWidget::mouseMoveEvent(QMouseEvent* event)
{

    if (!file)
        return;

    PaintWidget::mouseMoveEvent(event);

    if (!enabled) {
        return;
    }

#ifdef IS_QT5
    int Mx = event->position().x();
    int My = event->position().y();
#else
    int Mx = event->position().x();
    int My = event->position().y();
#endif

    if (!MidiPlayer::isPlaying() && !MidiInput::recording() && Tool::currentTool()) {
        Tool::currentTool()->move(Mx, My);
    }

    if (!MidiPlayer::isPlaying() && !MidiInput::recording()) {

        // get current note to display it
        x_mouse = -1;
        note_mouse = -1;

        foreach (MidiEvent* ev, *(activeEvents())) {

            NoteOnEvent* on = dynamic_cast<NoteOnEvent*>(ev);

            if(on && mouseX >= on->x() - 2 && mouseX < on->x() + on->width() + 2 &&
                    mouseY >= on->y() && mouseY <= on->y() + on->height()) {

                if(1) {
                    x_mouse = on->x() + on->width() / 2;
                    y_mouse = on->y();
                    note_mouse = on->note();
                    chan_mouse = ev->channel();
                    vel_mouse = on->velocity();
                    trk_mouse = on->track()->number();
                }

            } else if(ev->line() > 127 && mouseX >= ev->x() - 2 && mouseX < ev->x() + ev->width() + 2 &&
                      mouseY >= ev->y() && mouseY <= ev->y() + ev->height()) {

                  if(1) {
                      x_mouse = ev->x() + ev->width() / 2;
                      y_mouse = ev->y();
                      note_mouse = ev->line();
                      chan_mouse = ev->channel();
                      vel_mouse = 0;
                      trk_mouse = ev->track()->number();
                  }

              }
        }

        repaint();
    }
}

void MatrixWidget::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);
    calcSizes();
}

int MatrixWidget::xPosOfMs(int ms)
{
    return lineNameWidth + (ms - startTimeX) * (width() - lineNameWidth) / (endTimeX - startTimeX);
}

int MatrixWidget::yPosOfLine(int line)
{
    return timeHeight + (line - startLineY) * lineHeight();
}

double MatrixWidget::lineHeight()
{
    if (endLineY - startLineY == 0)
        return 0;
    return (double)(height() - timeHeight) / (double)(endLineY - startLineY);
}

void MatrixWidget::enterEvent(QEvent* event)
{
    PaintWidget::enterEvent(event);
    if (Tool::currentTool()) {
        Tool::currentTool()->enter();
        if (enabled) {
            update();
        }
    }
}
void MatrixWidget::leaveEvent(QEvent* event)
{
    PaintWidget::leaveEvent(event);
    if (Tool::currentTool()) {
        Tool::currentTool()->exit();
        if (enabled) {
            update();
        }
    }
}

int MatrixWidget::clicked = 0;
QTimer MatrixWidget::click_timer;

void MatrixWidget::remote_click() {

    if(disable_ev) {
        clicked = 0;
    }

    if(clicked == 1) {
        clicked = 0;
        if(Tool::currentTool()->isStandardTool()) {
           qWarning("special click");
           Tool::currentTool()->press(true);
        }
    }
}
void MatrixWidget::stop_remote_click() {
    click_timer.stop();
    clicked = 0;
}

void MatrixWidget::mousePressEvent(QMouseEvent* event)
{

    static int _locked = 0;

    PaintWidget::mousePressEvent(event);
    if (!MidiPlayer::isPlaying() && !MidiInput::recording() && Tool::currentTool() && mouseInRect(ToolArea)) {
        if((event->buttons() == Qt::LeftButton)) {
            if(!clicked) {
                MatrixWidget::stop_remote_click();
                clicked = 1;
                MatrixWidget::click_timer.singleShot(1200, this, SLOT(remote_click()));
            }
        }

        if (Tool::currentTool()->press(event->buttons() == Qt::LeftButton)) {
            if (enabled) {

                if(cursor() == Qt::SizeHorCursor && _locked == 0) { // 1

                    _locked = 1;

                    int tick = file->tick(msOfXPos(mouseX));

                    int x = mouseX;
                    int y = mouseY;

                    int breaked = 0;

                    QList<MidiEvent*> selected = Selection::instance()->selectedEvents();

                    for(int chan = 0; chan < 16; chan++) { // 2
                        if(file->channel(chan)->visible()) { //3
                            foreach (MidiEvent* event, file->channel(chan)->eventMap()->values()) {  // 4
                                NoteOnEvent* noteOn = dynamic_cast<NoteOnEvent*>(event);
                                if(noteOn && noteOn->shown() && noteOn->offEvent() &&
                                        noteOn->midiTime() <= tick
                                       && noteOn->offEvent()->midiTime() >= tick && !selected.contains(event)) {  // 5

                                    int yy =  noteOn->y();

                                    if(y >= yy && y < (yy + noteOn->height())
                                            && x >= noteOn->x() && x < (noteOn->x() + noteOn->width())) {  // 6

                                        if (!selected.contains(event)) {  // 7

                                            int tmid = 0x7fffffff;
                                            int tx = 0x7fffffff;
                                            breaked = 1;
                                            foreach (MidiEvent* event2, Selection::instance()->selectedEvents()) {
                                                if(event2->midiTime() < tmid) {
                                                    tx = event2->x(); tmid = event2->midiTime();
                                                }
                                            }

                                            if(tmid != 0x7fffffff) {
                                                tmid -= noteOn->midiTime();
                                                tx -= noteOn->x();

                                                int selected = Selection::instance()->selectedEvents().size();
                                                midiFile()->protocol()->startNewAction("Move events aligned (" + QString::number(selected) + ")");

                                                foreach (MidiEvent* event2, Selection::instance()->selectedEvents()) {
                                                    event2->setMidiTime(event2->midiTime() - tmid);
                                                    event2->setX(event2->x() - tx);
                                                    NoteOnEvent* noteOn2 = dynamic_cast<NoteOnEvent*>(event2);
                                                    if(noteOn2 && noteOn2->offEvent()) {

                                                        noteOn2->offEvent()->setMidiTime(noteOn2->offEvent()->midiTime() - tmid);
                                                        noteOn2->offEvent()->setX(noteOn2->offEvent()->x() - tx);

                                                    }
                                                }
                                                midiFile()->protocol()->endAction();
                                            }
                                            break;
                                        } // 7

                                    }  // 6
                                } // 5
                            } // 4
                        } // 3

                        if(breaked) break;
                    } // 2

                    _locked = 0;
                } // 1

                update();
            }
        }
    }

    if (enabled && (!MidiPlayer::isPlaying() && !MidiInput::recording()) && (mouseInRect(PianoArea))) {
        foreach (int key, pianoKeys.keys()) {
            bool inRect = mouseInRect(pianoKeys.value(key));
            if (inRect) {
                // play note
                pianoEvent->setNote(key);
                pianoEvent->setChannel(NewNoteTool::editChannel(), false);

                MidiPlayer::play(pianoEvent);

            }


        }
    } else if (enabled && (!MidiPlayer::isPlaying() && !MidiInput::recording()) && mouseInRect(TimeLineArea2)
               && event->buttons() == Qt::LeftButton) {
        int tick = file->tick(msOfXPos(mouseX));
        int dtick= file->tick(150); // range

        for(int chan = 0; chan < 16; chan++) {

            //if(file->channel(chan)->visible())
            foreach (MidiEvent* event2, file->channel(chan)->eventMap()->values()) {
                int track_index = file->MultitrackMode ? event2->track()->device_index() : 0;

                if(file->channel(chan)->visible(track_index) && (!file->MultitrackMode || (file->MultitrackMode
                        && (event2->track()->number() == NewNoteTool::editTrack()))))

                    if(event2->midiTime() >= tick - dtick && event2->midiTime() < tick + dtick) {


                        ControlChangeEvent* ctrl = dynamic_cast<ControlChangeEvent*>(event2);
                        PitchBendEvent* pitch = dynamic_cast<PitchBendEvent*>(event2);

                        if(!visible_Controlflag) ctrl = NULL;
                        if(!visible_PitchBendflag) pitch = NULL;

                        if ((ctrl && ctrl->control() == 1  && ctrl->channel() == chan) ||
                                (ctrl && ctrl->control() == 10 && ctrl->channel() == chan) ||
                                (ctrl && ctrl->control() == 7  && ctrl->channel() == chan) ||
                                (ctrl && ctrl->control() == 64 && ctrl->channel() == chan) ||
                                (ctrl && ctrl->control() == 66 && ctrl->channel() == chan) ||
                                (ctrl && ctrl->control() == 72 && ctrl->channel() == chan) ||
                                (ctrl && ctrl->control() == 73 && ctrl->channel() == chan) ||
                                (ctrl && ctrl->control() == 75 && ctrl->channel() == chan) ||
                                (ctrl && ctrl->control() == 91 && ctrl->channel() == chan) ||
                                (ctrl && ctrl->control() == 93 && ctrl->channel() == chan) ||
                                (pitch && pitch->channel() == chan)) {

                            _cur_edit = event2->midiTime();
                            SoundEffectChooser* d = new SoundEffectChooser(file, chan, this, SOUNDEFFECTCHOOSER_EDITALL, _cur_edit);
                            d->exec();
                            delete d;
                            _cur_edit = -1;
                            break;
                        }

                    }

            }
        }

        if (enabled) {
            update();
        }
        mouseX = mouseY = 0;
    } else if (enabled && (!MidiPlayer::isPlaying() && !MidiInput::recording()) && visible_TimeLineArea3 && mouseInRect(TimeLineArea3)
              && event->buttons() == Qt::LeftButton) {
       int tick = file->tick(msOfXPos(mouseX));
       int wtick = file->tick(msOfXPos(mouseX+16))-tick;
       int dtick= file->tick(150);
#ifdef USE_FLUIDSYNTH
       int track_index2 = file->track(NewNoteTool::editTrack())->device_index();
#endif
       foreach (MidiEvent* event2, *(file->eventsBetween(tick-dtick, tick+dtick))) {
// Estwald Visible
           bool show = true;
           int chan = event2->channel();

            int track_index = file->MultitrackMode ? event2->track()->device_index() : 0;
            if(!(chan == 16 || !file->MultitrackMode || (file->MultitrackMode
                                    && (event2->track()->number() == NewNoteTool::editTrack()))))
                continue;

#ifdef USE_FLUIDSYNTH
           if(!file->channel(chan)->visible(track_index) && chan != 16)
               show = false;
#else
           if(!file->channel(chan)->visible(track_index))
               show = false;
#endif
           if(tick>=event2->midiTime() && tick<=(event2->midiTime()+wtick) && show) {

               ProgChangeEvent* prg = dynamic_cast<ProgChangeEvent*>(event2);
#ifdef USE_FLUIDSYNTH

                SysExEvent* sys = dynamic_cast<SysExEvent*>(event2);

                if(sys){

                    QByteArray d= sys->data();
// Estwald Visible

                    if(chan == 16 && !file->channel(chan)->visible()) {

                        if(d[1]== (char) 0x66 && d[2]==(char) 0x66 && d[3]=='W') {
                            if(!file->channel(d[0] & 15)->visible()) {
                                continue;
                            }
                        } else if(d[1]== (char) 0x66 && d[2]==(char) 0x66 && (d[3] & 0x70) == 0x70) {
                            if(!file->channel(d[3] & 15)->visible(d[0] % 3)) {
                                continue;
                            }
                        }
                    }


                    if(d[1]==(char) 0x66 && d[2]==(char) 0x66 &&
                                          d[3]=='W') {
                        file->setCursorTick(sys->midiTime());
                        VST_proc::VST_LoadParameterStream(sys->save());
                        VST_chan vst(main_widget, d[0], 1);
                       // vst.exec();
                    } else if((((d[0]==(char) track_index2) && (d[3] & 0xF0)==(char) 0x70)
                               || (d[0]==(char) 0x10 && d[3]=='R'))
                            && d[1]==(char) 0x66 && d[2]==(char) 0x66 &&
                      (d[3]=='R' || (d[3] & 0xF0)==(char) 0x70)) {

                        MainWindow::FluidControl();

                        if(fluid_control) {
                            _cur_edit = event2->midiTime();
                            fluid_control->LoadSelectedPresset(event2->midiTime());
                        }
                    }
                }


#endif

               if (prg) {
                   int bank=-1;
                   // comprueba si tiene un ctrl proximo

                   foreach (MidiEvent* event3, *(file->eventsBetween(tick-dtick, tick+dtick))) {
                       ControlChangeEvent* toFind = dynamic_cast<ControlChangeEvent*>(event3);
                       if (toFind && event3->channel()==event2->channel()
                               && toFind->control()==0x0) {
                           bank=toFind->value();break;
                       }
                   }
                   if(bank<0) { // si no hay banco, busca desde el inicio
                       bank=0;
                       foreach (MidiEvent* event3, *(file->eventsBetween(0, tick))) {
                           ControlChangeEvent* toFind = dynamic_cast<ControlChangeEvent*>(event3);
                           if (toFind && event3->channel()==event2->channel()
                                   && toFind->control()==0x0) {
                               bank=toFind->value();break;
                           }
                       }
                   }
                   _cur_edit = event2->midiTime();
                   InstrumentChooser* d = new InstrumentChooser(file, event2->channel(), event2->track(), this, 1, event2->midiTime(), prg->program(), bank);
                   d->setModal(true);
                   d->exec();
                   delete d;
                   _cur_edit = -1;

               }

           }

       }
       if (enabled) {
           update();
       }
       mouseX = mouseY = 0;
   }
   if (enabled && (!MidiPlayer::isPlaying() && !MidiInput::recording()) && visible_TimeLineArea4 && mouseInRect(TimeLineArea4)
              && event->buttons() == Qt::LeftButton) {
       int tick = file->tick(msOfXPos(mouseX));
       int wtick = file->tick(msOfXPos(mouseX+16))-tick;
       int dtick= file->tick(150);

       foreach (MidiEvent* event2, *(file->eventsBetween(tick-dtick, tick+dtick))) {

           int track_index = file->MultitrackMode ? event2->track()->device_index() : 0;
           if(!(!file->MultitrackMode || (file->MultitrackMode
                                   && (event2->track()->number() == NewNoteTool::editTrack()))))
               continue;

           if(tick>=event2->midiTime() && tick<=(event2->midiTime()+wtick)
                   && file->channel(event2->channel())->visible(track_index)) {

               TextEvent* text = dynamic_cast<TextEvent*>(event2);

               if (text && (text->type() == TextEvent::TEXT ||
                            text->type() == TextEvent::MARKER ||
                            text->type() == TextEvent::LYRIK||
                            text->type() == TextEvent::TRACKNAME)) {

                   _cur_edit = event2->midiTime();
                   TextEventEdit* d = new TextEventEdit(file, event2->channel(), this,
                                        (text->type() == TextEvent::TEXT)
                                            ? TEXTEVENT_EDIT_TEXT
                                            : ((text->type() == TextEvent::LYRIK)
                                               ? TEXTEVENT_EDIT_LYRIK
                                               : ((text->type() == TextEvent::TRACKNAME)
                                                  ? TEXTEVENT_EDIT_TRACK_NAME
                                                  : TEXTEVENT_EDIT_MARKER)));


                   d->textEdit->setText(text->text());
                   d->setModal(true);
                   if(d->exec() ==QDialog::Accepted) {
                       file->protocol()->startNewAction("Text Event editor");
                       if(d->delBox->isChecked())
                           file->channel(event2->channel())->removeEvent(event2);
                       else
                           text->setText(d->textEdit->text());
                       file->protocol()->endAction();

                   }
                   delete d;
                   _cur_edit = -1;

               }

           }

       }
       if (enabled) {
           update();
       }
       mouseX = mouseY = 0;
   }
}
void MatrixWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        MatrixWidget::stop_remote_click();
    }

    PaintWidget::mouseReleaseEvent(event);

    if (!MidiPlayer::isPlaying() && !MidiInput::recording() && Tool::currentTool() && mouseInRect(ToolArea)) {
        if (Tool::currentTool()->release()) {
            if (enabled) {
                update();
            }
        }
    } else if (!MidiPlayer::isPlaying() && !MidiInput::recording() && Tool::currentTool()) {
        if (Tool::currentTool()->releaseOnly()) {
            if (enabled) {
                update();
            }
        }
    }
}


void MatrixWidget::takeKeyPressEvent(QKeyEvent* event)
{
    if(disable_ev)
        return;

    if (Tool::currentTool()) {
        if (Tool::currentTool()->pressKey(event->key())) {
            repaint();
        }
    }
}

void MatrixWidget::takeKeyReleaseEvent(QKeyEvent* event)
{
    if(disable_ev)
        return;

    if (Tool::currentTool()) {
        if (Tool::currentTool()->releaseKey(event->key())) {
            repaint();
        }
    }
}

QList<MidiEvent*>* MatrixWidget::activeEvents()
{
    return objects;
}

QList<MidiEvent*>* MatrixWidget::velocityEvents()
{
    return velocityObjects;
}

int MatrixWidget::msOfXPos(int x)
{
    return startTimeX + ((x - lineNameWidth) * (endTimeX - startTimeX)) / (width() - lineNameWidth);
}

int MatrixWidget::msOfTick(int tick)
{
    return file->msOfTick(tick, currentTempoEvents, msOfFirstEventInList);
}

int MatrixWidget::timeMsOfWidth(int w)
{
    return (w * (endTimeX - startTimeX)) / (width() - lineNameWidth);
}

bool MatrixWidget::eventInWidget(MidiEvent* event)
{
    if(!event)
        return false;
    NoteOnEvent* on = dynamic_cast<NoteOnEvent*>(event);
    OffEvent* off = dynamic_cast<OffEvent*>(event);
    if (on) {
        off = on->offEvent();
    } else if (off) {
        on = dynamic_cast<NoteOnEvent*>(off->onEvent());
    }
    if (on && off) {
        int line = off->line();
        int tick = on->midiTime();
        int tick2 = off->midiTime();
        int channel = event->channel();

        // visual octave correction for notes
        if(channel >= 0 && channel < 16 && OctaveChan_MIDI[channel]) {
            if((channel == 9 && event->track()->fluid_index() == 0) ||
                    ((channel == 9 && event->track()->fluid_index() != 0
                && MidiOutput::isFluidsynthConnected(event->track()->device_index()) && event->file()->DrumUseCh9))) {
                line = event->line();
            } else {
                line = (127 - line) + OctaveChan_MIDI[channel] * 12;
                if(line < 0) line = 0;
                if(line > 127) line = 127;
                line = 127 - line;
            }
        }


        bool offIn = line >= startLineY && line <= endLineY
                && tick2 >= startTick && tick2 <= endTick;

        line = on->line();

        // visual octave correction for notes
        // visual octave correction for notes
        if(channel >= 0 && channel < 16 && OctaveChan_MIDI[channel]) {
            if((channel == 9 && event->track()->fluid_index() == 0) ||
                    ((channel == 9 && event->track()->fluid_index() != 0
                && MidiOutput::isFluidsynthConnected(event->track()->device_index()) && event->file()->DrumUseCh9))) {
                line = event->line();
            } else {
                line = (127 - line) + OctaveChan_MIDI[channel] * 12;
                if(line < 0) line = 0;
                if(line > 127) line = 127;
                line = 127 - line;
            }
        }

        bool onIn = line >= startLineY && line <= endLineY &&
                ((tick >= startTick && tick <= endTick) ||
                 (tick2 >= endTick && tick <= startTick)); // veeery long notes

        off->setShown(offIn);
        on->setShown(onIn);

        return offIn || onIn;

    } else {
        int line = event->line();
        int tick = event->midiTime();
        bool shown = line >= startLineY && line <= endLineY
                && tick >= startTick && tick <= endTick;
        event->setShown(shown);

        return shown;
    }
}

int MatrixWidget::lineAtY(int y)
{
    return (y - timeHeight) / lineHeight() + startLineY;
}

void MatrixWidget::zoomStd()
{
    scaleX = 1;
    scaleY = 1;
    calcSizes();
}

void MatrixWidget::zoomHorIn()
{
    scaleX += 0.1;
    calcSizes();
}

void MatrixWidget::zoomHorOut()
{
    if (scaleX >= 0.2) {
        scaleX -= 0.1;
        calcSizes();
    }
}

void MatrixWidget::zoomVerIn()
{
    scaleY += 0.1;
    calcSizes();
}

void MatrixWidget::zoomVerOut()
{
    if (scaleY >= 0.2) {
        scaleY -= 0.1;
        if (height() <= NUM_LINES * lineHeight() * scaleY / (scaleY + 0.1)) {
            calcSizes();
        } else {
            scaleY += 0.1;
        }
    }
}

void MatrixWidget::mouseDoubleClickEvent(QMouseEvent* /*event*/)
{
    if (mouseInRect(TimeLineArea)) {

        int ms = msOfXPos(mouseX);
        if(EventTool::magnetEnabled()) {
            int size_x = divs().at(1).second - divs().at(0).second;
            if(size_x < 5)
                size_x = 5;

            size_x = file->msOfTick(size_x);
            ms-= ms % size_x;
        }
        int tick = file->tick(ms);

        file->setCursorTick(tick);

#ifdef USE_FLUIDSYNTH
        if(fluid_control) {
            fluid_control->UpdateCursor();
            if(!fluid_control->EditMode())
                _cur_edit = -666;
        } else
            _cur_edit = -666;
#endif

        update();
    }
}

void MatrixWidget::registerRelayout()
{
    if(pixmap)
        delete pixmap;
    pixmap = 0;
}

int MatrixWidget::minVisibleMidiTime()
{
    return startTick;
}

int MatrixWidget::maxVisibleMidiTime()
{
    return endTick;
}

void MatrixWidget::wheelEvent(QWheelEvent* event)
{
    /*
     * Qt has some underdocumented behaviors for reporting wheel events, so the
     * following were determined empirically:
     *
     * 1.  Some platforms use pixelDelta and some use angleDelta; you need to
     *     handle both.
     *
     * 2.  The documentation for angleDelta is very convoluted, but it boils
     *     down to a scaling factor of 8 to convert to pixels.  Note that
     *     some mouse wheels scroll very coarsely, but this should result in an
     *     equivalent amount of movement as seen in other programs, even when
     *     that means scrolling by multiple lines at a time.
     *
     * 3.  When a modifier key is held, the X and Y may be swapped in how
     *     they're reported, but which modifiers these are differ by platform.
     *     If you want to reserve the modifiers for your own use, you have to
     *     counteract this explicitly.
     *
     * 4.  A single-dimensional scrolling device (mouse wheel) seems to be
     *     reported in the Y dimension of the pixelDelta or angleDelta, but is
     *     subject to the same X/Y swapping when modifiers are pressed.
     */

    Qt::KeyboardModifiers km = event->modifiers();
    QPoint pixelDelta = event->pixelDelta();
    int pixelDeltaX = pixelDelta.x();
    int pixelDeltaY = pixelDelta.y();

    if ((pixelDeltaX == 0) && (pixelDeltaY == 0)) {
        QPoint angleDelta = event->angleDelta();
        pixelDeltaX = angleDelta.x() / 8;
        pixelDeltaY = angleDelta.y() / 8;
    }

    int horScrollAmount = 0;
    int verScrollAmount = 0;

    if (km) {
        int pixelDeltaLinear = pixelDeltaY;
        if (pixelDeltaLinear == 0) pixelDeltaLinear = pixelDeltaX;

        if (km == Qt::ShiftModifier) {
            if (pixelDeltaLinear > 0) {
                zoomVerIn();
            } else if (pixelDeltaLinear < 0) {
                zoomVerOut();
            }
        } else if (km == Qt::ControlModifier) {
            if (pixelDeltaLinear > 0) {
                zoomHorIn();
            } else if (pixelDeltaLinear < 0) {
                zoomHorOut();
            }
        } else if (km == Qt::AltModifier) {
            horScrollAmount = pixelDeltaLinear;
        }
    } else {
        horScrollAmount = pixelDeltaX;
        verScrollAmount = pixelDeltaY;
    }

    if (file) {
        int maxTimeInFile = file->maxTime();
        int widgetRange = endTimeX - startTimeX;

        if (horScrollAmount != 0) {
            int scroll = -1 * horScrollAmount * widgetRange / 1000;

            int newStartTime = startTimeX + scroll;

            if(x_mouse >= 0)
                x_mouse = -1;

            scrollXChanged(newStartTime);
            emit scrollChanged(startTimeX, maxTimeInFile - widgetRange, startLineY, NUM_LINES - (endLineY - startLineY));
        }

        if (verScrollAmount != 0) {
            int newStartLineY = startLineY - (verScrollAmount / (scaleY * PIXEL_PER_LINE));

            if (newStartLineY < 0)
                newStartLineY = 0;

            if(x_mouse >= 0)
                x_mouse = -1;

            // endline too large handled in scrollYchanged()
            scrollYChanged(newStartLineY);
            emit scrollChanged(startTimeX, maxTimeInFile - widgetRange, startLineY, NUM_LINES - (endLineY - startLineY));
        }
    }
}

void MatrixWidget::keyPressEvent(QKeyEvent* event)
{
    takeKeyPressEvent(event);
}

void MatrixWidget::keyReleaseEvent(QKeyEvent* event)
{
    takeKeyReleaseEvent(event);
}

void MatrixWidget::setColorsByChannel()
{
    _colorsByChannels = true;
}
void MatrixWidget::setColorsByTracks()
{
    _colorsByChannels = false;
}

bool MatrixWidget::colorsByChannel()
{
    return _colorsByChannels;
}

void MatrixWidget::setDiv(int div)
{
    if(disable_ev)
        return;
    _div = div;
    registerRelayout();
    update();
}

QList<QPair<int, int> > MatrixWidget::divs()
{
    return currentDivs;
}

int MatrixWidget::div()
{
    return _div;
}



