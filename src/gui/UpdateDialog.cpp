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

#include "UpdateDialog.h"

#include <QApplication>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QTextBrowser>
#include <QVariant>

#include "../UpdateManager.h"

UpdateDialog::UpdateDialog(Update* update, QWidget* parent)
    : QDialog(parent) {

    setMinimumWidth(550);
    setMaximumHeight(450);
    setWindowTitle(tr("About"));
    setWindowIcon(QIcon(":/run_environment/graphics/icon.png"));
    QGridLayout* layout = new QGridLayout(this);

    QLabel* icon = new QLabel();
    icon->setPixmap(QPixmap(":/run_environment/graphics/midieditor.png").scaledToWidth(80, Qt::SmoothTransformation));
    icon->setFixedSize(80, 80);
    layout->addWidget(icon, 0, 0, 3, 1);

    QLabel* title = new QLabel(tr("<h1>Update Available</h1>"), this);
    layout->addWidget(title, 0, 1, 1, 2);
    title->setStyleSheet("color: black");

    QLabel* version = new QLabel(tr("New Version: ") + update->versionString(), this);
    layout->addWidget(version, 1, 1, 1, 2);
    version->setStyleSheet("color: black");

    QScrollArea* a = new QScrollArea(this);
    QLabel* content = new QLabel("<html>"
                                 "<body>"
                                 "<p>"
                                 "Good news: a new version of MidiEditor has been released and is ready to be downloaded and installed on your system. "
                                 "Please read the instructions below to learn how to install the update."
                                 "</p>"
                                 "<h3>Update</h3>"
                                 "<p>"
                                 "<b>New version: </b> "
                                 + update->versionString() + " (Current version: " + QApplication::applicationVersion() + ")<br>" + "<b>Changes: </b> " + update->changelog() + "</p>"
                                 "<h3>How to install the Update</h3>"
                                 "<p>Click <a href=\""
                                 + update->path() + "\">here</a> to download the update. The link will open a browser window which contains a website which allows you to download the update. Close MidiEditor and execute the downloaded file to install the new version of MidiEditor.</p>"
                                 "<h3>Disable automatic Updates</h3>"
                                 "<p>"
                                 "In order to disable the automatic updates, plese open the editor's settings and uncheck the checkbox \"Automatically check for Updates\""
                                 "</p>"
                                 "</body>"
                                 "</html>");
    a->setWidgetResizable(true);
    a->setWidget(content);
    a->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    a->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    layout->addWidget(a, 2, 1, 2, 2);
    content->setStyleSheet("color: black; background-color: white; padding: 5px");

    content->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    content->setOpenExternalLinks(true);
    content->setWordWrap(true);

    layout->setRowStretch(3, 1);
    layout->setColumnStretch(1, 1);

    QFrame* f = new QFrame(this);
    f->setFrameStyle(QFrame::HLine | QFrame::Sunken);
    layout->addWidget(f, 4, 0, 1, 3);

    QPushButton* close = new QPushButton(tr("No, remind me later"));
    layout->addWidget(close, 5, 2, 1, 1);
    connect(close, SIGNAL(clicked()), this, SLOT(hide()));
}
