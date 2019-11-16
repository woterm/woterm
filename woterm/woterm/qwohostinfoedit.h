#ifndef QWOHOSTINFOEDIT_H
#define QWOHOSTINFOEDIT_H

#include "qwoglobal.h"

#include <QDialog>

namespace Ui {
class QWoHostInfo;
}

class QWoHostInfoEdit : public QDialog
{
    Q_OBJECT

public:
    explicit QWoHostInfoEdit(QWidget *parent = nullptr);
    explicit QWoHostInfoEdit(int idx, QWidget *parent = nullptr);
    ~QWoHostInfoEdit();

private slots:
    void onAuthCurrentIndexChanged(const QString & txt);
    void onButtonSaveClicked();
    void onButtonJumpBrowserClicked();
    void onButtonIdentifyBrowserClicked();
private:
    void init();
private:
    int m_idx;
    Ui::QWoHostInfo *ui;
};

#endif // QWOHOSTINFOEDIT_H
