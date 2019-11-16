#include "qwohostinfolist.h"
#include "ui_qwohostinfolist.h"

QWoHostInfoList::QWoHostInfoList(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QWoHostInfoList)
{
    Qt::WindowFlags flags = windowFlags();
    setWindowFlags(flags &~Qt::WindowContextHelpButtonHint);
    ui->setupUi(this);
}

QWoHostInfoList::~QWoHostInfoList()
{
    delete ui;
}
