#include "qwotermmask.h"
#include "ui_qwotermmask.h"

#include <QPainter>
#include <QDebug>

QWoTermMask::QWoTermMask(QWidget *parent) :
    QWoWidget(parent),
    ui(new Ui::QWoTermMask)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_TranslucentBackground, true);
    QObject::connect(ui->btnClose, SIGNAL(clicked()), this, SLOT(close()));
    QObject::connect(ui->btnReconnect, SIGNAL(clicked()), this, SLOT(onReconnect()));
}

QWoTermMask::~QWoTermMask()
{
    delete ui;
}

void QWoTermMask::onReconnect()
{
    hide();
    emit reconnect();
}

void QWoTermMask::paintEvent(QPaintEvent *paint)
{
    QPainter p(this);

    p.setBrush(QColor(128,128,128,128));
    p.drawRect(0,0, width(), height());
}
