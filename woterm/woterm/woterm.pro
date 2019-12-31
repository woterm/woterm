QT       += core gui network script scripttools
RC_ICONS = woterm.ico

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

TEMPLATE=app
include(../qtermwidget/qtermwidget.pri)
include($$PWD/../third/aes/aes.pri)

DESTDIR = $$PROJECT_BINDIR
unix:QMAKE_RPATHDIR+=$$PROJECT_LIBDIR
SOURCES += main.cpp \
    vte.cpp \
    ipchelper.cpp \
    qwotermwidget.cpp \
    qwoshellwidget.cpp \
    qwoshellwidgetimpl.cpp \
    qwotermwidgetimpl.cpp \
    qwoprocess.cpp \
    qwosshprocess.cpp \
    qwoevent.cpp \
    qwosetting.cpp \
    qwomainwindow.cpp \
    qwoshower.cpp \
    qwoshowerwidget.cpp \
    qwowidget.cpp \
    qwosshconf.cpp \
    qwohostinfolist.cpp \
    qwoutils.cpp \
    qwohostinfoedit.cpp \
    qwohostsimplelist.cpp \
    qwohostlistmodel.cpp \
    qwosessionproperty.cpp \
    qwoaes.cpp \
    qwosessionlist.cpp \
    qwosessionmanage.cpp \
    qwotermmask.cpp \
    qwopasswordinput.cpp \
    qwotermstyle.cpp \
    qwolinenoise.cpp \
    qwoaboutdialog.cpp \
    qwoscriptmaster.cpp \
    qwoscriptwidgetimpl.cpp \
    qwoscriptselector.cpp \
    qwoshellcommand.cpp \
    qwolocalcommand.cpp \
    qwoshellprocess.cpp \
    qwocmdspy.cpp \
    qworesult.cpp \
    qwolistview.cpp \
    qwotreeview.cpp \
    qwocommandlineinput.cpp

HEADERS += \
    vte.h \
    ipchelper.h \
    qwotermwidget.h \
    qwoshellwidget.h \
    qwoshellwidgetimpl.h \
    qwotermwidgetimpl.h \
    qwoprocess.h \
    qwosshprocess.h \
    qwoevent.h \
    qwosetting.h \
    qwomainwindow.h \
    qwoshower.h \
    qwoshowerwidget.h \
    qwowidget.h \
    qwosshconf.h \
    qwoglobal.h \
    qwohostinfolist.h \
    qwoutils.h \
    qwohostinfoedit.h \
    qwohostsimplelist.h \
    qwohostlistmodel.h \
    qwosessionproperty.h \
    qwoaes.h \
    qwosessionlist.h \
    qwosessionmanage.h \
    qwotermmask.h \
    qwopasswordinput.h \
    qwotermstyle.h \
    qwolinenoise.h \
    qwoaboutdialog.h \
    qwoscriptmaster.h \
    qwoscriptwidgetimpl.h \
    version.h \
    qwoscriptselector.h \
    qwoshellcommand.h \
    qwolocalcommand.h \
    qwoshellprocess.h \
    qwocmdspy.h \
    qwoscript.h \
    qworesult.h \
    qwolistview.h \
    qwotreeview.h \
    qwocommandlineinput.h


FORMS += \
    qwohostlist.ui \
    qwohostinfolist.ui \
    qwohostinfo.ui \
    qwomainwindow.ui \
    qwosessionproperty.ui \
    qwosessionmanage.ui \
    qwotermmask.ui \
    qwotermmask.ui \
    qwopasswordinput.ui \
    qwoaboutdialog.ui \
    qwoscriptselector.ui \
    qwocommandlineinput.ui

RESOURCES += qwoterm.qrc
