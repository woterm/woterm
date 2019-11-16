INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
TEMPLATE += fakelib
LIBWIDGET_NAME = $$qtLibraryTarget(qtermwidget)
TEMPLATE -= fakelib
include(../common.pri)
!qtermwidget-buildlib{
    LIBS += -L$$PROJECT_LIBDIR -l$$LIBWIDGET_NAME #-L指定库目录，-l指定库名
}else{
    HEADERS += ./Emulation.h \
        ./Filter.h \
        ./HistorySearch.h \
        ./wcwidth.h \
        ./qtermwidget.h \
        ./History.h \
        ./Screen.h \
        ./ScreenWindow.h \
        ./SearchBar.h \
        ./Session.h \
        ./TerminalDisplay.h \
        ./Vt102Emulation.h

    SOURCES += ./ColorScheme.cpp \
        ./Emulation.cpp \
        ./Filter.cpp \
        ./History.cpp \
        ./HistorySearch.cpp \
        ./KeyboardTranslator.cpp \
        ./wcwidth.cpp \
        ./qtermwidget.cpp \
        ./Screen.cpp \
        ./ScreenWindow.cpp \
        ./SearchBar.cpp \
        ./Session.cpp \
        ./TerminalCharacterDecoder.cpp \
        ./TerminalDisplay.cpp \
        ./tools.cpp \
        ./Vt102Emulation.cpp
}
