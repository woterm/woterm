#pragma once

#include "ipchelper.h"

#include <QObject>
#include <QMap>
#include <QPointer>
#include <QScriptValue>

class QProcess;
class QEventLoop;
class QLocalServer;
class QLocalSocket;
class QWoCmdSpy;

class QWoShellProcess : public QObject
{
    Q_OBJECT
public:
    explicit QWoShellProcess(QString target, QObject *parent);
    ~QWoShellProcess();

    void stop();
    bool login();    
    bool execCommand(const QString& cmd);
    bool sudo(const QString& pwd);
    bool su(const QString& user, const QString&pwd);
    bool upload(const QString &file);
    bool download(const QString& remote, const QString& local);
    void input(const QString& pwd);
    int exitCode();
    QString cmd() const;
    QStringList output() const;
    QString target() const;
private:
signals:
    void dataReceived(const QByteArray& data);
    void logArrived(const QString& level, const QString&msg);
    void dataSend(const QByteArray& data);
    void passwordRequest(const QString& prompt, bool echo);

private slots:
    void onNewConnection();
    void onClientError();
    void onClientDisconnected();
    void onClientReadyRead();
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onFinished(int code);
    void onInputArrived();
    void onApplicationKeypad(bool state);

    void onZmodemReadyReadStandardOutput();
    void onZmodemReadyReadStandardError();
    void onZmodemFinished(int code);

private:
    void echoPong();
    void updateTermSize();
    void updatePassword(const QString& pwd);
    void requestPassword(const QString& prompt, bool echo);

    void logWarning(const QString& msg);
    void logInfo(const QString& msg);
    void logError(const QString& msg);
private:
    bool wait(int *why = nullptr, int ms = 30 * 60 * 1000);
    void sleep(int ms);
    bool waitInput();
    bool seperator();
    int extractExitCode();
    bool setFailure();
    bool setOk();
    bool setOutput(int code, const QStringList& out);
private:
    friend class Smart;
    QPointer<QLocalServer> m_server;
    QPointer<QLocalSocket> m_ipc;
    QPointer<FunArgReader> m_reader;
    QPointer<FunArgWriter> m_writer;
    QPointer<QProcess> m_process;
    QPointer<QProcess> m_zmodem;
    QPointer<QEventLoop> m_eventLoop;
    QPointer<QWoCmdSpy> m_spy;
    QString m_target;
    QString m_cmd;
    QStringList m_cmdOutput;
    int m_cmdExitCode;
};

