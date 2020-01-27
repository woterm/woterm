#include "qwoidentifydialog.h"
#include "ui_qwoidentifydialog.h"
#include "qwoutils.h"
#include "qwosetting.h"
#include "qworenamedialog.h"
#include "qwoidentifypublickeydialog.h"

#include <QFileDialog>
#include <QDebug>
#include <QDirModel>
#include <QMessageBox>
#include <QProcess>
#include <QTimer>
#include <QEventLoop>
#include <QCryptographicHash>
#include <QTreeWidget>

#define ROLE_IDENTIFYKEY (Qt::UserRole+2)
#define ROLE_IDENTIFYTYPE (Qt::UserRole+3)

QWoIdentifyDialog::QWoIdentifyDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QWoIdentifyDialog)
{
    Qt::WindowFlags flags = windowFlags();
    setWindowFlags(flags &~Qt::WindowContextHelpButtonHint);    
    ui->setupUi(this);
    setWindowTitle(tr("Identify Manage"));

    QObject::connect(ui->btnCreate, SIGNAL(clicked()), this, SLOT(onButtonCreateClicked()));
    QObject::connect(ui->btnDelete, SIGNAL(clicked()), this, SLOT(onButtonDeleteClicked()));
    QObject::connect(ui->btnExport, SIGNAL(clicked()), this, SLOT(onButtonExportClicked()));
    QObject::connect(ui->btnImport, SIGNAL(clicked()), this, SLOT(onButtonImportClicked()));
    QObject::connect(ui->btnRename, SIGNAL(clicked()), this, SLOT(onButtonRenameClicked()));
    QObject::connect(ui->btnSelect, SIGNAL(clicked()), this, SLOT(onButtonSelectClicked()));
    QObject::connect(ui->btnView, SIGNAL(clicked()), this, SLOT(onButtonViewClicked()));
    QObject::connect(ui->identify, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(onItemDoubleClicked(QTreeWidgetItem*, int)));
    QStringList items;
    items.append(tr("name"));
    items.append(tr("fingerprint"));
    ui->identify->setHeaderLabels(items);

    reload();
}

QWoIdentifyDialog::~QWoIdentifyDialog()
{
    delete ui;
}

QString QWoIdentifyDialog::open(QWidget *parent)
{
    QWoIdentifyDialog dlg(parent);
    dlg.exec();
    return dlg.result();
}


QString QWoIdentifyDialog::result() const
{
    return m_result;
}

void QWoIdentifyDialog::onButtonCreateClicked()
{
    QString name = QWoRenameDialog::open("", this);
    if(name.isEmpty()) {
        QMessageBox::information(this, tr("info"), tr("the name is empty."));
        return;
    }
    QString path = QWoSetting::identifyFilePath() + "/" + nameToPath(name);
    if(!identifyFileSet(path, name)) {
        QMessageBox::information(this, tr("info"), QString(tr("failed to create identify:[%1]")).arg(name));
        return;
    }
    reload();
}

void QWoIdentifyDialog::onButtonImportClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"));
    qDebug() << "fileName" << fileName;
    if(fileName.isEmpty()) {
        return;
    }
    fileName = QDir::toNativeSeparators(fileName);
    IdentifyInfo info;
    if(!identifyInfomation(fileName, &info)) {
        QMessageBox::information(this, tr("info"), tr("the identify's file is bad"));
        return;
    }
    QFile f(fileName);
    f.open(QFile::ReadOnly);
    QByteArray buf = f.readAll();
    QByteArray data = toWotermIdentify(buf);
    f.close();
    QString name = info.name;
    for(int i = 0; i < 10; i++) {
        QString b64Name = nameToPath(name);
        QString dstFile = QWoSetting::identifyFilePath() + "/" + b64Name;
        if(!QFile::exists(dstFile)) {
            info.name = name;
            QFile dst(dstFile);
            if(dst.open(QFile::WriteOnly)) {
                dst.write("woterm:");
                dst.write(data);
            }
            dst.close();
            QStringList cols;
            cols.append(info.name);
            cols.append(info.fingureprint);
            QTreeWidgetItem *item = new QTreeWidgetItem(cols);
            item->setData(0, ROLE_IDENTIFYKEY, info.key);
            item->setData(0, ROLE_IDENTIFYTYPE, info.type);
            ui->identify->addTopLevelItem(item);
            break;
        }
        name = QWoRenameDialog::open(info.name, this);
        if(name.isEmpty()) {
            return;
        }
    }
}

void QWoIdentifyDialog::onButtonExportClicked()
{
    QModelIndex idx = ui->identify->currentIndex();
    if(!idx.isValid()) {
        QMessageBox::information(this, tr("info"), tr("no selection"));
        return;
    }
    QAbstractItemModel *model = ui->identify->model();
    QModelIndex idx2 = model->index(idx.row(), 0);
    QString name = idx2.data().toString();
    QString dstFile = QWoSetting::identifyFilePath() + "/" + nameToPath(name);
    QFile file(dstFile);
    if(!file.open(QFile::ReadOnly)) {
        QMessageBox::information(this, tr("info"), tr("the identify's file is not exist"));
        return;
    }
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"));
    QByteArray buf = toStandardIdentify(file.readAll());
    file.close();
    QFile prv(fileName);
    if(prv.open(QFile::WriteOnly)) {
        prv.write(buf);
        prv.close();

        QString type = idx2.data(ROLE_IDENTIFYTYPE).toString();
        QString key = idx2.data(ROLE_IDENTIFYKEY).toString();
        QString content = type + " " + key + " " + name;

        QFile pub(fileName + ".pub");
        if(pub.open(QFile::WriteOnly)){
            pub.write(content.toUtf8());
            pub.close();
        }
    }
}

void QWoIdentifyDialog::onButtonDeleteClicked()
{
    QModelIndex idx = ui->identify->currentIndex();
    if(!idx.isValid()) {
        QMessageBox::information(this, tr("info"), tr("no selection"));
        return;
    }
    QAbstractItemModel *model = ui->identify->model();
    QModelIndex idx2 = model->index(idx.row(), 0);
    QString name = idx2.data().toString();
    QDir dir(QWoSetting::identifyFilePath());
    if(!dir.remove(nameToPath(name))) {
        QMessageBox::warning(this, tr("Warning"), tr("failed to delete file:%1").arg(name));
        return;
    }
    model->removeRow(idx.row());
}

void QWoIdentifyDialog::onButtonSelectClicked()
{
    QModelIndex idx = ui->identify->currentIndex();
    if(!idx.isValid()) {
        QMessageBox::information(this, tr("info"), tr("no selection"));
        return;
    }
    m_result = idx.data().toString();
    close();
}

void QWoIdentifyDialog::onButtonRenameClicked()
{
    QModelIndex idx = ui->identify->currentIndex();
    if(!idx.isValid()) {
        QMessageBox::information(this, tr("info"), tr("no selection"));
        return;
    }
    QAbstractItemModel *model = ui->identify->model();
    QModelIndex idx2 = model->index(idx.row(), 0);
    QString name = idx2.data().toString();
    QString nameNew = QWoRenameDialog::open(name, this);
    if(nameNew.isEmpty() || name == nameNew) {
        return;
    }
    QDir dir(QWoSetting::identifyFilePath());

    if(!dir.rename(nameToPath(name), nameToPath(nameNew))) {
        QMessageBox::warning(this, tr("Warning"), tr("failed to rename file:[%1]->[%2]").arg(name).arg(nameNew));
    }
    reload();
}

void QWoIdentifyDialog::onButtonViewClicked()
{
    QModelIndex idx = ui->identify->currentIndex();
    if(!idx.isValid()) {
        QMessageBox::information(this, tr("info"), tr("no selection"));
        return;
    }
    QAbstractItemModel *model = ui->identify->model();
    QModelIndex idx2 = model->index(idx.row(), 0);
    QString key = idx2.data(ROLE_IDENTIFYKEY).toString();
    if(key.isEmpty()) {
        QMessageBox::information(this, tr("info"), tr("no selection"));
        return;
    }
    QString name = idx2.data().toString();
    QString type = idx2.data(ROLE_IDENTIFYTYPE).toString();
    QString content = type + " " + key + " " + name;
    QWoIdentifyPublicKeyDialog dlg(content, this);
    dlg.exec();
}

void QWoIdentifyDialog::onItemDoubleClicked(QTreeWidgetItem *row, int col)
{
    QString name = row->data(0, Qt::DisplayRole).toString();
    if(name.isEmpty()) {
        return;
    }
    m_result = name;
    close();
}

void QWoIdentifyDialog::onReadyReadStandardOutput()
{
    QProcess *proc = qobject_cast<QProcess*>(sender());
    QByteArray buf = proc->property("stdout").toByteArray();
    buf.append(proc->readAllStandardOutput());
    proc->setProperty("stdout", buf);
}

void QWoIdentifyDialog::onReadyReadStandardError()
{
    QProcess *proc = qobject_cast<QProcess*>(sender());
    QByteArray buf = proc->property("stderr").toByteArray();
    buf.append(proc->readAllStandardError());
    proc->setProperty("stderr", buf);
}

void QWoIdentifyDialog::onFinished(int code)
{
    m_eventLoop->exit(code);
}

void QWoIdentifyDialog::reload()
{
    QDir dir(QWoSetting::identifyFilePath());
    QStringList items = dir.entryList(QDir::Files);    
    QList<IdentifyInfo> all;
    for(int i = 0; i < items.length(); i++) {
        IdentifyInfo info;
        if(!identifyInfomation(dir.path() + "/" + items.at(i), &info)) {
            continue;
        }
        all.append(info);
    }
    ui->identify->clear();
    for(int i = 0; i < all.length(); i++) {
        IdentifyInfo info = all.at(i);
        QStringList cols;
        cols.append(info.name);
        cols.append(info.fingureprint);
        QTreeWidgetItem *item = new QTreeWidgetItem(cols);
        item->setData(0, ROLE_IDENTIFYKEY, info.key);
        item->setData(0, ROLE_IDENTIFYTYPE, info.type);
        QFontMetrics fm(ui->identify->font());
        QSize sz = fm.size(Qt::TextSingleLine, cols.at(0)) + QSize(50, 0);
        int csz = ui->identify->columnWidth(0);
        if(sz.width() > csz) {
            ui->identify->setColumnWidth(0, sz.width());
        }
        ui->identify->addTopLevelItem(item);
    }
}

QString QWoIdentifyDialog::nameToPath(const QString &name)
{
    return name.toUtf8().toBase64(QByteArray::OmitTrailingEquals|QByteArray::Base64UrlEncoding);
}

QString QWoIdentifyDialog::pathToName(const QString &path)
{
    return QByteArray::fromBase64(path.toUtf8(), QByteArray::OmitTrailingEquals|QByteArray::Base64UrlEncoding);
}

QMap<QString, QString> QWoIdentifyDialog::identifyFileGet(const QString &file)
{
    char fingure[] = "--fingerprint--:";
    int fingureLength=sizeof(fingure) / sizeof(char) - 1;
    char pubickey[] = "--publickey--:";
    int publickeyLength = sizeof(pubickey) / sizeof(char) - 1;

    QStringList args;    
    args.append("-l");
    args.append("-y");
    args.append("-f");
    args.append(file);
    QStringList envs;
    envs.push_back(QString("WOTERM_KEYGEN_ACTION=%1").arg("get"));
    QString output = runProcess(args, envs);
    QMap<QString, QString> infos;
    if(output.isEmpty()) {
        return infos;
    }
    QStringList lines = output.split("\n");   
    for(int i = 0; i < lines.length(); i++) {
        QString line = lines.at(i);
        qDebug() << "line:" << line;
        QString type="other";
        if(line.startsWith(fingure)) {
            line = line.remove(0, fingureLength);
            type = "fingureprint";
        }else if(line.startsWith(pubickey)) {
            line.remove(0, publickeyLength);
            type = "publickey";
        }
        QStringList blocks = line.split("]");
        for(int i = 0; i < blocks.length(); i++) {
            QString block = blocks.at(i);
            int idx = block.indexOf('[')+1;
            if(idx >= 1) {
                block = block.mid(idx);
                idx = block.indexOf(':');
                QString name = block.left(idx);
                QString value = block.mid(idx+1);
                infos.insert(type+"."+name, value);
            }
        }
    }
    return infos;
}

bool QWoIdentifyDialog::identifyFileSet(const QString &file, const QString &name)
{
    QStringList args;
    args.append("-t");
    args.append("rsa");
    args.append("-C");
    args.append(name);
    args.append("-P");
    args.append("");
    args.append("-f");
    args.append(file);
    QStringList envs;
    QString output = runProcess(args, envs);
    return !output.isEmpty();
}

bool QWoIdentifyDialog::identifyInfomation(const QString &file, IdentifyInfo *pinfo)
{
    QMap<QString, QString> info = identifyFileGet(file);
    if(info.isEmpty() || pinfo == nullptr) {
        return false;
    }
    QFileInfo fi(file);
    QStringList cols;
    QString name = info.value("fingureprint.comment");
    if(name.isEmpty()) {
        QString tmp = QDir::cleanPath(file);
        QString path = QDir::cleanPath(QWoSetting::identifyFilePath());
        if(tmp.startsWith(path)) {
            name = pathToName(fi.baseName());
        }else{
            name = fi.baseName();
        }
    }
    QString pubkey = info.value("publickey.public-key");
    QString type = info.value("publickey.ssh-name");
    pinfo->name = name;
    pinfo->key = pubkey;
    pinfo->type = type;
    QString hash = QCryptographicHash::hash(pubkey.toUtf8(), QCryptographicHash::Md5).toHex();
    pinfo->fingureprint = hash;
    return true;
}

QByteArray QWoIdentifyDialog::toWotermIdentify(const QByteArray &data)
{
    QByteArray key("woterm.2019");
    if(data.startsWith("woterm:")) {
        QByteArray buf(data);
        return buf.remove(0,  7);
    }
    return QWoUtils::rc4(data, key);
}

QByteArray QWoIdentifyDialog::toStandardIdentify(const QByteArray &data)
{
    QByteArray key("woterm.2019");
    if(data.startsWith("woterm:")) {
        QByteArray buf(data);
        buf = buf.remove(0,  7);
        return QWoUtils::rc4(buf, key);
    }
    return data;
}

QString QWoIdentifyDialog::runProcess(const QStringList &args, const QStringList &envs)
{
    QString keygen = QCoreApplication::applicationDirPath() + "/ssh-keygen";
#ifdef Q_OS_WIN
    keygen += ".exe";
#endif
    QProcess proc(this);
    QObject::connect(&proc, SIGNAL(readyReadStandardOutput()), this, SLOT(onReadyReadStandardOutput()));
    QObject::connect(&proc, SIGNAL(readyReadStandardError()), this, SLOT(onReadyReadStandardError()));
    QObject::connect(&proc, SIGNAL(finished(int)), this, SLOT(onFinished(int)));
    proc.setEnvironment(envs);
    proc.start(keygen, args);    
    wait(3000 * 100);
    int code = proc.exitCode();
    if( code == 0 ){
        QByteArray buf = proc.property("stdout").toByteArray();
        qDebug() << buf;
        return buf;
    }
    return "";
}

bool QWoIdentifyDialog::wait(int ms, int *why)
{
    QEventLoop loop;
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&loop] () {
        loop.exit(1);
    });
    timer.setSingleShot(true);
    timer.setInterval(ms);
    timer.start();
    m_eventLoop = &loop;
    int code = loop.exec();
    if(why != nullptr) {
        *why = code;
    }
    if(code != 0) {
        return false;
    }
    return true;
}
