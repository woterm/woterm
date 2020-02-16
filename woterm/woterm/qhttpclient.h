#ifndef QHTTPCLIENT_H
#define QHTTPCLIENT_H

#include <QPointer>
#include <QObject>
#include <QNetworkReply>

class QHttpClient : public QObject
{
    Q_OBJECT
public:
    explicit QHttpClient(QObject *parent = nullptr);
    ~QHttpClient();

    void get(const QString& url, QObject* receiver, const char* method);
    void post(const QString& url, const QByteArray& data, QObject* receiver, const char* method);

signals:
    void result(int code, const QByteArray& body);
private slots:
    void onFinished();

private:
    QNetworkReply *get(const QNetworkRequest& request);
    QNetworkReply *post(const QNetworkRequest& request, const QByteArray& data);

private:
    QPointer<QNetworkAccessManager> m_networkAccessManger;
};

#endif // QHTTPCLIENT_H
