/*
  rbsb.c - terminal handling stuff for lrzsz
  Copyright (C) until 1988 Chuck Forsberg (Omen Technology INC)
  Copyright (C) 1994 Matt Porter, Michael D. Black
  Copyright (C) 1996, 1997 Uwe Ohse

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
  02111-1307, USA.

  originally written by Chuck Forsberg
*/

/*
 *  Rev 05-05-1988
 *  ============== (not quite, but originated there :-). -- uwe 
 */
#include "zglobal.h"

#include <stdio.h>
#include <errno.h>

/*
 * return 1 if stdout and stderr are different devices
 *  indicating this program operating with a modem on a
 *  different line
 */
int Fromcu;		/* Were called from cu or yam */
int from_cu(void)
{
	struct stat a, b;
	int help=0;


	/* in case fstat fails */
	a.st_rdev=b.st_rdev=a.st_dev=b.st_dev=help;

	fstat(1, &a); fstat(2, &b);

	Fromcu = (a.st_rdev != b.st_rdev) || (a.st_dev != b.st_dev);

	return Fromcu;
}



int Twostop;		/* Use two stop bits */

/*
 * mode(n)
 *  3: save old tty stat, set raw mode with flow control
 *  2: set XON/XOFF for sb/sz with ZMODEM or YMODEM-g
 *  1: save old tty stat, set raw mode 
 *  0: restore original tty mode
 */
int io_mode(int fd, int n)
{
	static int did0 = FALSE;

	vfile("mode:%d", n);
	return OK;
}

void sendbrk(int fd)
{
	//tcsendbreak(fd,0);
}

void purgeline(int fd)
{
	readline_purge();
	//ioctl(fd, TCFLSH, 0);
}

/* End of rbsb.c */
