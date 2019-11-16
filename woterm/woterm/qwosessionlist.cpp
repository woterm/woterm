#include "qwosessionlist.h"
#include "qwosetting.h"
#include "qwosshconf.h"
#include "qwohostinfoedit.h"
#include "qwohostlistmodel.h"
#include "qwosessionproperty.h"

#include <QCloseEvent>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QListView>
#include <QApplication>
#include <QFile>
#include <QRegExp>
#include <QDebug>
#include <QtAlgorithms>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStringListModel>
#include <QTimer>
#include <QModelIndexList>
#include <QMenu>
#include <QAction>
#include <QPlainTextEdit>

QWoSessionList::QWoSessionList(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    setLayout(layout);
    layout->setSpacing(0);
    layout->setMargin(0);

    QHBoxLayout *hlayout = new QHBoxLayout(this);
    layout->addLayout(hlayout);
    m_list = new QListView(this);
    m_input = new QLineEdit(this);
    m_info = new QPlainTextEdit(this);
    QPushButton *all = new QPushButton("all", this);
    all->setMaximumWidth(25);
    all->hide();
    hlayout->addWidget(m_input);
    hlayout->addWidget(all);
    layout->addWidget(m_list);
    layout->addWidget(m_info);
    m_info->setReadOnly(true);
    m_info->setFixedHeight(150);
    m_list->installEventFilter(this);
    //m_list->setSelectionMode(QAbstractItemView::MultiSelection);

    m_model = QWoHostListModel::instance();
    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    m_list->setModel(m_proxyModel);
    refreshList();

    m_list->setEditTriggers(QAbstractItemView::NoEditTriggers);
    //m_list->setSelectionMode(QAbstractItemView::MultiSelection);

    QObject::connect(m_input, SIGNAL(returnPressed()), this, SLOT(onEditReturnPressed()));
    QObject::connect(all, SIGNAL(clicked()), this, SLOT(onOpenSelectSessions()));
    QObject::connect(m_input, SIGNAL(textChanged(const QString&)), this, SLOT(onEditTextChanged(const QString&)));
    QObject::connect(m_list, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(onListItemDoubleClicked(const QModelIndex&)));
    QObject::connect(m_list, SIGNAL(pressed(const QModelIndex&)), this, SLOT(onListItemPressed(const QModelIndex&)));
    QTimer *timer = new QTimer(this);
    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
    timer->start(1000);
    m_countLeft = 0;
}

QWoSessionList::~QWoSessionList()
{
    if(m_menu) {
        delete m_menu;
    }
}

void QWoSessionList::init()
{

}


void QWoSessionList::refreshList()
{
    QWoSshConf::instance()->save();
    QWoSshConf::instance()->refresh();
    m_model->refreshList();
}

void QWoSessionList::onReloadSessionList()
{
    refreshList();
}

void QWoSessionList::onOpenSelectSessions()
{
    int cnt = m_proxyModel->rowCount();
    qDebug() << "rowCount:" << cnt;
    QStringList sessions;
    for(int i = 0; i < cnt; i++) {
        QModelIndex mi = m_proxyModel->index(i, 0);
        sessions << mi.data().toString();
    }
    emit batchReadyToConnect(sessions);
}

void QWoSessionList::onEditTextChanged(const QString &txt)
{
    m_countLeft = 30;
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

void QWoSessionList::onListItemDoubleClicked(const QModelIndex &item)
{
    HostInfo hi = item.data(ROLE_HOSTINFO).value<HostInfo>();
    if(hi.name == "") {
        return;
    }
    qDebug() << "server:" << hi.name;

    onListItemPressed(item);
    emit readyToConnect(hi.name);
}

void QWoSessionList::onListItemPressed(const QModelIndex &item)
{
    HostInfo hi = item.data(ROLE_HOSTINFO).value<HostInfo>();
    if(hi.name == "") {
        return;
    }
    QString info;
    info.append(QString("%1 : %2\r\n").arg(tr("name")).arg(hi.name));
    info.append(QString("%1 : %2\r\n").arg(tr("host")).arg(hi.host));
    info.append(QString("%1 : %2").arg(tr("memo")).arg(hi.memo));

    m_info->setPlainText(info);
}

void QWoSessionList::onTimeout()
{
    if(m_countLeft < 0) {
        return;
    }
    if(m_countLeft > 0) {
        m_countLeft--;
        return;
    }
    m_input->setText("");
}

void QWoSessionList::onEditReturnPressed()
{
    QString txt = m_input->text().trimmed();
    if(txt.isEmpty()) {
        return;
    }
    int cnt = m_proxyModel->rowCount();
    if(cnt <= 0) {
        return;
    }
    QModelIndex idx = m_list->currentIndex();
    if(idx.isValid()) {
        idx = m_proxyModel->index(0, 0);
    }
    const HostInfo& hi = idx.data(ROLE_HOSTINFO).value<HostInfo>();
    emit readyToConnect(hi.name);
}

void QWoSessionList::onListViewItemOpen()
{
    QModelIndex idx = m_list->currentIndex();
    if(idx.isValid()) {
        idx = m_proxyModel->index(0, 0);
    }
    const HostInfo& hi = idx.data(ROLE_HOSTINFO).value<HostInfo>();
    emit readyToConnect(hi.name);
}

void QWoSessionList::onListViewItemReload()
{
    refreshList();
}

void QWoSessionList::onListViewItemModify()
{
    QVariant target = m_menu->property("itemIndex");
    if(!target.isValid()) {
        return;
    }
    int idx = target.toInt();
    QWoSessionProperty dlg(QWoSessionProperty::ModifySession, idx, this);
    QObject::connect(&dlg, SIGNAL(connect(const QString&)), this, SIGNAL(readyToConnect(const QString&)));
    dlg.exec();
    refreshList();
}

void QWoSessionList::onListViewItemAdd()
{
    QWoSessionProperty dlg(QWoSessionProperty::NewSession, 0, this);
    QObject::connect(&dlg, SIGNAL(connect(const QString&)), this, SIGNAL(readyToConnect(const QString&)));
    dlg.exec();
    refreshList();
}

void QWoSessionList::onListViewItemDelete()
{
    QVariant target = m_menu->property("itemIndex");
    if(target.isValid()) {
        QWoSshConf::instance()->removeAt(target.toInt());
        refreshList();
    }
}

bool QWoSessionList::handleListViewContextMenu(QContextMenuEvent *ev)
{
    QModelIndex mi = m_list->indexAt(ev->pos());
    if(m_menu == nullptr) {
        m_menu = new QMenu();
        m_itemOpen = m_menu->addAction(QIcon(":/qwoterm/resource/skin/connect.png"), tr("Connect"), this, SLOT(onListViewItemOpen()));
        m_menu->addAction(QIcon(":/qwoterm/resource/skin/reload.png"), tr("ReloadAll"), this, SLOT(onListViewItemReload()));
        m_menu->addAction(QIcon(":/qwoterm/resource/skin/linkcfg.png"), tr("Edit"), this, SLOT(onListViewItemModify()));
        m_menu->addAction(QIcon(":/qwoterm/resource/skin/add.png"), tr("Add"), this, SLOT(onListViewItemAdd()));
        m_menu->addAction(tr("Delete"), this, SLOT(onListViewItemDelete()));
    }    
    QVariant target = mi.data();
    m_itemOpen->setVisible(mi.isValid());
    if(!mi.isValid()) {
        m_list->clearSelection();
    }
    QVariant idx = mi.data(ROLE_INDEX);
    m_menu->setProperty("itemIndex", idx);
    m_menu->exec(ev->globalPos());
    return true;
}


void QWoSessionList::closeEvent(QCloseEvent *event)
{
    emit aboutToClose(event);
    if(event->isAccepted()) {
        return;
    }
    QWidget::closeEvent(event);
}

bool QWoSessionList::eventFilter(QObject *obj, QEvent *ev)
{
    QEvent::Type t = ev->type();
    if(obj == m_list) {
        if(t == QEvent::ContextMenu) {
            return handleListViewContextMenu((QContextMenuEvent *)ev);
        }
    }
    return false;
}
