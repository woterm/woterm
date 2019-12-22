#include "qwoscriptmaster.h"
#include "qtermwidget.h"
#include "qwoshellcommand.h"
#include "qwolocalcommand.h"
#include "qwosetting.h"

#include <QScriptEngine>
#include <QFile>
#include <QMessageBox>
#include <QDebug>
#include <QScriptEngineDebugger>
#include <QMainWindow>
#include <QMetaObject>
#include <QMetaType>
#include <QFileInfo>


Q_GLOBAL_STATIC(QWoResultPrototype, _resultPrototype);
QWoResultPrototype *resultPrototype()
{
    return _resultPrototype();
};

template <class Container>
QScriptValue toScriptValue(QScriptEngine *eng, const Container &cont)
{
    QScriptValue a = eng->newArray();
    typename Container::const_iterator begin = cont.begin();
    typename Container::const_iterator end = cont.end();
    typename Container::const_iterator it;
    for (it = begin; it != end; ++it){
        a.setProperty(quint32(it - begin), eng->toScriptValue(*it));
    }
    return a;
}

template <class Container>
void fromScriptValue(const QScriptValue &value, Container &cont)
{
    quint32 len = value.property("length").toUInt32();
    for (quint32 i = 0; i < len; ++i) {
        QScriptValue item = value.property(i);
        typedef typename Container::value_type ContainerValue;
        cont.push_back(qscriptvalue_cast<ContainerValue>(item));
    }
}

template <typename Type>
QScriptValue ScriptObjectToScriptValue(QScriptEngine *engine, Type* const &in)
{
    return engine->newQObject(in, QScriptEngine::AutoOwnership);
}

template <typename Type>
void ScriptObjectFromScriptValue(const QScriptValue &object, Type* &out)
{
    out = qobject_cast<Type*>(object.toQObject());
}

QScriptValue QWoResult_convertor(QScriptContext *context, QScriptEngine *engine)
{
    return engine->toScriptValue(QWoResult());
}

QWoScriptMaster::QWoScriptMaster(QTermWidget *term, QObject *parent)
    : QThread(parent)
    , m_term(term)
{
    QObject::connect(this, SIGNAL(error(const QString&)), this, SLOT(onShowError(const QString&)));
    QObject::connect(this, SIGNAL(error(const QStringList&)), this, SLOT(onShowError(const QStringList&)));
}

bool QWoScriptMaster::start(const QString& fullPath,const QStringList &files, const QString &entry, const QStringList& args, bool debug)
{
    if(m_engine) {
        return false;
    }
    for(int i = 0; i < files.count(); i++) {
        if(!QFileInfo::exists(files.at(i))){
            return false;
        }
    }
    m_fullPath = fullPath;
    m_scriptFiles = files;
    m_entry = entry;
    m_args = args;
    if(debug) {
        execute(true);
    }else{
        QThread::start();
    }

    return true;
}

void QWoScriptMaster::stop()
{
    emit stopit();
}

bool QWoScriptMaster::isRunning()
{
    return m_engine != nullptr;
}

void QWoScriptMaster::run()
{
    execute(false);
}

void QWoScriptMaster::showError(const QString &msg)
{
    emit error(msg);
}

void QWoScriptMaster::showError(const QStringList &msg)
{
    emit error(msg);
}

void QWoScriptMaster::execute(bool debug)
{
    QScriptEngine engine;
    QScriptEngineDebugger debugger;

    m_engine = &engine;
    m_debugger = &debugger;

    if(debug) {
        debugger.attachTo(&engine);
        QMainWindow *debugWindow = debugger.standardWindow();
        debugWindow->resize(1024, 640);
        debugWindow->show();
        debugger.action(QScriptEngineDebugger::InterruptAction)->triggered();
    }

    qRegisterMetaType<QWoResult>("QWoResult");
    qScriptRegisterMetaType<QStringList>(&engine, toScriptValue, fromScriptValue);
    qScriptRegisterMetaType<QList<QString>>(&engine, toScriptValue, fromScriptValue);

    engine.setDefaultPrototype(qMetaTypeId<QWoResult>(), engine.newQObject(resultPrototype()));

    QStringList coreFiles;
    coreFiles << "log.js" << "local.js" << "shell.js";
    for(int i = 0; i < coreFiles.count(); i++) {
        QString file = QWoSetting::privateJsCorePath() +"/"+ coreFiles.at(i);
        QFile tmp(file);
        if(!tmp.open(QFile::ReadOnly)) {
            showError("can't find the file:"+file);
            return;
        }
        engine.evaluate(tmp.readAll(), tmp.fileName());
    }

    for(int i = 0; i < m_scriptFiles.count(); i++) {
        QString file = m_scriptFiles.at(i);
        QFile tmp(file);
        if(!tmp.open(QFile::ReadOnly)) {
            showError("can't find the file:"+file);
            return;
        }
        engine.evaluate(tmp.readAll(), tmp.fileName());
    }

    if(engine.hasUncaughtException()) {
        QStringList except = engine.uncaughtExceptionBacktrace();
        showError(except);
        return;
    }
    QScriptValue global = engine.globalObject();
    QWoLog log;
    QObject::connect(&log, SIGNAL(logArrived(const QString&,const QString&)), this, SIGNAL(logArrived(const QString&,const QString&)));
    global.setProperty("log", engine.newQObject(&log));

    QWoShellCommand shell(&log);
    m_shell = &shell;
    QProxy proxy(&engine, &shell);
    QObject::connect(this, SIGNAL(stopit()), &proxy, SLOT(onStop()));
    QObject::connect(&shell, SIGNAL(passwordRequest(const QString&,bool)), this, SIGNAL(passwordRequest(const QString&,bool)));
    QObject::connect(&shell, SIGNAL(dataReceived(const QByteArray&)), this, SLOT(onDataReceived(const QByteArray&)));
    global.setProperty("shell", engine.newQObject(&shell));

    QWoLocalCommand local(m_fullPath, &log);
    global.setProperty("local", engine.newQObject(&local));
    QObject::connect(&local, SIGNAL(dataReceived(const QByteArray&)), this, SLOT(onDataReceived(const QByteArray&)));

    QScriptValue main = global.property(m_entry);
    if(!main.isFunction()) {
        showError(QString("failed to find the main entry[%1] function").arg(m_entry));
        return;
    }
    QScriptValueList args;
    for(int i = 0; i < m_args.count(); i++) {
        args << m_args.at(i);
    }
    QScriptValue result = main.call(QScriptValue(), args);
    if(engine.hasUncaughtException()) {
        QStringList except = engine.uncaughtExceptionBacktrace();
        showError(except);
        return;
    }
    showError("Finish All Job");
}

void QWoScriptMaster::onShowError(const QString& msg)
{
    m_term->parseSequenceText("\033[1m\033[31m");
    m_term->parseSequenceText(QString("\r\n%1").arg(msg).toUtf8());
    m_term->parseSequenceText("\033[0m");
}

void QWoScriptMaster::onShowError(const QStringList &msg)
{
    m_term->parseSequenceText("\033[1m\033[31m");
    for(int i = 0; i < msg.count(); i++) {
        m_term->parseSequenceText(QString("\r\n%1").arg(msg.at(i)).toUtf8());
    }
    m_term->parseSequenceText("\033[0m");
}

void QWoScriptMaster::onDataReceived(const QByteArray &data)
{
    m_term->parseSequenceText(data);
}

void QWoScriptMaster::passwordInput(const QString &pwd)
{
    QMetaObject::invokeMethod(m_shell, "onPasswordInput", Qt::AutoConnection, Q_ARG(const QString&, pwd));
}

void QWoLog::log(const QString &level, const QString &msg)
{
    if(level == "info") {
        info(msg);
    }else if(level == "warn") {
        warn(msg);
    }else{
        error(msg);
    }
}

void QWoLog::info(const QString& msg)
{
    emit logArrived("info", msg);
}

void QWoLog::warn(const QString& msg)
{
    emit logArrived("warn", msg);
}

void QWoLog::error(const QString& msg)
{
    emit logArrived("error", msg);
}

QProxy::QProxy(QScriptEngine *eng, QWoShellCommand *shell)
    : m_engine(eng)
    , m_shell(shell)
{

}

void QProxy::onStop()
{
    if(m_engine) {
        m_engine->abortEvaluation();
    }
    if(m_shell){
        m_shell->stop();
    }
}
