#include "qwowidget.h"

#include <QCloseEvent>


QWoWidget::QWoWidget(QWidget *parent)
    : QWidget (parent)
{

}

void QWoWidget::closeEvent(QCloseEvent *event)
{
    emit aboutToClose(event);
    if(event->isAccepted()) {
        return;
    }
    QWidget::closeEvent(event);
}
