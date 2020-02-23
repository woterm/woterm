#include "qwosshconf.h"
#include "qwosetting.h"
#include "qwoutils.h"

#include <QFile>
#include <QDebug>
#include <QMessageBox>
#include <QBuffer>

/*
 *  #
 *  #    <---memo--->
 *  #
 *  Host name
 *    IgnoreUnknown Password,Group,XXX
 *    HostName xxxx
 *    Port  22
 *    User  abc
 *    IdentityFile ~xxx
 *    Password xxxx
 *    ProxyJump xxx
 *    GroupName xxxx
 */

#define IGNORE_UNKNOW ("Group,Password")

bool lessThan(const HostInfo& a, const HostInfo& b) {
    return a.name < b.name;
}
bool moreThan(const HostInfo& a, const HostInfo& b) {
    return a.name > b.name;
}

void copyHostInfo(HostInfo &dst, const HostInfo &src)
{
    if(!src.host.isEmpty()) {
        dst.host = src.host;
    }
    if(!src.identityFile.isEmpty()) {
        dst.identityFile = src.identityFile;
    }
    if(!src.password.isEmpty()) {
        dst.password = src.password;
    }
    if(src.port > 0) {
        dst.port = src.port;
    }
    if(!src.proxyJump.isEmpty()) {
        dst.proxyJump = src.proxyJump;
    }
    if(!src.user.isEmpty()) {
        dst.user = src.user;
    }
}

QWoSshConf::QWoSshConf(const QString& conf, QObject *parent)
    :QObject (parent)
    ,m_conf(conf)
{
}

QWoSshConf *QWoSshConf::instance()
{
    static QWoSshConf sc(QWoSetting::sshServerListPath());
    return &sc;
}

QHash<QString, HostInfo> QWoSshConf::parse(const QByteArray& buf)
{
    QList<QByteArray> lines = buf.split('\n');
    QList<QStringList> blocks;
    QStringList block;
    for(int i = 0; i < lines.length(); i++) {
        QByteArray line = lines.at(i);
        if(line.startsWith("Host ")) {
            if(!block.isEmpty()) {
                blocks.push_back(block);
                block.clear();
            }
            for(int j = i-1; j > 0; j--) {
                QByteArray prev = lines.at(j);
                if(prev.startsWith('#')){
                    block.insert(0, prev);
                    continue;
                }
                break;
            }
            block.push_back(line);
        }else if(line.startsWith(' ') || line.startsWith('\t')) {
            if(!block.isEmpty()) {
                block.push_back(line);
            }
        }else{
            if(!block.isEmpty()) {
                blocks.push_back(block);
                block.clear();
            }
        }
    }
    if(!block.isEmpty()) {
        blocks.push_back(block);
        block.clear();
    }
    //for(QList<QStringList>::iterator iter = blocks.begin(); iter != blocks.end(); iter++) {
    //    qDebug() << *iter;
    //}
    QHash<QString, HostInfo> common;
    QHash<QString, HostInfo> wildcard;
    for(int i = 0; i < blocks.length(); i++) {
        QStringList host = blocks.at(i);
        QStringList memos;
        HostInfo hi;
        //qDebug() << host;
        for(int j = 0; j < host.length(); j++) {
            QString item = host.at(j).trimmed();
            if(item.startsWith("Host ")) {
                hi.name = item.mid(5).trimmed();
            }else if(item.startsWith("HostName ")) {
                hi.host = item.mid(9).trimmed();
            }else if(item.startsWith("Port ")) {
                hi.port = item.mid(5).trimmed().toInt();
            }else if(item.startsWith("User ")) {
                hi.user = item.mid(5).trimmed();
            }else if(item.startsWith("IdentityFile ")) {
                hi.identityFile = item.mid(13).trimmed();
            }else if(item.startsWith("Password")) {
                hi.password = item.mid(9).trimmed();
            }else if(item.startsWith("ProxyJump")) {
                hi.proxyJump = item.mid(9).trimmed();
            }else if(item.startsWith("Property")) {
                hi.property = item.mid(8).trimmed();
            }else if(item.startsWith("#")) {
                memos.push_back(item.mid(1));
            }
        }
        hi.memo = memos.join("\n");
        QStringList names = hi.name.split(' ');
        for(int i = 0; i < names.length(); i++) {
            QString name = names.at(i).trimmed();
            if(name.isEmpty()) {
                continue;
            }
            hi.name = name;
            if(name.contains("*")) {
                wildcard.insert(hi.name, hi);
            }else{
                common.insert(hi.name, hi);
            }
        }
    }
    QHash<QString, HostInfo> hosts;
    for(QHash<QString,HostInfo>::iterator iter = common.begin(); iter != common.end(); iter++) {
        QString name = iter.key();
        HostInfo hi = iter.value();
        HostInfo hiTmp;
        bool hitWildcard = false;
        for(QHash<QString,HostInfo>::iterator iter = wildcard.begin(); iter != wildcard.end(); iter++) {
            QString nameHit = iter.key();
            HostInfo hiHit = iter.value();
            QRegExp rx(nameHit);
            rx.setPatternSyntax(QRegExp::Wildcard);
            if(rx.exactMatch(name)) {
                hitWildcard = true;
                copyHostInfo(hiTmp, hiHit);
            }
        }
        if(hitWildcard) {
            copyHostInfo(hi, hiTmp);
        }

        hosts.insert(hi.name, hi);
    }

    return hosts;
}

QByteArray QWoSshConf::toStream()
{
    QByteArray buf;
    QBuffer file(&buf);
    file.open(QIODevice::WriteOnly);
    QList<QString> hosts = m_hosts.keys();
    std::sort(hosts.begin(), hosts.end());
    for(int i = 0; i < hosts.length(); i++) {
        HostInfo hi = m_hosts[hosts.at(i)];
        file.write("\n", 1);
        if(!hi.memo.isEmpty()) {
            QStringList comments = hi.memo.split('\n');
            for(int j = 0; j < comments.length(); j++) {
                QString line(QString("#%1\n").arg(comments.at(j)));
                file.write(line.toUtf8());
            }
        }
        if(!hi.name.isEmpty()) {
            QString line(QString("Host %1\n").arg(hi.name));
            file.write(line.toUtf8());
        }
        if(1){
            QString line(QString("  IgnoreUnknown %1\n").arg(IGNORE_UNKNOW));
            file.write(line.toUtf8());
        }
        if(!hi.host.isEmpty()) {
            QString line(QString("  HostName %1\n").arg(hi.host));
            file.write(line.toUtf8());
        }
        if(hi.port > 0) {
            QString line(QString("  Port %1\n").arg(hi.port));
            file.write(line.toUtf8());
        }
        if(!hi.user.isEmpty()) {
            QString line(QString("  User %1\n").arg(hi.user));
            file.write(line.toUtf8());
        }
        if(!hi.identityFile.isEmpty()) {
            QString line(QString("  IdentityFile %1\n").arg(hi.identityFile));
            file.write(line.toUtf8());
        }
        if(!hi.password.isEmpty()) {
            QString line(QString("  Password %1\n").arg(hi.password));
            file.write(line.toUtf8());
        }
        if(!hi.proxyJump.isEmpty()) {
            QString line(QString("  ProxyJump %1\n").arg(hi.proxyJump));
            file.write(line.toUtf8());
        }
        if(!hi.property.isEmpty()) {
            QString line(QString("  Property %1\n").arg(hi.property));
            file.write(line.toUtf8());
        }
    }
    return buf;
}

bool QWoSshConf::save()
{
    return exportToFile(m_conf);
}

bool QWoSshConf::refresh()
{
    QFile file(m_conf);
    if(!file.exists()) {
        return false;
    }
    if(!file.open(QFile::ReadOnly)) {
      QMessageBox::warning(nullptr, tr("LoadSshConfig"), tr("Failed to open file:")+m_conf, QMessageBox::Ok);
      return false;
    }
    QByteArray buf = file.readAll();
    buf = QWoUtils::fromWotermStream(buf);
    //qDebug() << buf;
    m_hosts = parse(buf);
    return true;
}

bool QWoSshConf::exportToFile(const QString &path)
{
    QByteArray buf = toStream();
    //qDebug() << buf;

    buf = QWoUtils::toWotermStream(buf);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)){
        return false;
    }
    file.write(buf);
    return true;
}

void QWoSshConf::remove(const QString &name)
{
    m_hosts.remove(name);
    save();
}

bool QWoSshConf::modify(const HostInfo &hi)
{
    if(!m_hosts.contains(hi.name)) {
        return false;
    }
    m_hosts.insert(hi.name, hi);
    return save();
}

bool QWoSshConf::append(const HostInfo &hi)
{
    if(m_hosts.contains(hi.name)) {
        return false;
    }
    m_hosts.insert(hi.name, hi);
    return save();
}

bool QWoSshConf::modifyOrAppend(const HostInfo &hi)
{
    m_hosts.insert(hi.name, hi);
    return save();
}

void QWoSshConf::resetAllProperty(const QString &v)
{
    for(QHash<QString, HostInfo>::iterator iter = m_hosts.begin(); iter != m_hosts.end(); iter++) {
        HostInfo& hi = iter.value();
        hi.property = v;
    }
    save();
}

bool QWoSshConf::exists(const QString &name)
{
    return m_hosts.contains(name);
}

void QWoSshConf::updatePassword(const QString &name, const QString &password)
{
    if(m_hosts.contains(name)) {
        HostInfo &hi = m_hosts[name];
        hi.password = password;
        save();
    }
}

QList<HostInfo> QWoSshConf::hostList() const
{
    QList<HostInfo> hosts = m_hosts.values();
    std::sort(hosts.begin(), hosts.end(), lessThan);
    return hosts;
}

QStringList QWoSshConf::hostNameList() const
{
    QStringList names = m_hosts.keys();
    std::sort(names.begin(), names.end());
    return names;
}

HostInfo QWoSshConf::find(const QString &name) {
    return m_hosts.value(name);
}
