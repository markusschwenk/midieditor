#include "UpdateSettingsWidget.h"

#include <QSettings>
#include <QGridLayout>
#include <QCheckBox>
#include <QPushButton>

#include "../UpdateManager.h"

UpdateSettingsWidget::UpdateSettingsWidget(QSettings *settings, QWidget *parent) : SettingsWidget("Updates", parent) {

	_settings = settings;

	QGridLayout *layout = new QGridLayout(this);
	setLayout(layout);

	_auto = new QCheckBox("Automatically check for Updates", this);
	_auto->setChecked(UpdateManager::autoCheckForUpdates());

	connect(_auto, SIGNAL(toggled(bool)), this, SLOT(enableAutoUpdates(bool)));
	layout->addWidget(_auto, 0, 0, 1, 6);

	layout->setRowStretch(5, 1);
}

void UpdateSettingsWidget::enableAutoUpdates(bool enable){
	UpdateManager::setAutoCheckUpdatesEnabled(enable);
}
