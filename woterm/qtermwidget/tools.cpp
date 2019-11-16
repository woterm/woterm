#include "tools.h"

#include <QCoreApplication>
#include <QDir>
#include <QtDebug>


/*! Helper function to get possible location of layout files.
By default the KB_LAYOUT_DIR is used (linux/BSD/macports).
But in some cases (apple bundle) there can be more locations).
*/

#define KB_LAYOUT_DIR "./kb-layouts"
QString get_kb_layout_dir()
{
    QString rval = QString();
    QString binPath = QCoreApplication::applicationDirPath();
    QString path = QDir::cleanPath(binPath +"/../opt/" + KB_LAYOUT_DIR+"/");
    qDebug() << "default KB_LAYOUT_DIR: " << path;
    QDir d(path);
    if (d.exists()) {
        return path;
    }
    return QString();
}

/*! Helper function to add custom location of color schemes.
*/
namespace {
    QStringList custom_color_schemes_dirs;
}
void add_custom_color_scheme_dir(const QString& custom_dir)
{
    if (!custom_color_schemes_dirs.contains(custom_dir))
        custom_color_schemes_dirs << custom_dir;
}

/*! Helper function to get possible locations of color schemes.
By default the COLORSCHEMES_DIR is used (linux/BSD/macports).
But in some cases (apple bundle) there can be more locations).
*/

#define COLORSCHEMES_DIR "/color-schemes"

QString get_color_schemes_dir()
{
    QString binPath = QCoreApplication::applicationDirPath();
    QString path = QDir::cleanPath(binPath +"/../opt/" + COLORSCHEMES_DIR);
    qDebug() << "default COLORSCHEMES_DIR: " << path;
    QDir d(path);
    if (d.exists()) {
        return path;
    }
    return QString();
}
