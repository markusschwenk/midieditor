/*
 * MidiEditor
 * Copyright (C) 2010  Markus Schwenk
 *
 * remoteVST - a Midieditor resource process to loading 32-bits VST plugins
 * Copyright (C) 2021 Francisco Munoz / "Estwald"
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

#include "mainwindow.h"
#include "VST/VST.h"

QSystemSemaphore * sys_sema_inW = NULL;
QSemaphore *sVST_EXT = NULL;

extern QSystemSemaphore *sys_sema_in;
extern QSystemSemaphore *sys_sema_out;


extern QSharedMemory *sharedVSText;
extern QSharedMemory *sharedAudioBuffer;

VST_IN *vst_in = NULL;

MainWindow::MainWindow(QWidget *parent, QString key)
    : QMainWindow(parent)
{
    qWarning("remoteVST MainWindow()");
    setWindowTitle(QApplication::applicationName() + " " + QApplication::applicationVersion());
    setWindowIcon(QIcon(":/icon.png"));

    setWindowFlags(Qt::FramelessWindowHint);
    setFixedSize(1,1);
    showMinimized();
    setWindowOpacity(0);
    setVisible(false);

    VST_proc::VST_setParent(this);

    sharedVSText = new QSharedMemory("remoteVST_mess" + key);
    if(!sharedVSText) exit(-11);

    // try first attach (old process exit with error)
    if(!sharedVSText->attach() && !sharedVSText->create(0x100000))
       exit(-11);

    sVST_EXT = new QSemaphore(1);
    sys_sema_inW = new QSystemSemaphore("VST_MAIN_SemaIn" + key);

    sharedAudioBuffer = new QSharedMemory("remoteVST_audio" + key);
    if(!sharedAudioBuffer) exit(-11);
    // try first attach (old process exit with error)
    if(!sharedAudioBuffer->attach() && !sharedAudioBuffer->create(sizeof(float) * 2 * 32 * 2048))
        exit(-11);

    sys_sema_in = new QSystemSemaphore("VST_IN_SemaIn" + key);
    sys_sema_out = new QSystemSemaphore("VST_IN_SemaOut" + key);

    sys_sema_out->release();

    vst_in = new VST_IN(this);

    connect(vst_in, SIGNAL(run_cmd_events()), this, SLOT(run_cmd_events()), Qt::QueuedConnection);
    connect(vst_in, SIGNAL(force_exit()), this, SLOT(force_exit()), Qt::QueuedConnection);

    vst_in->start(QThread::TimeCriticalPriority);

}

void MainWindow::force_exit() {
    // do something...

    exit(0);
}

MainWindow::~MainWindow() {

    if(vst_in) {
        vst_in->terminate();
        vst_in->wait(1000);
        delete vst_in;
        VST_proc::VST_exit();
    }

    DELETE(sVST_EXT)
    DELETE(sys_sema_in)
    DELETE(sys_sema_out)
    DELETE(sys_sema_inW)

    if(sharedVSText)
        sharedVSText->detach();

    if(sharedAudioBuffer)
        sharedAudioBuffer->detach();

    DELETE(sharedVSText)
    DELETE(sharedAudioBuffer)

    exit(0);

}


