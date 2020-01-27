#include "qwosshprocess.h"
#include "qwoevent.h"
#include "qwosetting.h"
#include "qwosshconf.h"
#include "qwoutils.h"
#include "qwosessionproperty.h"
#include "qwomainwindow.h"
#include "qwocmdspy.h"

#include <qtermwidget.h>

#include <QApplication>
#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QThread>
#include <QProcess>
#include <QFileDialog>
#include <QClipboard>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMessageBox>
#include <QMetaObject>
#include <QTime>
#include <QEventLoop>


QWoSshProcess::QWoSshProcess(const QString& target, QObject *parent)
    : QWoProcess (target, parent)
    , m_idleCount(0)
{
    triggerKeepAliveCheck();

    QString program = QWoSetting::sshProgramPath();
    if(program.isEmpty()) {
        QMessageBox::critical(m_term, "ssh", "can't find ssh program.");
        QApplication::exit(0);
        return;
    }
    QString cfg = QWoSetting::sshServerListPath();
    if(cfg.isEmpty()) {
        QMessageBox::critical(m_term, "ssh server list", "no path for ssh server list.");
        QApplication::exit(0);
        return;
    }
    setProgram(program);
    QStringList args;
    args.append(target);
    args.append("-F");
    args.append(cfg);
    setArguments(args);

    QString name = QString("WoTerm%1_%2").arg(QApplication::applicationPid()).arg(quint64(this));   
    m_server = new QLocalServer(this);
    m_server->listen(name);
    QString fullName = m_server->fullServerName();
    qDebug() << "ipc server name:" << name << " - fulllName:" << fullName;
    QStringList env = environment();
    env << "TERM=xterm-256color";
    env << "TERM_MSG_IPC_NAME="+fullName;
    env << "WOTERM_DATA_PATH=" + QWoSetting::identifyFilePath();
    setEnvironment(env);

    QObject::connect(m_server, SIGNAL(newConnection()), this, SLOT(onNewConnection()));

    QTimer *timer = new QTimer(this);
    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
    timer->start(1000);
}

QWoSshProcess::~QWoSshProcess()
{
    if(m_process->isOpen()) {
        m_process->kill();
        m_process->waitForFinished();
    }
}

void QWoSshProcess::triggerKeepAliveCheck()
{
    HostInfo hi = QWoSshConf::instance()->find(sessionName());
    QVariantMap mdata = QWoUtils::qBase64ToVariant(hi.property).toMap();
    if(mdata.value("liveCheck", false).toBool()) {
        m_idleDuration = mdata.value("liveDuration", 60).toInt();
    }else{
        m_idleDuration = -1;
    }
}

QProcess *QWoSshProcess::createZmodem()
{
    QProcess *process = new QProcess(this);
    QObject::connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(onZmodemReadyReadStandardOutput()));
    QObject::connect(process, SIGNAL(readyReadStandardError()), this, SLOT(onZmodemReadyReadStandardError()));
    QObject::connect(process, SIGNAL(finished(int)), this, SLOT(onZmodemFinished(int)));
    return process;
}

int QWoSshProcess::isZmodemCommand(const QByteArray &data)
{
    bool isApp = m_term->isAppMode();
    if(isApp || data.length() < 6) {
        return -1;
    }
    // hex way.
    //char zmodem_init_hex[] = {'*','*','\030', 'B', '0', '0', '\0'};
    const char *buf = data.data();
    for(int i = 0; i < data.length() && i < 300; i++) {
        if(buf[i] == '*' && buf[i+1] == '*' && buf[i+2] == '\030'
                && buf[i+3] == 'B' && buf[i+4] == '0') {
            if(buf[i+5] == '1') {
                // shell trigger rz command.
                return 1;
            }else if(buf[i+5] == '0') {
                // shell trigger sz command.
                return 0;
            }
            return -1;
        }
    }
    return -1;
}

void QWoSshProcess::checkCommand(const QByteArray &out)
{
    bool isApp = m_term->isAppMode();
    if(!isApp) {
        if(out.lastIndexOf('\r') >= 0 || out.lastIndexOf('\n') >= 0) {
            QString command = m_term->lineTextAtCursor(1);
            command = command.simplified();
            if(command.length() < 200) {
                int idx = command.lastIndexOf('$');
                if(idx >= 0) {
                    command = command.mid(idx+1).trimmed();
                }
                idx = command.lastIndexOf(QChar::Space);
                if(idx < 0) {
                    QString cmd = command;
                    if(cmd == "rz") {
                        qDebug() << "command" << command;
                        QMetaObject::invokeMethod(this, "onZmodemSend", Qt::QueuedConnection);
                    }
                }
            }
        }
    }
}

bool QWoSshProcess::checkShellProgram(const QByteArray &name)
{
    sleep(300);
    if(m_process->state() != QProcess::Running) {
        return false;
    }
    if(m_zmodem) {
        return false;
    }
    if(m_term->isAppMode()) {
        return false;
    }

    QWoCmdSpy spy(this);
    m_spy = &spy;
    QObject::connect(&spy, &QWoCmdSpy::inputArrived, [this]{
        if(m_eventLoop) {
            m_eventLoop->exit(0);
        }
    });
    waitInput();
    spy.reset();
    QByteArray cmd = "which " + name;
    write(cmd+"\r");
    if(!wait()) {
        return false;
    }
    QString out = spy.output().join(",");
    if(!out.contains(cmd)) {
        return false;
    }
    int code = extractExitCode();
    return code == 0;
}

bool QWoSshProcess::waitInput()
{
    m_process->write("\r");
    return wait();
}

int QWoSshProcess::extractExitCode()
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

void QWoSshProcess::sleep(int ms)
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

bool QWoSshProcess::wait(int *why, int ms)
{
    QEventLoop loop;
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&loop] () {
        loop.exit(1);
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
    if(code != 0) {
        return false;
    }
    return true;
}

void QWoSshProcess::onNewConnection()
{
    m_ipc = m_server->nextPendingConnection();
    m_reader = new FunArgReader(m_ipc, this);
    m_writer = new FunArgWriter(m_ipc, this);
    QObject::connect(m_ipc, SIGNAL(disconnected()), this, SLOT(onClientDisconnected()));
    QObject::connect(m_ipc, SIGNAL(error(QLocalSocket::LocalSocketError)), this, SLOT(onClientError(QLocalSocket::LocalSocketError)));
    QObject::connect(m_ipc, SIGNAL(readyRead()), this, SLOT(onClientReadyRead()));
}

void QWoSshProcess::onClientError(QLocalSocket::LocalSocketError socketError)
{
    Q_UNUSED(socketError);
    QLocalSocket *local = qobject_cast<QLocalSocket*>(sender());
    if(local) {
        local->deleteLater();
    }
}

void QWoSshProcess::onClientDisconnected()
{
    QLocalSocket *local = qobject_cast<QLocalSocket*>(sender());
    if(local) {
        local->deleteLater();
    }
}

void QWoSshProcess::onClientReadyRead()
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
            //m_process->write("which sz\r");
        }
    }
}

void QWoSshProcess::onZmodemSend(bool local)
{
    if(m_zmodem) {
        return;
    }

    if(local) {
        if(!checkShellProgram("rz")) {
            return;
        }
    }

    m_zmodem = createZmodem();

    QString pathLast = QWoSetting::value("zmodem/lastPath").toString();
    QStringList files = QFileDialog::getOpenFileNames(m_term, tr("Select Files"), pathLast);
    qDebug() << "zmodem send " << files;
    if(files.isEmpty()) {
        onZmodemAbort();
        return;
    }
    QStringList args;
    QString path = files.front();
    int idx = path.lastIndexOf('/');
    if(idx > 0) {
        path = path.left(idx);
        QWoSetting::setValue("zmodem/lastPath", path);
    }
    for(int i = 0; i < files.length(); i++) {
        QString path = files.at(i);
        args.push_back(path);
    }
    if(m_exeSend.isEmpty()) {
        m_exeSend = QWoSetting::zmodemSZPath();
        if(m_exeSend.isEmpty()){
            onZmodemAbort();
            QMessageBox::warning(m_term, "zmodem", "can't find sz program.");
            return;
        }
    }

    m_zmodem->setProgram(m_exeSend);
    m_zmodem->setArguments(args);
    m_zmodem->start();
}

void QWoSshProcess::onZmodemRecv(bool local)
{
    if(m_zmodem) {
        return;
    }
    m_zmodem = createZmodem();
    QString path = QWoSetting::value("zmodem/lastPath").toString();
    QString filePath = QFileDialog::getExistingDirectory(m_term, tr("Open Directory"), path,  QFileDialog::ShowDirsOnly);
    qDebug() << "filePath" << filePath;
    if(filePath.isEmpty()) {
        onZmodemAbort();
        return;
    }
    filePath = QDir::toNativeSeparators(filePath);
    QWoSetting::setValue("zmodem/lastPath", filePath);

    if(m_exeRecv.isEmpty()) {
        m_exeRecv = QWoSetting::zmodemRZPath();
        if(m_exeRecv.isEmpty()) {
            onZmodemAbort();
            QMessageBox::warning(m_term, "zmodem", "can't find rz program.");
            return;
        }
    }    
    m_zmodem->setProgram(m_exeRecv);
    m_zmodem->setWorkingDirectory(filePath);
    m_zmodem->start();
}

void QWoSshProcess::onZmodemAbort()
{
    //sendData(QByteArrayLiteral("\030\030\030\030")); // Abort
    static char canistr[] = {24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 0};
    if(m_zmodem == nullptr) {
        return;
    }
    write(canistr);
    m_zmodem->deleteLater();
//    if(m_zmodem->program() == m_exeRecv) {
//        write(canistr);
//    }else{
//        m_zmodem->close();
//        write(canistr);
//    }
}

void QWoSshProcess::onTermTitleChanged()
{
    QString name = m_term->title();
    m_prompt = name.toLocal8Bit();
    qDebug() << "prompt changed:" << m_prompt;
}

void QWoSshProcess::onModifyThisSession()
{
    if(!QWoSshConf::instance()->exists(m_sessionName)){
        QMessageBox::warning(m_term, tr("Error"), tr("can't find the session, maybe it had been delete ago"));
        return;
    }
    QWoSessionProperty dlg(m_sessionName, m_term);
    QObject::connect(&dlg, SIGNAL(connect(const QString&)), QWoMainWindow::instance(), SLOT(onSessionReadyToConnect(const QString&)));
    int ret = dlg.exec();
    if(ret == QWoSessionProperty::Save) {
        HostInfo hi = QWoSshConf::instance()->find(m_sessionName);
        QWoEvent ev(QWoEvent::PropertyChanged);
        QCoreApplication::sendEvent(m_term, &ev);
    }
}

void QWoSshProcess::onDuplicateInNewWindow()
{
    QString path = QApplication::applicationFilePath();
    path.append(" ");
    path.append(m_sessionName);
    QProcess::startDetached(path);
}

void QWoSshProcess::onTimeout()
{
    if(m_idleDuration > 0) {
        if(m_idleCount++ > m_idleDuration) {
            qDebug() << m_sessionName << "idle" << m_idleCount << "Duration" << m_idleDuration;
            m_idleCount = 0;
            write(" \b");
        }
    }
}

void QWoSshProcess::echoPong()
{
    static int count = 0;
    QStringList funArgs;

    funArgs << "pong" << QString("%1").arg(count++);
    if(m_ipc) {
        qDebug() << funArgs;
        m_writer->write(funArgs);
    }
}

void QWoSshProcess::onZmodemFinished(int code)
{
    Q_UNUSED(code);
    m_zmodem->deleteLater();
    write("\r");
}

void QWoSshProcess::onZmodemReadyReadStandardOutput()
{
    if(m_zmodem) {
        static int count = 0;
        QByteArray read = m_zmodem->readAllStandardOutput();
        qDebug() << "zrecv" << count++ << read.length();
        write(read);
    }
}

void QWoSshProcess::onZmodemReadyReadStandardError()
{
    if(m_zmodem) {
        QByteArray out = m_zmodem->readAllStandardError();
        m_term->parseSequenceText(out);
    }
}

void QWoSshProcess::updateTermSize()
{
    int linecnt = m_term->screenLinesCount();
    int column = m_term->screenColumnsCount();
    QStringList funArgs;
    funArgs << "setwinsize" << QString("%1").arg(column) << QString("%1").arg(linecnt);
    if(m_ipc) {
        qDebug() << funArgs;
        m_writer->write(funArgs);
    }
}

void QWoSshProcess::updatePassword(const QString &pwd)
{
    QWoSshConf::instance()->updatePassword(m_sessionName, pwd);
}

void QWoSshProcess::requestPassword(const QString &prompt, bool echo)
{
    QMetaObject::invokeMethod(m_term, "showPasswordInput", Qt::QueuedConnection, Q_ARG(QString, prompt), Q_ARG(bool, echo));
}

bool QWoSshProcess::eventFilter(QObject *obj, QEvent *ev)
{
    QEvent::Type t = ev->type();
    if (t == QEvent::Resize) {
        QMetaObject::invokeMethod(this, "updateTermSize",Qt::QueuedConnection);
    }else if(t == QWoEvent::EventType) {
        QWoEvent *we = (QWoEvent*)ev;
        QWoEvent::WoEventType t = we->eventType();
        m_idleCount = 0;
        if(t == QWoEvent::BeforeReadStdOut) {            
            if(m_zmodem) {
                QByteArray data = readAllStandardOutput();
                m_zmodem->setCurrentWriteChannel(QProcess::StandardOutput);
                m_zmodem->write(data);
                we->setAccepted(true);
            }else{                
                we->setAccepted(false);
            }
            return true;
        }

        if(t == QWoEvent::BeforeReadStdErr) {
            if(m_zmodem) {
                QByteArray data = readAllStandardError();
                m_zmodem->setCurrentWriteChannel(QProcess::StandardError);
                m_zmodem->write(data);
                we->setAccepted(true);
            }else{
                we->setAccepted(false);
            }
            return true;
        }

        if(t == QWoEvent::AfterReadStdOut) {
            QByteArray data = we->data().toByteArray();
            int code = isZmodemCommand(data);
            if(code == 0) {
                QMetaObject::invokeMethod(this, "onZmodemRecv", Qt::QueuedConnection, Q_ARG(bool, false));
            } else if(code == 1){
                QMetaObject::invokeMethod(this, "onZmodemSend", Qt::QueuedConnection, Q_ARG(bool, false));
            }
            if(m_spy) {
                m_spy->process(data);
            }
            return true;
        }

        if(t == QWoEvent::AfterReadStdErr) {
            return true;
        }

        if(t == QWoEvent::BeforeWriteStdOut) {
            //QByteArray out = we->data().toByteArray();
            //checkCommand(out);
            return true;
        }

        if(t == QWoEvent::BeforeFinish) {
            return true;
        }
        return false;
    }
    return false;
}

void QWoSshProcess::setTermWidget(QTermWidget *widget)
{
    QWoProcess::setTermWidget(widget);
    widget->installEventFilter(this);
    QObject::connect(m_term, SIGNAL(titleChanged()), this, SLOT(onTermTitleChanged()));
}

void QWoSshProcess::prepareContextMenu(QMenu *menu)
{
    if(m_zmodemSend == nullptr) {
        menu->addAction(QIcon(":/qwoterm/resource/skin/find.png"), tr("Find..."), m_term, SLOT(toggleShowSearchBar()), QKeySequence(Qt::CTRL +  Qt::Key_F));
        menu->addAction(QIcon(":/qwoterm/resource/skin/palette.png"), tr("Edit"), this, SLOT(onModifyThisSession()));
        menu->addAction(tr("Duplicate In New Window"), this, SLOT(onDuplicateInNewWindow()));
        m_zmodemSend = menu->addAction(QIcon(":/qwoterm/resource/skin/upload.png"), tr("Zmodem Upload"), this, SLOT(onZmodemSend()));
        m_zmodemRecv = menu->addAction(QIcon(":/qwoterm/resource/skin/download.png"), tr("Zmodem Receive"), this, SLOT(onZmodemRecv()));
        m_zmodemAbort = menu->addAction(tr("Zmoddem Abort"), this, SLOT(onZmodemAbort()), QKeySequence(Qt::CTRL +  Qt::Key_C));
    }
}
