#include "qwosessionmanage.h"
#include "ui_qwosessionmanage.h"
#include "qwohostsimplelist.h"
#include "qtermwidget.h"

#include "qwosetting.h"
#include "qwoutils.h"
#include "qwosshconf.h"
#include "qwosessionproperty.h"

#include <QFileDialog>
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
    ui->treeView->setModel(m_proxyModel);
    int w = m_model->widthColumn(ui->treeView->font(), 0);
    if(w < 100) {
        w = 100;
    }
    ui->treeView->setColumnWidth(0, w+30);
    ui->treeView->setColumnWidth(1, 200);

    QObject::connect(ui->lineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(onEditTextChanged(const QString&)));
    QObject::connect(ui->btnConnect, SIGNAL(clicked()), this, SLOT(onConnectReady()));
    QObject::connect(ui->btnDelete, SIGNAL(clicked()), this, SLOT(onDeleteReady()));
    QObject::connect(ui->btnModify, SIGNAL(clicked()), this, SLOT(onModifyReady()));
    QObject::connect(ui->btnNew, SIGNAL(clicked()), this, SLOT(onNewReady()));
    QObject::connect(ui->treeView, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(onTreeItemDoubleClicked(const QModelIndex&)));
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
    QModelIndex idx = ui->treeView->currentIndex();
    if(!idx.isValid()) {
        QMessageBox::information(this, tr("Modify"), tr("No Selection"));
        return;
    }
    QString target = idx.data(ROLE_FRIENDLY_NAME).toString();
    if(target.isEmpty()) {
        return;
    }
    emit connect(target);
    close();
}

void QWoSessionManage::onDeleteReady()
{
    QModelIndex idx = ui->treeView->currentIndex();
    if(!idx.isValid()) {
        QMessageBox::information(this, tr("Modify"), tr("No Selection"));
        return;
    }
    QVariant target = idx.data(ROLE_INDEX);
    if(!target.isValid()) {        
        return;
    }
    QMessageBox::StandardButton btn = QMessageBox::warning(this, tr("delete"), tr("Delete Or Not?"), QMessageBox::Ok|QMessageBox::No);
    if(btn == QMessageBox::No) {
        return ;
    }
    QWoSshConf::instance()->removeAt(target.toInt());
    refreshList();
}

void QWoSessionManage::onModifyReady()
{
    QModelIndex idx = ui->treeView->currentIndex();
    if(!idx.isValid()) {
        QMessageBox::information(this, tr("Modify"), tr("No Selection"));
        return;
    }
    QVariant target = idx.data(ROLE_INDEX);
    if(!target.isValid()) {
        return;
    }
    QWoSessionProperty dlg(QWoSessionProperty::ModifySession, target.toInt(), this);
    QObject::connect(&dlg, SIGNAL(connect(const QString&)), this, SIGNAL(readyToConnect(const QString&)));
    dlg.exec();
    refreshList();
}

void QWoSessionManage::onNewReady()
{
    QWoSessionProperty dlg(QWoSessionProperty::NewSession, 0, this);
    QObject::connect(&dlg, SIGNAL(connect(const QString&)), this, SIGNAL(readyToConnect(const QString&)));
    dlg.exec();
    refreshList();
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
