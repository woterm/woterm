#include "qwotermwidgetimpl.h"
#include "qwosshprocess.h"
#include "qwotermwidget.h"
#include "qwosessionproperty.h"
#include "qwoglobal.h"
#include "qwoshower.h"
#include "qwosshconf.h"
#include "qwomainwindow.h"
#include "qwoevent.h"
#include "qwoscriptwidgetimpl.h"
#include "qwocommandlineinput.h"

#include <QCloseEvent>
#include <QSplitter>
#include <QApplication>
#include <QMessageBox>
#include <QMenu>
#include <QLineEdit>
#include <QTabBar>
#include <QVBoxLayout>


QWoTermWidgetImpl::QWoTermWidgetImpl(QString target, QTabBar *tab, QWidget *parent)
    : QWoShowerWidget (parent)
    , m_tab(tab)
    , m_target(target)
    , m_termType(ESsh)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    setLayout(layout);
    layout->setMargin(0);
    layout->setSpacing(0);
    m_command = new QWoCommandLineInput(this);
    m_root = new QSplitter(this);
    layout->addWidget(m_root);
    layout->addWidget(m_command);
    m_command->setFixedHeight(24);
    m_command->hide();

    QObject::connect(m_command, SIGNAL(returnPressed(const QString&)), this, SLOT(onCommandInputReturnPressed(const QString&)));

    m_root->setHandleWidth(1);
    m_root->setOpaqueResize(false);
    m_root->setAutoFillBackground(true);
    QPalette pal(Qt::gray);
    m_root->setPalette(pal);
    QWoSshProcess *process = new QWoSshProcess(target, m_root);
    QWoTermWidget *term = new QWoTermWidget(process, m_root);
    m_root->addWidget(term);
    process->start();
    QObject::connect(m_root, SIGNAL(destroyed(QObject*)), this, SLOT(onRootSplitterDestroy()));
}

QWoTermWidgetImpl::~QWoTermWidgetImpl()
{

}

QWoTermWidgetImpl::ETermType QWoTermWidgetImpl::termType()
{
    return m_termType;
}

void QWoTermWidgetImpl::joinToVertical(const QString &target)
{
    m_terms.last()->splitWidget(target, true);
}

void QWoTermWidgetImpl::joinToHorizontal(int row, const QString &target)
{
    if(m_terms.length() < row) {
        return;
    }
    m_terms.at(row)->splitWidget(target, false);
}

void QWoTermWidgetImpl::closeEvent(QCloseEvent *event)
{
    emit aboutToClose(event);
    if(event->isAccepted()) {
        return;
    }
    QWidget::closeEvent(event);
}

bool QWoTermWidgetImpl::handleTabMouseEvent(QMouseEvent *ev)
{
    return false;
}

void QWoTermWidgetImpl::handleTabContextMenu(QMenu *menu)
{
    if(m_terms.length() > 1) {
        menu->addAction(tr("Command line input..."), this, SLOT(onCommandInputInSamePage()));
    }
}

void QWoTermWidgetImpl::onRootSplitterDestroy()
{
    close();
    deleteLater();
}

void QWoTermWidgetImpl::onCommandInputInSamePage()
{
    m_command->setVisible(!m_command->isVisible());
}

void QWoTermWidgetImpl::onCommandInputReturnPressed(const QString &cmd)
{
    for(int i = 0; i < m_terms.length(); i++) {
        QWoTermWidget *term = m_terms.at(i);
        term->sendData(cmd.toUtf8());
    }
}

void QWoTermWidgetImpl::broadcastMessage(int type, QVariant msg)
{
    for(int i = 0; i < m_terms.count(); i++) {
        QWoTermWidget *term = m_terms.at(i);
        if(term) {
            term->onBroadcastMessage(type, msg);
        }
    }
}

void QWoTermWidgetImpl::addToList(QWoTermWidget *w)
{
    m_terms.append(w);
    resetTabText();
    if(m_terms.length() == 1) {
        m_command->hide();
    }
}

void QWoTermWidgetImpl::removeFromList(QWoTermWidget *w)
{
    m_terms.removeOne(w);
    resetTabText();
    if(m_terms.length() == 1) {
        m_command->hide();
    }
}

void QWoTermWidgetImpl::resetTabText()
{
    if(m_terms.isEmpty()) {
        return;
    }
    QWoProcess *proc = m_terms.first()->process();
    QString title = proc->sessionName();
    if(m_terms.length() > 1) {
        title.append("*");
    }
    setTabText(title);
}

void QWoTermWidgetImpl::setTabText(const QString &title)
{
    for(int i = 0; i < m_tab->count(); i++){
        QVariant data = m_tab->tabData(i);
        QWoTermWidgetImpl *impl = data.value<QWoTermWidgetImpl*>();
        if(impl == this){
            m_tab->setTabText(i, title);
        }
    }
}

bool QWoTermWidgetImpl::event(QEvent *e)
{
    QEvent::Type t = e->type();
    if(t == QWoEvent::EventType) {
        for(int i = 0; i < m_terms.count(); i++) {
            QWoTermWidget *term = m_terms.at(i);
            QCoreApplication::sendEvent(term, e);
        }
        return false;
    }
    return QWoShowerWidget::event(e);
}
