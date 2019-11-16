#include "qwoshower.h"
#include "qwotermwidgetimpl.h"
#include "qwoshellwidgetimpl.h"
#include "qwosessionproperty.h"
#include "qwomainwindow.h"
#include "qwosshconf.h"
#include "qwoevent.h"

#include <QTabBar>
#include <QResizeEvent>
#include <QMessageBox>
#include <QtGlobal>
#include <QSplitter>
#include <QDebug>
#include <QPainter>
#include <QApplication>
#include <QIcon>
#include <QMenu>
#include <QProcess>


QWoShower::QWoShower(QTabBar *tab, QWidget *parent)
    : QStackedWidget (parent)
    , m_tab(tab)
{
    QObject::connect(tab, SIGNAL(tabCloseRequested(int)), this, SLOT(onTabCloseRequested(int)));
    QObject::connect(tab, SIGNAL(currentChanged(int)), this, SLOT(onTabCurrentChanged(int)));
    QObject::connect(tab, SIGNAL(tabBarDoubleClicked(int)), this, SLOT(onTabbarDoubleClicked(int)));
    tab->installEventFilter(this);
}

QWoShower::~QWoShower()
{

}

bool QWoShower::openLocalShell()
{
//    QWoShowerWidget *impl = new QWoShellWidgetImpl(this);
//    impl->setProperty(TAB_TYPE_NAME, ETShell);
//    createTab(impl, "local");
    return true;
}

bool QWoShower::openConnection(const QString &target)
{
    QWoShowerWidget *impl = new QWoTermWidgetImpl(target, m_tab,  this);
    impl->setProperty(TAB_TYPE_NAME, ETSsh);
    impl->setProperty(TAB_TARGET_NAME, target);
    createTab(impl, target);
    return true;
}

void QWoShower::setBackgroundColor(const QColor &clr)
{
    QPalette pal;
    pal.setColor(QPalette::Background, clr);
    pal.setColor(QPalette::Window, clr);
    setPalette(pal);
}

void QWoShower::openFindDialog()
{
    int idx = m_tab->currentIndex();
    if (idx < 0 || idx > m_tab->count()) {
        return;
    }
    QVariant v = m_tab->tabData(idx);
    QWoShowerWidget *target = v.value<QWoShowerWidget*>();
//    QSplitter *take = m_terms.at(idx);
//    Q_ASSERT(target == take);
    //    take->toggleShowSearchBar();
}

int QWoShower::tabCount()
{
    return m_tab->count();
}

void QWoShower::resizeEvent(QResizeEvent *event)
{
    QSize newSize = event->size();
    QRect rt(0, 0, newSize.width(), newSize.height());
}

void QWoShower::syncGeometry(QWidget *widget)
{
    QRect rt = geometry();
    rt.moveTo(0, 0);
    widget->setGeometry(rt);
}

void QWoShower::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    QRect rt(0, 0, width(), height());
    p.fillRect(rt, QColor(Qt::black));
    QFont ft = p.font();
    ft.setPixelSize(190);
    ft.setBold(true);
    p.setFont(ft);
    QPen pen = p.pen();
    pen.setStyle(Qt::DotLine);
    pen.setColor(Qt::lightGray);
    QBrush brush = pen.brush();
    brush.setStyle(Qt::Dense7Pattern);
    pen.setBrush(brush);
    p.setPen(pen);
    p.drawText(rt, Qt::AlignCenter, "WoTerm");
}

bool QWoShower::eventFilter(QObject *obj, QEvent *ev)
{
    switch (ev->type()) {
    case QEvent::MouseButtonPress:
        return tabMouseButtonPress((QMouseEvent*)ev);
    }
    return false;
}

void QWoShower::closeSession(int idx)
{
    if(idx >= m_tab->count()) {
        return;
    }
    QVariant v = m_tab->tabData(idx);
    QWoShowerWidget *target = v.value<QWoShowerWidget*>();
    target->deleteLater();
}

void QWoShower::createTab(QWoShowerWidget *impl, const QString& tabName)
{
    addWidget(impl);
    int idx = m_tab->addTab(tabName);
    m_tab->setCurrentIndex(idx);
    m_tab->setTabData(idx, QVariant::fromValue(impl));
    QObject::connect(impl, SIGNAL(destroyed(QObject*)), this, SLOT(onTermImplDestroy(QObject*)));
    setCurrentWidget(impl);
    qDebug() << "tabCount" << m_tab->count() << ",implCount" << count();
}

bool QWoShower::tabMouseButtonPress(QMouseEvent *ev)
{
    QPoint pt = ev->pos();
    int idx = m_tab->tabAt(pt);
    if(idx < 0) {
        emit openSessionManage();
        return false;
    }
    qDebug() << "tab hit" << idx;
    QVariant v = m_tab->tabData(idx);
    QWoShowerWidget *impl = v.value<QWoShowerWidget*>();
    if(ev->buttons().testFlag(Qt::RightButton)) {
        QMenu menu(impl);
        m_tabMenu = &menu;
        m_tabMenu->setProperty(TAB_TARGET_IMPL, QVariant::fromValue(impl));
        QAction *modify = menu.addAction(QIcon(":/qwoterm/resource/skin/linkcfg.png"), tr("Edit"));        
        QObject::connect(modify, SIGNAL(triggered()), this, SLOT(onModifyThisSession()));
        QAction *close = menu.addAction(tr("Close This Tab"));
        QObject::connect(close, SIGNAL(triggered()), this, SLOT(onCloseThisTabSession()));
        QAction *other = menu.addAction(tr("Close Other Tab"));
        QObject::connect(other, SIGNAL(triggered()), this, SLOT(onCloseOtherTabSession()));
        QAction *dup = menu.addAction(tr("Duplicate In New Window"));
        QObject::connect(dup, SIGNAL(triggered()), this, SLOT(onDuplicateInNewWindow()));
        menu.exec(QCursor::pos());
    }

    return false;
}

void QWoShower::onTabCloseRequested(int idx)
{
    QMessageBox::StandardButton btn = QMessageBox::warning(this, "CloseSession", "Close Or Not?", QMessageBox::Ok|QMessageBox::No);
    if(btn == QMessageBox::No) {
        return ;
    }
    closeSession(idx);
}

void QWoShower::onTabCurrentChanged(int idx)
{
    if(idx < 0) {
        return;
    }
    QVariant v = m_tab->tabData(idx);
    QWoShowerWidget *impl = v.value<QWoShowerWidget *>();
    setCurrentWidget(impl);
}

void QWoShower::onTermImplDestroy(QObject *it)
{
    QWidget *target = qobject_cast<QWidget*>(it);
    for(int i = 0; i < m_tab->count(); i++) {
        QVariant v = m_tab->tabData(i);
        QWidget *impl = v.value<QWidget *>();
        if(target == impl) {
            removeWidget(target);
            m_tab->removeTab(i);
            break;
        }
    }
    qDebug() << "tabCount" << m_tab->count() << ",implCount" << count();
}

void QWoShower::onTabbarDoubleClicked(int index)
{
    if(index < 0) {
        openLocalShell();
    }
}

void QWoShower::onModifyThisSession()
{
    if(m_tabMenu == nullptr) {
        return;
    }
    QVariant vimpl = m_tabMenu->property(TAB_TARGET_IMPL);
    QWidget *impl = vimpl.value<QWidget*>();
    if(impl == nullptr) {
        QMessageBox::warning(this, tr("alert"), tr("failed to find impl infomation"));
        return;
    }
    QVariant target = impl->property(TAB_TARGET_NAME);
    qDebug() << "target" << target;
    if(!target.isValid()) {
        QMessageBox::warning(this, tr("alert"), tr("failed to find host infomation"));
        return;
    }
    int idx = QWoSshConf::instance()->findHost(target.toString());
    if(idx < 0) {
        QMessageBox::warning(this, tr("alert"), tr("failed to find host infomation"));
        return;
    }
    QWoSessionProperty dlg(QWoSessionProperty::ModifySession, idx, this);
    QObject::connect(&dlg, SIGNAL(connect(const QString&)), QWoMainWindow::instance(), SLOT(onSessionReadyToConnect(const QString&)));
    int ret = dlg.exec();
    if(ret == QWoSessionProperty::Save) {
        HostInfo hi = QWoSshConf::instance()->hostInfo(idx);
        QWoEvent ev(QWoEvent::PropertyChanged);
        QCoreApplication::sendEvent(impl, &ev);
    }
}

void QWoShower::onCloseThisTabSession()
{
    deleteLater();
}

void QWoShower::onCloseOtherTabSession()
{
    if(m_tabMenu == nullptr) {
        return;
    }
    QVariant vimpl = m_tabMenu->property(TAB_TARGET_IMPL);
    QWidget *impl = vimpl.value<QWidget*>();
    if(impl == nullptr) {
        QMessageBox::warning(this, tr("alert"), tr("failed to find impl infomation"));
        return;
    }
    for(int i = 0; i < m_tab->count(); i++) {
        QVariant v = m_tab->tabData(i);
        QWidget *target = v.value<QWidget *>();
        if(target != impl) {
            target->deleteLater();
        }
    }
}

void QWoShower::onDuplicateInNewWindow()
{
    if(m_tabMenu) {
        QVariant vimpl = m_tabMenu->property(TAB_TARGET_IMPL);
        QWidget *impl = vimpl.value<QWidget*>();
        if(impl == nullptr) {
            QMessageBox::warning(this, tr("alert"), tr("failed to find impl infomation"));
            return;
        }
        QString target = impl->property(TAB_TARGET_NAME).toString();
        if(!target.isEmpty()){
            QString path = QApplication::applicationFilePath();
            path.append(" ");
            path.append(target);
            QProcess::startDetached(path);
        }
    }
}
