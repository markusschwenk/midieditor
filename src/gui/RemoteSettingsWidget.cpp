/*
 * MidiEditor
 * Copyright (C) 2010  Markus Schwenk
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.+
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "RemoteSettingsWidget.h"

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>

#include "../remote/RemoteServer.h"

RemoteSettingsWidget::RemoteSettingsWidget(RemoteServer* server, QWidget* parent)
    : SettingsWidget("Android Remote", parent)
{
    _server = server;

    QGridLayout* layout = new QGridLayout(this);
    setLayout(layout);
    setWindowTitle("Enter IP and Port for the connection");
    setMinimumWidth(400);
    setMinimumHeight(170);

    QLabel* title = new QLabel("Enter IP and Port for the connection.", this);
    layout->addWidget(title, 0, 0, 1, 3);

    QLabel* ipLabel = new QLabel("IP:", this);
    layout->addWidget(ipLabel, 1, 0, 1, 1);

    _ipField = new QLineEdit(_server->clientIp(), this);
    layout->addWidget(_ipField, 1, 1, 1, 2);

    QLabel* portL = new QLabel("Port:", this);
    layout->addWidget(portL, 2, 0, 1, 1);

    QString port = QString::number(_server->clientPort());
    if (_server->clientPort() < 0) {
        port = "";
    }
    _portField = new QLineEdit(port, this);
    layout->addWidget(_portField, 2, 1, 1, 2);

    layout->setRowStretch(6, 1);
}

bool RemoteSettingsWidget::accept()
{
    bool ok;

    int port = _portField->text().toInt(&ok);
    if (!ok) {
        QMessageBox::information(this, "Enter number", QString("Port is no number!"));
        return true;
    }
    QString ip = _ipField->text();

    _server->setIp(ip);
    _server->setPort(port);
    //_server->tryConnect();
    QMessageBox::information(this, "Restart", QString("You have to restart MidiEditor to connect!"));
    return true;
}
