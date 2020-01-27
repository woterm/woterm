#ifndef QWOIDENTIFYDIALOG_H
#define QWOIDENTIFYDIALOG_H

#include <QDialog>
#include <QPointer>

namespace Ui {
class QWoIdentifyDialog;
}

class QEventLoop;
class QTreeWidgetItem;

class QWoIdentifyDialog : public QDialog
{
    Q_OBJECT
private:
    typedef struct{
        QString name;
        QString fingureprint;
        QString key;
        QString type;
    } IdentifyInfo;
public:
    ~QWoIdentifyDialog();
    static QString open(QWidget *parent = nullptr);

protected:
    explicit QWoIdentifyDialog(QWidget *parent = nullptr);
    QString result() const;
private slots:
    void onButtonCreateClicked();
    void onButtonImportClicked();
    void onButtonExportClicked();
    void onButtonDeleteClicked();
    void onButtonSelectClicked();
    void onButtonRenameClicked();
    void onButtonViewClicked();
    void onItemDoubleClicked(QTreeWidgetItem*, int);

    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onFinished(int code);
private:
    void reload();
    QString nameToPath(const QString& name);
    QString pathToName(const QString& path);
    QMap<QString, QString> identifyFileGet(const QString &file);
    bool identifyFileSet(const QString& file, const QString& name);
    bool identifyInfomation(const QString&file, QWoIdentifyDialog::IdentifyInfo *pinfo);
    QByteArray toWotermIdentify(const QByteArray& data);
    QByteArray toStandardIdentify(const QByteArray& data);
    QString runProcess(const QStringList& args, const QStringList& envs=QStringList());
    bool wait(int ms = 30 * 60 * 1000, int *why = nullptr);
private:
    Ui::QWoIdentifyDialog *ui;
    QString m_result;
    QPointer<QEventLoop> m_eventLoop;
};

#endif // QWOIDENTIFYDIALOG_H
