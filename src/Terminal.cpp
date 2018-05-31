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

#include "Terminal.h"

#include <QProcess>
#include <QScrollBar>
#include <QTextEdit>
#include <QTimer>

#include "midi/MidiInput.h"
#include "midi/MidiOutput.h"

Terminal* Terminal::_terminal = 0;

Terminal::Terminal()
{
    _process = 0;
    _textEdit = new QTextEdit();
    _textEdit->setReadOnly(true);

    _inPort = "";
    _outPort = "";
}

void Terminal::initTerminal(QString startString, QString inPort,
    QString outPort)
{
    _terminal = new Terminal();
    _terminal->execute(startString, inPort, outPort);
}

Terminal* Terminal::terminal() { return _terminal; }

void Terminal::writeString(QString message)
{
    _textEdit->setText(_textEdit->toPlainText() + message + "\n");
    _textEdit->verticalScrollBar()->setValue(
        _textEdit->verticalScrollBar()->maximum());
}

void Terminal::execute(QString startString, QString inPort, QString outPort)
{
    _inPort = inPort;
    _outPort = outPort;

    if (startString != "") {
        if (_process) {
            _process->kill();
        }
        _process = new QProcess();

        connect(_process, SIGNAL(readyReadStandardOutput()), this,
            SLOT(printToTerminal()));
        connect(_process, SIGNAL(readyReadStandardError()), this,
            SLOT(printErrorToTerminal()));
        connect(_process, SIGNAL(started()), this, SLOT(processStarted()));

        _process->start(startString);
    } else {
        processStarted();
    }
}

void Terminal::processStarted()
{
    writeString("Started process");

    QStringList inputVariants;
    QString inPort = _inPort;
    inputVariants.append(inPort);
    if (inPort.contains(':')) {
        inPort = inPort.section(':', 0, 0);
    }
    inputVariants.append(inPort);
    if (inPort.contains('(')) {
        inPort = inPort.section('(', 0, 0);
    }
    inputVariants.append(inPort);

    QStringList outputVariants;
    QString outPort = _outPort;
    outputVariants.append(outPort);
    if (outPort.contains(':')) {
        outPort = outPort.section(':', 0, 0);
    }
    outputVariants.append(outPort);
    if (outPort.contains('(')) {
        outPort = outPort.section('(', 0, 0);
    }
    outputVariants.append(outPort);

    if (MidiInput::inputPort() == "" && _inPort != "") {
        writeString("Trying to set Input Port to " + _inPort);

        foreach (QString portVariant, inputVariants) {
            foreach (QString port, MidiInput::inputPorts()) {
                if (port.startsWith(portVariant)) {
                    writeString("Found port " + port);
                    MidiInput::setInputPort(port);
                    _inPort = "";
                    break;
                }
            }
            if (_inPort == "") {
                break;
            }
        }
    }

    if (MidiOutput::outputPort() == "" && _outPort != "") {
        writeString("Trying to set Output Port to " + _outPort);

        foreach (QString portVariant, outputVariants) {
            foreach (QString port, MidiOutput::outputPorts()) {
                if (port.startsWith(portVariant)) {
                    writeString("Found port " + port);
                    MidiOutput::setOutputPort(port);
                    _outPort = "";
                    break;
                }
            }
            if (_outPort == "") {
                break;
            }
        }
    }

    // if not both are set, try again in 1 second
    if ((MidiOutput::outputPort() == "" && _outPort != "") || (MidiInput::inputPort() == "" && _inPort != "")) {
        QTimer* timer = new QTimer();
        connect(timer, SIGNAL(timeout()), this, SLOT(processStarted()));
        timer->setSingleShot(true);
        timer->start(1000);
    }
}

void Terminal::printToTerminal()
{
    writeString(QString::fromLocal8Bit(_process->readAllStandardOutput()));
}

void Terminal::printErrorToTerminal()
{
    writeString(QString::fromLocal8Bit(_process->readAllStandardError()));
}

QTextEdit* Terminal::console() { return _textEdit; }
