#ifndef TEMPOTOOL_H
#define TEMPOTOOL_H

#include "EventTool.h"

class TempoTool : public EventTool {

public:
    TempoTool();
    TempoTool(TempoTool& other);

    void draw(QPainter* painter);

    bool press(bool leftClick);
    bool release();
    bool releaseOnly();

    bool move(int mouseX, int mouseY);

    ProtocolEntry* copy();
    void reloadState(ProtocolEntry* entry);

private:
    int _startX;
};

#endif // TEMPOTOOL_H
