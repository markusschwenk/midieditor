#include "MeasureTool.h"

#include <QInputDialog>

#include "EventTool.h"
#include "../gui/MatrixWidget.h"
#include "../midi/MidiFile.h"
#include "../protocol/Protocol.h"

MeasureTool::MeasureTool()
    : EventTool()
{
    setImage(":/run_environment/graphics/tool/measure.png");
    setToolTipText("Insert or delete measures");
    _firstSelectedMeasure = -1;
    _secondSelectedMeasure = -1;
}

MeasureTool::MeasureTool(MeasureTool& other) : MeasureTool(){
    _firstSelectedMeasure = other._firstSelectedMeasure;
    _secondSelectedMeasure = other._secondSelectedMeasure;
}

void MeasureTool::draw(QPainter* painter){
    if(_firstSelectedMeasure > -1) {
        painter->setOpacity(0.3);
        fillMeasures(painter, _firstSelectedMeasure, _secondSelectedMeasure);
    }
    if (_firstSelectedMeasure > -1) {
        int ms = matrixWidget->msOfXPos(mouseX);
        int tick = file()->tick(ms);
        int measureStartTick, measureEndTick;
        int inMeasure = file()->measure(tick, &measureStartTick, &measureEndTick);

        int measureFrom = _firstSelectedMeasure;
        int measureTo;

        if (QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier)) {
            measureTo = inMeasure;
        } else {
            measureTo = _secondSelectedMeasure;
        }

        // Draw selection
        if (measureFrom > measureTo) {
            int tmp = measureFrom;
            measureFrom = measureTo;
            measureTo = tmp;
        }
        int x1 = matrixWidget->xPosOfMs(file()->msOfTick(file()->startTickOfMeasure(measureFrom)));
        int x2 = matrixWidget->xPosOfMs(file()->msOfTick(file()->startTickOfMeasure(measureTo+1)));
        painter->setOpacity(0.2);
        painter->fillRect(x1, 0, x2-x1, matrixWidget->height(), Qt::lightGray);
    }
    int dist, measureX;
    this->closestMeasureStart(&dist, &measureX);
    if (dist < 10) {
        painter->drawLine(measureX - 10, 0, measureX - 10, matrixWidget->height());
        painter->drawLine(measureX + 10, 0, measureX + 10, matrixWidget->height());
    } else {
        int ms = matrixWidget->msOfXPos(mouseX);
        int tick = file()->tick(ms);
        int measureStartTick, measureEndTick;
        file()->measure(tick, &measureStartTick, &measureEndTick);

        int x1 = matrixWidget->xPosOfMs(file()->msOfTick(measureStartTick));
        int x2 = matrixWidget->xPosOfMs(file()->msOfTick(measureEndTick));
        painter->setOpacity(0.5);
        painter->fillRect(x1, 0, x2-x1, matrixWidget->height(), Qt::lightGray);
    }
}

bool MeasureTool::press(bool leftClick){
    return true;
}

bool MeasureTool::release(){
    if (_firstSelectedMeasure > -1 && QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier)) {
        int ms = matrixWidget->msOfXPos(mouseX);
        int tick = file()->tick(ms);
        int measureStartTick, measureEndTick;
        _secondSelectedMeasure = file()->measure(tick, &measureStartTick, &measureEndTick);
        return true;
    }
    int dist, measureX;
    int measure = this->closestMeasureStart(&dist, &measureX);
    if (dist < 10) {
        bool ok;
        if (measure < 2) {
            return true;
        }
        int num = QInputDialog::getInt(matrixWidget, "Insert Measures",
            "Number of measures:", 1, 1, 100000, 1, &ok);
        if (ok) {
            file()->protocol()->startNewAction("Insert measures", image());
            file()->insertMeasures(measure-1, num);
            _firstSelectedMeasure = -1;
            _secondSelectedMeasure = -1;
            file()->protocol()->endAction();
        }
        return true;
    } else {
        int ms = matrixWidget->msOfXPos(mouseX);
        int tick = file()->tick(ms);
        int measureStartTick, measureEndTick;
        _firstSelectedMeasure = file()->measure(tick, &measureStartTick, &measureEndTick);
        _secondSelectedMeasure = _firstSelectedMeasure;
        return true;
    }

    _firstSelectedMeasure = -1;
    _secondSelectedMeasure = -1;
    return true;
}

bool MeasureTool::releaseOnly(){
    _firstSelectedMeasure = -1;
    _secondSelectedMeasure = -1;
    return false;
}

bool MeasureTool::releaseKey(int key){
    if(key == Qt::Key_Delete && _firstSelectedMeasure > -1){
        file()->protocol()->startNewAction("Remove measures", image());
        if (_secondSelectedMeasure == -1) {
            _secondSelectedMeasure = _firstSelectedMeasure;
        }
        if (_firstSelectedMeasure > _secondSelectedMeasure) {
            int tmp = _firstSelectedMeasure;
            _firstSelectedMeasure = _secondSelectedMeasure;
            _secondSelectedMeasure = tmp;
        }
        file()->deleteMeasures(_firstSelectedMeasure, _secondSelectedMeasure);
        _firstSelectedMeasure = -1;
        _secondSelectedMeasure = -1;
        file()->protocol()->endAction();
        return true;
    }
    return false;
}

bool MeasureTool::move(int mouseX, int mouseY){
    EventTool::move(mouseX, mouseY);
    return true;
}

ProtocolEntry* MeasureTool::copy(){
    return new MeasureTool(*this);
}

void MeasureTool::reloadState(ProtocolEntry* entry){
    MeasureTool* other = dynamic_cast<MeasureTool*>(entry);
    if (!other) {
        return;
    }
    EventTool::reloadState(entry);
}

int MeasureTool::closestMeasureStart(int *distX, int *measureX) {
    int ms = matrixWidget->msOfXPos(mouseX);
    int tick = file()->tick(ms);
    int measureStartTick, measureEndTick;
    int measure = file()->measure(tick, &measureStartTick, &measureEndTick);

    int startX = matrixWidget->xPosOfMs(file()->msOfTick(measureStartTick));
    int endX = matrixWidget->xPosOfMs(file()->msOfTick(measureEndTick));

    int distStart = abs(mouseX - startX);
    int distEnd = abs(mouseX - endX);
    if (distEnd < distStart) {
        measure++;
        *distX = distEnd;
        *measureX = endX;
    } else {
        *distX = distStart;
        *measureX = startX;
    }
    return measure;
}

void MeasureTool::fillMeasures(QPainter *painter, int measureFrom, int measureTo){
    // Draw selection
    if (measureFrom > measureTo) {
        int tmp = measureFrom;
        measureFrom = measureTo;
        measureTo = tmp;
    }
    int x1 = matrixWidget->xPosOfMs(file()->msOfTick(file()->startTickOfMeasure(measureFrom)));
    int x2 = matrixWidget->xPosOfMs(file()->msOfTick(file()->startTickOfMeasure(measureTo+1)));
    painter->fillRect(x1, 0, x2-x1, matrixWidget->height(), Qt::lightGray);
}
