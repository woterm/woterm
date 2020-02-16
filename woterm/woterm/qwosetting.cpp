#include "qwosetting.h"

#include<QSettings>
#include<QApplication>
#include<QDir>
#include<QMessageBox>

QSettings *qSettings()
{
    static QString path = QDir::cleanPath(QWoSetting::applicationDataPath()+"/woterm.ini");
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

QString QWoSetting::applicationDataPath()
{
    static QString userDataPath;
    if(userDataPath.isEmpty()){
        QString flag = privateDataPath()+"/.local";
        QString dataPath = QDir::homePath() + "/.woterm/";
        if(QFileInfo::exists(flag)) {
            dataPath = privateDataPath() + "/../data/";
        }
        userDataPath = QDir::cleanPath(dataPath);
        QFileInfo fi(userDataPath);
        //QMessageBox::warning(nullptr, userDataPath, userDataPath);
        if(!fi.exists() || !fi.isDir()) {
            QDir dir;
            dir.rmpath(userDataPath);
            dir.mkpath(userDataPath);

        }
    }
    return userDataPath;
}

QString QWoSetting::identifyFilePath()
{
    return specialFilePath("identify");
}

QString QWoSetting::historyFilePath()
{
    return specialFilePath("history");
}

QString QWoSetting::specialFilePath(const QString &name)
{
    QString path = QDir::cleanPath(applicationDataPath() + "/" + name);
    QFileInfo fi(path);
    if(!fi.exists() || !fi.isDir()) {
        QDir dir;
        dir.rmpath(path);
        dir.mkpath(path);
    }
    return path;
}

QString QWoSetting::examplePath()
{
    static QString path = QDir::cleanPath(QApplication::applicationDirPath()+"/../example/");
    return QDir::cleanPath(path);
}

QString QWoSetting::privateDataPath()
{
    static QString path = QDir::cleanPath(QApplication::applicationDirPath()+"/../private/");
    return path;
}

QString QWoSetting::privateJsCorePath()
{
    static QString path = QDir::cleanPath(privateDataPath() + "/jscore/");
    return path;
}

QString QWoSetting::privateColorSchemaPath()
{
    static QString path = QDir::cleanPath(privateDataPath() + "/color-schemes/");
    return path;
}

QString QWoSetting::privateKeyboardLayoutPath()
{
    static QString path = QDir::cleanPath(privateDataPath() + "/kb-layouts/");
    return path;
}

QString QWoSetting::privateTranslationPath()
{
    static QString path = QDir::cleanPath(privateDataPath() + "/translations/");
    return path;
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
        path = QDir::cleanPath(QWoSetting::applicationDataPath() + "/servers.conf");
        path = QDir::toNativeSeparators(path);
        return path;
    }
    return path;
}

QString QWoSetting::lastJsLoadPath()
{
    QString path;
    path = QWoSetting::value("js/last").toString();
    if(!QFile::exists(path)) {
        return "";
    }
    return path;
}

void QWoSetting::setLastJsLoadPath(const QString &path)
{
    QWoSetting::setValue("js/last", path);
}
