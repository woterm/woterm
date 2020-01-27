#include <QApplication>
#include <QtDebug>
#include <QIcon>
#include <QMenuBar>
#include <QProcess>
#include <QMessageBox>
#include <QList>
#include <QByteArray>

#include "qwoglobal.h"
#include "qwomainwindow.h"
#include "qwotermwidget.h"
#include "qwosshprocess.h"
#include "qwosetting.h"
#include "qwosshconf.h"
#include "qwotermstyle.h"
#include "qwoutils.h"

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    static QApplication app(argc, argv);
#else
    QApplication app(argc, argv);
#endif
    QApplication::setStyle(new QWoTermStyle());
    QApplication::setWindowIcon(QIcon(":/qwoterm/resource/skin/woterm4.png"));

    QString path = QApplication::applicationDirPath();
    qDebug() << "path=" << path;
    QApplication::addLibraryPath(path);
    QString libPath = QDir::cleanPath(path + "/../lib");
    if(QFile::exists(libPath)) {
        QApplication::addLibraryPath(libPath);
    }

    QTermWidget::setDefaultColorSchemaDir(QWoSetting::privateColorSchemaPath());
    QTermWidget::setDefaultKeyboardDir(QWoSetting::privateKeyboardLayoutPath());
    QTermWidget::setDefaultTranslationDir(QWoSetting::privateTranslationPath());

    QWoMainWindow *mainWindow = QWoMainWindow::instance();
    mainWindow->show();

    if(app.arguments().length() <= 1) {
        QTimer::singleShot(0, mainWindow, SLOT(onAppStart()));
    }

    return app.exec();
}
