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

#ifndef PROTOCOLSTEP_H_
#define PROTOCOLSTEP_H_

#include <QStack>
#include <QString>

class ProtocolItem;
class QImage;

/**
 * \class ProtocolStep
 *
 * \brief ProtocolStep represents a Step in the Protocol, containing one ore
 * more ProtocolItems.
 *
 * Every ProtocolStep has a Stack with ProtocolItems. This items represent
 * a single Action in the Protocol. A new Item can be added with addItem().
 * Calling releaseStep() will release all single Items and copy them
 * to another ProtocolStep. This ProtocolStep is returned and represents
 * the undo Action of this ProtocolStep.
 * This is used to put a released undo-action onto the redo-stack of a
 * protocol.
 *
 */
class ProtocolStep {

public:
    /**
		 * \brief creates a new ProtocolStep with the given description.
		 */
    ProtocolStep(QString description, QImage* img = 0);

    /**
		 * \brief deletes the ProtocolStep.
		 */
    ~ProtocolStep();

    /**
		 * \brief adds item to the steps stack.
		 *
		 * Every item added with addItem() will be released on the call of
		 * releaseStep()
		 */
    void addItem(ProtocolItem* item);

    /**
		 * \brief returns the number of items on the stack.
		 */
    int items();

    /**
		 * \brief returns the steps Description.
		 */
    QString description();

    /**
		 * \brief returns the steps Image.
		 */
    QImage* image();

    /**
		 * \brief releases the ProtocolStep.
		 *
		 * Every item will be released. Every action will be written onto the
		 * returned ProtocolStep in reverse order. When calling
		 * ProtocolStep.releaseStep() from the undo stack, you can write the
		 * returned ProtoclStep onto the redo stack.
		 */
    ProtocolStep* releaseStep();

private:
    /**
		 * \brief Holds the Steps Description.
		 */
    QString _stepDescription;

    /**
		 * \brief Holds the Steps Image.
		 */
    QImage* _image;

    /**
		 * \brief The itemStack saves all ProtocolItems of the Step.
		 */
    QStack<ProtocolItem*>* _itemStack;
};

#endif
