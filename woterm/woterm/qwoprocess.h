#pragma once

#include <QPointer>
#include <QProcess>

class QTermWidget;
class QWoTermWidget;
class QMenu;

class QWoProcess : public QObject
{
    Q_OBJECT
public:
    explicit QWoProcess(QObject *parent=nullptr);
    virtual ~QWoProcess();
    QString program() const;
    void setProgram(const QString &program);

    QStringList arguments() const;
    void setArguments(const QStringList & arguments);

    QStringList environment() const;
    void setEnvironment(const QStringList& env);

    void setWorkingDirectory(const QString &dir);

    virtual void start();
    virtual QByteArray readAllStandardOutput();
    virtual QByteArray readAllStandardError();
    virtual void write(const QByteArray& data);
    virtual void writeError(const QByteArray& data);

    void closeReadChannel(QProcess::ProcessChannel channel);
    void closeWriteChannel();

    bool waitForBytesWritten(int msecs = 30000);
    void enableDebugConsole(bool on);

    QTermWidget *termWidget();

    Q_INVOKABLE void close();
    Q_INVOKABLE void terminate();
    Q_INVOKABLE void kill();
private:
signals:
    void readyReadStandardOutput();
    void readyReadStandardError();
    void finished(int code);

protected:    
    virtual void setTermWidget(QTermWidget *widget);
    virtual void prepareContextMenu(QMenu *menu);

private slots:
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onFinished(int code);

protected:
    QPointer<QTermWidget> m_term;
    QPointer<QProcess> m_process;

    friend class QWoTermWidget;
};
