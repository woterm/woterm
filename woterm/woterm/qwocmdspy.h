#pragma once

#include "vte.h"
#include <QObject>


class QWoCmdSpy: public QObject, public VTE
{
    Q_OBJECT
public:
    explicit QWoCmdSpy(QObject *parent);
    void reset();
    void process(const QByteArray& buf);
    QList<QString> output();
signals:
    void inputArrived();
    void applicationKeypad(bool state);
private:
    virtual void reset_color(int c);
    virtual void define_color(int c, const char *s);
    virtual void define_charset(int n, Charset cs);
    virtual void select_charset(int n);
    virtual void show_cursor(const Cursor& cursor);
    virtual void hide_cursor(const Cursor& cursor);
    virtual void set_pixel(int x, int y);
    virtual void clear_pixel(int x, int y);
    virtual void clear_tabs();
    virtual void set_tab(int x);
    virtual void reset_tab(int x);
    virtual int get_next_tab(int x, int n);
    virtual int get_char_width(uint32_t c);
    virtual void put_reversed_question_mark(const Cursor& cursor);
    virtual void put_char(uint32_t c, int char_width, const Cursor& cursor);
    virtual bool request_width(int width);
    virtual void clear_region(int x1, int y1, int x2, int y2, const Cursor& cursor);
    virtual void insert_chars(int x, int n, const Cursor& cursor);
    virtual void delete_chars(int x, int n, const Cursor& cursor);
    virtual void scroll_up(int y1, int y2, int n, const Cursor& cursor);
    virtual void scroll_down(int y1, int y2, int n, const Cursor& cursor);
    virtual void bell();
    virtual void set_line_text_size(int y, TextSize size);
    virtual void tty_write(const char *s);
    virtual void set_window_title(const char *title);
    virtual void flag_lock_keyboard(bool state);
    virtual void flag_application_keypad(bool state);
    virtual void flag_cursor_visible(bool state);
    virtual void flag_soft_scroll(bool state);
    virtual void flag_screen_reversed(bool state);
private:
    QString emptyLine();
private:
    QList<QString> m_tops;
    QList<QString> m_lines;
    bool m_stop;
};
