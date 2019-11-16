#include "qwoshellwidget.h"
#include "qwoprocess.h"
#include "qwosetting.h"
#include "qwosshconf.h"
#include "qwosshprocess.h"
#include "qwoutils.h"
#include "qwoglobal.h"
#include "qwoshellwidgetimpl.h"

#include <QApplication>
#include <QDebug>
#include <QMenu>
#include <QClipboard>
#include <QSplitter>


QWoShellWidget::QWoShellWidget(QWidget *parent)
    :QWoWidget (parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
}

QWoShellWidget::~QWoShellWidget()
{
    if(m_menu) {
        delete m_menu;
    }
}

void QWoShellWidget::closeAndDelete()
{
    m_bexit = true;
    deleteLater();
    QSplitter *splitParent = qobject_cast<QSplitter*>(parent());
    if(splitParent == nullptr) {
        return;
    }
    int cnt = splitParent->count();
    if(cnt == 1) {
        splitParent->deleteLater();
    }
}

void QWoShellWidget::onTimeout()
{

}

void QWoShellWidget::onVerticalSplitView()
{
    splitWidget(true);
}

void QWoShellWidget::onHorizontalSplitView()
{
    splitWidget(false);
}

void QWoShellWidget::onCloseThisSession()
{
    closeAndDelete();
}

void QWoShellWidget::contextMenuEvent(QContextMenuEvent *e)
{
    if(m_menu == nullptr) {
        m_menu = new QMenu();
        QAction *vsplit = m_menu->addAction(tr("Split Vertical"));
        QObject::connect(vsplit, SIGNAL(triggered()), this, SLOT(onVerticalSplitView()));
        QAction *hsplit = m_menu->addAction(tr("Split Horizontal"));
        QObject::connect(hsplit, SIGNAL(triggered()), this, SLOT(onHorizontalSplitView()));
        QAction *close = m_menu->addAction(tr("Close Session"));
        QObject::connect(close, SIGNAL(triggered()), this, SLOT(onCloseThisSession()));
    }
    m_menu->exec(QCursor::pos());
}

void QWoShellWidget::splitWidget(bool vertical)
{
    QSplitter *splitParent = qobject_cast<QSplitter*>(parent());
    if(splitParent == nullptr) {
        return;
    }
    int cnt = splitParent->count();
    QSplitter *splitter = splitParent;
    if(cnt > 1) {
        QList<int> ls = splitParent->sizes();
        int idx = splitParent->indexOf(this);
        QSplitter *splitNew = new QSplitter(splitParent);
        splitParent->insertWidget(idx, splitNew);
        setParent(nullptr);
        splitNew->addWidget(this);
        splitParent->setSizes(ls);
        splitter = splitNew;
        splitter->setHandleWidth(1);
        splitter->setOpaqueResize(false);
        splitter->setAutoFillBackground(true);
        QPalette pal(Qt::gray);
        splitter->setPalette(pal);
    }

    splitter->setOrientation(vertical ? Qt::Vertical : Qt::Horizontal);
    QWoShellWidget *term = new QWoShellWidget(splitter);
    splitter->addWidget(term);

    QList<int> ls;
    ls << 1 << 1;
    splitter->setSizes(ls);
}
