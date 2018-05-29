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

#include "SettingsWidget.h"

#include <QString>
#include <QLabel>
#include <QFrame>

SettingsWidget::SettingsWidget(QString title, QWidget *parent) :
		QWidget(parent)
{
	_title = title;
}

bool SettingsWidget::accept(){
	return true;
}

QString SettingsWidget::title(){
	return _title;
}


QWidget *SettingsWidget::createInfoBox(QString info){
	QLabel *label = new QLabel(info, this);
	label->setStyleSheet("color: gray; background-color: white; padding: 5px");
	label->setWordWrap(true);
	label->setAlignment(Qt::AlignJustify);
	return label;
}

QWidget *SettingsWidget::separator(){
	QFrame *f0 = new QFrame(this);
	f0->setFrameStyle( QFrame::HLine | QFrame::Sunken );
	return f0;
}

QIcon SettingsWidget::icon(){
	return QIcon();
}
