#pragma once

#include "qwoglobal.h"

#include <QMainWindow>
#include <QPointer>
#include <QDockWidget>

class QMenuBar;
class QVBoxLayout;
class QTabBar;
class QToolBar;
class QWoShower;
class QWoSessionList;
class QHttpClient;
class QWoCommandHistoryForm;

namespace Ui {
class QWoMainWindow;
}


class QWoMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit QWoMainWindow(QWidget *parent=nullptr);
    virtual ~QWoMainWindow();
    static QWoMainWindow *instance();
    static QWoCommandHistoryForm *historyForm();
    void setHistoryFormVisible(bool on);

protected:
    void closeEvent(QCloseEvent *event);
private slots:
    void onNewTerm();
    void onOpenTerm();
    void onLayout();
    void onEditConfig();
    void onSessionReadyToConnect(const QString& target);
    void onSessionBatchToConnect(const QStringList& targets,bool samepage);
    void onProcessStartCheck();
    void onAppStart();
    void onVersionCheck(int code, const QByteArray& body);
    void onShouldAppExit();

private slots:
    void onActionNewTriggered();
    void onActionOpenTriggered();
    void onActionDisconnectTriggered();
    void onActionReconnectTriggered();
    void onActionReconnectAllTriggered();
    void onActionImportTriggered();
    void onActionExportTriggered();
    void onActionSaveTriggered();
    void onActionTransferTriggered();
    void onActionLogTriggered();
    void onActionExitTriggered();
    void onActionConfigDefaultTriggered();
    void onActionSettingTriggered();
    void onActionFindTriggered();
    void onActionAboutTriggered();
    void onActionHelpTriggered();
    void onActionScriptRunTriggered();
    void onActionSshKeyManageTriggered();

private:
    void initMenuBar();
    void initToolBar();
    void initStatusBar();
    void restoreLastState();
    void saveLastState();

private:
    Ui::QWoMainWindow *ui;
    QPointer<QTabBar> m_tab;
    QPointer<QWoSessionList> m_sessions;
    QPointer<QDockWidget> m_sessionDock;
    QPointer<QWoCommandHistoryForm> m_historyForm;
    QPointer<QDockWidget> m_historyDock;
    QPointer<QWoShower> m_shower;
    QPointer<QHttpClient> m_httpClient;
};
