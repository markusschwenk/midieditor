#include "DataEditor.h"

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

DataLineEditor::DataLineEditor(int line, QPushButton* plus, QPushButton* minus, QLineEdit* edit)
    : QObject()
{
    _line = line;
    connect(plus, SIGNAL(clicked()), this, SLOT(plus()));
    if (minus) {
        connect(minus, SIGNAL(clicked()), this, SLOT(minus()));
    }
    if (edit) {
        connect(edit, SIGNAL(textChanged(QString)), this, SLOT(changed(QString)));
    }
}

void DataLineEditor::plus()
{
    emit plusClicked(_line);
}

void DataLineEditor::minus()
{
    emit minusClicked(_line);
}

void DataLineEditor::changed(QString text)
{
    bool ok;
    int i = text.toInt(&ok, 16);
    if (ok) {
        emit dataChanged(_line, (unsigned char)i);
    }
}

DataEditor::DataEditor(QWidget* parent)
    : QScrollArea(parent)
{
    _central = new QWidget(this);
    setWidget(_central);
    setWidgetResizable(true);
    QGridLayout* layout = new QGridLayout(_central);
    _central->setLayout(layout);
    layout->setColumnStretch(4, 1);
}

void DataEditor::setData(QByteArray data)
{
    this->_data = data;
    rebuild();
}

QByteArray DataEditor::data()
{
    return _data;
}

void DataEditor::rebuild()
{

    QGridLayout* layout = dynamic_cast<QGridLayout*>(_central->layout());
    if (layout) {
        QList<QWidget*> widgets = _central->findChildren<QWidget*>();
        foreach (QWidget* child, widgets) {
            layout->removeWidget(child);
            delete child;
        }

        int row = 1;
        QPushButton* plus = new QPushButton("+", _central);
        layout->addWidget(plus, 0, 3, 1, 1);
        plus->setMaximumWidth(50);
        DataLineEditor* dle0 = new DataLineEditor(0, plus);
        connect(dle0, SIGNAL(plusClicked(int)), this, SLOT(plusClicked(int)));

        foreach (unsigned char c, _data) {
            QLabel* label = new QLabel("0x", _central);
            layout->addWidget(label, row, 0, 1, 1);

            QLineEdit* edit = new QLineEdit(_central);
            edit->setInputMask("HH");
            QString text;
            text.sprintf("%02X", c);
            edit->setText(text);
            layout->addWidget(edit, row, 1, 1, 1);

            QPushButton* minus = new QPushButton("-", _central);
            minus->setMaximumWidth(50);
            layout->addWidget(minus, row, 2, 1, 1);

            QPushButton* plus = new QPushButton("+", _central);
            layout->addWidget(plus, row, 3, 1, 1);
            plus->setMaximumWidth(50);

            DataLineEditor* dle = new DataLineEditor(row, plus, minus, edit);
            connect(dle, SIGNAL(minusClicked(int)), this, SLOT(minusClicked(int)));
            connect(dle, SIGNAL(plusClicked(int)), this, SLOT(plusClicked(int)));
            connect(dle, SIGNAL(dataChanged(int, unsigned char)), this, SLOT(dataChanged(int, unsigned char)));
            row++;
        }
    }
}

void DataEditor::dataChanged(int line, unsigned char data)
{
    _data[line - 1] = data;
}

void DataEditor::plusClicked(int line)
{
    _data = _data.insert(line, (char)0);
    rebuild();
}

void DataEditor::minusClicked(int line)
{
    _data = _data.remove(line - 1, 1);
    rebuild();
}
