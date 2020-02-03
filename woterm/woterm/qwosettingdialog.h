#ifndef QWOSETTINGDIALOG_H
#define QWOSETTINGDIALOG_H

#include <QDialog>

namespace Ui {
class QWoSettingDialog;
}

class QWoSettingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QWoSettingDialog(QWidget *parent = nullptr);
    ~QWoSettingDialog();

private:
    Ui::QWoSettingDialog *ui;
};

#endif // QWOSETTINGDIALOG_H
