/*
 * MidiEditor
 * Copyright (C) 2010  Markus Schwenk
 *
 * tabConfig (FluidDialog)
 * Copyright (C) 2021 Francisco Munoz / "Estwald"
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


#ifdef USE_FLUIDSYNTH

#include "FluidDialog.h"
#include "../midi/MidiPlayer.h"

#include <QMessageBox>

extern instrument_list InstrumentList[129];
extern int itHaveInstrumentList;
extern int itHaveDrumList;

void FluidDialog::tab_Config(QDialog */*FluidDialog*/) {

    sf2Box = new QComboBox(tabConfig);
    sf2Box->setObjectName(QString::fromUtf8("sf2Box"));
    sf2Box->setGeometry(QRect(140, 20, 359 + 40, 22));
    sf2Box->setAcceptDrops(true);
    sf2Box->setMaxCount(10);
    sf2Box->setLayoutDirection(Qt::LeftToRight);
    sf2Box->setDuplicatesEnabled(true);
    int n, m;

    for(n = 0; n < 10; n++) {
        QString entry = QString("sf2_index")+QString().setNum(9-n);

        if (fluid_output->fluid_settings->value(entry).toString() != "") {
            if(QFile(fluid_output->fluid_settings->value(entry).toString()).exists()) {
                for(m = 0; m < n; m++) {
                    if(sf2Box->itemText(m) == fluid_output->fluid_settings->value(entry).toString()) {
                        fluid_output->fluid_settings->setValue(entry, ""); break;
                    }
                }
                if(n == m)
                    sf2Box->addItem(fluid_output->fluid_settings->value(entry).toString());
            } else fluid_output->fluid_settings->setValue(entry, "");
        }
    }

    if (fluid_output->fluid_settings->value("sf2_path").toString() != "")
        sf2Box->setCurrentText(fluid_output->fluid_settings->value("sf2_path").toString());

    int vmajor = FLUIDSYNTH_VERSION_MAJOR,
        vminor = FLUIDSYNTH_VERSION_MINOR,
        vmicro = FLUIDSYNTH_VERSION_MICRO;

    QString version;
    version.asprintf("version %i.%i.%i", vmajor, vminor, vmicro);
    QLabel *vlabel = new QLabel(tabConfig);
    vlabel->setObjectName(QString::fromUtf8("vlabel"));
    vlabel->setGeometry(QRect(140, 50, 132, 16));
    vlabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
    vlabel->setText(QString::asprintf("fluid synth LIB: v%i.%i.%i", vmajor, vminor, vmicro));

    vmajor = vminor = vmicro = 0;

    if(!fluid_output->status_fluid_err) fluid_version(&vmajor, &vminor, &vmicro);
    vlabel2 = new QLabel(tabConfig);
    vlabel2->setObjectName(QString::fromUtf8("vlabel2"));
    vlabel2->setGeometry(QRect(140, 70, 132, 16));
    vlabel2->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
    vlabel2->setText(QString::asprintf("fluid synth DLL: v%i.%i.%i", vmajor, vminor, vmicro));


    QString sf = QString("FluidSynth Status: ");

    switch(fluid_output->status_fluid_err) {
        case 0:
            sf+= "Ok"; break;
        case 1:
            sf+= "fail creating settings"; break;
        case 2:
            sf+= "fail creating synth"; break;
        case 3:
            sf+= "fail loading soundfont"; break;
        case 4:
            sf+= "fail creating thread"; break;
        default:
            sf+= "Unknown"; break;
    }

    QString sa = QString("Audio Output Status: ");

    switch(fluid_output->status_audio_err) {
        case 0:
            sa+= "Ok"; break;
        case 1:
            sa+= "fail creating Audio Output"; break;
        case 2:
            sa+= "fail starting Audio Output"; break;
        default:
            sa+= "Unknown"; break;
    }

    status_fluid = new QLabel(tabConfig);
    status_fluid->setObjectName(QString::fromUtf8("status_fluid"));
    status_fluid->setGeometry(QRect(140 + 132 + 40, 50, 200, 16));
    status_fluid->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
    status_fluid->setText(sf);

    status_audio = new QLabel(tabConfig);
    status_audio->setObjectName(QString::fromUtf8("status_audio"));
    status_audio->setGeometry(QRect(140 + 132 + 40, 70, 200, 16));
    status_audio->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
    status_audio->setText(sa);

    //connect(this, SIGNAL(signal_update_status()), this, SLOT(update_status()));

    loadSF2Button = new QToolButton(tabConfig);
    loadSF2Button->setObjectName(QString::fromUtf8("loadSF2Button"));
    loadSF2Button->setGeometry(QRect(30, 20, 79, 61));
    loadSF2Button->setText("Load\nSound Font");

    MIDIConnectbutton = new QToolButton(tabConfig);
    MIDIConnectbutton->setObjectName(QString::fromUtf8("MIDIConnectbutton"));
    MIDIConnectbutton->setGeometry(QRect(690-46, 10, 79, 61));
    MIDIConnectbutton->setText("MIDI Input\nConnect");
    MIDIConnectbutton->setDisabled(true);

    MIDISaveList = new QToolButton(tabConfig);
    MIDISaveList->setObjectName(QString::fromUtf8("MIDISaveList"));
    MIDISaveList->setGeometry(QRect(690-46-85, 10, 79, 61));
    MIDISaveList->setText("Save List\nSF2 Names");

    OutputAudio = new QGroupBox(tabConfig);
    OutputAudio->setObjectName(QString::fromUtf8("OutputAudio"));
    OutputAudio->setGeometry(QRect(30, 110, 741-46, 101));
    OutputBox = new QComboBox(OutputAudio);
    OutputBox->setObjectName(QString::fromUtf8("OutputBox"));
    OutputBox->setGeometry(QRect(90, 20, 381, 22));
    Outputlabel = new QLabel(OutputAudio);
    Outputlabel->setObjectName(QString::fromUtf8("Outputlabel"));
    Outputlabel->setGeometry(QRect(10, 20, 81, 16));
    OutputFreqBox = new QComboBox(OutputAudio);
    OutputFreqBox->setObjectName(QString::fromUtf8("OutputFreqBox"));
    OutputFreqBox->setGeometry(QRect(520, 20, 91, 22));
    OutputFreqlabel = new QLabel(OutputAudio);
    OutputFreqlabel->setObjectName(QString::fromUtf8("OutputFreqlabel"));
    OutputFreqlabel->setGeometry(QRect(480, 20, 31, 16));

    bufflabel = new QLabel(OutputAudio);
    bufflabel->setObjectName(QString::fromUtf8("bufflabel"));
    bufflabel->setGeometry(QRect(10, 60, 101, 16));
    bufflabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
    bufflabel->setText("Audio Buffer Size:");
    bufferSlider = new QSlider(OutputAudio);
    bufferSlider->setObjectName(QString::fromUtf8("bufferSlider"));
    bufferSlider->setGeometry(QRect(110, 60, 160, 22));
    bufferSlider->setMinimum(512);
    bufferSlider->setMaximum(2048);
    bufferSlider->setValue(fluid_output->fluid_settings->value("Out Samples" , 512).toInt());
    bufferSlider->setSingleStep(128);
    bufferSlider->setPageStep(128);
    bufferSlider->setTracking(false);
    bufferSlider->setOrientation(Qt::Horizontal);
    bufferSlider->setTickPosition(QSlider::TicksBelow);
    buffdisp = new QLabel(OutputAudio);
    buffdisp->setObjectName(QString::fromUtf8("buffdisp"));
    buffdisp->setGeometry(QRect(280, 60, 31, 16));
    buffdisp->setText(QString().number(bufferSlider->value()));
    OutFloat = new QCheckBox(OutputAudio);
    OutFloat->setObjectName(QString::fromUtf8("OutFloat"));
    OutFloat->setGeometry(QRect(624, 20, 70, 17));
    OutFloat->setText("Use Float");
    OutFloat->setChecked(fluid_output->fluid_settings->value("Out use float" , 0).toInt() ? true : false);

    connect(bufferSlider, QOverload<int>::of(&QSlider::valueChanged), [=](int size)
    {
        size = (size + 64) & ~127;
        fluid_output->fluid_out_samples = size;
        fluid_output->fluid_settings->setValue("Out Samples", size);
        bufferSlider->setValue(size);
        buffdisp->setNum(size);
    });

    connect(OutFloat, QOverload<bool>::of(&QCheckBox::clicked), [=](bool checked)
    {
        if(checked) fluid_output->output_float = 1; else fluid_output->output_float = 0;

        QThread::msleep(100);
        if(OutFloat->isChecked() != ((fluid_output->output_float) ? true : false))
        OutFloat->setChecked(fluid_output->output_float ? true : false);

    });

    WavAudio = new QGroupBox(tabConfig);
    WavAudio->setObjectName(QString::fromUtf8("WavAudio"));
    WavAudio->setGeometry(QRect(30, 230, 741 - 46, 101));
    WavFreqBox = new QComboBox(WavAudio);
    WavFreqBox->setObjectName(QString::fromUtf8("WavFreqBox"));
    WavFreqBox->setGeometry(QRect(90, 20, 91, 22));
    WavFreqlabel = new QLabel(WavAudio);
    WavFreqlabel->setObjectName(QString::fromUtf8("WavFreqlabel"));
    WavFreqlabel->setGeometry(QRect(50, 20, 31, 16));
    checkFloat = new QCheckBox(WavAudio);
    checkFloat->setObjectName(QString::fromUtf8("checkFloat"));
    checkFloat->setGeometry(QRect(210, 20, 70, 17));
    checkFloat->setCheckable(true);
    if(fluid_output->wav_is_float) checkFloat->setChecked(true);
    
    OutputAudio->setTitle("Output Audio");
    Outputlabel->setText("Output Device:");
    OutputFreqBox->setCurrentText(QString());
    OutputFreqlabel->setText("Freq:");

    WavAudio->setTitle("WAV Conversion");
    WavFreqBox->setCurrentText(QString());
    WavFreqlabel->setText("Freq:");
    checkFloat->setText("Use Float");

    WavFreqBox->addItem("44100 Hz", 44100);
    WavFreqBox->addItem("48000 Hz", 48000);
    WavFreqBox->addItem("88200 Hz", 88200);
    WavFreqBox->addItem("96000 Hz", 96000);

    fluid_output->_wave_sample_rate = fluid_output->fluid_settings->value("Wav Freq" , 44100).toInt();

    for(int n=0; n < WavFreqBox->count(); n++) {
        WavFreqBox->setCurrentIndex(n);
        if(WavFreqBox->currentData().toInt() ==
            fluid_output->_wave_sample_rate) break;

    }


    dev_out = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);

    int max_out= dev_out.count();
    for(int n=0; n < max_out; n++) {
        OutputBox->addItem(dev_out.at(n).deviceName());
    }

    if(max_out) {
        QList<int> freq = dev_out.at(OutputBox->currentIndex()).supportedSampleRates();
        int i = 0;
        int m= freq.count();
        int ind = -1;
        for(int n=0; n < m; n++) {
            if(freq.at(n) > 96000) continue; // not supported
            QString s;
            s.setNum(freq.at(n), 10)+ " Hz";
            OutputFreqBox->addItem(s, freq.at(i));

            if(freq.at(n)==fluid_output->_sample_rate) ind = i;

            if(freq.at(n)==44100 && ind < 0) ind = i;
            i++;
        }

        if(ind < 0) ind = 0;
        OutputFreqBox->setCurrentIndex(ind);
    }

    MP3Audio = new QGroupBox(tabConfig);
    MP3Audio->setObjectName(QString::fromUtf8("MP3Audio"));
    MP3Audio->setGeometry(QRect(30, 370, 741 - 46, 141));
    MP3BitrateBox = new QComboBox(MP3Audio);
    MP3BitrateBox->setObjectName(QString::fromUtf8("MP3BitrateBox"));
    MP3BitrateBox->setGeometry(QRect(50, 20, 91, 22));
    MP3Bitratelabel = new QLabel(MP3Audio);
    MP3Bitratelabel->setObjectName(QString::fromUtf8("MP3Bitratelabel"));
    MP3Bitratelabel->setGeometry(QRect(10, 20, 41, 16));
    MP3ModeButton = new QRadioButton(MP3Audio);
    MP3ModeButton->setObjectName(QString::fromUtf8("MP3ModeButton"));
    MP3ModeButton->setGeometry(QRect(50, 60, 51, 17));
    MP3ModeButton->setChecked(!fluid_output->fluid_settings->value("mp3_mode").toBool()); //-
    MP3ModeButton_2 = new QRadioButton(MP3Audio);
    MP3ModeButton_2->setObjectName(QString::fromUtf8("MP3ModeButton_2"));
    MP3ModeButton_2->setGeometry(QRect(100, 60, 51, 17));
    MP3ModeButton_2->setChecked(fluid_output->fluid_settings->value("mp3_mode").toBool()); //-
    MP3label_2 = new QLabel(MP3Audio);
    MP3label_2->setObjectName(QString::fromUtf8("MP3label_2"));
    MP3label_2->setGeometry(QRect(10, 60, 31, 16));
    MP3VBRcheckBox = new QCheckBox(MP3Audio);
    MP3VBRcheckBox->setObjectName(QString::fromUtf8("MP3VBRcheckBox"));
    MP3VBRcheckBox->setGeometry(QRect(10, 90, 51, 17));
    MP3VBRcheckBox->setChecked(fluid_output->fluid_settings->value("mp3_vbr").toBool()); //-
    MP3HQcheckBox = new QCheckBox(MP3Audio);
    MP3HQcheckBox->setObjectName(QString::fromUtf8("MP3HQcheckBox"));
    MP3HQcheckBox->setGeometry(QRect(60, 90, 71, 17));
    MP3HQcheckBox->setChecked(fluid_output->fluid_settings->value("mp3_hq").toBool()); //-

    MP3label_title = new QLabel(MP3Audio);
    MP3label_title->setObjectName(QString::fromUtf8("MP3label_title"));
    MP3label_title->setGeometry(QRect(250, 10, 47, 13));
    MP3label_artist = new QLabel(MP3Audio);
    MP3label_artist->setObjectName(QString::fromUtf8("MP3label_artist"));
    MP3label_artist->setGeometry(QRect(250, 40, 47, 13));
    MP3label_album = new QLabel(MP3Audio);
    MP3label_album->setObjectName(QString::fromUtf8("MP3label_album"));
    MP3label_album->setGeometry(QRect(250, 70, 47, 13));
    MP3label_genre = new QLabel(MP3Audio);
    MP3label_genre->setObjectName(QString::fromUtf8("MP3label_genre"));
    MP3label_genre->setGeometry(QRect(250, 100, 47, 13));

    MP3_lineEdit_title = new QLineEdit(MP3Audio);
    MP3_lineEdit_title->setObjectName(QString::fromUtf8("MP3_lineEdit_title"));
    MP3_lineEdit_title->setGeometry(QRect(290, 20, 311 - 40, 20));
    MP3_lineEdit_title->setMaxLength(64);
    MP3_lineEdit_title->setText(fluid_output->fluid_settings->value("mp3_title").toString()); //-
    MP3_lineEdit_artist = new QLineEdit(MP3Audio);
    MP3_lineEdit_artist->setObjectName(QString::fromUtf8("MP3_lineEdit_artist"));
    MP3_lineEdit_artist->setGeometry(QRect(290, 50, 311 - 40, 20));
    MP3_lineEdit_artist->setMaxLength(64);
    MP3_lineEdit_artist->setText(fluid_output->fluid_settings->value("mp3_artist").toString()); //-
    MP3_lineEdit_album = new QLineEdit(MP3Audio);
    MP3_lineEdit_album->setObjectName(QString::fromUtf8("MP3_lineEdit_album"));
    MP3_lineEdit_album->setGeometry(QRect(290, 80, 311 - 40, 20));
    MP3_lineEdit_album->setMaxLength(64);
    MP3_lineEdit_album->setText(fluid_output->fluid_settings->value("mp3_album").toString()); //-
    MP3_lineEdit_genre = new QLineEdit(MP3Audio);
    MP3_lineEdit_genre->setObjectName(QString::fromUtf8("MP3_lineEdit_genre"));
    MP3_lineEdit_genre->setGeometry(QRect(290, 110, 311 - 40, 20));
    MP3_lineEdit_genre->setMaxLength(64);
    MP3_lineEdit_genre->setText(fluid_output->fluid_settings->value("mp3_genre").toString()); //-
    MP3label_year = new QLabel(MP3Audio);
    MP3label_year->setObjectName(QString::fromUtf8("MP3label_year"));
    MP3label_year->setGeometry(QRect(620 - 40, 20, 31, 16));
    MP3label_track = new QLabel(MP3Audio);
    MP3label_track->setObjectName(QString::fromUtf8("MP3label_track"));
    MP3label_track->setGeometry(QRect(620 - 40, 50, 31, 16));
    MP3spinBox_year = new QSpinBox(MP3Audio);
    MP3spinBox_year->setObjectName(QString::fromUtf8("MP3spinBox_year"));
    MP3spinBox_year->setGeometry(QRect(660 - 40, 20, 61, 22));
    MP3spinBox_year->setMaximum(4000);
    MP3spinBox_year->setValue(fluid_output->fluid_settings->value("mp3_year").toInt()); //-

    MP3spinBox_track = new QSpinBox(MP3Audio);
    MP3spinBox_track->setObjectName(QString::fromUtf8("MP3spinBox_track"));
    MP3spinBox_track->setGeometry(QRect(660 - 40, 50, 61, 22));
    MP3spinBox_track->setMinimum(0);
    MP3spinBox_track->setMaximum(255);
    MP3spinBox_track->setValue(fluid_output->fluid_settings->value("mp3_track").toInt()); //-
    MP3checkBox_id3 = new QCheckBox(MP3Audio);
    MP3checkBox_id3->setObjectName(QString::fromUtf8("MP3checkBox_id3"));
    MP3checkBox_id3->setGeometry(QRect(190, 10, 41, 17));
    MP3checkBox_id3->setChecked(fluid_output->fluid_settings->value("mp3_id3").toBool());
    MP3Button_save = new QPushButton(MP3Audio);
    MP3Button_save->setObjectName(QString::fromUtf8("MP3Button_save"));
    MP3Button_save->setGeometry(QRect(170, 80, 61, 51));

    MP3Audio->setTitle(QCoreApplication::translate("FluidDialog", "MP3 Conversion/ FLAC Tags", nullptr));
    MP3BitrateBox->setCurrentText(QString());
    MP3BitrateBox->addItem("128 kbps", 128);
    MP3BitrateBox->addItem("160 kbps", 160);
    MP3BitrateBox->addItem("192 kbps", 192);
    MP3BitrateBox->addItem("224 kbps", 224);
    MP3BitrateBox->addItem("256 kbps", 256);
    MP3BitrateBox->addItem("320 kbps", 320);
    MP3BitrateBox->setCurrentIndex(fluid_output->fluid_settings->value("mp3_bitrate").toInt()); //-

    MP3Bitratelabel->setText(QCoreApplication::translate("FluidDialog", "Bitrate:", nullptr));
    MP3ModeButton->setText(QCoreApplication::translate("FluidDialog", "Joint", nullptr));
    MP3ModeButton_2->setText(QCoreApplication::translate("FluidDialog", "Simple", nullptr));
    MP3label_2->setText(QCoreApplication::translate("FluidDialog", "Mode:", nullptr));
    MP3VBRcheckBox->setText(QCoreApplication::translate("FluidDialog", "VBR", nullptr));
    MP3HQcheckBox->setText(QCoreApplication::translate("FluidDialog", "Hi Quality", nullptr));
    MP3label_title->setText(QCoreApplication::translate("FluidDialog", "Title:", nullptr));
    MP3label_artist->setText(QCoreApplication::translate("FluidDialog", "Artist:", nullptr));
    MP3label_album->setText(QCoreApplication::translate("FluidDialog", "Album:", nullptr));
    MP3label_genre->setText(QCoreApplication::translate("FluidDialog", "Genre:", nullptr));
    MP3label_year->setText(QCoreApplication::translate("FluidDialog", "Year:", nullptr));
    MP3label_track->setText(QCoreApplication::translate("FluidDialog", "Track:", nullptr));
    MP3checkBox_id3->setText(QCoreApplication::translate("FluidDialog", "ID3", nullptr));
    MP3Button_save->setText(QCoreApplication::translate("FluidDialog", "Save ID3", nullptr));

    QObject::connect(loadSF2Button, SIGNAL(clicked()), this, SLOT(load_SF2()));
    QObject::connect(sf2Box, SIGNAL(textActivated(QString)), this, SLOT(load_SF2(QString)));
    QObject::connect(MIDIConnectbutton, SIGNAL(clicked()), this, SLOT(MIDIConnectOnOff()));
    QObject::connect(MIDISaveList, SIGNAL(clicked()), this, SLOT(MIDISaveListfun()));

    QObject::connect(OutputBox, SIGNAL(activated(int)), this, SLOT(setOutput(int)));
    QObject::connect(OutputFreqBox, SIGNAL(activated(int)), this, SLOT(setOutputFreq(int)));
    QObject::connect(WavFreqBox, SIGNAL(activated(int)), this, SLOT(setWavFreq(int)));

    connect(checkFloat, QOverload<bool>::of(&QCheckBox::clicked), [=](bool checked)
    {
        if(checked) fluid_output->wav_is_float = 1; else fluid_output->wav_is_float = 0;

        fluid_output->fluid_settings->setValue("Wav to Float", fluid_output->wav_is_float);

    });

    QObject::connect(MP3Button_save, SIGNAL(clicked()), this, SLOT(save_MP3_settings()));
}

void FluidDialog::update_status() {

    if(!disable_mainmenu && fluid_output->status_fluid_err) {
        disable_mainmenu = 1;
        tabWidget->removeTab(tabWidget->indexOf(MainVolume));
        MainVolume->setDisabled(true);
    }
    if(disable_mainmenu && !fluid_output->status_fluid_err) {
        disable_mainmenu = 0;
        tabWidget->addTab(MainVolume, QString());
        tabWidget->setTabText(tabWidget->indexOf(MainVolume), "Main Volume");
        MainVolume->setDisabled(false);

        int vmajor = 0;
        int vminor = 0;
        int vmicro = 0;

        fluid_version(&vmajor, &vminor, &vmicro);

        vlabel2->setText(QString::asprintf("fluid synth DLL: v%i.%i.%i", vmajor, vminor, vmicro));

    }

    if(status_fluid) {

        QString sf = QString("FluidSynth Status: ");

        switch(fluid_output->status_fluid_err) {
            case 0:
                sf+= "Ok"; break;
            case 1:
                sf+= "fail creating settings"; break;
            case 2:
                sf+= "fail creating synth"; break;
            case 3:
                sf+= "fail loading soundfont"; break;
            case 4:
                sf+= "fail creating thread"; break;
            default:
                sf+= "Unknown"; break;
        }

        if(!fluid_output->status_fluid_err)
            status_fluid->setStyleSheet(QString::fromUtf8("background-color: #8000C000;"));
        else
            status_fluid->setStyleSheet(QString::fromUtf8("background-color: #80C00000;"));

        status_fluid->setText(sf);

    }

    if(status_audio) {

        QString sa = QString("Audio Output Status: ");

        switch(fluid_output->status_audio_err) {
            case 0:
                sa+= "Ok"; break;
            case 1:
                sa+= "fail creating Audio Output"; break;
            case 2:
                sa+= "fail starting Audio Output"; break;
            default:
                sa+= "Unknown"; break;
        }

        if(!fluid_output->status_audio_err)
            status_audio->setStyleSheet(QString::fromUtf8("background-color: #8000C000;"));
        else
            status_audio->setStyleSheet(QString::fromUtf8("background-color: #80C00000;"));

        status_audio->setText(sa);
    }
    update();
}

void FluidDialog::setOutput(int index) {

    if (MidiPlayer::isPlaying()) {
        QMessageBox::information(this, "Ehhh!", "You cannot change it playing a MIDI File...");
        return;
    }

    if(dev_out.count()) {
        QList<int> freq = dev_out.at(index).supportedSampleRates();
        int i = 0;
        int m= freq.count();
        int ind = -1;
        OutputFreqBox->clear();
        for(int n=0; n < m; n++) {
            if(freq.at(n) > 96000) continue; // not supported
            QString s;
            s.setNum(freq.at(n), 10)+ " Hz";
            OutputFreqBox->addItem(s, freq.at(i));

            if(freq.at(n)==fluid_output->_sample_rate) ind = i;

            if(freq.at(n)==44100 && ind < 0 ) ind = i;
            i++;
        }

        if(ind < 0) ind = 0;
        OutputFreqBox->setCurrentIndex(ind);
        OutputFreqBox->activated(ind);
    }

}

void FluidDialog::setOutputFreq(int) {

    if (MidiPlayer::isPlaying()) {
        QMessageBox::information(this, "Ehhh!", "You cannot change it playing a MIDI File...");
        return;
    }
/*
    if(0 && fluid_output->it_have_error == 2) {
        QMessageBox::information(this, "Apuff!", "Fluid Synth engine is stopped\nbecause a critical error");
        return;
    }
*/
    int freq = OutputFreqBox->currentData().toInt();

    fluid_output->stop_audio(true);

    if(fluid_output->out_sound_io) fluid_output->out_sound_io->close();
    fluid_output->out_sound_io = NULL;
    if(fluid_output->out_sound) {
        fluid_output->out_sound->stop();
        fluid_output->out_sound->disconnect(this);
        fluid_output->out_sound = NULL;
        delete fluid_output->out_sound;
    }
    fluid_output->out_sound = NULL;

    QAudioFormat format;
    format.setSampleRate(freq);
    format.setChannelCount(2);
    format.setSampleSize(32);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::Float);

    QAudioDeviceInfo info(dev_out.at(OutputBox->currentIndex()));

    info.isFormatSupported(format);
    if(!info.isFormatSupported(format) || !fluid_output->output_float) {
        format.setSampleSize(16);
        format.setSampleType(QAudioFormat::SignedInt);
        fluid_output->output_float = 0;
    } else fluid_output->output_float = 1;

    fluid_output->out_sound= new QAudioOutput(dev_out.at(OutputBox->currentIndex()), format);
    if(fluid_output->out_sound) {

        fluid_output->out_sound_io = fluid_output->out_sound->start();
        if(fluid_output->out_sound_io) {

            fluid_output->_sample_rate = format.sampleRate();

            //fluid_output->change_synth(fluid_output->_sample_rate, 1);

        }

    }

    fluid_output->new_sound_thread(fluid_output->_sample_rate);

    fluid_output->stop_audio(false);

    if(!fluid_output->out_sound) {

        QMessageBox::information(this, "FATAL ERROR", "QAudioOutput() cannot be created");
        return;
    }

    if(!fluid_output->out_sound_io) {

        QMessageBox::information(this, "FATAL ERROR", "QAudioOutput() cannot starts");
        return;
    }

    if(fluid_output->fluid_settings) {
        fluid_output->fluid_settings->setValue("Audio Device", dev_out.at(OutputBox->currentIndex()).deviceName());
        fluid_output->fluid_settings->setValue("Audio Freq", fluid_output->_sample_rate);
    }

    fluid_output->MidiClean();

    update_status();

}

void FluidDialog::setWavFreq(int) {
    int freq = WavFreqBox->currentData().toInt();
    fluid_output->_wave_sample_rate = freq;
    if(fluid_output->fluid_settings) {

        fluid_output->fluid_settings->setValue("Wav Freq", fluid_output->_wave_sample_rate);
    }
}

void FluidDialog::save_MP3_settings() {

    QString info;

    info.append("title: " + MP3_lineEdit_title->text()+"\n");
    fluid_output->fluid_settings->setValue("mp3_title", MP3_lineEdit_title->text());
    info.append("artist: " + MP3_lineEdit_artist->text()+"\n");
    fluid_output->fluid_settings->setValue("mp3_artist", MP3_lineEdit_artist->text());
    info.append("album: " + MP3_lineEdit_album->text()+"\n");
    fluid_output->fluid_settings->setValue("mp3_album", MP3_lineEdit_album->text());
    info.append("genre: " + MP3_lineEdit_genre->text()+"\n");
    fluid_output->fluid_settings->setValue("mp3_genre", MP3_lineEdit_genre->text());
    QString a;
    a.setNum(MP3BitrateBox->currentIndex());
    info.append("mp3_bitrate: " + a +"\n");
    fluid_output->fluid_settings->setValue("mp3_bitrate", MP3BitrateBox->currentIndex());
    a.setNum(MP3ModeButton_2->isChecked() ? 1 : 0);
    info.append("mp3_mode: " + a +"\n");
    fluid_output->fluid_settings->setValue("mp3_mode", MP3ModeButton_2->isChecked());
    a.setNum(MP3VBRcheckBox->isChecked() ? 1 : 0);
    info.append("mp3_vbr: " + a +"\n");
    fluid_output->fluid_settings->setValue("mp3_vbr", MP3VBRcheckBox->isChecked());
    a.setNum(MP3HQcheckBox->isChecked() ? 1 : 0);
    info.append("mp3_hq: " + a +"\n");
    fluid_output->fluid_settings->setValue("mp3_hq", MP3HQcheckBox->isChecked());
    fluid_output->fluid_settings->setValue("mp3_id3", MP3checkBox_id3->isChecked());
    a.setNum(MP3spinBox_year->value());
    info.append("mp3_year: " + a +"\n");
    fluid_output->fluid_settings->setValue("mp3_year", MP3spinBox_year->value());
    a.setNum(MP3spinBox_track->value());
    info.append("mp3_track: " + a +"\n");
    fluid_output->fluid_settings->setValue("mp3_track", MP3spinBox_track->value());

    ((MainWindow *) _parent)->getFile()->protocol()->startNewAction("Save Copyright info");

    TextEvent *event = new TextEvent(16, ((MainWindow *) _parent)->getFile()->track(0));
    event->setType(TextEvent::COPYRIGHT);
    event->setText(info);
    ((MainWindow *) _parent)->getFile()->channel(16)->insertEvent(event, 0);

    // borra eventos repetidos proximos
    foreach (MidiEvent* event2, *(((MainWindow *) _parent)->getFile()->eventsBetween(0, 10))) {
        TextEvent* toRemove = dynamic_cast<TextEvent*>(event2);
        if (toRemove && toRemove != event && toRemove->type() == TextEvent::COPYRIGHT) {
            ((MainWindow *) _parent)->getFile()->channel(16)->removeEvent(toRemove);
        }
    }

    ((MainWindow *) _parent)->getFile()->protocol()->endAction();

}

void FluidDialog::load_SF2() {
    load_SF2("");
}

void FluidDialog::load_SF2(QString path) {

    MainWindow *MWin = ((MainWindow *) _parent); // get MainWindow :D
    QString sf2Dir;

    if (MidiPlayer::isPlaying()) {
        QMessageBox::information(this, "Ehhh!", "You cannot change it playing a MIDI File...");
        return;
    }

    /*
    if(fluid_output->it_have_error == 2) {
        QMessageBox::information(this, "Apuff!", "Fluid Synth engine is stopped\nbecause a critical error");
        return;
    }
    */

    if(path =="") {  

        sf2Dir = QDir::rootPath();

        if (fluid_output->fluid_settings->value("sf2_path").toString() != "") {
            sf2Dir = fluid_output->fluid_settings->value("sf2_path").toString();
        } else {
            fluid_output->fluid_settings->setValue("sf2_path", sf2Dir);
        }

        QString newPath = QFileDialog::getOpenFileName(this, "Open SF2/SF3 file",
            sf2Dir, "SF2/SF3 Files(*.sf2 *.sf3)");
        if(newPath.isEmpty()) {
            return; // canceled
        }

        sf2Dir = newPath;
        sf2Box->addItem(sf2Dir);
        for(int n = 0; n < sf2Box->count(); n++) {
            QString entry=QString("sf2_index")+QString().setNum(n);
            fluid_output->fluid_settings->setValue(entry, sf2Box->itemText(n).toUtf8());
        }

    } else sf2Dir = path;

    if(!QFile::exists(sf2Dir.toUtf8())) { // removing is not found

        for(int n = 0; n < sf2Box->count(); n++) {
            QString entry=QString("sf2_index")+QString().setNum(n);

            if(sf2Dir.toUtf8() == sf2Box->itemText(n))
                    sf2Box->removeItem(n);
            if(fluid_output->fluid_settings->value(entry).toString() == sf2Dir.toUtf8()) {
                 fluid_output->fluid_settings->setValue(entry, "");
            }
        }

        QMessageBox::information(NULL, "SF2 file not found", sf2Dir.toUtf8());
        return;
    }

    sf2Box->setCurrentText(sf2Dir.toUtf8());
    fluid_output->fluid_settings->setValue("sf2_path", sf2Dir.toUtf8());

    if(fluid_output->sf2_name) delete fluid_output->sf2_name;
    fluid_output->sf2_name = new QString(sf2Dir);

    if(!fluid_output->status_audio_err)
        fluid_output->stop_audio(true);

    addInstrumentNamesFromSF2((const char *) sf2Dir.toUtf8());
    MWin->update_channel_list();

    QThread::msleep(100);
    if(!fluid_output->status_audio_err) { // test if Audio Output Ok
        fluid_output->new_sound_thread(fluid_output->_sample_rate);
        fluid_output->stop_audio(false);
        fluid_output->MidiClean();

        update_status();
    } else { // initialize audio output too (if it is possible)
        setOutputFreq(fluid_output->_sample_rate);
    }

}

int addInstrumentNamesFromSF2(const char *filename)
{

    QFile* f = new QFile(filename);

    unsigned char name[0x26];
    unsigned short preset, bank;
    unsigned dat, block;
    long long size;
    long long pos;

    itHaveInstrumentList = 0;
    itHaveDrumList = 0;
    int r;

    int n, m;
    for(n = 0; n < 129; n++)
        for(m = 0; m < 128; m++)
            InstrumentList[n].name[m]="undefined";

    if (!f->open(QIODevice::ReadOnly)) {
        QMessageBox::information(NULL, "Error openning file", filename);
        return -666; // file not found
    }


    name[0] = 0;
    r = f->read((char *) name, 4);
    if(r != 4 && name[0] != 'R' && name[1] != 'I' && name[2] != 'F' && name[3] != 'F') {
        f->close(); return -1; // no RIFF code
    }

    dat = 0;
    r = f->read((char  *) &dat, 4);
    if(r != 4 || dat == 0) {
        f->close(); return -2; // error reading size
    }

    size = (long long) dat;

    name[0] = 0;
    r = f->read((char *) name, 4);
    size-= 4;

    if(r != 4
       && name[0] != 's' && name[1] != 'f' && name[2] != 'b' && name[3] != 'k') {
        f->close();
        return -3; // no sound font name
    }

    int flag = 0;

    pos = 12;
    f->seek(pos);

    name[4]=0;

    while(size > 0) {
        if(flag == 0) { // reading LIST

            name[0] = 0;
            r = f->read((char *) name, 4);
            size-= 4;

            if(r != 4 ||
               name[0] != 'L' || name[1] != 'I' || name[2] != 'S' || name[3] != 'T') {
                f->close();
                return 0; // end of LIST
            }

            block = 0; size-= 4;
            r = f->read((char  *) &block, 4);

            if(r != 4 || block == 0) {
                f->close();
                return 0; // end of LIST
            }

            pos+= 8;
            name[0] = 0;

            r = f->read((char *) name, 4);

            if(r != 4 ||
                name[0] != 'p' || name[1] != 'd' || name[2] != 't' || name[3] != 'a') {

                if(r != 4) { // error reading
                    f->close();
                    return -3;
                }

                pos += block;
                f->seek(pos);
                size-= block;
                continue; // to the next LIST

            } else { // to read pdta section
                block -= 4;
                size = block;
                pos += 4;
                f->seek(pos);
                flag = 1;
            }
        }

        if(flag == 1) { // pdta

            name[0]=0;

            r = f->read((char *) name, 4);

            if(r != 4) { // error reading
                f->close();
                return -3;
            }

            r=f->read((char  *) &block, 4);

            if(r != 4) { // error reading
                f->close();
                return -3;
            }

            if(name[0]!='p' || name[1]!='h' || name[2]!='d' || name[3]!='r') {
                pos += 8 + block; // skip block
                f->seek(pos);
                size-= block+8;
                continue;
            } else { // to read instruments
                size=block;
                pos += 8;
                f->seek(pos);
                flag = 2;
            }

        }

        if(flag == 2) {
            r = f->read((char *) name, 0x26); size-=0x26;
            pos += 0x26;
            if(r != 0x26) { // error reading
                f->close();
                return -3;
            }

            preset = name[0x14] | (name[0x15]<<8);
            bank = name[0x16] | (name[0x17]<<8);
            name[0x14] = 0; // \0 add

            if(!strcmp((char *) name, "EOP")) { // END OF LIST
                f->close();
                return 0;
            }

            if(bank > 128 || preset > 127) continue; //ignore it
            InstrumentList[bank].name[preset]= QByteArray((char *) name);

            itHaveInstrumentList = 1;
            if(bank == 128) itHaveDrumList = 1;
            //if(bank == 128)
            //    MSG_OUT("n %i:%i %s\n", bank, preset, name);

        }

    }

    f->close();
    return 0;


}

static int MIDI_onoff = 0;

void FluidDialog::MIDIConnectOnOff() {

   if(MIDI_onoff == 0) {
       if(fluid_output->MIDIconnect(1)==0) {
           MIDI_onoff = 1;
           MIDIConnectbutton->setStyleSheet(QString::fromUtf8("background-color: green;"));
       }

   } else {
       MIDI_onoff = 0;
       MIDIConnectbutton->setStyleSheet(QString::fromUtf8("background-color: white;"));
       fluid_output->MIDIconnect(0);
   }

}

void FluidDialog::MIDISaveListfun() {

    QString appdir = QDir::homePath();
    QString path = appdir+"/instrumentlist.lsf";
    MainWindow *MWin = ((MainWindow *) _parent); // get MainWindow :D

    QSettings* settings = MWin->getSettings();

    QString newPath = QFileDialog::getSaveFileName(this, "Save List SF2 names",
        path, "List SF2 files (*.lsf)");

    if(newPath.isEmpty()) {
        QMessageBox::information(this, "Save List SF2 names", "save aborted!");
        return;
    }

    if(!newPath.endsWith(".lsf")) {
        newPath += ".lsf";
    }

    QFile* f = new QFile(newPath);

    if (!f->open(QIODevice::WriteOnly | QIODevice :: Text)) {
        QMessageBox::information(this, "Error Creating", newPath);
        return;
    }

    for(int n=0; n< 129; n++)
        for(int m=0; m<128; m++) {
            if(InstrumentList[n].name[m] != "undefined") {
                char head[32]="";
                sprintf(head,"%3.3i:%3.3i = ", n, m);
                QByteArray line;
                line.append(head);
                line.append(InstrumentList[n].name[m]);
                line.append('\n');
                if(f->write(line) < 0) {
                    QMessageBox::information(this, "Error Writting", newPath);
                }
            }
        }
    f->close();

    settings->setValue("instrumentList", newPath);

    QMessageBox::information(this, "List Saved At", newPath);
}

#endif
