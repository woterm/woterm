#include "qwocommandhistorymodel.h"
#include "qwosshconf.h"
#include "qwoutils.h"
#include "qwosetting.h"

#include <QPair>
#include <QFontMetrics>
#include <QVector>
#include <QBuffer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>

#define MAX_HISTORY_COMMAND_COUNT   (128)

QWoCommandHistoryModel::QWoCommandHistoryModel(const QString &name, QObject *parent)
    : QAbstractListModel (parent)
    , m_name(name)
    , m_maxColumn(1)
    , m_refCount(0)
{
    m_historyCommands.reserve(MAX_HISTORY_COMMAND_COUNT);
    reload();
}

QWoCommandHistoryModel::~QWoCommandHistoryModel()
{

}

void QWoCommandHistoryModel::setMaxColumnCount(int cnt)
{
    m_maxColumn = cnt;
}

int QWoCommandHistoryModel::widthColumn(const QFont &ft, int i)
{
    QFontMetrics fm(ft);
    QList<HostInfo> his = QWoSshConf::instance()->hostList();
    int maxWidth = 0;
    for(int i = 0; i < his.count(); i++) {
        int w = fm.width(his.at(i).name);
        if(maxWidth < w) {
            maxWidth = w;
        }
    }
    return maxWidth;
}

bool QWoCommandHistoryModel::add(const QString &command, const QString &path)
{
    if(command.isEmpty()) {
        return false;
    }
    bool ok = _add(command, path);
    if(ok){
        emit beginResetModel();
        emit endResetModel();
    }
    return ok;
}

bool QWoCommandHistoryModel::_add(const QString &command, const QString &path)
{
    HistoryCommand hc;
    hc.cmd = command;
    hc.path = path;
    if(m_historyCommands.length() >= MAX_HISTORY_COMMAND_COUNT) {
        m_historyCommands.pop_back();
    }
    int idx = m_historyCommands.indexOf(hc);
    if(idx > 0 && idx >= m_historyCommands.length() - 1) {
        return false;
    }
    if(idx < 0) {
        m_historyCommands.insert(0, hc);
        return true;
    }
    m_historyCommands.removeAt(idx);
    m_historyCommands.insert(0, hc);
    return true;
}

bool QWoCommandHistoryModel::reload()
{
    QString path = QWoSetting::historyFilePath() + "/" + QWoUtils::nameToPath(m_name);
    QFile file(path);
    if(!file.exists()) {
        return false;
    }
    if(!file.open(QFile::ReadOnly)) {
        return false;
    }
    emit beginResetModel();
    QByteArray buf = file.readAll();
    QByteArrayList lines = buf.split('\n');
    for(int i = 0; i < lines.length(); i++) {
        QJsonDocument doc = QJsonDocument::fromJson(lines.at(i));
        if(doc.isObject()) {
            QJsonObject root = doc.object();
            QString command = root.value("command").toString();
            QString other = root.value("path").toString();
            _add(command, other);
        }
    }
    emit endResetModel();
    return true;
}

void QWoCommandHistoryModel::save()
{
    QByteArray buf;
    {
        QBuffer file(&buf);
        file.open(QIODevice::WriteOnly);
        for(int i = 0; i < m_historyCommands.length(); i++) {
            const HistoryCommand& hc = m_historyCommands.at(i);
            QJsonObject root;
            root.insert("path", hc.path);
            root.insert("command", hc.cmd);
            QJsonDocument doc;
            doc.setObject(root);
            QByteArray jsdoc = doc.toJson(QJsonDocument::Compact) + "\n";
            file.write(jsdoc);
        }
    }
    {
        QString path = QWoSetting::historyFilePath() + "/" + QWoUtils::nameToPath(m_name);
        QFile file(path);
        if(file.open(QFile::WriteOnly)) {
            file.write(buf);
        }
    }
}

void QWoCommandHistoryModel::reset(const QList<HistoryCommand> &cmds)
{
    if(QWoSshConf::instance()->refresh()){
        emit beginResetModel();
        m_historyCommands = cmds;
        emit endResetModel();
    }
}

int QWoCommandHistoryModel::inscreaseReferCount()
{
    m_refCount++;
    return m_refCount;
}

int QWoCommandHistoryModel::descreaseReferCount()
{
    m_refCount--;
    return m_refCount;
}


int QWoCommandHistoryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()){
        return 0;
    }    
    return m_historyCommands.count();
}

QModelIndex QWoCommandHistoryModel::sibling(int row, int column, const QModelIndex &idx) const
{
    if (!idx.isValid() || column != 0 || row >= 2 || row < 0)
        return QModelIndex();

    return createIndex(row, 0);
}

QVariant QWoCommandHistoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role == Qt::DisplayRole) {
        switch (section) {
        case 0:
            return tr("Command");
        case 1:
            return tr("Path");
        }
    }
    return QAbstractListModel::headerData(section, orientation, role);
}

QVariant QWoCommandHistoryModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_historyCommands.size()) {
        return QVariant();
    }
    if (!index.isValid()){
        return QVariant();
    }
    const HistoryCommand& hc = m_historyCommands.at(index.row());

    if(role == Qt::ToolTipRole) {        
        return hc.cmd +"-"+ hc.path;
    }

    if(role == ROLE_INDEX) {
        return index.row();
    }

    if(role == ROLE_CMDINFO) {
        QVariant v;
        v.setValue(hc);
        return v;
    }

    if(role == ROLE_REFILTER) {
        return hc.cmd + '-' + hc.path;
    }

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        int col = index.column();
        switch (col) {
        case 0:
            return QVariant(hc.cmd);
        case 1:
            return QVariant(hc.path);
        }
    }
    return QVariant();
}

bool QWoCommandHistoryModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    return false;
}

Qt::ItemFlags QWoCommandHistoryModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()){
        return QAbstractListModel::flags(index) | Qt::ItemIsDropEnabled;
    }
    return QAbstractListModel::flags(index) | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

bool QWoCommandHistoryModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (count < 1 || row < 0 || row > rowCount(parent)){
        return false;
    }

    beginInsertRows(QModelIndex(), row, row + count - 1);

    for (int r = 0; r < count; ++r){
        m_historyCommands.insert(row, HistoryCommand());
    }

    endInsertRows();

    return QAbstractListModel::insertRows(row, count, parent);
}

bool QWoCommandHistoryModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (count <= 0 || row < 0 || (row + count) > rowCount(parent)){
        return false;
    }

    beginRemoveRows(QModelIndex(), row, row + count - 1);

    const auto it = m_historyCommands.begin() + row;
    m_historyCommands.erase(it, it + count);

    endRemoveRows();
    save();
    return true;
}

static bool ascendingLessThan(const QPair<QString, int> &s1, const QPair<QString, int> &s2)
{
    return s1.first < s2.first;
}

static bool decendingLessThan(const QPair<QString, int> &s1, const QPair<QString, int> &s2)
{
    return s1.first > s2.first;
}

void QWoCommandHistoryModel::sort(int column, Qt::SortOrder order)
{
#if 1
    return QAbstractListModel::sort(column, order);
#else
    emit layoutAboutToBeChanged(QList<QPersistentModelIndex>(), VerticalSortHint);

     QVector<QPair<QString, HostInfo> > list;
     const int lstCount = m_hosts.count();
     list.reserve(lstCount);
     for (int i = 0; i < lstCount; ++i){
         list.append(QPair<QString, HostInfo>(m_hosts.at(i), i));
     }

     if (order == Qt::AscendingOrder)
         std::sort(list.begin(), list.end(), ascendingLessThan);
     else
         std::sort(list.begin(), list.end(), decendingLessThan);

     m_hosts.clear();
     QVector<int> forwarding(lstCount);
     for (int i = 0; i < lstCount; ++i) {
         m_hosts.append(list.at(i).first);
         forwarding[list.at(i).second] = i;
     }

     QModelIndexList oldList = persistentIndexList();
     QModelIndexList newList;
     const int numOldIndexes = oldList.count();
     newList.reserve(numOldIndexes);
     for (int i = 0; i < numOldIndexes; ++i)
         newList.append(index(forwarding.at(oldList.at(i).row()), 0));
     changePersistentIndexList(oldList, newList);

     emit layoutChanged(QList<QPersistentModelIndex>(), VerticalSortHint);
#endif
}

int QWoCommandHistoryModel::columnCount(const QModelIndex &parent) const
{
    return m_maxColumn;
}

Qt::DropActions QWoCommandHistoryModel::supportedDropActions() const
{
    return QAbstractListModel::supportedDropActions();
}
