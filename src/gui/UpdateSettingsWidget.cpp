#include "UpdateSettingsWidget.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QPushButton>
#include <QSettings>

#include "../UpdateManager.h"

UpdateSettingsWidget::UpdateSettingsWidget(QSettings *settings, QWidget *parent)
    : SettingsWidget("Updates", parent) {

  _settings = settings;

  QGridLayout *layout = new QGridLayout(this);
  setLayout(layout);

  QWidget *tpqInfo = createInfoBox(
      "<p><b>Privacy Note</b></p>"
      "<p>"
      "Please read the privacy note on <a "
      "href=\"http://www.midieditor.org/"
      "index.php?category=update_privacy\">www.midieditor.org</a>."
      "</p>"
      "<p>"
      "By enabling the option below, you confirm that you have read and "
      "understood the terms under which MidiEditor provides this service and "
      "that you agree to them."
      "</p>");
  layout->addWidget(tpqInfo, 0, 0, 1, 6);

  _auto = new QCheckBox("Automatically check for Updates", this);
  _auto->setChecked(UpdateManager::autoCheckForUpdates());

  connect(_auto, SIGNAL(toggled(bool)), this, SLOT(enableAutoUpdates(bool)));
  layout->addWidget(_auto, 1, 0, 1, 6);

  layout->setRowStretch(6, 1);
}

void UpdateSettingsWidget::enableAutoUpdates(bool enable) {
  UpdateManager::setAutoCheckUpdatesEnabled(enable);
}
