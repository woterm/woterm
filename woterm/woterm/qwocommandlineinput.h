#ifndef QWOCOMMANDLINEINPUT_H
#define QWOCOMMANDLINEINPUT_H

#include <QWidget>

namespace Ui {
class QWoCommandLineInput;
}

class QWoCommandLineInput : public QWidget
{
    Q_OBJECT

public:
    explicit QWoCommandLineInput(QWidget *parent = nullptr);
    ~QWoCommandLineInput();
signals:
    void returnPressed(const QString& cmd);

private slots:
    void onInputReturnPressed();
    void onCloseButtonClicked();

private:
    Ui::QWoCommandLineInput *ui;
};

#endif // QWOCOMMANDLINEINPUT_H
