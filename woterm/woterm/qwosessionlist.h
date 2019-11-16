#pragma once

#include <QWidget>
#include <QPointer>
#include "qwoglobal.h"

class QLineEdit;
class QListWidget;
class QPushButton;
class QWoHostListModel;
class QSortFilterProxyModel;
class QListView;
class QMenu;
class QPlainTextEdit;


class QWoSessionList : public QWidget
{
    Q_OBJECT
public:
    explicit QWoSessionList(QWidget *parent=nullptr);
    virtual ~QWoSessionList();
signals:
    void aboutToClose(QCloseEvent* event);
    void readyToConnect(const QString& target);
    void batchReadyToConnect(const QStringList& targets);

private:
    void init();
    void refreshList();
private slots:
    void onReloadSessionList();
    void onOpenSelectSessions();
    void onEditTextChanged(const QString& txt);
    void onListItemDoubleClicked(const QModelIndex& item);
    void onListItemPressed(const QModelIndex& item);
    void onTimeout();
    void onEditReturnPressed();
    void onListViewItemOpen();
    void onListViewItemReload();
    void onListViewItemModify();
    void onListViewItemAdd();
    void onListViewItemDelete();
private:
    bool handleListViewContextMenu(QContextMenuEvent* ev);
private:
    void closeEvent(QCloseEvent *event);
    bool eventFilter(QObject *obj, QEvent *ev);
private:
    QPointer<QWoHostListModel> m_model;
    QPointer<QSortFilterProxyModel> m_proxyModel;
    QPointer<QLineEdit> m_input;
    QPointer<QListView> m_list;
    QPointer<QPlainTextEdit> m_info;
    int m_countLeft;
    QPointer<QMenu> m_menu;
    QPointer<QAction> m_itemOpen;
};
