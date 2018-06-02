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
