#pragma once

#include <QObject>

class QAesWrap;

class QWoAes : public QObject
{
    Q_OBJECT
public:
    explicit QWoAes(const QByteArray & password, QObject *parent=nullptr);
    virtual ~QWoAes();

    bool encrypt(const QByteArray& in, QByteArray& out);
    bool decrypt(const QByteArray& in, QByteArray& out);
private:
    QAesWrap* m_aes;
};
