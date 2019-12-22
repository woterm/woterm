#pragma once

#include "qworesult.h"

#include <QObject>
#include <QPointer>
#include <QScriptValue>
#include <QVariantMap>
#include <QScriptable>

class QWoShellProcess;


class QWoResultPrototype : public QObject, public QScriptable
{
    Q_OBJECT
    Q_PROPERTY(bool err READ hasError)
    Q_PROPERTY(QString cmd READ command)
    Q_PROPERTY(QString output READ output)
    Q_PROPERTY(QString code READ code)

public:
    bool hasError() const;
    QString command() const;
    QString output() const;
    int code() const;
    Q_INVOKABLE int hostCode(const QString& host) const;
    Q_INVOKABLE QString hostOutput(const QString& host) const;
};

class QWoResult
{
public:
    QWoResult();
    QWoResult(const QWoResult&v);

    void append(QWoShellProcess* proc);
    void append(const QString& cmd, int code, const QString& output);
    bool hasError();
    void operator=(const QWoResult&v);
    QString command() const;
    int code(const QString& host);
    QString output(const QString& host);
private:
    typedef struct {
        QString guessCmd;
        int guessCode;
        QString guessOutput;
    } Result;
    QMap<QString, Result> m_data;
};

Q_DECLARE_METATYPE(QWoResult);
Q_DECLARE_METATYPE(QWoResult*);
