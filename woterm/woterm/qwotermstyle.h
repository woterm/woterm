#ifndef QWOTERMSTYLE_H
#define QWOTERMSTYLE_H

#include <QProxyStyle>
#include <QtWidgets>

class QWoTermStyle : public QProxyStyle
{
    Q_OBJECT
public:
    QWoTermStyle();

protected:
    QIcon standardIcon(QStyle::StandardPixmap standardIcon, const QStyleOption *option = nullptr, const QWidget *widget = nullptr) const;
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const override;
    void drawControl(QStyle::ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = nullptr) const override;
private:

};

#endif // QWOTERMSTYLE_H
