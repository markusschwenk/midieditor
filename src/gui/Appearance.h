#ifndef APPEARANCE_H
#define APPEARANCE_H

#include <QSettings>
#include <QColor>

class Appearance
{
public:
    static void init(QSettings *settings);
    static QColor *channelColor(int channel);
    static QColor *trackColor(int track);
    static void writeSettings(QSettings *settings);
    static void setTrackColor(int track, QColor color);
    static void setChannelColor(int channel, QColor color);
    static void reset();
    static int opacity();
    static void setOpacity(int opacity);

private:
    static int trackToColorIndex(int track);
    static int channelToColorIndex(int channel);
    static QMap<int, QColor*> channelColors;
    static QMap<int, QColor*> trackColors;
    static QColor *defaultColor(int n);
    static QColor *decode(QString name, QSettings *settings, QColor *defaultColor);
    static void write(QString name, QSettings *settings, QColor *color);
    static int _opacity;
};

#endif // APPEARANCE_H
