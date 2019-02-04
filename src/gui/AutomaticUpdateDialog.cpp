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

#include "AutomaticUpdateDialog.h"

#include <QApplication>
#include <QFile>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QTextBrowser>
#include <QTextStream>
#include <QCheckBox>
#include <QVariant>

#include "../UpdateManager.h"


AutomaticUpdateDialog::AutomaticUpdateDialog(
    QWidget *parent)
    : QDialog(parent) {

    setMinimumWidth(550);
    setMaximumHeight(450);
    setWindowTitle(tr("Note"));
    setWindowIcon(QIcon(":/run_environment/graphics/icon.png"));
    QGridLayout *layout = new QGridLayout(this);

    QLabel *icon = new QLabel();
    icon->setPixmap(QPixmap(":/run_environment/graphics/midieditor.png")
                    .scaledToWidth(80, Qt::SmoothTransformation));
    icon->setFixedSize(80, 80);
    layout->addWidget(icon, 0, 0, 1, 1);

    QScrollArea *a = new QScrollArea(this);
    QLabel *content = new QLabel(tr("<html>"
                                    "<body>"
                                    "<h3>Enable Automatic Checking for Updates?</h3>"
                                    "<p>"
                                    "Click below to enable or disable automatic checking for updates. "
                                    "</p>"
                                    "<p>"
                                    "You can enable this option now, or later by navigating to Edit - Settings - Updates in the menu bar. "
                                    "You can always update your preference in that very same Settings page."
                                    "</p>"
                                    "<p>"
                                    "<p><b>Privacy Note</b></p>"
                                    "<p>"
                                    "When checking for updates, MidiEditor transmits your IP address as well as the currently "
                                    "installed version of MidiEditor and you operating system to a server which is located within the European Union."
                                    "</p>"
                                    "<p>"
                                    "Please read our privacy policy at <a "
                                    "href=\"https://www.midieditor.org/"
                                    "updates-privacy\">www.midieditor.org/updates-privacy</a> for further information."
                                    "</p>"
                                    "<p>"
                                    "By enabling the option, you confirm that you have read and "
                                    "understood the terms under which MidiEditor provides this service and "
                                    "that you agree to them."
                                    "</p>"
                                    "</body>"
                                    "</html>"));
    a->setWidgetResizable(true);
    a->setWidget(content);
    a->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    a->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    layout->addWidget(a, 0, 1, 4, 2);
    content->setStyleSheet("color: black; background-color: white; padding: 5px");

    content->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    content->setOpenExternalLinks(true);
    content->setWordWrap(true);

    layout->setRowStretch(1, 1);
// layout->setColumnStretch(1, 1);

    QFrame *f = new QFrame(this);
    f->setFrameStyle(QFrame::HLine | QFrame::Sunken);
    layout->addWidget(f, 6, 0, 1, 3);

    QPushButton *close = new QPushButton(tr("No, thanks!"));
    layout->addWidget(close, 7, 1, 1, 1);
    connect(close, SIGNAL(clicked()), this, SLOT(hide()));

    QPushButton *accept = new QPushButton(tr("Yes, enable checks"));
    layout->addWidget(accept, 7, 2, 1, 1);
    connect(accept, SIGNAL(clicked()), this, SLOT(enableAutoUpdates()));
}

void AutomaticUpdateDialog::enableAutoUpdates() {
    UpdateManager::setAutoCheckUpdatesEnabled(true);
    this->hide();
}


