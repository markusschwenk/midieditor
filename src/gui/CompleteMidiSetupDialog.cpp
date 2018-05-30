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

#include "CompleteMidiSetupDialog.h"

#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QScrollArea>

CompleteMidiSetupDialog::CompleteMidiSetupDialog(QWidget *parent) : QDialog(parent) {

    setMinimumWidth(550);
    setMaximumHeight(450);
    setWindowTitle(tr("No Sound?"));
    setWindowIcon(QIcon(":/run_environment/graphics/icon.png"));
    QGridLayout *layout = new QGridLayout(this);

    QLabel *icon = new QLabel();
    icon->setPixmap(QPixmap(":/run_environment/graphics/midieditor.png").scaledToWidth(80, Qt::SmoothTransformation));
    icon->setFixedSize(80, 80);
    layout->addWidget(icon, 0, 0, 3, 1);

    QLabel *title = new QLabel("<h1>No Sound?</h1>", this);
    layout->addWidget(title, 0, 1, 1, 2);
    title->setStyleSheet("color: black");

    QLabel *version = new QLabel("It appears that you did not complete your midi setup!", this);
    layout->addWidget(version, 1, 1, 1, 2);
    version->setStyleSheet("color: black");

    QScrollArea *a = new QScrollArea(this);
    QLabel *content = new QLabel("<html>"
                                     "<body>"
                                     "<h3>MidiEditor is not connected to a midi port</h3>"
                                     "<p>"
                                        "In order to play your music, you have to connect MidiEditor to a midi device (on your computer or externally) to play your sounds.</br>"
                                        "Please refere to the user's manual."
                                     "</p>"
                                     "</body>"
                                 "</html>"
                                 );
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

    QFrame *f = new QFrame( this );
    f->setFrameStyle(QFrame::HLine | QFrame::Sunken);
    layout->addWidget(f, 4, 0, 1, 3);

    QPushButton *close = new QPushButton("Close");
    layout->addWidget(close, 5, 2, 1, 1);
    connect(close, SIGNAL(clicked()), this, SLOT(hide()));
}

