#pragma once

#include <QVariant>

class QWoSetting
{
public:
    static void setValue(const QString& key, const QVariant& v);
    static QVariant value(const QString& key);

    static QString zmodemSZPath();
    static QString zmodemRZPath();
    static QString sshProgramPath();
    static QString sshServerListPath();
    static QString ipcProgramPath();
    static QString shellProgramPath();
    static QString utilsCommandPath();
    static QStringList utilsCommandList();
};
