rem ------------- This script is used to create the windows installer. ---------- 

rem Required environment variables:

rem MIDIEDITOR_RELEASE_VERSION_ID=2
rem MIDIEDITOR_RELEASE_VERSION_STRING=3.1.0
rem MIDIEDITOR_PACKAGE_VERSION=1
rem INSTALLJAMMER=/path/to/installjammer
rem %1 -> build directory
rem %2 -> source directory

rem Setup folder structure

md MidiEditor-win64
md MidiEditor-win64\win_root
md MidiEditor-win64\win_root\encoders
md MidiEditor-win64\win_root\metronome

rem Copy binary
rem %1 -> build directory
rem %2 -> source directory
rem %3 -> Midieditor version string (optional)

setlocal enabledelayedexpansion

set RAW_PATH1=%1
set RAW_PATH2=%2

set "PATH_1=!RAW_PATH1:/=\!"
set "PATH_2=!RAW_PATH2:/=\!"

if "%~3"=="" (
    set "MIDI_RELEASE_F=%MIDIEDITOR_RELEASE_VERSION_STRING%"
) else (
    set "MIDI_RELEASE_F=%~3"
)


copy %PATH_1%\bin\midieditor.exe MidiEditor-win64\win_root\midieditor.exe

rem Copy Encoders
copy %PATH_2%\encoders\*.* MidiEditor-win64\win_root\encoders\

rem Copy metronome
copy %PATH_2%\packaging\metronome\. MidiEditor-win64\win_root\metronome
copy %PATH_2%\packaging\windows\windows-installer\. MidiEditor-win64

cd MidiEditor-win64

sh %INSTALLJAMMER% -DVersion %MIDI_RELEASE_F% --build-for-release windows-installer64.mpi

cd ..

mkdir install
copy /Y MidiEditor-win64\output\MidiEditor-%MIDI_RELEASE_F%-Setup.exe install\MidiEditor-%MIDI_RELEASE_F%-64bits-static-Setup.exe

