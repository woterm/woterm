#ifndef TOOLS_H
#define TOOLS_H

#include <QString>
#include <QStringList>

QString get_kb_layout_dir();
void add_custom_color_scheme_dir(const QString& custom_dir);
QString get_color_schemes_dir();
QString get_translation_dir();
void set_default_color_scheme_dir(const QString& path);
void set_default_keyboard_layout_dir(const QString& path);
void set_default_translation_dir(const QString& path);

#endif
