#include "qwoshellprocess.h"
#include "qwosetting.h"
#include "qwocmdspy.h"
#include "qwoscript.h"

#include <QProcess>
#include <QThread>
#include <QEventLoop>
#include <QDebug>
#include <QLocalServer>
#include <QLocalSocket>
#include <QCoreApplication>
#include <QTimer>
#include <QDir>

QWoShellProcess::QWoShellProcess(QString target, QObject *parent)
    : QObject (parent)
    , m_process(new QProcess())
    , m_target(target)
{
    QObject::connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(onReadyReadStandardOutput()));
    QObject::connect(m_process, SIGNAL(readyReadStandardError()), this, SLOT(onReadyReadStandardError()));
    QObject::connect(m_process, SIGNAL(finished(int)), this, SLOT(onFinished(int)));
    m_spy = new QWoCmdSpy(this);
    QObject::connect(m_spy, SIGNAL(inputArrived()), this, SLOT(onInputArrived()));
    QObject::connect(m_spy, SIGNAL(applicationKeypad(bool)), this, SLOT(onApplicationKeypad(bool)));
}

QWoShellProcess::~QWoShellProcess()
{
    m_process->kill();
    m_process->terminate();
    delete m_process;
}

void QWoShellProcess::stop()
{
    if(m_eventLoop) {
        m_eventLoop->exit(ERR_SCRIPT_STOP);
    }
}

bool QWoShellProcess::login()
{
    QString program = QWoSetting::sshProgramPath();
    QString cfg = QWoSetting::sshServerListPath();
    m_process->setProgram(program);

    QStringList args;
    args.append(m_target);
    args.append("-F");
    args.append(cfg);
    m_process->setArguments(args);

    QByteArray sep = "\033[1m\033[31m\r\n----------"+m_target.toUtf8()+"--------------\033[0m\r\n";
    emit dataReceived(sep);

    QString name = QString("WoTerm%1_%2").arg(QCoreApplication::applicationPid()).arg(quint64(this));
    m_server = new QLocalServer(this);
    QObject::connect(m_server, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
    m_server->listen(name);
    QString fullName = m_server->fullServerName();
    qDebug() << "ipc server name:" << name << " - fulllName:" << fullName;
    QStringList env = m_process->environment();
    env << "TERM=xterm-256color";
    env << "TERM_MSG_IPC_NAME="+fullName;
    m_process->setEnvironment(env);
    m_process->start();
    return wait();
}

bool QWoShellProcess::seperator()
{
    QByteArray sep = "\033[1m\033[31m\r\n----------"+m_target.toUtf8()+"--------------\033[0m\r\033[?0G";
    QByteArray sep2 = "\033[1m\033[31m\r\n----------"+m_target.toUtf8()+"--------------\033[0m\r\033[?1034h";
    emit dataReceived(sep);
    return waitInput();
}

bool QWoShellProcess::execCommand(const QString &cmd)
{
    if(!seperator()) {
        return false;
    }
    QString cmdNew = cmd.simplified();
    cmdNew.append("\r");
    m_process->write(cmdNew.toUtf8());
    if(!wait()) {
        return false;
    }
    QStringList output = m_spy->output();
    int code = extractExitCode();
    return setOutput(code, output);
}

bool QWoShellProcess::sudo(const QString &pwd)
{
    if(!seperator()) {
        return false;
    }
    m_process->write("sudo -i\r");
    int why = 0;
    QStringList output;
    if(!wait(&why, 3000)) {
        if(why != ERR_SCRIPT_TIMEOUT) {
            return false;
        }
        output.append(m_spy->output());
        m_process->write(pwd.toUtf8());
        m_process->write("\r");
        if(!wait()) {
            return false;
        }
    }
    output.append(m_spy->output());
    int code = extractExitCode();
    return setOutput(code, output);
}

bool QWoShellProcess::su(const QString &user, const QString &pwd)
{
    if(!seperator()) {
        return false;
    }
    m_process->write(QString("su %1\r").arg(user).toUtf8());
    int why = 0;
    QStringList output;
    if(!wait(&why, 3000)) {
        if(why != ERR_SCRIPT_TIMEOUT) {
            return false;
        }
        output.append(m_spy->output());
        m_process->write(pwd.toUtf8());
        m_process->write("\r");
        if(!wait()) {
            return false;
        }
    }
    output.append(m_spy->output());
    int code = extractExitCode();
    return setOutput(code, output);
}

bool QWoShellProcess::upload(const QString &file)
{
    if(!seperator()) {
        return false;
    }
    QFileInfo fi(file);
    if(!fi.isFile()) {
        logError(QString("The Local File[%1] is not file.").arg(file));
        return false;
    }

    QProcess process(this);
    m_zmodem = &process;
    QObject::connect(&process, SIGNAL(readyReadStandardOutput()), this, SLOT(onZmodemReadyReadStandardOutput()));
    QObject::connect(&process, SIGNAL(readyReadStandardError()), this, SLOT(onZmodemReadyReadStandardError()));
    QObject::connect(&process, SIGNAL(finished(int)), this, SLOT(onZmodemFinished(int)));

    QString sz = QWoSetting::zmodemSZPath();
    process.setProgram(sz);
    QStringList args;
    args.append(file);
    process.setArguments(args);
    process.start();
    return wait();
}

bool QWoShellProcess::download(const QString &remote, const QString &local)
{
    if(!seperator()) {
        return false;
    }
    QString cmd(remote);
    QFileInfo fi(local);
    if(!fi.isDir()) {
        logError(QString("The Local Directory[%1] is not exist.").arg(local));
        return false;
    }
    m_process->write(QString("ls '%1/.'\r").arg(remote).toUtf8());
    if(!wait()) {
        // failed to sz file directory.
        return false;
    }
    int code = extractExitCode();
    if(code == 0) {
        logError("Do not support direcoty.");
        return false;
    }
    m_process->write(QString("sz %1\r").arg(remote).toUtf8());
    QProcess process(this);
    m_zmodem = &process;
    QObject::connect(&process, SIGNAL(readyReadStandardOutput()), this, SLOT(onZmodemReadyReadStandardOutput()));
    QObject::connect(&process, SIGNAL(readyReadStandardError()), this, SLOT(onZmodemReadyReadStandardError()));
    QObject::connect(&process, SIGNAL(finished(int)), this, SLOT(onZmodemFinished(int)));

    QString rz = QWoSetting::zmodemRZPath();
    process.setProgram(rz);
    process.setWorkingDirectory(local);
    process.start();
    return wait();
}

void QWoShellProcess::input(const QString &pwd)
{
    m_process->write(pwd.toUtf8());
    m_process->write("\r");
}

int QWoShellProcess::exitCode()
{
    return m_cmdExitCode;
}

QString QWoShellProcess::cmd() const
{
    return m_cmd;
}

QStringList QWoShellProcess::output() const
{
    return m_cmdOutput;
}

QString QWoShellProcess::target() const
{
    return m_target;
}

void QWoShellProcess::onReadyReadStandardOutput()
{
    QByteArray out = m_process->readAllStandardOutput();
    if(m_zmodem) {
        m_zmodem->write(out);
    }else{
        qDebug() << out;
        m_spy->process(out.data());
        emit dataReceived(out);
    }
}

void QWoShellProcess::onReadyReadStandardError()
{
    QByteArray err = m_process->readAllStandardError();
    if(m_zmodem) {
        m_zmodem->write(err);
    }else{
        qDebug() << err;
        m_spy->process(err.data());
        emit dataReceived(err);
    }
}

void QWoShellProcess::onFinished(int code)
{
    Q_UNUSED(code);
    if(m_zmodem) {
        m_zmodem->kill();
        m_zmodem->terminate();
    }
    if(m_eventLoop){
        m_eventLoop->exit(ERR_SCRIPT_FINISH);
    }
}

void QWoShellProcess::onInputArrived()
{
    if(m_eventLoop && m_zmodem == nullptr){
        m_eventLoop->exit(ERR_SCRIPT_NORMAL);
    }
}

void QWoShellProcess::onApplicationKeypad(bool state)
{
    if(state) {
        m_eventLoop->exit(ERR_SCRIPT_APP_KEYPAD);
    }
}

void QWoShellProcess::onZmodemReadyReadStandardOutput()
{
    if(m_zmodem){
        QByteArray output = m_zmodem->readAllStandardOutput();
        m_process->write(output);
    }
}

void QWoShellProcess::onZmodemReadyReadStandardError()
{
    if(m_zmodem){
        QByteArray error = m_zmodem->readAllStandardError();
        emit dataReceived(error);
    }
}

void QWoShellProcess::onZmodemFinished(int code)
{
    Q_UNUSED(code);
    if(m_zmodem){
        m_eventLoop->exit(ERR_SCRIPT_NORMAL);
    }
}

void QWoShellProcess::onNewConnection()
{
    m_ipc = m_server->nextPendingConnection();
    m_reader = new FunArgReader(m_ipc, this);
    m_writer = new FunArgWriter(m_ipc, this);
    QObject::connect(m_ipc, SIGNAL(disconnected()), this, SLOT(onClientDisconnected()));
    QObject::connect(m_ipc, SIGNAL(error(QLocalSocket::LocalSocketError)), this, SLOT(onClientError()));
    QObject::connect(m_ipc, SIGNAL(readyRead()), this, SLOT(onClientReadyRead()));
}

void QWoShellProcess::onClientError()
{
    QLocalSocket *local = qobject_cast<QLocalSocket*>(sender());
    if(local) {
        local->deleteLater();
    }
}

void QWoShellProcess::onClientDisconnected()
{
    QLocalSocket *local = qobject_cast<QLocalSocket*>(sender());
    if(local) {
        local->deleteLater();
    }
}

void QWoShellProcess::onClientReadyRead()
{
    m_reader->readAll();
    while(1) {
        QStringList funArgs = m_reader->next();
        if(funArgs.length() <= 0) {
            return;
        }
        qDebug() << funArgs;
        if(funArgs[0] == "ping") {
            echoPong();
        }else if(funArgs[0] == "getwinsize") {
            updateTermSize();
        }else if(funArgs[0] == "requestpassword") {
            if(funArgs.count() == 3) {
                QString prompt = funArgs.at(1);
                bool echo = QVariant(funArgs.at(2)).toBool();
                requestPassword(prompt, echo);
            }
        }else if(funArgs[0] == "successToLogin") {

        }
    }
}

void QWoShellProcess::echoPong()
{

}

void QWoShellProcess::updateTermSize()
{
    QStringList funArgs;
    funArgs << "setwinsize" << QString("%1").arg(1024) << QString("%1").arg(768);
    if(m_ipc) {
        qDebug() << funArgs;
        m_writer->write(funArgs);
    }
}

void QWoShellProcess::updatePassword(const QString &pwd)
{

}

void QWoShellProcess::requestPassword(const QString &prompt, bool echo)
{
    emit passwordRequest(prompt, echo);
}

void QWoShellProcess::logWarning(const QString &msg)
{
    emit logArrived("warn", msg);
}

void QWoShellProcess::logInfo(const QString &msg)
{
    emit logArrived("info", msg);
}

void QWoShellProcess::logError(const QString &msg)
{
    emit logArrived("error", msg);
}


bool QWoShellProcess::wait(int *why, int ms)
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
    m_spy->reset();
    int code = loop.exec();
    if(why != nullptr) {
        *why = code;
    }
    if(code != ERR_SCRIPT_NORMAL) {
        return setFailure();
    }
    return setOk();
}

void QWoShellProcess::sleep(int ms)
{
    QEventLoop loop;
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&loop] () {
        loop.exit(ERR_SCRIPT_NORMAL);
    });
    timer.setSingleShot(true);
    timer.setInterval(ms);
    timer.start();
    loop.exec();
}

bool QWoShellProcess::waitInput()
{
    m_process->write("\r");
    return wait();
}

int QWoShellProcess::extractExitCode()
{
    m_process->write("echo $?\r");
    if(!wait()) {
        return -1;
    }
    QStringList lines = m_spy->output();
    QString code = "";
    for(int i= 0; i < lines.count(); i++) {
        if(lines.at(i).endsWith("echo $?")) {
            if(i+1 < lines.count()) {
                code = lines.at(i+1);
                break;
            }
        }
    }
    if(code.isEmpty()) {
        return -1;
    }
    bool ok = false;
    int icode = code.toInt(&ok);
    if(!ok) {
        return -1;
    }
    return icode;
}

bool QWoShellProcess::setFailure()
{
    QStringList result;
    result << "command execution" << "failed for bad status.";
    return setOutput(-1, result);
}

bool QWoShellProcess::setOk()
{
    QStringList result;
    result << "command execution" << "ok";
    return setOutput(0, result);
}

bool QWoShellProcess::setOutput(int code, const QStringList &out)
{
    m_cmdExitCode = code;
    m_cmdOutput = out;
    return code == 0;
}
