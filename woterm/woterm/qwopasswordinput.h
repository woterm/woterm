#ifndef QWOPASSWORDINPUT_H
#define QWOPASSWORDINPUT_H

#include "qwowidget.h"

namespace Ui {
class QWoPasswordInput;
}

class QWoPasswordInput : public QWoWidget
{
    Q_OBJECT

public:
    explicit QWoPasswordInput(QWidget *parent = nullptr);
    ~QWoPasswordInput();

    void reset(const QString& prompt, bool echo);

signals:
    void result(const QString& pass, bool isSave) const;
private slots:
    void onPasswordVisible(bool checked);
    void onClose();
private:
    void paintEvent(QPaintEvent* paint);
private:
    Ui::QWoPasswordInput *ui;
};

#endif // QWOPASSWORDINPUT_H
