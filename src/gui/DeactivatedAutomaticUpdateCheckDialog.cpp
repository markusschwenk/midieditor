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

#include "DeactivatedAutomaticUpdateCheckDialog.h"

#include <QApplication>
#include <QFile>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QTextBrowser>
#include <QTextStream>
#include <QVariant>

DeactivatedAutomaticUpdateCheckDialog::DeactivatedAutomaticUpdateCheckDialog(
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
  layout->addWidget(icon, 0, 0, 3, 1);

  QScrollArea *a = new QScrollArea(this);
  QLabel *content = new QLabel("<html>"
                               "<body>"
                               "<h3>Deactivated Automatic Updates</h3>"
                               "<p>"
                               "Due to the new General Data Protection Regulation (GDPR), "
                               "automatic updates have to be deactivated by default. "
                               "</p>"
                               "<p>"
                               "You can re-activate them by navigating to Edit "
                               "- Settings- Updates in the menu bar."
                               "</p>"
                               "</body>"
                               "</html>");
  a->setWidgetResizable(true);
  a->setWidget(content);
  a->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  a->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  layout->addWidget(a, 0, 1, 4, 2);
  content->setStyleSheet("color: black; background-color: white; padding: 5px");

  content->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
  content->setOpenExternalLinks(true);
  content->setWordWrap(true);

  layout->setRowStretch(3, 1);
  layout->setColumnStretch(1, 1);

  QFrame *f = new QFrame(this);
  f->setFrameStyle(QFrame::HLine | QFrame::Sunken);
  layout->addWidget(f, 4, 0, 1, 3);

  QPushButton *close = new QPushButton("Close");
  layout->addWidget(close, 5, 2, 1, 1);
  connect(close, SIGNAL(clicked()), this, SLOT(hide()));
}
