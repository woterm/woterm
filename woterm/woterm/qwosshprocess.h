#pragma once

#include "qwoglobal.h"
#include "qwoprocess.h"
#include "ipchelper.h"

#include <QPointer>
#include <QLocalSocket>

class QTermWidget;
class QLocalServer;
class QLocalSocket;
class QAction;
class QFileDialog;
class QThread;
class QWoCmdSpy;
class QEventLoop;
class QFile;
class QWoCommandHistoryForm;
class QWoCommandHistoryModel;
class QSortFilterProxyModel;

class QWoSshProcess : public QWoProcess
{
    Q_OBJECT
private:
    enum EInputState {
        Init,
        Ready,
        Running
    };
    struct InputState {
        EInputState state;
        QString title;
        QString command;
    };

public:
    explicit QWoSshProcess(const QString& target, QObject *parent);
    virtual ~QWoSshProcess();

    void triggerKeepAliveCheck();

private slots:
    void onNewConnection();
    void onClientError(QLocalSocket::LocalSocketError socketError);
    void onClientDisconnected();
    void onClientReadyRead();
    void onZmodemSend(bool local=true);
    void onZmodemRecv(bool local=true);
    void onZmodemAbort();
    void onZmodemFinished(int code);
    void onZmodemReadyReadStandardOutput();
    void onZmodemReadyReadStandardError();
    void onTermTitleChanged();
    void onTermGetFocus();
    void onTermLostFocus();
    void onModifyThisSession();
    void onSessionCommandHistory();
    void onDuplicateInNewWindow();
    void onTimeout();
    void onWindowAttributeArrived(int type, const QString& val);
    void onCommandReplay(const HistoryCommand& hc);

private:
    Q_INVOKABLE void echoPong();
    Q_INVOKABLE void updateTermSize();
    Q_INVOKABLE void updatePassword(const QString& pwd);
    void requestPassword(const QString& prompt, bool echo);

private:
    virtual bool eventFilter(QObject *obj, QEvent *ev);
    virtual void setTermWidget(QTermWidget *widget);
    virtual void prepareContextMenu(QMenu *menu);

private:
    QProcess *createZmodem();
    int isZmodemCommand(const QByteArray& data);
    void checkCommand(const QByteArray& data);

private:
    bool checkShellProgram(const QByteArray& name);
    bool wait(int *why = nullptr, int ms = 30 * 60 * 1000);
    bool waitInput();
    int extractExitCode();
    void sleep(int ms);
    void pushToHistory(const QString& command, const QString& other);
    void saveHistory();

private:
    static QWoCommandHistoryModel *get(const QString& name);
private:
    QPointer<QLocalServer> m_server;

    QPointer<QLocalSocket> m_ipc;
    QPointer<FunArgReader> m_reader;
    QPointer<FunArgWriter> m_writer;

    QPointer<QAction> m_zmodemSend;
    QPointer<QAction> m_zmodemRecv;
    QPointer<QAction> m_zmodemAbort;

    QPointer<QProcess> m_zmodem;
    QString m_exeSend;
    QString m_exeRecv;

    QPointer<QWoCmdSpy> m_spy;
    QPointer<QEventLoop> m_eventLoop;

    int m_idleCount;
    int m_idleDuration;

    QByteArray m_prompt;

    InputState m_stateInput;
    QList<HistoryCommand> m_historyCommands;
    int m_historyLeftSaveCount;

    QPointer<QWoCommandHistoryForm> m_historyForm;
    QPointer<QWoCommandHistoryModel> m_model;
    QPointer<QSortFilterProxyModel> m_proxyModel;
};
