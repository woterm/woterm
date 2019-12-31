#include "qwosessionmanage.h"
#include "ui_qwosessionmanage.h"
#include "qwohostsimplelist.h"
#include "qtermwidget.h"

#include "qwosetting.h"
#include "qwoutils.h"
#include "qwosshconf.h"
#include "qwosessionproperty.h"
#include "qwotreeview.h"

#include <QFileDialog>
#include <QMenu>
#include <QDebug>
#include <QIntValidator>
#include <QStringListModel>
#include <QMessageBox>
#include <QSortFilterProxyModel>

QWoSessionManage::QWoSessionManage(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QWoSessionManage)
{
    Qt::WindowFlags flags = windowFlags();
    setWindowFlags(flags &~Qt::WindowContextHelpButtonHint);
    ui->setupUi(this);
    setWindowTitle(tr("Session List"));

    m_model = QWoHostListModel::instance();
    m_model->setMaxColumnCount(3);
    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);

    QVBoxLayout *layout = new QVBoxLayout(ui->frame);
    ui->frame->setLayout(layout);
    layout->setSpacing(0);
    layout->setMargin(0);
    m_tree = new QWoTreeView(ui->frame);
    m_tree->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tree->setSelectionMode(QAbstractItemView::MultiSelection);
    layout->addWidget(m_tree);
    m_tree->installEventFilter(this);

    m_tree->setModel(m_proxyModel);
    int w = m_model->widthColumn(m_tree->font(), 0);
    if(w < 100) {
        w = 100;
    }
    m_tree->setColumnWidth(0, w+30);
    m_tree->setColumnWidth(1, 200);

    QObject::connect(ui->lineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(onEditTextChanged(const QString&)));
    QObject::connect(ui->btnConnect, SIGNAL(clicked()), this, SLOT(onConnectReady()));
    QObject::connect(ui->btnDelete, SIGNAL(clicked()), this, SLOT(onDeleteReady()));
    QObject::connect(ui->btnModify, SIGNAL(clicked()), this, SLOT(onModifyReady()));
    QObject::connect(ui->btnNew, SIGNAL(clicked()), this, SLOT(onNewReady()));
    QObject::connect(m_tree, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(onTreeItemDoubleClicked(const QModelIndex&)));
}

QWoSessionManage::~QWoSessionManage()
{
    delete ui;
}

void QWoSessionManage::onEditTextChanged(const QString &txt)
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
    m_proxyModel->setFilterRegExp(regex);
    m_proxyModel->setFilterRole(ROLE_REFILTER);
}

void QWoSessionManage::onConnectReady()
{
    onTreeViewOpenInDifferentPage();
}

void QWoSessionManage::onDeleteReady()
{
    QItemSelectionModel *model = m_tree->selectionModel();
    QModelIndexList idxs = model->selectedRows();
    if(idxs.isEmpty()) {
        QMessageBox::warning(this, "delete", "no selection?", QMessageBox::Ok|QMessageBox::No);
        return;
    }
    QMessageBox::StandardButton btn = QMessageBox::warning(this, "delete", "delete all the selective items?", QMessageBox::Ok|QMessageBox::No);
    if(btn == QMessageBox::No) {
        return ;
    }
    for(int i = 0; i < idxs.length(); i++) {
        QModelIndex idx = idxs.at(i);
        QString name = idx.data().toString();
        QWoSshConf::instance()->remove(name);
    }
    refreshList();
}

void QWoSessionManage::onModifyReady()
{
    QModelIndex idx = m_tree->currentIndex();
    if(!idx.isValid()) {
        QMessageBox::information(this, tr("Modify"), tr("No Selection"));
        return;
    }
    QVariant host = idx.data(ROLE_HOSTINFO);
    if(!host.isValid()) {
        return;
    }
    HostInfo hi = host.value<HostInfo>();
    QWoSessionProperty dlg(hi.name, this);
    dlg.setButtonFlags(QWoSessionProperty::ButtonSave);
    dlg.exec();
    refreshList();
}

void QWoSessionManage::onNewReady()
{
    QWoSessionProperty dlg("", this);
    dlg.setButtonFlags(QWoSessionProperty::ButtonSave);
    dlg.exec();
    refreshList();
}

void QWoSessionManage::onTreeViewOpenInSamePage()
{
    QItemSelectionModel *model = m_tree->selectionModel();
    QModelIndexList idxs = model->selectedRows();
    if(idxs.length() > 6) {
        QMessageBox::information(this, tr("Info"), tr("can't open over 6 session in same page."));
        return;
    }
    QStringList targets;
    for(int i = 0; i < idxs.length(); i++) {
        QModelIndex idx = idxs.at(i);
        QString name = idx.data().toString();
        targets.append(name);
    }
    if(targets.isEmpty()){
        QMessageBox::warning(this, tr("Info"), tr("no selection"));
        return;
    }
    emit connect(targets, true);
    close();
}

void QWoSessionManage::onTreeViewOpenInDifferentPage()
{
    QItemSelectionModel *model = m_tree->selectionModel();
    QModelIndexList idxs = model->selectedRows();
    QStringList targets;
    for(int i = 0; i < idxs.length(); i++) {
        QModelIndex idx = idxs.at(i);
        QString name = idx.data().toString();
        targets.append(name);
    }
    if(targets.isEmpty()){
        QMessageBox::warning(this, tr("Info"), tr("no selection"));
        return;
    }
    emit connect(targets, false);
    close();
}

void QWoSessionManage::onTreeItemDoubleClicked(const QModelIndex &idx)
{
    if(!idx.isValid()) {
        return;
    }
    QString target = idx.data(ROLE_FRIENDLY_NAME).toString();
    if(target.isEmpty()) {
        return;
    }
    emit connect(target);
    close();
}

void QWoSessionManage::refreshList()
{
    QWoSshConf::instance()->save();
    QWoSshConf::instance()->refresh();
    m_model->refreshList();
}

bool QWoSessionManage::handleTreeViewContextMenu(QContextMenuEvent *ev)
{
    QItemSelectionModel *model = m_tree->selectionModel();
    QModelIndexList idxs = model->selectedRows();

    QMenu menu(this);

    if(idxs.isEmpty()) {
        menu.addAction(QIcon(":/qwoterm/resource/skin/add.png"), tr("Add"), this, SLOT(onNewReady()));
    } else if(idxs.length() == 1) {
        menu.addAction(QIcon(":/qwoterm/resource/skin/add.png"), tr("Add"), this, SLOT(onNewReady()));
        menu.addAction(QIcon(":/qwoterm/resource/skin/connect.png"), tr("Connect"), this, SLOT(onConnectReady()));
        menu.addAction(QIcon(":/qwoterm/resource/skin/palette.png"), tr("Edit"), this, SLOT(onModifyReady()));
        menu.addAction(tr("Delete"), this, SLOT(onDeleteReady()));
    }else{
        menu.addAction(tr("Delete"), this, SLOT(onDeleteReady()));
        menu.addAction(tr("Open in same page"), this, SLOT(onTreeViewOpenInSamePage()));
        menu.addAction(tr("Open in different page"), this, SLOT(onTreeViewOpenInDifferentPage()));
    }
    menu.exec(ev->globalPos());
    return true;
}

bool QWoSessionManage::eventFilter(QObject *obj, QEvent *ev)
{
    QEvent::Type t = ev->type();
    if(obj == m_tree) {
        if(t == QEvent::ContextMenu) {
            return handleTreeViewContextMenu(dynamic_cast<QContextMenuEvent*>(ev));
        }
    }
    return QDialog::eventFilter(obj, ev);
}
