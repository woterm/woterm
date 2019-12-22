#ifndef QWOSCRIPTMASTER_H
#define QWOSCRIPTMASTER_H

#include <QThread>
#include <QPointer>

class QTermWidget;
class QScriptEngine;
class QScriptEngineDebugger;
class QWoShellCommand;
class QWoLocalCommand;
class QFile;

class QWoLog : public QObject
{
    Q_OBJECT
public:
signals:
    void logArrived(const QString&level, const QString& msg);
public:
    void log(const QString&level, const QString& msg);
    Q_INVOKABLE void info(const QString& msg);
    Q_INVOKABLE void warn(const QString& msg);
    Q_INVOKABLE void error(const QString& msg);
};

class QProxy : public QObject
{
    Q_OBJECT
public:
    QProxy(QScriptEngine *eng, QWoShellCommand *shell);
private slots:
     void onStop();
private:
     QPointer<QScriptEngine> m_engine;
     QPointer<QWoShellCommand> m_shell;     
};

class QWoScriptMaster : public QThread
{
    Q_OBJECT
public:
    explicit QWoScriptMaster(QTermWidget *term, QObject *parent = nullptr);
    bool start(const QString& fullPath,const QStringList& files, const QString& entry, const QStringList& args, bool debug = false);
    void stop();
    bool isRunning();
    void passwordInput(const QString& pwd);
private:
    virtual void run();
private:
    void showError(const QString& msg);
    void showError(const QStringList& msg);
    void execute(bool debug = false);

signals:
    void error(const QString& msg);
    void error(const QStringList& msg);
    void stopit();
    void logArrived(const QString& level, const QString& msg);
    void passwordRequest(const QString& prompt, bool echo);


private slots:
    void onShowError(const QString& msg);
    void onShowError(const QStringList& msg);
    void onDataReceived(const QByteArray& data);


private:
    QPointer<QTermWidget> m_term;
    QString m_fullPath;
    QStringList m_scriptFiles;
    QString m_entry;
    QStringList m_args;
    QPointer<QWoShellCommand> m_shell;
    QPointer<QWoLocalCommand> m_local;
    QPointer<QScriptEngine> m_engine;
    QPointer<QScriptEngineDebugger> m_debugger;
};

#endif // QWOSCRIPTMASTER_H
