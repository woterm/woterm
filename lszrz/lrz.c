/*
  lrz - receive files with x/y/zmodem
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

#define SS_NORMAL 0

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "timing.h"

#ifdef _MSC_VER
#include <windows.h>
#else
#define Sleep sleep
#endif // _MS


#ifndef STRICT_PROTOTYPES

extern time_t time();

extern char *strerror();

extern char *strstr();

#endif

#ifndef HAVE_ERRNO_DECLARATION
extern int errno;
#endif

#define MAX_BLOCK 8192

/*
 * Max value for HOWMANY is 255 if NFGVMIN is not defined.
 *   A larger value reduces system overhead but may evoke kernel bugs.
 *   133 corresponds to an XMODEM/CRC sector
 */

#include "mywin.h"

unsigned Baudrate = 2400;

FILE *fout;


int Lastrx;
int Crcflg;
int Firstsec;
int errors;
int Restricted = 1;    /* restricted; no /.. or ../ in filenames */
int Readnum = HOWMANY;    /* Number of bytes to ask for in read() from modem */
int skip_if_not_found;

char *Pathname;
const char *program_name;        /* the name by which we were called */

int MakeLCPathname = TRUE;    /* make received pathname lower case */
int Verbose = 0;
int Quiet = 0;        /* overrides logic that would otherwise set verbose */
int Nflag = 0;        /* Don't really transfer files */
int Rxclob = FALSE;    /* Clobber existing file */
int Rxbinary = FALSE;    /* receive all files in bin mode */
int Rxascii = FALSE;    /* receive files in ascii (translate) mode */
int Thisbinary;        /* current file is to be received in bin mode */
int try_resume = FALSE;
int allow_remote_commands = FALSE;
int junk_path = FALSE;
int no_timeout = FALSE;
enum zm_type_enum protocol;
int under_rsh = FALSE;
int zmodem_requested = FALSE;

char secbuf[MAX_BLOCK + 1];

#if defined(F_GETFD) && defined(F_SETFD) && defined(O_SYNC)
static int o_sync = 0;
#endif

static int rzfiles(struct zm_fileinfo *);

static int tryz(void);

static void checkpath(const char *name);

static void chkinvok(const char *s);

static void report(int sct);

static void uncaps(char *s);

static int IsAnyLower(const char *s);

static int putsec(struct zm_fileinfo *zi, char *buf, size_t n);

static int make_dirs(char *pathname);

static int procheader(char *name, struct zm_fileinfo *);

static int wcgetsec(size_t * Blklen,
                                 char *rxbuf,
                                 unsigned int maxtime);

static int wcrx(struct zm_fileinfo *);

static int wcrxpn(struct zm_fileinfo *, char *rpn);

static int wcreceive(int argc, char **argp);

static int rzfile(struct zm_fileinfo *);

static void usage(int exitcode, const char *what);

static void usage1(int exitcode);

static void exec2(const char *s);

static int closeit(struct zm_fileinfo *);

static void ackbibi(void);

static int sys2(const char *s);

static void zmputs(const char *s);

static size_t getfree(void);

static long buffersize = 32768;
static unsigned long min_bps = 0;
static long min_bps_time = 120;

char Lzmanag;        /* Local file management request */
char zconv;        /* ZMODEM file conversion request */
char zmanag;        /* ZMODEM file management request */
char ztrans;        /* ZMODEM file transport request */
int Zctlesc;        /* Encode control characters */
int Zrwindow = 1400;    /* RX window size (controls garbage count) */

int tryzhdrtype = ZRINIT;    /* Header type to send corresponding to Last rx close */
time_t stop_time;

#ifdef _MSC_VER
void mycontinue() {
	int pid = GetCurrentProcessId();
	char buf[128];
	sprintf(buf, "lrz:pid=%d", pid);
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

int main(int argc, char *argv[]) {
    int exitcode = 0;

    //mycontinue();

    Rxtimeout = 100;

    from_cu();
    chkinvok(argv[0]);
	Zctlesc = 1;
	Verbose = 2;
    
    /* initialize zsendline tab */
    zsendline_init();

    if (Restricted && allow_remote_commands) {
        allow_remote_commands = FALSE;
    }
    if (Fromcu && !Quiet) {
        if (Verbose == 0)
            Verbose = 2;
    }

    vfile("%s %s\n", program_name, VERSION);
	fflush(stdout);
    io_mode(0, 1);
    readline_setup(0, HOWMANY, MAX_BLOCK * 2);

    if (wcreceive(0, NULL) == MYERROR) {
        exitcode = 0200;
        //canit(STDOUT_FILENO);
		canit(1);
    }
	fflush(stdout);
    io_mode(0, 0);
	if (exitcode && !zmodem_requested) {    
		/* bellow again with all thy might. */
		//canit(STDOUT_FILENO);
		canit(1);
	}

	vstringf("\r\n");
    if (exitcode)
		vstringf(_("Transfer incomplete\n"));
    else
		vstringf(_("Transfer complete\n"));
	//add sleep 1s for make sure the output string can show on screen completely.
    exit(exitcode);
}

/*
 * Let's receive something already.
 */

static int wcreceive(int argc, char **argp) {
    int c;
    struct zm_fileinfo zi;

    zi.fname = NULL;
    zi.modtime = 0;
    zi.mode = 0;
    zi.bytes_total = 0;
    zi.bytes_sent = 0;
    zi.bytes_received = 0;
    zi.bytes_skipped = 0;
    zi.eof_seen = 0;

    if (protocol != ZM_XMODEM || argc == 0) {
        Crcflg = 1;
        if (!Quiet)
            vstringf(_("%s waiting to receive."), program_name);
        if ((c = tryz()) != 0) {
            if (c == ZCOMPL)
                return OK;
            if (c == MYERROR)
                goto fubar;
            c = rzfiles(&zi);
            if (c)
                goto fubar;
        } else {
            for (;;) {
                if (Verbose > 1)
                    timing(1, NULL);
                if (wcrxpn(&zi, secbuf) == MYERROR)
                    goto fubar;
                if (secbuf[0] == 0)
                    return OK;
                if (procheader(secbuf, &zi) == MYERROR)
                    goto fubar;
                if (wcrx(&zi) == MYERROR)
                    goto fubar;

                if (Verbose > 1) {
                    double d;
                    long bps;
                    d = timing(0, NULL);
                    if (d == 0)
                        d = 0.5; /* can happen if timing uses time() */
                    bps = (zi.bytes_received - zi.bytes_skipped) / d;

                    if (Verbose > 1) {
                        vstringf(
                                _("\rBytes received: %7ld/%7ld\r\n"), (long) zi.bytes_received, (long) zi.bytes_total);
                    }
                }
            }
        }
    } else {
        char dummy[128];
        dummy[0] = '\0'; /* pre-ANSI HPUX cc demands this */
        dummy[1] = '\0'; /* procheader uses name + 1 + strlen(name) */
        zi.bytes_total = DEFBYTL;

        if (Verbose > 1)
            timing(1, NULL);
        procheader(dummy, &zi);

        if (Pathname)
            free(Pathname);
        errno = 0;
        Pathname = malloc(PATH_MAX + 1);
        if (!Pathname){
            vstringf(_("out of memory"));
        }

        strcpy(Pathname, *argp);
        checkpath(Pathname);

        vchar('\n');
        vstringf(_("%s: ready to receive %s"), program_name, Pathname);
        vstring("\r\n");

        if ((fout = fopen(Pathname, "wb")) == NULL) {
            return MYERROR;
        }
        if (wcrx(&zi) == MYERROR) {
            goto fubar;
        }
        if (Verbose > 1) {
            double d;
            long bps;
            d = timing(0, NULL);
            if (d == 0)
                d = 0.5; /* can happen if timing uses time() */
            bps = (zi.bytes_received - zi.bytes_skipped) / d;
            if (Verbose) {
                vstringf(
                        _("\rBytes received: %7ld\r\n"), (long) zi.bytes_received);
            }
        }
    }
    return OK;
fubar:
	//canit(STDOUT_FILENO); 
	canit(1);
    if (fout)
        fclose(fout);

    if (Restricted && Pathname) {
        unlink(Pathname);
        vstringf(_("\r\n%s: %s removed.\r\n"), program_name, Pathname);
    }
    return MYERROR;
}


/*
 * Fetch a pathname from the other end as a C ctyle ASCIZ string.
 * Length is indeterminate as long as less than Blklen
 * A null string represents no more files (YMODEM)
 */
static int
wcrxpn(struct zm_fileinfo *zi, char *rpn) {
    register int c;
    size_t Blklen = 0;        /* record length of received packets */

    READLINE_PF(1);
et_tu:
    Firstsec = TRUE;
    zi->eof_seen = FALSE;
    sendline(Crcflg ? WANTCRC : NAK);
    flushmo();
    purgeline(0); /* Do read next time ... */
    while ((c = wcgetsec(&Blklen, rpn, 100)) != 0) {
        if (c == WCEOT) {
            zperr(_("Pathname fetch returned EOT"));
            sendline(ACK);
            flushmo();
            purgeline(0);    /* Do read next time ... */
            READLINE_PF(1);
            goto et_tu;
        }
        return MYERROR;
    }
    sendline(ACK);
    flushmo();
    return OK;
}

/*
 * Adapted from CMODEM13.C, written by
 * Jack M. Wierda and Roderick W. Hart
 */
static int
wcrx(struct zm_fileinfo *zi) {
    register int sectnum, sectcurr;
    register char sendchar;
    size_t Blklen;

    Firstsec = TRUE;
    sectnum = 0;
    zi->eof_seen = FALSE;
    sendchar = Crcflg ? WANTCRC : NAK;

    for (;;) {
        sendline(sendchar);    /* send it now, we're ready! */
        flushmo();
        purgeline(0);    /* Do read next time ... */
        sectcurr = wcgetsec(&Blklen, secbuf,
                            (unsigned int) ((sectnum & 0177) ? 50 : 130));
        report(sectcurr);
        if (sectcurr == ((sectnum + 1) & 0377)) {
            sectnum++;
            /* if using xmodem we don't know how long a file is */
            if (zi->bytes_total && R_BYTESLEFT(zi) < Blklen)
                Blklen = R_BYTESLEFT(zi);
            zi->bytes_received += Blklen;
            if (putsec(zi, secbuf, Blklen) == MYERROR)
                return MYERROR;
            sendchar = ACK;
        } else if (sectcurr == (sectnum & 0377)) {
            zperr(_("Received dup Sector"));
            sendchar = ACK;
        } else if (sectcurr == WCEOT) {
            if (closeit(zi))
                return MYERROR;
            sendline(ACK);
            flushmo();
            purgeline(0);    /* Do read next time ... */
            return OK;
        } else if (sectcurr == MYERROR)
            return MYERROR;
        else {
            zperr(_("Sync Error"));
            return MYERROR;
        }
    }
}

/*
 * Wcgetsec fetches a Ward Christensen type sector.
 * Returns sector number encountered or ERROR if valid sector not received,
 * or CAN CAN received
 * or WCEOT if eot sector
 * time is timeout for first char, set to 4 seconds thereafter
 ***************** NO ACK IS SENT IF SECTOR IS RECEIVED OK **************
 *    (Caller must do that when he is good and ready to get next sector)
 */
static int
wcgetsec(size_t *Blklen, char *rxbuf, unsigned int maxtime) {
    register int checksum, wcj, firstch;
    register unsigned short oldcrc;
    register char *p;
    int sectcurr;

    for (Lastrx = errors = 0; errors < RETRYMAX; errors++) {
        if ((firstch = READLINE_PF(maxtime)) == STX) {
            *Blklen = 1024;
            goto get2;
        }
        if (firstch == SOH) {
            *Blklen = 128;
            get2:
            sectcurr = READLINE_PF(1);
            if ((sectcurr + (oldcrc = READLINE_PF(1))) == 0377) {
                oldcrc = checksum = 0;
                for (p = rxbuf, wcj = *Blklen; --wcj >= 0;) {
                    if ((firstch = READLINE_PF(1)) < 0)
                        goto bilge;
                    oldcrc = updcrc(firstch, oldcrc);
                    checksum += (*p++ = firstch);
                }
                if ((firstch = READLINE_PF(1)) < 0)
                    goto bilge;
                if (Crcflg) {
                    oldcrc = updcrc(firstch, oldcrc);
                    if ((firstch = READLINE_PF(1)) < 0)
                        goto bilge;
                    oldcrc = updcrc(firstch, oldcrc);
                    if (oldcrc & 0xFFFF)
                        zperr(_("CRC"));
                    else {
                        Firstsec = FALSE;
                        return sectcurr;
                    }
                } else if (((checksum - firstch) & 0377) == 0) {
                    Firstsec = FALSE;
                    return sectcurr;
                } else
                    zperr(_("Checksum"));
            } else
                zperr(_("Sector number garbled"));
        }
        /* make sure eot really is eot and not just mixmash */
		else if (firstch == EOT && READLINE_PF(1) == TIMEOUT) {
			return WCEOT;
		}

        else if (firstch == CAN) {
            if (Lastrx == CAN) {
                zperr(_("Sender Cancelled"));
                return MYERROR;
            } else {
                Lastrx = CAN;
                continue;
            }
        } else if (firstch == TIMEOUT) {
            if (Firstsec)
                goto humbug;
            bilge:
            zperr(_("TIMEOUT"));
        } else
            zperr(_("Got 0%o sector header"), firstch);

        humbug:
        Lastrx = 0;
        {
            int cnt = 1000;
            while (cnt-- && READLINE_PF(1) != TIMEOUT);
        }
        if (Firstsec) {
            sendline(Crcflg ? WANTCRC : NAK);
            flushmo();
            purgeline(0);    /* Do read next time ... */
        } else {
            maxtime = 40;
            sendline(NAK);
            flushmo();
            purgeline(0);    /* Do read next time ... */
        }
    }
    /* try to stop the bubble machine. */
	//canit(STDOUT_FILENO); 
	canit(1);
    return MYERROR;
}

#define ZCRC_DIFFERS (MYERROR+1)
#define ZCRC_EQUAL (MYERROR+2)

/*
 * do ZCRC-Check for open file f.
 * check at most check_bytes bytes (crash recovery). if 0 -> whole file.
 * remote file size is remote_bytes.
 */
static int
do_crc_check(FILE *f, size_t remote_bytes, size_t check_bytes) {
    struct stat st;
    unsigned long crc;
    unsigned long rcrc;
    size_t n;
    int c;
    int t1 = 0, t2 = 0;
    if (-1 == fstat(fileno(f), &st)) {
        return MYERROR;
    }
    if (check_bytes == 0 && ((size_t) st.st_size) != remote_bytes)
        return ZCRC_DIFFERS; /* shortcut */

    crc = 0xFFFFFFFFL;
    n = check_bytes;
    if (n == 0)
        n = st.st_size;
    while (n-- && ((c = getc(f)) != EOF))
        crc = UPDC32(c, crc);
    crc = ~crc;
    clearerr(f);  /* Clear EOF */
    fseek(f, 0L, 0);

    while (t1 < 3) {
        stohdr(check_bytes);
        zshhdr(ZCRC, Txhdr);
        while (t2 < 3) {
            size_t tmp;
            c = zgethdr(Rxhdr, 0, &tmp);
            rcrc = (unsigned long) tmp;
            switch (c) {
                default: /* ignore */
                    break;
                case ZFIN:
                    return MYERROR;
                case ZRINIT:
                    return MYERROR;
                case ZCAN:
                    if (Verbose)
                        vstringf(_("got ZCAN"));
                    return MYERROR;
                    break;
                case ZCRC:
                    if (crc != rcrc)
                        return ZCRC_DIFFERS;
                    return ZCRC_EQUAL;
                    break;
            }
        }
    }
    return MYERROR;
}

/*
 * Process incoming file information header
 */
static int
procheader(char *name, struct zm_fileinfo *zi) {
    const char *openmode;
    char *p;
    static char *name_static = NULL;
    char *nameend;

    if (name_static)
        free(name_static);
    if (junk_path) {
        p = strrchr(name, '/');
        if (p) {
            p++;
            if (!*p) {
                /* alert - file name ended in with a / */
                if (Verbose)
                    vstringf(_("file name ends with a /, skipped: %s\n"), name);
                return MYERROR;
            }
            name = p;
        }
    }
    name_static = malloc(strlen(name) + 1);
    if (!name_static){
        vstringf(_("out of memory"));
    }

    strcpy(name_static, name);
    zi->fname = name_static;

    if (Verbose > 2) {
        vstringf(_("zmanag=%d, Lzmanag=%d\n"), zmanag, Lzmanag);
        vstringf(_("zconv=%d\n"), zconv);
    }

    /* set default parameters and overrides */
    openmode = "wb";
    Thisbinary = (!Rxascii) || Rxbinary;
    if (Lzmanag)
        zmanag = Lzmanag;

    /*
     *  Process ZMODEM remote file management requests
     */
    if (!Rxbinary && zconv == ZCNL)    /* Remote ASCII override */
        Thisbinary = 0;
    if (zconv == ZCBIN)    /* Remote Binary override */
        Thisbinary = TRUE;
    if (Thisbinary && zconv == ZCBIN && try_resume)
        zconv = ZCRESUM;
    if (zmanag == ZF1_ZMAPND && zconv != ZCRESUM)
        openmode = "ab";
    if (skip_if_not_found)
        openmode = "r+b";

    zi->bytes_total = DEFBYTL;
    zi->mode = 0;
    zi->eof_seen = 0;
    zi->modtime = 0;

    nameend = name + 1 + strlen(name);
    if (*nameend) {    /* file coming from Unix or DOS system */
        long modtime;
        long bytes_total;
        int mode;
        sscanf(nameend, "%ld%lo%o", &bytes_total, &modtime, &mode);
        zi->modtime = modtime;
        zi->bytes_total = bytes_total;
        zi->mode = mode;
        if (zi->mode & UNIXFILE)
            ++Thisbinary;
    }

    /* Check for existing file */
    if (zconv != ZCRESUM && !Rxclob && (zmanag & ZF1_ZMMASK) != ZF1_ZMCLOB
        && (zmanag & ZF1_ZMMASK) != ZF1_ZMAPND
        && (fout = fopen(name, "rb"))) {
        struct stat sta;
        char *tmpname;
        char *ptr;
        int i;
        if (zmanag == ZF1_ZMNEW || zmanag == ZF1_ZMNEWL) {
            if (-1 == fstat(fileno(fout), &sta)) {
                if (Verbose)
                    vstringf(_("file exists, skipped: %s\n"), name);
                return MYERROR;
            }
            if (zmanag == ZF1_ZMNEW) {
                if (sta.st_mtime > zi->modtime) {
                    return MYERROR; /* skips file */
                }
            } else {
                /* newer-or-longer */
                if (((size_t) sta.st_size) >= zi->bytes_total
                    && sta.st_mtime > zi->modtime) {
                    return MYERROR; /* skips file */
                }
            }
            fclose(fout);
        } else if (zmanag == ZF1_ZMCRC) {
            int r = do_crc_check(fout, zi->bytes_total, 0);
            if (r == MYERROR) {
                fclose(fout);
                return MYERROR;
            }
            if (r != ZCRC_DIFFERS) {
                return MYERROR; /* skips */
            }
            fclose(fout);
        } else {
            size_t namelen;
            fclose(fout);
            if ((zmanag & ZF1_ZMMASK) != ZF1_ZMCHNG) {
                if (Verbose)
                    vstringf(_("file exists, skipped: %s\n"), name);
                return MYERROR;
            }
            /* try to rename */
            namelen = strlen(name);
            tmpname = malloc(namelen + 5);
            memcpy(tmpname, name, namelen);
            ptr = tmpname + namelen;
            *ptr++ = '.';
            i = 0;
            do {
                sprintf(ptr, "%d", i++);
            } while (i < 1000 && stat(tmpname, &sta) == 0);
            if (i == 1000)
                return MYERROR;
            free(name_static);
            name_static = malloc(strlen(tmpname) + 1);
            if (!name_static){
                vstringf(_("out of memory"));
            }

            strcpy(name_static, tmpname);
            zi->fname = name_static;
        }
    }

    if (!*nameend) {        /* File coming from CP/M system */
        for (p = name_static; *p; ++p)        /* change / to _ */
            if (*p == '/')
                *p = '_';

        if (*--p == '.')        /* zap trailing period */
            *p = 0;
    }

    if (!zmodem_requested && MakeLCPathname && !IsAnyLower(name_static)
        && !(zi->mode & UNIXFILE))
        uncaps(name_static);
    {
        if (protocol == ZM_XMODEM)
            /* we don't have the filename yet */
            return OK; /* dummy */
        if (Pathname)
            free(Pathname);
        Pathname = malloc((PATH_MAX) * 2);
        if (!Pathname){
            vstringf(_("out of memory"));
        }
        strcpy(Pathname, name_static);
        if (Verbose) {
            /* overwrite the "waiting to receive" line */
            vstring("\r                                                                     \r");
            vstringf(_("Receiving: %s\n"), name_static);
        }
        checkpath(name_static);
        if (Nflag) {
            /* cast because we might not have a prototyp for strdup :-/ */
            free(name_static);
            name_static = (char *) strdup("/dev/null");
            if (!name_static) {
                fprintf(stderr, "%s: %s\n", program_name, _("out of memory"));
                exit(1);
            }
        }

        if (Thisbinary && zconv == ZCRESUM) {
            struct stat st;
            fout = fopen(name_static, "r+b");
            if (fout && 0 == fstat(fileno(fout), &st)) {
                int can_resume = TRUE;
                if (zmanag == ZF1_ZMCRC) {
                    int r = do_crc_check(fout, zi->bytes_total, st.st_size);
                    if (r == MYERROR) {
                        fclose(fout);
                        return ZFERR;
                    }
                    if (r == ZCRC_DIFFERS) {
                        can_resume = FALSE;
                    }
                }
                if ((unsigned long) st.st_size > zi->bytes_total) {
                    can_resume = FALSE;
                }
                /* retransfer whole blocks */
                zi->bytes_skipped = st.st_size & ~(1023);
                if (can_resume) {
                    if (fseek(fout, (long) zi->bytes_skipped, SEEK_SET)) {
                        fclose(fout);
                        return ZFERR;
                    }
                } else
                    zi->bytes_skipped = 0; /* resume impossible, file has changed */
                goto buffer_it;
            }
            zi->bytes_skipped = 0;
            if (fout)
                fclose(fout);
        }
        fout = fopen(name_static, openmode);
#ifdef ENABLE_MKDIR
        if (!fout && Restricted < 2) {
            if (make_dirs(name_static))
                fout = fopen(name_static, openmode);
        }
#endif
        if (!fout) {
            zpfatal(_("cannot open %s"), name_static);
            return MYERROR;
        }
    }
buffer_it:
    zi->bytes_received = zi->bytes_skipped;

    return OK;
}

#ifdef ENABLE_MKDIR
/*
 *  Directory-creating routines from Public Domain TAR by John Gilmore
 */

/*
 * After a file/link/symlink/dir creation has failed, see if
 * it's because some required directory was not present, and if
 * so, create all required dirs.
 */
static int
make_dirs(char *pathname) {
    register char *p;        /* Points into path */
    int madeone = 0;        /* Did we do anything yet? */
    int save_errno = errno;        /* Remember caller's errno */

    if (errno != ENOENT)
        return 0;        /* Not our problem */

    for (p = strchr(pathname, '/'); p != NULL; p = strchr(p + 1, '/')) {
        /* Avoid mkdir of empty string, if leading or double '/' */
        if (p == pathname || p[-1] == '/')
            continue;
        /* Avoid mkdir where last part of path is '.' */
        if (p[-1] == '.' && (p == pathname + 1 || p[-2] == '/'))
            continue;
        *p = 0;                /* Truncate the path there */
        if (!mkdir(pathname, 0777)) {    /* Try to create it as a dir */
            vfile("Made directory %s\n", pathname);
            madeone++;        /* Remember if we made one */
            *p = '/';
            continue;
        }
        *p = '/';
        if (errno == EEXIST)        /* Directory already exists */
            continue;
        /*
         * Some other error in the mkdir.  We return to the caller.
         */
        break;
    }
    errno = save_errno;        /* Restore caller's errno */
    return madeone;            /* Tell them to retry if we made one */
}

#endif /* ENABLE_MKDIR */

/*
 * Putsec writes the n characters of buf to receive file fout.
 *  If not in binary mode, carriage returns, and all characters
 *  starting with CPMEOF are discarded.
 */
static int
putsec(struct zm_fileinfo *zi, char *buf, size_t n) {
    register char *p;

    if (n == 0)
        return OK;
    if (Thisbinary) {
        if (fwrite(buf, n, 1, fout) != 1)
            return MYERROR;
    } else {
        if (zi->eof_seen)
            return OK;
        for (p = buf; n > 0; ++p, n--) {
            if (*p == '\r')
                continue;
            if (*p == CPMEOF) {
                zi->eof_seen = TRUE;
                return OK;
            }
            putc(*p, fout);
        }
    }
    return OK;
}

/* make string s lower case */
static void
uncaps(char *s) {
    for (; *s; ++s)
        if (isupper((unsigned char) (*s)))
            *s = tolower(*s);
}

/*
 * IsAnyLower returns TRUE if string s has lower case letters.
 */
static int
IsAnyLower(const char *s) {
    for (; *s; ++s)
        if (islower((unsigned char) (*s)))
            return TRUE;
    return FALSE;
}

static void
report(int sct) {
    if (Verbose > 1) {
        vstringf(_("Blocks received: %d"), sct);
        vchar('\r');
    }
}

/*
 * If called as [-][dir/../]vrzCOMMAND set Verbose to 1
 * If called as [-][dir/../]rzCOMMAND set the pipe flag
 * If called as rb use YMODEM protocol
 */
static void
chkinvok(const char *s) {
    char *p = strrchr(s, '/');
    char *p2 = strrchr(s, '\\');
    if (p2 > p) {
        p = p2;
    }

    if (p != NULL) {
        p++;
    } else {
        p = s;
    }
    program_name = p;
    protocol = ZM_ZMODEM;
}

/*
 * Totalitarian Communist pathname processing
 */
static void
checkpath(const char *name) {
    if (Restricted) {
        const char *p;
        p = strrchr(name, '/');
        if (p)
            p++;
        else
            p = name;
        /* don't overwrite any file in very restricted mode.
         * don't overwrite hidden files in restricted mode */
        if ((Restricted == 2 || *name == '.') && fopen(name, "rb") != NULL) {
            //canit(STDOUT_FILENO);
			canit(1);
            vstring("\r\n");
            vstringf(_("%s: %s exists\n"), program_name, name);
        }
        /* restrict pathnames to current tree or uucppublic */
        if (strstr(name, "../")
#ifdef PUBDIR
            || (name[0]== '/' && strncmp(name, PUBDIR,
                strlen(PUBDIR)))
#endif
                ) {
			//canit(STDOUT_FILENO); 
			canit(1);
            vstring("\r\n");
            vstringf(_("%s:\tSecurity Violation"), program_name);
            vstring("\r\n");
        }
        if (Restricted > 1) {
            if (name[0] == '.' || strstr(name, "/.")) {
                //canit(STDOUT_FILENO);
				canit(1);
                vstring("\r\n");
                vstringf(_("%s:\tSecurity Violation"), program_name);
                vstring("\r\n");
            }
        }
    }
}

/*
 * Initialize for Zmodem receive attempt, try to activate Zmodem sender
 *  Handles ZSINIT frame
 *  Return ZFILE if Zmodem filename received, -1 on error,
 *   ZCOMPL if transaction finished,  else 0
 */
static int
tryz(void) {
    register int c, n;
    register int cmdzack1flg;
    int zrqinits_received = 0;
    size_t bytes_in_block = 0;

    if (protocol != ZM_ZMODEM)        /* Check for "rb" program name */
        return 0;

    for (n = zmodem_requested ? 15 : 5;
         (--n + zrqinits_received) >= 0 && zrqinits_received < 10;) {
        /* Set buffer length (0) and capability flags */
        stohdr(0L);
        Txhdr[ZF0] = CANFC32 | CANFDX | CANOVIO;
        if (Zctlesc)
            Txhdr[ZF0] |= TESCCTL; /* TESCCTL == ESCCTL */
        zshhdr(tryzhdrtype, Txhdr);

        if (tryzhdrtype == ZSKIP)    /* Don't skip too far */
            tryzhdrtype = ZRINIT;    /* CAF 8-21-87 */
again:
        switch (zgethdr(Rxhdr, 0, NULL)) {
            case ZRQINIT:
                /* getting one ZRQINIT is totally ok. Normally a ZFILE follows
                 * (and might be in our buffer, so don't purge it). But if we
                 * get more ZRQINITs than the sender has started up before us
                 * and sent ZRQINITs while waiting.
                 */
                zrqinits_received++;
                continue;

            case ZEOF:
                continue;
            case TIMEOUT:
                continue;
            case ZFILE:
                zconv = Rxhdr[ZF0];
                if (!zconv)
                    /* resume with sz -r is impossible (at least with unix sz)
                     * if this is not set */
                    zconv = ZCBIN;
                if (Rxhdr[ZF1] & ZF1_ZMSKNOLOC) {
                    Rxhdr[ZF1] &= ~(ZF1_ZMSKNOLOC);
                    skip_if_not_found = TRUE;
                }
                zmanag = Rxhdr[ZF1];
                ztrans = Rxhdr[ZF2];
                tryzhdrtype = ZRINIT;
                c = zrdata(secbuf, MAX_BLOCK, &bytes_in_block);
                io_mode(0, 3);
                if (c == GOTCRCW)
                    return ZFILE;
                zshhdr(ZNAK, Txhdr);
                goto again;
            case ZSINIT:
                /* this once was:
                 * Zctlesc = TESCCTL & Rxhdr[ZF0];
                 * trouble: if rz get --escape flag:
                 * - it sends TESCCTL to sz,
                 *   get a ZSINIT _without_ TESCCTL (yeah - sender didn't know),
                 *   overwrites Zctlesc flag ...
                 * - sender receives TESCCTL and uses "|=..."
                 * so: sz escapes, but rz doesn't unescape ... not good.
                 */
                Zctlesc |= TESCCTL & Rxhdr[ZF0];
                if (zrdata(Attn, ZATTNLEN, &bytes_in_block) == GOTCRCW) {
                    stohdr(1L);
                    zshhdr(ZACK, Txhdr);
                    goto again;
                }
                zshhdr(ZNAK, Txhdr);
                goto again;
            case ZFREECNT:
                stohdr(getfree());
                zshhdr(ZACK, Txhdr);
                goto again;
            case ZCOMMAND:
                cmdzack1flg = Rxhdr[ZF0];
                if (zrdata(secbuf, MAX_BLOCK, &bytes_in_block) == GOTCRCW) {
                    if (Verbose) {
                        vstringf("%s: %s\n", program_name,
                                 _("remote command execution requested"));
                        vstringf("%s: %s\n", program_name, secbuf);
                    }
                    if (!allow_remote_commands) {
                        if (Verbose)
                            vstringf("%s: %s\n", program_name,
                                     _("not executed"));
                        zshhdr(ZCOMPL, Txhdr);
                        return ZCOMPL;
                    }
                    if (cmdzack1flg & ZCACK1)
                        stohdr(0L);
                    else
                        stohdr((size_t) sys2(secbuf));
                    purgeline(0);    /* dump impatient questions */
                    do {
                        zshhdr(ZCOMPL, Txhdr);
                    } while (++errors < 20 && zgethdr(Rxhdr, 1, NULL) != ZFIN);
                    ackbibi();
                    if (cmdzack1flg & ZCACK1)
                        exec2(secbuf);
                    return ZCOMPL;
                }
                zshhdr(ZNAK, Txhdr);
                goto again;
            case ZCOMPL:
                goto again;
            default:
                continue;
            case ZFIN:
                ackbibi();
                return ZCOMPL;
            case ZRINIT:
                if (Verbose)
                    vstringf(_("got ZRINIT"));
                return MYERROR;
            case ZCAN:
                if (Verbose)
                    vstringf(_("got ZCAN"));
                return MYERROR;
        }
    }
    return 0;
}


/*
 * Receive 1 or more files with ZMODEM protocol
 */
static int
rzfiles(struct zm_fileinfo *zi) {
    register int c;

    for (;;) {
        timing(1, NULL);
        c = rzfile(zi);
        switch (c) {
            case ZEOF:
                if (Verbose > 1) {
                    double d;
                    long bps;
                    d = timing(0, NULL);
                    if (d == 0)
                        d = 0.5; /* can happen if timing uses time() */
                    bps = (zi->bytes_received - zi->bytes_skipped) / d;
                    if (Verbose > 1) {
                        vstringf(
                                _("\rBytes received: %7ld/%7ld\r\n"), (long) zi->bytes_received, (long) zi->bytes_total);
                    }
                }
                /* FALL THROUGH */
            case ZSKIP:
                if (c == ZSKIP) {
                    if (Verbose)
                        vstringf(_("Skipped\r\n"));
                }
                switch (tryz()) {
                    case ZCOMPL:
                        return OK;
                    default:
                        return MYERROR;
                    case ZFILE:
                        break;
                }
                continue;
            default:
                return c;
            case MYERROR:
                return MYERROR;
        }
    }
}

/* "OOSB" means Out Of Sync Block. I once thought that if sz sents
 * blocks a,b,c,d, of which a is ok, b fails, we might want to save 
 * c and d. But, alas, i never saw c and d.
 */
#define SAVE_OOSB
#ifdef SAVE_OOSB
typedef struct oosb_t {
    size_t pos;
    size_t len;
    char *data;
    struct oosb_t *next;
} oosb_t;
struct oosb_t *anker = NULL;
#endif

/*
 * Receive a file with ZMODEM protocol
 *  Assumes file name frame is in secbuf
 */
static int
rzfile(struct zm_fileinfo *zi) {
    register int c, n;
    long last_rxbytes = 0;
    unsigned long last_bps = 0;
    long not_printed = 0;
    time_t low_bps = 0;
    size_t bytes_in_block = 0;

    zi->eof_seen = FALSE;

    n = 20;

    if (procheader(secbuf, zi) == MYERROR) {
        return (tryzhdrtype = ZSKIP);
    }

    for (;;) {
        stohdr(zi->bytes_received);
        zshhdr(ZRPOS, Txhdr);
        goto skip_oosb;
nxthdr:
#ifdef SAVE_OOSB
        if (anker) {
            oosb_t *akt, *last, *next;
            for (akt = anker, last = NULL; akt; last = akt ? akt : last, akt = next) {
                if (akt->pos == zi->bytes_received) {
                    putsec(zi, akt->data, akt->len);
                    zi->bytes_received += akt->len;
                    vfile("using saved out-of-sync-paket %lx, len %ld",
                          akt->pos, akt->len);
                    goto nxthdr;
                }
                next = akt->next;
                if (akt->pos < zi->bytes_received) {
                    vfile("removing unneeded saved out-of-sync-paket %lx, len %ld",
                          akt->pos, akt->len);
                    if (last)
                        last->next = akt->next;
                    else
                        anker = akt->next;
                    free(akt->data);
                    free(akt);
                    akt = NULL;
                }
            }
        }
#endif
skip_oosb:
        c = zgethdr(Rxhdr, 0, NULL);
        switch (c) {
            default:
                vfile("rzfile: zgethdr returned %d", c);
                return MYERROR;
            case ZNAK:
            case TIMEOUT:
                if (--n < 0) {
                    vfile("rzfile: zgethdr returned %d", c);
                    return MYERROR;
                }
            case ZFILE:
                zrdata(secbuf, MAX_BLOCK, &bytes_in_block);
                continue;
            case ZEOF:
                if (rclhdr(Rxhdr) != (long) zi->bytes_received) {
                    /*
                     * Ignore eof if it's at wrong place - force
                     *  a timeout because the eof might have gone
                     *  out before we sent our zrpos.
                     */
                    errors = 0;
                    goto nxthdr;
                }
                if (closeit(zi)) {
                    tryzhdrtype = ZFERR;
                    vfile("rzfile: closeit returned <> 0");
                    return MYERROR;
                }
                vfile("rzfile: normal EOF");
                return c;
            case MYERROR:    /* Too much garbage in header search error */
                if (--n < 0) {
                    vfile("rzfile: zgethdr returned %d", c);
                    return MYERROR;
                }
                zmputs(Attn);
                continue;
            case ZSKIP:
                closeit(zi);
                vfile("rzfile: Sender SKIPPED file");
                return c;
            case ZDATA:
                if (rclhdr(Rxhdr) != (long) zi->bytes_received) {
#if defined(SAVE_OOSB)
                    oosb_t *neu;
                    size_t pos = rclhdr(Rxhdr);
#endif
                    if (--n < 0) {
                        vfile("rzfile: out of sync");
                        return MYERROR;
                    }
#if defined(SAVE_OOSB)
                    switch (c = zrdata(secbuf, MAX_BLOCK, &bytes_in_block)) {
                        case GOTCRCW:
                        case GOTCRCG:
                        case GOTCRCE:
                        case GOTCRCQ:
                            if (pos > zi->bytes_received) {
                                neu = malloc(sizeof(oosb_t));
                                if (neu)
                                    neu->data = malloc(bytes_in_block);
                                if (neu && neu->data) {
                                    vfile("saving out-of-sync-block %lx, len %lu", pos,
                                          (unsigned long) bytes_in_block);
                                    memcpy(neu->data, secbuf, bytes_in_block);
                                    neu->pos = pos;
                                    neu->len = bytes_in_block;
                                    neu->next = anker;
                                    anker = neu;
                                } else if (neu)
                                    free(neu);
                            }
                    }
#endif
                    zmputs(Attn);
                    continue;
                }
            moredata:
                if ((Verbose > 1 || min_bps || stop_time)
                    && (not_printed > (min_bps ? 3 : 7)
                        || zi->bytes_received > last_bps / 2 + last_rxbytes)) {
                    int minleft = 0;
                    int secleft = 0;
                    time_t now;
                    double d;
                    d = timing(0, &now);
                    if (d == 0)
                        d = 0.5; /* timing() might use time() */
                    last_bps = zi->bytes_received / d;
                    if (last_bps > 0) {
                        minleft = (R_BYTESLEFT(zi)) / last_bps / 60;
                        secleft = ((R_BYTESLEFT(zi)) / last_bps) % 60;
                    }
                    if (min_bps) {
                        if (low_bps) {
                            if (last_bps < min_bps) {
                                if (now - low_bps >= min_bps_time) {
                                    /* too bad */
                                    vfile(_("rzfile: bps rate %ld below min %ld"),
                                          last_bps, min_bps);
                                    return MYERROR;
                                }
                            } else
                                low_bps = 0;
                        } else if (last_bps < min_bps) {
                            low_bps = now;
                        }
                    }
                    if (stop_time && now >= stop_time) {
                        /* too bad */
                        vfile(_("rzfile: reached stop time"));
                        return MYERROR;
                    }

                    if (Verbose > 1) {
                        vstringf(_("\rBytes received: %7ld/%7ld"), (long) zi->bytes_received, (long) zi->bytes_total);
                        last_rxbytes = zi->bytes_received;
                        not_printed = 0;
                    }
                } else if (Verbose)
                    not_printed++;
                switch (c = zrdata(secbuf, MAX_BLOCK, &bytes_in_block))
                {
                    case ZCAN:
                        vfile("rzfile: zrdata returned %d", c);
                        return MYERROR;
                    case MYERROR:    /* CRC error */
                        if (--n < 0) {
                            vfile("rzfile: zgethdr returned %d", c);
                            return MYERROR;
                        }
                        zmputs(Attn);
                        continue;
                    case TIMEOUT:
                        if (--n < 0) {
                            vfile("rzfile: zgethdr returned %d", c);
                            return MYERROR;
                        }
                        continue;
                    case GOTCRCW:
                        n = 20;
                        putsec(zi, secbuf, bytes_in_block);
                        zi->bytes_received += bytes_in_block;
                        stohdr(zi->bytes_received);
                        zshhdr(ZACK | 0x80, Txhdr);
                        goto nxthdr;
                    case GOTCRCQ:
                        n = 20;
                        putsec(zi, secbuf, bytes_in_block);
                        zi->bytes_received += bytes_in_block;
                        stohdr(zi->bytes_received);
                        zshhdr(ZACK, Txhdr);
                        goto moredata;
                    case GOTCRCG:
                        n = 20;
                        putsec(zi, secbuf, bytes_in_block);
                        zi->bytes_received += bytes_in_block;
                        goto moredata;
                    case GOTCRCE:
                        n = 20;
                        putsec(zi, secbuf, bytes_in_block);
                        zi->bytes_received += bytes_in_block;
                        goto nxthdr;
                }
        }
    }
}

/*
 * Send a string to the modem, processing for \336 (sleep 1 sec)
 *   and \335 (break signal)
 */
static void
zmputs(const char *s) {
    const char *p;

    while (s && *s) {
        p = strpbrk(s, "\335\336");
        if (!p) {
            write(1, s, strlen(s));
            return;
        }
        if (p != s) {
            write(1, s, (size_t) (p - s));
            s = p;
        }
        if (*p == '\336')
            Sleep(1);
        else
            sendbrk(0);
        p++;
    }
}

/*
 * Close the receive dataset, return OK or ERROR
 */
static int
closeit(struct zm_fileinfo *zi) {
    int ret;

    ret = fclose(fout);
    if (ret) {
        zpfatal(_("file close error"));
        /* this may be any sort of error, including random data corruption */

        unlink(Pathname);
        return MYERROR;
    }
    if (zi->modtime) {
        time_t timep[2];
        timep[0] = time(NULL);
        timep[1] = zi->modtime;
        utime(Pathname, timep);
    }
#ifdef S_ISREG
    if (S_ISREG(zi->mode)) {
#else
        if ((zi->mode&S_IFMT) == S_IFREG) {
#endif
        /* we must not make this program executable if running
         * under rsh, because the user might have uploaded an
         * unrestricted shell.
         */
        if (under_rsh)
            chmod(Pathname, (00666 & zi->mode));
        else
            chmod(Pathname, (07777 & zi->mode));
    }
    return OK;
}

/*
 * Ack a ZFIN packet, let byegones be byegones
 */
static void
ackbibi(void) {
    int n;

    vfile("ackbibi:");
    Readnum = 1;
    stohdr(0L);
    for (n = 3; --n >= 0;) {
        purgeline(0);
        zshhdr(ZFIN, Txhdr);
        switch (READLINE_PF(100)) {
            case 'O':
                READLINE_PF(1);    /* Discard 2nd 'O' */
                vfile("ackbibi complete");
                return;
            case RCDO:
                return;
            case TIMEOUT:
            default:
                break;
        }
    }
}

/*
 * Strip leading ! if present, do shell escape. 
 */
static int
sys2(const char *s) {
    if (*s == '!')
        ++s;
    return system(s);
}

/*
 * Strip leading ! if present, do exec.
 */
static void
exec2(const char *s) {
    if (*s == '!')
        ++s;
    io_mode(0, 0);
    execl("/bin/sh", "sh", "-c", s);
    zpfatal("execl");
    exit(1);
}

/*
 * Routine to calculate the free bytes on the current file system
 *  ~0 means many free bytes (unknown)
 */
static size_t
getfree(void) {
    return ((size_t) (~0L));    /* many free bytes ... */
}

/* End of lrz.c */
