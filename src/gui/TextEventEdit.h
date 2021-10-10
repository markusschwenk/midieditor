/********************************************************************************
** Form generated from reading UI file 'SoundEffectChooserUfuXWz.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef TEXTEVENTEDIT_H
#define TEXTEVENTEDIT_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>

class MidiFile;
#define TEXTEVENT_EDIT_TEXT 0
#define TEXTEVENT_EDIT_MARKER 1
#define TEXTEVENT_EDIT_LYRIK 2
#define TEXTEVENT_EDIT_TRACK_NAME 3
#define TEXTEVENT_NEW_TEXT 128
#define TEXTEVENT_NEW_MARKER 129
#define TEXTEVENT_NEW_LYRIK 130
#define TEXTEVENT_NEW_TRACK_NAME 131

class TextEventEdit: public QDialog
{
public:
    QDialogButtonBox *buttonBox;
    QLineEdit *textEdit;
    QLabel *label;
    QCheckBox *delBox;

    int result;

    TextEventEdit(MidiFile* f, int channel, QWidget* parent, int flag);

public slots:
    void accept();


private:
    MidiFile* _file;
    int _channel;
    int _flag;

};


#endif
