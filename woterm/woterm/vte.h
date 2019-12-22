#ifndef _VTE_HPP_
#define _VTE_HPP_

#include <stdint.h>
#include <cstdlib>
#include <cstring>

class VTE {
private:
    VTE(const VTE&);
    VTE& operator=(const VTE&);

public:
    typedef unsigned char uchar;
    typedef unsigned int  uint;
    typedef uint32_t      u32;
    typedef uint16_t      u16;
    typedef uint8_t       u8;

    VTE(int width, int height);

    void reset();
    void process(char c);
    void process(const char *s);

protected:
    enum Charset {
        CS_UK = 0,
        CS_US,
        CS_G0,
        CS_G1
    };

    enum TextSize {
        TS_Normal = 0,
        TS_TopHalf,
        TS_BottomHalf,
        TS_DoubleWidth
    };

    static const int CharsetTableSize = 4;

    struct Cursor {
        struct Attributes {
            struct Color {
                enum Type {
                    CT_Default = 0,
                    CT_Indexed,
                    CT_Truecolor
                };

                Color() { reset(); }

                void reset() {
                    type = CT_Default;
                    color = 0;
                }

                Type type;
                uint32_t color;
            };

            enum AttributeFlags {
                AF_Underline  = (1 <<  0),
                AF_Reverse    = (1 <<  1),
                AF_Bold       = (1 <<  2),
                AF_Blink      = (1 <<  3),
                AF_HalfBright = (1 <<  4),
                AF_Italic     = (1 <<  5),
                AF_Invisible  = (1 <<  6)
            };

            Attributes() { reset(); }

            void reset() {
                fg.reset();
                bg.reset();
                flags = 0;
            }

            Color fg;
            Color bg;
            u32 flags;
        };

        enum CursorState {
            CS_Normal = 0,
            CS_WrapNext
        };

        Cursor() { reset(); }

        void reset() {
            attrs.reset();
            x = y = 0;
            cs = CS_Normal;
            for (int i = 0; i < CharsetTableSize; ++i) {
                charset_table[i] = CS_US;
            }
            selected_charset = 0;
        }

        int x;
        int y;
        Attributes attrs;
        CursorState cs;
        Charset charset_table[CharsetTableSize];
        int selected_charset;
    };

    const Cursor& get_cursor() const;

    // to implement
    virtual void reset_color(int c) = 0;
    virtual void define_color(int c, const char *s) = 0;
    virtual void define_charset(int n, Charset cs) = 0;
    virtual void select_charset(int n) = 0;
    virtual void show_cursor(const Cursor& cursor) = 0;
    virtual void hide_cursor(const Cursor& cursor) = 0;
    virtual void set_pixel(int x, int y) = 0;
    virtual void clear_pixel(int x, int y) = 0;
    virtual void clear_tabs() = 0;
    virtual void set_tab(int x) = 0;
    virtual void reset_tab(int x) = 0;
    virtual int get_next_tab(int x, int n) = 0;
    virtual int get_char_width(uint32_t c) = 0;
    virtual void put_reversed_question_mark(const Cursor& cursor) = 0;
    virtual void put_char(uint32_t c, int char_width, const Cursor& cursor) = 0;
    virtual bool request_width(int width) = 0;
    virtual void clear_region(int x1, int y1, int x2, int y2, const Cursor& cursor) = 0;
    virtual void insert_chars(int x, int n, const Cursor& cursor) = 0;
    virtual void delete_chars(int x, int n, const Cursor& cursor) = 0;
    virtual void scroll_up(int y1, int y2, int n, const Cursor& cursor) = 0;
    virtual void scroll_down(int y1, int y2, int n, const Cursor& cursor) = 0;
    virtual void bell() = 0;
    virtual void set_line_text_size(int y, TextSize size) = 0;
    virtual void tty_write(const char *s) = 0;
    virtual void set_window_title(const char *title) = 0;
    virtual void flag_lock_keyboard(bool state) = 0;
    virtual void flag_application_keypad(bool state) = 0;
    virtual void flag_cursor_visible(bool state) = 0;
    virtual void flag_soft_scroll(bool state) = 0;
    virtual void flag_screen_reversed(bool state) = 0;

private:
    typedef void (*ModeModifier)(u32& mode, int flag);

    static const int UTF8_BUF_LEN = 8;
    static const int STR_BUF_LEN = 32;

    enum TerminalMode {
        TM_Ground = 0,
        TM_ESC,
        TM_CSI,
        TM_Hash,
        TM_Charset,
        TM_UTF8,
        TM_String
    };

    enum TerminalFlags {
        TF_KAM     = (1 <<  0),     // keyboard action mode
        TF_IRM     = (1 <<  1),     // insert/replace mode
        TF_SRM     = (1 <<  2),     // send/receive mode
        TF_LNM     = (1 <<  3),     // line feedback/new line mode
        TF_DECTCEM = (1 <<  4),     // text cursor enable mode
        TF_DECCKM  = (1 <<  5),     // cursor keys mode
        TF_DECANM  = (1 <<  6),     // ANSI/VT52 mode
        TF_DECCOLM = (1 <<  7),     // column mode (80/132)
        TF_DECSCLM = (1 <<  8),     // scolling mode (soft scroll)
        TF_DECSCNM = (1 <<  9),     // screen mode
        TF_DECOM   = (1 << 10),     // origin mode
        TF_DECAWM  = (1 << 11),     // autowrap mode
        TF_DECARM  = (1 << 12),     // auto repeat mode
        TF_DECPFF  = (1 << 13),     // print form feed mode
        TF_DECPEX  = (1 << 14),     // print extent mode
        TF_DECKPAM = (1 << 15),     // keypad application mode
        TF_UTF8    = (1 << 16)      // UTF8 mode
    };

    struct ControlData {
        static const int CTRL_DATA_LEN = 32;

        ControlData() { reset(); }

        void reset() {
            memset(ctrl_data, 0, sizeof(ctrl_data));
            ctrl_next_data = ctrl_data;
            nargs = 1;
            dec_mode = false;
            ctrl_args[0] = ctrl_data;
        }

        int get_arg(int arg) {
            if (arg >= 0 && arg < nargs) {
                return atoi(ctrl_args[arg]);
            }
            return 0;
        }

        bool space_left() {
            return (ctrl_next_data < ctrl_data + CTRL_DATA_LEN - 2);
        }

        bool add_arg(char c) {
            if (c == '?' && ctrl_next_data == ctrl_data) {
                dec_mode = true;
                return true;
            } else if (c == ';') {
                ctrl_args[nargs++] = ++ctrl_next_data;
                return true;
            } else if (c >= '0' && c <= '9') {
                *ctrl_next_data++ = c;
                return true;
            }
            return false;
        }

        char ctrl_data[CTRL_DATA_LEN];
        char *ctrl_next_data;
        const char *ctrl_args[CTRL_DATA_LEN];
        int nargs;
        bool dec_mode;
    };

    struct StringData {
        static const int STRING_DATA_LEN = 256;
        static const int STRING_ARG_LEN = 32;

        enum StringType {
            ST_None = 0,
            ST_Title,
            ST_OSC,
            ST_PM,
            ST_DCS,
            ST_APC
        };

        StringData() { reset(); }

        void reset() {
            reset(ST_None);
        }

        void reset(StringType st) {
            memset(str_data, 0, sizeof(str_data));
            next_str_data = str_data;
            string_type = st;
            finished = false;
            nargs = 1;
            args[0] = str_data;
        }

        int parse(char *s, int n) {
            int i = 0;
            while (*s && i < n && nargs < STRING_ARG_LEN) {
                if (*s == ';') {
                    *s = 0;
                    i++;
                    args[nargs++] = s + 1;
                }
                s++;
            }
            return i;
        }

        char *get_arg(int arg) {
            if (arg >= 0 && arg < nargs) {
                return args[arg];
            }
            return 0;
        }

        bool add_char(char c) {
            if (next_str_data >= str_data + STRING_DATA_LEN - 2) {
                return false;
            }
            *next_str_data++ = c;
            return true;
        }

        char str_data[STRING_DATA_LEN];
        char *next_str_data;
        StringType string_type;
        bool finished;
        char *args[STRING_ARG_LEN];
        int nargs;
    };

    int width;
    int height;
    int top;
    int bottom;
    int defining_charset;
    Cursor cursor;
    Cursor saved_cursor;
    TerminalMode mode;
    u32 flags;
    ControlData csi_data;
    StringData str_data;
    bool cursor_hidden;
    char utf8_buffer[UTF8_BUF_LEN];
    int next_utf8;

    void RIS();
    bool NEL();
    void IND();
    void RI();
    void DECID();
    void DECSC();
    void DECRC();
    void CPR();
    void DECALN();

    void reset_utf8_buffer();
    void handle_control(uchar c);
    void handle_esc(uchar c);
    void handle_csi(uchar c);
    void handle_hash(uchar c);
    void handle_charset(uchar c);
    void handle_utf8(uchar c);
    bool handle_string(uchar c);
    void parse_string();
    void reset_escape();
    void hc();
    void sc();
    void set_char_pre(int char_width);
    void set_char(uint32_t c, int char_width);
    void set_char_post(int char_width);
    void move_cursor(int x, int y);
    void move_cursor_absolute(int x, int y);
    void insert_lines(int y, int n);
    void delete_lines(int y, int n);
    void trm_insert_chars(int x, int n);
    void trm_delete_chars(int x, int n);
    void trm_clear_region(int x1, int y1, int x2, int y2);
    void trm_scroll_up(int y, int n);
    void trm_scroll_down(int y, int n);
    void set_color(int& n, Cursor::Attributes::Color& color);
    void set_attributes();
    void set_modes();
    void reset_modes();
    void modify_mode(bool dec_mode, u32 v, ModeModifier modify);
    void notify_KAM();
    void notify_KPAM();
    void notify_TCEM();
    void notify_DECCOLM();
    void notify_DECSCLM();
    void notify_DECSCNM();
    void notify_DECOM();
};

#endif
