#include "qhttpclient.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QGlobalStatic>
#include <QMetaEnum>
#include <QMetaObject>

Q_GLOBAL_STATIC(QNetworkAccessManager, getNetworkAccessManager)

QHttpClient::QHttpClient(QObject *parent):
    QObject (parent)
{

}

QNetworkReply *QHttpClient::get(const QNetworkRequest &request)
{
    return getNetworkAccessManager()->get(request);
}

QNetworkReply *QHttpClient::post(const QNetworkRequest &request, const QByteArray &data)
{
    return getNetworkAccessManager()->post(request, data);
}

QNetworkReply *QHttpClient::get(const QString &url, QObject* receiver, const char *method)
{
    QNetworkReply *reply = get(QNetworkRequest(url));
    QHttpClient *client = new QHttpClient();
    QObject::connect(client, SIGNAL(result(int,const QByteArray&)), receiver, method);
    QObject::connect(reply, SIGNAL(finished()), client, SLOT(onFinished()));
    return reply;
}

QNetworkReply *QHttpClient::post(const QString &url, const QByteArray &data, QObject *receiver, const char *method)
{
    QNetworkReply *reply = post(QNetworkRequest(url), data);
    QHttpClient *client = new QHttpClient();
    QObject::connect(client, SIGNAL(result(int,const QByteArray&)), receiver, method);
    QObject::connect(reply, SIGNAL(finished()), client, SLOT(onFinish()));
    return reply;
}

void QHttpClient::onFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    QNetworkReply::NetworkError err =  reply->error();
    reply->deleteLater();
    if( err > 0 ) {
        QMetaEnum meta = QMetaEnum::fromType<QNetworkReply::NetworkError>();
        QByteArray body = meta.valueToKey(err);
        emit result(-err, body);
        return;
    }
    int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray body = reply->readAll();
    emit result(code, body);
}
