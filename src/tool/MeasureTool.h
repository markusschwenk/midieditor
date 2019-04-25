#ifndef MEASURETOOL_H
#define MEASURETOOL_H

#include "EventTool.h"

class MeasureTool : public EventTool {

public:
    MeasureTool();
    MeasureTool(MeasureTool& other);

    void draw(QPainter* painter);

    bool press(bool leftClick);
    bool release();
    bool releaseOnly();
    bool releaseKey(int key);

    bool move(int mouseX, int mouseY);

    ProtocolEntry* copy();
    void reloadState(ProtocolEntry* entry);

private:
    int _firstSelectedMeasure, _secondSelectedMeasure;

    int closestMeasureStart(int *distX, int *measureX);
    void fillMeasures(QPainter *painter, int start, int end);
};

#endif // MEASURETOOL_H
