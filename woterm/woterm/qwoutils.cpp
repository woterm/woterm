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
