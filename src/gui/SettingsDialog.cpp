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

#include "SettingsDialog.h"

#include <QFrame>
#include <QGridLayout>
#include <QList>
#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QWidget>

#include "MidiSettingsWidget.h"
#include "AppearanceSettingsWidget.h"
#include "SettingsWidget.h"
#ifdef ENABLE_REMOTE
#include "../remote/RemoteServer.h"
#include "RemoteSettingsWidget.h"
#endif
#include "UpdateSettingsWidget.h"

SettingsDialog::SettingsDialog(QString title, QSettings* settings, RemoteServer* server, QWidget* parent)
    : QDialog(parent)
{

    setWindowTitle(title);

    _settingsWidgets = new QList<SettingsWidget*>;

    setMinimumHeight(400);

    QGridLayout* layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    setModal(true);

    // the central widget
    QWidget* central = new QWidget(this);
    QGridLayout* centralLayout = new QGridLayout(central);
    central->setLayout(centralLayout);

    // the list on the left side
    _listWidget = new QListWidget(central);
    _listWidget->setFixedWidth(170);

    connect(_listWidget, SIGNAL(currentRowChanged(int)), this,
        SLOT(rowChanged(int)));

    centralLayout->addWidget(_listWidget, 0, 0, 1, 1);

    // the settings on the right side
    _container = new QStackedWidget(central);
    _container->setFrameStyle(QFrame::Sunken);
    _container->setFrameShape(QFrame::Box);

    centralLayout->addWidget(_container, 0, 1, 1, 1);
    centralLayout->setColumnStretch(1, 1);
    centralLayout->setRowStretch(0, 1);

    layout->addWidget(central, 1, 0, 1, 1);
    layout->setRowStretch(1, 1);

    // buttons
    QWidget* buttonBar = new QWidget(this);
    QGridLayout* buttonLayout = new QGridLayout(buttonBar);

    buttonBar->setLayout(buttonLayout);

    // ok
    QPushButton* ok = new QPushButton("Close", buttonBar);
    buttonLayout->addWidget(ok, 0, 2, 1, 1);
    connect(ok, SIGNAL(clicked()), this, SLOT(submit()));

    buttonLayout->setColumnStretch(0, 1);
    layout->addWidget(buttonBar, 2, 0, 1, 1);

    // add content
    addSetting(new MidiSettingsWidget(central));
    addSetting(new AdditionalMidiSettingsWidget(settings, central));
#ifdef ENABLE_REMOTE
    addSetting(new RemoteSettingsWidget(server, central));
#endif
    addSetting(new AppearanceSettingsWidget(central));
    addSetting(new UpdateSettingsWidget(settings, central));
}

void SettingsDialog::addSetting(SettingsWidget* settingWidget)
{

    _settingsWidgets->append(settingWidget);
    _container->addWidget(settingWidget);

    // create the new ListEntry
    QListWidgetItem* newItem = new QListWidgetItem(settingWidget->title());
    newItem->setIcon(settingWidget->icon());
    _listWidget->addItem(newItem);

    // set the index to 1 if its the first widget
    if (_settingsWidgets->count() == 1) {
        _container->setCurrentIndex(0);
        _listWidget->setCurrentRow(0);
    }
}

void SettingsDialog::rowChanged(int row)
{
    int oldIndex = _container->currentIndex();
    if (_settingsWidgets->at(oldIndex)) {
        if (!_settingsWidgets->at(oldIndex)->accept()) {
            return;
        }
    }
    _container->setCurrentIndex(row);
}

void SettingsDialog::submit()
{
    int oldIndex = _container->currentIndex();
    if (_settingsWidgets->at(oldIndex)) {
        if (!_settingsWidgets->at(oldIndex)->accept()) {
            return;
        }
    }
    hide();
    emit settingsChanged();
}
