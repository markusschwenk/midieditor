#include "TimeSignatureTool.h"

#include "EventTool.h"
#include "../gui/MatrixWidget.h"
#include "../gui/TimeSignatureDialog.h"
#include "../midi/MidiFile.h"


TimeSignatureTool::TimeSignatureTool()
    : EventTool()
{
    setImage(":/run_environment/graphics/tool/meter.png");
    setToolTipText("Edit time signatures");
}

TimeSignatureTool::TimeSignatureTool(TimeSignatureTool& other) : TimeSignatureTool(){
}

void TimeSignatureTool::draw(QPainter* painter){
    int ms = matrixWidget->msOfXPos(mouseX);
    int tick = file()->tick(ms);
    int measureStartTick, measureEndTick;
    file()->measure(tick, &measureStartTick, &measureEndTick);

    int startX = matrixWidget->xPosOfMs(matrixWidget->msOfTick(measureStartTick));
    int endX = matrixWidget->xPosOfMs(matrixWidget->msOfTick(measureEndTick));
    painter->setOpacity(0.5);
    painter->fillRect(startX, 0, endX-startX, matrixWidget->height(), Qt::lightGray);
}

bool TimeSignatureTool::press(bool leftClick){
    return false;
}

bool TimeSignatureTool::release(){
    int ms = matrixWidget->msOfXPos(mouseX);
    int tick = file()->tick(ms);
    int measureStartTick, measureEndTick;
    int measure = file()->measure(tick, &measureStartTick, &measureEndTick);

    TimeSignatureDialog *dialog = new TimeSignatureDialog(file(), measure, measureStartTick);
    dialog->exec();

    return true;
}

bool TimeSignatureTool::releaseOnly(){
    return false;
}

bool TimeSignatureTool::move(int mouseX, int mouseY){
    EventTool::move(mouseX, mouseY);
    return true;
}

ProtocolEntry* TimeSignatureTool::copy(){
    return new TimeSignatureTool(*this);
}

void TimeSignatureTool::reloadState(ProtocolEntry* entry){
    TimeSignatureTool* other = dynamic_cast<TimeSignatureTool*>(entry);
    if (!other) {
        return;
    }
    EventTool::reloadState(entry);
}
