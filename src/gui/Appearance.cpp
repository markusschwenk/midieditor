#include "Appearance.h"

QMap<int, QColor*> Appearance::channelColors = QMap<int, QColor*>();
QMap<int, QColor*> Appearance::trackColors = QMap<int, QColor*>();
int Appearance::_opacity = 100;

void Appearance::init(QSettings *settings){
    for (int channel = 0; channel < 17; channel++) {
        channelColors.insert(channel,
                             decode("channel_color_" + QString::number(channel),
                                    settings, defaultColor(channel)));
    }
    for (int track = 0; track < 17; track++) {
        trackColors.insert(track,
                           decode("track_color_" + QString::number(track),
                                  settings, defaultColor(track)));
    }
    _opacity = settings->value("appearance_opacity", 100).toInt();
}

QColor *Appearance::channelColor(int channel){
    QColor *color = channelColors[channelToColorIndex(channel)];
    color->setAlpha(_opacity * 255 / 100);
    return color;
}

QColor *Appearance::trackColor(int track) {
    QColor *color = trackColors[trackToColorIndex(track)];
    color->setAlpha(_opacity * 255 / 100);
    return color;
}

void Appearance::writeSettings(QSettings *settings) {
    for (int channel = 0; channel < 17; channel++) {
        write("channel_color_" + QString::number(channel), settings, channelColors[channel]);
    }
    for (int track = 0; track < 17; track++) {
        write("track_color_" + QString::number(track), settings, trackColors[track]);
    }
    settings->setValue("appearance_opacity", _opacity);
}

QColor *Appearance::defaultColor(int n) {
    QColor* color;

    switch (n) {
    case 0: {
        color = new QColor(241, 70, 57, 255);
        break;
    }
    case 1: {
        color = new QColor(205, 241, 0, 255);
        break;
    }
    case 2: {
        color = new QColor(50, 201, 20, 255);
        break;
    }
    case 3: {
        color = new QColor(107, 241, 231, 255);
        break;
    }
    case 4: {
        color = new QColor(127, 67, 255, 255);
        break;
    }
    case 5: {
        color = new QColor(241, 127, 200, 255);
        break;
    }
    case 6: {
        color = new QColor(170, 212, 170, 255);
        break;
    }
    case 7: {
        color = new QColor(222, 202, 170, 255);
        break;
    }
    case 8: {
        color = new QColor(241, 201, 20, 255);
        break;
    }
    case 9: {
        color = new QColor(80, 80, 80, 255);
        break;
    }
    case 10: {
        color = new QColor(202, 50, 127, 255);
        break;
    }
    case 11: {
        color = new QColor(0, 132, 255, 255);
        break;
    }
    case 12: {
        color = new QColor(102, 127, 37, 255);
        break;
    }
    case 13: {
        color = new QColor(241, 164, 80, 255);
        break;
    }
    case 14: {
        color = new QColor(107, 30, 107, 255);
        break;
    }
    case 15: {
        color = new QColor(50, 127, 127, 255);
        break;
    }
    default: {
        color = new QColor(50, 50, 255, 255);
        break;
    }
    }
    return color;
}

QColor *Appearance::decode(QString name, QSettings *settings, QColor *defaultColor){
   bool ok;
   int r = settings->value(name + "_r").toInt(&ok);
   if (!ok) {
       return new QColor(*defaultColor);
   }
   int g = settings->value(name + "_g").toInt(&ok);
   if (!ok) {
       return defaultColor;
   }
   int b = settings->value(name + "_b").toInt(&ok);
   if (!ok) {
       return defaultColor;
   }
   return new QColor(r, g, b);
}

void Appearance::write(QString name, QSettings *settings, QColor *color) {
    settings->setValue(name + "_r", QVariant(color->red()));
    settings->setValue(name + "_g", QVariant(color->green()));
    settings->setValue(name + "_b", QVariant(color->blue()));
}

void Appearance::setTrackColor(int track, QColor color) {
    trackColors[trackToColorIndex(track)] = new QColor(color);
}

void Appearance::setChannelColor(int channel, QColor color){
    channelColors[channelToColorIndex(channel)] = new QColor(color);
}

int Appearance::trackToColorIndex(int track){
    int mod = (track-1) %17;
    if (mod < 0) {
        mod+=17;
    }
    return mod;
}

int Appearance::channelToColorIndex(int channel) {
    if (channel > 16) {
        channel = 16;
    }
    return channel;
}

void Appearance::reset() {
    for (int channel = 0; channel < 17; channel++) {
        channelColors[channel] = defaultColor(channel);
    }
    for (int track = 0; track < 17; track++) {
        trackColors[track] = defaultColor(track);
    }
}

int Appearance::opacity(){
    return _opacity;
}

void Appearance::setOpacity(int opacity){
    _opacity = opacity;
}
