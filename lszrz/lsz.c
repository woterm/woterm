/*
  lsz - send files with x/y/zmodem
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
#include "zglobal.h"

/* char *getenv(); */

#define SS_NORMAL 0
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <ctype.h>
#include <errno.h>
#ifdef _MSC_VER
#include <winsock.h>
#else
#include <sys/select.h>
#include <unistd.h>
#define Sleep sleep
#endif // 

#ifndef R_OK
#  define R_OK 4
#endif

#include "timing.h"

#ifndef STRICT_PROTOTYPES
extern time_t time();
extern char *strerror();
extern char *strstr();
#endif

#ifndef HAVE_ERRNO_DECLARATION
extern int errno;
#endif

unsigned Baudrate=2400;	/* Default, should be set by first mode() call */
unsigned Txwindow;	/* Control the size of the transmitted window */
unsigned Txwspac;	/* Spacing between zcrcq requests */
unsigned Txwcnt;	/* Counter used to space ack requests */
size_t Lrxpos;		/* Receiver's last reported offset */
int errors;
enum zm_type_enum protocol;
int under_rsh=FALSE;
extern int turbo_escape;
static int no_unixmode;

int Canseek=1; /* 1: can; 0: only rewind, -1: neither */

static int zsendfile(struct zm_fileinfo *zi, const char *buf, size_t blen);
static int getnak(void);
static int wctxpn(struct zm_fileinfo *);
static int wcs(const char *oname, const char *remotename);
static size_t zfilbuf(struct zm_fileinfo *zi);
static size_t filbuf(char *buf, size_t count);
static int getzrxinit(void);
static int calc_blklen(long total_sent);
static int sendzsinit(void);
static int wctx(struct zm_fileinfo *);
static int zsendfdata(struct zm_fileinfo *);
static int getinsync(struct zm_fileinfo *, int flag);
static void countem(int argc, char **argv);
static void chkinvok(const char *s);
static void usage(int exitcode, const char *what);
static int zsendcmd(const char *buf, size_t blen);
static void saybibi(void);
static int wcsend(int argc, char *argp[]);
static int wcputsec(char *buf, int sectnum, size_t cseclen);
static void usage1(int exitcode);

#define ZSDATA(x,y,z) \
	do { if (Crc32t) {zsda32(x,y,z); } else {zsdata(x,y,z);}} while(0)

int Filesleft;
long Totalleft;
size_t buffersize=16384;


/*
 * Attention string to be executed by receiver to interrupt streaming data
 *  when an error is detected.  A pause (0336) may be needed before the
 *  ^C (03) or after it.
 */
char Myattn[] = { 03, 0336, 0 };

FILE *input_f;

#define MAX_BLOCK 8192
char txbuf[MAX_BLOCK];

long vpos = 0;			/* Number of bytes read from file */

char Lastrx;
char Crcflg;
int Verbose=0;
int Restricted=0;	/* restricted; no /.. or ../ in filenames */
int Quiet=0;		/* overrides logic that would otherwise set verbose */
int Ascii=0;		/* Add CR's for brain damaged programs */
int Fullname=0;		/* transmit full pathname */
int Dottoslash=0;	/* Change foo.bar.baz to foo/bar/baz */
int firstsec;
int errcnt=0;		/* number of files unreadable */
size_t blklen=128;		/* length of transmitted records */
int Optiong;		/* Let it rip no wait for sector ACK's */
int Totsecs;		/* total number of sectors this file */
int Filcnt=0;		/* count of number of files opened */
int Lfseen=0;
unsigned Rxbuflen = 16384;	/* Receiver's max buffer length */
unsigned Tframlen = 0;	/* Override for tx frame length */
unsigned blkopt=0;		/* Override value for zmodem blklen */
int Rxflags = 0;
int Rxflags2 = 0;
size_t bytcnt;
int Wantfcs32 = TRUE;	/* want to send 32 bit FCS */
char Lzconv;	/* Local ZMODEM file conversion request */
char Lzmanag;	/* Local ZMODEM file management request */
int Lskipnocor;
char Lztrans;
char zconv;		/* ZMODEM file conversion request */
char zmanag;		/* ZMODEM file management request */
char ztrans;		/* ZMODEM file transport request */
int command_mode;		/* Send a command, then exit. */
int Cmdtries = 11;
int Cmdack1;		/* Rx ACKs command, then do it */
int Exitcode;
int enable_timesync=0;
size_t Lastsync;		/* Last offset to which we got a ZRPOS */
int Beenhereb4;		/* How many times we've been ZRPOS'd same place */

int no_timeout=FALSE;
size_t max_blklen=1024;
size_t start_blklen=0;
int zmodem_requested;
time_t stop_time=0;

int error_count;
#define OVERHEAD 18
#define OVER_ERR 20

#define MK_STRING(x) #x

jmp_buf intrjmp;	/* For the interrupt on RX CAN */

static long min_bps;
static long min_bps_time;

static int io_mode_fd=0;
static int zrqinits_sent=0;

int Zctlesc;	/* Encode control characters */
const char *program_name = "sz";
int Zrwindow = 1400;	/* RX window size (controls garbage count) */

#ifdef _MSC_VER
void mycontinue() {
	int pid = GetCurrentProcessId();
	char buf[128];
	sprintf(buf, "lsz:pid=%d", pid);
	MessageBoxA(0, buf, buf, 0);
}

#else
void mycontinue() {
	int count = 10;
	int i = 0;
	do {
		Sleep(1);
	} while (i++ < count);
}
#endif

int 
main(int argc, char **argv)
{
	int npats;
	int dm;
	int i;
	char **patts;
	const char *Cmdstr=NULL;

	//mycontinue();

    if (argc<2){
        usage(2,_("need at least one file to send"));
    }

    from_cu();
    chkinvok(argv[0]);

	Rxtimeout = 600;
    npats = 1;
	Zctlesc = 1;
	Verbose = 2;	
	start_blklen = 8192;
	max_blklen = 8192;
	blklen = start_blklen;
	
    patts=&argv[1];
    npats = argc - 1;

    if (npats < 1 && !command_mode)
        usage(2,_("need at least one file to send"));
    if (command_mode && Restricted) {
        printf(_("Can't send command in restricted mode\n"));
        exit(1);
    }

	zsendline_init();

	if (start_blklen==0) {
		if (protocol == ZM_ZMODEM) {
			start_blklen=1024;
			if (Tframlen) {
				start_blklen=max_blklen=Tframlen;
			}
		} else {
			start_blklen=128;
		}
	}

    if (Fromcu && !Quiet) {
		if (Verbose == 0)
			Verbose = 2;
	}
	vfile("%s %s\n", program_name, VERSION);
	
	io_mode(io_mode_fd,1);
	readline_setup(io_mode_fd, 128, 256);

	if ( protocol!=ZM_XMODEM) {
		if (protocol==ZM_ZMODEM) {
			printf("rz\r");
			fflush(stdout);
		}
		countem(npats, patts);
		if (protocol == ZM_ZMODEM) {
			/* throw away any input already received. This doesn't harm
			 * as we invite the receiver to send it's data again, and
			 * might be useful if the receiver has already died or
			 * if there is dirt left if the line 
			 */

			purgeline(io_mode_fd);
			stohdr(0L);
			if (command_mode)
				Txhdr[ZF0] = ZCOMMAND;
			zshhdr(ZRQINIT, Txhdr);
			zrqinits_sent++;
		}
	}
	fflush(stdout);

	if (Cmdstr) {
		if (getzrxinit()) {
			Exitcode=0200;
			//canit(STDOUT_FILENO);
			canit(1);
		}
		else if (zsendcmd(Cmdstr, strlen(Cmdstr)+1)) {
			Exitcode=0200;
			//canit(STDOUT_FILENO); 
			canit(1);
		}
	} else if (wcsend(npats, patts)==MYERROR) {
		Exitcode=0200;
		//canit(STDOUT_FILENO);
		canit(1);
	}
	fflush(stdout);
	io_mode(io_mode_fd,0);
	if (Exitcode)
		dm=Exitcode;
	else if (errcnt)
		dm=1;
	else
		dm=0;
	
	fputs("\r\n",stderr);
	if (dm)
		fputs(_("Transfer incomplete\n"),stderr);
	else
		fputs(_("Transfer complete\n"),stderr);
	exit(dm);
	/*NOTREACHED*/
}

static int 
send_pseudo(const char *name, const char *data)
{
	char *tmp;
	const char *p;
	int ret=0; /* ok */
	size_t plen;
	int fd;
	int lfd;
	
	p = getenv ("TMPDIR");
	if (!p)
		p = getenv ("TMP");
	if (!p)
		p = "/tmp";
	tmp=malloc(PATH_MAX+1);
	if (!tmp){
		vstringf(_("out of memory"));
    }
	plen=strlen(p);
	memcpy(tmp,p,plen);	
	tmp[plen++]='/';

	lfd=0;
	do {
		if (lfd++==10) {
			free(tmp);
			vstringf (_ ("send_pseudo %s: cannot open tmpfile %s: %s"),
					 name, tmp, strerror (errno));
			vstring ("\r\n");
			return 1;
		}
		sprintf(tmp+plen,"%s.%lu.%d",name,(unsigned long) getpid(),lfd);
		fd=open(tmp,O_WRONLY|O_CREAT|O_EXCL,0700);
		/* is O_EXCL guaranted to not follow symlinks? 
		 * I don`t know ... so be careful
		 */
		if (fd!=-1) {
			struct stat st;
			if (0!=lstat(tmp,&st)) {
				vstringf (_ ("send_pseudo %s: cannot lstat tmpfile %s: %s"),
						 name, tmp, strerror (errno));
				vstring ("\r\n");
				unlink(tmp);
				close(fd);
				fd=-1;
			} else {
				if (S_ISLNK(st.st_mode)) {
					vstringf (_ ("send_pseudo %s: avoiding symlink trap"),name);
					vstring ("\r\n");
					unlink(tmp);
					close(fd);
					fd=-1;
				}
			}
		}
	} while (fd==-1);
	if (write(fd,data,strlen(data))!=(signed long) strlen(data)
		|| close(fd)!=0) {
		vstringf (_ ("send_pseudo %s: cannot write to tmpfile %s: %s"),
				 name, tmp, strerror (errno));
		vstring ("\r\n");
		free(tmp);
		return 1;
	}

	if (wcs (tmp,name) == MYERROR) {
		if (Verbose)
			vstringf (_ ("send_pseudo %s: failed"),name);
		else {
			if (Verbose)
				vstringf (_ ("send_pseudo %s: ok"),name);
			Filcnt--;
		}
		vstring ("\r\n");
		ret=1;
	}
	unlink (tmp);
	free(tmp);
	return ret;
}

static int wcsend (int argc, char *argp[])
{
	int n;

	Crcflg = FALSE;
	firstsec = TRUE;
	bytcnt = (size_t) -1;

	for (n = 0; n < argc; ++n) {
		Totsecs = 0;
		if (wcs (argp[n],NULL) == MYERROR)
			return MYERROR;
	}
	Totsecs = 0;
	if (Filcnt == 0) {			/* bitch if we couldn't open ANY files */
		//canit(STDOUT_FILENO); 
		canit(1);
		vstring ("\r\n");
		vstringf (_ ("Can't open any requested files."));
		vstring ("\r\n");
		return MYERROR;
	}
	if (zmodem_requested){
		saybibi();
	} else if (protocol != ZM_XMODEM) {
		struct zm_fileinfo zi;
		char *pa;
		pa=malloc(PATH_MAX+1);
		*pa='\0';
		zi.fname = pa;
		zi.modtime = 0;
		zi.mode = 0;
		zi.bytes_total = 0;
		zi.bytes_sent = 0;
		zi.bytes_received = 0;
		zi.bytes_skipped = 0;
		wctxpn (&zi);
	}
	return OK;
}

static int
wcs(const char *oname, const char *remotename)
{
	int c;
	struct stat f;
	char *name;
	struct zm_fileinfo zi;
	
	if ((input_f=fopen(oname, "rb"))==NULL) {
		int e=errno;
		vstringf(_("cannot open %s"),oname);
		++errcnt;
		return OK;	/* pass over it, there may be others */
	} else {
		char sep = '/';
        char sep2 = '\\';
		char *p = NULL;
        char *p2 = NULL;
		name=malloc(PATH_MAX+1);
		p = strrchr(oname, sep);
        p2 = strrchr(oname, sep2);
        if (p2 > p) {
            p = p2;
        }
		if (p != NULL) {
			strcpy(name, p + 1);
		} else {
			strcpy(name, oname);
		}
	}

	vpos = 0;
	/* Check for directory or block special files */
	fstat(fileno(input_f), &f);
	c = f.st_mode & S_IFMT;
	if (c == S_IFDIR) {
		vstringf(_("is not a file: %s"),name);
		fclose(input_f);
		return OK;
	}

	zi.fname = name;	
	zi.modtime=f.st_mtime;
	zi.mode=f.st_mode;
	zi.bytes_total= f.st_size;
	zi.bytes_sent=0;
	zi.bytes_received=0;
	zi.bytes_skipped=0;
	zi.eof_seen=0;
	timing(1,NULL);

	++Filcnt;
	switch (wctxpn(&zi)) {
	case MYERROR:
		return MYERROR;
	case ZSKIP:
		vstringf(_("skipped: %s"),name);
		return OK;
	}
	if (!zmodem_requested && wctx(&zi)==MYERROR)
	{
		return MYERROR;
	}

	return 0;
}

/*
 * generate and transmit pathname block consisting of
 *  pathname (null terminated),
 *  file length, mode time and file mode in octal
 *  as provided by the Unix fstat call.
 *  N.B.: modifies the passed name, may extend it!
 */
static int
wctxpn(struct zm_fileinfo *zi)
{
	register char *p, *q;
	char *name2;
	struct stat f;

	name2=malloc(PATH_MAX+1);

	if (protocol==ZM_XMODEM) {
		if (Verbose && *zi->fname && fstat(fileno(input_f), &f)!= -1) {
			vstringf(_("Sending %s, %ld blocks: "),
			  zi->fname, (long) (f.st_size>>7));
		}
		vstringf(_("Give your local XMODEM receive command now."));
		vstring("\r\n");
		return OK;
	}
	if (!zmodem_requested)
		if (getnak()) {
			vfile("getnak failed");
			return MYERROR;
		}

	q = (char *) 0;
	if (Dottoslash) {		/* change . to . */
		for (p=zi->fname; *p; ++p) {
			if (*p == '/')
				q = p;
			else if (*p == '.')
				*(q=p) = '/';
		}
		if (q && strlen(++q) > 8) {	/* If name>8 chars */
			q += 8;			/*   make it .ext */
			strcpy(name2, q);	/* save excess of name */
			*q = '.';
			strcpy(++q, name2);	/* add it back */
		}
	}

	for (p=zi->fname, q=txbuf ; *p; )
		if ((*q++ = *p++) == '/' && !Fullname)
			q = txbuf;
	*q++ = 0;
	p=q;
	while (q < (txbuf + MAX_BLOCK))
		*q++ = 0;
	/* note that we may lose some information here in case mode_t is wider than an 
	 * int. But i believe sending %lo instead of %o _could_ break compatability
	 */
	if (!Ascii && (input_f!=stdin) && *zi->fname && fstat(fileno(input_f), &f)!= -1)
		sprintf(p, "%lu %lo %o 0 %d %ld", (long) f.st_size, f.st_mtime,
		  (unsigned int)((no_unixmode) ? 0 : f.st_mode), 
		  Filesleft, Totalleft);
	if (Verbose)
		vstringf(_("Sending: %s\n"),txbuf);
	Totalleft -= f.st_size;
	if (--Filesleft <= 0)
		Totalleft = 0;
	if (Totalleft < 0)
		Totalleft = 0;

	/* force 1k blocks if name won't fit in 128 byte block */
	if (txbuf[125])
		blklen=1024;
	else {		/* A little goodie for IMP/KMD */
		txbuf[127] = (f.st_size + 127) >>7;
		txbuf[126] = (f.st_size + 127) >>15;
	}
	if (zmodem_requested)
		return zsendfile(zi,txbuf, 1+strlen(p)+(p-txbuf));
	if (wcputsec(txbuf, 0, 128)==MYERROR) {
		vfile("wcputsec failed");
		return MYERROR;
	}
	return OK;
}

static int 
getnak(void)
{
	int firstch;
	int tries=0;

	Lastrx = 0;
	for (;;) {
		tries++;
		switch (firstch = READLINE_PF(100)) {
		case ZPAD:
			if (getzrxinit())
				return MYERROR;
			Ascii = 0;	/* Receiver does the conversion */
			return FALSE;
		case TIMEOUT:
			/* 30 seconds are enough */
			if (tries==3) {
				zperr(_("Timeout on pathname"));
				return TRUE;
			}
			/* don't send a second ZRQINIT _directly_ after the
			 * first one. Never send more then 4 ZRQINIT, because
			 * omen rz stops if it saw 5 of them */
			if ((zrqinits_sent>1 || tries>1) && zrqinits_sent<4) {
				/* if we already sent a ZRQINIT we are using zmodem
				 * protocol and may send further ZRQINITs 
				 */
				stohdr(0L);
				zshhdr(ZRQINIT, Txhdr);
				zrqinits_sent++;
			}
			continue;
		case WANTG:
			io_mode(io_mode_fd,2);	/* Set cbreak, XON/XOFF, etc. */
			Optiong = TRUE;
			blklen=1024;
		case WANTCRC:
			Crcflg = TRUE;
		case NAK:
			return FALSE;
		case CAN:
			if ((firstch = READLINE_PF(20)) == CAN && Lastrx == CAN)
				return TRUE;
		default:
			break;
		}
		Lastrx = firstch;
	}
}


static int 
wctx(struct zm_fileinfo *zi)
{
	register size_t thisblklen;
	register int sectnum, attempts, firstch;

	firstsec=TRUE;  thisblklen = blklen;
	vfile("wctx:file length=%ld", (long) zi->bytes_total);

	while ((firstch=READLINE_PF(Rxtimeout))!=NAK && firstch != WANTCRC
	  && firstch != WANTG && firstch!=TIMEOUT && firstch!=CAN)
		;
	if (firstch==CAN) {
		zperr(_("Receiver Cancelled"));
		return MYERROR;
	}
	if (firstch==WANTCRC)
		Crcflg=TRUE;
	if (firstch==WANTG)
		Crcflg=TRUE;
	sectnum=0;
	for (;;) {
		if (zi->bytes_total <= (zi->bytes_sent + 896L))
			thisblklen = 128;
		if ( !filbuf(txbuf, thisblklen))
			break;
		if (wcputsec(txbuf, ++sectnum, thisblklen)==MYERROR)
			return MYERROR;
		zi->bytes_sent += thisblklen;
	}
	fclose(input_f);
	attempts=0;
	do {
		purgeline(io_mode_fd);
		sendline(EOT);
		flushmo();
		++attempts;
	} while ((firstch=(READLINE_PF(Rxtimeout)) != ACK) && attempts < RETRYMAX);
	if (attempts == RETRYMAX) {
		zperr(_("No ACK on EOT"));
		return MYERROR;
	}
	else
		return OK;
}

static int 
wcputsec(char *buf, int sectnum, size_t cseclen)
{
	int checksum, wcj;
	char *cp;
	unsigned oldcrc;
	int firstch;
	int attempts;

	firstch=0;	/* part of logic to detect CAN CAN */

	if (Verbose>1) {
		vchar('\r');
		if (protocol==ZM_XMODEM) {
			vstringf(_("Xmodem sectors/kbytes sent: %3d/%2dk"), Totsecs, Totsecs/8 );
		} else {
			vstringf(_("Ymodem sectors/kbytes sent: %3d/%2dk"), Totsecs, Totsecs/8 );
		}
	}
	for (attempts=0; attempts <= RETRYMAX; attempts++) {
		Lastrx= firstch;
		sendline(cseclen==1024?STX:SOH);
		sendline(sectnum);
		sendline(-sectnum -1);
		oldcrc=checksum=0;
		for (wcj=cseclen,cp=buf; --wcj>=0; ) {
			sendline(*cp);
			oldcrc=updcrc((0377& *cp), oldcrc);
			checksum += *cp++;
		}
		if (Crcflg) {
			oldcrc=updcrc(0,updcrc(0,oldcrc));
			sendline((int)oldcrc>>8);
			sendline((int)oldcrc);
		}
		else
			sendline(checksum);

		flushmo();
		if (Optiong) {
			firstsec = FALSE; return OK;
		}
		firstch = READLINE_PF(Rxtimeout);
gotnak:
		switch (firstch) {
		case CAN:
			if(Lastrx == CAN) {
cancan:
				zperr(_("Cancelled"));  return MYERROR;
			}
			break;
		case TIMEOUT:
			zperr(_("Timeout on sector ACK")); continue;
		case WANTCRC:
			if (firstsec)
				Crcflg = TRUE;
		case NAK:
			zperr(_("NAK on sector")); continue;
		case ACK: 
			firstsec=FALSE;
			Totsecs += (cseclen>>7);
			return OK;
		case MYERROR:
			zperr(_("Got burst for sector ACK")); break;
		default:
			zperr(_("Got %02x for sector ACK"), firstch); break;
		}
		for (;;) {
			Lastrx = firstch;
			if ((firstch = READLINE_PF(Rxtimeout)) == TIMEOUT)
				break;
			if (firstch == NAK || firstch == WANTCRC)
				goto gotnak;
			if (firstch == CAN && Lastrx == CAN)
				goto cancan;
		}
	}
	zperr(_("Retry Count Exceeded"));
	return MYERROR;
}

/* fill buf with count chars padding with ^Z for CPM */
static size_t 
filbuf(char *buf, size_t count)
{
	int c;
	size_t m;

	if ( !Ascii) {
		m = read(fileno(input_f), buf, count);
		if (m <= 0)
			return 0;
		while (m < count)
			buf[m++] = 032;
		return count;
	}
	m=count;
	if (Lfseen) {
		*buf++ = 012; --m; Lfseen = 0;
	}
	while ((c=getc(input_f))!=EOF) {
		if (c == 012) {
			*buf++ = 015;
			if (--m == 0) {
				Lfseen = TRUE; break;
			}
		}
		*buf++ =c;
		if (--m == 0)
			break;
	}
	if (m==count)
		return 0;
	else
		while (m--!=0)
			*buf++ = CPMEOF;
	return count;
}

/* Fill buffer with blklen chars */
static size_t
zfilbuf (struct zm_fileinfo *zi)
{
	size_t n;

	n = fread(txbuf, 1, blklen, input_f);
	if (n < blklen) {
		zi->eof_seen = 1;
	} else {
		/* save one empty paket in case file ends ob blklen boundary */
		int c = getc(input_f);

		if (c != EOF || !feof(input_f)) {
			ungetc(c, input_f);
		} else {
			zi->eof_seen = 1;
		}
	}
	return n;
}

static void
usage1 (int exitcode)
{
	usage (exitcode, NULL);
}

static void
usage(int exitcode, const char *what)
{
	FILE *f=stdout;

	if (exitcode)
	{
		if (what)
			fprintf(stderr, "%s: %s\n",program_name,what);
	    fprintf (stderr, _("Try `%s --help' for more information.\n"),
            program_name);
		exit(exitcode);
	}

	fprintf(f, _("%s version %s\n"), program_name,
		VERSION);

	fprintf(f,_("Usage: %s [options] file ...\n"),
		program_name);
	fprintf(f,_("   or: %s [options] -{c|i} COMMAND\n"),program_name);
	fputs(_("Send file(s) with ZMODEM/YMODEM/XMODEM protocol\n"),f);
	fputs(_(
		"    (X) = option applies to XMODEM only\n"
		"    (Y) = option applies to YMODEM only\n"
		"    (Z) = option applies to ZMODEM only\n"
		),f);
	/* splitted into two halves for really bad compilers */
	fputs(_(
"  -+, --append                append to existing destination file (Z)\n"
"  -2, --twostop               use 2 stop bits\n"
"  -4, --try-4k                go up to 4K blocksize\n"
"      --start-4k              start with 4K blocksize (doesn't try 8)\n"
"  -8, --try-8k                go up to 8K blocksize\n"
"      --start-8k              start with 8K blocksize\n"
"  -a, --ascii                 ASCII transfer (change CR/LF to LF)\n"
"  -b, --binary                binary transfer\n"
"  -B, --bufsize N             buffer N bytes (N==auto: buffer whole file)\n"
"  -c, --command COMMAND       execute remote command COMMAND (Z)\n"
"  -C, --command-tries N       try N times to execute a command (Z)\n"
"  -d, --dot-to-slash          change '.' to '/' in pathnames (Y/Z)\n"
"      --delay-startup N       sleep N seconds before doing anything\n"
"  -e, --escape                escape all control characters (Z)\n"
"  -E, --rename                force receiver to rename files it already has\n"
"  -f, --full-path             send full pathname (Y/Z)\n"
"  -i, --immediate-command CMD send remote CMD, return immediately (Z)\n"
"  -h, --help                  print this usage message\n"
"  -k, --1k                    send 1024 byte packets (X)\n"
"  -L, --packetlen N           limit subpacket length to N bytes (Z)\n"
"  -l, --framelen N            limit frame length to N bytes (l>=L) (Z)\n"
"  -m, --min-bps N             stop transmission if BPS below N\n"
"  -M, --min-bps-time N          for at least N seconds (default: 120)\n"
		),f);
	fputs(_(
"  -n, --newer                 send file if source newer (Z)\n"
"  -N, --newer-or-longer       send file if source newer or longer (Z)\n"
"  -o, --16-bit-crc            use 16 bit CRC instead of 32 bit CRC (Z)\n"
"  -O, --disable-timeouts      disable timeout code, wait forever\n"
"  -p, --protect               protect existing destination file (Z)\n"
"  -r, --resume                resume interrupted file transfer (Z)\n"
"  -R, --restricted            restricted, more secure mode\n"
"  -q, --quiet                 quiet (no progress reports)\n"
"  -s, --stop-at {HH:MM|+N}    stop transmission at HH:MM or in N seconds\n"
"      --tcp                   build a TCP connection to transmit files\n"
"      --tcp-server            open socket, wait for connection\n"
"  -u, --unlink                unlink file after transmission\n"
"  -U, --unrestrict            turn off restricted mode (if allowed to)\n"
"  -v, --verbose               be verbose, provide debugging information\n"
"  -w, --windowsize N          Window is N bytes (Z)\n"
"  -X, --xmodem                use XMODEM protocol\n"
"  -y, --overwrite             overwrite existing files\n"
"  -Y, --overwrite-or-skip     overwrite existing files, else skip\n"
"      --ymodem                use YMODEM protocol\n"
"  -Z, --zmodem                use ZMODEM protocol\n"
"\n"
"short options use the same arguments as the long ones\n"
	),f);
	exit(exitcode);
}

/*
 * Get the receiver's init parameters
 */
static int 
getzrxinit(void)
{
	static int dont_send_zrqinit=1;
	int old_timeout=Rxtimeout;
	int n;
	struct stat f;
	size_t rxpos;
	int timeouts=0;

	Rxtimeout=100; /* 10 seconds */
	/* XXX purgeline(io_mode_fd); this makes _real_ trouble. why? -- uwe */

	for (n=10; --n>=0; ) {
		/* we might need to send another zrqinit in case the first is 
		 * lost. But *not* if getting here for the first time - in
		 * this case we might just get a ZRINIT for our first ZRQINIT.
		 * Never send more then 4 ZRQINIT, because
		 * omen rz stops if it saw 5 of them.
		 */
		if (zrqinits_sent<4 && n!=10 && !dont_send_zrqinit) {
			zrqinits_sent++;
			stohdr(0L);
			zshhdr(ZRQINIT, Txhdr);
		}
		dont_send_zrqinit=0;
		
		switch (zgethdr(Rxhdr, 1,&rxpos)) {
		case ZCHALLENGE:	/* Echo receiver's challenge numbr */
			stohdr(rxpos);
			zshhdr(ZACK, Txhdr);
			continue;
		case ZCOMMAND:		/* They didn't see our ZRQINIT */
			/* ??? Since when does a receiver send ZCOMMAND?  -- uwe */
			continue;
		case ZRINIT:
			Rxflags = 0377 & Rxhdr[ZF0];
			Rxflags2 = 0377 & Rxhdr[ZF1];
			Txfcs32 = (Wantfcs32 && (Rxflags & CANFC32));
			{
				int old=Zctlesc;
				Zctlesc |= Rxflags & TESCCTL;
				/* update table - was initialised to not escape */
				if (Zctlesc && !old)
					zsendline_init();
			}
			Rxbuflen = (0377 & Rxhdr[ZP0])+((0377 & Rxhdr[ZP1])<<8);
			if ( !(Rxflags & CANFDX))
				Txwindow = 0;
			vfile("Rxbuflen=%d Tframlen=%d", Rxbuflen, Tframlen);
			io_mode(io_mode_fd,2);	/* Set cbreak, XON/XOFF, etc. */
			/* Use MAX_BLOCK byte frames if no sample/interrupt */
			if (Rxbuflen < 32 || Rxbuflen > MAX_BLOCK) {
				Rxbuflen = MAX_BLOCK;
				vfile("Rxbuflen=%d", Rxbuflen);
			}
			/* Override to force shorter frame length */
			if (Tframlen && Rxbuflen > Tframlen)
				Rxbuflen = Tframlen;
			if ( !Rxbuflen)
				Rxbuflen = 1024;
			vfile("Rxbuflen=%d", Rxbuflen);

			/* If using a pipe for testing set lower buf len */
			fstat(0, &f);
#if defined(S_ISCHR)
			if (! (S_ISCHR(f.st_mode))) {
#else
			if ((f.st_mode & S_IFMT) != S_IFCHR) {
#endif
				Rxbuflen = MAX_BLOCK;
			}
			/*
			 * If input is not a regular file, force ACK's to
			 *  prevent running beyond the buffer limits
			 */
			if ( !command_mode) {
				fstat(fileno(input_f), &f);
#if defined(S_ISREG)
				if (!(S_ISREG(f.st_mode))) {
#else
				if ((f.st_mode & S_IFMT) != S_IFREG) {
#endif
					Canseek = -1;
					/* return MYERROR; */
				}
			}
			/* Set initial subpacket length */
			if (blklen < 1024) {	/* Command line override? */
				if (Baudrate > 300)
					blklen = 256;
				if (Baudrate > 1200)
					blklen = 512;
				if (Baudrate > 2400)
					blklen = 1024;
			}
			if (Rxbuflen && blklen>Rxbuflen)
				blklen = Rxbuflen;
			if (blkopt && blklen > blkopt)
				blklen = blkopt;
			vfile("Rxbuflen=%d blklen=%d", Rxbuflen, blklen);
			vfile("Txwindow = %u Txwspac = %d", Txwindow, Txwspac);
			Rxtimeout=old_timeout;
			return (sendzsinit());
		case ZCAN:
		case TIMEOUT:
			if (timeouts++==0)
				continue; /* force one other ZRQINIT to be sent */
			return MYERROR;
		case ZRQINIT:
			if (Rxhdr[ZF0] == ZCOMMAND)
				continue;
		default:
			zshhdr(ZNAK, Txhdr);
			continue;
		}
	}
	return MYERROR;
}

/* Send send-init information */
static int 
sendzsinit(void)
{
	int c;

	if (Myattn[0] == '\0' && (!Zctlesc || (Rxflags & TESCCTL)))
		return OK;
	errors = 0;
	for (;;) {
		stohdr(0L);
		if (Zctlesc) {
			Txhdr[ZF0] |= TESCCTL; zshhdr(ZSINIT, Txhdr);
		}
		else
			zsbhdr(ZSINIT, Txhdr);
		ZSDATA(Myattn, 1+strlen(Myattn), ZCRCW);
		c = zgethdr(Rxhdr, 1,NULL);
		switch (c) {
		case ZCAN:
			return MYERROR;
		case ZACK:
			return OK;
		default:
			if (++errors > 19)
				return MYERROR;
			continue;
		}
	}
}

/* Send file name and related info */
static int 
zsendfile(struct zm_fileinfo *zi, const char *buf, size_t blen)
{
	int c;
	unsigned long crc;
	size_t rxpos;

	/* we are going to send a ZFILE. There cannot be much useful
	 * stuff in the line right now (*except* ZCAN?). 
	 */
#if 0
	purgeline(io_mode_fd); /* might possibly fix stefan glasers problems */
#endif

	for (;;) {
		Txhdr[ZF0] = Lzconv;	/* file conversion request */
		Txhdr[ZF1] = Lzmanag;	/* file management request */
		if (Lskipnocor)
			Txhdr[ZF1] |= ZF1_ZMSKNOLOC;
		Txhdr[ZF2] = Lztrans;	/* file transport request */
		Txhdr[ZF3] = 0;
		zsbhdr(ZFILE, Txhdr);
		ZSDATA(buf, blen, ZCRCW);
again:
		c = zgethdr(Rxhdr, 1, &rxpos);
		switch (c) {
		case ZRINIT:
			while ((c = READLINE_PF(50)) > 0)
				if (c == ZPAD) {
					goto again;
				}
			/* **** FALL THRU TO **** */
		default:
			continue;
		case ZRQINIT:  /* remote site is sender! */
			if (Verbose)
				vstringf(_("got ZRQINIT"));
			return MYERROR;
		case ZCAN:
			if (Verbose)
				vstringf(_("got ZCAN"));
			return MYERROR;
		case TIMEOUT:
			return MYERROR;
		case ZABORT:
			return MYERROR;
		case ZFIN:
			return MYERROR;
		case ZCRC:
			crc = 0xFFFFFFFFL;
			if (Canseek >= 0) {
				if (rxpos==0) {
					struct stat st;
					if (0==fstat(fileno(input_f),&st)) {
						rxpos=st.st_size;
					} else
						rxpos=-1;
				}
				while (rxpos-- && ((c = getc(input_f)) != EOF))
					crc = UPDC32(c, crc);
				crc = ~crc;
				clearerr(input_f);	/* Clear EOF */
				fseek(input_f, 0L, 0);
			}
			stohdr(crc);
			zsbhdr(ZCRC, Txhdr);
			goto again;
		case ZSKIP:
			if (input_f)
				fclose(input_f);
			vfile("receiver skipped");
			return c;
		case ZRPOS:
			/*
			 * Suppress zcrcw request otherwise triggered by
			 * lastsync==bytcnt
			 */

			if (rxpos && fseek(input_f, (long) rxpos, 0)) {
				int er=errno;
				vfile("fseek failed: %s", strerror(er));
				return MYERROR;
			}
			if (rxpos)
				zi->bytes_skipped=rxpos;
			bytcnt = zi->bytes_sent = rxpos;
			Lastsync = rxpos -1;
	 		return zsendfdata(zi);
		}
	}
}

/* Send the data in the file */
static int
zsendfdata (struct zm_fileinfo *zi)
{
	static int c;
	int newcnt;
	static int junkcount;				/* Counts garbage chars received by TX */
	static size_t last_txpos = 0;
	static long last_bps = 0;
	static long not_printed = 0;
	static long total_sent = 0;
	static time_t low_bps=0;

	Lrxpos = 0;
	junkcount = 0;
	Beenhereb4 = 0;
  somemore:
	if (setjmp (intrjmp)) {
	  waitack:
		junkcount = 0;
		c = getinsync (zi, 0);
	  gotack:
		switch (c) {
		default:
			if (input_f)
				fclose (input_f);
			return MYERROR;
		case ZCAN:
			if (input_f)
				fclose (input_f);
			return MYERROR;
		case ZSKIP:
			if (input_f)
				fclose (input_f);
			return c;
		case ZACK:
		case ZRPOS:
			break;
		case ZRINIT:
			return OK;
		}
	}

	newcnt = Rxbuflen;
	Txwcnt = 0;
	stohdr (zi->bytes_sent);
	zsbhdr (ZDATA, Txhdr);

	do {
		size_t n;
		int e;
		unsigned old = blklen;
		blklen = calc_blklen (total_sent);
		total_sent += blklen + OVERHEAD;
		if (Verbose > 2 && blklen != old)
			vstringf (_("blklen now %d\n"), blklen);

		n = zfilbuf (zi);
		if (zi->eof_seen) {
			e = ZCRCE;
			if (Verbose>3)
				vstring("e=ZCRCE/eof seen");
		} else if (junkcount > 3) {
			e = ZCRCW;
			if (Verbose>3)
				vstring("e=ZCRCW/junkcount > 3");
		} else if (bytcnt == Lastsync) {
			e = ZCRCW;
			if (Verbose>3)
				vstringf("e=ZCRCW/bytcnt == Lastsync == %ld", 
					(unsigned long) Lastsync);
#if 0
		/* what is this good for? Rxbuflen/newcnt normally are short - so after
		 * a few KB ZCRCW will be used? (newcnt is never incremented)
		 */
		} else if (Rxbuflen && (newcnt -= n) <= 0) {
			e = ZCRCW;
			if (Verbose>3)
				vstringf("e=ZCRCW/Rxbuflen(newcnt=%ld,n=%ld)", 
					(unsigned long) newcnt,(unsigned long) n);
#endif
		} else if (Txwindow && (Txwcnt += n) >= Txwspac) {
			Txwcnt = 0;
			e = ZCRCQ;
			if (Verbose>3)
				vstring("e=ZCRCQ/Window");
		} else {
			e = ZCRCG;
			if (Verbose>3)
				vstring("e=ZCRCG");
		}
		if ((Verbose > 1 || min_bps || stop_time)
			&& (not_printed > (min_bps ? 3 : 7) 
				|| zi->bytes_sent > last_bps / 2 + last_txpos)) {
			int minleft = 0;
			int secleft = 0;
			time_t now;
			last_bps = (zi->bytes_sent / timing (0,&now));
			if (last_bps > 0) {
				minleft = (zi->bytes_total - zi->bytes_sent) / last_bps / 60;
				secleft = ((zi->bytes_total - zi->bytes_sent) / last_bps) % 60;
			}
			if (min_bps) {
				if (low_bps) {
					if (last_bps<min_bps) {
						if (now-low_bps>=min_bps_time) {
							/* too bad */
							if (Verbose) {
								vstringf(_("zsendfdata: bps rate %ld below min %ld"),
								  last_bps, min_bps);
								vstring("\r\n");
							}
							return MYERROR;
						}
					} else
						low_bps=0;
				} else if (last_bps < min_bps) {
					low_bps=now;
				}
			}
			if (stop_time && now>=stop_time) {
				/* too bad */
				if (Verbose) {
					vstring(_("zsendfdata: reached stop time"));
					vstring("\r\n");
				}
				return MYERROR;
			}

			if (Verbose > 1) {
				vchar ('\r');
				vstringf (_("Bytes Sent:%7ld/%7ld"), (long) zi->bytes_sent, (long) zi->bytes_total);
			}
			last_txpos = zi->bytes_sent;
		} else if (Verbose)
			not_printed++;
		ZSDATA (txbuf, n, e);
		bytcnt = zi->bytes_sent += n;
		if (e == ZCRCW)
			goto waitack;
		if (Txwindow) {
			size_t tcount = 0;
			while ((tcount = zi->bytes_sent - Lrxpos) >= Txwindow) {
				vfile ("%ld (%ld,%ld) window >= %u", tcount, 
					(long) zi->bytes_sent, (long) Lrxpos,
					Txwindow);
				if (e != ZCRCQ)
					ZSDATA (txbuf, 0, e = ZCRCQ);
				c = getinsync (zi, 1);
				if (c != ZACK) {
					ZSDATA (txbuf, 0, ZCRCE);
					goto gotack;
				}
			}
			vfile ("window = %ld", tcount);
		}
	} while (!zi->eof_seen);


	for (;;) {
		stohdr (zi->bytes_sent);
		zsbhdr (ZEOF, Txhdr);
		switch (getinsync (zi, 0)) {
		case ZACK:
			continue;
		case ZRPOS:
			goto somemore;
		case ZRINIT:
			return OK;
		case ZSKIP:
			if (input_f)
				fclose (input_f);
			return c;
		default:
			if (input_f)
				fclose (input_f);
			return MYERROR;
		}
	}
}

static int
calc_blklen(long total_sent)
{
	static long total_bytes=0;
	static int calcs_done=0;
	static long last_error_count=0;
	static int last_blklen=0;
	static long last_bytes_per_error=0;
	unsigned long best_bytes=0;
	long best_size=0;
	long this_bytes_per_error;
	long d;
	unsigned int i;
	if (total_bytes==0)
	{
		/* called from countem */
		total_bytes=total_sent;
		return 0;
	}

	/* it's not good to calc blklen too early */
	if (calcs_done++ < 5) {
		if (error_count && start_blklen >1024)
			return last_blklen=1024;
		else 
			last_blklen/=2;
		return last_blklen=start_blklen;
	}

	if (!error_count) {
		/* that's fine */
		if (start_blklen==max_blklen)
			return start_blklen;
		this_bytes_per_error=2141483647; //LONG_MAX
		goto calcit;
	}

	if (error_count!=last_error_count) {
		/* the last block was bad. shorten blocks until one block is
		 * ok. this is because very often many errors come in an
		 * short period */
		if (error_count & 2)
		{
			last_blklen/=2;
			if (last_blklen < 32)
				last_blklen = 32;
			else if (last_blklen > 512)
				last_blklen=512;
			if (Verbose > 3)
				vstringf(_("calc_blklen: reduced to %d due to error\n"),
					last_blklen);
		}
		last_error_count=error_count;
		last_bytes_per_error=0; /* force recalc */
		return last_blklen;
	}

	this_bytes_per_error=total_sent / error_count;
		/* we do not get told about every error, because
		 * there may be more than one error per failed block.
		 * but one the other hand some errors are reported more
		 * than once: If a modem buffers more than one block we
		 * get at least two ZRPOS for the same position in case
		 * *one* block has to be resent.
		 * so don't do this:
		 * this_bytes_per_error/=2;
		 */
	/* there has to be a margin */
	if (this_bytes_per_error<100)
		this_bytes_per_error=100;

	/* be nice to the poor machine and do the complicated things not
	 * too often
	 */
	if (last_bytes_per_error>this_bytes_per_error)
		d=last_bytes_per_error-this_bytes_per_error;
	else
		d=this_bytes_per_error-last_bytes_per_error;
	if (d<4)
	{
		if (Verbose > 3)
		{
			vstringf(_("calc_blklen: returned old value %d due to low bpe diff\n"),
				last_blklen);
			vstringf(_("calc_blklen: old %ld, new %ld, d %ld\n"),
				last_bytes_per_error,this_bytes_per_error,d );
		}
		return last_blklen;
	}
	last_bytes_per_error=this_bytes_per_error;

calcit:
	if (Verbose > 3)
		vstringf(_("calc_blklen: calc total_bytes=%ld, bpe=%ld, ec=%ld\n"),
			total_bytes,this_bytes_per_error,(long) error_count);
	for (i=32;i<=max_blklen;i*=2) {
		long ok; /* some many ok blocks do we need */
		long failed; /* and that's the number of blocks not transmitted ok */
		unsigned long transmitted;
		ok=total_bytes / i + 1;
		failed=((long) i + OVERHEAD) * ok / this_bytes_per_error;
		transmitted=total_bytes + ok * OVERHEAD  
			+ failed * ((long) i+OVERHEAD+OVER_ERR);
		if (Verbose > 4)
			vstringf(_("calc_blklen: blklen %d, ok %ld, failed %ld -> %lu\n"),
				i,ok,failed,transmitted);
		if (transmitted < best_bytes || !best_bytes)
		{
			best_bytes=transmitted;
			best_size=i;
		}
	}
	if (best_size > 2*last_blklen)
		best_size=2*last_blklen;
	last_blklen=best_size;
	if (Verbose > 3)
		vstringf(_("calc_blklen: returned %d as best\n"),
			last_blklen);
	return last_blklen;
}

/*
 * Respond to receiver's complaint, get back in sync with receiver
 */
static int 
getinsync(struct zm_fileinfo *zi, int flag)
{
	int c;
	size_t rxpos;

	for (;;) {
		c = zgethdr(Rxhdr, 0, &rxpos);
		switch (c) {
		case ZCAN:
		case ZABORT:
		case ZFIN:
		case TIMEOUT:
			return MYERROR;
		case ZRPOS:
			if(zi->eof_seen != 1)
			{
				/* ************************************* */
				/*  If sending to a buffered modem, you  */
				/*   might send a break at this point to */
				/*   dump the modem's buffer.		 */
				if (input_f)
					clearerr(input_f);	/* In case file EOF seen */

				if (fseek(input_f, (long) rxpos, 0))
					return MYERROR;
				zi->eof_seen = 0;
				bytcnt = Lrxpos = zi->bytes_sent = rxpos;
				if (Lastsync == rxpos) {
					error_count++;
				}
				Lastsync = rxpos;
			}
			return c;
		case ZACK:
			Lrxpos = rxpos;
			if (flag || zi->bytes_sent == rxpos)
				return ZACK;
			continue;
		case ZRINIT:
		case ZSKIP:
			if (input_f)
				fclose(input_f);
			return c;
		case MYERROR:
		default:
			error_count++;
			zsbhdr(ZNAK, Txhdr);
			continue;
		}
	}
}


/* Say "bibi" to the receiver, try to do it cleanly */
static void saybibi(void)
{
	for (;;) {
		stohdr(0L);		/* CAF Was zsbhdr - minor change */
		zshhdr(ZFIN, Txhdr);	/*  to make debugging easier */
		switch (zgethdr(Rxhdr, 0,NULL)) {
		case ZFIN:
			sendline('O');
			sendline('O');
			flushmo();
		case ZCAN:
		case TIMEOUT:
			return;
		}
	}
}

/* Send command and related info */
static int zsendcmd(const char *buf, size_t blen)
{
	int c;
	size_t rxpos;
#ifdef _MSC_VER
	int cmdnum = GetCurrentProcessId();
#else
	pid_t cmdnum = getpid();
#endif
	errors = 0;
	for (;;) {
		stohdr((size_t) cmdnum);
		Txhdr[ZF0] = Cmdack1;
		zsbhdr(ZCOMMAND, Txhdr);
		ZSDATA(buf, blen, ZCRCW);
listen:
		Rxtimeout = 100;		/* Ten second wait for resp. */
		c = zgethdr(Rxhdr, 1, &rxpos);

		switch (c) {
		case ZRINIT:
			goto listen;	/* CAF 8-21-87 */
		case MYERROR:
		case TIMEOUT:
			if (++errors > Cmdtries)
				return MYERROR;
			continue;
		case ZCAN:
		case ZABORT:
		case ZFIN:
		case ZSKIP:
		case ZRPOS:
			return MYERROR;
		default:
			if (++errors > 20)
				return MYERROR;
			continue;
		case ZCOMPL:
			Exitcode = rxpos;
			saybibi();
			return OK;
		case ZRQINIT:
			vfile("******** RZ *******");
			system("rz");
			vfile("******** SZ *******");
			goto listen;
		}
	}
}

/*
 * If called as lsb use YMODEM protocol
 */
static void
chkinvok (const char *s)
{
	const char *p;

	p = s;
	while (*p == '-')
		s = ++p;
	while (*p)
		if (*p++ == '/')
			s = p;
	if (*s == 'v') {
		Verbose = 1;
		++s;
	}
	program_name = s;
	if (*s == 'l')
		s++;					/* lsz -> sz */
	protocol = ZM_ZMODEM;
	if (s[0] == 's' && s[1] == 'x')
		protocol = ZM_XMODEM;
	if (s[0] == 's' && (s[1] == 'b' || s[1] == 'y')) {
		protocol = ZM_YMODEM;
	}
}

static void
countem (int argc, char **argv)
{
	struct stat f;

	for (Totalleft = 0, Filesleft = 0; --argc >= 0; ++argv) {
		f.st_size = -1;
		if (Verbose > 2) {
			vstringf ("\nCountem: %03d %s ", argc, *argv);
		}
		if (access (*argv, R_OK) >= 0 && stat (*argv, &f) >= 0) {
			int c;
			c = f.st_mode & S_IFMT;
			if (c != S_IFDIR) {
				++Filesleft;
				Totalleft += f.st_size;
			}
		} else if (strcmp (*argv, "-") == 0) {
			++Filesleft;
			Totalleft += DEFBYTL;
		}
		if (Verbose > 2)
			vstringf (" %ld", (long) f.st_size);
	}
	if (Verbose > 2)
		vstringf (_("\ncountem: Total %d %ld\n"),
				 Filesleft, Totalleft);
	calc_blklen (Totalleft);
}

/* End of lsz.c */


