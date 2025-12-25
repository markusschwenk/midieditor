/*
 * MidiEditor
 * Copyright (C) 2010  Markus Schwenk
 *
 * maudio - Audio interface for Qt 5.x - Qt 6.x
 * Copyright (C) 2025 Francisco Munoz / "Estwald"
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.+
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MAUDIO_H
#define MAUDIO_H

#ifdef USE_FLUIDSYNTH

#include <QObject>

#ifdef IS_QT5
#include <QAudioDeviceInfo>
#include <QAudioOutput>
typedef QAudioOutput MAudioOutput;
#else
#include <QMediaDevices>
#include <QAudioDevice>
#include <QAudioOutput>
#include <QAudioSink>
typedef QAudioSink MAudioOutput;
#endif

#ifdef IS_QT5
typedef struct {
    QAudioDeviceInfo device;
    QAudioFormat format;
    QString device_name;
    bool is_default_device;
    int sample_rate;
    bool is_float;
    bool float_is_supported;
    int err;

} MAudioDev;
#else
typedef struct {
    QAudioDevice device;
    QAudioFormat format;
    QString device_name;
    bool is_default_device;
    int sample_rate;
    bool is_float;
    bool float_is_supported;
    int err;

} MAudioDev;
#endif
class MAudio : public QObject
{
    Q_OBJECT
public:
    MAudio (QObject *parent = nullptr);
    static MAudioDev * MAudio_findDev(MAudioDev param);
    static MAudioOutput * MAudio_CreateDev(MAudioDev *dev);
    static QList<QString> MAudio_OutputDevices();
    static bool MAudio_SupportedSampleRate(QString outputdevice, int samplerate);

};

#endif
#endif // MAUDIO_H
