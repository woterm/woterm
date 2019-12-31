#include "qwoprocess.h"
#include "qwoevent.h"

#include <qtermwidget.h>
#include <QApplication>
#include <QDebug>
#include <QMenu>
#include <QClipboard>
#include <QProcess>

QWoProcess::QWoProcess(const QString& sessionName, QObject *parent)
    : QObject (parent)
    , m_sessionName(sessionName)
    , m_process(new QProcess())
{
    QObject::connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(onReadyReadStandardOutput()));
    QObject::connect(m_process, SIGNAL(readyReadStandardError()), this, SLOT(onReadyReadStandardError()));
    QObject::connect(m_process, SIGNAL(finished(int)), this, SLOT(onFinished(int)));
    installEventFilter(this);
}

QWoProcess::~QWoProcess()
{
    m_process->terminate();
    m_process->kill();
    delete m_process;
}

QString QWoProcess::program() const
{
    return m_process->program();
}

void QWoProcess::setProgram(const QString &program)
{
    m_process->setProgram(program);
}

QStringList QWoProcess::arguments() const
{
    return m_process->arguments();
}

void QWoProcess::setArguments(const QStringList &arguments)
{
    return m_process->setArguments(arguments);
}

QString QWoProcess::sessionName() const
{
    return m_sessionName;
}

QStringList QWoProcess::environment() const
{
    return m_process->environment();
}

void QWoProcess::setEnvironment(const QStringList &env)
{
    m_process->setEnvironment(env);
}

void QWoProcess::setWorkingDirectory(const QString &dir)
{
    m_process->setWorkingDirectory(dir);
}

void QWoProcess::start()
{
    m_process->reset();
    m_process->start();
}

QByteArray QWoProcess::readAllStandardOutput()
{
    QByteArray data = m_process->readAllStandardOutput();
    QWoEvent ev(QWoEvent::AfterReadStdOut, data);
    QApplication::sendEvent(this, &ev);
    if(ev.isAccepted()) {
        if(ev.hasResult()) {
            return ev.result().toByteArray();
        }
    }
    return data;
}

QByteArray QWoProcess::readAllStandardError()
{
    QByteArray data = m_process->readAllStandardError();
    QWoEvent ev(QWoEvent::AfterReadStdErr, data);
    QApplication::sendEvent(this, &ev);
    if(ev.isAccepted()) {
        if(ev.hasResult()) {
            return ev.result().toByteArray();
        }
    }
    return data;
}

void QWoProcess::write(const QByteArray &data)
{
    if(data.isEmpty()) {
        return;
    }
    QWoEvent ev(QWoEvent::BeforeWriteStdOut, data);
    QApplication::sendEvent(this, &ev);
    m_process->setCurrentWriteChannel(QProcess::StandardOutput);
    if(ev.isAccepted()) {
        if(ev.hasResult()) {
            m_process->write(ev.result().toByteArray());
            return;
        }
        return;
    }
    m_process->write(data);
}

void QWoProcess::writeError(const QByteArray &data)
{
    QWoEvent ev(QWoEvent::BeforeWriteStdErr, data);
    QApplication::sendEvent(this, &ev);
    m_process->setCurrentWriteChannel(QProcess::StandardError);
    if(ev.isAccepted()) {
        if(ev.hasResult()) {
            m_process->write(ev.result().toByteArray());
        }
        return;
    }
    m_process->write(data);
}

void QWoProcess::close()
{
    m_process->close();
}

void QWoProcess::terminate()
{
    m_process->terminate();
}

void QWoProcess::kill()
{
    m_process->kill();
}

void QWoProcess::closeReadChannel(QProcess::ProcessChannel channel)
{
    m_process->closeReadChannel(channel);
}

void QWoProcess::closeWriteChannel()
{
    m_process->closeWriteChannel();
}

bool QWoProcess::waitForBytesWritten(int msecs)
{
    return m_process->waitForBytesWritten(msecs);
}


QTermWidget *QWoProcess::termWidget()
{
    return m_term;
}

void QWoProcess::setTermWidget(QTermWidget *widget)
{
    m_term = widget;
}

void QWoProcess::prepareContextMenu(QMenu *menu)
{

}

void QWoProcess::onReadyReadStandardOutput()
{
    QWoEvent ev(QWoEvent::BeforeReadStdOut);
    QApplication::sendEvent(this, &ev);
    if(ev.isAccepted()) {
        return;
    }
    emit readyReadStandardOutput();
}

void QWoProcess::onReadyReadStandardError()
{
    QWoEvent ev(QWoEvent::BeforeReadStdErr);
    QApplication::sendEvent(this, &ev);
    if(ev.isAccepted()) {
        return;
    }
    emit readyReadStandardError();
}

void QWoProcess::onFinished(int code)
{
    QWoEvent ev(QWoEvent::BeforeFinish, code);
    QApplication::sendEvent(this, &ev);
    if(ev.isAccepted()) {
        return;
    }
    emit finished(code);
}
