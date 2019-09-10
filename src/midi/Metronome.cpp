#include "Metronome.h"

#include "MidiFile.h"

#include <QtCore/qmath.h>
#include <QFile>
#include <QFileInfo>

#include <QAudioDeviceInfo>
#include <QSoundEffect>
#include <QtGlobal>

Metronome *Metronome::_instance = new Metronome();
bool Metronome::_enable = false;

Metronome::Metronome(QObject *parent) :	QObject(parent) {
    _file = 0;
    num = 4;
    denom = 2;
#if QT_VERSION >= 0x050D00
    _player = new QSoundEffect(QAudioDeviceInfo::defaultOutputDevice(), this);
#else
    _player = new QSoundEffect(this);
#endif
    _player->setVolume(1.0);
    _player->setSource(QUrl("qrc:/run_environment/metronome/metronome-01.wav"));
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
    _instance->_player->setVolume(value / 100.0);
}

int Metronome::loudness(){
    return (int)(_instance->_player->volume() * 100);
}
