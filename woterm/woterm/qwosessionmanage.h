#ifndef QWOSESSIONMANAGE_H
#define QWOSESSIONMANAGE_H

#include "qwoglobal.h"
#include "qwohostlistmodel.h"

#include <QDialog>
#include <QPointer>
#include <QStandardItemModel>

namespace Ui {
class QWoSessionManage;
}

class QTermWidget;
class QStringListModel;
class QSortFilterProxyModel;


class QWoSessionManage : public QDialog
{
    Q_OBJECT

public:
    explicit QWoSessionManage(QWidget *parent = nullptr);
    ~QWoSessionManage();

signals:
    void connect(const QString& host);

private slots:
    void onEditTextChanged(const QString &txt);
    void onConnectReady();
    void onDeleteReady();
    void onModifyReady();
    void onNewReady();
    void onTreeItemDoubleClicked(const QModelIndex& idx);
private:
    void refreshList();
private:
    Ui::QWoSessionManage *ui;
    QPointer<QWoHostListModel> m_model;
    QPointer<QSortFilterProxyModel> m_proxyModel;
};

#endif // QWOSESSIONPROPERTY_H
