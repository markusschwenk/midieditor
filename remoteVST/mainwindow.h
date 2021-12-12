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


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMutex>
#include <QSemaphore>
#include <QSystemSemaphore>
#include "VST/aeffectx.h"
#include <QSharedMemory>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    MainWindow(QWidget *parent = nullptr,  QString key = 0);
    ~MainWindow();
public slots:

    void run_cmd_events();
    void force_exit();

};
#endif
