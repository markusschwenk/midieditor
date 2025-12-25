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

#ifndef MATRIXWIDGET_H_
#define MATRIXWIDGET_H_

#include "PaintWidget.h"

#include <QApplication>
#include <QColor>
#include <QMap>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QResizeEvent>
#include <QWidget>
#ifdef IS_QT5
#include <QTextCodec>
#endif
#include "../tool/EventMoveTool.h"

#undef CUSTOM_MIDIEDITOR_GUI
#define CUSTOM_MIDIEDITOR_GUI "By Estwald"

#ifndef CUSTOM_MIDIEDITOR_GUI
#undef COLOR_BACKGROUND
#define COLOR_BACKGROUND   QApplication::palette().window()
#define COLOR_PIANO_LINES1 QColor(194, 230, 255)
#define COLOR_PIANO_LINES2 QColor(234, 246, 255)
#define COLOR_PIANO_NOTE   Qt::white
#define COLOR_BACK_RASTER  QColor(234, 246, 255)
#define COLOR_VEL_RASTER   QColor(194, 230, 255)
#define COLOR_INK_EVENT    Qt::darkGray
#define COLOR_MEASURES1    Qt::gray
#define COLOR_MEASURES2    Qt::lightGray
#define COLOR_MEASURES_BK  Qt::white
#else
// Estwald Color Changes
#define COLOR_PIANO_LINES1 QColor(0xe0f0d0)
#define COLOR_PIANO_LINES2 QColor(234, 246, 255)
#define COLOR_PIANO_NOTE   QColor(0xffffe0)
#undef COLOR_BACKGROUND
#define COLOR_BACKGROUND   background3 //QColor(0xf03030)
#define COLOR_VEL_RASTER   Qt::darkGray
#define COLOR_BACK_RASTER  QColor(0xe0e0f0)
#define COLOR_INK_EVENT    QColor(0xe0e0f0)
#define COLOR_MEASURES1    Qt::black
#define COLOR_MEASURES2    Qt::black
#define COLOR_MEASURES_BK  QColor(0xffffe0)
#endif

class MidiFile;
class TempoChangeEvent;
class TimeSignatureEvent;
class MidiEvent;
class GraphicObject;
class NoteOnEvent;

class MatrixWidget : public PaintWidget {

    Q_OBJECT

public:
    MatrixWidget(QWidget* parent = 0);
    void setFile(MidiFile* file);
    MidiFile* midiFile();
    QList<MidiEvent*>* activeEvents();
    QList<MidiEvent*>* velocityEvents();

    int bpm_rhythm_ms;
    double lineHeight();
    int lineAtY(int y);
    int msOfXPos(int x);
    int timeMsOfWidth(int w);
    bool eventInWidget(MidiEvent* event);
    int yPosOfLine(int line);
    void setScreenLocked(bool b);
    bool screenLocked();
    int minVisibleMidiTime();
    int maxVisibleMidiTime();

    void setColorsByChannel();
    void setColorsByTracks();
    bool colorsByChannel();

    int msOfTick(int tick);
    int xPosOfMs(int ms);
    QList<QPair<int, int> > divs();

    bool visible_Controlflag;
    bool visible_PitchBendflag;
    bool visible_TimeLineArea3;
    bool visible_TimeLineArea4;

    bool visible_karaoke;

    bool shadow_selection;

    bool disable_ev;

    void DeletePixmap() {

        delete pixmap;
        pixmap = NULL;

    }

public slots:
    void remote_click();
    void scrollXChanged(int scrollPositionX);
    void scrollYChanged(int scrollPositionY);
    void zoomHorIn();
    void zoomHorOut();
    void zoomVerIn();
    void zoomVerOut();
    void zoomStd();
    void timeMsChanged(int ms, bool ignoreLocked = false);
    void registerRelayout();
    void calcSizes();
    void takeKeyPressEvent(QKeyEvent* event);
    void takeKeyReleaseEvent(QKeyEvent* event);
    void setDiv(int div);
    int div();
    static void stop_remote_click();

signals:
    void sizeChanged(int maxScrollTime, int maxScrollLine, int valueX,
        int valueY);
    void objectListChanged();
    void scrollChanged(int startMs, int maxMs, int startLine, int maxLine);

protected:
    void paintEvent(QPaintEvent* /*event*/);
    void mouseMoveEvent(QMouseEvent* event);
    void resizeEvent(QResizeEvent* event);
    void enterEvent(QEvent* event);
    void leaveEvent(QEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseDoubleClickEvent(QMouseEvent* /*event*/);
    void mouseReleaseEvent(QMouseEvent* event);
    void keyPressEvent(QKeyEvent* e);
    void keyReleaseEvent(QKeyEvent* event);
    void wheelEvent(QWheelEvent* event);

private:
    bool paint_in_use;
    void paintChannel(QPainter* painter, int channel, int view = 0);
    void paintPianoKey(QPainter* painter, int number, int x, int y,
        int width, int height);

    int startTick, endTick, startTimeX, endTimeX, startLineY, endLineY,
        lineNameWidth, timeHeight, msOfFirstEventInList;
    double scaleX, scaleY;
    MidiFile* file;

    QRectF ToolArea, PianoArea, TimeLineArea, TimeLineArea2,
        TimeLineArea3, TimeLineArea4;
    bool screen_locked;
    static int clicked;
    static QTimer click_timer;
    int x_mouse;
    int y_mouse;
    int note_mouse;
    int chan_mouse;
    int vel_mouse;
    int trk_mouse;


    // pixmap is the painted widget (without tools and cursorLines).
    // it will be zero if it needs to be repainted
    QPixmap* pixmap;

    // saves all TempoEvents from one before the first shown tick to the
    // last in the window
    QList<MidiEvent*>* currentTempoEvents;
    QList<TimeSignatureEvent*>* currentTimeSignatureEvents;

    // All Events to show in the velocityWidget are saved in velocityObjects
    QList<MidiEvent *> *objects, *velocityObjects;
    QList<QPair<int, int> > currentDivs;

    // To play the pianokeys, there is one NoteOnEvent
    NoteOnEvent* pianoEvent;

    bool _colorsByChannels;
    int _div;

    QMap<int, QRect> pianoKeys;
    QMultiMap<int, int> listAreaSel;

    static const unsigned sharp_strip_mask = (1 << 4) | (1 << 6) | (1 << 9) | (1 << 11) | (1 << 1);

};


#endif
