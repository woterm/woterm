#include "qwoidentifyinfomation.h"
#include "qwosetting.h"
#include "qwoutils.h"

#include <QEventLoop>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QCryptographicHash>
#include <QProcess>
#include <QCoreApplication>
#include <QTimer>

QWoIdentifyInfomation::QWoIdentifyInfomation(QObject *parent) :
    QObject(parent)
{

}

void QWoIdentifyInfomation::onReadyReadStandardOutput()
{
    QProcess *proc = qobject_cast<QProcess*>(sender());
    QByteArray buf = proc->property("stdout").toByteArray();
    buf.append(proc->readAllStandardOutput());
    proc->setProperty("stdout", buf);
}

void QWoIdentifyInfomation::onReadyReadStandardError()
{
    QProcess *proc = qobject_cast<QProcess*>(sender());
    QByteArray buf = proc->property("stderr").toByteArray();
    buf.append(proc->readAllStandardError());
    proc->setProperty("stderr", buf);
}

void QWoIdentifyInfomation::onFinished(int code)
{
    m_eventLoop->exit(code);
}

QMap<QString, QString> QWoIdentifyInfomation::identifyFileGet(const QString &file)
{
    char fingure[] = "--fingerprint--:";
    int fingureLength=sizeof(fingure) / sizeof(char) - 1;
    char pubickey[] = "--publickey--:";
    int publickeyLength = sizeof(pubickey) / sizeof(char) - 1;

    QStringList args;
    args.append("-l");
    args.append("-y");
    args.append("-f");
    args.append(file);
    QStringList envs;
    envs.push_back(QString("WOTERM_KEYGEN_ACTION=%1").arg("get"));
    QString output = runProcess(args, envs);
    QMap<QString, QString> infos;
    if(output.isEmpty()) {
        return infos;
    }
    QStringList lines = output.split("\n");
    for(int i = 0; i < lines.length(); i++) {
        QString line = lines.at(i);
        qDebug() << "line:" << line;
        QString type="other";
        if(line.startsWith(fingure)) {
            line = line.remove(0, fingureLength);
            type = "fingureprint";
        }else if(line.startsWith(pubickey)) {
            line.remove(0, publickeyLength);
            type = "publickey";
        }
        QStringList blocks = line.split("]");
        for(int i = 0; i < blocks.length(); i++) {
            QString block = blocks.at(i);
            int idx = block.indexOf('[')+1;
            if(idx >= 1) {
                block = block.mid(idx);
                idx = block.indexOf(':');
                QString name = block.left(idx);
                QString value = block.mid(idx+1);
                infos.insert(type+"."+name, value);
            }
        }
    }
    return infos;
}

bool QWoIdentifyInfomation::identifyFileSet(const QString &file, const QString &name)
{
    QStringList args;
    args.append("-t");
    args.append("rsa");
    args.append("-C");
    args.append(name);
    args.append("-P");
    args.append("");
    args.append("-f");
    args.append(file);
    QStringList envs;
    QString output = runProcess(args, envs);
    return !output.isEmpty();
}

bool QWoIdentifyInfomation::identifyInfomation(const QString &file, IdentifyInfo *pinfo)
{
    QMap<QString, QString> info = identifyFileGet(file);
    if(info.isEmpty() || pinfo == nullptr) {
        return false;
    }
    QFileInfo fi(file);
    QStringList cols;
    QString name = info.value("fingureprint.comment");
    QString tmp = QDir::cleanPath(file);
    QString path = QDir::cleanPath(QWoSetting::identifyFilePath());
    if(tmp.startsWith(path)) {
        name = QWoUtils::pathToName(fi.baseName());
    }if(name.isEmpty()){
        name = fi.baseName();
    }
    QString pubkey = info.value("publickey.public-key");
    QString type = info.value("publickey.ssh-name");
    pinfo->name = name;
    pinfo->key = pubkey;
    pinfo->type = type;
    pinfo->path = file;
    QString hash = QCryptographicHash::hash(pubkey.toUtf8(), QCryptographicHash::Md5).toHex();
    pinfo->fingureprint = hash;
    return true;
}

QMap<QString, IdentifyInfo> QWoIdentifyInfomation::listAll()
{
    QDir dir(QWoSetting::identifyFilePath());
    QStringList items = dir.entryList(QDir::Files);
    QMap<QString, IdentifyInfo> all;
    QWoIdentifyInfomation identify;
    for(int i = 0; i < items.length(); i++) {
        IdentifyInfo info;
        if(!identify.identifyInfomation(dir.path() + "/" + items.at(i), &info)) {
            continue;
        }
        all.insert(info.fingureprint, info);
    }
    return all;
}

QString QWoIdentifyInfomation::runProcess(const QStringList &args, const QStringList &envs)
{
    QString keygen = QCoreApplication::applicationDirPath() + "/ssh-keygen";
#ifdef Q_OS_WIN
    keygen += ".exe";
#endif
    QProcess proc(this);
    QObject::connect(&proc, SIGNAL(readyReadStandardOutput()), this, SLOT(onReadyReadStandardOutput()));
    QObject::connect(&proc, SIGNAL(readyReadStandardError()), this, SLOT(onReadyReadStandardError()));
    QObject::connect(&proc, SIGNAL(finished(int)), this, SLOT(onFinished(int)));
    proc.setEnvironment(envs);
    proc.start(keygen, args);
    wait(3000 * 100);
    int code = proc.exitCode();
    if( code == 0 ){
        QByteArray buf = proc.property("stdout").toByteArray();
        qDebug() << buf;
        return buf;
    }
    return "";
}

bool QWoIdentifyInfomation::wait(int ms, int *why)
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
    int code = loop.exec();
    if(why != nullptr) {
        *why = code;
    }
    if(code != 0) {
        return false;
    }
    return true;
}
