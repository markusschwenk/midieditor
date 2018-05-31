#ifndef NTOLEQUANTIZATIONDIALOGONDIALOG_H
#define NTOLEQUANTIZATIONDIALOGONDIALOG_H

#include <QDialog>
class QComboBox;

class NToleQuantizationDialog : public QDialog {

    Q_OBJECT

public:
    NToleQuantizationDialog(QWidget* parent = 0);
    static int ntoleNNum, ntoleBeatNum, replaceNumNum, replaceDenomNum;

public slots:
    void takeResults();

private:
    QComboBox *ntoleN, *ntoleBeat, *replaceNum, *replaceDenom;
};

#endif
