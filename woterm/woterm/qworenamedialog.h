#ifndef QWORENAMEDIALOG_H
#define QWORENAMEDIALOG_H

#include <QDialog>

namespace Ui {
class QWoRenameDialog;
}

class QWoRenameDialog : public QDialog
{
    Q_OBJECT

public:
    static QString open(const QString& name, QWidget *parent= nullptr);
private slots:
    void onButtonSaveClicked();
protected:
    explicit QWoRenameDialog(const QString& name, QWidget *parent = nullptr);
    ~QWoRenameDialog();
    QString result() const;
private:
    Ui::QWoRenameDialog *ui;
    QString m_result;
};

#endif // QWORENAMEDIALOG_H
