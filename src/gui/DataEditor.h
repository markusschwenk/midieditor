#ifndef DATAEDITOR_H
#define DATAEDITOR_H

#include <QScrollArea>

class QPushButton;
class QLineEdit;

class DataLineEditor : public QObject {

    Q_OBJECT

public:
    DataLineEditor(int line, QPushButton* plus, QPushButton* minus = 0, QLineEdit* edit = 0);

public slots:
    void plus();
    void minus();
    void changed(QString text);

signals:
    void dataChanged(int line, unsigned char data);
    void plusClicked(int line);
    void minusClicked(int line);

private:
    int _line;
};

class DataEditor : public QScrollArea {
    Q_OBJECT

public:
    DataEditor(QWidget* parent = 0);
    void setData(QByteArray data);
    QByteArray data();

    void rebuild();

public slots:
    void dataChanged(int line, unsigned char data);
    void plusClicked(int line);
    void minusClicked(int line);

private:
    QByteArray _data;
    QWidget* _central;
};

#endif
