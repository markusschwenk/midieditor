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

#ifndef MISCWIDGET_H
#define MISCWIDGET_H

#include "PaintWidget.h"

class MatrixWidget;
class MidiEvent;
class SelectTool;

#include <QList>
#include <QPair>

#define SINGLE_MODE 0
#define LINE_MODE 1
#define MOUSE_MODE 2

#define VelocityEditor 0
#define ControllEditor 1
#define PitchBendEditor 2
#define KeyPressureEditor 3
#define ChannelPressureEditor 4
#define TempoEditor 5
#define MiscModeEnd 6

class MiscWidget : public PaintWidget {

    Q_OBJECT

public:
    MiscWidget(MatrixWidget* mw, QWidget* parent = 0);

    static QString modeToString(int mode);
    void setMode(int mode);
    void setEditMode(int mode);

public slots:
    void setChannel(int);
    void setControl(int ctrl);

protected:
    void paintEvent(QPaintEvent* event);
    void keyPressEvent(QKeyEvent* e);
    void keyReleaseEvent(QKeyEvent* event);

    void mouseReleaseEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void leaveEvent(QEvent* event);
    void mouseMoveEvent(QMouseEvent* event);

private:
    MatrixWidget* matrixWidget;

    // Mode is SINGLE_MODE or LINE_MODE
    int edit_mode;
    int mode;
    int channel;
    int controller;

    void resetState();

    QList<QPair<int, int> > getTrack(QList<MidiEvent*>* accordingEvents = 0);
    void computeMinMax();
    QPair<int, int> processEvent(MidiEvent* e, bool* ok);
    double interpolate(QList<QPair<int, int> > track, int x);
    int tickOfXPos(int x);
    int xPosOfTick(int tick);
    int tickOfMs(int ms);
    int msOfTick(int tick);
    int msOfXPos(int x);
    int xPosOfMs(int ms);
    int value(double y);
    bool filter(MidiEvent* e);

    int _max, _default;

    // single
    int dragY;
    bool dragging;
    SelectTool* _dummyTool;
    int trackIndex;

    // free hand
    QList<QPair<int, int> > freeHandCurve;
    bool isDrawingFreehand;

    // line
    int lineX, lineY;
    bool isDrawingLine;
};

#endif
