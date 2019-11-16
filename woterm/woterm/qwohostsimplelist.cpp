#include "qwohostsimplelist.h"
#include "ui_qwohostlist.h"
#include "qwohostlistmodel.h"

#include <QSortFilterProxyModel>
#include <QDebug>

QWoHostSimpleList::QWoHostSimpleList(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QWoHostList)
{
    Qt::WindowFlags flags = windowFlags();
    setWindowFlags(flags &~Qt::WindowContextHelpButtonHint);
    ui->setupUi(this);

    setWindowTitle(tr("jump list"));

    m_model = new QWoHostListModel(this);
    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    ui->hostList->setModel(m_proxyModel);

    QObject::connect(ui->rxfind, SIGNAL(textChanged(const QString&)), this, SLOT(onEditTextChanged(const QString&)));
    QObject::connect(ui->hostList, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(onListItemDoubleClicked(const QModelIndex&)));
    QObject::connect(ui->ok, SIGNAL(clicked()), this, SLOT(onOpenSelectSessions()));
    m_model->refreshList();
}

QWoHostSimpleList::~QWoHostSimpleList()
{
    if(m_model){
        delete m_model;
    }
    if(m_proxyModel) {
        delete m_proxyModel;
    }
    delete ui;
}

bool QWoHostSimpleList::result(HostInfo *phi)
{
    if(m_result.host.isEmpty()) {
        return false;
    }
    *phi = m_result;
    return true;
}

void QWoHostSimpleList::onEditTextChanged(const QString &txt)
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
}

void QWoHostSimpleList::onListItemDoubleClicked(const QModelIndex &item)
{
    QString name = item.data().toString();
    if(name == "") {
        return;
    }
    m_result = item.data(ROLE_HOSTINFO).value<HostInfo>();
    close();
}

void QWoHostSimpleList::onOpenSelectSessions()
{
    QModelIndex item = ui->hostList->currentIndex();
    m_result = item.data(ROLE_HOSTINFO).value<HostInfo>();
    close();
}
