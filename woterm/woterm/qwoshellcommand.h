#pragma once

#include "qworesult.h"

#include <QObject>
#include <QMap>
#include <QPointer>
#include <QScriptValue>
#include <QVariantMap>
#include <QScriptable>


class QWoProcess;
class QProcess;
class QEventLoop;
class QWoShellProcess;
class QWoLog;

class QWoShellCommand : public QObject
{
    Q_OBJECT
public:
    QWoShellCommand(QWoLog *log, QObject *parent=nullptr);
    ~QWoShellCommand();
    void stop();

public:
    Q_INVOKABLE QWoResult login(const QStringList& hosts);
    Q_INVOKABLE QWoResult execCommand(const QString& cmd, const QString& host="");
    Q_INVOKABLE QWoResult sudo(const QString& pwd="", const QString& host="");
    Q_INVOKABLE QWoResult su(const QString& user, const QString& pwd="", const QString& host="");    
    Q_INVOKABLE QWoResult upload(const QString& file);
    Q_INVOKABLE QWoResult download(const QString& remote, const QString& local);
    Q_INVOKABLE void sleep(int ms);
private:
    void cleanAll();
    void showSeperator(const QString& host);
    void showError(const QString& msg);
    void showCommandFailure(const QString&cmd, const QString& host);
    void showOk(const QString& msg);
private:
signals:
    void dataReceived(const QByteArray& data);
    void passwordRequest(const QString& prompt, bool echo);
private slots:
    void onPasswordRequest(const QString& prompt, bool echo);
    void onPasswordInput(const QString& pwd);
    void onLogArrived(const QString& level, const QString& msg);
    void onDataReceived(const QByteArray& data);

private:
    QList<QPointer<QWoShellProcess>> m_all;
    QPointer<QWoShellProcess> m_current;
    QPointer<QWoLog> m_log;
};

