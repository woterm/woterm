#ifndef QWOHOSTINFILIST_H
#define QWOHOSTINFILIST_H

#include <QDialog>

namespace Ui {
class QWoHostInfoList;
}

class QWoHostInfoList : public QDialog
{
    Q_OBJECT

public:
    explicit QWoHostInfoList(QWidget *parent = nullptr);
    ~QWoHostInfoList();

private:
    Ui::QWoHostInfoList *ui;
};

#endif // QWOHOSTINFILIST_H
