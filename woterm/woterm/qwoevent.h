#pragma once

#include <QEvent>
#include <QVariant>


class QWoEvent : public QEvent
{
public:
    enum WoEventType{
        BeforeReadStdOut,
        AfterReadStdOut,
        BeforeReadStdErr,
        AfterReadStdErr,
        BeforeWriteStdOut,
        BeforeWriteStdErr,
        BeforeFinish,

        PropertyChanged,
    };
public:
    QWoEvent(WoEventType t, const QVariant& data = QVariant());

    WoEventType eventType() const;
    QVariant data() const;

    void setResult(const QVariant& result);
    QVariant result() const;
    bool hasResult();

    static int EventType;
private:
    WoEventType m_type;
    QVariant m_data;
    QVariant m_result;

};
