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

#include "midi/MidiFile.h"
#include "midi/MidiInput.h"
#include "midi/MidiOutput.h"

Terminal* Terminal::_terminal = 0;

Terminal::Terminal()
{
    _process = 0;
    _textEdit = new QTextEdit();
    if(!_textEdit)
        ERROR_CRITICAL_NO_MEMORY();
    _textEdit->setReadOnly(true);

    for(int n = 0; n < MAX_INPUT_DEVICES; n++) {
        _inPort[n] = "";
    }

    for(int n = 0; n < MAX_OUTPUT_DEVICES; n++) {
        _outPort[n] = "";
    }
}

void Terminal::initTerminal(QString startString, QString inPort[],
    QString outPort[])
{
    _terminal = new Terminal();
    if(!_terminal)
        ERROR_CRITICAL_NO_MEMORY();
    _terminal->execute(startString, inPort, outPort);
}

Terminal* Terminal::terminal() { return _terminal; }

void Terminal::writeString(QString message)
{
    _textEdit->setText(_textEdit->toPlainText() + message + "\n");
    _textEdit->verticalScrollBar()->setValue(
        _textEdit->verticalScrollBar()->maximum());
}

void Terminal::execute(QString startString, QString inPort[], QString outPort[])
{
    for(int n = 0; n < MAX_INPUT_DEVICES; n++)
        _inPort[n] = inPort[n];
    for(int n = 0; n < MAX_OUTPUT_DEVICES; n++)
        _outPort[n] = outPort[n];

    if (startString != "") {
        if (_process) {
            _process->kill();
        }

        _process = new QProcess();
        if(!_process)
            ERROR_CRITICAL_NO_MEMORY();

        connect(_process, SIGNAL(readyReadStandardOutput()), this,
            SLOT(printToTerminal()));
        connect(_process, SIGNAL(readyReadStandardError()), this,
            SLOT(printErrorToTerminal()));
        connect(_process, SIGNAL(started()), this, SLOT(processStarted()));

        QStringList args;
        _process->start(startString, args);
    } else {
        processStarted();
    }
}

void Terminal::setOutport(int port, QString device)
{
    if((port < 0) || (port > MAX_OUTPUT_DEVICES)) return;

    _outPort[port] = device;
    if(MidiOutput::setOutputPort(_outPort[port], port))
        _outPort[port] = "";
}

void Terminal::processStarted()
{
    writeString("Started process");

    for(int n = 0; n < MAX_INPUT_DEVICES; n++) {
        if (MidiInput::inputPort(n) == "" && _inPort[n] != "") {
            writeString("Trying to set Input Port to " + _inPort[n] + "device #" +
                        QString::number(n));

            if(MidiInput::setInputPort(_inPort[n], n))
                _inPort[n] = "";
        }
    }

#ifdef USE_FLUIDSYNTH
    if(!MidiOutput::FluidSynthTracksAuto)
#endif
        for(int n = 0; n < MAX_OUTPUT_DEVICES; n++) {
            if(MidiOutput::AllTracksToOne && n != 0)
                break;
            if (MidiOutput::outputPort(n) == "" && _outPort[n] != "") {
                writeString("Trying to set Output Port to " + _outPort[n]);
                if(MidiOutput::setOutputPort(_outPort[n], n))
                    _outPort[n] = "";

            }
        }

    bool try_input = false;

    for(int n = 0; n < MAX_INPUT_DEVICES; n++) {

        if(MidiInput::inputPort(n) == "" && _inPort[n] != "")
            try_input = true;
    }

    bool try_output = false;

#ifdef USE_FLUIDSYNTH
    if(!MidiOutput::FluidSynthTracksAuto)
#endif
        for(int n = 0; n < MAX_OUTPUT_DEVICES; n++) {
            if(MidiOutput::AllTracksToOne && n != 0)
                break;
            if(MidiOutput::outputPort(n) == "" && _outPort[n] != "")
                try_output = true;
        }

    // if not both are set, try again in 1 second
    if ((try_output) || (try_input)) {
        QTimer* timer = new QTimer();
        if(!timer) ERROR_CRITICAL_NO_MEMORY();
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
