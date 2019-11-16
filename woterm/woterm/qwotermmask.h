#ifndef QWOTERMMASK_H
#define QWOTERMMASK_H

#include "qwowidget.h"

namespace Ui {
class QWoTermMask;
}

class QWoTermMask : public QWoWidget
{
    Q_OBJECT

public:
    explicit QWoTermMask(QWidget *parent = nullptr);
    ~QWoTermMask();

signals:
    void reconnect();

private slots:
    void onReconnect();

private:
    void paintEvent(QPaintEvent* paint);
private:
    Ui::QWoTermMask *ui;
};

#endif // QWOTERMMASK_H
