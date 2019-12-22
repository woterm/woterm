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

