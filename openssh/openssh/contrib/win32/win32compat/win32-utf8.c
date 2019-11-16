/* 
 * Windows versions of functions implemented in utf8.c
 */
#include <stdio.h>
#include <stdarg.h>
#include <Windows.h>

#include "console.h"


int
vfmprintf(FILE *stream, const char *fmt, va_list ap)
{
	int ret;
	ret = vfprintf(stream, fmt, ap);
	return ret;
}

int
mprintf(const char *fmt, ...)
{
	int ret = 0;
	va_list ap;
	va_start(ap, fmt);
	ret = vfmprintf(stdout, fmt, ap);
	va_end(ap);
	return ret;
}

int
fmprintf(FILE *stream, const char *fmt, ...)
{
	int ret = 0;
	va_list ap;
	va_start(ap, fmt);
	ret = vfmprintf(stream, fmt, ap);
	va_end(ap);
	return ret;
}

int
snmprintf(char *buf, size_t len, int *written, const char *fmt, ...)
{
	int ret;
	va_list valist;
	va_start(valist, fmt);
	ret = vsnprintf_s(buf, len, _TRUNCATE, fmt, valist);		
	va_end(valist);
	if (written != NULL && ret != -1)
		*written = ret;
	return ret;
}

void
msetlocale(void)
{
}

