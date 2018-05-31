#ifndef UPDATESETTINGSWIDGET_H
#define UPDATESETTINGSWIDGET_H

#include "SettingsWidget.h"

class QSettings;
class QCheckBox;

class UpdateSettingsWidget : public SettingsWidget {

    Q_OBJECT

public:
    UpdateSettingsWidget(QSettings* settings, QWidget* parent = 0);

public slots:
    void enableAutoUpdates(bool enable);

private:
    QCheckBox* _auto;
    QSettings* _settings;
};

#endif
