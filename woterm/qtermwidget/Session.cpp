/*
    This file is part of Konsole

    Copyright (C) 2006-2007 by Robert Knight <robertknight@gmail.com>
    Copyright (C) 1997,1998 by Lars Doelle <lars.doelle@on-line.de>

    Rewritten for QT4 by e_k <e_k at users.sourceforge.net>, Copyright (C)2008

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301  USA.
*/

// Own
#include "Session.h"

// Standard
#include <stdlib.h>

// Qt
#include <QApplication>
#include <QByteRef>
#include <QDir>
#include <QFile>
#include <QRegExp>
#include <QStringList>
#include <QFile>
#include <QtDebug>
#include <QObject>
#include <QProcess>

#include "TerminalDisplay.h"
#include "Vt102Emulation.h"

using namespace Konsole;

int Session::lastSessionId = 0;

Session::Session(QObject* parent) :
    QObject(parent)
    , _emulation(0)
    , _monitorActivity(false)
    , _monitorSilence(false)
    , _notifiedActivity(false)
    , _autoClose(true)
    , _wantedClose(false)
    , _silenceSeconds(10)
    , _isTitleChanged(false)
    , _addToUtmp(false)  // disabled by default because of a bug encountered on certain systems
    // which caused Konsole to hang when closing a tab and then opening a new
    // one.  A 'QProcess destroyed while still running' warning was being
    // printed to the terminal.  Likely a problem in KPty::logout()
    // or KPty::login() which uses a QProcess to start /usr/bin/utempter
    , _flowControl(true)
    , _fullScripting(false)
    , _sessionId(0)
    //   , _zmodemBusy(false)
    //   , _zmodemProc(0)
    //   , _zmodemProgress(0)
    , _hasDarkBackground(false)
    , m_pProcess(nullptr)
{
    _sessionId = ++lastSessionId;
    //create emulation backend
    _emulation = new Vt102Emulation();


    connect( _emulation, SIGNAL( titleChanged( int, const QString & ) ), this, SLOT( setUserTitle( int, const QString & ) ) );
    connect( _emulation, SIGNAL( stateSet(int) ), this, SLOT( activityStateSet(int) ) );
//  connect( _emulation, SIGNAL( zmodemDetected() ), this, SLOT( fireZModemDetected() ) );
    connect( _emulation, SIGNAL( changeTabTextColorRequest( int ) ), this, SIGNAL( changeTabTextColorRequest( int ) ) );
    connect( _emulation, SIGNAL(profileChangeCommandReceived(const QString &)), this, SIGNAL( profileChangeCommandReceived(const QString &)) );

    connect(_emulation, SIGNAL(imageResizeRequest(QSize)), this, SLOT(onEmulationSizeChange(QSize)));
    connect(_emulation, SIGNAL(imageSizeChanged(int, int)), this, SLOT(onViewSizeChange(int, int)));
    connect(_emulation, SIGNAL(cursorChanged()), this, SIGNAL(cursorChanged()));

    connect(_emulation, SIGNAL(sendData(const QByteArray&)), this, SIGNAL(sendData(const QByteArray&)));

    //setup timer for monitoring session activity
    _monitorTimer = new QTimer(this);
    _monitorTimer->setSingleShot(true);
    connect(_monitorTimer, SIGNAL(timeout()), this, SLOT(monitorTimerDone()));
}

WId Session::windowId() const
{
    // On Qt5, requesting window IDs breaks QQuickWidget and the likes,
    // for example, see the following bug reports:
    // https://bugreports.qt.io/browse/QTBUG-40765
    // https://codereview.qt-project.org/#/c/94880/
    return 0;
}

void Session::setDarkBackground(bool darkBackground)
{
    _hasDarkBackground = darkBackground;
}

bool Session::hasDarkBackground() const
{
    return _hasDarkBackground;
}

void Session::setCodec(QTextCodec * codec)
{
    emulation()->setCodec(codec);
}

QList<TerminalDisplay *> Session::views() const
{
    return _views;
}

void Session::addView(TerminalDisplay * widget)
{
    Q_ASSERT( !_views.contains(widget) );

    _views.append(widget);

    if ( _emulation != 0 ) {
        connect( widget , SIGNAL(keyPressedSignal(QKeyEvent *)), _emulation, SLOT(sendKeyEvent(QKeyEvent *)) );
        // connect emulation - view signals and slots
        connect( widget , SIGNAL(mouseSignal(int,int,int,int)) , _emulation, SLOT(sendMouseEvent(int,int,int,int)) );
        connect( widget , SIGNAL(sendStringToEmu(const char *)) , _emulation, SLOT(sendString(const char *)) );

        // allow emulation to notify view when the foreground process
        // indicates whether or not it is interested in mouse signals
        connect( _emulation , SIGNAL(programUsesMouseChanged(bool)), widget, SLOT(setUsesMouse(bool)) );

        widget->setUsesMouse( _emulation->programUsesMouse() );

        connect( _emulation , SIGNAL(programBracketedPasteModeChanged(bool)), widget , SLOT(setBracketedPasteMode(bool)) );

        widget->setBracketedPasteMode(_emulation->programBracketedPasteMode());

        widget->setScreenWindow(_emulation->createWindow());
    }

    //connect view signals and slots
    QObject::connect( widget ,SIGNAL(changedContentSizeSignal(int,int)),this, SLOT(onViewSizeChange(int,int)));

    QObject::connect( widget ,SIGNAL(destroyed(QObject *)) , this , SLOT(viewDestroyed(QObject *)) );

    QObject::connect(this, SIGNAL(finished()), widget, SLOT(close()));

    widget->installEventFilter(_emulation);
}

void Session::viewDestroyed(QObject * view)
{
    TerminalDisplay * display = (TerminalDisplay *)view;
    Q_ASSERT( _views.contains(display) );
    removeView(display);
}

void Session::removeView(TerminalDisplay * widget)
{
    _views.removeAll(widget);

    disconnect(widget,0,this,0);

    if ( _emulation != 0 ) {
        disconnect( widget, 0, _emulation, 0);
        // disconnect state change signals emitted by emulation
        disconnect( _emulation , 0 , widget , 0);
    }

    // close the session automatically when the last view is removed
    if ( _views.count() == 0 ) {
        close();
    }
}

void Session::setUserTitle( int what, const QString & caption )
{
    //set to true if anything is actually changed (eg. old _nameTitle != new _nameTitle )
    bool modified = false;

    // (btw: what=0 changes _userTitle and icon, what=1 only icon, what=2 only _nameTitle
    if ((what == 0) || (what == 2)) {
        _isTitleChanged = true;
        if ( _userTitle != caption ) {
            _userTitle = caption;
            modified = true;
        }
    }

    if ((what == 0) || (what == 1)) {
        _isTitleChanged = true;
        if ( _iconText != caption ) {
            _iconText = caption;
            modified = true;
        }
    }

    if (what == 11) {
        QString colorString = caption.section(QLatin1Char(';'),0,0);
        qDebug() << __FILE__ << __LINE__ << ": setting background colour to " << colorString;
        QColor backColor = QColor(colorString);
        if (backColor.isValid()) { // change color via \033]11;Color\007
            if (backColor != _modifiedBackground) {
                _modifiedBackground = backColor;

                // bail out here until the code to connect the terminal display
                // to the changeBackgroundColor() signal has been written
                // and tested - just so we don't forget to do this.
                Q_ASSERT( 0 );

                emit changeBackgroundColorRequest(backColor);
            }
        }
    }

    if (what == 30) {
        _isTitleChanged = true;
        if ( _nameTitle != caption ) {
            setTitle(Session::NameRole,caption);
            return;
        }
    }

    if (what == 31) {
        QString cwd=caption;
        cwd=cwd.replace( QRegExp(QLatin1String("^~")), QDir::homePath() );
        emit openUrlRequest(cwd);
    }

    // change icon via \033]32;Icon\007
    if (what == 32) {
        _isTitleChanged = true;
        if ( _iconName != caption ) {
            _iconName = caption;

            modified = true;
        }
    }

    if (what == 50) {
        emit profileChangeCommandReceived(caption);
        return;
    }

    if ( modified ) {
        emit titleChanged();
    }
}

QString Session::userTitle() const
{
    return _userTitle;
}

void Session::setTabTitleFormat(TabTitleContext context , const QString & format)
{
    if ( context == LocalTabTitle ) {
        _localTabTitleFormat = format;
    } else if ( context == RemoteTabTitle ) {
        _remoteTabTitleFormat = format;
    }
}

QString Session::tabTitleFormat(TabTitleContext context) const
{
    if ( context == LocalTabTitle ) {
        return _localTabTitleFormat;
    } else if ( context == RemoteTabTitle ) {
        return _remoteTabTitleFormat;
    }

    return QString();
}

void Session::monitorTimerDone()
{
    //FIXME: The idea here is that the notification popup will appear to tell the user than output from
    //the terminal has stopped and the popup will disappear when the user activates the session.
    //
    //This breaks with the addition of multiple views of a session.  The popup should disappear
    //when any of the views of the session becomes active


    //FIXME: Make message text for this notification and the activity notification more descriptive.
    if (_monitorSilence) {
        emit silence();
        emit stateChanged(NOTIFYSILENCE);
    } else {
        emit stateChanged(NOTIFYNORMAL);
    }

    _notifiedActivity=false;
}

void Session::activityStateSet(int state)
{
    if (state==NOTIFYBELL) {
        QString s;
        s.sprintf("Bell in session '%s'",_nameTitle.toUtf8().data());

        emit bellRequest( s );
    } else if (state==NOTIFYACTIVITY) {
        if (_monitorSilence) {
            _monitorTimer->start(_silenceSeconds*1000);
        }

        if ( _monitorActivity ) {
            //FIXME:  See comments in Session::monitorTimerDone()
            if (!_notifiedActivity) {
                _notifiedActivity=true;
                emit activity();
            }
        }
    }

    if ( state==NOTIFYACTIVITY && !_monitorActivity ) {
        state = NOTIFYNORMAL;
    }
    if ( state==NOTIFYSILENCE && !_monitorSilence ) {
        state = NOTIFYNORMAL;
    }

    emit stateChanged(state);
}

void Session::onViewSizeChange(int /*height*/, int /*width*/)
{
    updateTerminalSize();
}

void Session::onEmulationSizeChange(QSize size)
{
    setSize(size);
}

void Session::updateTerminalSize()
{
    QListIterator<TerminalDisplay *> viewIter(_views);

    int minLines = -1;
    int minColumns = -1;

    // minimum number of lines and columns that views require for
    // their size to be taken into consideration ( to avoid problems
    // with new view widgets which haven't yet been set to their correct size )
    const int VIEW_LINES_THRESHOLD = 2;
    const int VIEW_COLUMNS_THRESHOLD = 2;

    //select largest number of lines and columns that will fit in all visible views
    while ( viewIter.hasNext() ) {
        TerminalDisplay * view = viewIter.next();
        if ( view->isHidden() == false &&
                view->lines() >= VIEW_LINES_THRESHOLD &&
                view->columns() >= VIEW_COLUMNS_THRESHOLD ) {
            minLines = (minLines == -1) ? view->lines() : qMin( minLines , view->lines() );
            minColumns = (minColumns == -1) ? view->columns() : qMin( minColumns , view->columns() );
        }
    }

    // backend emulation must have a _terminal of at least 1 column x 1 line in size
    if ( minLines > 0 && minColumns > 0 ) {
        _emulation->setImageSize( minLines , minColumns );
    }
}

bool Session::sendSignal(int signal)
{
    return false;
//    int result = ::kill(_shellProcess->pid(),signal);
//
//     if ( result == 0 )
//     {
//         _shellProcess->waitForFinished();
//         return true;
//     }
//     else
//         return false;
}

void Session::close()
{
    _autoClose = true;
    _wantedClose = true;
}

void Session::sendText(const QString & text) const
{
    _emulation->sendText(text);
}

void Session::parseSequenceText(const QByteArray &buf) const
{
    _emulation->receiveData(buf.data(), buf.length());
}

void Session::receiveData(const char *text, int length)
{
    _emulation->receiveData(text, length);
}

Session::~Session()
{
    delete _emulation;
//    delete _shellProcess;
//  delete _zmodemProc;
}

void Session::done(int exitStatus)
{
    if (!_autoClose) {
        _userTitle = QString::fromLatin1("This session is done. Finished");
        emit titleChanged();
        return;
    }

    QString message;
    if (!_wantedClose || exitStatus != 0) {

//        if (_shellProcess->exitStatus() == QProcess::NormalExit) {
//            message.sprintf("Session '%s' exited with status %d.",
//                          _nameTitle.toUtf8().data(), exitStatus);
//        } else {
//            message.sprintf("Session '%s' crashed.",
//                          _nameTitle.toUtf8().data());
//        }
    }

//    if ( !_wantedClose && _shellProcess->exitStatus() != QProcess::NormalExit )
//        message.sprintf("Session '%s' exited unexpectedly.",
//                        _nameTitle.toUtf8().data());
//    else
//        emit finished();

}

Emulation * Session::emulation() const
{
    return _emulation;
}

QString Session::keyBindings() const
{
    return _emulation->keyBindings();
}

int Session::sessionId() const
{
    return _sessionId;
}

void Session::setKeyBindings(const QString & id)
{
    _emulation->setKeyBindings(id);
}

void Session::setTitle(TitleRole role , const QString & newTitle)
{
    if ( title(role) != newTitle ) {
        if ( role == NameRole ) {
            _nameTitle = newTitle;
        } else if ( role == DisplayedTitleRole ) {
            _displayTitle = newTitle;
        }

        emit titleChanged();
    }
}

QString Session::title(TitleRole role) const
{
    if ( role == NameRole ) {
        return _nameTitle;
    } else if ( role == DisplayedTitleRole ) {
        return _displayTitle;
    } else {
        return QString();
    }
}

void Session::setIconName(const QString & iconName)
{
    if ( iconName != _iconName ) {
        _iconName = iconName;
        emit titleChanged();
    }
}

void Session::setIconText(const QString & iconText)
{
    _iconText = iconText;
    //kDebug(1211)<<"Session setIconText " <<  _iconText;
}

QString Session::iconName() const
{
    return _iconName;
}

QString Session::iconText() const
{
    return _iconText;
}

bool Session::isTitleChanged() const
{
    return _isTitleChanged;
}

void Session::setHistoryType(const HistoryType & hType)
{
    _emulation->setHistory(hType);
}

const HistoryType & Session::historyType() const
{
    return _emulation->history();
}

void Session::clearHistory()
{
    _emulation->clearHistory();
}

// unused currently
bool Session::isMonitorActivity() const
{
    return _monitorActivity;
}
// unused currently
bool Session::isMonitorSilence()  const
{
    return _monitorSilence;
}

void Session::setMonitorActivity(bool _monitor)
{
    _monitorActivity=_monitor;
    _notifiedActivity=false;

    activityStateSet(NOTIFYNORMAL);
}

void Session::setMonitorSilence(bool _monitor)
{
    if (_monitorSilence==_monitor) {
        return;
    }

    _monitorSilence=_monitor;
    if (_monitorSilence) {
        _monitorTimer->start(_silenceSeconds*1000);
    } else {
        _monitorTimer->stop();
    }

    activityStateSet(NOTIFYNORMAL);
}

void Session::setMonitorSilenceSeconds(int seconds)
{
    _silenceSeconds=seconds;
    if (_monitorSilence) {
        _monitorTimer->start(_silenceSeconds*1000);
    }
}

void Session::setAddToUtmp(bool set)
{
    _addToUtmp = set;
}

void Session::setFlowControlEnabled(bool enabled)
{
    if (_flowControl == enabled) {
        return;
    }

    _flowControl = enabled;

//    if (_shellProcess) {
//        _shellProcess->setFlowControlEnabled(_flowControl);
//    }

    emit flowControlEnabledChanged(enabled);
}
bool Session::flowControlEnabled() const
{
    return _flowControl;
}

QSize Session::size()
{
    return _emulation->imageSize();
}

void Session::setSize(const QSize & size)
{
    if ((size.width() <= 1) || (size.height() <= 1)) {
        return;
    }

    emit resizeRequest(size);
}
