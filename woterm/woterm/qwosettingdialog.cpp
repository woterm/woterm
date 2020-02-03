#include "qwosettingdialog.h"
#include "ui_qwosettingdialog.h"

QWoSettingDialog::QWoSettingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QWoSettingDialog)
{
    Qt::WindowFlags flags = windowFlags();
    setWindowFlags(flags &~Qt::WindowContextHelpButtonHint);
    ui->setupUi(this);
}

QWoSettingDialog::~QWoSettingDialog()
{
    delete ui;
}
