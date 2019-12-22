#include "qwoscriptwidgetimpl.h"
#include "qwoglobal.h"
#include "qwoevent.h"
#include "qwotermwidget.h"
#include "qwosetting.h"
#include "qwoutils.h"
#include "qwoscriptselector.h"
#include "qwoscriptmaster.h"
#include "qwopasswordinput.h"

#include <QCloseEvent>
#include <QSplitter>
#include <QVBoxLayout>
#include <QApplication>
#include <QMessageBox>
#include <QScriptEngineDebugger>
#include <QScriptEngine>
#include <QFile>


QWoScriptWidgetImpl::QWoScriptWidgetImpl(QWidget *parent)
    : QWoShowerWidget (parent)
{
    //setAutoFillBackground(true);
    //QPalette pal(Qt::red);
    //setPalette(pal);

    QVBoxLayout *layout = new QVBoxLayout(this);
    setLayout(layout);

    layout->setSpacing(0);
    layout->setMargin(0);
    m_vspliter = new QSplitter(Qt::Vertical, this);
    m_vspliter->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    layout->addWidget(m_vspliter);

    QFont font = QApplication::font();
    font.setFamily(DEFAULT_FONT_FAMILY);
    font.setPointSize(DEFAULT_FONT_SIZE);
    m_term = new QTermWidget();
    m_term->setAttribute(Qt::WA_DeleteOnClose);
    m_term->setTerminalFont(font);
    m_term->setScrollBarPosition(QTermWidget::ScrollBarRight);
    m_term->setColorScheme(DEFAULT_COLOR_SCHEMA);
    m_term->setKeyBindings(DEFAULT_KEYBOARD_BINDING);
    m_term->setBlinkingCursor(true);
    m_term->setBidiEnabled(true);
    QString val = QWoSetting::value("property/default").toString();
    QVariantMap mdata = QWoUtils::qBase64ToVariant(val).toMap();
    resetProperty(mdata);

    m_selector = new QWoScriptSelector();
    QObject::connect(m_selector, SIGNAL(readyStop()), this, SLOT(onReadyScriptStop()));
    QObject::connect(m_selector, SIGNAL(readyDebug(const QString&,const QStringList&,const QString&,const QStringList&)), this, SLOT(onReadyScriptDebug(const QString&,const QStringList&,const QString&,const QStringList&)));
    QObject::connect(m_selector, SIGNAL(readyRun(const QString&,const QStringList&,const QString&,const QStringList&)), this, SLOT(onReadyScriptRun(const QString&,const QStringList&,const QString&,const QStringList&)));

    m_selector->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    m_term->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

    m_term->setMinimumHeight(300);
    m_term->setMaximumHeight(102400);
    m_selector->setMinimumHeight(200);
    m_selector->resize(0, 150);
    m_selector->setMaximumHeight(1024);

    m_vspliter->addWidget(m_term);
    m_vspliter->addWidget(m_selector);
    m_vspliter->setChildrenCollapsible(false);

    m_master = new QWoScriptMaster(m_term, this);
    QObject::connect(m_master, SIGNAL(passwordRequest(const QString&,bool)), this, SLOT(onPasswordRequest(const QString&, bool)));
    QObject::connect(m_master, SIGNAL(logArrived(const QString&,const QString&)), m_selector, SLOT(onLogArrived(const QString&,const QString&)));
}

QWoScriptWidgetImpl::~QWoScriptWidgetImpl()
{
    delete m_term;
}

void QWoScriptWidgetImpl::closeEvent(QCloseEvent *event)
{
    emit aboutToClose(event);
    if(event->isAccepted()) {
        return;
    }
    QWidget::closeEvent(event);
}

void QWoScriptWidgetImpl::resizeEvent(QResizeEvent *event)
{
    QWoWidget::resizeEvent(event);
    QSize newSize = event->size();
    QRect rt(0, 0, newSize.width(), newSize.height());
}

bool QWoScriptWidgetImpl::event(QEvent *event)
{
    return QWoShowerWidget::event(event);
}

void QWoScriptWidgetImpl::resetProperty(QVariantMap mdata)
{
    if(mdata.isEmpty()) {
        return;
    }
    QString schema = mdata.value("colorSchema", DEFAULT_COLOR_SCHEMA).toString();
    m_term->setColorScheme(schema);

    QString fontName = mdata.value("fontName", DEFAULT_FONT_FAMILY).toString();
    int fontSize = mdata.value("fontSize", DEFAULT_FONT_SIZE).toInt();
    QFont ft(fontName, fontSize);
    m_term->setTerminalFont(ft);

    QString cursorType = mdata.value("cursorType", "block").toString();
    if(cursorType.isEmpty() || cursorType == "block") {
        m_term->setKeyboardCursorShape(Konsole::Emulation::KeyboardCursorShape::BlockCursor);
    }else if(cursorType == "underline") {
        m_term->setKeyboardCursorShape(Konsole::Emulation::KeyboardCursorShape::UnderlineCursor);
    }else {
        m_term->setKeyboardCursorShape(Konsole::Emulation::KeyboardCursorShape::IBeamCursor);
    }
    int lines = mdata.value("historyLength", DEFAULT_HISTORY_LINE_LENGTH).toInt();
    m_term->setHistorySize(lines);
}

void QWoScriptWidgetImpl::onReadyScriptRun(const QString& fullPath, const QStringList &scripts, const QString &entry, const QStringList &args)
{
    m_master->start(fullPath, scripts, entry, args, false);
}

void QWoScriptWidgetImpl::onReadyScriptStop()
{
    m_master->stop();
}

void QWoScriptWidgetImpl::onReadyScriptDebug(const QString& fullPath, const QStringList &scripts, const QString &entry, const QStringList &args)
{
    m_master->start(fullPath, scripts, entry, args, true);
}

void QWoScriptWidgetImpl::onPasswordInputResult(const QString &pass, bool isSave)
{
    m_master->passwordInput(pass);
}

void QWoScriptWidgetImpl::onPasswordRequest(const QString &prompt, bool echo)
{
    showPasswordInput(prompt, echo);
}

void QWoScriptWidgetImpl::init()
{

}

void QWoScriptWidgetImpl::showPasswordInput(const QString &prompt, bool echo)
{
    if(m_passInput == nullptr) {
        m_passInput = new QWoPasswordInput(m_term);
        QObject::connect(m_passInput, SIGNAL(result(const QString&,bool)), this, SLOT(onPasswordInputResult(const QString&,bool)));
    }
    m_passInput->reset(prompt, echo);
    QSize sz = m_term->size();
    m_passInput->setGeometry(0, 0, sz.width(), sz.height());
    m_passInput->show();
}
