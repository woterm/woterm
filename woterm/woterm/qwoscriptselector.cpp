#include "qwoscriptselector.h"
#include "ui_qwoscriptselector.h"

#include "qwosetting.h"

#include <QDebug>
#include <QFileDialog>
#include <QApplication>
#include <QMessageBox>
#include <QTextBrowser>
#include <QScrollBar>

QWoScriptSelector::QWoScriptSelector(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QWoScriptSelector)
{
    ui->setupUi(this);
    setAutoFillBackground(true);

    ui->log->setWordWrapMode(QTextOption::WordWrap);
    ui->log->setOpenLinks(false);
    ui->log->setOpenExternalLinks(true);
    ui->log->setReadOnly(true);
    ui->log->setTextInteractionFlags( Qt::TextBrowserInteraction | Qt::TextEditorInteraction );

    QPalette pal;
    pal.setColor(QPalette::Background, Qt::gray);
    pal.setColor(QPalette::Window, Qt::gray);
    setPalette(pal);

    QObject::connect(ui->btnBrowser, SIGNAL(clicked()), this, SLOT(onBrowserScriptFile()));
    QObject::connect(ui->btnReload, SIGNAL(clicked()), this, SLOT(onReloadScriptFile()));
    QObject::connect(ui->btnDebug, SIGNAL(clicked()), this, SLOT(onBtnDebugClicked()));
    QObject::connect(ui->btnRun, SIGNAL(clicked()), this, SLOT(onBtnRunClicked()));
    QObject::connect(ui->btnStop, SIGNAL(clicked()), this, SIGNAL(readyStop()));
    QObject::connect(ui->task, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(onCurrentIndexChanged(const QString&)));

    QString last = QWoSetting::lastJsLoadPath();
    if(last.isEmpty()) {
        last = QDir::cleanPath(QWoSetting::examplePath() + "/simple/hello.conf");
    }
    if(!last.isEmpty()) {
        ui->scriptFile->setText(last);
        reinit();
    }
}

QWoScriptSelector::~QWoScriptSelector()
{
    delete ui;
}

void QWoScriptSelector::onBrowserScriptFile()
{
    QString last = QWoSetting::lastJsLoadPath();
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), last, tr("ConfigFile(*.conf)"));
    qDebug() << "fileName" << fileName;
    if(!fileName.isEmpty()) {
        fileName = QDir::toNativeSeparators(fileName);
        QWoSetting::setLastJsLoadPath(fileName);
        ui->scriptFile->setText(fileName);
        reinit();
    }
}

void QWoScriptSelector::onReloadScriptFile()
{
    reinit();
}

void QWoScriptSelector::onBtnRunClicked()
{
    Task t = reset();
    if(t.name.isEmpty()) {
        QMessageBox::warning(this, tr("warn"), tr("should be not empty"));
        return;
    }
    emit readyRun(m_fullPath, t.files, t.entry, t.args);
}

void QWoScriptSelector::onBtnDebugClicked()
{
    Task t = reset();
    if(t.name.isEmpty()) {
        QMessageBox::warning(this, tr("warn"), tr("should be not empty"));
        return;
    }
    emit readyDebug(m_fullPath, t.files, t.entry, t.args);
}

void QWoScriptSelector::onLogArrived(const QString &level, const QString &msg)
{
    QTextBrowser *log = ui->log;

    QTextCursor tc = log->textCursor();
    QTextCharFormat fmt;

    tc.movePosition(QTextCursor::End);

    QTextBlockFormat block;
    block.setTopMargin(0);
    block.setBottomMargin(0);
    block.setLeftMargin(0);
    block.setRightMargin(0);
    block.setNonBreakableLines(true);
    tc.insertBlock(block);
    if(level == "info") {
        fmt.setForeground(Qt::black);
    }else if(level == "warning") {
        fmt.setForeground(Qt::yellow);
    }else {
        fmt.setForeground(Qt::red);
    }
    tc.insertText("\n");
    tc.insertText(QString("......%1.....\n").arg(level), fmt);
    tc.insertText(msg, fmt);

    QScrollBar *vbar = log->verticalScrollBar();
    int toY = vbar->maximum();
    vbar->setSliderPosition(toY);
}

void QWoScriptSelector::onCurrentIndexChanged(const QString &txt)
{
    Task t = m_tasks.value(txt);
    ui->entry->setText(t.entry);
    ui->args->setText(t.args.join(","));
}

QWoScriptSelector::Task QWoScriptSelector::reset()
{
    QString name = ui->task->currentText();
    Task t = m_tasks.value(name);
    t.entry = ui->entry->text().trimmed();
    QString targs = ui->args->text().trimmed();
    if(!targs.isEmpty()){
        QStringList args = targs.split(",");
        for(int i = 0; i < args.length(); i++) {
            t.args << args.at(i).trimmed();
        }
    }
    if(t.entry.isEmpty()) {
        t.entry = "main";
    }
    return t;
}

bool QWoScriptSelector::parse()
{
    QString fullName = QDir::toNativeSeparators(QDir::cleanPath(ui->scriptFile->text()));
    QFile file(fullName);
    if(!file.open(QFile::ReadOnly)) {
        //QMessageBox::warning(this, "warn", "file is not exist");
        return false;
    }
    int idx = fullName.lastIndexOf(QDir::separator());
    m_fullPath = fullName.mid(0, idx);
    QList<Task> tasks;
    while(!file.atEnd()) {
        QByteArray line = file.readLine();
        line = line.trimmed();
        if(line.startsWith("Task ")) {
            QString name = line.mid(5);
            name = name.simplified();
            Task t;
            t.name = name;
            t.entry = "main";
            tasks.push_back(t);
        }else{
            Task& t = tasks.last();
            if(line.startsWith("Files ")) {
                QByteArrayList files = line.mid(6).split(',');
                for(int i = 0; i < files.count(); i++) {
                    QString tmp = QDir::cleanPath(m_fullPath + "/./"+files.at(i).trimmed());                    
                    if(!QFile::exists(tmp)) {
                        QMessageBox::warning(this, tr("warn"), QString("can't find the file:%1").arg(tmp));
                        return false;
                    }
                    t.files.append(tmp);
                }
            }else if(line.startsWith("Entry ")) {
                t.entry = line.mid(6).trimmed();
            }else if(line.startsWith("Arguments ")){
                QByteArrayList args = line.mid(10).split(',');
                for(int i = 0; i < args.count(); i++) {
                    t.args.append(args.at(i).trimmed());
                }
            }
        }
    }
    m_tasks.clear();
    for(int i = 0; i < tasks.count(); i++){
        Task t = tasks.at(i);        
        m_tasks.insert(t.name, t);
    }
    return !m_tasks.isEmpty();
}

void QWoScriptSelector::reinit()
{
    if(!parse()) {
        return;
    }
    QStringList items = m_tasks.keys();
    ui->task->clear();
    ui->task->addItems(items);
    ui->task->setCurrentIndex(0);
    Task t = m_tasks.value(items.first());
    ui->entry->setText(t.entry);
    ui->args->setText(t.args.join(","));
}
