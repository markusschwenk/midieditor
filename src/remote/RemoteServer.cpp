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

#include "RemoteServer.h"

#include "../midi/MidiFile.h"

#define newline "\n\r"
#include <QStringList>
#include <iostream>
#include <math.h>

#include <QtNetwork/QHostAddress>

using namespace std;

RemoteServer::RemoteServer(QObject* parent)
    : QObject(parent)
{
    _timeSentLast = -1;
    _file = 0;
    _connected = false;
    _clientIp = "";
    _port = -1;
    _clientname = "Not connected";
    connect(&udpSocket, SIGNAL(readyRead()), this, SLOT(readUDP()));
}

void RemoteServer::readUDP()
{
    while (udpSocket.hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(udpSocket.pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        udpSocket.readDatagram(datagram.data(), datagram.size(),
            &sender, &senderPort);

        if (sender == QHostAddress(_clientIp)) {
            QString message(datagram);
            receive(message);
        }
    }
}

void RemoteServer::receive(QString message)
{

    QStringList pair = message.trimmed().split("=");
    if (pair[0].trimmed() == "action") {
        if (pair[1].trimmed() == "play") {
            emit playRequest();
        }
        if (pair[1].trimmed() == "stop") {
            emit stopRequest(true, true);
        }
        if (pair[1].trimmed() == "stop_discard") {
            emit stopRequest(true, false);
        }
        if (pair[1].trimmed() == "record") {
            emit recordRequest();
        }
        if (pair[1].trimmed() == "pause") {
            emit pauseRequest();
        }
        if (pair[1].trimmed() == "forward") {
            emit forwardRequest();
        }
        if (pair[1].trimmed() == "back") {
            emit backRequest();
        }
        if (pair[1].trimmed() == "disconnect") {
            stopServer();
        }
    } else if (pair[0].trimmed() == "seek") {
        bool ok;
        _file->setPauseTick(-1);
        int timeMs = pair[1].trimmed().toInt(&ok);
        if (ok) {
            emit setTimeRequest(timeMs);
            if (_file) {
                _file->setCursorTick(_file->tick(timeMs));
            }
        }
    } else if (pair[0].trimmed() == "state") {
        if (pair[1].trimmed() == "search_program") {
            sendMessage("program=MidiEditor");
        }
        if (pair[1].trimmed() == "found_program") {
            _connected = true;
            if (_file) {
                setFile(_file);
            }
            emit connected();
        }
    } else if (pair[0].trimmed() == "program") {
        _clientname = pair[1].trimmed();
    }
}

void RemoteServer::sendMessage(QString message)
{
    message += newline;
    udpSocket.writeDatagram(message.toLatin1(), QHostAddress(_clientIp), _port);
}

void RemoteServer::setTime(int ms)
{
    if (ms >= _timeSentLast + 1000 || _timeSentLast < 0 || _timeSentLast > ms) {
        sendMessage("time=" + QString::number(ms - ms % 1000));
        _timeSentLast = ms - ms % 1000;
    }
}

void RemoteServer::setTonality(int tonality)
{
    sendMessage("tonality=" + QString::number(tonality));
}

void RemoteServer::setMeter(int num, int denum)
{
    sendMessage("numerator=" + QString::number(num));
    sendMessage(QString("denumerator=") + QString::number(powf(2, denum)));
}

void RemoteServer::play()
{
    sendMessage("state=play");
}

void RemoteServer::record()
{
    sendMessage("state=record");
}

void RemoteServer::stop()
{
    sendMessage("state=stop");
    cursorPositionChanged();
}

void RemoteServer::pause()
{
    sendMessage("state=pause");
}

void RemoteServer::setFile(MidiFile* file)
{

    _file = file;

    connect(_file, SIGNAL(cursorPositionChanged()), this, SLOT(cursorPositionChanged()));

    QString filename = file->path().split("/").last();
    sendMessage("song=" + filename);
    setMaxTime(file->maxTime());
    setTime(0);
    stop();
    setPositionInMeasure(0);
    setMeasure(0);

    cursorPositionChanged();
}

void RemoteServer::setMaxTime(int ms)
{
    sendMessage("maxtime=" + QString::number(ms));
}

void RemoteServer::setMeasure(int measure)
{
    if (measure < 1) {
        measure = 1;
    }
    sendMessage("measure=" + QString::number(measure));
}

void RemoteServer::setPositionInMeasure(double pos)
{
    sendMessage("position_in_measure=" + QString::number(pos));
}

void RemoteServer::cursorPositionChanged()
{
    if (_file) {

        int tick = _file->cursorTick();
        if (_file->pauseTick() >= 0) {
            tick = _file->pauseTick();
        }
        setTime(_file->msOfTick(tick));

        setTonality(_file->tonalityAt(tick));
        int num, denum;
        _file->meterAt(tick, &num, &denum);
        setMeter(num, denum);
        //int midiTimeInMeasure;
        QList<TimeSignatureEvent*>* list = 0;
        int measure = _file->measure(tick, tick, &list);
        if (measure < 1) {
            measure = 1;
        }
        setMeasure(measure);
    }
}

void RemoteServer::setPort(int port)
{
    _port = port;
}

void RemoteServer::setIp(QString ip)
{
    _clientIp = ip;
}

QString RemoteServer::clientIp()
{
    return _clientIp;
}

int RemoteServer::clientPort()
{
    return _port;
}

void RemoteServer::stopServer()
{
    sendMessage("action=disconnect");
    sendMessage("action=disconnect");
    _connected = false;
}

bool RemoteServer::isConnected()
{
    return _connected;
}

void RemoteServer::tryConnect()
{
    if (isConnected()) {
        stopServer();
    }
    if (_port < 0) {
        return;
    }
    udpSocket.bind(_port);
}

QString RemoteServer::clientName()
{
    return _clientname;
}
