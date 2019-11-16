#include "qwoaboutdialog.h"
#include "ui_qwoaboutdialog.h"

#include "version.h"

QWoAboutDialog::QWoAboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QWoAboutDialog)
{
    Qt::WindowFlags flags = windowFlags();
    setWindowFlags(flags &~Qt::WindowContextHelpButtonHint);
    ui->setupUi(this);

    QString txt = QString("Current Version: %1").arg(WOTERM_VERSION);
    txt.append("<br>Check New Version: <a href=\"http://www.woterm.com\">http://www.woterm.com</a>");

    ui->desc->setText(txt);
    ui->desc->setOpenExternalLinks(true);
    ui->desc->setWordWrap(true);
    ui->desc->setTextFormat(Qt::RichText);
}

QWoAboutDialog::~QWoAboutDialog()
{
    delete ui;
}
