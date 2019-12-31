#ifndef QWOLISTVIEW_H
#define QWOLISTVIEW_H

#include <QListView>

class QWoListView : public QListView
{
    Q_OBJECT
public:
    explicit QWoListView(QWidget *parent=nullptr);
signals:
    void itemChanged(const QModelIndex& idx);
    void returnKeyPressed();
protected:
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void currentChanged(const QModelIndex &current, const QModelIndex &previous);
};

#endif // QWOLISTVIEW_H
