/* $XFree86: xc/programs/xterm/wcwidth.h,v 1.2 2001/06/18 19:09:27 dickey Exp $ */

/* Markus Kuhn -- 2001-01-12 -- public domain */
/* Adaptions for KDE by Waldo Bastian <bastian@kde.org> */
/*
    Rewritten for QT4 by e_k <e_k at users.sourceforge.net>
*/


#ifndef _WCWIDTH_H_
#define _WCWIDTH_H_

// Standard
#include <string>



int mk_wcwidth(unsigned int ucs);
int mk_wcwidth_cjk(unsigned int ucs);
int string_width( const std::wstring & wstr );

#define char_width(x) mk_wcwidth_cjk(x)

#endif
