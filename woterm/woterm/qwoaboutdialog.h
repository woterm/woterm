#ifndef QWOABOUTDIALOG_H
#define QWOABOUTDIALOG_H

#include <QDialog>
#include <QPointer>

namespace Ui {
class QWoAboutDialog;
}

class QWoAboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QWoAboutDialog(QWidget *parent = nullptr);
    ~QWoAboutDialog();

private slots:
    void onVersionCheck();
    void onResult(int code, const QByteArray& body);
private:
    Ui::QWoAboutDialog *ui;
};

#endif // QWOABOUTDIALOG_H
