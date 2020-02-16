#pragma once

#include <QObject>
#include <QUuid>
#include <QVariantMap>


typedef struct{
    QString name;
    QString host;
    int port;
    QString user;
    QString password;
    QString identityFile;
    QString proxyJump;
    QString memo;
    QString property;
    QString group;
}HostInfo;

struct HistoryCommand {
    QString cmd;
    QString path;

    bool operator==(const HistoryCommand& other) {
        return other.cmd == cmd;
    }
};

Q_DECLARE_METATYPE(HostInfo)
Q_DECLARE_METATYPE(HistoryCommand)

#ifdef Q_OS_MACOS
#define DEFAULT_FONT_FAMILY "Menlo"
#elif defined(Q_OS_WIN)
//#define DEFAULT_FONT_FAMILY "Lucida Console"
#define DEFAULT_FONT_FAMILY "Courier New"
#else
#define DEFAULT_FONT_FAMILY "Monospace"
#endif
#define DEFAULT_FONT_SIZE (10)

#define DEFAULT_COLOR_SCHEMA "Linux"

#ifdef Q_OS_WIN
#define DEFAULT_KEYBOARD_BINDING "windows"
#elif defined(Q_OS_UNIX)
#define DEFAULT_KEYBOARD_BINDING "linux"
#elif defined(Q_OS_MACOS)
#define DEFAULT_KEYBOARD_BINDING "macbook"
#endif
#define DEFAULT_HISTORY_LINE_LENGTH  (1000)

const QString scrollbarSheetH =  \
"QScrollBar::horizontal \
{\
    margin:0px 0px 0px 0px;\
    padding-top:1px;\
    padding-bottom:1px;\
    padding-left:0px;\
    padding-right:0px;\
    background: rgba(0,0,0,0);\
    border:0px solid grey;\
    height:12px;\
}\
QScrollBar::handle:horizontal \
{\
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 rgb(250,244,241),stop:1.0 rgb(241,226,218));\
    border: 1px solid rgb(219,184,163);\
    border-radius:3px;\
    height:8px;\
    min-width: 30px;\
    min-height: 8px;\
}\
QScrollBar::handle:horizontal:hover\
{\
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 rgb(215,151,113),stop:0.2 rgb(205,127,81),stop:0.8 rgb(202,119,70),stop:1.0 rgb(215,151,113));\
    border: 1px solid rgb(198,108,55);\
    border-radius:3px;\
    height:8px;\
    min-width: 30px;\
    min-height: 8px;\
}\
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal \
{\
    width:0px;\
    height:0px;\
    background:none;\
}\
QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal\
{\
    background: none;\
}";

const QString scrollbarSheetV =  \
"QScrollBar::vertical \
{\
    margin:0px 0px 0px 0px;\
    padding-top:0px;\
    padding-bottom:0px;\
    padding-left:1px;\
    padding-right:1px;\
    background: rgba(0,0,0,0);\
    border:0px solid grey;\
    width:12px;\
}\
QScrollBar::handle:vertical \
{\
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 rgb(253,250,249),stop:1.0 rgb(249,243,239));\
    border: 1px solid rgb(228,201,185);\
    border-radius:3px;\
    width:8px;\
    min-width:8px;\
    min-height: 30px;\
}\
QScrollBar::handle:vertical:hover\
{\
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 rgb(215,151,113),stop:0.2 rgb(205,127,81),stop:0.8 rgb(202,119,70),stop:1.0 rgb(215,151,113));\
    border: 1px solid rgb(198,108,55);\
    border-radius:3px;\
    width:8px;\
    min-width:8px;\
    min-height: 30px;\
}\
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical \
{\
    width:0px;\
    height:0px;\
    background:none;\
}\
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical\
{\
    background: none;\
}";

const QString scrollbarSheet = scrollbarSheetH + scrollbarSheetV;
