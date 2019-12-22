#ifndef QWOSCRIPSELECTOR_H
#define QWOSCRIPSELECTOR_H

#include <QWidget>
#include <QMap>

namespace Ui {
class QWoScriptSelector;
}

class QWoScriptSelector : public QWidget
{
    Q_OBJECT
private:
    typedef struct{
        QString name;
        QString entry;
        QStringList args;
        QStringList files;
    }Task;
public:
    explicit QWoScriptSelector(QWidget *parent = nullptr);
    ~QWoScriptSelector();

signals:
    void readyRun(const QString& fullPath, const QStringList& scriptFiles, const QString& entry, const QStringList& args);
    void readyStop();
    void readyDebug(const QString& fullPath, const QStringList& scriptFiles, const QString& entry, const QStringList& args);

private slots:
    void onBrowserScriptFile();
    void onReloadScriptFile();
    void onBtnRunClicked();
    void onBtnDebugClicked();
    void onLogArrived(const QString& level, const QString& msg);
    void onCurrentIndexChanged(const QString& txt);
private:
    void reinit();
    bool parse();
    Task reset();
private:
    Ui::QWoScriptSelector *ui;
    QString m_fullPath;
    QMap<QString, Task> m_tasks;
};

#endif // QWOSCRIPSELECTOR_H
