#pragma once

#include "qwowidget.h"

#include <QPointer>
#include <QDir>

class QWoProcess;
class QMenu;
class QWoShellWidgetImpl;
class QWoEmbedCommand;


class QWoShellWidget : public QWoWidget
{
    Q_OBJECT
public:
    explicit QWoShellWidget(QWidget *parent=nullptr);
    virtual ~QWoShellWidget();

    void closeAndDelete();

signals:
    void aboutToClose(QCloseEvent* event);

private
slots:
    void onTimeout();
    void onVerticalSplitView();
    void onHorizontalSplitView();
    void onCloseThisSession();
private:
    void contextMenuEvent(QContextMenuEvent *event);
private:
    void splitWidget(bool vertical);
private:
    friend class QWoShellWidgetImpl;
    QPointer<QMenu> m_menu;
    bool m_bexit;
    bool m_bScrollToEnd;
};
