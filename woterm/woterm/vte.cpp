#include "vte.h"

#include <cstdio>

// for reference (xterm control sequences):
// http://www.xfree86.org/current/ctlseqs.html

// * UTF-8 *********************************************************************

/*
 * Parts of this code is based on the utf-8 decoder of Bjoern Hoehrmann.
 * See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
 */

static const int UTF8_ACCEPT = 0;
static const int UTF8_REJECT = 12;

static const uint8_t _utf8_masks[] = {
    // The first part of the table maps bytes to character classes that
    // to reduce the size of the transition table and create bitmasks.
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
     7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
     8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8
};

static const int _utf8_states[] = {
    // The second part is a transition table that maps a combination
    // of a state of the automaton and a character class to a state.
     0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
    12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
    12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
    12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
    12,36,12,12,12,12,12,12,12,12,12,12
};

inline int utf8_decode(const unsigned char byte, int& state, uint32_t& codep) {
    uint32_t type = _utf8_masks[byte];
    codep = (state != UTF8_ACCEPT) ? (byte & 0x3fu) | (codep << 6) : (0xff >> type) & (byte);
    state = _utf8_states[state + type];

    return state;
}

// * local helpers *************************************************************

static inline bool is_control_c0(VTE::uchar c) {
    return (c <= 0x1f);
}

static inline bool is_control_c1(VTE::uchar c) {
    return (c >= 0x80 && c <= 0x9f);
}

static inline bool is_control(VTE::uchar c) {
    return is_control_c0(c) || is_control_c1(c);
}

static inline bool is_parameter_separator(VTE::uchar c) {
    return (c == ';');
}

static inline bool is_parameter(VTE::uchar c) {
    return ((c >= '0' && c <= '9'));
}

static inline bool has_flag(VTE::u32 v, int flag) {
    return (v & flag) > 0;
}

template<typename T> static inline void set_range(T& v, T l, T h) {
    if (v < l) {
        v = l;
    }
    if (v > h) {
        v = h;
    }
}

template<typename T> static inline T get_default(T v, T d) {
    return v < 1 ? d : v;
}

// * mode modifiers ************************************************************

static void set_flag(VTE::u32& mode, int flag) {
    mode |= flag;
}

static void reset_flag(VTE::u32& mode, int flag) {
    mode &= ~flag;
}

// * publics *******************************************************************

VTE::VTE(int width, int height) :
    width(width), height(height) { }

void VTE::reset() {
    RIS();
}

void VTE::process(char c) {
    uchar uc = static_cast<uchar>(c);

    if (mode != TM_String || !handle_string(uc)) {
        if (!next_utf8 && is_control(uc)) {
            handle_control(uc);
        } else {
            switch (mode) {
                case TM_ESC:
                    handle_esc(uc);
                    break;
                case TM_CSI:
                    handle_csi(uc);
                    break;
                case TM_Hash:
                    handle_hash(uc);
                    break;
                case TM_Charset:
                    handle_charset(uc);
                    break;
                case TM_UTF8:
                    handle_utf8(uc);
                    break;
                default:
                    if (has_flag(flags, TF_UTF8)) {
                        if (next_utf8 == UTF8_BUF_LEN - 1) {
                            set_char_pre(1);
                            hc();
                            put_reversed_question_mark(cursor);
                            sc();
                            set_char_post(1);
                            reset_utf8_buffer();
                        } else {
                            utf8_buffer[next_utf8++] = uc;
                            int state = UTF8_ACCEPT;
                            uint32_t codepoint = 0;
                            for (int i = 0; i < next_utf8; ++i) {
                                if (!utf8_decode(utf8_buffer[i], state, codepoint)) {
                                    int width = get_char_width(codepoint);
                                    if (width < 1) {
                                        // replacement character
                                        // 0xEF 0xBF 0xBD
                                        codepoint = 65533;
                                        width = 2;
                                    }
                                    set_char_pre(width);
                                    set_char(codepoint, width);
                                    set_char_post(width);
                                    reset_utf8_buffer();
                                    break;
                                }
                            }
                        }
                    } else if (uc > 31 && uc < 127) {
                        set_char_pre(1);
                        set_char(uc, 1);
                        set_char_post(1);
                    }
                    break;
            }
        }
    }
}

void VTE::process(const char *s) {
    while (*s) {
        process(*s++);
    }
}

// * protected *****************************************************************

const VTE::Cursor& VTE::get_cursor() const {
    return cursor;
}

// * privates ******************************************************************

void VTE::RIS() {
    top = 0;
    bottom = height - 1;
    clear_tabs();
    for (int x = 8; x < width; x += 8) {
        set_tab(x);
    }
    cursor.reset();
    mode = TM_Ground;

    // according to the datasheet of DEC, DECAWM is default OFF.
    // but the linux console defaults to ON.
    // http://vt100.net/docs/vt510-rm/chapter2.html#S2.13
    flags = TF_DECAWM | TF_DECTCEM | TF_DECARM | TF_DECSCLM | TF_UTF8;

    saved_cursor = cursor;
    reset_escape();
    clear_region(0, 0, width - 1, height - 1, cursor);
    cursor_hidden = true;
    notify_KAM();
    notify_KPAM();
    notify_DECSCNM();
    notify_DECSCLM();
    reset_utf8_buffer();
    str_data.reset();

    // reset all colors
    for (int i = 0; i < 256; ++i) {
        reset_color(i);
    }

    // setup charsets
    defining_charset = 0;
    for (int i = 0; i < CharsetTableSize; ++i) {
        define_charset(i, cursor.charset_table[i]);
    }
    select_charset(cursor.selected_charset);

    // show cursor
    flag_cursor_visible(has_flag(flags, TF_DECTCEM));
    sc();
}

bool VTE::NEL() {
    int y = cursor.y;
    if (cursor.y == bottom) {
        trm_scroll_up(top, 1);
    } else {
        y++;
    }
    bool lnm = has_flag(flags, TF_LNM);
    move_cursor(lnm ? cursor.x : 0, y);

    return lnm;
}

void VTE::IND() {
    if (cursor.y == bottom) {
        trm_scroll_up(top, 1);
    } else {
        move_cursor(cursor.x, cursor.y + 1);
    }
}

void VTE::RI() {
    if (cursor.y == top) {
        trm_scroll_down(top, 1);
    } else {
        move_cursor(cursor.x, cursor.y - 1);
    }
}

void VTE::DECID() {
    tty_write("\033[?6c");
}

void VTE::DECSC() {
    saved_cursor = cursor;
}

void VTE::DECRC() {
    hc();
    cursor = saved_cursor;
    set_range(cursor.y, top, bottom);
    for (int i = 0; i < CharsetTableSize; ++i) {
        define_charset(i, cursor.charset_table[i]);
    }
    select_charset(cursor.selected_charset);
    sc();
}

void VTE::CPR() {
    char cpr_buffer[32];
    snprintf(cpr_buffer, sizeof(cpr_buffer), "\033[%i;%iR", cursor.y + 1, cursor.x + 1);
    tty_write(cpr_buffer);
}

void VTE::DECALN() {
    hc();
    Cursor cursor_save = cursor;
    cursor.reset();
    for (cursor.x = 0; cursor.x < width; ++cursor.x) {
        for (cursor.y = 0; cursor.y < height; ++cursor.y) {
            put_char('E', 1, cursor);
        }
    }
    cursor = cursor_save;
    sc();
}

void VTE::reset_utf8_buffer() {
    memset(utf8_buffer, 0, sizeof(utf8_buffer));
    next_utf8 = 0;
}

// * handlers ******************************************************************

void VTE::handle_control(uchar c) {
    switch (c) {
        case 0x07:      // BEL
            if (str_data.finished) {
                parse_string();
            } else {
                bell();
            }
            reset_escape();
            break;
        case 0x08:      // BS
            move_cursor(cursor.x - 1, cursor.y);
            break;
        case 0x09:      // HT
            move_cursor(get_next_tab(cursor.x, 1), cursor.y);
            break;
        case 0x0a:      // LF
        case 0x0b:      // VT
            NEL();
            break;
        case 0x0d:      // CR
            if (cursor.x) {
                move_cursor(0, cursor.y);
            }
            break;
        case 0x0e:      // SO
            cursor.selected_charset = 1;
            select_charset(cursor.selected_charset);
            break;
        case 0x0f:      // SI
            cursor.selected_charset = 0;
            select_charset(cursor.selected_charset);
            break;
        case 0x1a:      // SUB
            set_char_pre(1);
            hc();
            put_reversed_question_mark(cursor);
            sc();
            set_char_post(1);
            reset_escape();
            str_data.reset();
            break;
        case 0x18:      // CAN
            reset_escape();
            str_data.reset();
            break;
        case 0x1b:      // ESC introducer
            mode = TM_ESC;
            break;
        case 0x7f:      // DEL (ignored)
            break;
        case 0x84:      // IND (obsolete)
            IND();
            break;
        case 0x85:      // NEL
            NEL();
            reset_escape();
            break;
        case 0x88:      // HTS
            set_tab(cursor.x);
            break;
        case 0x8d:      // RI
            RI();
            break;
        case 0x90:      // DCS introducer
            reset_escape();
            str_data.reset(StringData::ST_DCS);
            mode = TM_String;
            break;
        case 0x9a:      // DECID
            DECID();
            reset_escape();
            break;
        case 0x9b:      // CSI introducer
            reset_escape();
            mode = TM_CSI;
            break;
        case 0x9d:      // OSC introducer
            reset_escape();
            str_data.reset(StringData::ST_OSC);
            mode = TM_String;
            break;
        case 0x9e:      // PM introducer
            reset_escape();
            str_data.reset(StringData::ST_PM);
            mode = TM_String;
            break;
        case 0x9f:      // APC introducer
            reset_escape();
            str_data.reset(StringData::ST_APC);
            mode = TM_String;
            break;
    }
}

void VTE::handle_esc(uchar c) {
    switch (c) {
        case '[':       // CSI introducer
            reset_escape();
            mode = TM_CSI;
            return;
        case '#':       // HASH introducer
            reset_escape();
            mode = TM_Hash;
            return;
        case '%':       // UTF8
            reset_escape();
            mode = TM_UTF8;
            return;
        case 'P':       // DCS introducer
            reset_escape();
            str_data.reset(StringData::ST_DCS);
            mode = TM_String;
            return;
        case '_':       // APC introducer
            reset_escape();
            str_data.reset(StringData::ST_APC);
            mode = TM_String;
            return;
        case '^':       // PM introducer
            reset_escape();
            str_data.reset(StringData::ST_PM);
            mode = TM_String;
            return;
        case ']':       // OSC introducer
            reset_escape();
            str_data.reset(StringData::ST_OSC);
            mode = TM_String;
            return;
        case 'k':       // set window title
            reset_escape();
            str_data.reset(StringData::ST_Title);
            mode = TM_String;
            break;
        case 'n':       // LS2
            cursor.selected_charset = 2;
            select_charset(cursor.selected_charset);
            break;
        case 'o':       // LS3
            cursor.selected_charset = 3;
            select_charset(cursor.selected_charset);
            break;
        case 'D':       // IND
            IND();
            break;
        case 'E':       // NEL
            NEL();
            break;
        case 'H':       // HTS
            set_tab(cursor.x);
            break;
        case 'M':       // RI
            RI();
            break;
        case 'Z':       // DECID
            DECID();
            break;
        case 'c':       // RIS
            RIS();
            break;
        case '=':       // DECPAM
            flags |= TF_DECKPAM;
            notify_KPAM();
            break;
        case '>':       // DECPNM
            flags &= ~TF_DECKPAM;
            notify_KPAM();
            break;
        case '7':       // DECSC
            DECSC();
            break;
        case '8':       // DECRC
            DECRC();
            break;
        case '(':       // G0
        case ')':       // G1
        case '*':       // G2
        case '+':       // G3
            defining_charset = c - '(';
            reset_escape();
            mode = TM_Charset;
            return;
        case '\\':      // ST
            if (str_data.finished) {
                parse_string();
            }
            break;
    }
    reset_escape();
}

void VTE::handle_csi(uchar c) {
    if (csi_data.space_left()) {
        if (!csi_data.add_arg(c)) {
            switch (c) {
                case '@':   // ICH
                    trm_insert_chars(cursor.x, get_default(csi_data.get_arg(0), 1));
                    break;
                case 'A':   // CUU
                {
                    int y = cursor.y - get_default(csi_data.get_arg(0), 1);
                    set_range(y, top, bottom);
                    move_cursor(cursor.x, y);
                    break;
                }
                case 'B':   // CUD
                case 'e':   // VPR
                {
                    int y = cursor.y + get_default(csi_data.get_arg(0), 1);
                    set_range(y, top, bottom);
                    move_cursor(cursor.x, y);
                    break;
                }
                case 'c':   // DA
                    if (csi_data.get_arg(0) == 0) {
                        DECID();
                    }
                    break;
                case 'C':   // CUF
                case 'a':   // HPR
                    move_cursor(cursor.x + get_default(csi_data.get_arg(0), 1), cursor.y);
                    break;
                case 'D':   // CUB
                    move_cursor(cursor.x - get_default(csi_data.get_arg(0), 1), cursor.y);
                    break;
                case 'E':   // CNL
                    move_cursor(0, cursor.y + get_default(csi_data.get_arg(0), 1));
                    break;
                case 'F':   // CPL
                    move_cursor(0, cursor.y - get_default(csi_data.get_arg(0), 1));
                    break;
                case 'g':   // TBC
                    switch (csi_data.get_arg(0)) {
                        case 0:     // delete current tab
                            reset_tab(cursor.x);
                            break;
                        case 3:     // delete all tabs
                            clear_tabs();
                            break;
                    }
                    break;
                case 'G':   // CHA
                case '`':   // HPA
                    move_cursor(get_default(csi_data.get_arg(0), 1) - 1, cursor.y);
                    break;
                case 'H':   // CUP
                case 'f':   // HVP
                    move_cursor_absolute(
                        get_default(csi_data.get_arg(1), 1) - 1,
                        get_default(csi_data.get_arg(0), 1) - 1
                    );
                    break;
                case 'I':   // CHT
                    move_cursor(get_next_tab(cursor.x, get_default(csi_data.get_arg(0), 1)), cursor.y);
                    break;
                case 'J':   // ED
                    switch (csi_data.get_arg(0)) {
                        case 0:     // below
                            trm_clear_region(cursor.x, cursor.y, width - 1, cursor.y);
                            if (cursor.y < height - 1) {
                                trm_clear_region(0, cursor.y + 1, width - 1, height - 1);
                            }
                            break;
                        case 1:     // above
                            if (cursor.y > 0) {
                                trm_clear_region(0, 0, width - 1, cursor.y - 1);
                            }
                            trm_clear_region(0, cursor.y, cursor.x, cursor.y);
                            break;
                        case 2:     // all
                            trm_clear_region(0, 0, width - 1, height - 1);
                            break;
                    }
                    break;
                case 'K':   // EL
                    switch (csi_data.get_arg(0)) {
                        case 0:     // right
                            trm_clear_region(cursor.x, cursor.y, width - 1, cursor.y);
                            break;
                        case 1:     // left
                            trm_clear_region(0, cursor.y, cursor.x, cursor.y);
                            break;
                        case 2:     // all
                            trm_clear_region(0, cursor.y, width - 1, cursor.y);
                            break;;
                    }
                    break;
                case 'S':   // SU
                    trm_scroll_up(top, get_default(csi_data.get_arg(0), 1));
                    break;
                case 'T':   // SD
                    trm_scroll_down(top, get_default(csi_data.get_arg(0), 1));
                    break;
                case 'L':   // IL
                    insert_lines(cursor.y, get_default(csi_data.get_arg(0), 1));
                    break;
                case 'l':   // RM
                    reset_modes();
                    break;
                case 'M':   // DL
                    delete_lines(cursor.y, get_default(csi_data.get_arg(0), 1));
                    break;
                case 'X':   // ECH
                    trm_clear_region(
                        cursor.x,
                        cursor.y,
                        cursor.x + get_default(csi_data.get_arg(0), 1),
                        cursor.y
                    );
                    break;
                case 'P':   // DCH
                    trm_delete_chars(cursor.x, get_default(csi_data.get_arg(0), 1));
                    break;
                case 'Z':   // CBT
                    move_cursor(get_next_tab(cursor.x, -get_default(csi_data.get_arg(0), 1)), cursor.y);
                    break;
                case 'd':   // VPA
                    move_cursor_absolute(cursor.x, get_default(csi_data.get_arg(0), 1));
                    break;
                case 'h':   // SM
                    set_modes();
                    break;
                case 'm':   // SGR
                    set_attributes();
                    break;
                case 'n':   // DSR
                    switch (csi_data.get_arg(0)) {
                        case 5: // DS
                            tty_write("\x1b[0n");
                            break;
                        case 6: // CPR
                            CPR();
                            break;
                    }
                    break;
                case 'r':   // DECSTBM
                    if (!csi_data.dec_mode) {
                        int t = get_default(csi_data.get_arg(0), 1) - 1;
                        int b = get_default(csi_data.get_arg(1), height) - 1;
                        if (b > t) {
                            top = t;
                            bottom = b;
                            move_cursor_absolute(0, 0);
                        }
                    }
                    break;
                case 's':   // DECSC
                    DECSC();
                    break;
                case 'u':   // DECRC
                    DECRC();
                    break;
            }
            reset_escape();
        }
    } else {
        reset_escape();
    }
}

void VTE::handle_hash(uchar c) {
    switch (c) {
        case '3':       // DECDHL, Double-height letters, top half
            set_line_text_size(cursor.y, TS_TopHalf);
            break;
        case '4':       // DECDHL, Double-height letters, bottom half
            set_line_text_size(cursor.y, TS_BottomHalf);
            break;
        case '5':       // DECSWL, Single width, single height letters
            set_line_text_size(cursor.y, TS_Normal);
            break;
        case '6':       // DECDWL, Double width, single height letters
            set_line_text_size(cursor.y, TS_DoubleWidth);
            break;
        case '8':       // DECALN
            DECALN();
            break;
    }
    reset_escape();
}

void VTE::handle_charset(uchar c) {
    switch (c) {
        case 'A':       // British
            cursor.charset_table[defining_charset] = CS_UK;
            define_charset(defining_charset, cursor.charset_table[defining_charset]);
            break;
        case 'B':       // default ISO 8891 (US ASCII)
            cursor.charset_table[defining_charset] = CS_US;
            define_charset(defining_charset, cursor.charset_table[defining_charset]);
            break;
        case '0':       // VT100 graphics
            cursor.charset_table[defining_charset] = CS_G0;
            define_charset(defining_charset, cursor.charset_table[defining_charset]);
            break;
        case 'U':       // null, straight ROM
            cursor.charset_table[defining_charset] = CS_US;
            define_charset(defining_charset, cursor.charset_table[defining_charset]);
            break;
        case 'K':       // user
            cursor.charset_table[defining_charset] = CS_US;
            define_charset(defining_charset, cursor.charset_table[defining_charset]);
            break;
    }
    reset_escape();
}

void VTE::handle_utf8(uchar c) {
    switch (c) {
        case '@':       // Select default (ISO 646 / ISO 8859-1)
            reset_flag(flags, TF_UTF8);
            reset_utf8_buffer();
            break;
        case 'G':       // Select UTF-8
            set_flag(flags, TF_UTF8);
            reset_utf8_buffer();
            break;
    }
    reset_escape();
}

bool VTE::handle_string(uchar c) {
    // abort (BEL, CAN, SUB, ESC or C1)
    if (c == 0x07 || c == 0x18 || c == 0x1a || c == 0x1b || is_control_c1(c)) {
        reset_escape();
        str_data.finished = true;
        return false;
    }

    // abort if buffer full
    if (!str_data.add_char(c)) {
        reset_escape();
        str_data.reset();
    }

    return true;
}

void VTE::parse_string() {
    switch (str_data.string_type) {
        case StringData::ST_OSC:
            if (str_data.parse(str_data.str_data, 1) == 1) {
                int f = atoi(str_data.get_arg(0));
                switch (f) {
                    case 0:     // set window title
                    case 1:
                    case 2:
                        set_window_title(str_data.get_arg(1));
                        break;
                    case 4:     // define color
                        if (str_data.parse(str_data.get_arg(1), 1) == 1) {
                            define_color(atoi(str_data.get_arg(1)), str_data.get_arg(2));
                        }
                        break;
                    case 104:   // reset color
                        reset_color(atoi(str_data.get_arg(1)));
                        break;
                    default:
                        break;
                }
            }
            break;
        case StringData::ST_Title:
            if (str_data.parse(str_data.str_data, 1) == 1) {
                set_window_title(str_data.get_arg(0));
            }
            break;
        case StringData::ST_None:
        case StringData::ST_APC:
        case StringData::ST_DCS:
        case StringData::ST_PM:
            // ignored
            break;
    }
    str_data.reset();
}

void VTE::reset_escape() {
    csi_data.reset();
    mode = TM_Ground;
}

void VTE::hc() {
    if (!cursor_hidden) {
        hide_cursor(cursor);
        cursor_hidden = true;
    }
}

void VTE::sc() {
    if (has_flag(flags, TF_DECTCEM)) {
        if (cursor_hidden) {
            show_cursor(cursor);
            cursor_hidden = false;
        }
    }
}

void VTE::set_char_pre(int char_width) {
    if (cursor.cs == Cursor::CS_WrapNext) {
        if (has_flag(flags, TF_DECAWM)) {
            if (NEL()) {
                move_cursor(0, cursor.y);
            }
        }
    }
    if (has_flag(flags, TF_IRM)) {
        trm_insert_chars(cursor.x, char_width);
    }
}

void VTE::set_char(uint32_t c, int char_width) {
    hc();
    put_char(c, char_width, cursor);
    sc();
}

void VTE::set_char_post(int char_width) {
    if (cursor.x >= width - char_width) {
        cursor.cs = Cursor::CS_WrapNext;
    } else {
        move_cursor(cursor.x + char_width, cursor.y);
    }
}

void VTE::move_cursor(int x, int y) {
    hc();
    cursor.x = x;
    cursor.y = y;
    if (has_flag(flags, TF_DECOM)) {
        set_range(cursor.x, 0, width - 1);
        set_range(cursor.y, top, bottom);
    } else {
        set_range(cursor.x, 0, width - 1);
        set_range(cursor.y, 0, height - 1);
    }
    cursor.cs = Cursor::CS_Normal;
    sc();
}

void VTE::move_cursor_absolute(int x, int y) {
    move_cursor(x, y + (has_flag(flags, TF_DECOM) ? top : 0));
}

void VTE::insert_lines(int y, int n) {
    trm_scroll_down(y, n);
}

void VTE::delete_lines(int y, int n) {
    trm_scroll_up(y, n);
}

void VTE::trm_insert_chars(int x, int n) {
    hc();
    if (n > width) {
        n = width;
    }
    insert_chars(x, n, cursor);
    sc();
}

void VTE::trm_delete_chars(int x, int n) {
    hc();
    if (n > width) {
        n = width;
    }
    delete_chars(x, n, cursor);
    sc();
}

void VTE::trm_clear_region(int x1, int y1, int x2, int y2) {
    hc();
    set_range(y2, 0, height - 1);
    set_range(y2, 0, height - 1);
    clear_region(x1, y1, x2, y2, cursor);
    sc();
}

void VTE::trm_scroll_up(int y, int n) {
    hc();
    scroll_up(y, bottom, n, cursor);
    cursor.x = 0;
    sc();
}

void VTE::trm_scroll_down(int y, int n) {
    hc();
    scroll_down(y, bottom, n, cursor);
    cursor.x = 0;
    sc();
}

void VTE::set_color(int& n, Cursor::Attributes::Color& color) {
    n++;
    if (n < csi_data.nargs) {
        switch (csi_data.get_arg(n)) {
            case 2:     // RGB
            {
                if (n + 3 < csi_data.nargs) {
                    int r = csi_data.get_arg(n + 1);
                    int g = csi_data.get_arg(n + 2);
                    int b = csi_data.get_arg(n + 3);
                    if (r >= 0 && r < 256 && g >= 0 && g < 256 && b >= 0 && b < 256) {
                        color.type = Cursor::Attributes::Color::CT_Truecolor;
                        color.color = 1 << 24 | r << 16 | g << 8 | b;
                    }
                    n += 3;
                }
                break;
            }
            case 5:     // indexed
                if (n + 1 < csi_data.nargs) {
                    color.type = Cursor::Attributes::Color::CT_Indexed;
                    color.color = csi_data.get_arg(n + 1);
                    n += 1;
                }
                break;
        }
    }
}

void VTE::set_attributes() {
    // TODO: incomplete
    for (int i = 0; i < csi_data.nargs; ++i) {
        int v = csi_data.get_arg(i);
        switch (v) {
            case 0:
                cursor.attrs.reset();
                break;
            case 1:
                cursor.attrs.flags |= Cursor::Attributes::AF_Bold;
                break;
            case 2:
                cursor.attrs.flags |= Cursor::Attributes::AF_HalfBright;
                break;
            case 3:
                cursor.attrs.flags |= Cursor::Attributes::AF_Italic;
                break;
            case 4:
                cursor.attrs.flags |= Cursor::Attributes::AF_Underline;
                break;
            case 5:
            case 6:
                cursor.attrs.flags |= Cursor::Attributes::AF_Blink;
                break;
            case 7:
                cursor.attrs.flags |= Cursor::Attributes::AF_Reverse;
                break;
            case 8:
                cursor.attrs.flags |= Cursor::Attributes::AF_Invisible;
                break;
            case 22:
                cursor.attrs.flags &= ~(Cursor::Attributes::AF_Bold | Cursor::Attributes::AF_HalfBright);
                break;
            case 23:
                cursor.attrs.flags &= ~Cursor::Attributes::AF_Italic;
                break;
            case 24:
                cursor.attrs.flags &= ~Cursor::Attributes::AF_Underline;
                break;
            case 25:
                cursor.attrs.flags &= ~Cursor::Attributes::AF_Blink;
                break;
            case 27:
                cursor.attrs.flags &= ~Cursor::Attributes::AF_Reverse;
                break;
            case 28:
                cursor.attrs.flags &= ~Cursor::Attributes::AF_Invisible;
                break;
            case 38:
                set_color(i, cursor.attrs.fg);
                break;
            case 39:
                cursor.attrs.fg.reset();
                break;
            case 48:
                set_color(i, cursor.attrs.bg);
                break;
            case 49:
                cursor.attrs.bg.reset();
                break;
            default:
                if (v >= 30 && v <= 37) {
                    cursor.attrs.fg.type = Cursor::Attributes::Color::CT_Indexed;
                    cursor.attrs.fg.color = v - 30;
                } else if (v >= 40 && v <= 47) {
                    cursor.attrs.bg.type = Cursor::Attributes::Color::CT_Indexed;
                    cursor.attrs.bg.color = v - 40;
                } else if (v >= 90 && v <= 97) {
                    cursor.attrs.fg.type = Cursor::Attributes::Color::CT_Indexed;
                    cursor.attrs.fg.color = v - 90 + 8;
                } else if (v >= 100 && v <= 107) {
                    cursor.attrs.bg.type = Cursor::Attributes::Color::CT_Indexed;
                    cursor.attrs.bg.color = v - 100 + 8;
                }
                break;
        }
    }
}

void VTE::set_modes() {
    for (int i = 0; i < csi_data.nargs; ++i) {
        modify_mode(csi_data.dec_mode, csi_data.get_arg(i), set_flag);
    }
}

void VTE::reset_modes() {
    for (int i = 0; i < csi_data.nargs; ++i) {
        modify_mode(csi_data.dec_mode, csi_data.get_arg(i), reset_flag);
    }
}

void VTE::modify_mode(bool dec_mode, u32 v, ModeModifier modify) {
    static const struct ModeTableEntry {
        u32 v;
        bool dec_mode;
        int flag;
        void (VTE::*notifier)();
    } ModeTableEntries[] = {
        {  2, false, TF_KAM    , &VTE::notify_KAM     },
        {  4, false, TF_IRM    , 0                    },
        { 12, false, TF_SRM    , 0                    },
        { 20, false, TF_LNM    , 0                    },
        {  1, true , TF_DECCKM , 0                    },
        {  2, true , TF_DECANM , 0                    },
        {  3, true , TF_DECCOLM, &VTE::notify_DECCOLM },
        {  4, true , TF_DECSCLM, &VTE::notify_DECSCLM },
        {  5, true , TF_DECSCNM, &VTE::notify_DECSCNM },
        {  6, true , TF_DECOM  , &VTE::notify_DECOM   },
        {  7, true , TF_DECAWM , 0                    },
        {  8, true , TF_DECARM , 0                    },
        { 18, true , TF_DECPFF , 0                    },
        { 19, true , TF_DECPEX , 0                    },
        { 25, true , TF_DECTCEM, &VTE::notify_TCEM    },
        {  0, false, 0         , 0                    }
    };

    const ModeTableEntry *mte = ModeTableEntries;
    while (mte->v) {
        if (mte->dec_mode == dec_mode && mte->v == v) {
            modify(flags, mte->flag);
            if (mte->notifier) {
                (this->*mte->notifier)();
            }
            break;
        }
        mte++;
    }
}

void VTE::notify_KAM() {
    flag_lock_keyboard(has_flag(flags, TF_KAM));
}

void VTE::notify_KPAM() {
    flag_application_keypad(has_flag(flags, TF_DECKPAM));
}

void VTE::notify_TCEM() {
    hc();
    flag_cursor_visible(has_flag(flags, TF_DECTCEM));
    sc();
}

void VTE::notify_DECCOLM() {
    hc();
    int requested_width = (has_flag(flags, TF_DECCOLM) ? 132 : 80);
    if (request_width(requested_width)) {
        width = requested_width;
    }
    top = 0;
    bottom = height - 1;
    cursor.reset();
    clear_region(0, 0, width - 1, height - 1, cursor);
    sc();
}

void VTE::notify_DECSCLM() {
    flag_soft_scroll(has_flag(flags, TF_DECSCLM));
}

void VTE::notify_DECSCNM() {
    flag_screen_reversed(has_flag(flags, TF_DECSCNM));
}

void VTE::notify_DECOM() {
    move_cursor_absolute(0, 0);
}
