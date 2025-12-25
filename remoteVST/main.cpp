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

#include <QApplication>
#include <QProcess>
#include <QFile>
#include <QFileDevice>
#include <QThread>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "VST/VST.h"

#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QStringList args = a.arguments();

    a.setWindowIcon(QIcon(":/icon.png"));

    if(args.count() < 2) {

        QMessageBox::information(NULL, "remoteVST", "remoteVST is a Midieditor resource process to loading 32-bits VST plugins");

        return -1;

    }

    a.setQuitOnLastWindowClosed(false);

    // args.at(1) is a key to create shared memory and system semaphores for a Midieditor instance

    MainWindow *w = new MainWindow(NULL,  args.count() > 1 ? args.at(1) : "");

    int r = a.exec();

    delete w;
    return r;
}
