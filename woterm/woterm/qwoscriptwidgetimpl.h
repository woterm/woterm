#pragma once

#include "qwoshowerwidget.h"

#include <QPointer>

class QTermWidget;
class QWoProcess;
class QScriptEngine;
class QScriptEngineDebugger;
class QWoScriptSelector;
class QWoScriptMaster;
class QSplitter;
class QWoPasswordInput;

class QWoScriptWidgetImpl : public QWoShowerWidget
{
    Q_OBJECT
public:
    explicit QWoScriptWidgetImpl(QWidget *parent=nullptr);
    ~QWoScriptWidgetImpl();

signals:
    void aboutToClose(QCloseEvent* event);
private:
    void closeEvent(QCloseEvent *event);
    void resizeEvent(QResizeEvent *event);
    bool event(QEvent *event);
    void init();
    void resetProperty(QVariantMap mdata);
    void showPasswordInput(const QString &prompt, bool echo);
private slots:
    void onReadyScriptRun(const QString& fullPath, const QStringList &scripts, const QString &entry, const QStringList &args);
    void onReadyScriptStop();
    void onReadyScriptDebug(const QString& fullPath, const QStringList &scripts, const QString &entry, const QStringList &args);
    void onPasswordInputResult(const QString& pass, bool isSave);
    void onPasswordRequest(const QString &prompt, bool echo);

private:
    QPointer<QSplitter> m_vspliter;
    QPointer<QTermWidget> m_term;
    QPointer<QWoScriptSelector> m_selector;
    QPointer<QWoScriptMaster> m_master;
    QPointer<QWoPasswordInput> m_passInput;
};
