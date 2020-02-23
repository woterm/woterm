#ifndef QWOIDENTIFYDIALOG_H
#define QWOIDENTIFYDIALOG_H

#include <QDialog>
#include <QPointer>

#include "qwoglobal.h"

namespace Ui {
class QWoIdentifyDialog;
}

class QEventLoop;
class QTreeWidgetItem;

class QWoIdentifyDialog : public QDialog
{
    Q_OBJECT
private:

public:
    ~QWoIdentifyDialog();
    static QString open(QWidget *parent = nullptr);

protected:
    explicit QWoIdentifyDialog(QWidget *parent = nullptr);
    QString result() const;
private slots:
    void onButtonCreateClicked();
    void onButtonImportClicked();
    void onButtonExportClicked();
    void onButtonDeleteClicked();
    void onButtonSelectClicked();
    void onButtonRenameClicked();
    void onButtonViewClicked();
    void onItemDoubleClicked(QTreeWidgetItem*, int);

private:
    void reload();
private:
    Ui::QWoIdentifyDialog *ui;
    QString m_result;
};

#endif // QWOIDENTIFYDIALOG_H
