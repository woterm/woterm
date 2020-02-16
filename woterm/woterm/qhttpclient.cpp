#include "qhttpclient.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QGlobalStatic>
#include <QMetaEnum>
#include <QMetaObject>


QHttpClient::QHttpClient(QObject *parent):
    QObject (parent)
{
    m_networkAccessManger = new QNetworkAccessManager(this);
}

QHttpClient::~QHttpClient()
{
    delete m_networkAccessManger;
}

QNetworkReply *QHttpClient::get(const QNetworkRequest &request)
{
    return m_networkAccessManger->get(request);
}

QNetworkReply *QHttpClient::post(const QNetworkRequest &request, const QByteArray &data)
{
    return m_networkAccessManger->post(request, data);
}

void QHttpClient::get(const QString &url, QObject* receiver, const char *method)
{
    QNetworkReply *reply = get(QNetworkRequest(url));
    QObject::connect(this, SIGNAL(result(int,const QByteArray&)), receiver, method);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(onFinished()));
}

void QHttpClient::post(const QString &url, const QByteArray &data, QObject *receiver, const char *method)
{
    QNetworkReply *reply = post(QNetworkRequest(url), data);
    QObject::connect(this, SIGNAL(result(int,const QByteArray&)), receiver, method);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(onFinish()));
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
