#ifndef QWOABOUTDIALOG_H
#define QWOABOUTDIALOG_H

#include <QDialog>

namespace Ui {
class QWoAboutDialog;
}

class QWoAboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QWoAboutDialog(QWidget *parent = nullptr);
    ~QWoAboutDialog();

private:
    Ui::QWoAboutDialog *ui;
};

#endif // QWOABOUTDIALOG_H
