#pragma once

#include "qwoglobal.h"

#include <QAbstractItemModel>
#include <QFont>

class QWoHostTreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit QWoHostTreeModel(QObject *parent = nullptr);
    virtual ~QWoHostTreeModel() override;

    static QWoHostTreeModel *instance();

    void setMaxColumnCount(int cnt);
    int widthColumn(const QFont& ft, int i);

    void refreshList();
    void add(const HostInfo& hi);
    QList<int> exists(const QString &name);
    void resetAllProperty(QString v);
    void modify(int idx, const HostInfo &hi);
    void append(const HostInfo& hi);
private:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex sibling(int row, int column, const QModelIndex &idx) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    Qt::DropActions supportedDropActions() const override;
private:
    void clear();
private:
    Q_DISABLE_COPY(QWoHostTreeModel)
    struct TreeItem{
        QString name;
        HostInfo hi;
        TreeItem *parent;
        QList<TreeItem*> childs;
    };
    TreeItem *m_root;
    int m_maxColumn;
};
