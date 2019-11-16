#include "qwoaes.h"
#include "qaeswrap.h"

//https://github.com/dushibaiyu/QAes
#define FIX_SALTS_STRING  ("woterm.wingo.he")
QWoAes::QWoAes(const QByteArray &password, QObject *parent)
    :QObject (parent)
{
   m_aes = new QAesWrap(password, FIX_SALTS_STRING, QAesWrap::AES_256);
}

QWoAes::~QWoAes()
{
    delete m_aes;
}

bool QWoAes::encrypt(const QByteArray &in, QByteArray &out)
{
    return m_aes->encrypt(in, out, QAesWrap::AES_CTR);
}

bool QWoAes::decrypt(const QByteArray &in, QByteArray &out)
{
    return m_aes->decrypt(in, out, QAesWrap::AES_CTR);
}
