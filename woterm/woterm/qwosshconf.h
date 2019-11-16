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
    void removeAt(int idx);
    void remove(const QString& name);
    void append(const HostInfo& hi);
    void modify(int idx, const HostInfo& hi);
    void resetAllProperty(const QString& v);
    QList<int> exists(const QString& name);
    void updatePassword(const QString& name, const QString& password);

    QList<HostInfo> hostList() const;
    QStringList hostNameList() const;

    int findHost(const QString& name);
    HostInfo hostInfo(int i);
    HostInfo findHostInfo(const QString&name) {
        return hostInfo(findHost(name));
    }
private:
    QList<HostInfo> parse(const QByteArray& buf);
private:
    QString m_conf;
    QList<HostInfo> m_hosts;
};
