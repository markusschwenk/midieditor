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



#include "maudio.h"

#ifdef USE_FLUIDSYNTH
#include "fluidsynth_proc.h"

#ifndef IS_QT5
#if defined(__WINDOWS_MM__)

/*
    Windows only and Qt 6.x: getWaveFormatFromQAudioDevice()
    is used to obtain the real audio format without resampling,
    to avoid a failure in Qt's internal functions.

*/

#include <mmdeviceapi.h>
#include <audioclient.h>
#include <propsys.h>
#include <propvarutil.h>
#include <initguid.h>

// GUID manual de PKEY_Device_FriendlyName
const PROPERTYKEY PKEY_FriendlyName = {
    {0xa45c254e, 0xdf1c, 0x4efd, {0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0}},
    14
};

bool getWaveFormatFromQAudioDevice(QString qtDevice, WAVEFORMATEX **outWaveFormat) {

    CoInitialize(nullptr);

    IMMDeviceEnumerator *enumerator = nullptr;
    IMMDeviceCollection *deviceCollection = nullptr;

    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                  CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                  (void**)&enumerator);

    if (FAILED(hr))
        return false;

    if(qtDevice == QString("Default Output Device")) {
        //qWarning("Default Output Device");
        IMMDevice *device = nullptr;
        hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
        if (FAILED(hr)) {
            qWarning("Failed to get default output device\n");
            enumerator->Release();
            CoUninitialize();
            return false;
        }
        IAudioClient *audioClient = nullptr;
        hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&audioClient);
        if (SUCCEEDED(hr)) {
            hr = audioClient->GetMixFormat(outWaveFormat);
            audioClient->Release();
            PROPVARIANT nameProp;
            PropVariantClear(&nameProp);
            device->Release();
            enumerator->Release();
            CoUninitialize();
            return SUCCEEDED(hr);
        }

    } else {

        hr = enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &deviceCollection);
        if (FAILED(hr)) {
            enumerator->Release();
            CoUninitialize();
            return false;
        }

        UINT count = 0;
        deviceCollection->GetCount(&count);

        for (UINT i = 0; i < count; ++i) {
            IMMDevice *device = nullptr;
            deviceCollection->Item(i, &device);

            IPropertyStore *props = nullptr;
            device->OpenPropertyStore(STGM_READ, &props);

            PROPVARIANT nameProp;
            PropVariantInit(&nameProp);
            props->GetValue(PKEY_FriendlyName, &nameProp);

            QString deviceName = QString::fromWCharArray(nameProp.pwszVal);

            if (deviceName == qtDevice) {
                IAudioClient *audioClient = nullptr;
                hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&audioClient);
                if (SUCCEEDED(hr)) {
                    hr = audioClient->GetMixFormat(outWaveFormat);
                    audioClient->Release();
                    PropVariantClear(&nameProp);
                    props->Release();
                    device->Release();
                    deviceCollection->Release();
                    enumerator->Release();
                    CoUninitialize();
                    return SUCCEEDED(hr);
                }
            }

            PropVariantClear(&nameProp);
            props->Release();
            device->Release();
        }
    }

    deviceCollection->Release();
    enumerator->Release();
    CoUninitialize();
    return false;
}
#endif
#endif

MAudio::MAudio(QObject *parent) : QObject{ parent }
{
}

MAudioOutput * MAudio::MAudio_CreateDev(MAudioDev *dev) {

    MAudioOutput *AudioOutput;

    if(!dev)
        return NULL;

#ifdef IS_QT5
    AudioOutput = new QAudioOutput(dev->device, dev->format);
#else

    AudioOutput = new QAudioSink(dev->device, dev->format);

    if(AudioOutput)
        AudioOutput->setBufferSize(fluid_output->fluid_out_samples * (dev->format.bytesPerSample() * 2));

    //qWarning("bysamp %i", dev->format.bytesPerSample());
#endif

    return AudioOutput;
}

#ifdef IS_QT5
MAudioDev * MAudio::MAudio_findDev(MAudioDev param) {

    MAudioDev *ret = new MAudioDev;
    if(!ret)
        return NULL; // error creating MAudioDev;

    ret->device = QAudioDeviceInfo::defaultOutputDevice();
    ret->device_name = ret->device.deviceName();
    ret->is_default_device = false;
    ret->sample_rate = 44100;
    ret->is_float = false;
    ret->err = 0;   

    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);

    devices.append(ret->device);
    devices.move(devices.count() - 1, 0);

    int max_out = devices.count();
    int def = -1;

    for(int n = 0; n < max_out; n++) {

        if(devices.at(n).deviceName() == param.device_name) {

            def = n; break;
        }

        if(def < 0 && devices.at(n).deviceName() == ret->device_name) def = n;
    }

    if(max_out < 1) {
        ret->err = -1; // Audio Device not found
        return ret;
    }

    if(def < 0) def = 0; // get by default

    if(def == 0)
        ret->is_default_device = true;
    else
        ret->device_name = devices.at(def).deviceName();

    if(def >= 0) {

        ret->device = devices.at(def);
        QList<int> s = ret->device.supportedSampleRates();

        int m = s.count();
        int freq = -1;

        for(int n = 0; n < m; n++) {
            if(param.sample_rate == s.at(n)) {
                freq = param.sample_rate;
                break;
            }

        }

        if(freq < 0)
            ret->sample_rate = 44100;
        else
            ret->sample_rate = freq;
    }


    ret->float_is_supported = false;
    QAudioFormat format;

    format.setSampleRate(ret->sample_rate);
    format.setChannelCount(2);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    if(def >= 0) {

        QAudioDeviceInfo info(devices.at(def));

        format.setSampleSize(32);
        format.setSampleType(QAudioFormat::Float);

        info.isFormatSupported(format);
        if(!info.isFormatSupported(format)) {
            format.setSampleSize(16);
            format.setSampleType(QAudioFormat::SignedInt);
            ret->is_float = false;
        } else {
            ret->float_is_supported = true;
            if(!param.is_float) {
                format.setSampleSize(16);
                format.setSampleType(QAudioFormat::SignedInt);
            }
        }
    }

    ret->format = format;
    return ret;
}

#else
MAudioDev * MAudio::MAudio_findDev(MAudioDev param) {

    MAudioDev *ret = new MAudioDev;

    if(!ret)
        return NULL; // error creating MAudioDev;

    bool have16bits = false;
    bool havefloat = false;

    ret->device = QMediaDevices::defaultAudioOutput();
    ret->device_name = QString("default") ;
    ret->is_default_device = false;
    ret->sample_rate = ret->device.minimumSampleRate();
    ret->is_float = false;
    ret->err = 0;

    QList<QAudioDevice> devices = QMediaDevices::audioOutputs();
    QAudioFormat format;

    devices.append(ret->device);
    devices.move(devices.count() - 1, 0);

    int max_out = devices.count();
    int def = -1;


    if(param.device_name == QString("Default Output Device")) {
        def = 0;
        ret->is_default_device = true;
        //qWarning("it is default...");
    }

    for(int n = 1; n < max_out; n++) {

        if(devices.at(n).description() == param.device_name) {

            def = n; break;
        }


    }

    if(max_out < 1) {
        ret->err = -1; // Audio Device not found
        return ret;
    }

    if(def < 0) def = 0; // get by default

    if(def >= 0) {

        ret->device = devices.at(def);
        if(def == 0)
            ret->device_name = QString("Default Output Device");
        else
            ret->device_name = ret->device.description();

        // test Audioformat supported

#if defined(__WINDOWS_MM__)

        WAVEFORMATEX *format = nullptr;
        QAudioDevice qtDev = ret->device;

        if (getWaveFormatFromQAudioDevice(ret->device_name, &format)) {
#if 0
            qDebug() << "Real Format: " << format->nSamplesPerSec << " Hz"
                     << format->nChannels << " canales, "
                     << format->wBitsPerSample << " bits";
#endif
            if(format->wBitsPerSample == 16)
                have16bits = true;
            if(format->wBitsPerSample == 32)
                havefloat = true;

            CoTaskMemFree(format);
        } else {
            qWarning("Failed to get the actual format of the device");
        }

#else

        QList<QAudioFormat::SampleFormat> sf =	ret->device.supportedSampleFormats();

        for(int n = 0; n < sf.length();n++) {
            switch (sf.at(n)) {

                case QAudioFormat::Int16:
                    have16bits = true;
                break;

                case QAudioFormat::Float:
                    havefloat = true;
                break;

            }
        }
#endif
        if(have16bits == false && havefloat == false) {
            ret->err = -2; // Audio format unsupported
            return ret;
        }


        QList<int> s;
        s.append(ret->device.minimumSampleRate());
        if(ret->device.minimumSampleRate() != ret->device.maximumSampleRate())
             s.append(ret->device.maximumSampleRate());

        int m = s.count();
        int freq = -1;

        for(int n = 0; n < m; n++) {
            if(param.sample_rate == s.at(n)) {
                freq = param.sample_rate;
                break;
            }

        }

        if(freq < 0) {
            ret->sample_rate = ret->device.minimumSampleRate();
        } else
            ret->sample_rate = freq;
    }


    ret->float_is_supported = havefloat;

    // verify format is supported
    format.setSampleRate(ret->sample_rate);
    format.setChannelCount(2);

    if((!param.is_float && have16bits) || (param.is_float && !havefloat)) {
        format.setSampleFormat(QAudioFormat::Int16);
        ret->is_float = false;
    } else {
        format.setSampleFormat(QAudioFormat::Float);
        ret->is_float = true;
    }

    format.setChannelConfig(QAudioFormat::ChannelConfigStereo);

    if(def >= 0) {

        //QAudioDevice info(devices.at(def));

        if(0)
        if(!ret->device.isFormatSupported(format)) {
            qWarning("unsuporrteeed");
            ret->err = -1; // Audio format unsupported
            return ret;
        }

    } else {
        ret->err = -1; // Audio Device not found
        return ret;
    }

    ret->format = format;
    return ret;
}

#endif

#ifdef IS_QT5
QList<QString> MAudio::MAudio_OutputDevices() {

    QList<QAudioDeviceInfo> dev_out;
    QList<QString> dev_out_name;

    dev_out = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    dev_out.append(QAudioDeviceInfo::defaultOutputDevice());
    dev_out.move(dev_out.count() - 1, 0);

    for(int n = 0; n < dev_out.count();n++) {
        dev_out_name.append(dev_out[n].deviceName());
    }

    return dev_out_name;
}
#else
QList<QString> MAudio::MAudio_OutputDevices() {

    QList<QAudioDevice> dev_out;
    QList<QString> dev_out_name;

    dev_out = QMediaDevices::audioOutputs();

    qWarning("default minimun %i", QMediaDevices::defaultAudioOutput().minimumSampleRate());
    dev_out.append(QMediaDevices::defaultAudioOutput());
    dev_out.move(dev_out.count() - 1, 0);

    for(int n = 0; n < dev_out.count();n++) {
        if(n == 0)
            dev_out_name.append(QString("Default Output Device"));
        else
            dev_out_name.append(dev_out[n].description());
    }

    return dev_out_name;
}
#endif

bool MAudio::MAudio_SupportedSampleRate(QString outputdevice, int samplerate) {

    MAudioDev param;
    param.device_name = outputdevice;
    param.is_float = false;
    param.sample_rate = samplerate;

    MAudioDev *dev = MAudio::MAudio_findDev(param);

    if(dev) {
        if(!dev->err) {

            bool ret = true;
            if(dev->sample_rate != samplerate)
                ret = false;
            delete dev;
            return ret;

        }

        delete dev;
    }

    return false;
}
#endif
