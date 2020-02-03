#include "qwosessionproperty.h"
#include "ui_qwosessionproperty.h"
#include "qwohostsimplelist.h"
#include "qtermwidget.h"
#include "qwohostlistmodel.h"
#include "qwoidentifydialog.h"

#include "qwosetting.h"
#include "qwoutils.h"
#include "qwosshconf.h"

#include <QFileDialog>
#include <QDebug>
#include <QIntValidator>
#include <QStringListModel>
#include <QMessageBox>
#include <QModelIndex>

QWoSessionProperty::QWoSessionProperty(const QString& name,  QWidget *parent)
    : QDialog(parent)
    , m_type(ModifySession)
    , m_name(name)
    , ui(new Ui::QWoSessionProperty)
{
    init();
}

QWoSessionProperty::QWoSessionProperty(QWidget *parent)
    : QDialog(parent)
    , m_type(DefaultProperty)
    , ui(new Ui::QWoSessionProperty)
{
    init();
}


QWoSessionProperty::~QWoSessionProperty()
{
    delete ui;
}

void QWoSessionProperty::setButtonFlags(EButtonFlags flags)
{
    ui->applyall->setVisible(flags.testFlag(ButtonApplyAll));
    ui->connect->setVisible(flags.testFlag(ButtonConnect));
    ui->save->setVisible(flags.testFlag(ButtonSave));
}

void QWoSessionProperty::onAuthCurrentIndexChanged(const QString &txt)
{
    bool isPass = txt == tr("Password");

    QWoUtils::setLayoutVisible(ui->passLayout, isPass);
    QWoUtils::setLayoutVisible(ui->identifyLayout, !isPass);
}

/*
Ps = 3 0  -> Set foreground color to Black.
            Ps = 3 1  -> Set foreground color to Red.
            Ps = 3 2  -> Set foreground color to Green.
            Ps = 3 3  -> Set foreground color to Yellow.
            Ps = 3 4  -> Set foreground color to Blue.
            Ps = 3 5  -> Set foreground color to Magenta.
            Ps = 3 6  -> Set foreground color to Cyan.
            Ps = 3 7  -> Set foreground color to White.
            Ps = 3 9  -> Set foreground color to default, ECMA-48 3rd.
            Ps = 4 0  -> Set background color to Black.
            Ps = 4 1  -> Set background color to Red.
            Ps = 4 2  -> Set background color to Green.
            Ps = 4 3  -> Set background color to Yellow.
            Ps = 4 4  -> Set background color to Blue.
            Ps = 4 5  -> Set background color to Magenta.
            Ps = 4 6  -> Set background color to Cyan.
            Ps = 4 7  -> Set background color to White.
            Ps = 4 9  -> Set background color to default, ECMA-48 3rd.
*/
void QWoSessionProperty::onColorCurrentIndexChanged(const QString &txt)
{
    m_preview->setColorScheme(txt);
}

void QWoSessionProperty::onKeyBindCurrentIndexChanged(const QString &txt)
{
    m_preview->setKeyBindings(txt);
}

void QWoSessionProperty::onCurrentFontChanged(const QFont &font)
{
    QFont f = font;
    f.setPointSize(ui->fontSize->value());
    m_preview->setTerminalFont(f);
    m_preview->setBlinkingCursor(true);
}

void QWoSessionProperty::onBlockCursorToggled()
{
    m_preview->setKeyboardCursorShape(Konsole::Emulation::KeyboardCursorShape::BlockCursor);
    m_preview->setBlinkingCursor(true);
}

void QWoSessionProperty::onUnderlineCursorToggled()
{
    m_preview->setKeyboardCursorShape(Konsole::Emulation::KeyboardCursorShape::UnderlineCursor);
    m_preview->setBlinkingCursor(true);
}

void QWoSessionProperty::onBeamCursorToggled()
{
    m_preview->setKeyboardCursorShape(Konsole::Emulation::KeyboardCursorShape::IBeamCursor);
    m_preview->setBlinkingCursor(true);
}

void QWoSessionProperty::onFontValueChanged(int i)
{
    QFont f = m_preview->getTerminalFont();
    f.setPointSize(i);
    m_preview->setTerminalFont(f);
}

void QWoSessionProperty::onTreeItemClicked(const QModelIndex &idx)
{
    QString name = idx.data().toString();
    if(name == tr("Connection")) {
        ui->stacked->setCurrentWidget(ui->connectWidget);
    }else if(name == tr("Terminal")){
        ui->stacked->setCurrentWidget(ui->terminalWidget);
    }else if(name == tr("Appearance")){
        ui->stacked->setCurrentWidget(ui->appearanceWidget);
    }else if(name == tr("FileTransfer")){
        ui->stacked->setCurrentWidget(ui->fileTransferWidget);
    }
}

void QWoSessionProperty::onIdentifyBrowserClicked()
{
    QString fileName = QWoIdentifyDialog::open(this);
    if(fileName.isEmpty()) {
        return;
    }
    qDebug() << "fileName" << fileName;
    fileName = QDir::toNativeSeparators(fileName);
    ui->identify->setText(fileName);
}

void QWoSessionProperty::onJumpBrowserClicked()
{
    QWoHostSimpleList dlg(this);
    dlg.exec();
    HostInfo hi;
    if(dlg.result(&hi)) {
        ui->jump->setEditText(hi.name);
    }
}

void QWoSessionProperty::onReadyToConnect()
{
    if(!saveConfig()) {
        return;
    }
    emit connect(ui->hostName->text());
    done(Connect);
}

void QWoSessionProperty::onApplyToAll()
{
    savePropertyToAll();
}

void QWoSessionProperty::onReadyToSave()
{
    if(!saveConfig()) {
        return;
    }
    done(Save);
}

void QWoSessionProperty::onSzDirBrowser()
{
    QString path = ui->szUpload->text();
    QString fileName = QFileDialog::getExistingDirectory(this, tr("Open Directory"), path, QFileDialog::ShowDirsOnly);
    qDebug() << "fileName" << fileName;
    if(!fileName.isEmpty()) {
        fileName = QDir::toNativeSeparators(fileName);
        ui->szUpload->setText(fileName);
    }
}

void QWoSessionProperty::onRzDirBrowser()
{
    QString path = ui->rzDown->text();
    QString fileName = QFileDialog::getExistingDirectory(this, tr("Open Directory"), path, QFileDialog::ShowDirsOnly);
    qDebug() << "fileName" << fileName;
    if(!fileName.isEmpty()) {
        fileName = QDir::toNativeSeparators(fileName);
        ui->rzDown->setText(fileName);
    }
}

void QWoSessionProperty::initDefault()
{
    QString val = QWoSetting::value("property/default").toString();
    QVariantMap mdata = QWoUtils::qBase64ToVariant(val).toMap();
    resetProerty(mdata);
    ui->userName->setEditText("");
    ui->identify->setText("");
    ui->jump->setEditText("");
}

void QWoSessionProperty::initHistory()
{
    {
        QVariant v = QWoSetting::value("history/userNameList");
        QStringList el = v.toStringList();
        QStringListModel *model = new QStringListModel(this);
        model->setStringList(el);
        ui->userName->setModel(model);
    }
    {
        QVariant v = QWoSetting::value("history/proxyJumpList");
        QStringList el = v.toStringList();
        QStringListModel *model = new QStringListModel(this);
        model->setStringList(el);
        ui->jump->setModel(model);
    }
}

void QWoSessionProperty::initCustom()
{
    if(m_name.isEmpty()) {
        return;
    }
    HostInfo hi = QWoSshConf::instance()->find(m_name);
    QVariant v = QWoUtils::qBase64ToVariant(hi.property);
    QVariantMap mdata = v.toMap();
    resetProerty(mdata);
    ui->hostName->setText(hi.name);
    ui->host->setText(hi.host);
    ui->port->setText(QString("%1").arg(hi.port));
    ui->memo->setPlainText(hi.memo);
    ui->userName->setEditText(hi.user);
    ui->password->setText(hi.password);
    if(!hi.identityFile.isEmpty()){
        QString identify = hi.identityFile;
        if(identify.startsWith("woterm:")) {
            identify = identify.remove(0, 7);
        }
        ui->identify->setText(QWoUtils::pathToName(identify));
    }
    if(!hi.password.isEmpty()) {
        ui->authType->setCurrentText("Password");
    }else{
        ui->authType->setCurrentText("Identify");
    }
    ui->jump->setEditText(hi.proxyJump);
}

void QWoSessionProperty::resetProerty(QVariantMap mdata)
{
    QDir home = QDir::home();
    QString szPathUpload = mdata.value("szPath", QDir::toNativeSeparators(home.path())).toString();
    QString rzPathDown = mdata.value("rzPath", QDir::toNativeSeparators(home.path())).toString();
    ui->szUpload->setText(szPathUpload);
    ui->rzDown->setText(rzPathDown);

    ui->port->setText("22");

    QString schema = mdata.value("colorSchema", DEFAULT_COLOR_SCHEMA).toString();
    ui->schema->setCurrentText(schema);

    QString fontName = mdata.value("fontName", DEFAULT_FONT_FAMILY).toString();
    int fontSize = mdata.value("fontSize", DEFAULT_FONT_SIZE).toInt();
    ui->fontSize->setValue(fontSize);

    QFont font(fontName, fontSize);
    ui->fontChooser->setCurrentFont(font);

    ui->liveCheck->setChecked(mdata.value("liveCheck", false).toBool());
    ui->liveDuration->setValue(mdata.value("liveDuration", 60).toInt());

    QString cursorType = mdata.value("cursorType", "block").toString();
    if(cursorType.isEmpty() || cursorType == "block") {
        ui->blockCursor->setChecked(true);
    }else if(cursorType == "underline") {
        ui->underlineCursor->setChecked(true);
    }else {
        ui->beamCursor->setChecked(true);
    }
    QString line = mdata.value("historyLength", QString("%1").arg(DEFAULT_HISTORY_LINE_LENGTH)).toString();
    ui->lineSize->setText(line);
}

bool QWoSessionProperty::saveConfig()
{    
    QString property = sessionProperty();
    if(m_type == DefaultProperty) {
        QWoSetting::setValue("property/default", property);
    }else {
        HostInfo hi;
        hi.property = property;
        hi.name = ui->hostName->text();
        hi.host = ui->host->text();
        hi.port = ui->port->text().toInt();
        hi.memo = ui->memo->toPlainText();
        hi.user = ui->userName->currentText();
        hi.proxyJump = ui->jump->currentText();        
        if(ui->authType->currentText() == "Password") {
            hi.password = ui->password->text();
        }else{
            hi.identityFile = QWoUtils::nameToPath(ui->identify->text());
            QString identify = QWoSetting::identifyFilePath() + "/" + hi.identityFile;
            if(!QFileInfo::exists(identify)) {
                QMessageBox::warning(this, tr("Info"), tr("failed to find the identify file"));
                return false;
            }
            hi.identityFile.insert(0, "woterm:");
        }

        if(hi.name.isEmpty()) {
            QMessageBox::warning(this, tr("Info"), tr("The name can't be empty"));
            return false;
        }
        if(hi.host.isEmpty()) {
            QMessageBox::warning(this, tr("Info"), tr("The host can't be empty"));
            return false;
        }
        if(hi.user.isEmpty()) {
            QMessageBox::warning(this, tr("Info"), tr("The userName can't be empty"));
            return false;
        }
        if(hi.port < 10 || hi.port > 65535) {
            QMessageBox::warning(this, tr("Info"), tr("The port should be at [10,65535]"));
            return false;
        }
        if(m_name != hi.name) {
            if(QWoHostListModel::instance()->exists(hi.name)) {
                QMessageBox::warning(this, tr("Info"), tr("The host name had been used.."));
                return false;
            }
        }
        QWoHostListModel::instance()->modifyOrAppend(hi);
    }
    saveHistory();
    return true;
}

QString QWoSessionProperty::sessionProperty()
{
    QVariantMap mdata;
    mdata["szPath"] = ui->szUpload->text();
    mdata["rzPath"] = ui->rzDown->text();
    mdata["colorSchema"] = ui->schema->currentText();
    mdata["fontName"] = ui->fontChooser->currentFont().family();
    mdata["fontSize"] = ui->fontSize->value();
    if(ui->blockCursor->isChecked()) {
        mdata["cursorType"] = "block";
    }else if(ui->underlineCursor->isChecked()) {
        mdata["cursorType"] = "underline";
    }else {
        mdata["cursorType"] = "beam";
    }
    mdata["historyLength"] = ui->lineSize->text();
    mdata["liveCheck"] = ui->liveCheck->isChecked();
    mdata["liveDuration"] = ui->liveDuration->value();
    return QWoUtils::qVariantToBase64(mdata);
}

void QWoSessionProperty::savePropertyToAll()
{
    QMessageBox::StandardButton btn = QMessageBox::information(this, tr("Info"), tr("Only the property will override all session. Press yes to continue."), QMessageBox::Ok|QMessageBox::No);
    if(btn == QMessageBox::No) {
        return;
    }
    QString property = sessionProperty();
    QWoHostListModel::instance()->resetAllProperty(property);
}

void QWoSessionProperty::saveHistory()
{
    QString identityFile = QDir::toNativeSeparators(ui->identify->text());
    if(!identityFile.isEmpty())
    {
        QVariant v = QWoSetting::value("history/identifyList");
        QStringList el = v.toStringList();
        el.removeAll(identityFile);
        el.insert(0, identityFile);
        if(el.length() > 5) {
            el.removeLast();
        }
        QWoSetting::setValue("history/identifyList", el);
    }
    QString user = ui->userName->currentText();
    if(!user.isEmpty())
    {
        QVariant v = QWoSetting::value("history/userNameList");
        QStringList el = v.toStringList();
        el.removeAll(user);
        el.insert(0, user);
        if(el.length() > 5) {
            el.removeLast();
        }
        QWoSetting::setValue("history/userNameList", el);
    }
    QString proxyJump = ui->jump->currentText();
    if(!proxyJump.isEmpty())
    {
        QVariant v = QWoSetting::value("history/proxyJumpList");
        QStringList el = v.toStringList();
        el.removeAll(proxyJump);
        el.insert(0, proxyJump);
        if(el.length() > 5) {
            el.removeLast();
        }
        QWoSetting::setValue("history/proxyJumpList", el);
    }
}

void QWoSessionProperty::setFixPreviewString()
{
    m_preview->clear();
    QByteArray seqTxt;
    seqTxt.append("\033[31mRed \033[32mGreen \033[33mYellow \033[34mBlue");
    seqTxt.append("\r\n\033[35mMagenta \033[36mCyan \033[37mWhite \033[39mDefault");
    seqTxt.append("\r\n\033[40mBlack \033[41mRed \033[42mGreen \033[43mYellow \033[44mBlue");
    seqTxt.append("\r\n\033[45mMagenta \033[46mCyan \033[47mWhite \033[49mDefault");
    m_preview->parseSequenceText(seqTxt);
    m_preview->setBlinkingCursor(true);
}

void QWoSessionProperty::onTimeout()
{
    setFixPreviewString();
}

void QWoSessionProperty::init()
{
    Qt::WindowFlags flags = windowFlags();
    setWindowFlags(flags &~Qt::WindowContextHelpButtonHint);
    ui->setupUi(this);
    ui->identify->setReadOnly(true);

    if(m_type == DefaultProperty) {
        setWindowTitle(QString(tr("Session[Default]")));
    }else{
        setWindowTitle(QString(tr("Session[%1]")).arg(m_name.isEmpty() ? tr("New") : m_name));
        ui->applyall->hide();
    }

    m_preview = new QTermWidget(this);
    m_preview->setScrollBarPosition(QTermWidget::ScrollBarRight);
    ui->previewLayout->addWidget(m_preview);
    QTimer *timer = new QTimer(this);
    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
    timer->start(1000);

    QStringList schemas = m_preview->availableColorSchemes();
    schemas.sort();
    ui->schema->setModel(new QStringListModel(schemas, this));
    QObject::connect(ui->schema, SIGNAL(currentIndexChanged(const QString &)),  this, SLOT(onColorCurrentIndexChanged(const QString &)));

    ui->port->setValidator(new QIntValidator(1, 65535));
    ui->lineSize->setValidator(new QIntValidator(DEFAULT_HISTORY_LINE_LENGTH, 65535));

    ui->fontChooser->setEditable(false);
    ui->fontChooser->setFontFilters(QFontComboBox::MonospacedFonts);
    QObject::connect(ui->fontChooser, SIGNAL(currentFontChanged(const QFont&)), this, SLOT(onCurrentFontChanged(const QFont&)));

    ui->tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tree->setModel(&m_model);
    ui->tree->setIndentation(10);
    ui->tree->setRootIsDecorated(true);

    if(m_type == DefaultProperty) {
        ui->connect->hide();
        ui->connectWidget->hide();
    }else{
        QStandardItem *connect = new QStandardItem(tr("Connection"));
        m_model.appendRow(connect);
    }

    QStandardItem *terminal = new QStandardItem(tr("Terminal"));
    m_model.appendRow(terminal);
    QStandardItem *appearance = new QStandardItem(tr("Appearance"));
    m_model.appendRow(appearance);
    QStandardItem *fileTransfre = new QStandardItem(tr("FileTransfer"));
    m_model.appendRow(fileTransfre);

    QObject::connect(ui->tree, SIGNAL(clicked(const QModelIndex&)), this, SLOT(onTreeItemClicked(const QModelIndex&)));
    QModelIndex index = m_model.index(0, 0);
    ui->tree->clicked(index);

    QObject::connect(ui->blockCursor, SIGNAL(toggled(bool)), this, SLOT(onBlockCursorToggled()));
    QObject::connect(ui->underlineCursor, SIGNAL(toggled(bool)), this, SLOT(onUnderlineCursorToggled()));
    QObject::connect(ui->beamCursor, SIGNAL(toggled(bool)), this, SLOT(onBeamCursorToggled()));

    QObject::connect(ui->authType, SIGNAL(currentIndexChanged(const QString &)),  this, SLOT(onAuthCurrentIndexChanged(const QString &)));
    QObject::connect(ui->connect, SIGNAL(clicked()), this, SLOT(onReadyToConnect()));
    QObject::connect(ui->applyall, SIGNAL(clicked()), this, SLOT(onApplyToAll()));
    QObject::connect(ui->save, SIGNAL(clicked()), this, SLOT(onReadyToSave()));
    QObject::connect(ui->cancel, SIGNAL(clicked()), this, SLOT(close()));

    QObject::connect(ui->szDirBrowser, SIGNAL(clicked()), this, SLOT(onSzDirBrowser()));
    QObject::connect(ui->rzDirBrowser, SIGNAL(clicked()), this, SLOT(onRzDirBrowser()));

    QObject::connect(ui->identifyBrowser, SIGNAL(clicked()),  this, SLOT(onIdentifyBrowserClicked()));
    QObject::connect(ui->jumpBrowser, SIGNAL(clicked()),  this, SLOT(onJumpBrowserClicked()));

    QObject::connect(ui->fontSize, SIGNAL(valueChanged(int)), this, SLOT(onFontValueChanged(int)));

    onAuthCurrentIndexChanged(tr("Password"));

    initHistory();
    initDefault();
    initCustom();

    ui->tree->expandAll();
}
