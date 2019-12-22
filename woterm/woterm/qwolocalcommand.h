#pragma once

#include "qworesult.h"

#include <QObject>
#include <QPointer>
#include <QScriptValue>
#include <QVariantMap>
#include <QScriptable>
#include <QDir>

class QWoLog;
class QEventLoop;

class QWoLocalCommand : public QObject
{
    Q_OBJECT
public:
    QWoLocalCommand(const QString& path, QWoLog *log, QObject *parent=nullptr);
    ~QWoLocalCommand();

    Q_INVOKABLE QString scriptDirectory() const;
    Q_INVOKABLE QString pwd();
    Q_INVOKABLE QString nativeSeperator() const;
    Q_INVOKABLE bool isFile(const QString& path);
    Q_INVOKABLE QString toAbsolutePath(const QString& path) const;
    Q_INVOKABLE bool isAbsolutePath(const QString& path) const;
    Q_INVOKABLE QString cleanPath(const QString& path) const;
    Q_INVOKABLE QString toNativePath(const QString& path) const;
    Q_INVOKABLE QStringList ls(const QString& path);
    Q_INVOKABLE bool cd(const QString& path);
    Q_INVOKABLE bool exist(const QString& path);
    Q_INVOKABLE bool mkpath(const QString& path);
    Q_INVOKABLE bool rmpath(const QString& path);
    Q_INVOKABLE void echo(const QString& msg);
    Q_INVOKABLE QWoResult fileContent(const QString& path);
    Q_INVOKABLE QWoResult runScriptFile(const QString& file);
private:
    bool wait(int *why = nullptr, int ms = 30 * 60 * 1000);
signals:
    void dataReceived(const QByteArray& data);
private slots:
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onFinished(int code);
private:
    void showCommand(const QString& cmd);
    void showResult(const QString& msg);

private:
    QPointer<QEventLoop> m_eventLoop;
    QPointer<QWoLog> m_log;
    QDir m_dir;
    const QString m_pathScript;
};

