#pragma once

#include <QWidget>

#include <QPointer>

class QWoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QWoWidget(QWidget *parent=nullptr);
signals:
    void aboutToClose(QCloseEvent* event);
private:
    void closeEvent(QCloseEvent *event);
};
