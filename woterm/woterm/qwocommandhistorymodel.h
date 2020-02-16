#pragma once

#include "qwoglobal.h"

#include <QAbstractListModel>
#include <QFont>

#define ROLE_INDEX   (Qt::UserRole+1)
#define ROLE_CMDINFO (Qt::UserRole+2)
#define ROLE_REFILTER (Qt::UserRole+3)
#define ROLE_FRIENDLY_NAME (Qt::UserRole+4)

class QWoCommandHistoryModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit QWoCommandHistoryModel(const QString& name, QObject *parent = nullptr);
    virtual ~QWoCommandHistoryModel() override;

    void setMaxColumnCount(int cnt);
    int widthColumn(const QFont& ft, int i);

    bool add(const QString& cmd, const QString& path);
    void save();

    void reset(const QList<HistoryCommand>& cmds);
    bool exists(const QString &name);
    void resetAllProperty(QString v);
    void modifyOrAppend(const HostInfo& hi);

    int inscreaseReferCount();
    int descreaseReferCount();
private:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex sibling(int row, int column, const QModelIndex &idx) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    Qt::DropActions supportedDropActions() const override;
private:
    bool _add(const QString& cmd, const QString& path);
    bool reload();
private:
    Q_DISABLE_COPY(QWoCommandHistoryModel)
    const QString m_name;
    int m_maxColumn;
    QList<HistoryCommand> m_historyCommands;
    int m_refCount;
};
