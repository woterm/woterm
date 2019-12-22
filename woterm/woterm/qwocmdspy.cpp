#include "qwocmdspy.h"
#include <QDebug>

static const int fix_collumn = 1024;
static const int fix_row = 768;

QWoCmdSpy::QWoCmdSpy(QObject *parent)
    : QObject(parent)
    , VTE(fix_collumn, fix_row)
{
    reset();
}

void QWoCmdSpy::reset()
{
    VTE::reset();
    m_lines.clear();
    m_tops.clear();
    for(int r = 0; r < fix_row; r++) {
        QString line = emptyLine();
        m_lines.push_back(line);
    }
}

void QWoCmdSpy::process(const QByteArray &buf)
{
    m_stop = false;
    for(int i = 0; i < buf.count(); i++) {
        if(m_stop){
            return;
        }
        VTE::process(buf.at(i));
    }
}

QList<QString> QWoCmdSpy::output()
{
    QList<QString> bufs;
    while(m_tops.count() > 0) {
        QString line = m_tops.takeFirst().simplified();
        if(!line.isEmpty()) {
            bufs.append(line);
        }
    }
    while (m_lines.count() > 0) {
        QString line = m_lines.takeFirst().simplified();
        if(!line.isEmpty()) {
            bufs.append(line);
        }
    }
    return bufs;
}

void QWoCmdSpy::reset_color(int c)
{
    qDebug() << "reset_color:" << c;
}

void QWoCmdSpy::define_color(int c, const char *s)
{
    qDebug() << "define_color";
}

void QWoCmdSpy::define_charset(int n, VTE::Charset cs)
{
    qDebug() << "define_charset";
}

void QWoCmdSpy::select_charset(int n)
{
    qDebug() << "select_charset";
}

void QWoCmdSpy::show_cursor(const VTE::Cursor &cursor)
{
    qDebug() << "show_cursor";
}

void QWoCmdSpy::hide_cursor(const VTE::Cursor &cursor)
{
    qDebug() << "hide_cursor";
}

void QWoCmdSpy::set_pixel(int x, int y)
{
    qDebug() << "set_pixel";
}

void QWoCmdSpy::clear_pixel(int x, int y)
{
    qDebug() << "clear_pixel";
}

void QWoCmdSpy::clear_tabs()
{
    qDebug() << "clear_tabs";
}

void QWoCmdSpy::set_tab(int x)
{
    qDebug() << "set_tab";
}

void QWoCmdSpy::reset_tab(int x)
{
    qDebug() << "reset_tab";
}

int QWoCmdSpy::get_next_tab(int x, int n)
{
    qDebug() << "get_next_tab";
    return n+1;
}

int QWoCmdSpy::get_char_width(uint32_t c)
{
    qDebug() << "get_char_width";
    return 1;
}

void QWoCmdSpy::put_reversed_question_mark(const VTE::Cursor &cursor)
{
    qDebug() << "put_reversed_question_mark";
}

void QWoCmdSpy::put_char(uint32_t c, int char_width, const VTE::Cursor &cursor)
{
    qDebug() << "put_char" << QChar(c);
    m_lines[cursor.y][cursor.x] = QChar(c);
}

bool QWoCmdSpy::request_width(int width)
{
    qDebug() << "request_width";
    return true;
}

void QWoCmdSpy::clear_region(int x1, int y1, int x2, int y2, const VTE::Cursor &cursor)
{
    qDebug() << "clear_region";
}

void QWoCmdSpy::insert_chars(int x, int n, const VTE::Cursor &cursor)
{
    qDebug() << "insert_chars";
}

void QWoCmdSpy::delete_chars(int x, int n, const VTE::Cursor &cursor)
{
    qDebug() << "delete_chars";
}

void QWoCmdSpy::scroll_up(int y1, int y2, int n, const VTE::Cursor &cursor)
{
    qDebug() << "scroll_up";
    for(int i = 0; i < n; i++){
        m_tops.append(m_lines.takeFirst());
        QString line = emptyLine();
        m_lines.append(line);
    }
}

void QWoCmdSpy::scroll_down(int y1, int y2, int n, const VTE::Cursor &cursor)
{
    qDebug() << "scroll_down";
}

void QWoCmdSpy::bell()
{
    qDebug() << "bell";
}

void QWoCmdSpy::set_line_text_size(int y, VTE::TextSize size)
{
    qDebug() << "set_line_text_size";
}

void QWoCmdSpy::tty_write(const char *s)
{
    qDebug() << "tty_write" << s;
}

void QWoCmdSpy::set_window_title(const char *title)
{
    qDebug() << "set_window_title" << title;
    m_stop = true;
    emit inputArrived();
}

void QWoCmdSpy::flag_lock_keyboard(bool state)
{
    qDebug() << "flag_lock_keyboard" << state;
}

void QWoCmdSpy::flag_application_keypad(bool state)
{
    emit applicationKeypad(state);
    qDebug() << "flag_application_keypad" << state;
}

void QWoCmdSpy::flag_cursor_visible(bool state)
{
    qDebug() << "flag_cursor_visible" << state;
}

void QWoCmdSpy::flag_soft_scroll(bool state)
{
    qDebug() << "flag_soft_scroll" << state;
}

void QWoCmdSpy::flag_screen_reversed(bool state)
{
    qDebug() << "flag_screen_reversed" << state;
}

QString QWoCmdSpy::emptyLine()
{
    QString line;
    for(int c = 0; c < fix_collumn; c++){
        line.push_back(QChar(' '));
    }
    return line;
}
