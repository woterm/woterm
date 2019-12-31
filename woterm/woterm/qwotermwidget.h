#pragma once

#include <qtermwidget.h>

#include <QPointer>

class QWoProcess;
class QMenu;
class QWoTermMask;
class QWoPasswordInput;
class QWoTermWidgetImpl;
class QSplitter;
class QLabel;

class QWoTermWidget : public QTermWidget
{
    Q_OBJECT
public:
    explicit QWoTermWidget(QWoProcess *process, QWidget *parent=nullptr);
    virtual ~QWoTermWidget();

    QWoProcess *process();
    void closeAndDelete();

    void splitWidget(const QString& target, bool vertical);

    void triggerPropertyCheck();

signals:
    void aboutToClose(QCloseEvent* event);

private
slots:
    void onTimeout();
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onFinished(int code);
    void onSendData(const QByteArray& buf);
    void onCopyToClipboard();
    void onPasteFromClipboard();
    void onVerticalSplitView();
    void onHorizontalSplitView();
    void onVerticalInviteView();
    void onHorizontalInviteView();
    void onCloseThisSession();
    void onForceToCloseThisSession();
    void onSessionReconnect();
    void onPasswordInputResult(const QString& pass, bool isSave);

private:
    Q_INVOKABLE void showPasswordInput(const QString& prompt, bool echo);
private:
    void contextMenuEvent(QContextMenuEvent *event);
    void closeEvent(QCloseEvent *event);
    void resizeEvent(QResizeEvent *event);
    bool event(QEvent* ev);
    bool eventFilter(QObject *obj, QEvent *event);
private:
    void initTitle();
    void initDefault();
    void initCustom();
    void resetProperty(QVariantMap data);
    void resetTitlePosition();
    void replaceWidget(QSplitter *spliter, int idx, QWidget *widget);

    QWoTermWidgetImpl *findTermImpl();
    void addToTermImpl();
    void removeFromTermImpl();

    void onBroadcastMessage(int type, QVariant msg);
    bool handleWoEvent(QEvent *ev);
private:
    friend class QWoTermWidgetImpl;
    QPointer<QLabel> m_title;
    QPointer<QWoProcess> m_process;
    QPointer<QMenu> m_menu;
    QPointer<QAction> m_copy;
    QPointer<QAction> m_paste;
    QPointer<QWoTermMask> m_mask;
    QPointer<QWoPasswordInput> m_passInput;
    bool m_bexit;
};
