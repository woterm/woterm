#include "qwopasswordinput.h"
#include "ui_qwopasswordinput.h"

#include <QPainter>
#include <QMessageBox>

QWoPasswordInput::QWoPasswordInput(QWidget *parent) :
    QWoWidget(parent),
    ui(new Ui::QWoPasswordInput)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_TranslucentBackground, true);

    ui->setupUi(this);

    QObject::connect(ui->visible, SIGNAL(clicked(bool)), this, SLOT(onPasswordVisible(bool)));
    QObject::connect(ui->btnFinish, SIGNAL(clicked()), this, SLOT(onClose()));
    QObject::connect(ui->password, SIGNAL(returnPressed()), this, SLOT(onClose()));

    ui->inputArea->setAutoFillBackground(true);
    QPalette pal;
    pal.setColor(QPalette::Background, Qt::lightGray);
    pal.setColor(QPalette::Window, Qt::lightGray);
    ui->inputArea->setAutoFillBackground(true);
    ui->inputArea->setPalette(pal);
    setFocusPolicy(Qt::StrongFocus);
    setFocus();
    ui->password->setFocusPolicy(Qt::StrongFocus);
    ui->password->setFocus();
}

QWoPasswordInput::~QWoPasswordInput()
{
    delete ui;
}

void QWoPasswordInput::reset(const QString &prompt, bool echo)
{
    ui->tip->setText(prompt);
    ui->save->setChecked(false);
    ui->save->setVisible(!echo);
    ui->visible->setVisible(!echo);
    ui->visible->setChecked(echo);
    ui->password->clear();
    ui->password->setEchoMode(echo ? QLineEdit::Normal : QLineEdit::Password);
}

void QWoPasswordInput::onPasswordVisible(bool checked)
{
    ui->password->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
}

void QWoPasswordInput::onClose()
{
    QString pass = ui->password->text();
    if(pass.isEmpty()) {
        QMessageBox::StandardButton btn = QMessageBox::warning(this, tr("Tip"), tr("The Password is Empty, continue to finish?"), QMessageBox::Ok|QMessageBox::No);
        if(btn == QMessageBox::No) {
            return ;
        }
    }
    hide();
    emit result(pass, ui->save->isChecked());
}

void QWoPasswordInput::paintEvent(QPaintEvent *paint)
{
    QPainter p(this);
    p.setBrush(QColor(128,128,128,128));
    p.drawRect(0,0, width(), height());
}
