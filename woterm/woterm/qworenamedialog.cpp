#include "qworenamedialog.h"
#include "ui_qworenamedialog.h"

QString QWoRenameDialog::open(const QString &name, QWidget *parent)
{
    QWoRenameDialog dlg(name, parent);
    dlg.exec();
    return dlg.result();
}

QWoRenameDialog::QWoRenameDialog(const QString &name, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QWoRenameDialog)
{
    Qt::WindowFlags flags = windowFlags();
    setWindowFlags(flags &~Qt::WindowContextHelpButtonHint);

    setWindowTitle(tr("Rename"));

    ui->setupUi(this);

    ui->name->setText(name);
    QObject::connect(ui->btnSave, SIGNAL(clicked()), this, SLOT(onButtonSaveClicked()));
    QObject::connect(ui->btnCancel, SIGNAL(clicked()), this, SLOT(close()));
}

QWoRenameDialog::~QWoRenameDialog()
{
    delete ui;
}

QString QWoRenameDialog::result() const
{
    return m_result;
}

void QWoRenameDialog::onButtonSaveClicked()
{
    m_result = ui->name->text();
    close();
}
