#include "qwocommandhistoryform.h"
#include "ui_qwocommandhistoryform.h"
#include "qwocommandhistorymodel.h"

#include "qwotreeview.h"

#include <QSortFilterProxyModel>
#include <QAbstractItemModel>
#include <QMenu>

QWoCommandHistoryForm::QWoCommandHistoryForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QWoCommandHistoryForm)
{
    ui->setupUi(this);
    QVBoxLayout *layout = new QVBoxLayout(ui->frame);
    ui->frame->setLayout(layout);
    layout->setSpacing(0);
    layout->setMargin(0);
    m_tree = new QWoTreeView(ui->frame);
    m_tree->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tree->setSelectionMode(QAbstractItemView::MultiSelection);
    layout->addWidget(m_tree);

    QObject::connect(m_tree, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(onTreeItemDoubleClicked(const QModelIndex&)));
    QObject::connect(ui->filter, SIGNAL(textChanged(const QString&)), this, SLOT(onEditTextChanged(const QString&)));
    QObject::connect(m_tree, SIGNAL(itemChanged(const QModelIndex&)), this, SLOT(onTreeItemChanged(const QModelIndex&)));

    m_tree->installEventFilter(this);
}

QWoCommandHistoryForm::~QWoCommandHistoryForm()
{
    delete ui;
}

QAbstractItemModel *QWoCommandHistoryForm::model()
{
    return m_tree->model();
}

void QWoCommandHistoryForm::resetModel(QAbstractItemModel *model)
{
    m_tree->setModel(model);
}

void QWoCommandHistoryForm::onTreeItemDoubleClicked(const QModelIndex &idx)
{
    if(!idx.isValid()) {
        return;
    }
    const HistoryCommand& hc = idx.data(ROLE_CMDINFO).value<HistoryCommand>();
    if(hc.cmd.isEmpty()) {
        return;
    }
    emit replay(hc);
}

void QWoCommandHistoryForm::onEditTextChanged(const QString &txt)
{
    QStringList sets = txt.split(' ');
    for(QStringList::iterator iter = sets.begin(); iter != sets.end(); ) {
        if(*iter == "") {
            iter = sets.erase(iter);
        }else{
            iter++;
        }
    }

    QRegExp regex(sets.join(".*"), Qt::CaseInsensitive);
    regex.setPatternSyntax(QRegExp::RegExp2);
    QAbstractItemModel *model = m_tree->model();
    QSortFilterProxyModel *proxy = qobject_cast<QSortFilterProxyModel*>(model);
    if(proxy == nullptr) {
        return;
    }
    proxy->setFilterRegExp(regex);
    proxy->setFilterRole(ROLE_REFILTER);
}

void QWoCommandHistoryForm::onTreeItemChanged(const QModelIndex &idx)
{
    if(!idx.isValid()) {
        return;
    }
    const HistoryCommand& hc = idx.data(ROLE_CMDINFO).value<HistoryCommand>();
    if(hc.cmd.isEmpty()) {
        return;
    }
    QString txt = hc.cmd +"\r\n" + hc.path;
    ui->plainTextEdit->setPlainText(txt);
}

void QWoCommandHistoryForm::onDeleteReady()
{
    QItemSelectionModel *selectModel = m_tree->selectionModel();
    QModelIndexList idxs = selectModel->selectedRows();
    QAbstractItemModel *model = m_tree->model();
    if(idxs.length() <= 0) {
        return;
    }
    int row = idxs.at(0).row();
    model->removeRows(row, idxs.length());
}

void QWoCommandHistoryForm::onTreeViewReplayCommand()
{
    QItemSelectionModel *selectModel = m_tree->selectionModel();
    QModelIndexList idxs = selectModel->selectedRows();
    if(idxs.length() <= 0) {
        return;
    }
    const HistoryCommand& hc = idxs.at(0).data(ROLE_CMDINFO).value<HistoryCommand>();
    emit replay(hc);
}

bool QWoCommandHistoryForm::eventFilter(QObject *obj, QEvent *ev)
{
    QEvent::Type t = ev->type();
    if(obj == m_tree) {
        if(t == QEvent::ContextMenu) {
            return handleTreeViewContextMenu(dynamic_cast<QContextMenuEvent*>(ev));
        }
    }
    return QWidget::eventFilter(obj, ev);
}

bool QWoCommandHistoryForm::handleTreeViewContextMenu(QContextMenuEvent *ev)
{
    QItemSelectionModel *model = m_tree->selectionModel();
    QModelIndexList idxs = model->selectedRows();

    QMenu menu(this);

    menu.addAction(tr("Delete"), this, SLOT(onDeleteReady()));
    if(idxs.length() == 1) {
        menu.addAction(tr("Replay"), this, SLOT(onTreeViewReplayCommand()));
    }

    menu.exec(ev->globalPos());
    return true;
}
