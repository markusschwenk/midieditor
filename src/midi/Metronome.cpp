#include "Metronome.h"

#include "MidiFile.h"

#include <QtCore/qmath.h>
#include <QFile>
#include <QFileInfo>

#include <QMediaPlayer>

Metronome *Metronome::_instance = new Metronome();
bool Metronome::_enable = false;

Metronome::Metronome(QObject *parent) :	QObject(parent) {
    _file = 0;
    num = 4;
    denom = 2;
    _player = new QMediaPlayer(this, QMediaPlayer::LowLatency);
    _player->setVolume(100);
    _player->setMedia(QUrl::fromLocalFile(QFileInfo("metronome/metronome-01.wav").absoluteFilePath()));
}

void Metronome::setFile(MidiFile *file){
    _file = file;
}

void Metronome::measureUpdate(int measure, int tickInMeasure){

    // compute pos
    if(!_file){
        return;
    }

    int ticksPerClick = (_file->ticksPerQuarter()*4)/qPow(2, denom);
    int pos = tickInMeasure / ticksPerClick;

    if(lastMeasure < measure){
        click();
        lastMeasure = measure;
        lastPos = 0;
        return;
    } else {
        if(pos > lastPos){
            click();
            lastPos = pos;
            return;
        }
    }
}

void Metronome::meterChanged(int n, int d){
    num = n;
    denom = d;
}

void Metronome::playbackStarted(){
    reset();
}

void Metronome::playbackStopped(){

}

Metronome *Metronome::instance(){
    return _instance;
}

void Metronome::reset(){
    lastPos = 0;
    lastMeasure = -1;
}

void Metronome::click(){

    if(!enabled()){
        return;
    }
    _player->play();
}

bool Metronome::enabled(){
    return _enable;
}

void Metronome::setEnabled(bool b){
    _enable = b;
}

void Metronome::setLoudness(int value){
    _instance->_player->setVolume(value);
}

int Metronome::loudness(){
    return _instance->_player->volume();
}
