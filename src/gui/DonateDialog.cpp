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

#include "DonateDialog.h"

#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>

DonateDialog::DonateDialog(QWidget* parent)
    : QDialog(parent) {
    setMinimumWidth(550);
    setMaximumHeight(450);
    setWindowTitle(tr("Donate"));
    setWindowIcon(QIcon(":/run_environment/graphics/icon.png"));
    QGridLayout* layout = new QGridLayout(this);

    QLabel* icon = new QLabel();
    icon->setPixmap(QPixmap(":/run_environment/graphics/midieditor.png").scaledToWidth(80, Qt::SmoothTransformation));
    icon->setFixedSize(80, 80);
    layout->addWidget(icon, 0, 0, 3, 1);

    QLabel* title = new QLabel(tr("<h1>Donate</h1>"), this);
    layout->addWidget(title, 0, 1, 1, 2);
    title->setStyleSheet("color: black");

    QLabel* version = new QLabel(tr("Please read the developer's note below"), this);
    layout->addWidget(version, 1, 1, 1, 2);
    version->setStyleSheet("color: black");

    QScrollArea* a = new QScrollArea(this);
    QLabel* content = new QLabel(tr("<html>"
                                    "<body>"
                                    "<h3>Reasons to donate</h3>"
                                    "<p>"
                                    "Dear User,<br>"
                                    "I offer MidiEditor for free and I hope you enjoy it. Please note that I developed the editor during my spare time and that I've been working on it for more than four years by now! If MidiEditor is useful for you, please send me a little donation so that I can at least cover my huge coffee-bill ;)  <br>"
                                    "</p>"
                                    "<h3>How to donate</h3>"
                                    "<p>"
                                    "Donating is very simple - and secure. Please visit the \"Donate\" section at the editor's project page in order to make a donation.<br>"
                                    "<a href=\"https://midieditor.org/donate\">https://midieditor.org/donate</a>"
                                    "</p>"
                                    "<h3>Thank you!</h3>"
                                    "<p>"
                                    "Your donation will help me to maintain and update MidiEditor."
                                    "</p>"
                                    "<h3>Other ways to Contribute</h3>"
                                    "<p>"
                                    "<a href=\"https://github.com/markusschwenk/midieditor\">Rate MidiEditor and provide feedback</a> to let me and other users know about how you liked the software!"
                                    "</p>"
                                    "</body>"
                                    "</html>"));
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

    QPushButton* close = new QPushButton(tr("Close"));
    layout->addWidget(close, 5, 2, 1, 1);
    connect(close, SIGNAL(clicked()), this, SLOT(hide()));
}
