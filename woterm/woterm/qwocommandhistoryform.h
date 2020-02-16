#ifndef QWOCOMMANDHISTORYFORM_H
#define QWOCOMMANDHISTORYFORM_H

#include <QWidget>
#include <QPointer>

#include "qwoglobal.h"

namespace Ui {
class QWoCommandHistoryForm;
}

class QWoTreeView;
class QAbstractItemModel;

class QWoCommandHistoryForm : public QWidget
{
    Q_OBJECT

public:
    explicit QWoCommandHistoryForm(QWidget *parent = nullptr);
    ~QWoCommandHistoryForm();

    QAbstractItemModel *model();
    void resetModel(QAbstractItemModel *model);
signals:
    void replay(const HistoryCommand& hc);

private slots:
    void onTreeItemDoubleClicked(const QModelIndex& idx);
    void onEditTextChanged(const QString& txt);
    void onTreeItemChanged(const QModelIndex &idx);
    void onDeleteReady();
    void onTreeViewReplayCommand();
protected:
    bool eventFilter(QObject *obj, QEvent *ev);
    bool handleTreeViewContextMenu(QContextMenuEvent *ev);
private:
    Ui::QWoCommandHistoryForm *ui;
    QPointer<QWoTreeView> m_tree;
};

#endif // QWOCOMMANDHISTORYFORM_H
