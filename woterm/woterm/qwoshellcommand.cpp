#include "qwoshellcommand.h"
#include "qwosetting.h"
#include "qwoshellprocess.h"
#include "qwocmdspy.h"
#include "qwoscriptmaster.h"

#include <QProcess>
#include <QThread>
#include <QEventLoop>
#include <QTimer>
#include <QScriptEngine>

QWoShellCommand::QWoShellCommand(QWoLog *log, QObject *parent)
    :QObject (parent)
    ,m_log(log)
{
}

QWoShellCommand::~QWoShellCommand()
{

}

QWoResult QWoShellCommand::login(const QStringList& hosts)
{
    QWoResult result;
    if(hosts.isEmpty()) {
        showError(QString("the host should not be empty array."));
        return result;
    }
    for(int i = 0; i < hosts.count(); i++) {
        QString target = hosts.at(i);
        QWoShellProcess *proc = new QWoShellProcess(target, this);
        QObject::connect(proc, SIGNAL(dataReceived(const QByteArray&)), this, SLOT(onDataReceived(const QByteArray&)));
        QObject::connect(proc, SIGNAL(passwordRequest(const QString&,bool)), this, SLOT(onPasswordRequest(const QString&,bool)));
        QObject::connect(proc, SIGNAL(logArrived(const QString&,const QString&)), this, SLOT(onLogArrived(const QString&, const QString&)));
        m_all.push_back(proc);        
        bool ok = proc->login();
        result.append(proc);
        if(!ok) {
            showError(QString("failed to connect host:[%1]").arg(target));
            cleanAll();
            return result;
        }
    }
    return result;
}

void QWoShellCommand::stop()
{
    for(int i = 0; i < m_all.count(); i++) {
        QWoShellProcess *proc = m_all.at(i);
        proc->stop();
    }
}

QWoResult QWoShellCommand::execCommand(const QString &cmd, const QString& host)
{
    QWoResult result;
    if(m_all.isEmpty()) {
        showError(QString("the host should not be empty array."));
        return result;
    }
    for(int i = 0; i < m_all.count(); i++) {
        QWoShellProcess *proc = m_all.at(i);
        if(host.isEmpty() || proc->target() == host){
            bool ok = proc->execCommand(cmd);
            result.append(proc);
            if(!ok) {
                showCommandFailure(cmd, proc->target());
                cleanAll();
                return result;
            }
        }
    }
    return result;
}

QWoResult QWoShellCommand::sudo(const QString &pwd, const QString& host)
{
    QWoResult result;
    if(m_all.isEmpty()) {
        showError(QString("the host should not be empty array."));
        return result;
    }
    for(int i = 0; i < m_all.count(); i++) {
        QWoShellProcess *proc = m_all.at(i);
        if(host.isEmpty() || proc->target() == host){
            bool ok = proc->sudo(pwd);
            result.append(proc);
            if(!ok) {
                showCommandFailure("sudo", proc->target());
                cleanAll();
                return result;
            }
        }
    }
    return result;
}

QWoResult QWoShellCommand::su(const QString &user, const QString &pwd, const QString& host)
{
    QWoResult result;
    if(m_all.isEmpty()) {
        showError(QString("the host should not be empty array."));
        return result;
    }
    for(int i = 0; i < m_all.count(); i++) {
        QWoShellProcess *proc = m_all.at(i);
        if(host.isEmpty() || proc->target() == host){
            bool ok = proc->su(user, pwd);
            result.append(proc);
            if(!ok) {
                showCommandFailure("su", proc->target());
                cleanAll();
                return result;
            }
        }
    }
    return result;
}

QWoResult QWoShellCommand::upload(const QString &file)
{
    QWoResult result;
    if(m_all.isEmpty()) {
        showError(QString("the host should not be empty array."));
        return result;
    }
    for(int i = 0; i < m_all.count(); i++) {
        QWoShellProcess *proc = m_all.at(i);
        bool ok = proc->upload(file);
        result.append(proc);
        sleep(2000);
        if(!ok) {
            showCommandFailure("upload", proc->target());
            cleanAll();
            return result;
        }
    }
    return result;
}

QWoResult QWoShellCommand::download(const QString &remote, const QString &local)
{
    QWoResult result;
    if(m_all.isEmpty()) {
        showError(QString("the host should not be empty array."));
        return result;
    }
    for(int i = 0; i < m_all.count(); i++) {
        QWoShellProcess *proc = m_all.at(i);
        bool ok = proc->download(remote, local);
        result.append(proc);
        sleep(2000);
        if(!ok) {
            showCommandFailure("download", proc->target());
            cleanAll();
            return result;
        }
    }
    return result;
}

void QWoShellCommand::sleep(int ms)
{
    QEventLoop loop;
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&loop] () {
        loop.exit(0);
    });
    timer.setSingleShot(true);
    timer.setInterval(ms);
    timer.start();
    loop.exec();
}

void QWoShellCommand::cleanAll()
{
    while(m_all.count() > 0) {
        QWoShellProcess *proc = m_all.takeFirst();
        delete proc;
    }
}

void QWoShellCommand::showSeperator(const QString &host)
{
    QByteArray buf = "\033[1m\033[31m\r\n" + host.toUtf8() + "\033[0m\r\n";
    emit dataReceived(buf);
}

void QWoShellCommand::showError(const QString &msg)
{
    QByteArray buf = "\033[1m\033[31m\r\n" + msg.toUtf8() + "\033[0m\r\n";
    emit dataReceived(buf);
}

void QWoShellCommand::showCommandFailure(const QString &cmd, const QString &host)
{
    showError(QString("failed to execute command[%1] on target host[%2]").arg(cmd).arg(host));
}

void QWoShellCommand::showOk(const QString &msg)
{
    QByteArray buf = "\033[1m\033[31m\r\n" + msg.toUtf8() + "\033[0m\r\n";
    emit dataReceived(buf);
}

void QWoShellCommand::onPasswordRequest(const QString &prompt, bool echo)
{
    QWoShellProcess *proc = qobject_cast<QWoShellProcess*>(sender());
    m_current = proc;
    emit passwordRequest(prompt, echo);
}

void QWoShellCommand::onPasswordInput(const QString &pwd)
{
    if(m_current) {
        m_current->input(pwd);
    }
}

void QWoShellCommand::onLogArrived(const QString &level, const QString &msg)
{
    if(m_log) {
        m_log->log(level, msg);
    }
}

void QWoShellCommand::onDataReceived(const QByteArray &data)
{
    emit dataReceived(data);
}
