#ifndef QWOTREEVIEW_H
#define QWOTREEVIEW_H

#include <QTreeView>

class QWoTreeView : public QTreeView
{
    Q_OBJECT
public:
    explicit QWoTreeView(QWidget *parent=nullptr);
signals:
    void itemChanged(const QModelIndex& idx);
    void returnKeyPressed();
protected:
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void currentChanged(const QModelIndex &current, const QModelIndex &previous);
};

#endif // QWoTreeView_H
