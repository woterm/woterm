#ifndef QWOIDENTIFYPUBLICKEYDIALOG_H
#define QWOIDENTIFYPUBLICKEYDIALOG_H

#include <QDialog>

namespace Ui {
class QWoIdentifyPublicKeyDialog;
}

class QWoIdentifyPublicKeyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QWoIdentifyPublicKeyDialog(const QString& key, QWidget *parent = nullptr);
    ~QWoIdentifyPublicKeyDialog();

private slots:
    void onCopy();
private:
    Ui::QWoIdentifyPublicKeyDialog *ui;
};

#endif // QWOIDENTIFYPUBLICKEYDIALOG_H
