#pragma once

#include <QVariant>
#include <QString>

class QLayout;

class QWoUtils
{
public:
    static void setLayoutVisible(QLayout* layout, bool visible);
    static QString qVariantToBase64(const QVariant& v);
    static QVariant qBase64ToVariant(const QString& v);
    static QStringList parseCombinedArgString(const QString &program);
};
