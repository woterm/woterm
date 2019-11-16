#include "qwosetting.h"

#include<QSettings>
#include<QApplication>
#include<QDir>

QSettings *qSettings()
{
    static QString path = QDir::cleanPath(QApplication::applicationDirPath()+"/../opt/woterm.ini");
    static QSettings setting(path, QSettings::IniFormat);
    return &setting;
}

void QWoSetting::setValue(const QString &key, const QVariant &v)
{
    qSettings()->setValue(key, v);
    qSettings()->sync();
}

QVariant QWoSetting::value(const QString &key)
{
    return qSettings()->value(key);
}

QString QWoSetting::zmodemSZPath()
{
    QString path;
    path = QWoSetting::value("zmodem/sz").toString();
    if(!QFile::exists(path)) {
        path = QDir::cleanPath(QApplication::applicationDirPath() + "/sz");
#ifdef Q_OS_WIN
        path.append(".exe");
#endif
        path = QDir::toNativeSeparators(path);
        if(!QFile::exists(path)){
            return "";
        }
    }
    return path;
}

QString QWoSetting::zmodemRZPath()
{
    QString path;
    path = QWoSetting::value("zmodem/rz").toString();
    if(!QFile::exists(path)) {
        path = QDir::cleanPath(QApplication::applicationDirPath() + "/rz");
#ifdef Q_OS_WIN
        path.append(".exe");
#endif
        path = QDir::toNativeSeparators(path);
        if(!QFile::exists(path)){
            return "";
        }
    }
    return path;
}

QString QWoSetting::sshProgramPath()
{
    QString path;
    path = QWoSetting::value("ssh/program").toString();
    if(!QFile::exists(path)) {
        path = QDir::cleanPath(QApplication::applicationDirPath() + "/ssh");
#ifdef Q_OS_WIN
        path.append(".exe");
#endif
        path = QDir::toNativeSeparators(path);
        if(!QFile::exists(path)){
            return "";
        }
    }
    return path;
}

QString QWoSetting::sshServerListPath()
{
    QString path;
    path = QWoSetting::value("ssh/serverList").toString();
    if(!QFile::exists(path)) {
        path = QDir::cleanPath(QApplication::applicationDirPath() + "/../opt/servers.conf");
        path = QDir::toNativeSeparators(path);
        return path;
    }
    return path;
}

QString QWoSetting::ipcProgramPath()
{
    QString path;
    path = QWoSetting::value("ipc/program").toString();
    if(!QFile::exists(path)) {
        path = QDir::cleanPath(QApplication::applicationDirPath() + "/woipc");
#ifdef Q_OS_WIN
        path.append(".dll");
#endif
        path = QDir::toNativeSeparators(path);
        if(!QFile::exists(path)){
            return "";
        }
    }
    return path;
}

QString QWoSetting::shellProgramPath()
{
    QString path;
    path = QWoSetting::value("shell/program").toString();
    if(!QFile::exists(path)) {
#ifdef Q_OS_WIN
        QString cmd = qEnvironmentVariable("ComSpec");
        if(QFile::exists(cmd)) {
            return cmd;
        }
        QString path = qEnvironmentVariable("path");
        QStringList paths = path.split(";");
        for(int i = 0; i < paths.count(); i++) {
            QString cmd = paths.at(i) + "/cmd.exe";
            if(QFile::exists(cmd)) {
                return cmd;
            }
        }
        return "";
#endif
    }
    return path;
}

QString QWoSetting::utilsCommandPath()
{
    QString path;
    path = QWoSetting::value("shell/utils").toString();
    if(!QFile::exists(path)) {
        path = QDir::cleanPath(QApplication::applicationDirPath() + "/../unxutils");
        path = QDir::toNativeSeparators(path);
        if(!QFile::exists(path)){
            return "";
        }
    }
    return path;
}

QStringList QWoSetting::utilsCommandList()
{
    QString path = utilsCommandPath();
    if(path.isEmpty()) {
        return QStringList();
    }
    QStringList cmds;
    QDir dir(path);
    QStringList exes = dir.entryList(QDir::Executable|QDir::Files);
    for(int i = 0; i < exes.count(); i++) {
        QString fullName = QDir::toNativeSeparators(dir.path() + "/" + exes.at(i));
        cmds << fullName;
    }
    return cmds;
}
