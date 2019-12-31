#pragma once

#include "qwoshowerwidget.h"

#include <QPointer>

class QSplitter;
class QWoTermWidget;

class QWoShellWidgetImpl : public QWoShowerWidget
{
    Q_OBJECT
public:
    explicit QWoShellWidgetImpl(QWidget *parent=nullptr);
    ~QWoShellWidgetImpl();

signals:
    void aboutToClose(QCloseEvent* event);
private:
    void closeEvent(QCloseEvent *event);
    void resizeEvent(QResizeEvent *event);
    bool event(QEvent *event);

private slots:
    void onRootSplitterDestroy();

private:
    friend class QWoTermWidget;
    const QString m_target;
    QPointer<QSplitter> m_root;
    QList<QPointer<QWoTermWidget>> m_terms;
};
