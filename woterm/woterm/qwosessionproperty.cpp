#include "qwosessionproperty.h"
#include "ui_qwosessionproperty.h"
#include "qwohostsimplelist.h"
#include "qtermwidget.h"
#include "qwohostlistmodel.h"

#include "qwosetting.h"
#include "qwoutils.h"
#include "qwosshconf.h"

#include <QFileDialog>
#include <QDebug>
#include <QIntValidator>
#include <QStringListModel>
#include <QMessageBox>
#include <QModelIndex>

QWoSessionProperty::QWoSessionProperty(ETypeSession ts, int idx, QWidget *parent)
    : QDialog(parent)
    , m_type(ts)
    , m_idx(idx)
    , ui(new Ui::QWoSessionProperty)
{
    Qt::WindowFlags flags = windowFlags();
    setWindowFlags(flags &~Qt::WindowContextHelpButtonHint);
    ui->setupUi(this);
    setWindowTitle(tr("Session Property"));

    if(ts != ModifySession) {
        m_idx = -1;
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

    if(m_type == ResetProperty) {
        ui->connect->hide();
        ui->connectWidget->hide();
    }else{
        QStandardItem *connect = new QStandardItem(tr("Connect"));
        m_model.appendRow(connect);
        connect->appendRow(new QStandardItem(tr("Authentication")));
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

QWoSessionProperty::~QWoSessionProperty()
{
    delete ui;
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
    if(name == tr("Connect")) {
        ui->stacked->setCurrentWidget(ui->connectWidget);
    }else if(name == tr("Authentication")){
        ui->stacked->setCurrentWidget(ui->authenticationWidget);
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
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"));
    qDebug() << "fileName" << fileName;
    fileName = QDir::toNativeSeparators(fileName);
    ui->identify->setCurrentText(fileName);
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
    ui->identify->setEditText("");
    ui->jump->setEditText("");
}

void QWoSessionProperty::initHistory()
{
    {
        QVariant v = QWoSetting::value("history/identifyList");
        QStringList el = v.toStringList();
        QStringListModel *model = new QStringListModel(this);
        model->setStringList(el);
        ui->identify->setModel(model);
    }
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
    if(m_idx < 0) {
        return;
    }
    HostInfo hi = QWoSshConf::instance()->hostInfo(m_idx);
    QVariant v = QWoUtils::qBase64ToVariant(hi.property);
    QVariantMap mdata = v.toMap();
    resetProerty(mdata);
    ui->hostName->setText(hi.name);
    ui->host->setText(hi.host);
    ui->port->setText(QString("%1").arg(hi.port));
    ui->memo->setPlainText(hi.memo);
    ui->userName->setEditText(hi.user);
    ui->password->setText(hi.password);
    ui->identify->setEditText(hi.identityFile);
    if(!hi.password.isEmpty()) {
        ui->authType->setCurrentText("Password");
    }else{
        ui->authType->setCurrentText("IdentifyFile");
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
    if(m_type == ResetProperty) {
        QWoSetting::setValue("property/default", property);
    }else {
        HostInfo hi;
        hi.property = property;
        hi.name = ui->hostName->text();
        hi.host = ui->host->text();
        hi.port = ui->port->text().toInt();
        hi.memo = ui->memo->toPlainText();
        hi.user = ui->userName->currentText();
        hi.password = ui->password->text();
        hi.identityFile = QDir::toNativeSeparators(ui->identify->currentText());
        hi.proxyJump = ui->jump->currentText();

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
        QList<int> idxs = QWoHostListModel::instance()->exists(hi.name);
        if(m_idx > -1) {
            if(idxs.length() > 2) {
                QMessageBox::warning(this, tr("Info"), tr("The same name has been used."));
                return false;
            }
            if(idxs.length() > 0 && idxs.indexOf(m_idx) < 0) {
                QMessageBox::warning(this, tr("Info"), tr("The same name has been used."));
                return false;
            }
            QWoHostListModel::instance()->modify(m_idx, hi);
        }else{
            if(idxs.length() > 0) {
                QMessageBox::warning(this, tr("Info"), tr("The same name has been used."));
                return false;
            }
            QWoHostListModel::instance()->append(hi);
        }
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
    QString identityFile = QDir::toNativeSeparators(ui->identify->currentText());
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
#if 1
    seqTxt.append("\033[31mRed \033[32mGreen \033[33mYellow \033[34mBlue");
    seqTxt.append("\r\n\033[35mMagenta \033[36mCyan \033[37mWhite \033[39mDefault");
    seqTxt.append("\r\n\033[40mBlack \033[41mRed \033[42mGreen \033[43mYellow \033[44mBlue");
    seqTxt.append("\r\n\033[45mMagenta \033[46mCyan \033[47mWhite \033[49mDefault");
#else
    seqTxt.append("\033[mgYw \033[40mgYw \033[41mgYw \033[42mgYw \033[43mgYw \033[44mgYw \033[45mgYw \033[46mgYw \033[47mgYw ");
    seqTxt.append("\r\n");
    seqTxt.append("\033[1mgYw \033[1;40mgYw \033[1;41mgYw \033[1;42mgYw \033[1;43mgYw \033[1;44mgYw \033[1;45mgYw \033[1;46mgYw \033[1;47mgYw ");
    seqTxt.append("\r\n");
    seqTxt.append("\033[30mgYw \033[30;40mgYw \033[30;41mgYw \033[30;42mgYw \033[30;43mgYw \033[30;44mgYw \033[30;45mgYw \033[30;46mgYw \033[30;47mgYw ");
    seqTxt.append("\r\n");
    seqTxt.append("\033[1;30mgYw \033[1;30;40mgYw \033[1;30;41mgYw \033[1;30;42mgYw \033[1;30;43mgYw \033[1;30;44mgYw \033[1;30;45mgYw \033[1;30;46mgYw \033[1;30;47mgYw ");
    seqTxt.append("\r\n");
    seqTxt.append("\033[31mgYw \033[31;40mgYw \033[31;41mgYw \033[31;42mgYw \033[31;43mgYw \033[31;44mgYw \033[31;45mgYw \033[31;46mgYw \033[31;47mgYw ");
    seqTxt.append("\r\n");
    seqTxt.append("\033[1;31mgYw \033[1;31;40mgYw \033[1;31;41mgYw \033[1;31;42mgYw \033[1;31;43mgYw \033[1;31;44mgYw \033[1;31;45mgYw \033[1;31;46mgYw \033[1;31;47mgYw ");
    seqTxt.append("\r\n");
    seqTxt.append("\033[32mgYw \033[32;40mgYw \033[32;41mgYw \033[32;42mgYw \033[32;43mgYw \033[32;44mgYw \033[32;45mgYw \033[32;46mgYw \033[32;47mgYw ");
    seqTxt.append("\r\n");
    seqTxt.append("\033[1;32mgYw \033[1;32;40mgYw \033[1;32;41mgYw \033[1;32;42mgYw \033[1;32;43mgYw \033[1;32;44mgYw \033[1;32;45mgYw \033[1;32;46mgYw \033[1;32;47mgYw ");
    seqTxt.append("\r\n");
    seqTxt.append("\033[33mgYw \033[33;40mgYw \033[33;41mgYw \033[33;42mgYw \033[33;43mgYw \033[33;44mgYw \033[33;45mgYw \033[33;46mgYw \033[33;47mgYw ");
    seqTxt.append("\r\n");
    seqTxt.append("\033[1;33mgYw \033[1;33;40mgYw \033[1;33;41mgYw \033[1;33;42mgYw \033[1;33;43mgYw \033[1;33;44mgYw \033[1;33;45mgYw \033[1;33;46mgYw \033[1;33;47mgYw ");
    seqTxt.append("\r\n");
    seqTxt.append("\033[34mgYw \033[34;40mgYw \033[34;41mgYw \033[34;42mgYw \033[34;43mgYw \033[34;44mgYw \033[34;45mgYw \033[34;46mgYw \033[34;47mgYw ");
    seqTxt.append("\r\n");
    seqTxt.append("\033[1;34mgYw \033[1;34;40mgYw \033[1;34;41mgYw \033[1;34;42mgYw \033[1;34;43mgYw \033[1;34;44mgYw \033[1;34;45mgYw \033[1;34;46mgYw \033[1;34;47mgYw ");
    seqTxt.append("\r\n");
    seqTxt.append("\033[35mgYw \033[35;40mgYw \033[35;41mgYw \033[35;42mgYw \033[35;43mgYw \033[35;44mgYw \033[35;45mgYw \033[35;46mgYw \033[35;47mgYw ");
    seqTxt.append("\r\n");
    seqTxt.append("\033[1;35mgYw \033[1;35;40mgYw \033[1;35;41mgYw \033[1;35;42mgYw \033[1;35;43mgYw \033[1;35;44mgYw \033[1;35;45mgYw \033[1;35;46mgYw \033[1;35;47mgYw ");
    seqTxt.append("\r\n");
    seqTxt.append("\033[36mgYw \033[36;40mgYw \033[36;41mgYw \033[36;42mgYw \033[36;43mgYw \033[36;44mgYw \033[36;45mgYw \033[36;46mgYw \033[36;47mgYw ");
    seqTxt.append("\r\n");
    seqTxt.append("\033[1;36mgYw \033[1;36;40mgYw \033[1;36;41mgYw \033[1;36;42mgYw \033[1;36;43mgYw \033[1;36;44mgYw \033[1;36;45mgYw \033[1;36;46mgYw \033[1;36;47mgYw ");
    seqTxt.append("\r\n");
    seqTxt.append("\033[37mgYw \033[37;40mgYw \033[37;41mgYw \033[37;42mgYw \033[37;43mgYw \033[37;44mgYw \033[37;45mgYw \033[37;46mgYw \033[37;47mgYw ");
    seqTxt.append("\r\n");
    seqTxt.append("\033[1;37mgYw \033[1;37;40mgYw \033[1;37;41mgYw \033[1;37;42mgYw \033[1;37;43mgYw \033[1;37;44mgYw \033[1;37;45mgYw \033[1;37;46mgYw \033[1;37;47mgYw ");
#endif
    m_preview->parseSequenceText(seqTxt);
    m_preview->setBlinkingCursor(true);
}

void QWoSessionProperty::onTimeout()
{
    setFixPreviewString();
}
