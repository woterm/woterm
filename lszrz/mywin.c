//
// Created by abc on 1/1/19.
//
#include "mywin.h"

//#define DEBUGZ

FILE *logFile() {
#ifdef DEBUGZ
	static FILE *dbgf = NULL;
	if (dbgf == NULL) {
#ifdef _MSC_VER
		dbgf = fopen("d:\\debugz_win32.txt", "w+b");
#else
		dbgf = fopen("/home/abc/debugz_linux.txt", "w+b");
#endif // _MSC_VER
	}
	return dbgf;
#else
	return stderr;
#endif //DEBUGZ
}

int my_vprintf(int level, char *fmt, ...) {
	if(Verbose >= level) {
		char buf[512];
		va_list argptr;
		int cnt;
		va_start(argptr, fmt);
		cnt = vsnprintf(buf, 512, fmt, argptr);
		va_end(argptr);
		//if (strcmp(buf, "zsdat32: 40 ZCRCW") == 0) {
		//	int ii = 0;
		//}
		//strcat(buf, "\r\n");
		fputs(buf, logFile());
		return cnt;
	}
	return 0;
}

int my_vstringf(char *fmt, ...) {
	char buf[512];
	va_list argptr;
	int cnt;
	va_start(argptr, fmt);
	cnt = vsnprintf(buf, 512 ,fmt, argptr);
	va_end(argptr);
	//strcat(buf, "\r\n");
	fputs(buf, logFile());
	return cnt;
}

void my_vchar(char c) {
	if (c == '0x7a') {
		int ii = 0;
	}
	fputc(c, logFile());
}

void my_vstring(char *sz) {
	fputs(sz, logFile());
}