#ifndef QHTTPCLIENT_H
#define QHTTPCLIENT_H

#include <QPointer>
#include <QObject>
#include <QNetworkReply>

class QHttpClient : public QObject
{
    Q_OBJECT
public:
    static QNetworkReply *get(const QNetworkRequest& request);
    static QNetworkReply *post(const QNetworkRequest& request, const QByteArray& data);
    static QNetworkReply* get(const QString& url, QObject* receiver, const char* method);
    static QNetworkReply* post(const QString& url, const QByteArray& data, QObject* receiver, const char* method);
private:
    explicit QHttpClient(QObject *parent = nullptr);
signals:
    void result(int code, const QByteArray& body);
private slots:
    void onFinished();
};

#endif // QHTTPCLIENT_H
