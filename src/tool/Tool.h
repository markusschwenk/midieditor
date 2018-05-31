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

#ifndef TOOL_H
#define TOOL_H

#include <QImage>
#include <QList>
#include <QString>

#include "../protocol/ProtocolEntry.h"

class Protocol;
class MidiFile;
class EditorTool;
class ToolButton;
class StandardTool;

/**
 * \class Tool
 *
 * \brief Tool is the superclass for every Tool in the program.
 *
 * Every Tool can be represented by a ToolButton; for this is has to be set
 * either an image or an imagetext.
 *
 * Tool inherits ProtocolEntry, so every action on a Tool can be written to the
 * history of the program.
 *
 * The method selected() can be used to give the ToolButton another background.
 *
 * A Tool can either be accessed using the Toolbuttons or it may be set by the
 * StandardTool.
 * This Tool decides on every click in the Editor which Tool to take.
 * If a tool has a StandardTool not equal 0, it has to return to this standard
 * tool when its action has been finished.
 */

class Tool : public ProtocolEntry {

public:
    /**
		 * \brief creates a new Tool.
		 */
    Tool();

    /**
		 * \brief creates a new Tool copying all data from &other.
		 */
    Tool(Tool& other);

    /**
		 * \brief returns wether the Tool is selected or not.
		 */
    virtual bool selected();

    /**
		 * \brief sets the Tools image.
		 */
    void setImage(QString name);

    /**
		 * \brief returns the Tools image.
		 */
    QImage* image();

    /**
		 * \brief sets the Tools ToolTipText
		 */
    void setToolTipText(QString text);

    /**
		 * \brief returns the Tools imagetext.
		 */
    QString toolTip();

    /**
		 * \brief sets the Tools ToolButton.
		 */
    void setButton(ToolButton* b);

    /**
		 * \brief this method is called when the user presses the Tools Button.
		 */
    virtual void buttonClick();

    /**
		 * \brief returns the Tools ToolButton.
		 */
    ToolButton* button();

    /**
		 * \brief sets the static current Tool.
		 *
		 * This method is used by EditorTool
		 */
    static void setCurrentTool(EditorTool* editorTool);

    /**
		 * \brief returns the current Tool.
		 */
    static EditorTool* currentTool();

    /**
		 * \brief sets the static current MidiFile.
		 */
    static void setFile(MidiFile* file);

    /**
		 * \brief returns the currenty opened File.
		 */
    static MidiFile* currentFile();

    /**
		 * \brief returns the Protocol of the currently opened Document.
		 */
    static Protocol* currentProtocol();

    /**
		 * \brief sets the StandardTool. When the StandardTool is set, the Tool
		 * has to set StandardTool as currentTool when its action is finished
		 */
    void setStandardTool(StandardTool* stdTool);

    /*
		 * The following functions are redefinitions from the superclass
		 * ProtocolEntry
		 */
    virtual ProtocolEntry* copy();

    virtual void reloadState(ProtocolEntry* entry);

    MidiFile* file();

protected:
    /**
		 * \brief the Tools Button if existing.
		 */
    ToolButton* _button;

    /**
		 * \brief the image representing the Tool.
		 *
		 * Used in the protoc list and on the Buttons.
		 */
    QImage* _image;

    /**
		 * \brief The ToolTip the Button should display.
		 */
    QString _toolTip;

    /**
		 * \brief the StandardTool.
		 *
		 * If existing the Tool has to set _standardTool as current tool
		 * after his action has been finished.
		 */
    StandardTool* _standardTool;

    /**
		 * \brief the current opened file.
		 */
    static MidiFile* _currentFile;

    /**
		 * \brief The active EditorTool.
		 *
		 * Is not always the selected Tool (If the selected tool is the
		 * StandardTool).
		 */
    static EditorTool* _currentTool;
};

#endif
