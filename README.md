

<img align="left" width="70px" src="run_environment/midieditor.ico">

MidiEditor 
===========

-----------------
| **`  Linux  `** | **` Windows `** |  
|-----------------|---------------------|
| [![Build Status](https://api.travis-ci.org/abreheret/MidiEditor.svg?branch=master)](https://travis-ci.org/abreheret/MidiEditor) | [![Appveyor Build Status](https://img.shields.io/appveyor/ci/abreheret/midieditor.svg)](https://ci.appveyor.com/project/abreheret/MidiEditor) |


Download : Windows [last release](https://github.com/abreheret/MidiEditor/releases/)

### Building Dependencies
* [Qt >= 5.7](https://www.qt.io/download-open-source/)
* [CMake](https://cmake.org/download/) >= 2.8.11 
* Linux : 
     * [sfml](http://www.sfml-dev.org/download-fr.php) `sudo apt-get install libsfml-dev`
     * asound `sudo apt-get install libasound2-dev`
* Windows Compiler : Works under Visual Studio >= 2013
* Windows deployment : [nsis](http://nsis.sourceforge.net/Download)
 
### Introduction

MidiEditor is a free software providing an interface to edit, record, and play Midi data.

The editor is able to open existing Midi files and modify their content. New files can be created and the user can enter his own composition by either recording Midi data from a connected Midi device (e.g., a digital piano or a keyboard) or by manually creating new notes and other Midi events. The recorded data can be easily quantified and edited afterwards using MidiEditor.

![image](midieditor.png)

### Features

* Easily edits, records and plays Midi files
* can be connected to any Midi port (e.g., a digital piano or a synthesizer)
* Tracks, channels and Midi events can be edited
* Event quantization
* Control changes can be visualized
* Automatic Updates
* Free
* Available for Windows and Linux

### Note

MidiEditor was developed by Markus Schwenk. It is entirely written in C++ (Qt) and is available for the platforms Linux and Windows. Should MidiEditor be a software which is helpful for you and which you use often, please let the developer and other users know by providing feedback. Moreover, the developer worked on MidiEditor in his (rare) sparetime and offers it for free. So, when you feel like it, pay him a coffee (or two). Please also feel free to contact the developer in case you have any ideas which could help to improve the editor or in the case you found any bugs you want the developer to	fix.

### Suggestion, Improvements, Bugreports...

Please feel free to contact the developer if you have any suggestions! Please also reach out to the developer if you found a bug.

### Origin Project Page

[Here](https://sourceforge.net/projects/midieditor/) you can find the original project page on sourceforge.net. You will find the code and a way to provide feedback.


### Manual 

[Here](http://midieditor.sourceforge.net/index.php?category=manual)






