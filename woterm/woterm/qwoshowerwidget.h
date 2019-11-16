#pragma once

#include "qwowidget.h"

#include <QPointer>

class QMenu;

class QWoShowerWidget : public QWoWidget
{
    Q_OBJECT
public:
    explicit QWoShowerWidget(QWidget *parent=nullptr);


protected:
    virtual bool handleTabMouseEvent(QMouseEvent *ev);
    virtual void handleTabContextMenu(QMenu *menu);
private:
    friend class QWoShower;
};
