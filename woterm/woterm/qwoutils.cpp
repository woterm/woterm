#include "qwoutils.h"

#include <QObject>
#include<QLayout>
#include<QWidget>
#include <QSpacerItem>
#include <QDebug>
#include <QBoxLayout>
#include <QDataStream>
#include <QByteArray>

void QWoUtils::setLayoutVisible(QLayout *layout, bool visible)
{
    QBoxLayout *box = qobject_cast<QBoxLayout*>(layout);
    for (int i = 0; i < layout->count(); ++i) {
        QLayoutItem *item = layout->itemAt(i);
        QWidget *w = item->widget();
        if(w) {
            w->setVisible(visible);
        }
    }
}

QString QWoUtils::qVariantToBase64(const QVariant& v)
{
    QByteArray buf;
    QDataStream in(&buf, QIODevice::WriteOnly);
    in << v;
    return QString(buf.toBase64());
}

QVariant QWoUtils::qBase64ToVariant(const QString& v)
{
    QByteArray buf = QByteArray::fromBase64(v.toUtf8());
    QDataStream out(buf);
    QVariant data;
    out >> data;
    return data;
}

QStringList QWoUtils::parseCombinedArgString(const QString &program)
{
    QStringList args;
    QString tmp;
    int quoteCount = 0;
    bool inQuote = false;

    // handle quoting. tokens can be surrounded by double quotes
    // "hello world". three consecutive double quotes represent
    // the quote character itself.
    for (int i = 0; i < program.size(); ++i) {
        if (program.at(i) == QLatin1Char('"')) {
            ++quoteCount;
            if (quoteCount == 3) {
                // third consecutive quote
                quoteCount = 0;
                tmp += program.at(i);
            }
            continue;
        }
        if (quoteCount) {
            if (quoteCount == 1)
                inQuote = !inQuote;
            quoteCount = 0;
        }
        if (!inQuote && program.at(i).isSpace()) {
            if (!tmp.isEmpty()) {
                args += tmp;
                tmp.clear();
            }
        } else {
            tmp += program.at(i);
        }
    }
    if (!tmp.isEmpty())
        args += tmp;

    return args;
}


void rc4_init(unsigned char *s, unsigned char *key, unsigned long Len) //初始化函数
{
    int i =0, j = 0;
    unsigned char k[256] = {0};
    unsigned char tmp = 0;
    for (i=0;i<256;i++) {
        s[i] = i;
        k[i] = key[i%Len];
    }
    for (i=0; i<256; i++) {
        j=(j+s[i]+k[i])%256;
        tmp = s[i];
        s[i] = s[j]; //交换s[i]和s[j]
        s[j] = tmp;
    }
}

void rc4_crypt(unsigned char *s, unsigned char *Data, unsigned long Len) //加解密
{
    int i = 0, j = 0, t = 0;
    long k = 0;
    unsigned char tmp;
    for(k=0;k<Len;k++) {
        i=(i+1)%256;
        j=(j+s[i])%256;
        tmp = s[i];
        s[i] = s[j]; //交换s[x]和s[y]
        s[j] = tmp;
        t=(s[i]+s[j])%256;
        Data[k] ^= s[t];
    }
}

QByteArray QWoUtils::rc4(const QByteArray &data, const QByteArray &key)
{
    unsigned char s[256] = {0};
    QByteArray buf(data);
    rc4_init(s, (unsigned char *)key.data(),  key.length());
    rc4_crypt(s, (unsigned char *)buf.data(), buf.length());
    return buf;
}

QString QWoUtils::nameToPath(const QString &name)
{
    return name.toUtf8().toBase64(QByteArray::OmitTrailingEquals|QByteArray::Base64UrlEncoding);
}

QString QWoUtils::pathToName(const QString &path)
{
    return QByteArray::fromBase64(path.toUtf8(), QByteArray::OmitTrailingEquals|QByteArray::Base64UrlEncoding);
}

int QWoUtils::versionToLong(const QString &ver)
{
    QStringList parts = ver.split('.');
    if(parts.length() != 3) {
        return 0;
    }
    int ver_major = parts.at(0).toInt() * 100000;
    int ver_minor = parts.at(1).toInt() * 1000;
    int ver_minor2 = parts.at(2).toInt();
    return ver_major + ver_minor + ver_minor2;
}
