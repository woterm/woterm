#ifndef QWOHOSTSIMPLELIST_H
#define QWOHOSTSIMPLELIST_H

#include "qwoglobal.h"

#include <QDialog>
#include <QPointer>

namespace Ui {
class QWoHostList;
}

class QWoHostListModel;
class QSortFilterProxyModel;

class QWoHostSimpleList : public QDialog
{
    Q_OBJECT

public:
    explicit QWoHostSimpleList(QWidget *parent = nullptr);
    ~QWoHostSimpleList();

    bool result(HostInfo *phi);
private slots:
    void onEditTextChanged(const QString& txt);
    void onListItemDoubleClicked(const QModelIndex& item);
    void onOpenSelectSessions();
private:
    void init();
private:
    Ui::QWoHostList *ui;
    QPointer<QWoHostListModel> m_model;
    QPointer<QSortFilterProxyModel> m_proxyModel;
    HostInfo m_result;
};

#endif // QWOHOSTSIMPLELIST_H
