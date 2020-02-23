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
class QWoTreeView;


class QWoSessionManage : public QDialog
{
    Q_OBJECT

public:
    explicit QWoSessionManage(QWidget *parent = nullptr);
    ~QWoSessionManage();

signals:
    void connect(const QString& host);
    void connect(const QStringList& hosts, bool samepage);

private slots:
    void onEditTextChanged(const QString &txt);
    void onConnectReady();
    void onDeleteReady();
    void onModifyReady();
    void onNewReady();
    void onImportReady();
    void onTreeViewOpenInSamePage();
    void onTreeViewOpenInDifferentPage();
    void onTreeItemDoubleClicked(const QModelIndex& idx);
private:
    void refreshList();
    bool handleTreeViewContextMenu(QContextMenuEvent *ev);
protected:
    bool eventFilter(QObject *obj, QEvent *ev);
private:
    Ui::QWoSessionManage *ui;
    QPointer<QWoHostListModel> m_model;
    QPointer<QSortFilterProxyModel> m_proxyModel;
    QPointer<QWoTreeView> m_tree;
};

#endif // QWOSESSIONPROPERTY_H
