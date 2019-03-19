#ifndef TIMESIGNATURETOOL_H
#define TIMESIGNATURETOOL_H

#include "EventTool.h"

class TimeSignatureTool : public EventTool {

public:
    TimeSignatureTool();
    TimeSignatureTool(TimeSignatureTool& other);

    void draw(QPainter* painter);

    bool press(bool leftClick);
    bool release();
    bool releaseOnly();

    bool move(int mouseX, int mouseY);

    ProtocolEntry* copy();
    void reloadState(ProtocolEntry* entry);

private:
};

#endif // TIMESIGNATURETOOL_H
