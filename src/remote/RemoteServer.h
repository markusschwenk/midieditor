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

#ifndef REMOTESERVER_H_
#define REMOTESERVER_H_

#include <QObject>

#include <QObject>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QtNetwork>

#include <QtNetwork/QUdpSocket>

class MidiFile;

class RemoteServer : public QObject {

    Q_OBJECT

public:
    RemoteServer(QObject* parent = 0);

    QString clientIp();
    int clientPort();

    void setPort(int port);
    void setIp(QString ip);

    bool isConnected();

    void stopServer();
    void tryConnect();
    QString clientName();

public slots:

    void receive(QString message);
    void setTime(int ms);
    void setTonality(int tonality);
    void setMeter(int num, int denum);

    void play();
    void record();
    void stop();
    void pause();

    void setFile(MidiFile* file);
    void setMaxTime(int ms);

    void setMeasure(int measure);
    void setPositionInMeasure(double pos);

    void cursorPositionChanged();

    void readUDP();
    void sendMessage(QString message);

signals:
    void connected();

    void playRequest();
    void pauseRequest();
    void stopRequest(bool autoConfirmRecord, bool addEvents);
    void forwardRequest();
    void backRequest();
    void recordRequest();
    void setTimeRequest(int timeMs);
    void disconnected();

private:
    QTcpServer server;
    QTcpSocket* client;
    int _timeSentLast;
    MidiFile* _file;

    QUdpSocket udpSocket;

    QString _clientIp;
    int _port;
    QString _clientname;
    bool _connected;
};

#endif
