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

#include <QApplication>

#include "gui/MainWindow.h"
#include "midi/MidiInput.h"
#include "midi/MidiOutput.h"

#include <QFile>
#include <QTextStream>

#include "UpdateManager.h"
#include <QMultiMap>
#include <QResource>

#ifdef USE_FLUIDSYNTH
    #include "fluid/fluidsynth_proc.h"
    fluidsynth_proc *fluid_output=NULL;
#endif

#ifdef NO_CONSOLE_MODE
#include <tchar.h>
#include <windows.h>
std::string wstrtostr(const std::wstring& wstr)
{
    std::string strTo;
    char* szTo = new char[wstr.length() + 1];
    szTo[wstr.size()] = '\0';
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, szTo, (int)wstr.length(),
        NULL, NULL);
    strTo = szTo;
    delete[] szTo;
    return strTo;
}
int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show)
{
    int argc = 1;
    char* argv[] = { "", "" };
    std::string str;
    LPWSTR* szArglist = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (NULL != szArglist && argc > 1) {
        str = wstrtostr(szArglist[1]);
        argv[1] = &str.at(0);
        argc = 2;
    }

#else
int main(int argc, char* argv[])
{
#endif

    QApplication a(argc, argv);
    bool ok = QResource::registerResource(a.applicationDirPath() + "/ressources.rcc");
    if (!ok) {
        ok = QResource::registerResource("ressources.rcc");
    }
#ifndef CUSTOM_MIDIEDITOR
    UpdateManager::instance()->init();
    a.setApplicationVersion(UpdateManager::instance()->versionString());
    a.setApplicationName("MidiEditor");
    a.setQuitOnLastWindowClosed(true);
    a.setProperty("date_published", UpdateManager::instance()->date());
#else
#define STRINGIZER(arg) #arg
#define STR_VALUE(arg) STRINGIZER(arg)
    a.setApplicationVersion(STR_VALUE(MIDIEDITOR_RELEASE_VERSION_STRING_DEF));
    a.setApplicationName("MidiEditor");
    a.setQuitOnLastWindowClosed(true);
    a.setProperty("date_published", STR_VALUE(MIDIEDITOR_RELEASE_DATE_DEF));
#endif
#ifdef __ARCH64__
    a.setProperty("arch", "64");
#else
    a.setProperty("arch", "32");
#endif

#ifdef USE_FLUIDSYNTH
    fluid_output = new fluidsynth_proc();
    if(!fluid_output) {
        qWarning("Error creating fluid_output from main.c");
        exit(-1);
    }
#endif

    MidiOutput::init();
    MidiInput::init();

#ifdef USE_FLUIDSYNTH
    fluid_output->MidiClean();
#endif

    MainWindow* w;

#if 0

    if (argc == 2)
        w = new MainWindow(argv[1]);
    else
        w = new MainWindow();
    w->showMaximized();
 #else

    if (argc == 2) {
        // Modified by Estwald: get ANSI or UTF8 string

        int length = strlen(argv[1]);
        char *file = argv[1];

        QString str;
        char fileNew[length + 1];
        int counter =0;
        int is_utf8 = 0;
        for (int i = 0; i < length; i++) {
            unsigned char tempByte = (unsigned char) file[i];

            fileNew[i] = tempByte;
            if(tempByte & 128) {// UTF8 autodetecting decoding chars
                if(counter == 0) {
                    if((tempByte & ~7) == 0xF0) counter = 3;
                    else if((tempByte & ~15) == 0xE0) counter = 2;
                    else if((tempByte & ~31) == 0xC0) counter = 1;
                } else if((tempByte & 0xC0) == 0x80) {
                    counter--;
                    if(!counter) is_utf8 = 1;
                } else counter = 0;
            } else counter = 0;
        }
        /*

          ASCII section (0 to 127 ch) is the same thing, but 128+ in
          UTF8 characters is codified as 2 to 4 bytes length and you
          can try to detect this encode method to import correctly
          UTF8 characters, assuming Local8Bit (i have some .kar/.mid
          in this format and works fine) as alternative.

        */

        if(!is_utf8)
            str = QString::fromLocal8Bit((const char *) fileNew, length);
        else
            str = QString::fromUtf8((const char *) fileNew, length);

        w = new MainWindow(str);
    }
    else
        w = new MainWindow();
    w->showMaximized();
#endif
    int ret = a.exec();
    delete w;
    return ret;
}
