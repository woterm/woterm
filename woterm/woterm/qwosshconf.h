#pragma once

#include "qwoglobal.h"

#include <QObject>
#include <QHash>

class QBuffer;

class QWoSshConf : public QObject
{
public:
    explicit QWoSshConf(const QString& conf, QObject *parent = nullptr);

    static QWoSshConf* instance();

    bool save();
    bool refresh();
    bool exportTo(const QString& path);

    void remove(const QString& name);
    bool modify(const HostInfo& hi);
    bool append(const HostInfo& hi);
    bool modifyOrAppend(const HostInfo& hi);
    HostInfo find(const QString&name);

    void resetAllProperty(const QString& v);
    bool exists(const QString& name);
    void updatePassword(const QString& name, const QString& password);

    QList<HostInfo> hostList() const;
    QStringList hostNameList() const;

private:
    QHash<QString, HostInfo> parse(const QByteArray& buf);
private:
    QString m_conf;
    QHash<QString, HostInfo> m_hosts;
};
