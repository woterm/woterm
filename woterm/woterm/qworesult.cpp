#include "qworesult.h"
#include "qwoshellprocess.h"

#include <QProcess>
#include <QThread>
#include <QEventLoop>
#include <QTimer>
#include <QScriptEngine>

QWoResult::QWoResult()
{

}

QWoResult::QWoResult(const QWoResult &v)
{
    m_data = v.m_data;
}

void QWoResult::operator=(const QWoResult &v)
{
    m_data = v.m_data;
}

QString QWoResult::command() const
{
    if(m_data.isEmpty()) {
        return "not run";
    }
    return m_data.first().guessCmd;
}

void QWoResult::append(QWoShellProcess *proc)
{
    QStringList output = proc->output();
    QString cmd = output.takeFirst();
    Result r;
    r.guessCode = proc->exitCode();
    r.guessOutput = output.join("\r\n");
    r.guessCmd = cmd;
    m_data.insert(proc->target(), r);
}

void QWoResult::append(const QString &cmd, int code, const QString &output)
{
    Result r;
    r.guessCode = code;
    r.guessOutput = output;
    r.guessCmd = cmd;
    m_data.insert("local", r);
}


bool QWoResultPrototype::hasError() const
{
    QWoResult result = qscriptvalue_cast<QWoResult>(thisObject());
    return result.hasError();
}

QString QWoResultPrototype::command() const
{
    QWoResult result = qscriptvalue_cast<QWoResult>(thisObject());
    return result.command();
}

QString QWoResultPrototype::output() const
{
    QWoResult result = qscriptvalue_cast<QWoResult>(thisObject());
    return result.output(QString());
}


int QWoResultPrototype::code() const
{
    QWoResult result = qscriptvalue_cast<QWoResult>(thisObject());
    return result.code(QString());
}

int QWoResultPrototype::hostCode(const QString &host) const
{
    QWoResult result = qscriptvalue_cast<QWoResult>(thisObject());
    return result.code(host);
}

QString QWoResultPrototype::hostOutput(const QString &host) const
{
    QWoResult result = qscriptvalue_cast<QWoResult>(thisObject());
    return result.output(host);
}

bool QWoResult::hasError()
{
    if(m_data.isEmpty()) {
        return true;
    }
    for(QMap<QString, Result>::iterator iter  = m_data.begin(); iter != m_data.end(); iter++) {
        if(iter.value().guessCode != 0) {
            return true;
        }
    }
    return false;
}

int QWoResult::code(const QString &host)
{
    if(host.isEmpty()) {
        for(QMap<QString, Result>::iterator iter  = m_data.begin(); iter != m_data.end(); iter++) {
            if(iter.value().guessCode != 0) {
                return 1;
            }
        }
        return 0;
    }
    if(!m_data.contains(host)) {
        return -1;
    }
    return m_data.value(host).guessCode;
}

QString QWoResult::output(const QString &host)
{
    if(host.isEmpty()) {
        QStringList buf;
        for(QMap<QString, Result>::iterator iter  = m_data.begin(); iter != m_data.end(); iter++) {
            buf.append(iter.value().guessOutput);
        }
        return buf.join("\r\n");
    };
    if(!m_data.contains(host)) {
        return "not run";
    }
    return m_data.value(host).guessOutput;
}
