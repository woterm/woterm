#pragma once

#include <QVariant>

class QWoSetting
{
public:
    static void setValue(const QString& key, const QVariant& v);
    static QVariant value(const QString& key);

    static QString applicationDataPath();
    static QString identifyFilePath();
    static QString historyFilePath();
    static QString examplePath();
    static QString privateDataPath();
    static QString privateJsCorePath();
    static QString privateColorSchemaPath();
    static QString privateKeyboardLayoutPath();
    static QString privateTranslationPath();
    static QString zmodemSZPath();
    static QString zmodemRZPath();
    static QString sshProgramPath();
    static QString sshServerListPath();
    static QString lastJsLoadPath();
    static void setLastJsLoadPath(const QString& path);
private:
    static QString specialFilePath(const QString& name);
};
