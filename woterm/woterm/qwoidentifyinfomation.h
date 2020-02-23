#ifndef QWOIDENTIFYINFOMATION_H
#define QWOIDENTIFYINFOMATION_H

#include <QObject>
#include <QPointer>
#include <QMap>

#include "qwoglobal.h"

class QEventLoop;

class QWoIdentifyInfomation : public QObject
{
    Q_OBJECT
public:
    explicit QWoIdentifyInfomation(QObject *parent = nullptr);
    bool identifyInfomation(const QString&file, IdentifyInfo *pinfo);
    bool identifyFileSet(const QString& file, const QString& name);
    static QMap<QString, IdentifyInfo> listAll();
public slots:
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onFinished(int code);
private:
    QMap<QString, QString> identifyFileGet(const QString &file);
    QString runProcess(const QStringList& args, const QStringList& envs=QStringList());
    bool wait(int ms = 30 * 60 * 1000, int *why = nullptr);
private:
    QPointer<QEventLoop> m_eventLoop;
};

#endif // QWOIDENTIFYINFOMATION_H
