#include "qwotermwidget.h"
#include "qwoprocess.h"
#include "qwosetting.h"
#include "qwosshconf.h"
#include "qwosshprocess.h"
#include "qwoutils.h"
#include "qwoglobal.h"
#include "qwotermmask.h"
#include "qwotermwidgetimpl.h"
#include "qwopasswordinput.h"
#include "qwoevent.h"

#include <QApplication>
#include <QDebug>
#include <QMenu>
#include <QClipboard>
#include <QSplitter>


QWoTermWidget::QWoTermWidget(QWoProcess *process, QWidget *parent)
    : QTermWidget (parent)
    , m_process(process)
    , m_bexit(false)
{
    addToTermImpl();

    m_process->setTermWidget(this);
    setAttribute(Qt::WA_DeleteOnClose);

    QFont font = QApplication::font();
    font.setFamily(DEFAULT_FONT_FAMILY);
    font.setPointSize(DEFAULT_FONT_SIZE);
    setTerminalFont(font);
    setScrollBarPosition(QTermWidget::ScrollBarRight);
    addCustomColorSchemeDir(QWoSetting::privateColorSchemaPath());

    setColorScheme(DEFAULT_COLOR_SCHEMA);
    setKeyBindings(DEFAULT_KEYBOARD_BINDING);

    setBlinkingCursor(true);
    setBidiEnabled(true);

    initDefault();
    initCustom();

    setFocusPolicy(Qt::StrongFocus);
    setFocus();

    QObject::connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(onReadyReadStandardOutput()));
    QObject::connect(m_process, SIGNAL(readyReadStandardError()), this, SLOT(onReadyReadStandardError()));
    QObject::connect(m_process, SIGNAL(finished(int)), this, SLOT(onFinished(int)));
    QObject::connect(this, SIGNAL(sendData(const QByteArray&)), this, SLOT(onSendData(const QByteArray&)));

    //QTimer::singleShot(1000, this, SLOT(onTimeout()));
    QTimer *timer = new QTimer(this);
    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
    timer->start(5000);
    parseWarningText("Make Sure You Had Install lrzsz Program for upload or download");
}

QWoTermWidget::~QWoTermWidget()
{
    removeFromTermImpl();
}

QWoProcess *QWoTermWidget::process()
{
    return m_process;
}

void QWoTermWidget::closeAndDelete()
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

void QWoTermWidget::triggerPropertyCheck()
{
    initDefault();
    initCustom();
    QWoSshProcess *sshproc = qobject_cast<QWoSshProcess*>(m_process);
    if(sshproc) {
        sshproc->triggerKeepAliveCheck();
    }
}

void QWoTermWidget::onTimeout()
{
    qDebug() << "onTimeout()";
    //m_process->write("echo -e \"\\033[6n\"\r");
    char tmp1[] = "\033[0c";
    char tmp2[] = "\033[6n";
    char tmp3[] = "\033[?1049h";
    //m_process->write(tmp2);
   // m_process->write("\033[0n");
}

void QWoTermWidget::onReadyReadStandardOutput()
{
    QByteArray out = m_process->readAllStandardOutput();
    parseSequenceText(out);
}

void QWoTermWidget::onReadyReadStandardError()
{
    QByteArray err = m_process->readAllStandardError();
    parseSequenceText(err);
}

void QWoTermWidget::onFinished(int code)
{
    qDebug() << "exitcode" << code;
    if(code != 0) {
        if(m_mask == nullptr) {
            m_mask = new QWoTermMask(this);
            QObject::connect(m_mask, SIGNAL(aboutToClose(QCloseEvent*)), this, SLOT(onForceToCloseThisSession()));
            QObject::connect(m_mask, SIGNAL(reconnect()), this, SLOT(onSessionReconnect()));
        }
        m_mask->setGeometry(0, 0, width(), height());
        m_mask->show();
        return;
    }
    closeAndDelete();
}

void QWoTermWidget::onSendData(const QByteArray &buf)
{
    scrollToEnd();
    m_process->write(buf);
}

void QWoTermWidget::onCopyToClipboard()
{
    QClipboard *clip = QGuiApplication::clipboard();
    QString selTxt = selectedText();
    if(!selTxt.isEmpty()) {
        clip->setText(selTxt);
    }
}

void QWoTermWidget::onPasteFromClipboard()
{
    QClipboard *clip = QGuiApplication::clipboard();
    QString clipTxt = clip->text();
    QByteArray buf = clipTxt.toUtf8();
    m_process->write(buf);
}

void QWoTermWidget::onVerticalSplitView()
{
    int sz = height() / 2;
    splitWidget(sz, true);
}

void QWoTermWidget::onHorizontalSplitView()
{
    int sz = width() / 2;
    splitWidget(sz, false);
}

void QWoTermWidget::onCloseThisSession()
{
    QWoSshProcess *sshproc = qobject_cast<QWoSshProcess*>(m_process);
    if(sshproc == nullptr) {
        return;
    }
    sshproc->close();
    closeAndDelete();
}

void QWoTermWidget::onForceToCloseThisSession()
{
    closeAndDelete();
}

void QWoTermWidget::onSessionReconnect()
{
    m_process->start();
    setFocus();
}

void QWoTermWidget::onPasswordInputResult(const QString &pass, bool isSave)
{
    if(isSave) {
        QMetaObject::invokeMethod(m_process, "updatePassword", Qt::QueuedConnection, Q_ARG(QString, pass));
    }
    m_process->write(pass.toUtf8());
    m_process->write("\r");
}

void QWoTermWidget::showPasswordInput(const QString &prompt, bool echo)
{
    if(m_passInput == nullptr) {
        m_passInput = new QWoPasswordInput(this);
        QObject::connect(m_passInput, SIGNAL(result(const QString&,bool)), this, SLOT(onPasswordInputResult(const QString&,bool)));
    }
    m_passInput->reset(prompt, echo);
    m_passInput->setGeometry(0, 0, width(), height());
    m_passInput->show();
}

void QWoTermWidget::contextMenuEvent(QContextMenuEvent *e)
{
    if(m_menu == nullptr) {
        m_menu = new QMenu(this);
        m_copy = m_menu->addAction(tr("Copy"));
        QObject::connect(m_copy, SIGNAL(triggered()), this, SLOT(onCopyToClipboard()));
        m_paste = m_menu->addAction(tr("Paste"));
        QObject::connect(m_paste, SIGNAL(triggered()), this, SLOT(onPasteFromClipboard()));        
        QAction *vsplit = m_menu->addAction(QIcon(":/qwoterm/resource/skin/vsplit.png"), tr("Split Vertical"));
        QObject::connect(vsplit, SIGNAL(triggered()), this, SLOT(onVerticalSplitView()));
        QAction *hsplit = m_menu->addAction(QIcon(":/qwoterm/resource/skin/hsplit.png"), tr("Split Horizontal"));
        QObject::connect(hsplit, SIGNAL(triggered()), this, SLOT(onHorizontalSplitView()));
        QAction *close = m_menu->addAction(tr("Close Session"));
        QObject::connect(close, SIGNAL(triggered()), this, SLOT(onCloseThisSession()));
    }

    QString selTxt = selectedText();
    m_copy->setDisabled(selTxt.isEmpty());
    QClipboard *clip = QGuiApplication::clipboard();
    QString clipTxt = clip->text();
    m_paste->setDisabled(clipTxt.isEmpty());

    m_process->prepareContextMenu(m_menu);

    m_menu->exec(QCursor::pos());
}

void QWoTermWidget::closeEvent(QCloseEvent *event)
{
    emit aboutToClose(event);
    if(event->isAccepted()) {
        return;
    }
    QTermWidget::closeEvent(event);
}

void QWoTermWidget::resizeEvent(QResizeEvent *event)
{
    QSize sz = event->size();
    if(m_mask) {
        m_mask->setGeometry(0, 0, sz.width(), sz.height());
    }
    if(m_passInput) {
        m_passInput->setGeometry(0, 0, sz.width(), sz.height());
    }
    QTermWidget::resizeEvent(event);
}

bool QWoTermWidget::event(QEvent *ev)
{
    if(ev->type() == QWoEvent::EventType) {
        return handleWoEvent(ev);
    }
    return QTermWidget::event(ev);
}

void QWoTermWidget::initDefault()
{
    QString val = QWoSetting::value("property/default").toString();
    QVariantMap mdata = QWoUtils::qBase64ToVariant(val).toMap();
    resetProperty(mdata);
}

void QWoTermWidget::initCustom()
{
    QWoSshProcess *sshprocess = qobject_cast<QWoSshProcess*>(m_process);
    if(sshprocess == nullptr) {
        return;
    }
    QString target = sshprocess->target();
    HostInfo hi = QWoSshConf::instance()->findHostInfo(target);
    QVariantMap mdata = QWoUtils::qBase64ToVariant(hi.property).toMap();
    resetProperty(mdata);
}

void QWoTermWidget::resetProperty(QVariantMap mdata)
{
    if(mdata.isEmpty()) {
        return;
    }
    QString schema = mdata.value("colorSchema", DEFAULT_COLOR_SCHEMA).toString();
    setColorScheme(schema);

    QString fontName = mdata.value("fontName", DEFAULT_FONT_FAMILY).toString();
    int fontSize = mdata.value("fontSize", DEFAULT_FONT_SIZE).toInt();
    QFont ft(fontName, fontSize);
    setTerminalFont(ft);

    QString cursorType = mdata.value("cursorType", "block").toString();
    if(cursorType.isEmpty() || cursorType == "block") {
        setKeyboardCursorShape(Konsole::Emulation::KeyboardCursorShape::BlockCursor);
    }else if(cursorType == "underline") {
        setKeyboardCursorShape(Konsole::Emulation::KeyboardCursorShape::UnderlineCursor);
    }else {
        setKeyboardCursorShape(Konsole::Emulation::KeyboardCursorShape::IBeamCursor);
    }
    int lines = mdata.value("historyLength", DEFAULT_HISTORY_LINE_LENGTH).toInt();
    setHistorySize(lines);
}

void QWoTermWidget::replaceWidget(QSplitter *spliter, int idx, QWidget *widget)
{
    QWidget *current = spliter->widget(idx);
    QRect geom = current->geometry();
    current->setParent(nullptr);
    widget->setParent(spliter);
    widget->setGeometry(geom);
}

void QWoTermWidget::splitWidget(int sz, bool vertical)
{
    QSplitter *splitParent = qobject_cast<QSplitter*>(parent());
    if(splitParent == nullptr) {
        return;
    }
    QWoSshProcess *sshproc = qobject_cast<QWoSshProcess*>(m_process);
    if(sshproc == nullptr) {
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
    QString target = sshproc->target();
    QWoSshProcess *process = new QWoSshProcess(target, splitter);
    QWoTermWidget *term = new QWoTermWidget(process, splitter);
    splitter->addWidget(term);
    process->start();

    QList<int> ls;
    ls << 1 << 1;
    splitter->setSizes(ls);
}

QWoTermWidgetImpl *QWoTermWidget::findTermImpl()
{
    QWidget *widgetParent = parentWidget();
    QWoTermWidgetImpl *impl = qobject_cast<QWoTermWidgetImpl*>(widgetParent);
    while(impl == nullptr) {
        widgetParent = widgetParent->parentWidget();
        if(widgetParent == nullptr) {
            return nullptr;
        }
        impl = qobject_cast<QWoTermWidgetImpl*>(widgetParent);
    }
    return impl;
}

void QWoTermWidget::addToTermImpl()
{
    QWoTermWidgetImpl *impl = findTermImpl();
    if(impl) {
        impl->addToList(this);
    }
}

void QWoTermWidget::removeFromTermImpl()
{
    QWoTermWidgetImpl *impl = findTermImpl();
    if(impl) {
        impl->removeFromList(this);
    }
}

void QWoTermWidget::onBroadcastMessage(int type, QVariant msg)
{
    qDebug() << "type" << type << "msg" << msg;
}

bool QWoTermWidget::handleWoEvent(QEvent *ev)
{
    if(ev->type() != QWoEvent::EventType) {
        return false;
    }
    QWoEvent *we = (QWoEvent*)ev;
    QWoEvent::WoEventType type = we->eventType();
    if(type == QWoEvent::PropertyChanged) {
        triggerPropertyCheck();
    }

    return false;
}
