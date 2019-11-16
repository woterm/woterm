QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = term
TEMPLATE = lib

CONFIG += qtermwidget-buildlib #关于库的配置信息
CONFIG += c++11 #关于库的配置信息
include(qtermwidget.pri)

DEFINES += QT_DEPRECATED_WARNINGS QTERMWIDGET_BUILD
TARGET =  $$LIBWIDGET_NAME   #表示生成的库的名字
DESTDIR = $$PROJECT_LIBDIR #指定编译后库的位置

CONFIG += debug_and_release build_all

FORMS += ./SearchBar.ui
RESOURCES += qtermwidget.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

#win32 {
#    COPY_DEST = $$replace(PROJECT_BINDIR, /, \\)
#    #system("echo $$COPY_DEST")
#    #system("mkdir $$COPY_DEST\\translations")
#    #system("mkdir $$COPY_DEST\\kb-layouts")
#    #system("mkdir $$COPY_DEST\\color-schemes")
#    #system("copy translations\\* $$COPY_DEST\\translations\\")
#    #system("copy kb-layouts\\* $$COPY_DEST\\kb-layouts\\")
#    #system("copy color-schemes\\* $$COPY_DEST\\color-schemes\\")
#}
