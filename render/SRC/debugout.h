#ifndef DEBUGOUT_H
#define DEBUGOUT_H

#include <stdio.h>
#include <stdarg.h>
#include <windows.h>

inline void dbg_print(const char *msg, ...)
{
#ifdef _DEBUG
	char buf[256];
	
	va_list ap;
	va_start(ap, msg); // use variable arg list
	vsprintf(buf, msg, ap);
	va_end( ap );
	
	OutputDebugString(buf);
#endif
}

#endif // DEBUGOUT_H
