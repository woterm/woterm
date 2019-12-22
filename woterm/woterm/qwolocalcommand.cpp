#include "qwolocalcommand.h"
#include "qwosetting.h"
#include "qwoscriptmaster.h"
#include "qwoutils.h"
#include "qwoscript.h"

#include <QProcess>
#include <QStandardPaths>
#include <QEventLoop>
#include <QTimer>

QWoLocalCommand::QWoLocalCommand(const QString &path, QWoLog *log, QObject *parent)
    :QObject (parent)
    ,m_log(log)
    ,m_pathScript(path)
{
    m_dir.setPath(m_pathScript);
}

QWoLocalCommand::~QWoLocalCommand()
{

}

QString QWoLocalCommand::scriptDirectory() const
{
    return m_pathScript;
}

bool QWoLocalCommand::cd(const QString &path)
{
    QString tmp = toAbsolutePath(path);
    showCommand("cd "+tmp);
    if(isFile(tmp)) {
        showResult("failed to cd a file");
        return false;
    }
    m_dir.setPath(tmp);
    return true;
}

bool QWoLocalCommand::exist(const QString &path)
{
    QString tmp = toAbsolutePath(path);
    showCommand("exist "+tmp);
    if(!QFileInfo::exists(tmp)) {
        showResult("the path is not exist:" + tmp);
        return false;
    }
    return true;
}

QString QWoLocalCommand::pwd()
{
    showCommand("pwd");
    return m_dir.currentPath();
}

QString QWoLocalCommand::nativeSeperator() const
{
    return QDir::separator();
}

bool QWoLocalCommand::isFile(const QString& path)
{
    return QFileInfo(path).isFile();
}

QString QWoLocalCommand::toAbsolutePath(const QString& path) const
{
    if(QDir::isAbsolutePath(path)) {
        return path;
    }
    return QDir::cleanPath(m_dir.absolutePath() + "/./" + path);
}

bool QWoLocalCommand::isAbsolutePath(const QString &path) const
{
    return QDir::isAbsolutePath(path);
}

QString QWoLocalCommand::cleanPath(const QString &path) const
{
    return QDir::cleanPath(path);
}

QString QWoLocalCommand::toNativePath(const QString &path) const
{
    return QDir::toNativeSeparators(path);
}

QStringList QWoLocalCommand::ls(const QString &path)
{
    QString tmp = toAbsolutePath(path);
    showCommand("ls " + tmp);
    if(isFile(tmp)) {
        showResult("failed to ls a file:" + tmp);
        return QStringList();
    }
    QStringList fis = QDir(tmp).entryList();
    showResult(fis.join(" "));
    return fis;
}

bool QWoLocalCommand::rmpath(const QString &path)
{
    QString tmp = toAbsolutePath(path);
    showCommand("rmpath "+tmp);
    if(!exist(tmp)) {
        showResult("the path is not exist:" + tmp);
    }
    QDir dir;
    if(!dir.rmpath(tmp)) {
        showResult("failed to remove the path.");
        return false;
    }
    return true;
}

void QWoLocalCommand::echo(const QString &msg)
{
    showResult(msg);
}

bool QWoLocalCommand::mkpath(const QString &path)
{
    QString tmp = toAbsolutePath(path);
    showCommand("mkpath "+tmp);
    QDir dir;
    if(!dir.mkpath(tmp)) {
        showResult("failed to mkpath:"+tmp);
        return false;
    }
    return true;
}

QWoResult QWoLocalCommand::fileContent(const QString &path)
{
    QWoResult result;
    QString tmp = toAbsolutePath(path);
    showCommand("fileContent "+tmp);
    QFile file(tmp);
    if(file.open(QFile::ReadOnly)){
        result.append("content:" + path, 0, file.readAll());
    }
    return result;
}

QWoResult QWoLocalCommand::runScriptFile(const QString &file)
{
    QWoResult result;

    QString tmp = toAbsolutePath(file);
    showCommand("runScriptFile "+tmp);

    if(!isFile(tmp)) {
        result.append("runScriptFile "+ tmp, 0, "should be a file");
        return result;
    }
    QProcess process;
    QObject::connect(&process, SIGNAL(readyReadStandardOutput()), this, SLOT(onReadyReadStandardOutput()));
    QObject::connect(&process, SIGNAL(readyReadStandardError()), this, SLOT(onReadyReadStandardError()));
    QObject::connect(&process, SIGNAL(finished(int)), this, SLOT(onFinished(int)));
    process.start(tmp);

    if(!wait()) {
        QByteArray err = process.property("stderr").toByteArray();
        result.append(file, 1, err);
        return result;
    }    
    QByteArray out = process.property("stdout").toByteArray();
    result.append(file, 0, out);
    return result;
}

void QWoLocalCommand::onReadyReadStandardOutput()
{
    QProcess *proc = qobject_cast<QProcess*>(sender());
    QByteArray buf = proc->property("stdout").toByteArray();
    buf.append(proc->readAllStandardOutput());
    proc->setProperty("stdout", buf);
}

void QWoLocalCommand::onReadyReadStandardError()
{
    QProcess *proc = qobject_cast<QProcess*>(sender());
    QByteArray buf = proc->property("stderr").toByteArray();
    buf.append(proc->readAllStandardError());
    proc->setProperty("stderr", buf);
}

void QWoLocalCommand::onFinished(int code)
{
    m_eventLoop->exit(ERR_SCRIPT_NORMAL);
}

void QWoLocalCommand::showCommand(const QString &cmd)
{
    QByteArray sep = "\033[1m\033[31m\r\n----------local--------------\033[0m";
    QByteArray tmp = "\033[1m\033[0m\r\n"+cmd.toUtf8()+"\033[0m";
    emit dataReceived(sep+tmp);
}

void QWoLocalCommand::showResult(const QString &msg)
{
    QByteArray result = "\r\n"+msg.toUtf8();
    emit dataReceived(result);
}

bool QWoLocalCommand::wait(int *why, int ms)
{
    QEventLoop loop;
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&loop] () {
        loop.exit(ERR_SCRIPT_TIMEOUT);
    });
    timer.setSingleShot(true);
    timer.setInterval(ms);
    timer.start();
    m_eventLoop = &loop;
    int code = loop.exec();
    if(why != nullptr) {
        *why = code;
    }
    if(code != ERR_SCRIPT_NORMAL) {
        return false;
    }
    return true;
}
